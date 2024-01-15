#include <fstream>
#include <mutex>
#include <sys/mman.h>

#include "SimdCrack.hpp"
#include "SharedRefptr.hpp"
#include "Util.hpp"
#include "WordGenerator.hpp"

SimdCrack::~SimdCrack(
    void
)
{
    if (m_BinaryFd != nullptr)
    {
        free(m_Targets);
    }
    else
    {
        munmap(m_Targets, m_TargetsCount * m_HashWidth);
        fclose(m_BinaryFd);
    }
}

void
SimdCrack::InitAndRun(
    void
)
{
    if (m_Threads == 0)
    {
        m_Threads = std::thread::hardware_concurrency();
    }

    if (m_Algorithm == Algorithm::sha256)
    {
        m_HashWidth = SHA256_SIZE;
    }
    else if (m_Algorithm == Algorithm::sha1)
    {
        m_HashWidth = SHA1_SIZE;
    }

    if(!ProcessHashList())
    {
        return;
    }

    m_DispatchPool = dispatch::CreateDispatchPool("pool", m_Threads);

    std::cerr << "Starting cracking using " << m_Threads << " threads" << std::endl;

    for (size_t i = 0; i < m_Threads; i++)
    {
        m_DispatchPool->PostTask(
            dispatch::bind(
                &SimdCrack::GenerateBlocks,
                this,
                i + 1,
                m_Threads
            )
        );
    }
}

int comparator(
    const void* x,
    const void* y,
    void* Width
)
{
    return memcmp(x, y, (size_t)Width);
}

inline bool
SimdCrack::AddHashToList(
    const uint8_t* Hash
)
{
    if (m_TargetsCount == m_TargetsAllocated)
    {
        m_TargetsAllocated += 1024;
        m_Targets = (uint8_t*)realloc(m_Targets, m_TargetsAllocated * m_HashWidth);
        if (m_Targets == NULL)
        {
            std::cerr << "Not enough memory to allocate hash targets!" << std::endl;
            return false;
        }
    }

    uint8_t* next = m_Targets + (m_TargetsCount * m_HashWidth);
    memcpy(next, Hash, m_HashWidth);
    m_TargetsCount++;
    return true;
}

bool
SimdCrack::AddHashToList(
    const std::string Hash
)
{
    auto nexthash = Util::ParseHex(Hash);
    return AddHashToList(&nexthash[0]);
}

bool
SimdCrack::ProcessHashList(
    void
)
{
    m_TargetsAllocated = 1024;
    m_TargetsCount = 0;
    m_Targets = (uint8_t*)malloc(1024 * m_HashWidth);

    if (!m_HashList.empty())
    {
        std::cerr << "Processing hash list" << std::endl;

        std::ifstream infile(m_HashList);
        std::string line;
        while (std::getline(infile, line))
        {
            if (line.size() != m_HashWidth * 2)
            {
                std::cerr << "Invalid hash found, ignoring: " << line << std::endl;
                continue;
            }

            if(!AddHashToList(std::move(line)))
            {
                return false;
            }
        }
    }
    else if (!m_BinaryHashList.empty())
    {
        size_t len = std::filesystem::file_size(m_BinaryHashList);
        m_TargetsCount = len / m_HashWidth;
        m_BinaryFd = fopen(m_BinaryHashList.c_str(), "rb");
        m_Targets = (uint8_t*)mmap(nullptr, len, PROT_READ|PROT_WRITE, MAP_PRIVATE, fileno(m_BinaryFd), 0);
        if (m_Targets == MAP_FAILED)
        {
            std::cerr << "Error mapping hash list file" << std::endl;
            return false;
        }
    }
    else
    {
        for (auto& target : m_Target)
        {
            if (!AddHashToList(&target[0]))
            {
                return false;
            }
        }
    }

    if (m_BinaryHashList.empty())
    {
        qsort_r(m_Targets, m_TargetsCount, m_HashWidth, comparator, (void*)m_HashWidth);
    }

    return true;
}

void
SimdCrack::SetAlgorithm(
    const Algorithm Algo
)
{
    m_Algorithm = Algo;
}

void
SimdCrack::FoundResult(
    const std::vector<uint8_t> Hash,
    const std::string Result
)
{
    assert(dispatch::CurrentDispatcher() == dispatch::GetDispatcher("main").get());

    //
    // Output it
    //
    for (size_t i = 0; i < Hash.size(); i++)
    {
        printf("%02x", Hash[i]);
    }
    std::cout << " " << Result << std::endl;
    m_Found++;

    //
    // Check if we have found all
    // targets
    //
    if (m_Found == m_TargetsCount)
    {
        //
        // Stop the pool
        //
        m_DispatchPool->Stop();
        m_DispatchPool->Wait();

        //
        // Stop the current (main) dispatcher
        //
        dispatch::CurrentDispatcher()->Stop();
    }
}

void
SimdCrack::BlockProcessed(
    const std::vector<uint8_t> Hash,
    const std::string Result
)
{
    dispatch::PostTaskToDispatcher(
        "main",
        dispatch::bind(
            &SimdCrack::FoundResult,
            this,
            std::move(Hash),
            Result
        )
    );
}

void
SimdCrack::ProcessContext(
    PreimageContext* Context
)
{
    Context->CheckAndHandle(
        dispatch::bindf<ResultHandler>(
            &SimdCrack::BlockProcessed,
            this,
            std::placeholders::_1,
            std::placeholders::_2
        )
    );
}

void
SimdCrack::GenerateBlock(
    PreimageContext* Context,
    const size_t Start,
    const size_t Step,
    size_t* Next
)
{
    size_t index = Start;
    size_t wordSize = 0;
    std::string word;

    word = m_Generator.Generate(index);
    wordSize = word.size();
    Context->Initialize(wordSize);

    do
    {
        index += Step;
        Context->AddEntry(word);
        word = m_Generator.Generate(index);
    } while (!Context->IsFull() && word.size() == wordSize);

    *Next = index;
}

void
SimdCrack::GenerateBlocks(
    const size_t Start,
    const size_t Step
)
{
    std::string word;
    PreimageContext ctx(m_Algorithm, m_Targets, m_TargetsCount);
    size_t index;

    index = Start;

    //
    // Generate _count_ blocks (contexts)
    // and hash and check them
    //
    for (
        size_t counter = 0;
        counter < m_Blocksize;
        counter++
    )
    {
        GenerateBlock(
            &ctx,
            index,
            Step,
            &index
        );

        ProcessContext(
            &ctx
        );
    }

    dispatch::PostTaskFast(
        dispatch::bind(
            &SimdCrack::GenerateBlocks,
            this,
            index,
            Step
        )
    );
}