// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_YIELDSTRATEGY_H
#define THERON_YIELDSTRATEGY_H


/**
\file YieldStrategy.h
Defines the YieldStrategy enumerated type.
*/


namespace Theron
{


/**
\brief Enumerates the available worker thread yield strategies.

Each \ref Theron::Framework contains a pool of worker threads that are used to execute
the actors hosted in the framework. The worker threads service a queue, processing actors that
have received messages and executing their registered message handlers.

When constructing a Framework, a \ref Theron::Framework::Parameters object may be provided with
parameters that control the structure and behavior of the the framework's internal threadpool.
This enum defines the available values of the \ref Theron::Framework::Parameters::mYieldStrategy "mYieldStrategy"
member of the Parameters structure.

The mYieldStrategy member defines the strategy that the worker threads use to avoid \em busy \em waiting
on the work queue. The available strategies have different performance characteristics, and are best
suited to different kinds of applications.

The default strategy, \ref YIELD_STRATEGY_POLITE,
causes the threads to go to sleep for a short period when they fail to find work on the work queue.
Going to sleep frees up the processor during quiet periods and so gives other threads on the system
a chance to be executed. The downside is that if a message arrives after a period of inactivity then
it may only be processed when one or more threads awake, leading to some small latency.

The more aggressive strategies cause the threads to yield to other threads, or simply spin, without going to
sleep. These strategies typically have lower worst-case latency. However the reduced latency is at
the expense of increased CPU usage, increased power consumption, and potentially lower throughput.

When choosing a yield strategy it pays to consider how important immediate responsiveness is to your
application: in most applications a latency of a few milliseconds is not significant, and the default
strategy is a reasonable choice.
*/
enum YieldStrategy
{
    YIELD_STRATEGY_BLOCKING,            ///< Threads wait on locks and condition variables.
    YIELD_STRATEGY_POLITE,              ///< Threads go to sleep when not in use.
    YIELD_STRATEGY_STRONG,              ///< Threads yield to other threads but don't go to sleep.
    YIELD_STRATEGY_AGGRESSIVE           ///< Threads never sleep or yield to other threads.
};


} // namespace Theron


#endif // THERON_YIELDSTRATEGY_H
