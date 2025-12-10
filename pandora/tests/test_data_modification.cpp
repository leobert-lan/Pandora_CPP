#include <gtest/gtest.h>
#include <string>

#include "pandora/transaction.h"

using namespace pandora;

// Test data structure
struct Person {
  std::string name;
  int age;

  Person(std::string n, int a) : name(std::move(n)), age(a) {}
  Person() : age(0) {}



  bool operator==(const Person& other) const {
    return name == other.name && age == other.age;
  }
};
//
// TEST(DataModificationTest, UpdateAtSingleElement) {
//   RealDataSet<Person> dataset;
//   dataset.Add(Person("Alice", 25));
//   dataset.Add(Person("Bob", 30));
//   dataset.Add(Person("Charlie", 35));
//
//   bool success = dataset.UpdateAt(1, [](Person& p) {
//     p.age = 31;  // Update Bob's age
//   });
//
//   EXPECT_TRUE(success);
//   EXPECT_EQ(31, dataset.GetDataByIndex(1)->age);
//   EXPECT_EQ("Bob", dataset.GetDataByIndex(1)->name);
// }
//
// // 测试2：UpdateAll - 批量更新所有元素
// TEST(DataModificationTest, UpdateAllElements) {
//   RealDataSet<Person> dataset;
//   dataset.Add(Person("Alice", 25));
//   dataset.Add(Person("Bob", 30));
//   dataset.Add(Person("Charlie", 35));
//
//   // 所有人都长大一岁
//   dataset.UpdateAll([](Person& p) {
//     p.age += 1;
//   });
//
//   EXPECT_EQ(26, dataset.GetDataByIndex(0)->age);
//   EXPECT_EQ(31, dataset.GetDataByIndex(1)->age);
//   EXPECT_EQ(36, dataset.GetDataByIndex(2)->age);
// }
//
// // 测试3：UpdateIf - 条件更新
// TEST(DataModificationTest, UpdateIfCondition) {
//   RealDataSet<Person> dataset;
//   dataset.Add(Person("Alice", 25));
//   dataset.Add(Person("Bob", 30));
//   dataset.Add(Person("Charlie", 35));
//   dataset.Add(Person("David", 40));
//
//   // 只给30岁以上的人加薪（这里用名字后缀表示）
//   int updated = dataset.UpdateIf(
//     [](const Person& p) { return p.age >= 30; },  // 条件
//     [](Person& p) { p.name += "_Senior"; }        // 更新操作
//   );
//
//   EXPECT_EQ(3, updated);  // 更新了3个人
//   EXPECT_EQ("Alice", dataset.GetDataByIndex(0)->name);  // Alice未更新
//   EXPECT_EQ("Bob_Senior", dataset.GetDataByIndex(1)->name);
//   EXPECT_EQ("Charlie_Senior", dataset.GetDataByIndex(2)->name);
//   EXPECT_EQ("David_Senior", dataset.GetDataByIndex(3)->name);
// }
//
// // 测试4：GetMutableDataByIndex - 直接获取可修改引用
// TEST(DataModificationTest, DirectModificationWithNotify) {
//   RealDataSet<Person> dataset;
//   dataset.Add(Person("Alice", 25));
//   dataset.Add(Person("Bob", 30));
//
//   // 方式1：直接修改后手动通知
//   Person* person = dataset.GetMutableDataByIndex(0);
//   ASSERT_NE(nullptr, person);
//   person->age = 26;
//   person->name = "Alice Smith";
//   dataset.NotifyDataChanged();  // 手动触发通知
//
//   EXPECT_EQ("Alice Smith", dataset.GetDataByIndex(0)->name);
//   EXPECT_EQ(26, dataset.GetDataByIndex(0)->age);
// }
//
// // 测试5：在Transaction中使用Update
// TEST(DataModificationTest, UpdateInTransaction) {
//   RealDataSet<Person> dataset;
//   dataset.Add(Person("Alice", 25));
//   dataset.Add(Person("Bob", 30));
//
//   Transaction<Person> transaction(&dataset);
//
//   // 在事务中批量更新
//   transaction.Apply([](PandoraBoxAdapter<Person>* adapter) {
//     auto* realDataset = static_cast<RealDataSet<Person>*>(adapter);
//     realDataset->UpdateAll([](Person& p) {
//       p.age += 10;
//     });
//   });
//
//   EXPECT_EQ(35, dataset.GetDataByIndex(0)->age);
//   EXPECT_EQ(40, dataset.GetDataByIndex(1)->age);
// }
//
// // 测试6：对比值拷贝的场景
// TEST(DataModificationTest, CompareValueCopyVsUpdate) {
//   RealDataSet<Person> dataset;
//   dataset.Add(Person("Alice", 25));
//
//   // 旧方式：需要完整拷贝
//   Person p = *dataset.GetDataByIndex(0);  // 拷贝
//   p.age = 26;
//   dataset.ReplaceAtPosIfExist(0, p);  // 再次拷贝
//
//   EXPECT_EQ(26, dataset.GetDataByIndex(0)->age);
//
//   // 新方式：直接修改，无需拷贝
//   dataset.UpdateAt(0, [](Person& p) {
//     p.age = 27;  // 直接在原地修改
//   });
//
//   EXPECT_EQ(27, dataset.GetDataByIndex(0)->age);
// }
//
// // 测试7：复杂对象的修改
// struct ComplexData {
//   std::vector<int> values;
//   std::string description;
//
//   void addValue(int v) { values.push_back(v); }
// };
//
// TEST(DataModificationTest, ComplexObjectModification) {
//   RealDataSet<ComplexData> dataset;
//   ComplexData data;
//   data.description = "Numbers";
//   dataset.Add(data);
//
//   // 使用UpdateAt添加数据到vector
//   dataset.UpdateAt(0, [](ComplexData& d) {
//     d.addValue(1);
//     d.addValue(2);
//     d.addValue(3);
//   });
//
//   EXPECT_EQ(3, dataset.GetDataByIndex(0)->values.size());
//   EXPECT_EQ(1, dataset.GetDataByIndex(0)->values[0]);
// }

