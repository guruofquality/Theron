// Copyright (C) by Ashton Mason. See LICENSE.txt for licensing information.


#ifndef THERON_TESTS_TESTSUITES_LISTTESTSUITE_H
#define THERON_TESTS_TESTSUITES_LISTTESTSUITE_H


#include <Theron/Detail/Containers/List.h>

#include "TestFramework/TestSuite.h"


namespace UnitTests
{


/// A suite of unit tests.
class ListTestSuite : public TestFramework::TestSuite
{
public:

    /// Default constructor. Registers the tests.
    inline ListTestSuite()
    {
        TESTFRAMEWORK_REGISTER_TESTSUITE(ListTestSuite);

        TESTFRAMEWORK_REGISTER_TEST(TestInitialize);
        TESTFRAMEWORK_REGISTER_TEST(TestInsert);
        TESTFRAMEWORK_REGISTER_TEST(TestRemoveOnlyItem);
        TESTFRAMEWORK_REGISTER_TEST(TestRemoveFrontItem);
        TESTFRAMEWORK_REGISTER_TEST(TestRemoveBackItem);
        TESTFRAMEWORK_REGISTER_TEST(TestRemoveMiddleItem);
        TESTFRAMEWORK_REGISTER_TEST(TestContains);
        TESTFRAMEWORK_REGISTER_TEST(TestContainsNegative);
        TESTFRAMEWORK_REGISTER_TEST(TestDuplicateItems);
        TESTFRAMEWORK_REGISTER_TEST(TestContainsAfterRemove);
        TESTFRAMEWORK_REGISTER_TEST(TestTemporarilyEmpty);
        TESTFRAMEWORK_REGISTER_TEST(TestSizeInitially);
        TESTFRAMEWORK_REGISTER_TEST(TestSizeAfterInsert);
        TESTFRAMEWORK_REGISTER_TEST(TestSizeAfterInsertRemove);
        TESTFRAMEWORK_REGISTER_TEST(TestSizeAfterInsertContains);
        TESTFRAMEWORK_REGISTER_TEST(TestSizeAfterInsertInsert);
    }

    /// Virtual destructor
    inline virtual ~ListTestSuite()
    {
    }

    /// Unit test method.
    inline static void TestInitialize()
    {
        Theron::Detail::List<MockItem> list;
    }

    /// Unit test method.
    inline static void TestInsert()
    {
        Theron::Detail::List<MockItem> list;

        MockItem item(5);
        list.Insert(item);
    }

    /// Unit test method.
    inline static void TestRemoveOnlyItem()
    {
        Theron::Detail::List<MockItem> list;

        MockItem item(5);
        list.Insert(item);
        Check(list.Remove(item), "Failed to remove list item after inserting it");
    }

    /// Unit test method.
    inline static void TestRemoveFrontItem()
    {
        Theron::Detail::List<MockItem> list;

        MockItem itemOne(5);
        MockItem itemTwo(6);
        list.Insert(itemOne);
        list.Insert(itemTwo);
        Check(list.Remove(itemOne), "Failed to remove front list item");
    }

    /// Unit test method.
    inline static void TestRemoveBackItem()
    {
        Theron::Detail::List<MockItem> list;

        MockItem itemOne(5);
        MockItem itemTwo(6);
        list.Insert(itemOne);
        list.Insert(itemTwo);
        Check(list.Remove(itemTwo), "Failed to remove back list item");
    }

    /// Unit test method.
    inline static void TestRemoveMiddleItem()
    {
        Theron::Detail::List<MockItem> list;

        MockItem itemOne(5);
        MockItem itemTwo(6);
        MockItem itemThree(7);
        list.Insert(itemOne);
        list.Insert(itemTwo);
        list.Insert(itemThree);
        Check(list.Remove(itemTwo), "Failed to remove middle list item");
    }

    /// Unit test method.
    inline static void TestContains()
    {
        Theron::Detail::List<MockItem> list;

        MockItem item(5);
        list.Insert(item);
        Check(list.Contains(item), "Contains returned an incorrect result");
    }

    /// Unit test method.
    inline static void TestContainsNegative()
    {
        Theron::Detail::List<MockItem> list;

        MockItem item(5);
        list.Insert(item);
        MockItem otherItem(6);
        Check(list.Contains(otherItem) == false, "Contains returned an incorrect result");
    }

    /// Unit test method.
    inline static void TestDuplicateItems()
    {
        Theron::Detail::List<MockItem> list;

        MockItem item(6);
        list.Insert(item);
        list.Insert(item);

        Check(list.Size() == 2, "Duplicate items not handled correctly by Insert");
        Check(list.Contains(item), "Duplicate items not handled correctly by Contains");
        Check(list.Remove(item), "Duplicate items not handled correctly by Remove");
        Check(list.Size() == 1, "Duplicate items not handled correctly by Remove");
        Check(list.Contains(item), "Duplicate items not handled correctly by Contains");
        Check(list.Remove(item), "Duplicate items not handled correctly by Remove");
        Check(list.Size() == 0, "Duplicate items not handled correctly by Size");
        Check(list.Contains(item) == false, "Duplicate items not handled correctly by Contains");
    }

    /// Unit test method.
    inline static void TestContainsAfterRemove()
    {
        Theron::Detail::List<MockItem> list;

        MockItem itemOne(5);
        MockItem itemTwo(6);
        list.Insert(itemOne);
        list.Insert(itemTwo);
        list.Remove(itemOne);
        Check(list.Contains(itemOne) == false, "Contains confused by removing one of two items");
        Check(list.Contains(itemTwo), "Contains confused by removing one of two items");
    }

    /// Unit test method.
    inline static void TestTemporarilyEmpty()
    {
        Theron::Detail::List<MockItem> list;

        MockItem itemOne(5);
        MockItem itemTwo(6);
        list.Insert(itemOne);
        list.Remove(itemOne);
        list.Insert(itemTwo);
        
        Check(list.Contains(itemOne) == false, "Contains confused by temporarily empty list");
        Check(list.Contains(itemTwo), "Contains confused by temporarily empty list");
        Check(list.Remove(itemOne) == false, "Remove confused by temporarily empty list");
        Check(list.Remove(itemTwo), "Remove failed to remove item after list was temporarily empty");
    }

    /// Unit test method.
    inline static void TestSizeInitially()
    {
        Theron::Detail::List<MockItem> list;

        Check(list.Size() == 0, "Size not initially zero");
    }

    /// Unit test method.
    inline static void TestSizeAfterInsert()
    {
        Theron::Detail::List<MockItem> list;

        MockItem item(6);
        list.Insert(item);

        Check(list.Size() == 1, "Size not one after Insert");
    }

    /// Unit test method.
    inline static void TestSizeAfterInsertRemove()
    {
        Theron::Detail::List<MockItem> list;

        MockItem item(6);
        list.Insert(item);
        list.Remove(item);

        Check(list.Size() == 0, "Size not zero after Insert, Remove");
    }

    /// Unit test method.
    inline static void TestSizeAfterInsertContains()
    {
        Theron::Detail::List<MockItem> list;

        MockItem item(6);
        list.Insert(item);

        Check(list.Contains(item) == true, "Expected Contains to return true for inserted item");
        Check(list.Size() == 1, "Size not one after Insert, Contains");
    }

    /// Unit test method.
    inline static void TestSizeAfterInsertInsert()
    {
        Theron::Detail::List<MockItem> list;

        MockItem item(6);
        list.Insert(item);
        list.Insert(item);

        Check(list.Size() == 2, "Size not two after Insert, Insert");
    }

private:

    class MockItem
    {
    public:

        inline MockItem() : mValue(0) { }
        inline explicit MockItem(const int value) : mValue(value) { }
        inline MockItem(const MockItem &other) : mValue(other.mValue) { }
        inline MockItem &operator=(const MockItem &other) { mValue = other.mValue; return *this; }
        inline bool operator==(const MockItem &other) const { return mValue == other.mValue; }
        inline bool operator!=(const MockItem &other) const { return mValue != other.mValue; }

    private:

        int mValue;
    };
};


} // namespace UnitTests


#endif // THERON_TESTS_TESTSUITES_LISTTESTSUITE_H

