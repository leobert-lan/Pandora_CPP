# Pandora-CPP 设计权衡分析

## 当前设计的问题

### 1. 值拷贝导致的冗余
当前实现使用值拷贝（pass-by-value），存在以下问题：
- **Add/SetData等操作**：每次都会拷贝整个对象
- **GetDataByIndex**：返回指针，但数据存储为值
- **修改数据困难**：获取指针后修改，但缺少变更通知机制

### 2. 数据修改不便
```cpp
// 当前方式 - 不够方便
auto* data = dataset.GetDataByIndex(0);
data->someField = newValue;  // 修改了，但没有通知机制
```

## 推荐的改进方案

### 方案A：智能指针存储（推荐用于大对象）
适合：数据对象较大、需要频繁修改的场景

```cpp
template <typename T>
class RealDataSet : public PandoraBoxAdapter<std::shared_ptr<T>> {
    std::vector<std::shared_ptr<T>> data_;
public:
    void Add(std::shared_ptr<T> item) {
        OnBeforeChanged();
        data_.push_back(item);
        OnAfterChanged();
    }
    
    // 直接获取共享指针，可以安全修改
    std::shared_ptr<T>* GetDataByIndex(int index) {
        return &data_[index];
    }
};

// 使用示例
auto dataset = RealDataSet<MyData>();
auto item = std::make_shared<MyData>();
dataset.Add(item);
// 可以直接通过item修改，多处共享
item->field = newValue;
```

### 方案B：混合模式（灵活性最高）
同时支持值类型和指针类型

```cpp
// 小对象用值
RealDataSet<int> intDataset;
intDataset.Add(42);

// 大对象用智能指针
RealDataSet<std::shared_ptr<BigObject>> bigDataset;
bigDataset.Add(std::make_shared<BigObject>());
```

### 方案C：添加Update接口（最小改动）
保持当前值存储，增加修改通知

```cpp
template <typename T>
class RealDataSet : public PandoraBoxAdapter<T> {
public:
    // 新增：带通知的更新方法
    void UpdateAt(int index, std::function<void(T&)> updater) {
        if (index < 0 || index >= data_.size()) return;
        OnBeforeChanged();
        updater(data_[index]);
        OnAfterChanged();
    }
    
    // 批量更新
    void UpdateAll(std::function<void(T&)> updater) {
        OnBeforeChanged();
        for (auto& item : data_) {
            updater(item);
        }
        OnAfterChanged();
    }
};

// 使用示例
dataset.UpdateAt(0, [](MyData& data) {
    data.field1 = newValue;
    data.field2 = anotherValue;
});
```

## 性能对比

### 内存占用
- **值存储**：`sizeof(T) * count`
- **shared_ptr存储**：`(sizeof(shared_ptr) + sizeof(T)) * count` ≈ 1.5x ~ 2x
- **unique_ptr存储**：`(sizeof(unique_ptr) + sizeof(T)) * count` ≈ 1.1x ~ 1.3x

### 拷贝开销
- **值存储**：每次Add/Set都拷贝，O(n)复杂度（n为对象大小）
- **指针存储**：只拷贝指针，O(1)复杂度

### 适用场景建议

| 数据类型 | 推荐方案 | 原因 |
|---------|---------|------|
| 基本类型(int, float等) | 值存储 | 拷贝成本低，语义清晰 |
| 小对象(<32字节) | 值存储 | 拷贝成本可接受 |
| 中等对象(32-128字节) | 方案C(Update接口) | 平衡易用性和性能 |
| 大对象(>128字节) | shared_ptr | 避免频繁拷贝 |
| 需要共享所有权 | shared_ptr | 多处引用同一数据 |
| 需要独占所有权 | unique_ptr | 明确所有权语义 |

## 推荐实施步骤

1. **短期**：添加Update接口（方案C），最小改动
2. **中期**：提供PandoraBoxAdapter的不同特化版本
3. **长期**：考虑模板策略模式，让用户选择存储策略

