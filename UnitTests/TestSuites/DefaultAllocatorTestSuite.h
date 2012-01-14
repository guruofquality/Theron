// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_TESTS_TESTSUITES_DEFAULTALLOCATORTESTSUITE_H
#define THERON_TESTS_TESTSUITES_DEFAULTALLOCATORTESTSUITE_H


#include <Theron/Detail/BasicTypes.h>

#include <Theron/Align.h>
#include <Theron/DefaultAllocator.h>

#include "TestFramework/TestSuite.h"


namespace UnitTests
{


class DefaultAllocatorTestSuite : public TestFramework::TestSuite
{
public:

    inline DefaultAllocatorTestSuite()
    {
        TESTFRAMEWORK_REGISTER_TESTSUITE(DefaultAllocatorTestSuite);

        TESTFRAMEWORK_REGISTER_TEST(TestConstruct);
        TESTFRAMEWORK_REGISTER_TEST(TestAllocate);
        TESTFRAMEWORK_REGISTER_TEST(TestUseAllocatedBlock);
        TESTFRAMEWORK_REGISTER_TEST(TestAllocateMultiple);
        TESTFRAMEWORK_REGISTER_TEST(TestAllocateMultipleDifferentSizes);
        TESTFRAMEWORK_REGISTER_TEST(TestFreeOutOfOrder);
        TESTFRAMEWORK_REGISTER_TEST(TestUseMultipleAllocatedBlocks);
        TESTFRAMEWORK_REGISTER_TEST(TestAllocateAligned4);
        TESTFRAMEWORK_REGISTER_TEST(TestAllocateAligned8);
        TESTFRAMEWORK_REGISTER_TEST(TestAllocateAligned16);
        TESTFRAMEWORK_REGISTER_TEST(TestAllocateAligned32);
        TESTFRAMEWORK_REGISTER_TEST(TestAllocateAligned64);
        TESTFRAMEWORK_REGISTER_TEST(TestAllocateAligned128);
        TESTFRAMEWORK_REGISTER_TEST(TestAllocateMultipleAligned);
        TESTFRAMEWORK_REGISTER_TEST(TestGetBytesAllocated);
        TESTFRAMEWORK_REGISTER_TEST(TestGetPeakBytesAllocated);
    }

    inline virtual ~DefaultAllocatorTestSuite()
    {
    }

    inline static void TestConstruct()
    {
        Theron::DefaultAllocator allocator;
    }

    inline static void TestAllocate()
    {
        Theron::DefaultAllocator allocator;

        void *const block(allocator.Allocate(sizeof(Item)));
        
        Check(block != 0, "Allocate return null block pointer");
        Check(THERON_ALIGNED(block, 4), "Allocated block isn't aligned");

        allocator.Free(block);
    }

    inline static void TestUseAllocatedBlock()
    {
        Theron::DefaultAllocator allocator;

        void *const block(allocator.Allocate(sizeof(Item)));

        Item *const item(reinterpret_cast<Item *>(block));

        const Item val(0, 1, 2);
        *item = val;

        Check(item->mA == val.mA, "Allocated memory can't be used");
        Check(item->mB == val.mB, "Allocated memory can't be used");
        Check(item->mC == val.mC, "Allocated memory can't be used");

        allocator.Free(block);
    }

    inline static void TestAllocateMultiple()
    {
        Theron::DefaultAllocator allocator;

        void *const block0(allocator.Allocate(sizeof(Item)));
        void *const block1(allocator.Allocate(sizeof(Item)));

        Check(block0 != 0, "Allocate return null block pointer");
        Check(block1 != 0, "Allocate return null block pointer");

        Check(THERON_ALIGNED(block0, 4), "Allocated block isn't aligned");
        Check(THERON_ALIGNED(block1, 4), "Allocated block isn't aligned");

        Theron::uint8_t *const bytes0(reinterpret_cast<Theron::uint8_t *>(block0));
        Theron::uint8_t *const bytes1(reinterpret_cast<Theron::uint8_t *>(block1));

        Check(bytes0 + sizeof(Item) <= bytes1 || bytes1 + sizeof(Item) <= bytes0, "Allocate returned overlapping blocks");

        allocator.Free(block1);
        allocator.Free(block0);
    }

    inline static void TestAllocateMultipleDifferentSizes()
    {
        Theron::DefaultAllocator allocator;

        void *const block0(allocator.Allocate(sizeof(Item)));
        void *const block1(allocator.Allocate(sizeof(ItemTwo)));

        Check(block0 != 0, "Allocate return null block pointer");
        Check(block1 != 0, "Allocate return null block pointer");

        Check(THERON_ALIGNED(block0, 4), "Allocated block isn't aligned");
        Check(THERON_ALIGNED(block1, 4), "Allocated block isn't aligned");

        Theron::uint8_t *const bytes0(reinterpret_cast<Theron::uint8_t *>(block0));
        Theron::uint8_t *const bytes1(reinterpret_cast<Theron::uint8_t *>(block1));

        Check(bytes0 + sizeof(Item) <= bytes1 || bytes1 + sizeof(ItemTwo) <= bytes0, "Allocate returned overlapping blocks");

        allocator.Free(block1);
        allocator.Free(block0);
    }

    inline static void TestFreeOutOfOrder()
    {
        Theron::DefaultAllocator allocator;

        void *const block0(allocator.Allocate(sizeof(Item)));
        void *const block1(allocator.Allocate(sizeof(Item)));

        allocator.Free(block0);
        allocator.Free(block1);
    }

    inline static void TestUseMultipleAllocatedBlocks()
    {
        Theron::DefaultAllocator allocator;

        void *const block0(allocator.Allocate(sizeof(Item)));
        void *const block1(allocator.Allocate(sizeof(Item)));

        Item *const item0(reinterpret_cast<Item *>(block0));
        Item *const item1(reinterpret_cast<Item *>(block1));

        const Item val0(0, 1, 2);
        const Item val1(5000000, 4000000, 3000000);

        *item0 = val0;
        *item1 = val1;

        Check(item0->mA == val0.mA, "Allocated memory can't be used");
        Check(item0->mB == val0.mB, "Allocated memory can't be used");
        Check(item0->mC == val0.mC, "Allocated memory can't be used");

        Check(item1->mA == val1.mA, "Allocated memory can't be used");
        Check(item1->mB == val1.mB, "Allocated memory can't be used");
        Check(item1->mC == val1.mC, "Allocated memory can't be used");

        allocator.Free(block1);
        allocator.Free(block0);
    }

    inline static void TestAllocateAligned4()
    {
        Theron::DefaultAllocator allocator;

        void *const block(allocator.AllocateAligned(sizeof(Item), 4));
        Check(THERON_ALIGNED(block, 4), "Allocated block isn't aligned");

        allocator.Free(block);
    }

    inline static void TestAllocateAligned8()
    {
        Theron::DefaultAllocator allocator;

        void *const block(allocator.AllocateAligned(sizeof(Item), 8));
        Check(THERON_ALIGNED(block, 8), "Allocated block isn't aligned");

        allocator.Free(block);
    }

    inline static void TestAllocateAligned16()
    {
        Theron::DefaultAllocator allocator;

        void *const block(allocator.AllocateAligned(sizeof(Item), 16));
        Check(THERON_ALIGNED(block, 16), "Allocated block isn't aligned");

        allocator.Free(block);
    }

    inline static void TestAllocateAligned32()
    {
        Theron::DefaultAllocator allocator;

        void *const block(allocator.AllocateAligned(sizeof(Item), 32));
        Check(THERON_ALIGNED(block, 32), "Allocated block isn't aligned");

        allocator.Free(block);
    }

    inline static void TestAllocateAligned64()
    {
        Theron::DefaultAllocator allocator;

        void *const block(allocator.AllocateAligned(sizeof(Item), 64));
        Check(THERON_ALIGNED(block, 64), "Allocated block isn't aligned");

        allocator.Free(block);
    }

    inline static void TestAllocateAligned128()
    {
        Theron::DefaultAllocator allocator;

        void *const block(allocator.AllocateAligned(sizeof(Item), 128));
        Check(THERON_ALIGNED(block, 128), "Allocated block isn't aligned");

        allocator.Free(block);
    }

    inline static void TestAllocateMultipleAligned()
    {
        Theron::DefaultAllocator allocator;

        void *const block0(allocator.AllocateAligned(sizeof(Item), 4));
        void *const block1(allocator.AllocateAligned(sizeof(Item), 8));
        void *const block2(allocator.AllocateAligned(sizeof(Item), 16));
        void *const block3(allocator.AllocateAligned(sizeof(Item), 32));
        void *const block4(allocator.AllocateAligned(sizeof(Item), 64));
        void *const block5(allocator.AllocateAligned(sizeof(Item), 128));

        Check(THERON_ALIGNED(block0, 4), "Allocated block isn't aligned");
        Check(THERON_ALIGNED(block1, 8), "Allocated block isn't aligned");
        Check(THERON_ALIGNED(block2, 16), "Allocated block isn't aligned");
        Check(THERON_ALIGNED(block3, 32), "Allocated block isn't aligned");
        Check(THERON_ALIGNED(block4, 64), "Allocated block isn't aligned");
        Check(THERON_ALIGNED(block5, 128), "Allocated block isn't aligned");

        allocator.Free(block5);
        allocator.Free(block4);
        allocator.Free(block3);
        allocator.Free(block2);
        allocator.Free(block1);
        allocator.Free(block0);
    }

    inline static void TestGetBytesAllocated()
    {
        Theron::DefaultAllocator allocator;

        void *const block0(allocator.AllocateAligned(sizeof(Item), 4));
        void *const block1(allocator.AllocateAligned(sizeof(Item), 128));
        void *const block2(allocator.AllocateAligned(sizeof(Item), 4));
        void *const block3(allocator.AllocateAligned(sizeof(Item), 128));

#if THERON_ENABLE_DEFAULTALLOCATOR_CHECKS
        Check(allocator.GetBytesAllocated() == 4 * sizeof(Item), "Allocated byte count incorrect");
#else
        Check(allocator.GetBytesAllocated() == 0, "Allocated byte count incorrect");
#endif // THERON_ENABLE_DEFAULTALLOCATOR_CHECKS

        allocator.Free(block3);
        allocator.Free(block2);
        allocator.Free(block1);
        allocator.Free(block0);

        Check(allocator.GetBytesAllocated() == 0, "Allocated byte count incorrect");
    }

    inline static void TestGetPeakBytesAllocated()
    {
        Theron::DefaultAllocator allocator;

        void *const block0(allocator.AllocateAligned(sizeof(Item), 4));
        void *const block1(allocator.AllocateAligned(sizeof(Item), 128));
        void *const block2(allocator.AllocateAligned(sizeof(Item), 4));
        void *const block3(allocator.AllocateAligned(sizeof(Item), 128));

#if THERON_ENABLE_DEFAULTALLOCATOR_CHECKS
        Check(allocator.GetPeakBytesAllocated() == 4 * sizeof(Item), "Allocated byte count incorrect");
#else
        Check(allocator.GetPeakBytesAllocated() == 0, "Allocated byte count incorrect");
#endif // THERON_ENABLE_DEFAULTALLOCATOR_CHECKS

        allocator.Free(block1);
        allocator.Free(block0);

#if THERON_ENABLE_DEFAULTALLOCATOR_CHECKS
        Check(allocator.GetPeakBytesAllocated() == 4 * sizeof(Item), "Allocated byte count incorrect");
#else
        Check(allocator.GetPeakBytesAllocated() == 0, "Allocated byte count incorrect");
#endif // THERON_ENABLE_DEFAULTALLOCATOR_CHECKS

        void *const block4(allocator.Allocate(sizeof(Item)));
        void *const block5(allocator.AllocateAligned(sizeof(Item), 128));

#if THERON_ENABLE_DEFAULTALLOCATOR_CHECKS
        Check(allocator.GetPeakBytesAllocated() == 4 * sizeof(Item), "Allocated byte count incorrect");
#else
        Check(allocator.GetPeakBytesAllocated() == 0, "Allocated byte count incorrect");
#endif // THERON_ENABLE_DEFAULTALLOCATOR_CHECKS

        void *const block6(allocator.Allocate(sizeof(Item)));
        void *const block7(allocator.AllocateAligned(sizeof(Item), 128));

#if THERON_ENABLE_DEFAULTALLOCATOR_CHECKS
        Check(allocator.GetPeakBytesAllocated() == 6 * sizeof(Item), "Allocated byte count incorrect");
#else
        Check(allocator.GetPeakBytesAllocated() == 0, "Allocated byte count incorrect");
#endif // THERON_ENABLE_DEFAULTALLOCATOR_CHECKS

        allocator.Free(block3);
        allocator.Free(block2);
        allocator.Free(block4);
        allocator.Free(block5);
        allocator.Free(block6);
        allocator.Free(block7);
    
#if THERON_ENABLE_DEFAULTALLOCATOR_CHECKS
        Check(allocator.GetPeakBytesAllocated() == 6 * sizeof(Item), "Allocated byte count incorrect");
#else
        Check(allocator.GetPeakBytesAllocated() == 0, "Allocated byte count incorrect");
#endif // THERON_ENABLE_DEFAULTALLOCATOR_CHECKS
    }

private:

    struct Item
    {
        inline Item(const Theron::uint32_t a, const Theron::uint32_t b, const Theron::uint32_t c) :
          mA(a),
          mB(b),
          mC(c)
        {
        }

        Theron::uint32_t mA;
        Theron::uint32_t mB;
        Theron::uint32_t mC;
    };

    struct ItemTwo
    {
        inline ItemTwo(const Theron::uint32_t a, const Theron::uint32_t b) :
          mA(a),
          mB(b)
        {
        }

        Theron::uint32_t mA;
        Theron::uint32_t mB;
    };
};


} // namespace UnitTests


#endif // THERON_TESTS_TESTSUITES_DEFAULTALLOCATORTESTSUITE_H

