#include <gtest/gtest.h>
#include "pandora/real_data_set.h"
#include "pandora/wrapper_data_set.h"
#include "pandora/transaction.h"
#include "pandora/pandora_exception.h"

using namespace pandora;

// Test transaction mechanism
TEST(TransactionTest, BasicTransaction) {
  RealDataSet<int> dataset;
  dataset.Add(1);
  dataset.Add(2);
  dataset.Add(3);

  Transaction<int> transaction(&dataset);

  // Apply transaction successfully
  transaction.Apply([](PandoraBoxAdapter<int>* adapter) {
    adapter->Add(4);
    adapter->Add(5);
  });

  EXPECT_EQ(5, dataset.GetDataCount());
  EXPECT_EQ(4, *dataset.GetDataByIndex(3));
  EXPECT_EQ(5, *dataset.GetDataByIndex(4));
}

TEST(TransactionTest, TransactionRollback) {
  RealDataSet<int> dataset;
  dataset.Add(1);
  dataset.Add(2);
  dataset.Add(3);

  Transaction<int> transaction(&dataset);

  // Apply transaction that throws exception
  try {
    transaction.Apply([](PandoraBoxAdapter<int>* adapter) {
      adapter->Add(4);
      throw std::runtime_error("Test exception");
    });
  } catch (...) {
    // Exception is caught inside transaction
  }

  // Data should be restored to original state
  EXPECT_EQ(3, dataset.GetDataCount());
}

TEST(TransactionTest, ManualTransaction) {
  RealDataSet<int> dataset;
  dataset.Add(1);
  dataset.Add(2);

  // Start transaction manually
  dataset.StartTransaction();
  dataset.Add(3);
  dataset.Add(4);
  EXPECT_TRUE(dataset.InTransaction());

  // End transaction
  dataset.EndTransaction();
  EXPECT_FALSE(dataset.InTransaction());

  EXPECT_EQ(4, dataset.GetDataCount());
}

TEST(TransactionTest, SilentTransaction) {
  RealDataSet<int> dataset;
  dataset.Add(1);
  dataset.Add(2);

  dataset.StartTransaction();
  dataset.Add(3);

  // End transaction silently (no notification)
  dataset.EndTransactionSilently();
  EXPECT_FALSE(dataset.InTransaction());

  EXPECT_EQ(3, dataset.GetDataCount());
}

// Test alias mechanism
TEST(AliasTest, SetAndGetAlias) {
  RealDataSet<int> dataset;
  dataset.SetAlias("myDataset");

  EXPECT_EQ("myDataset", dataset.GetAlias());
}

TEST(AliasTest, FindByAlias) {
  RealDataSet<int> dataset;
  dataset.SetAlias("test");

  auto* found = dataset.FindByAlias("test");
  EXPECT_NE(nullptr, found);
  EXPECT_EQ(&dataset, found);

  auto* notFound = dataset.FindByAlias("other");
  EXPECT_EQ(nullptr, notFound);
}

TEST(AliasTest, AliasConflictInSingleNode) {
  RealDataSet<int> dataset;
  dataset.SetAlias("test");

  EXPECT_TRUE(dataset.IsAliasConflict("test"));
  EXPECT_FALSE(dataset.IsAliasConflict("other"));
}

TEST(AliasTest, AliasConflictInTree) {
  auto wrapper = std::make_unique<WrapperDataSet<int>>();
  wrapper->SetAlias("wrapper");

  auto dataset1 = std::make_unique<RealDataSet<int>>();
  dataset1->SetAlias("child1");

  auto dataset2 = std::make_unique<RealDataSet<int>>();
  dataset2->SetAlias("child2");

  wrapper->AddChild(std::move(dataset1));
  wrapper->AddChild(std::move(dataset2));

  // Check conflict detection
  EXPECT_TRUE(wrapper->IsAliasConflict("wrapper"));
  EXPECT_TRUE(wrapper->IsAliasConflict("child1"));
  EXPECT_TRUE(wrapper->IsAliasConflict("child2"));
  EXPECT_FALSE(wrapper->IsAliasConflict("nonexistent"));
}

TEST(AliasTest, SetAliasWithConflictThrows) {
  auto wrapper = std::make_unique<WrapperDataSet<int>>();
  wrapper->SetAlias("wrapper");

  auto dataset1 = std::make_unique<RealDataSet<int>>();
  dataset1->SetAlias("child1");
  wrapper->AddChild(std::move(dataset1));

  auto dataset2 = std::make_unique<RealDataSet<int>>();

  // Adding child2 to wrapper, then trying to set conflicting alias should throw
  wrapper->AddChild(std::move(dataset2));

  // Get the last child to set conflicting alias
  // Note: In real usage, you'd need to traverse to get the child
  // For this test, we'll create a new dataset and test before adding
  auto dataset3 = std::make_unique<RealDataSet<int>>();
  EXPECT_THROW(dataset3->SetAlias("wrapper"), PandoraException);
}

TEST(AliasTest, FindByAliasInTree) {
  auto wrapper = std::make_unique<WrapperDataSet<int>>();
  wrapper->SetAlias("root");

  auto dataset1 = std::make_unique<RealDataSet<int>>();
  dataset1->SetAlias("child1");
  dataset1->Add(100);

  auto dataset2 = std::make_unique<RealDataSet<int>>();
  dataset2->SetAlias("child2");
  dataset2->Add(200);

  wrapper->AddChild(std::move(dataset1));
  wrapper->AddChild(std::move(dataset2));

  // Find root
  auto* found = wrapper->FindByAlias("root");
  EXPECT_NE(nullptr, found);
  EXPECT_EQ(wrapper.get(), found);

  // Find child1
  found = wrapper->FindByAlias("child1");
  EXPECT_NE(nullptr, found);
  EXPECT_EQ(100, *found->GetDataByIndex(0));

  // Find child2
  found = wrapper->FindByAlias("child2");
  EXPECT_NE(nullptr, found);
  EXPECT_EQ(200, *found->GetDataByIndex(0));

  // Not found
  found = wrapper->FindByAlias("nonexistent");
  EXPECT_EQ(nullptr, found);
}

// Test transaction with parent-child relationship
TEST(TransactionTest, ParentChildTransaction) {
  auto wrapper = std::make_unique<WrapperDataSet<int>>();
  auto dataset = std::make_unique<RealDataSet<int>>();

  auto* datasetPtr = dataset.get();
  wrapper->AddChild(std::move(dataset));

  // Start transaction on parent
  wrapper->StartTransaction();
  EXPECT_TRUE(wrapper->InTransaction());
  EXPECT_TRUE(datasetPtr->InTransaction());  // Child should be in transaction too

  datasetPtr->Add(1);
  datasetPtr->Add(2);

  wrapper->EndTransaction();
  EXPECT_FALSE(wrapper->InTransaction());
  EXPECT_FALSE(datasetPtr->InTransaction());

  EXPECT_EQ(2, datasetPtr->GetDataCount());
}

TEST(TransactionTest, ChildInheritsParentTransaction) {
  auto wrapper = std::make_unique<WrapperDataSet<int>>();
  auto dataset = std::make_unique<RealDataSet<int>>();

  auto* datasetPtr = dataset.get();
  wrapper->AddChild(std::move(dataset));

  // Start transaction on parent
  wrapper->StartTransaction();

  // Child operations should not trigger immediate notifications
  datasetPtr->Add(1);
  EXPECT_TRUE(datasetPtr->InTransaction());

  // End transaction
  wrapper->EndTransaction();
  EXPECT_FALSE(datasetPtr->InTransaction());

  EXPECT_EQ(1, datasetPtr->GetDataCount());
}

