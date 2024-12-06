#include <algorithm>
#include <chrono>
#include <fstream>
#include <inttypes.h>
#include <gmp.h>
#include <mutex>
#include <sys/mman.h>

#include "SimdCrack.hpp"
#include "SimdHashBuffer.hpp"
#include "SharedRefptr.hpp"
#include "Util.hpp"
#include "WordGenerator.hpp"

#include "simdhash.h"

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

    if(!ProcessHashList())
    {
        return;
    }

    //
    // Open the output file handle
    //
    if(!m_Outfile.empty())
    {
        m_OutfileStream.open(m_Outfile, std::ofstream::out);
    }

    m_DispatchPool = dispatch::CreateDispatchPool("pool", m_Threads);

    std::cerr << "Starting cracking using " << m_Threads << " threads" << std::endl;

    for (size_t i = 0; i < m_Threads; i++)
    {
        mpz_class start(m_Resume + i + 1);

        m_DispatchPool->PostTask(
            dispatch::bind(
                &SimdCrack::GenerateBlocks,
                this,
                i,
                std::move(start),
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

#ifdef __APPLE__
int
Compare(
    void* Length,
    const void* Value1,
    const void* Value2
)
{
    return memcmp(Value1, Value2, (size_t)Length);
}
#endif

bool
SimdCrack::ProcessHashList(
    void
)
{
    m_TargetsAllocated = 1024;
    m_TargetsCount = 0;
    m_Targets = (uint8_t*)malloc(1024 * m_HashWidth);

    if (!m_HashFile.empty())
    {
        std::cerr << "Processing hash list" << std::endl;

        std::ifstream infile(m_HashFile);
        std::string line;
        while (std::getline(infile, line))
        {
            if (line.size() != m_HashWidth * 2)
            {
                std::cerr << "Invalid hash found, ignoring " << line.size() << "!=" << m_HashWidth*2 << ": \"" << line << "\"" << std::endl;
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
        m_TargetsSize = std::filesystem::file_size(m_BinaryHashList);
        m_TargetsCount = m_TargetsSize / m_HashWidth;
        m_BinaryFd = fopen(m_BinaryHashList.c_str(), "rb");
        m_Targets = (uint8_t*)mmap(nullptr, m_TargetsSize, PROT_READ, MAP_SHARED, fileno(m_BinaryFd), 0);
        if (m_Targets == MAP_FAILED)
        {
            std::cerr << "Error mapping hash list file" << std::endl;
            return false;
        }
        // Inform the kernel that we will be performing random accesses at all offsets
        auto ret = madvise(m_Targets, m_TargetsSize, MADV_RANDOM | MADV_WILLNEED);
        if (ret != 0)
        {
            std::cerr << "Madvise not happy" << std::endl;
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
        std::cerr << "Sorting hashes" << std::endl;
#ifdef __APPLE__
        qsort_r(m_Targets, m_TargetsCount, m_HashWidth, (void*)m_HashWidth, Compare);
#else
        qsort_r(m_Targets, m_TargetsCount, m_HashWidth, (__compar_d_fn_t)memcmp, (void*)m_HashWidth);
#endif
    }

    m_HashList.Initialize(
        m_Targets,
        m_TargetsSize,
        m_HashWidth,
        false
    );

    return true;
}

void
SimdCrack::FoundResults(
    std::vector<std::tuple<std::string, std::string>> Results
)
{
    assert(dispatch::CurrentDispatcher() == dispatch::GetDispatcher("main").get());

    m_Found += Results.size();

    // Output them
    for (auto& [h, w] : Results){
        if (m_OutfileStream.is_open())
        {
            m_OutfileStream << h << m_Separator << w << std::endl;
        }
        else
        {
            std::cout << h << m_Separator << w << std::endl;
        }
    }

    // Check if we have found all targets
    if (m_Found == m_TargetsCount)
    {
        // Stop the pool
        m_DispatchPool->Stop();
        m_DispatchPool->Wait();

        // Stop the current (main) dispatcher
        dispatch::CurrentDispatcher()->Stop();
    }
}

void
SimdCrack::GenerateBlocks(
    const size_t ThreadId,
    const mpz_class Start,
    const size_t Step
)
{
    mpz_class index(Start);
    SimdHashBufferFixed<MAX_OPTIMIZED_BUFFER_SIZE> words;
    std::array<uint8_t, MAX_HASH_SIZE * MAX_LANES> hashes;
    std::vector<std::tuple<std::string, std::string>> results;

    auto start = std::chrono::system_clock::now();

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
        for (size_t i = 0; i < SimdLanes(); i++)
        {
            const size_t length = m_Generator.Generate((char*)words[i], MAX_OPTIMIZED_BUFFER_SIZE, index);
            words.SetLength(i, length);
            index += Step;
        }

        SimdHashOptimized(
            m_Algorithm,
            words.GetLengths(),
            words.ConstBuffers(),
            &hashes[0]
        );
        
        for (size_t i = 0; i < SimdLanes(); i++)
        {
            const uint8_t* hash = &hashes[i * m_HashWidth];
            if (m_HashList.Lookup(hash))
            {
                results.push_back({
                    Util::ToHex(hash, m_HashWidth),
                    words.GetString(i)
                });
            }
        }

        if (!results.empty())
        {
            dispatch::PostTaskToDispatcher(
                "main",
                dispatch::bind(
                    &SimdCrack::FoundResults,
                    this,
                    std::move(results)
                )
            );
            results.clear();
        }
    }

    auto end = std::chrono::system_clock::now();
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    

    dispatch::PostTaskToDispatcher(
        "main",
        dispatch::bind(
            &SimdCrack::ThreadPulse,
            this,
            ThreadId,
            elapsed_ms.count(),
            words.GetString(SimdLanes() - 1)
        )
    );

    dispatch::PostTaskFast(
        dispatch::bind(
            &SimdCrack::GenerateBlocks,
            this,
            ThreadId,
            index,
            Step
        )
    );
}

void
SimdCrack::ThreadPulse(
    const size_t ThreadId,
    const uint64_t BlockTime,
    const std::string Last
)
{
    char statusbuf[96];

    assert(dispatch::CurrentDispatcher() == dispatch::GetDispatcher("main").get());

    m_BlocksCompleted++;
    m_LastBlockMs[ThreadId] = BlockTime;

    auto lastIndex = m_Generator.ParseReversed(Last);
    // Get the upper and lower bound for this current length
    auto lower = m_Generator.WordLengthIndex(Last.size());
    auto upper = m_Generator.WordLengthIndex(Last.size() + 1);
    mpz_class diff = (lastIndex - lower) / 1000000;
    mpz_class outof = (upper - lower) / 1000000;
    mpz_class percent = (diff / outof) * 100;

    if (!m_Outfile.empty())
    {
        uint64_t averageMs = 0;
        for (auto const& [thread, val] : m_LastBlockMs)
        {
            averageMs += val;
        }
        averageMs /= m_Threads;
        
        double hashesPerSec = 1000.f * ((m_Blocksize * SimdLanes()) / averageMs);
        char multiplechar = ' ';
        if (hashesPerSec > 1000000000.f)
        {
            hashesPerSec /= 1000000000.f;
            multiplechar = 'b';
        }
        else if (hashesPerSec > 1000000.f)
        {
            hashesPerSec /= 1000000.f;
            multiplechar = 'm';
        }
        else if (hashesPerSec > 1000.f)
        {
            hashesPerSec /= 1000.f;
            multiplechar = 'k';
        }

        statusbuf[sizeof(statusbuf) - 1] = '\0';
        memset(statusbuf, '\b', sizeof(statusbuf) - 1);
        fprintf(stderr, "%s", statusbuf);
        memset(statusbuf, ' ', sizeof(statusbuf) - 1);
        int count = snprintf(
            statusbuf, sizeof(statusbuf),
            "H/s:%.1lf%c C:%zu/%zu L:\"%s\" C:\"%s\" #:%sM/%sM (%s%%)",
                hashesPerSec,
                multiplechar,
                m_Found,
                m_TargetsCount,
                m_LastWord.c_str(),
                Last.c_str(),
                diff.get_str().c_str(),
                outof.get_str().c_str(),
                percent.get_str().c_str()
        );
        if (count < sizeof(statusbuf) - 1)
        {
            statusbuf[count] = ' ';
        }
        fprintf(stderr, "%s", statusbuf);
    }
}