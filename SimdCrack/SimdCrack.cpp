#include <mutex>
#include "SimdCrack.hpp"
#include "SharedRefptr.hpp"
#include "WordGenerator.hpp"

void
SimdCrack::InitAndRun(
    void
)
{
    if (m_Threads == 0)
    {
        m_Threads = std::thread::hardware_concurrency();
    }

    m_DispatchPool = dispatch::CreateDispatchPool("pool", m_Threads);

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
    std::cout << ": " << Result << std::endl;
    m_Found++;

    //
    // Check if we have found all
    // targets
    //
    if (m_Found == m_Target.size())
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
    index += Step;
    wordSize = word.size();
    Context->Initialize(wordSize, m_Target);


    // for (size_t i = 0; i < SIMD_COUNT; i++, index += Step)
    do
    {
        Context->AddEntry(word);
        word = m_Generator.Generate(index);
        index += Step;
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
    PreimageContext ctx;
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