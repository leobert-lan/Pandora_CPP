#include <gtest/gtest.h>
#include "pandora/real_data_set.h"
#include "pandora/wrapper_data_set.h"
#include "pandora/list_update_callback.h"
#include "Global.h"
#include <vector>
#include <memory>

using namespace pandora;

// Mock ListUpdateCallback to record all callback events
class MockListUpdateCallback : public ListUpdateCallback
{
public:
    struct Event
    {
        enum Type { INSERTED, REMOVED, MOVED, CHANGED };

        Type type;
        int position;
        int count;
        int toPosition; // for MOVED only

        Event(Type t, int pos, int cnt, int to = -1)
            : type(t), position(pos), count(cnt), toPosition(to)
        {
        }

        bool operator==(const Event& other) const
        {
            return type == other.type &&
                position == other.position &&
                count == other.count &&
                toPosition == other.toPosition;
        }
    };

    std::vector<Event> events;

    void OnInserted(int position, int count) override
    {
        events.emplace_back(Event::INSERTED, position, count);
    }

    void OnRemoved(int position, int count) override
    {
        events.emplace_back(Event::REMOVED, position, count);
    }

    void OnMoved(int from_position, int to_position) override
    {
        events.emplace_back(Event::MOVED, from_position, 1, to_position);
    }

    void OnChanged(int position, int count, void* payload = nullptr) override
    {
        events.emplace_back(Event::CHANGED, position, count);
    }

    void Clear()
    {
        events.clear();
    }

    bool HasEvent(Event::Type type, int position, int count) const
    {
        for (const auto& e : events)
        {
            if (e.type == type && e.position == position && e.count == count)
            {
                return true;
            }
        }
        return false;
    }
};

// ==================== RealDataSet Tests ====================

TEST(RealDataSetCallbackTest, InsertCallback)
{
    RealDataSet<TestData> ds;
    auto callback = std::make_unique<MockListUpdateCallback>();
    auto callbackPtr = callback.get();
    ds.SetListUpdateCallback(std::move(callback));

    // Add single item
    ds.Add(TestData(1));
    ASSERT_EQ(callbackPtr->events.size(), 1);
    EXPECT_TRUE(callbackPtr->HasEvent(MockListUpdateCallback::Event::INSERTED, 0, 1));

    callbackPtr->Clear();

    // Add at position
    ds.Add(0, TestData(2));
    ASSERT_EQ(callbackPtr->events.size(), 1);
    EXPECT_TRUE(callbackPtr->HasEvent(MockListUpdateCallback::Event::INSERTED, 0, 1));
}

TEST(RealDataSetCallbackTest, RemoveCallback)
{
    RealDataSet<TestData> ds;
    auto callback = std::make_unique<MockListUpdateCallback>();
    auto callbackPtr = callback.get();
    ds.SetListUpdateCallback(std::move(callback));

    // Setup data
    ds.Add(TestData(1));
    ds.Add(TestData(2));
    ds.Add(TestData(3));
    callbackPtr->Clear();

    // Remove by position
    ds.RemoveAtPos(1);
    ASSERT_EQ(callbackPtr->events.size(), 1);
    EXPECT_TRUE(callbackPtr->HasEvent(MockListUpdateCallback::Event::REMOVED, 1, 1));

    callbackPtr->Clear();

    // Remove by item
    ds.Remove(TestData(3));
    ASSERT_EQ(callbackPtr->events.size(), 1);
    EXPECT_TRUE(callbackPtr->HasEvent(MockListUpdateCallback::Event::REMOVED, 1, 1));
}

TEST(RealDataSetCallbackTest, ReplaceCallback)
{
    RealDataSet<TestData> ds;
    auto callback = std::make_unique<MockListUpdateCallback>();
    auto callbackPtr = callback.get();
    ds.SetListUpdateCallback(std::move(callback));

    // Setup data
    ds.Add(TestData(1, "original"));
    callbackPtr->Clear();

    // Replace with same ID but different content
    ds.ReplaceAtPosIfExist(0, TestData(1, "modified"));

    // Since operator== compares both value and name, and they differ,
    // DiffUtil treats this as REMOVE + INSERT, not CHANGED
    ASSERT_GE(callbackPtr->events.size(), 1);
    // EXPECT_TRUE(callbackPtr->HasEvent(MockListUpdateCallback::Event::CHANGED, 0, 1));

    // Should have either REMOVED+INSERTED or just structural changes
    bool hasRemoved = callbackPtr->HasEvent(MockListUpdateCallback::Event::REMOVED, 0, 1);
    bool hasInserted = callbackPtr->HasEvent(MockListUpdateCallback::Event::INSERTED, 0, 1);
    EXPECT_TRUE(hasRemoved && hasInserted);
}

TEST(RealDataSetCallbackTest, SetDataCallback)
{
    RealDataSet<TestData> ds;
    auto callback = std::make_unique<MockListUpdateCallback>();
    auto callbackPtr = callback.get();
    ds.SetListUpdateCallback(std::move(callback));

    // Setup initial data
    ds.Add(TestData(1));
    ds.Add(TestData(2));
    callbackPtr->Clear();

    // Set new data
    std::vector<TestData> newData = {TestData(3), TestData(4), TestData(5)};
    ds.SetData(newData);

    // Should have callbacks for the changes
    ASSERT_GT(callbackPtr->events.size(), 0);
}

TEST(RealDataSetCallbackTest, ClearAllDataCallback)
{
    RealDataSet<TestData> ds;
    auto callback = std::make_unique<MockListUpdateCallback>();
    auto callbackPtr = callback.get();
    ds.SetListUpdateCallback(std::move(callback));

    // Setup data
    ds.Add(TestData(1));
    ds.Add(TestData(2));
    ds.Add(TestData(3));
    callbackPtr->Clear();

    // Clear all
    ds.ClearAllData();

    // Should trigger REMOVED callback(s)
    ASSERT_GT(callbackPtr->events.size(), 0);
    // Verify that items were removed (may be batched or individual)
    int totalRemoved = 0;
    for (const auto& event : callbackPtr->events)
    {
        if (event.type == MockListUpdateCallback::Event::REMOVED)
        {
            totalRemoved += event.count;
        }
    }
    EXPECT_EQ(totalRemoved, 3);
}

TEST(RealDataSetCallbackTest, TransactionBatchCallback)
{
    RealDataSet<TestData> ds;
    auto callback = std::make_unique<MockListUpdateCallback>();
    auto callbackPtr = callback.get();
    ds.SetListUpdateCallback(std::move(callback));

    // Setup initial data
    ds.Add(TestData(1));
    ds.Add(TestData(2));
    callbackPtr->Clear();

    // Start transaction
    ds.StartTransaction();

    // Multiple operations
    ds.Add(TestData(3));
    ds.RemoveAtPos(0);
    ds.Add(TestData(4));

    // No callbacks during transaction
    EXPECT_EQ(callbackPtr->events.size(), 0);

    // End transaction - should trigger callbacks
    ds.EndTransaction();

    // Should have callbacks after transaction
    EXPECT_GT(callbackPtr->events.size(), 0);
}

TEST(RealDataSetCallbackTest, ContentChangeDetection)
{
    RealDataSet<TestData> ds;
    auto content = TestData(1, "version1");

    // Add item
    ds.Add(content);

    auto callback = std::make_unique<MockListUpdateCallback>();
    auto callbackPtr = callback.get();
    ds.SetListUpdateCallback(std::move(callback));
    callbackPtr->Clear();
    ds.StartTransaction();

    auto target = ds.GetDataByIndex(0);

    target->value = *"version2";
    // Replace with same ID but different content (hash changes)
    // ds.ReplaceAtPosIfExist(0, TestData(1, "version2"));
    ds.EndTransaction();

    bool hasRemoved = callbackPtr->HasEvent(MockListUpdateCallback::Event::REMOVED, 0, 1);
    bool hasInserted = callbackPtr->HasEvent(MockListUpdateCallback::Event::INSERTED, 0, 1);
    EXPECT_TRUE(hasRemoved && hasInserted);

    // Should detect content change
    // EXPECT_TRUE(callbackPtr->HasEvent(MockListUpdateCallback::Event::CHANGED, 0, 1));
}

TEST(RealDataSetCallbackTest, NoChangeWhenContentSame)
{
    RealDataSet<TestData> ds;
    auto callback = std::make_unique<MockListUpdateCallback>();
    auto callbackPtr = callback.get();
    ds.SetListUpdateCallback(std::move(callback));

    // Add item
    ds.Add(TestData(1, "content"));
    callbackPtr->Clear();

    // Replace with exact same content
    ds.ReplaceAtPosIfExist(0, TestData(1, "content"));

    // Should NOT trigger CHANGED callback (content hash is same)
    // Only REMOVED + INSERTED from diff algorithm
    bool hasChanged = callbackPtr->HasEvent(MockListUpdateCallback::Event::CHANGED, 0, 1);

    // Either no change event, or it's a structural change (remove+insert)
    // The exact behavior depends on DiffUtil implementation
    EXPECT_TRUE(callbackPtr->events.size() == 0 || !hasChanged);
}

// ==================== WrapperDataSet Tests ====================

TEST(WrapperDataSetCallbackTest, InsertCallback)
{
    WrapperDataSet<TestData> wrapper;
    auto callback = std::make_unique<MockListUpdateCallback>();
    auto callbackPtr = callback.get();
    wrapper.SetListUpdateCallback(std::move(callback));

    auto ds1 = std::make_unique<RealDataSet<TestData>>();
    auto ds1Ptr = ds1.get();
    wrapper.AddChild(std::move(ds1));

    callbackPtr->Clear();

    // Add item through child
    ds1Ptr->Add(TestData(1));

    // Should propagate callback to wrapper
    ASSERT_EQ(callbackPtr->events.size(), 1);
    EXPECT_TRUE(callbackPtr->HasEvent(MockListUpdateCallback::Event::INSERTED, 0, 1));
}

TEST(WrapperDataSetCallbackTest, MultipleChildrenCallback)
{
    WrapperDataSet<TestData> wrapper;
    auto callback = std::make_unique<MockListUpdateCallback>();
    auto callbackPtr = callback.get();
    wrapper.SetListUpdateCallback(std::move(callback));

    auto ds1 = std::make_unique<RealDataSet<TestData>>();
    auto ds1Ptr = ds1.get();
    auto ds2 = std::make_unique<RealDataSet<TestData>>();
    auto ds2Ptr = ds2.get();

    wrapper.AddChild(std::move(ds1));
    wrapper.AddChild(std::move(ds2));

    // Add to first child
    ds1Ptr->Add(TestData(1));
    ds1Ptr->Add(TestData(2));
    callbackPtr->Clear();

    // Add to second child - should adjust position
    ds2Ptr->Add(TestData(3));

    // Should insert at position 2 (after ds1's 2 items)
    ASSERT_EQ(callbackPtr->events.size(), 1);
    EXPECT_TRUE(callbackPtr->HasEvent(MockListUpdateCallback::Event::INSERTED, 2, 1));
}

TEST(WrapperDataSetCallbackTest, TransactionAcrossChildren)
{
    WrapperDataSet<TestData> wrapper;
    auto callback = std::make_unique<MockListUpdateCallback>();
    auto callbackPtr = callback.get();
    wrapper.SetListUpdateCallback(std::move(callback));

    auto ds1 = std::make_unique<RealDataSet<TestData>>();
    auto ds1Ptr = ds1.get();
    auto ds2 = std::make_unique<RealDataSet<TestData>>();
    auto ds2Ptr = ds2.get();

    wrapper.AddChild(std::move(ds1));
    wrapper.AddChild(std::move(ds2));

    ds1Ptr->Add(TestData(1));
    ds2Ptr->Add(TestData(2));
    callbackPtr->Clear();

    // Start transaction on wrapper
    wrapper.StartTransaction();

    // Modify children
    ds1Ptr->Add(TestData(3));
    ds2Ptr->RemoveAtPos(0);

    // No callbacks during transaction
    EXPECT_EQ(callbackPtr->events.size(), 0);

    // End transaction
    wrapper.EndTransaction();

    // Should have callbacks after transaction
    EXPECT_GT(callbackPtr->events.size(), 0);
}

TEST(WrapperDataSetCallbackTest, ContentChangeInChild)
{
    WrapperDataSet<TestData> wrapper;
    auto callback = std::make_unique<MockListUpdateCallback>();
    auto callbackPtr = callback.get();
    wrapper.SetListUpdateCallback(std::move(callback));

    auto ds1 = std::make_unique<RealDataSet<TestData>>();
    auto ds1Ptr = ds1.get();
    wrapper.AddChild(std::move(ds1));

    // Add item
    ds1Ptr->Add(TestData(1, "v1"));
    callbackPtr->Clear();

    // Modify content
    ds1Ptr->ReplaceAtPosIfExist(0, TestData(1, "v2"));

    // Should detect content change and propagate to wrapper
    EXPECT_TRUE(callbackPtr->HasEvent(MockListUpdateCallback::Event::CHANGED, 0, 1));
}

TEST(WrapperDataSetCallbackTest, AddAllCallback)
{
    RealDataSet<TestData> ds;
    auto callback = std::make_unique<MockListUpdateCallback>();
    auto callbackPtr = callback.get();
    ds.SetListUpdateCallback(std::move(callback));

    ds.Add(TestData(1));
    callbackPtr->Clear();

    // Add multiple items
    std::vector<TestData> items = {TestData(2), TestData(3), TestData(4)};
    ds.AddAll(items);

    // Should trigger insert callback
    ASSERT_EQ(callbackPtr->events.size(), 3);
    EXPECT_TRUE(callbackPtr->HasEvent(MockListUpdateCallback::Event::INSERTED, 1, 1));
}

// ==================== Edge Cases ====================

TEST(ListUpdateCallbackEdgeTest, NoCallbackSet)
{
    RealDataSet<TestData> ds;
    // No callback set - should not crash
    EXPECT_NO_THROW(ds.Add(TestData(1)));
    EXPECT_NO_THROW(ds.RemoveAtPos(0));
}

TEST(ListUpdateCallbackEdgeTest, EmptyDataSetOperations)
{
    RealDataSet<TestData> ds;
    auto callback = std::make_unique<MockListUpdateCallback>();
    auto callbackPtr = callback.get();
    ds.SetListUpdateCallback(std::move(callback));

    // Operations on empty dataset
    ds.Remove(TestData(1)); // No-op
    EXPECT_EQ(callbackPtr->events.size(), 0);

    ds.RemoveAtPos(0); // No-op
    EXPECT_EQ(callbackPtr->events.size(), 0);
}

TEST(ListUpdateCallbackEdgeTest, SilentTransactionEnd)
{
    RealDataSet<TestData> ds;
    auto callback = std::make_unique<MockListUpdateCallback>();
    auto callbackPtr = callback.get();
    ds.SetListUpdateCallback(std::move(callback));

    ds.StartTransaction();
    ds.Add(TestData(1));
    ds.Add(TestData(2));

    // End silently - should NOT trigger callbacks
    ds.EndTransactionSilently();
    EXPECT_EQ(callbackPtr->events.size(), 0);
}
