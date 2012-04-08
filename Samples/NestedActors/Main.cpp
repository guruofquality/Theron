// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


//
// This sample demonstrates nesting of actors to create abstracted subsystems.
//


#include <stdio.h>
#include <vector>
#include <queue>

#include <Theron/Actor.h>
#include <Theron/ActorRef.h>
#include <Theron/Address.h>
#include <Theron/Framework.h>
#include <Theron/Receiver.h>


// Actor representing a pipeline stage.
class Stage : public Theron::Actor
{
public:

    // A simple message representing some work to be done by the stage.
    // For this example the work message is just an integer to be incremented.
    // The work message also remembers the address of the client that requested the work.
    struct WorkMessage
    {
        inline WorkMessage(const Theron::Address client, const int value) :
          mClient(client),
          mValue(value)
        {
        }

        Theron::Address mClient;
        int mValue;
    };

    typedef Theron::Address Parameters;

    inline Stage(const Parameters &nextStageAddress) : mNextStageAddress(nextStageAddress)
    {
        RegisterHandler(this, &Stage::HandleWork);
    }

private:

    inline void HandleWork(const WorkMessage &work, const Theron::Address /*from*/)
    {
        // Do the work and then forward the work message to the next stage.
        // In this trivial example the work done by each stage is just to increment the message.
        TailSend(WorkMessage(work.mClient, work.mValue + 1), mNextStageAddress);
    }

    Theron::Address mNextStageAddress;
};


// Pipeline actor which owns the stages as children.
class Pipeline : public Theron::Actor
{
public:

    struct WorkMessage
    {
        inline WorkMessage(const int value) : mValue(value)
        {
        }

        int mValue;
    };

    typedef Stage::WorkMessage StageWorkMessage;
    typedef int Parameters;

    inline Pipeline(const Parameters &numStages) : mStages(numStages)
    {
        // Create the stages. Each stage knows the address of the next stage in the pipeline.
        // The last stage sends its result back to the Pipeline actor as its "next stage"
        Theron::Address nextStageAddress(GetAddress());
        for (int i = numStages - 1; i >= 0; --i)
        {
            // Pass the address of the next stage to each stage on construction.
            mStages[i] = GetFramework().CreateActor<Stage>(nextStageAddress);
            nextStageAddress = mStages[i].GetAddress();
        }

        RegisterHandler(this, &Pipeline::HandleWork);
        RegisterHandler(this, &Pipeline::HandleStageResult);
    }

private:

    inline void HandleWork(const WorkMessage &work, const Theron::Address from)
    {
        // Handle the incoming work message from a client by dispatching a work message
        // to the first pipeline stage, marked with the address of the requesting client.
        TailSend(StageWorkMessage(from, work.mValue), mStages[0].GetAddress());
    }

    inline void HandleStageResult(const StageWorkMessage &result, const Theron::Address /*from*/)
    {
        // Handle the incoming result from the last pipeline stage by sending the computed
        // result back to the client that requested the work, whose address is stored in the message.
        TailSend(WorkMessage(result.mValue), result.mClient);
    }

    // The stage child actors are referenced by actor references held as members, which
    // is important since it prevents them from being garbage collected. When the
    // Pipeline is destructed the actor refs stored in the vector are destructed too,
    // which causes the stage actors to become unreferenced and scheduled for garbage
    // collection.
    std::vector<Theron::ActorRef> mStages;
};


// A handler to catch the results sent back from the pipeline.
class Catcher
{
public:

    inline void Catch(const Pipeline::WorkMessage &result, const Theron::Address /*from*/)
    {
        mResults.push(result);
    }

    inline bool Empty() const
    {
        return mResults.empty();
    }

    inline Pipeline::WorkMessage Pop()
    {
        const Pipeline::WorkMessage result(mResults.front());
        mResults.pop();
        return result;
    }

private:

    std::queue<Pipeline::WorkMessage> mResults;
};


int main()
{
    Theron::Framework framework;
    
    // Create a receiver to receive the results sent back from the pipeline, and a catcher
    // to hold the results it receives.
    Theron::Receiver receiver;
    Catcher catcher;
    receiver.RegisterHandler(&catcher, &Catcher::Catch);

    // Create the pipeline. It creates its stages as children privately.
    const int numStages(10);
    Theron::ActorRef pipeline(framework.CreateActor<Pipeline>(numStages));

    // Send some work messages to the pipeline.
    // Each consists of an integer, which is incremented by every stage.
    const int numWorkItems(10);
    for (int i = 0; i < numWorkItems; ++i)
    {
        printf("Sending work request '%d'\n", i);
        framework.Send(Pipeline::WorkMessage(i), receiver.GetAddress(), pipeline.GetAddress());
    }

    // Wait for the replies sent back by the pipeline when all stages complete.
    int outstanding(numWorkItems);
    while (outstanding)
    {
        int batchSize(receiver.Wait(outstanding));
        outstanding -= batchSize;

        // Process this batch of results.
        while (batchSize--)
        {
            const Pipeline::WorkMessage result(catcher.Pop());
            printf("Caught result '%d'\n", result.mValue);
        }
    }

    return 0;
}

