#include <gtest/gtest.h>
#include "pandora/wrapper_data_set.h"
#include "pandora/real_data_set.h"
#include "Global.h"
#include <memory>

using namespace pandora;

TEST(WrapperDataSetTest, AggregateOperations) {

    auto ds1 = std::make_unique<RealDataSet<TestData>>();
    auto ds1Ptr = ds1.get();

    auto ds2 = std::make_unique<RealDataSet<TestData>>();
    auto ds2Ptr = ds2.get();

    WrapperDataSet<TestData> wrapper;
    wrapper.AddChild(std::move(ds1));
    wrapper.AddChild(std::move(ds2));


    auto test1 = TestData(1);
    auto test2 = TestData(2);
    ds1Ptr->Add(test1);
    ds1Ptr->Add(test2);

    auto test3 = TestData(3);
    auto test4 = TestData(4);

    ds2Ptr->Add(test3);
    ds2Ptr->Add(test4);


    EXPECT_EQ(wrapper.GetDataCount(), 4);
    EXPECT_EQ(wrapper.GetDataByIndex(0)->value, 1);
    EXPECT_EQ(wrapper.GetDataByIndex(3)->value, 4);
    wrapper.Remove(test2);
    EXPECT_EQ(wrapper.GetDataCount(), 3);
    EXPECT_EQ(wrapper.IndexOf(test3), 1);
}
