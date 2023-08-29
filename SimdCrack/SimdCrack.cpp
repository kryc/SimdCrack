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
                &SimdCrack::GenerateBlock2,
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
    dispatch::BoundRefPtr<PreimageContext> Context
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
SimdCrack::GenerateBlock2(
    const size_t Start,
    const size_t Count,
    const size_t Step
)
{
    std::string word;
    dispatch::BoundRefPtr<PreimageContext> ctx;
    size_t index;

    ctx = dispatch::MakeBoundRefPtr<PreimageContext>();
    index = Start;

    for (
        size_t counter = 0;
        counter < Count;
        index += Step, counter++
    )
    {
        word = WordGenerator::Generate(index, ALPHANUMERIC, false);

        if (ctx->GetLength() == 0)
        {
            ctx->Initialize(word.length(), m_Target);
        }

        if (ctx->GetLength() == word.length())
        {
            ctx->AddEntry(word);
        }
        else
        {
            dispatch::PostTaskFast(
                dispatch::bind(
                    &SimdCrack::ProcessContext,
                    this,
                    std::move(ctx)
                )
            );
            
            ctx = dispatch::MakeBoundRefPtr<PreimageContext>();
            ctx->Initialize(word.length(), m_Target);
            ctx->AddEntry(word);
        }

        if (ctx->IsFull())
        {
            dispatch::PostTaskFast(
                dispatch::bind(
                    &SimdCrack::ProcessContext,
                    this,
                    std::move(ctx)
                )
            );
            ctx = dispatch::MakeBoundRefPtr<PreimageContext>();
        }
    }

    dispatch::PostTaskFast(
        dispatch::bind(
            &SimdCrack::GenerateBlock2,
            this,
            index,
            Count,
            Step
        )
    );

}