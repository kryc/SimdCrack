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
                &SimdCrack::GenerateBlock,
                this,
                i + 1,
                1000,
                m_Threads
            )
        );
    }    
}

void
SimdCrack::FoundResult(
    std::string Result
)
{
    assert(dispatch::CurrentDispatcher() == dispatch::GetDispatcher("main").get());

    //
    // Output it
    //
    std::cout << Result << std::endl;
    m_Found = true;

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

void
SimdCrack::BlockProcessed(
    std::string Result
)
{
    dispatch::PostTaskToDispatcher(
        "main",
        dispatch::bind(
            &SimdCrack::FoundResult,
            this,
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
            std::placeholders::_1
        )
    );
}

void
SimdCrack::GenerateBlock(
    const size_t Start,
    const size_t Count,
    const size_t Step
)
{
    std::string word;
    PreimageContext ctx;
    size_t index;

    index = Start;

    //
    // Generate the first word to get the size so
    // we can initialize the context
    //
    word = m_Generator.Generate(index);
    ctx.Initialize(word.length(), m_Target);
    ctx.AddEntry(word);

    for (
        size_t counter = 0;
        counter < Count;
        index += Step, counter++
    )
    {
        if (ctx.IsFull())
        {
            ProcessContext(&ctx);
            ctx.Initialize(word.length(), m_Target);
        }

        word = m_Generator.Generate(index);

        if (ctx.GetLength() == word.length())
        {
            ctx.AddEntry(word);
        }
        else
        {
            if (!ctx.IsEmpty())
            {
                ProcessContext(&ctx);
            }
            ctx.Initialize(word.length(), m_Target);
            ctx.AddEntry(word);
        }
    }

    //
    // If there is a partial context
    // process it now before the next round
    //
    if (!ctx.IsEmpty())
    {
        ProcessContext(&ctx);
    }

    dispatch::PostTaskFast(
        dispatch::bind(
            &SimdCrack::GenerateBlock,
            this,
            index,
            Count,
            Step
        )
    );
}