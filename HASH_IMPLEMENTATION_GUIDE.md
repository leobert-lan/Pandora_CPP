# 如何为自定义类型实现哈希函数

## 概述

在 Pandora-CPP 中使用 `WrapperDataSet<T>` 时，类型 `T` 必须提供哈希函数以支持内容变化检测。本指南介绍三种实现方式。

## 方案一：在类中实现 `Hash()` 成员函数（推荐）

这是最简单直接的方式，类型系统会自动检测并使用。

```cpp
class Person {
public:
    std::string name;
    int age;
    std::string address;
    
    // 实现 Hash() 成员函数
    size_t Hash() const {
        size_t seed = 0;
        pandora::HashCombine(seed, name);
        pandora::HashCombine(seed, age);
        pandora::HashCombine(seed, address);
        return seed;
    }
    
    // 同时也需要实现 operator==
    bool operator==(const Person& other) const {
        return name == other.name && age == other.age && address == other.address;
    }
};
```

## 方案二：特化 `ContentHasher` 模板（适用于无法修改原类）

当无法修改原始类定义时，可以在外部特化 `ContentHasher`。

```cpp
#include "pandora/pandora_traits.h"

class ThirdPartyClass {
    // 假设这是第三方库的类，无法修改
public:
    int id;
    std::string data;
};

// 在 pandora 命名空间中特化 ContentHasher
namespace pandora {
    template<>
    struct ContentHasher<ThirdPartyClass> {
        size_t operator()(const ThirdPartyClass& obj) const {
            size_t seed = 0;
            HashCombine(seed, obj.id);
            HashCombine(seed, obj.data);
            return seed;
        }
    };
    
    // 同时特化 ContentEquals（如果没有 operator==）
    template<>
    struct ContentEquals<ThirdPartyClass> {
        bool operator()(const ThirdPartyClass& lhs, const ThirdPartyClass& rhs) const {
            return lhs.id == rhs.id && lhs.data == rhs.data;
        }
    };
}
```

## 方案三：使用继承（适用于设计新类）

```cpp
#include "pandora/pandora_traits.h"

// 基类提供哈希接口
template<typename Derived>
class Hashable {
public:
    size_t Hash() const {
        return static_cast<const Derived*>(this)->ComputeHash();
    }
};

class Product : public Hashable<Product> {
public:
    std::string sku;
    double price;
    int quantity;
    
    size_t ComputeHash() const {
        size_t seed = 0;
        pandora::HashCombine(seed, sku);
        pandora::HashCombine(seed, price);
        pandora::HashCombine(seed, quantity);
        return seed;
    }
    
    bool operator==(const Product& other) const {
        return sku == other.sku && price == other.price && quantity == other.quantity;
    }
};
```

## 编译时类型检查

如果类型 `T` 没有实现哈希函数，编译时会得到清晰的错误提示：

```
error: static_assert failed: "ContentHasher not specialized for this type. 
Please provide a specialization of pandora::ContentHasher<T> 
or implement a Hash() member function in your type."
```

## 实际使用示例

```cpp
#include "pandora/wrapper_data_set.h"

class Item {
public:
    int id;
    std::string name;
    double value;
    
    size_t Hash() const {
        size_t seed = 0;
        pandora::HashCombine(seed, id);
        pandora::HashCombine(seed, name);
        pandora::HashCombine(seed, value);
        return seed;
    }
    
    bool operator==(const Item& other) const {
        return id == other.id;  // 只比较 ID 判断是否同一对象
    }
};

int main() {
    pandora::WrapperDataSet<Item> dataset;
    
    // 添加数据
    Item item1{1, "Apple", 1.5};
    dataset.Add(item1);
    
    // 开始事务
    dataset.StartTransaction();
    
    // 修改数据内容（指针不变，但内容变了）
    item1.value = 2.0;  // 价格改变
    dataset.ReplaceAtPosIfExist(0, item1);
    
    // 结束事务 - DiffUtil 会检测到内容变化（通过哈希值）
    dataset.EndTransaction();  // 触发 notifyItemChanged(0)
    
    return 0;
}
```

## 关键点

1. **哈希函数应该计算所有影响对象"内容"的字段**
2. **`operator==` 用于判断是否是"同一个对象"**（通常只比较 ID）
3. **哈希值变化表示"内容发生了变化"**
4. **使用 `HashCombine` 组合多个字段的哈希值**

## 性能考虑

- 哈希计算在 `Snapshot()` 和 `AreContentsTheSame()` 中执行
- 只哈希关键字段可以提升性能
- 对于大对象，考虑只哈希部分关键字段

## 与 Java 的对比

| Java | C++ |
|------|-----|
| `@Override int hashCode()` | `size_t Hash() const` 或 特化 `ContentHasher<T>` |
| `@Override boolean equals()` | `bool operator==()` 或 特化 `ContentEquals<T>` |
| 自动装箱 | 值语义，零开销 |
| `HashMap` 存储 | `std::vector<size_t>` 位置对应 |


