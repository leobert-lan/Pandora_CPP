#include <gtest/gtest.h>
#include "pandora/real_data_set.h"
#include "pandora/wrapper_data_set.h"
#include "pandora/transaction.h"
#include "pandora/pandora_exception.h"
#include "Global.h"

using namespace pandora;

// Test transaction mechanism
TEST(TransactionTest, BasicTransaction) {
  RealDataSet<TestData> dataset;
  dataset.Add(TestData(1));
  dataset.Add(TestData(2));
  dataset.Add(TestData(3));

  Transaction<TestData> transaction(&dataset);

  // Apply transaction successfully
  transaction.Apply([](PandoraBoxAdapter<TestData>* adapter) {
    adapter->Add(TestData(4));
    adapter->Add(TestData(5));
  });

  EXPECT_EQ(5, dataset.GetDataCount());
  EXPECT_EQ(4, dataset.GetDataByIndex(3)->value);
  EXPECT_EQ(5, dataset.GetDataByIndex(4)->value);
}

TEST(TransactionTest, TransactionRollback) {
  RealDataSet<TestData> dataset;
  dataset.Add(TestData(1));
  dataset.Add(TestData(2));
  dataset.Add(TestData(3));

  Transaction<TestData> transaction(&dataset);

  // Apply transaction that throws exception
  try {
    transaction.Apply([](PandoraBoxAdapter<TestData>* adapter) {
      adapter->Add(TestData(4));
      throw std::runtime_error("Test exception");
    });
  } catch (...) {
    // Exception is caught inside transaction
  }

  // Data should be restored to original state
  EXPECT_EQ(3, dataset.GetDataCount());
}

TEST(TransactionTest, ManualTransaction) {
  RealDataSet<TestData> dataset;
  dataset.Add(TestData(1));
  dataset.Add(TestData(2));

  // Start transaction manually
  dataset.StartTransaction();
  dataset.Add(TestData(3));
  dataset.Add(TestData(4));
  EXPECT_TRUE(dataset.InTransaction());

  // End transaction
  dataset.EndTransaction();
  EXPECT_FALSE(dataset.InTransaction());

  EXPECT_EQ(4, dataset.GetDataCount());
}

TEST(TransactionTest, SilentTransaction) {
  RealDataSet<TestData> dataset;
  dataset.Add(TestData(1));
  dataset.Add(TestData(2));

  dataset.StartTransaction();
  dataset.Add(TestData(3));

  // End transaction silently (no notification)
  dataset.EndTransactionSilently();
  EXPECT_FALSE(dataset.InTransaction());

  EXPECT_EQ(3, dataset.GetDataCount());
}

// Test alias mechanism
TEST(AliasTest, SetAndGetAlias) {
  RealDataSet<TestData> dataset;
  dataset.SetAlias("myDataset");

  EXPECT_EQ("myDataset", dataset.GetAlias());
}

TEST(AliasTest, FindByAlias) {
  RealDataSet<TestData> dataset;
  dataset.SetAlias("test");

  auto* found = dataset.FindByAlias("test");
  EXPECT_NE(nullptr, found);
  EXPECT_EQ(&dataset, found);

  auto* notFound = dataset.FindByAlias("other");
  EXPECT_EQ(nullptr, notFound);
}

TEST(AliasTest, AliasConflictInSingleNode) {
  RealDataSet<TestData> dataset;
  dataset.SetAlias("test");

  EXPECT_TRUE(dataset.IsAliasConflict("test"));
  EXPECT_FALSE(dataset.IsAliasConflict("other"));
}

TEST(AliasTest, AliasConflictInTree) {
  auto wrapper = std::make_unique<WrapperDataSet<TestData>>();
  wrapper->SetAlias("wrapper");

  auto dataset1 = std::make_unique<RealDataSet<TestData>>();
  dataset1->SetAlias("child1");

  auto dataset2 = std::make_unique<RealDataSet<TestData>>();
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
  auto wrapper = std::make_unique<WrapperDataSet<TestData>>();
  wrapper->SetAlias("wrapper");

  auto dataset1 = std::make_unique<RealDataSet<TestData>>();
  // Save raw pointer before moving
  auto* dataset1Ptr = dataset1.get();
  wrapper->AddChild(std::move(dataset1));
  dataset1Ptr->SetAlias("child1");

  // Adding child2 to wrapper, then trying to set conflicting alias should throw
  auto dataset2 = std::make_unique<RealDataSet<TestData>>();
  auto* dataset2Ptr = dataset2.get();
  wrapper->AddChild(std::move(dataset2));

  // This should throw because "child1" alias already exists in the tree
  EXPECT_THROW(dataset2Ptr->SetAlias("child1"), PandoraException);
}

TEST(AliasTest, FindByAliasInTree) {
  auto wrapper = std::make_unique<WrapperDataSet<TestData>>();
  wrapper->SetAlias("root");

  auto dataset1 = std::make_unique<RealDataSet<TestData>>();
  dataset1->SetAlias("child1");
  dataset1->Add(TestData(100));

  auto dataset2 = std::make_unique<RealDataSet<TestData>>();
  dataset2->SetAlias("child2");
  dataset2->Add(TestData(200));

  wrapper->AddChild(std::move(dataset1));
  wrapper->AddChild(std::move(dataset2));

  // Find root
  auto* found = wrapper->FindByAlias("root");
  EXPECT_NE(nullptr, found);
  EXPECT_EQ(wrapper.get(), found);

  // Find child1
  found = wrapper->FindByAlias("child1");
  EXPECT_NE(nullptr, found);
  EXPECT_EQ(100, found->GetDataByIndex(0)->value);

  // Find child2
  found = wrapper->FindByAlias("child2");
  EXPECT_NE(nullptr, found);
  EXPECT_EQ(200, found->GetDataByIndex(0)->value);

  // Not found
  found = wrapper->FindByAlias("nonexistent");
  EXPECT_EQ(nullptr, found);
}

// Test transaction with parent-child relationship
TEST(TransactionTest, ParentChildTransaction) {
  auto wrapper = std::make_unique<WrapperDataSet<TestData>>();
  auto dataset = std::make_unique<RealDataSet<TestData>>();

  auto* datasetPtr = dataset.get();
  wrapper->AddChild(std::move(dataset));

  // Start transaction on parent
  wrapper->StartTransaction();
  EXPECT_TRUE(wrapper->InTransaction());
  EXPECT_TRUE(datasetPtr->InTransaction());  // Child should be in transaction too

  datasetPtr->Add(TestData(1));
  datasetPtr->Add(TestData(2));

  wrapper->EndTransaction();
  EXPECT_FALSE(wrapper->InTransaction());
  EXPECT_FALSE(datasetPtr->InTransaction());

  EXPECT_EQ(2, datasetPtr->GetDataCount());
}

TEST(TransactionTest, ChildInheritsParentTransaction) {
  auto wrapper = std::make_unique<WrapperDataSet<TestData>>();
  auto dataset = std::make_unique<RealDataSet<TestData>>();

  auto* datasetPtr = dataset.get();
  wrapper->AddChild(std::move(dataset));

  // Start transaction on parent
  wrapper->StartTransaction();

  // Child operations should not trigger immediate notifications
  datasetPtr->Add(TestData(1));
  EXPECT_TRUE(datasetPtr->InTransaction());

  // End transaction
  wrapper->EndTransaction();
  EXPECT_FALSE(datasetPtr->InTransaction());

  EXPECT_EQ(1, datasetPtr->GetDataCount());
}

