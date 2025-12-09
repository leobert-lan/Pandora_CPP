#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "pandora/diff_util.h"
#include "pandora/list_update_callback.h"
#include "pandora/diff_callback.h"

using namespace pandora;

// Test data structure
struct TestItem {
  int id;
  std::string name;

  TestItem(int i, const std::string& n) : id(i), name(n) {}

  bool operator==(const TestItem& other) const {
    return id == other.id && name == other.name;
  }
};

// Test DiffCallback implementation
class TestDiffCallback : public DiffCallback {
 public:
  TestDiffCallback(const std::vector<TestItem>& old_list,
                   const std::vector<TestItem>& new_list)
      : old_list_(old_list), new_list_(new_list) {}

  int GetOldListSize() const override {
    return static_cast<int>(old_list_.size());
  }

  int GetNewListSize() const override {
    return static_cast<int>(new_list_.size());
  }

  bool AreItemsTheSame(int old_item_position, int new_item_position) const override {
    return old_list_[old_item_position].id == new_list_[new_item_position].id;
  }

  bool AreContentsTheSame(int old_item_position, int new_item_position) const override {
    return old_list_[old_item_position] == new_list_[new_item_position];
  }

 private:
  const std::vector<TestItem>& old_list_;
  const std::vector<TestItem>& new_list_;
};

// Test ListUpdateCallback implementation
class TestListUpdateCallback : public ListUpdateCallback {
 public:
  struct Update {
    enum Type { INSERT, REMOVE, MOVE, CHANGE };
    Type type;
    int position;
    int count;
    int to_position;  // For moves

    Update(Type t, int pos, int cnt = 1, int to = -1)
        : type(t), position(pos), count(cnt), to_position(to) {}
  };

  void OnInserted(int position, int count) override {
    updates.emplace_back(Update::INSERT, position, count);
  }

  void OnRemoved(int position, int count) override {
    updates.emplace_back(Update::REMOVE, position, count);
  }

  void OnMoved(int from_position, int to_position) override {
    updates.emplace_back(Update::MOVE, from_position, 1, to_position);
  }

  void OnChanged(int position, int count, void* payload) override {
    updates.emplace_back(Update::CHANGE, position, count);
  }

  std::vector<Update> updates;
};

TEST(DiffUtilTest, BasicAddition) {
  std::vector<TestItem> old_list = {
      TestItem(1, "Item1"),
      TestItem(2, "Item2")
  };

  std::vector<TestItem> new_list = {
      TestItem(1, "Item1"),
      TestItem(2, "Item2"),
      TestItem(3, "Item3")
  };

  TestDiffCallback callback(old_list, new_list);
  auto result = DiffUtil::CalculateDiff(&callback);

  TestListUpdateCallback update_callback;
  result->DispatchUpdatesTo(&update_callback);

  ASSERT_EQ(update_callback.updates.size(), 1);
  EXPECT_EQ(update_callback.updates[0].type, TestListUpdateCallback::Update::INSERT);
  EXPECT_EQ(update_callback.updates[0].position, 2);
  EXPECT_EQ(update_callback.updates[0].count, 1);
}

TEST(DiffUtilTest, BasicRemoval) {
  std::vector<TestItem> old_list = {
      TestItem(1, "Item1"),
      TestItem(2, "Item2"),
      TestItem(3, "Item3")
  };

  std::vector<TestItem> new_list = {
      TestItem(1, "Item1"),
      TestItem(3, "Item3")
  };

  TestDiffCallback callback(old_list, new_list);
  auto result = DiffUtil::CalculateDiff(&callback);

  TestListUpdateCallback update_callback;
  result->DispatchUpdatesTo(&update_callback);

  ASSERT_EQ(update_callback.updates.size(), 1);
  EXPECT_EQ(update_callback.updates[0].type, TestListUpdateCallback::Update::REMOVE);
  EXPECT_EQ(update_callback.updates[0].position, 1);
  EXPECT_EQ(update_callback.updates[0].count, 1);
}

TEST(DiffUtilTest, BasicChange) {
  std::vector<TestItem> old_list = {
      TestItem(1, "Item1"),
      TestItem(2, "Item2")
  };

  std::vector<TestItem> new_list = {
      TestItem(1, "Item1"),
      TestItem(2, "Item2_Modified")
  };

  TestDiffCallback callback(old_list, new_list);
  auto result = DiffUtil::CalculateDiff(&callback);

  TestListUpdateCallback update_callback;
  result->DispatchUpdatesTo(&update_callback);

  ASSERT_EQ(update_callback.updates.size(), 1);
  EXPECT_EQ(update_callback.updates[0].type, TestListUpdateCallback::Update::CHANGE);
  EXPECT_EQ(update_callback.updates[0].position, 1);
  EXPECT_EQ(update_callback.updates[0].count, 1);
}

TEST(DiffUtilTest, BasicMove) {
  std::vector<TestItem> old_list = {
      TestItem(1, "Item1"),
      TestItem(2, "Item2"),
      TestItem(3, "Item3")
  };

  std::vector<TestItem> new_list = {
      TestItem(2, "Item2"),
      TestItem(1, "Item1"),
      TestItem(3, "Item3")
  };

  TestDiffCallback callback(old_list, new_list);
  auto result = DiffUtil::CalculateDiff(&callback, true);

  TestListUpdateCallback update_callback;
  result->DispatchUpdatesTo(&update_callback);

  // Should detect a move operation
  bool has_move = false;
  for (const auto& update : update_callback.updates) {
    if (update.type == TestListUpdateCallback::Update::MOVE) {
      has_move = true;
      break;
    }
  }
  EXPECT_TRUE(has_move);
}

TEST(DiffUtilTest, ComplexChanges) {
  std::vector<TestItem> old_list = {
      TestItem(1, "A"),
      TestItem(2, "B"),
      TestItem(3, "C"),
      TestItem(4, "D")
  };

  std::vector<TestItem> new_list = {
      TestItem(1, "A"),
      TestItem(3, "C_Modified"),
      TestItem(5, "E"),
      TestItem(4, "D")
  };

  TestDiffCallback callback(old_list, new_list);
  auto result = DiffUtil::CalculateDiff(&callback);

  TestListUpdateCallback update_callback;
  result->DispatchUpdatesTo(&update_callback);

  // Should have multiple updates
  EXPECT_GT(update_callback.updates.size(), 0);
}

TEST(DiffUtilTest, ConvertPositions) {
  std::vector<TestItem> old_list = {
      TestItem(1, "Item1"),
      TestItem(2, "Item2"),
      TestItem(3, "Item3")
  };

  std::vector<TestItem> new_list = {
      TestItem(1, "Item1"),
      TestItem(3, "Item3")
  };

  TestDiffCallback callback(old_list, new_list);
  auto result = DiffUtil::CalculateDiff(&callback);

  // Item at old position 0 should be at new position 0
  EXPECT_EQ(result->ConvertOldPositionToNew(0), 0);

  // Item at old position 1 was removed
  EXPECT_EQ(result->ConvertOldPositionToNew(1), DiffUtil::DiffResult::NO_POSITION);

  // Item at old position 2 should be at new position 1
  EXPECT_EQ(result->ConvertOldPositionToNew(2), 1);
}

TEST(DiffUtilTest, EmptyLists) {
  std::vector<TestItem> old_list;
  std::vector<TestItem> new_list;

  TestDiffCallback callback(old_list, new_list);
  auto result = DiffUtil::CalculateDiff(&callback);

  TestListUpdateCallback update_callback;
  result->DispatchUpdatesTo(&update_callback);

  // No updates expected for empty lists
  EXPECT_EQ(update_callback.updates.size(), 0);
}

TEST(DiffUtilTest, OldEmptyNewFilled) {
  std::vector<TestItem> old_list;
  std::vector<TestItem> new_list = {
      TestItem(1, "Item1"),
      TestItem(2, "Item2")
  };

  TestDiffCallback callback(old_list, new_list);
  auto result = DiffUtil::CalculateDiff(&callback);

  TestListUpdateCallback update_callback;
  result->DispatchUpdatesTo(&update_callback);

  // Should have insertions
  ASSERT_GT(update_callback.updates.size(), 0);
  EXPECT_EQ(update_callback.updates[0].type, TestListUpdateCallback::Update::INSERT);
}

TEST(DiffUtilTest, OldFilledNewEmpty) {
  std::vector<TestItem> old_list = {
      TestItem(1, "Item1"),
      TestItem(2, "Item2")
  };
  std::vector<TestItem> new_list;

  TestDiffCallback callback(old_list, new_list);
  auto result = DiffUtil::CalculateDiff(&callback);

  TestListUpdateCallback update_callback;
  result->DispatchUpdatesTo(&update_callback);

  // Should have removals
  ASSERT_GT(update_callback.updates.size(), 0);
  EXPECT_EQ(update_callback.updates[0].type, TestListUpdateCallback::Update::REMOVE);
}

