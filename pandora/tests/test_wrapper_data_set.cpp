#include <gtest/gtest.h>
#include "pandora/wrapper_data_set.h"
#include "pandora/real_data_set.h"
#include <memory.h>

using namespace pandora;

TEST(WrapperDataSetTest, AggregateOperations) {
    auto ds1 = std::make_unique<RealDataSet<int>>();
    ds1->Add(1);
    ds1->Add(2);
    auto ds2 = std::make_unique<RealDataSet<int>>();
    ds2->Add(3);
    ds2->Add(4);
    WrapperDataSet<int> wrapper;
    wrapper.AddChild(std::move(ds1));
    wrapper.AddChild(std::move(ds2));
    EXPECT_EQ(wrapper.GetDataCount(), 4);
    EXPECT_EQ(*wrapper.GetDataByIndex(0), 1);
    EXPECT_EQ(*wrapper.GetDataByIndex(3), 4);
    wrapper.Remove(2);
    EXPECT_EQ(wrapper.GetDataCount(), 3);
    EXPECT_EQ(wrapper.IndexOf(3), 1);
}
