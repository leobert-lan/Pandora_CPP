# PandoraRV C++ 迁移方案

## 概述
将 Java 版本的 PandoraRV（RecyclerView 适配器框架）迁移到 C++ 版本。PandoraRV 是基于 Pandora 核心库的扩展，为 Android RecyclerView 提供多类型支持。

## 当前状态分析

### 已完成的核心库（C++）
- ✅ `data_adapter.h` - 数据适配器基础接口
- ✅ `pandora_box_adapter.h` - Pandora 盒子适配器
- ✅ `real_data_set.h` - 真实数据集
- ✅ `wrapper_data_set.h` - 包装数据集
- ✅ `diff_util.h` - 差异计算工具
- ✅ `logger.h` - 日志工具
- ✅ `type_visitor.h` - 类型访问者

### Java 源文件清单（需迁移）
1. `DataSet.java` - 数据集基类（核心）
2. `IViewHolder.java` - ViewHolder 接口
3. `ViewHolderCreator.java` - ViewHolder 创建器
4. `DataVhMappingPool.java` - 数据-ViewHolder 映射池
5. `DataObserver.java` - 数据观察者接口
6. `PandoraDataSet.java` - Pandora 数据集基类
7. `PandoraRealRvDataSet.java` - Real 数据集实现
8. `PandoraWrapperRvDataSet.java` - Wrapper 数据集实现
9. `IReactiveViewHolder.java` - 响应式 ViewHolder 接口
10. `ReactiveData.java` - 响应式数据接口
11. `TypeCell.java` - 类型单元（内部类）

## 迁移策略

### 1. 核心概念映射

| Java 概念 | C++ 实现方案 |
|----------|-------------|
| Interface | 纯虚类（抽象基类） |
| Abstract Class | 抽象基类 |
| Generic `<T>` | 模板 `template<typename T>` |
| `WeakReference` | `std::weak_ptr` |
| `SparseArray` | `std::map` 或 `std::unordered_map` |
| `ArrayList` | `std::vector` |
| `Pair` | `std::pair` |
| Exception | 异常类 + optional/expected |
| Reflection | 模板特化 + 工厂模式 |

### 2. 架构设计

#### 2.1 命名空间结构
```cpp
namespace pandora {
namespace rv {
    // RecyclerView 相关的所有类
}}
```

#### 2.2 内存管理策略
- ViewHolder: `std::shared_ptr`（共享所有权）
- Data: `std::shared_ptr`（可能被多处引用）
- Observer: `std::weak_ptr`（避免循环引用）
- Creator: 值语义或 `unique_ptr`（单一所有权）

#### 2.3 类型安全
- 使用 CRTP (Curiously Recurring Template Pattern) 实现类型安全的继承
- 使用 `std::any` 或 `std::variant` 处理运行时类型
- 使用 `std::type_index` 实现类型注册

### 3. 文件迁移计划

#### Phase 1: 基础接口层（优先级：高）
1. **i_view_holder.h** - ViewHolder 接口
   - 纯虚接口
   - Visitor 模式支持
   - 生命周期回调

2. **view_holder_creator.h** - ViewHolder 创建器
   - 工厂模式基类
   - Lambda/函数对象支持

3. **data_observer.h** - 数据观察者
   - 观察者模式接口
   - 所有通知方法

#### Phase 2: 核心实现层（优先级：高）
4. **type_cell.h** - 类型单元
   - 内部辅助类
   - 类型映射逻辑

5. **data_vh_mapping_pool.h** - 映射池
   - 类型注册机制
   - ViewType 计算
   - ViewHolder 创建

6. **data_set.h** - 数据集基类
   - 模板基类
   - 观察者管理
   - 通知机制

#### Phase 3: 扩展实现层（优先级：中）
7. **pandora_data_set.h** - Pandora 数据集
   - 继承自 DataSet
   - 委托模式实现

8. **pandora_real_rv_data_set.h** - Real 数据集
9. **pandora_wrapper_rv_data_set.h** - Wrapper 数据集

#### Phase 4: 高级特性层（优先级：低）
10. **reactive_data.h** - 响应式数据
11. **i_reactive_view_holder.h** - 响应式 ViewHolder

### 4. 技术挑战与解决方案

#### 4.1 类型擦除问题
**挑战**: Java 的泛型在运行时保留类型信息，C++ 模板会擦除
**解决方案**:
```cpp
// 使用 type_index 注册类型
template<typename T>
void register_type() {
    type_map[std::type_index(typeid(T))] = ...;
}
```

#### 4.2 反射机制
**挑战**: Java 的反射创建对象，C++ 无直接支持
**解决方案**:
```cpp
// 使用工厂函数 + lambda
using CreatorFunc = std::function<std::shared_ptr<IViewHolder>(ViewGroup*)>;
mapping_pool.register_creator<MyData>([](ViewGroup* parent) {
    return std::make_shared<MyViewHolder>(parent);
});
```

#### 4.3 弱引用与观察者
**挑战**: Java 的 WeakReference + 清理逻辑
**解决方案**:
```cpp
std::vector<std::weak_ptr<DataObserver>> observers_;

void notify_changed() {
    // 自动清理过期引用
    observers_.erase(
        std::remove_if(observers_.begin(), observers_.end(),
            [](const auto& weak) { return weak.expired(); }),
        observers_.end()
    );
    // 通知存活的观察者
    for (auto& weak : observers_) {
        if (auto obs = weak.lock()) {
            obs->on_data_set_changed();
        }
    }
}
```

#### 4.4 SparseArray 替代
**挑战**: Android SparseArray 优化的整型键值映射
**解决方案**:
```cpp
// 使用 unordered_map，性能足够好
std::unordered_map<int, TypeCell> view_type_cache_;
// 或者使用 vector（如果键是连续的）
std::vector<TypeCell> view_type_cache_;
```

### 5. API 设计示例

#### 5.1 DataSet 基类
```cpp
template<typename T>
class DataSet {
public:
    virtual ~DataSet() = default;
    
    virtual int get_count() const = 0;
    virtual std::shared_ptr<T> get_item(int position) const = 0;
    
    void add_data_observer(std::weak_ptr<DataObserver> observer);
    void remove_data_observer(const std::shared_ptr<DataObserver>& observer);
    
    int get_item_view_type(int pos) const;
    std::shared_ptr<IViewHolder> create_view_holder(ViewGroup* parent, int view_type);
    
    template<typename DataType, typename CreatorType>
    DataSet& register_dv_relation(CreatorType&& creator);
    
protected:
    void notify_changed();
    void notify_item_changed(int position);
    
private:
    DataVhMappingPool mapping_pool_;
    std::vector<std::weak_ptr<DataObserver>> observers_;
};
```

#### 5.2 ViewHolder 创建器
```cpp
class ViewHolderCreator {
public:
    virtual ~ViewHolderCreator() = default;
    virtual std::shared_ptr<IViewHolder> create_view_holder(ViewGroup* parent) = 0;
};

// Lambda 支持
template<typename Func>
class LambdaCreator : public ViewHolderCreator {
    Func func_;
public:
    explicit LambdaCreator(Func f) : func_(std::move(f)) {}
    std::shared_ptr<IViewHolder> create_view_holder(ViewGroup* parent) override {
        return func_(parent);
    }
};
```

### 6. 使用示例对比

#### Java 用法
```java
dataSet.registerDVRelation(MyData.class, new ViewHolderCreator() {
    @Override
    public IViewHolder createViewHolder(ViewGroup parent) {
        return new MyViewHolder(parent);
    }
});
```

#### C++ 用法（目标）
```cpp
// 方式1：Lambda
data_set.register_dv_relation<MyData>([](ViewGroup* parent) {
    return std::make_shared<MyViewHolder>(parent);
});

// 方式2：自定义 Creator
data_set.register_dv_relation<MyData>(
    std::make_unique<MyViewHolderCreator>()
);
```

### 7. 测试策略
1. 单元测试：每个类独立测试
2. 集成测试：DataSet + MappingPool 协同
3. 性能测试：与 Java 版本对比
4. 内存测试：Valgrind/AddressSanitizer

### 8. 兼容性考虑
- **C++ 标准**: 使用 C++17（项目已支持）
- **跨平台**: 避免平台特定代码
- **Qt 集成**: 考虑 Qt 信号槽机制（可选）

### 9. 文档与注释
- Doxygen 风格注释
- 每个类提供使用示例
- 迁移差异说明文档

### 10. 迁移时间估算
- Phase 1: 2-3 天
- Phase 2: 3-4 天
- Phase 3: 2-3 天
- Phase 4: 2 天
- 测试与优化: 3-4 天
- **总计**: 12-16 天

## 关键决策点

### ViewGroup 的处理
**问题**: Java 的 ViewGroup 是 Android 特定类
**方案**:
1. 抽象出 `ViewParent` 接口（推荐）
2. 使用 `void*` + 类型安全包装
3. 使用模板参数化

### 数据生命周期
**问题**: C++ 需要明确所有权
**方案**:
- DataSet 拥有数据：`std::vector<std::shared_ptr<T>>`
- ViewHolder 持有数据弱引用或裸指针（短期）

## 下一步行动
1. ✅ 创建迁移计划文档
2. ⏳ 创建目录结构
3. ⏳ 实现 Phase 1 基础接口
4. ⏳ 实现 Phase 2 核心逻辑
5. ⏳ 编写单元���试
6. ⏳ 集成测试与文档

---
*文档创建时间: 2025-12-09*
*作者: GitHub Copilot*

