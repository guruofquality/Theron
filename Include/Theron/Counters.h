// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_COUNTERS_H
#define THERON_COUNTERS_H


/**
\file Counters.h
Performance event counters.
*/


namespace Theron
{


/**
\brief Enumerated type that lists event counters available for querying.

The counters measure threadpool activity levels, so are useful for
managing the size of the internal thread-pools within \ref Framework "frameworks".
    
\note All counters are local to each Framework instance, and count events in
the queried Framework only.

\see Framework::GetCounterValue
*/
enum Counter
{
    COUNTER_MESSAGES_PROCESSED = 0,     ///< Number of messages processed by the framework.
    COUNTER_YIELDS,                     ///< Number of times a worker thread yielded to other threads.
    COUNTER_LOCAL_PUSHES,               ///< Number of times a mailbox was pushed to a thread's local queue.
    COUNTER_SHARED_PUSHES,              ///< Number of times a mailbox was pushed to the shared queue.
    COUNTER_MAILBOX_QUEUE_MAX,          ///< Maximum number of messages ever seen in the actor mailboxes.
    MAX_COUNTERS                        ///< Number of counters available for querying.
};


} // namespace Theron


#endif // THERON_COUNTERS_H
