#include <gtest/gtest.h>
#include "pandora/real_data_set.h"
#include "pandora/pandora_exception.h"

using namespace pandora;

TEST(RealDataSetTest, BasicOperations) {
    RealDataSet<int> ds;
    EXPECT_EQ(ds.GetDataCount(), 0);
    ds.Add(1);
    ds.Add(2);
    ds.Add(3);
    EXPECT_EQ(ds.GetDataCount(), 3);
    EXPECT_EQ(*ds.GetDataByIndex(0), 1);
    EXPECT_EQ(*ds.GetDataByIndex(2), 3);
    ds.Remove(2);
    EXPECT_EQ(ds.GetDataCount(), 2);
    EXPECT_EQ(ds.IndexOf(3), 1);
    ds.ReplaceAtPosIfExist(1, 5);
    EXPECT_EQ(*ds.GetDataByIndex(1), 5);
    ds.ClearAllData();
    EXPECT_EQ(ds.GetDataCount(), 0);
}

TEST(RealDataSetTest, ExceptionOnAddChild) {
    RealDataSet<int> ds;
    EXPECT_THROW(ds.AddChild(nullptr), PandoraException);
}

