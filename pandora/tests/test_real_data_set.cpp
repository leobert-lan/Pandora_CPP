#include <gtest/gtest.h>
#include "pandora/real_data_set.h"
#include "pandora/pandora_exception.h"
#include "Global.h"

using namespace pandora;

TEST(RealDataSetTest, BasicOperations) {
    RealDataSet<TestData> ds;
    EXPECT_EQ(ds.GetDataCount(), 0);
    ds.Add(TestData(1));
    ds.Add(TestData(2));
    ds.Add(TestData(3));
    EXPECT_EQ(ds.GetDataCount(), 3);
    EXPECT_EQ(ds.GetDataByIndex(0)->value, 1);
    EXPECT_EQ(ds.GetDataByIndex(2)->value, 3);
    ds.Remove(TestData(2));
    EXPECT_EQ(ds.GetDataCount(), 2);
    EXPECT_EQ(ds.IndexOf(TestData(3)), 1);
    ds.ReplaceAtPosIfExist(1, TestData(5));
    EXPECT_EQ(ds.GetDataByIndex(1)->value, 5);
    ds.ClearAllData();
    EXPECT_EQ(ds.GetDataCount(), 0);
}

TEST(RealDataSetTest, ExceptionOnAddChild) {
    RealDataSet<TestData> ds;
    EXPECT_THROW(ds.AddChild(nullptr), PandoraException);
}

