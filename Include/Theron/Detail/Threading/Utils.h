// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.
#ifndef THERON_DETAIL_THREADING_UTILS_H
#define THERON_DETAIL_THREADING_UTILS_H


#include <Theron/Assert.h>
#include <Theron/BasicTypes.h>
#include <Theron/Defines.h>


#ifdef _MSC_VER
#pragma warning(push,0)
#endif //_MSC_VER

#if THERON_WINDOWS

#include <windows.h>

#elif THERON_BOOST

#include <boost/thread/thread.hpp>

#elif THERON_CPP11

#include <thread>

#elif defined(THERON_POSIX)

#include <pthread.h>
#include <sched.h>  
#include <time.h>

#endif

#if THERON_NUMA
#if THERON_WINDOWS

#include <windows.h>

#elif THERON_GCC

// NUMA support on Linux requires the libnuma library.
#include <numa.h>

#elif THERON_GCC

#error NUMA support currently requires Windows or GCC with libnuma.

#endif
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif //_MSC_VER


namespace Theron
{
namespace Detail
{


/**
Static thread utility function interface.
*/
class Utils
{
public:

    /**
    Waste some cycles to avoid busy-waiting on a shared memory resource.
    \note This function is intentionally not force-inlined.
    */
    inline static void Backoff(uint32_t &backoff);

    /**
    Yield execution of the calling thread to another hyperthread on the same core.
    */
    inline static void YieldToHyperthread();

    /**
    Yield execution of the calling thread to any available thread on the same core.
    */
    inline static void YieldToLocalThread();

    /**
    Yield execution of the calling thread to any other available thread.
    */
    inline static void YieldToAnyThread();

    /**
    Put the calling thread to sleep for a given number of milliseconds.
    */
    inline static void SleepThread(const uint32_t milliseconds);

    /**
    Gets the number of processor nodes in a NUMA system.

    NUMA stands for Non-Uniform Memory Architecture and refers to systems with memory
    controllers local to specific processors, or groups of processors, such as some
    multi-socket systems. Such systems are characterized by different access speeds
    to different parts of memory from different processors -- depending on whether
    the memory is serviced by a local memory controller or a distant one. In the context
    of a NUMA system, a node is a set of processors whose view of memory is the same,
    ie. equivalent from a performance point of view.

    \return True, if NUMA support is detected and the returned node count is valid.
    */
    inline static bool GetNodeCount(uint32_t &nodeCount);

    /**
    Hints to the OS to run the current thread on the given processors of the given processor nodes of a NUMA system.

    This function requests the OS to execute the calling thread only on the specific processors of
    a specific set of NUMA nodes, or sets of processors. Although restricting a thread's
    execution in this way (typically to the processors of a single node) can reduce its
    execution opportunities, doing so can have positive effects on overall system
    performance by avoiding contention for the system bus caused by shared writes to the
    same memory from different nodes.
    */
    inline static bool SetThreadAffinity(const uint32_t nodeMask, const uint32_t processorMask);

private:

    Utils(const Utils &other);
    Utils &operator=(const Utils &other);
};


inline void Utils::Backoff(uint32_t &backoff)
{
    // The backoff scales from a simple 'nop' to putting the calling thread to sleep.
    // This implementation is based roughly on http://www.1024cores.net/home/lock-free-algorithms/tricks/spinning
    if (++backoff < 10)
    {
        YieldToHyperthread();
    }
    else if (backoff < 20)
    {
        for (uint32_t i = 0; i < 50; ++i)
        {
            YieldToHyperthread();
        }
    }
    else if (backoff < 22)
    {
        YieldToLocalThread();
    }
    else
    {
        YieldToAnyThread();
    }
}


THERON_FORCEINLINE void Utils::YieldToHyperthread()
{

#if THERON_WINDOWS

    YieldProcessor();

#elif THERON_GCC

    __asm__ __volatile__ ("pause");

#endif

}


THERON_FORCEINLINE void Utils::YieldToLocalThread()
{

#if THERON_WINDOWS

    SwitchToThread();

#elif THERON_BOOST

    boost::this_thread::yield();

#elif THERON_CPP11

    std::this_thread::yield();

#elif defined(THERON_POSIX)

    sched_yield();

#endif

}


THERON_FORCEINLINE void Utils::YieldToAnyThread()
{

#if THERON_WINDOWS

    ::Sleep(0);

#elif THERON_BOOST

    boost::this_thread::yield();

#elif THERON_CPP11

    std::this_thread::yield();

#elif defined(THERON_POSIX)

    sched_yield();

#endif

}


THERON_FORCEINLINE void Utils::SleepThread(const uint32_t milliseconds)
{
    THERON_ASSERT(milliseconds < 1000);

#if THERON_WINDOWS

    ::Sleep(milliseconds);

#elif THERON_BOOST

    boost::this_thread::sleep(boost::posix_time::milliseconds(milliseconds));

#elif THERON_CPP11

    std::this_thread::sleep_for(std::chrono::microseconds(milliseconds * 1000));

#elif defined(THERON_POSIX)

    timespec req;
    req.tv_sec = 0;
    req.tv_nsec = 1000 * 1000 * milliseconds;
    nanosleep(&req, 0);

#endif

}


inline bool Utils::GetNodeCount(uint32_t &nodeCount)
{

#if THERON_NUMA

#if THERON_WINDOWS

    ULONG num;
    if (GetNumaHighestNodeNumber(&num))
    {
        nodeCount = static_cast<uint32_t>(num + 1);
        return true;
    }

#elif THERON_GCC

    if (numa_available() >= 0)
    {
        nodeCount = static_cast<uint32_t>(numa_max_node() + 1);
        return true;
    }

#endif

#endif // THERON_NUMA

    return false;
}


inline bool Utils::SetThreadAffinity(const uint32_t nodeMask, const uint32_t processorMask)
{
    // We check this mainly so the arguments are always used.
    if (nodeMask == 0 || processorMask == 0)
    {
        return false;
    }

#if THERON_NUMA

    uint32_t nodeCount(0);
    if (!GetNodeCount(nodeCount))
    {
        return false;
    }

#if THERON_WINDOWS && _WIN32_WINNT >= 0x0601

    // Gather the accumulated mask corresponding to the node mask and processor mask.
    KAFFINITY accumulatedMask(0);

    for (uint32_t node = 0; node < 32 && node < nodeCount; ++node)
    {
        if (nodeMask & (1UL << node))
        {
            GROUP_AFFINITY groupAffinity;
            if (!GetNumaNodeProcessorMaskEx(static_cast<USHORT>(node), &groupAffinity))
            {
                return false;
            }

            // Shift the processor mask to match the node processor mask.
            // This assumes the processors of a node are contiguous.
            KAFFINITY testMask(groupAffinity.Mask);
            KAFFINITY shiftedMask(static_cast<KAFFINITY>(processorMask));

            while ((testMask & 1) == 0)
            {
                testMask >>= 1;
                shiftedMask <<= 1;
            }

            // AND the processor mask with the processor mask for this node.
            accumulatedMask |= (shiftedMask & groupAffinity.Mask);
        }
    }

    if (SetThreadAffinityMask(GetCurrentThread(), static_cast<DWORD_PTR>(accumulatedMask)))
    {
        return true;
    }

#elif THERON_WINDOWS

    // Gather the accumulated mask corresponding to the node mask and processor mask.
    ULONGLONG accumulatedMask(0);

    for (uint32_t node = 0; node < 32 && node < nodeCount; ++node)
    {
        if (nodeMask & (1UL << node))
        {
            ULONGLONG nodeAffinity(0);
            if (!GetNumaNodeProcessorMask(static_cast<UCHAR>(node), &nodeAffinity))
            {
                return false;
            }

            // Shift the processor mask to match the node processor mask.
            // This assumes the processors of a node are contiguous.
            ULONGLONG testMask(nodeAffinity);
            ULONGLONG shiftedMask(static_cast<ULONGLONG>(processorMask));

            while ((testMask & 1) == 0)
            {
                testMask >>= 1;
                shiftedMask <<= 1;
            }

            // AND the processor mask with the processor mask for this node.
            accumulatedMask |= (shiftedMask & nodeAffinity);
        }
    }

    if (SetThreadAffinityMask(GetCurrentThread(), static_cast<DWORD_PTR>(accumulatedMask)))
    {
        return true;
    }

#elif THERON_GCC && defined(LIBNUMA_API_VERSION) && (LIBNUMA_API_VERSION > 1)

    if (numa_available() < 0)
    {
        return false;
    }

    int ret = 0;

    // Maximum number of CPUs to loop through to calc accumulator mask
    const uint32_t maxCPUs = static_cast<uint32_t>(numa_num_configured_cpus());

    // Gather the accumulated mask corresponding to the node mask and processor mask.
    struct bitmask *accumulatedMask = numa_allocate_cpumask();
    struct bitmask *nodeAffinity = numa_allocate_cpumask();
    numa_bitmask_clearall(accumulatedMask);

    // Loop through each node, determine CPUs available and apply processorMask
    for (uint32_t node = 0; node < 32 && node < nodeCount; ++node)
    {
        if ((nodeMask & (1UL << node)) == 0) continue;

        numa_bitmask_clearall(nodeAffinity);
        ret = numa_node_to_cpus(node, nodeAffinity);
        if (ret != 0) goto numa_cleanup_and_done;

        // Shift the processor mask to match the node processor mask.
        // This assumes the processors of a node are contiguous.
        uint32_t procMaskIndex(0);
        for (uint32_t cpu = 0; cpu < maxCPUs; cpu++)
        {
            if (numa_bitmask_isbitset(nodeAffinity, cpu) && (processorMask & (1UL << procMaskIndex++)) != 0)
            {
                numa_bitmask_setbit(accumulatedMask, cpu);
            }
        }
    }

    // AccumulatedMask determined, this call actually sets the affinity
    ret = numa_sched_setaffinity(0, accumulatedMask);

numa_cleanup_and_done:
    numa_free_cpumask(accumulatedMask);
    numa_free_cpumask(nodeAffinity);
    return ret == 0;

#elif THERON_GCC && defined(LIBNUMA_API_VERSION)

    if (numa_available() < 0)
    {
        return false;
    }

    // Init a numa nodemask to fill with the bits of nodeMask
    nodemask_t nm;
    nodemask_zero(&nm);

    // Loop through all nodes to set the bitmask struct
    for (uint32_t node = 0; node < 32 && node < nodeCount; ++node)
    {
        if ((nodeMask & (1UL << node)) != 0)
        {
            nodemask_set(&nm, node);
        }
    }

    // Set the affinity on the nodes set in the bitmask
    return numa_run_on_node_mask(&nm) == 0;

#endif

#endif // THERON_NUMA

    return false;
}


} // namespace Detail
} // namespace Theron


#endif // THERON_DETAIL_THREADING_UTILS_H
