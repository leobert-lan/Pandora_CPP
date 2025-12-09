# PandoraRV - C++ RecyclerView Adapter Framework

注：此文档由AI生成，尚未检查。

## 概述

PandoraRV 是 Pandora 框架的 RecyclerView 扩展，提供了强大的多类型列表适配器支持。它已成功从 Java 迁移到 C++，保持了原有的架构设计和功能特性。

## 特性

- ✅ **多类型支持**: 在同一个列表中支持多种数据类型和 ViewHolder
- ✅ **类型安全**: 使用 C++ 模板确保编译时类型检查
- ✅ **响应式绑定**: 支持数据变化自动更新 UI（可选）
- ✅ **观察者模式**: 自动通知 UI 更新
- ✅ **内存安全**: 使用智能指针和弱引用避免内存泄漏
- ✅ **灵活的数据结构**: 支持 RealDataSet（线性列表）和 WrapperDataSet（层次结构）

## 架构

```
pandora/rv/
├── i_view_holder.h              # ViewHolder 接口
├── view_holder_creator.h        # ViewHolder 创建器
├── data_observer.h              # 数据观察者接口
├── type_cell.h                  # 类型单元（内部）
├── data_vh_mapping_pool.h       # 数据-ViewHolder 映射池
├── data_set.h                   # DataSet 基类
├── pandora_data_set.h           # Pandora DataSet 基类
├── pandora_real_rv_data_set.h   # Real DataSet 实现
├── pandora_wrapper_rv_data_set.h # Wrapper DataSet 实现
├── reactive_data.h              # 响应式数据接口
└── i_reactive_view_holder.h     # 响应式 ViewHolder 接口
```

## 快速开始

### 1. 基本使用

```cpp
#include <pandora/pandora_rv.h>

using namespace pandora;
using namespace pandora::rv;

// 定义数据类型
class MyData : public DataSet<MyData>::Data {
public:
    std::string title;
    std::string subtitle;
};

// 定义 ViewHolder
class MyViewHolder : public IViewHolder<MyData> {
public:
    explicit MyViewHolder(void* parent) {
        // 初始化视图
    }

    void set_data(std::shared_ptr<MyData> data) override {
        data_ = data;
        // 更新 UI
        if (data_) {
            // 显示 data_->title 和 data_->subtitle
        }
    }

    void on_view_attached_to_window() override {
        // 视图附加到窗口时调用
    }

    void on_view_detached_from_window() override {
        // 视图从窗口分离时调用
    }

    void accept(IViewHolderVisitor& visitor) override {
        // Visitor 模式支持
    }

private:
    std::shared_ptr<MyData> data_;
};

// 使用示例
void example() {
    // 创建 RealDataSet
    auto real_ds = std::make_shared<RealDataSet<MyData>>();
    
    // 创建 RV DataSet
    auto rv_data_set = std::make_shared<PandoraRealRvDataSet<MyData>>(real_ds);
    
    // 注册数据类型和 ViewHolder 的映射
    rv_data_set->register_dv_relation<MyData>(
        make_lambda_creator<MyData>([](void* parent) {
            return std::make_shared<MyViewHolder>(parent);
        })
    );
    
    // 添加数据
    auto data1 = std::make_shared<MyData>();
    data1->title = "Item 1";
    data1->subtitle = "Description 1";
    rv_data_set->add(data1);
    
    auto data2 = std::make_shared<MyData>();
    data2->title = "Item 2";
    data2->subtitle = "Description 2";
    rv_data_set->add(data2);
    
    // 在适配器中使用
    int count = rv_data_set->get_count();  // 2
    auto item = rv_data_set->get_item(0);  // 获取第一个数据
    
    // 获取 ViewType 并创建 ViewHolder
    void* parent = nullptr;  // 实际使用时传入真实的父视图
    int view_type = rv_data_set->get_item_view_type_v2(0);
    auto holder = rv_data_set->create_view_holder_v2(parent, view_type);
}
```

### 2. 多类型支持

```cpp
// 定义多种数据类型
class TextData : public DataSet<DataSet<void>::Data>::Data {
    std::string text;
};

class ImageData : public DataSet<DataSet<void>::Data>::Data {
    std::string image_url;
};

// 定义对应的 ViewHolder
class TextViewHolder : public IViewHolder<TextData> { /* ... */ };
class ImageViewHolder : public IViewHolder<ImageData> { /* ... */ };

// 注册多个映射
auto rv_data_set = std::make_shared<PandoraRealRvDataSet<DataSet<void>::Data>>(real_ds);

rv_data_set->register_dv_relation<TextData>(
    make_typed_creator<TextData, TextViewHolder>()
);

rv_data_set->register_dv_relation<ImageData>(
    make_typed_creator<ImageData, ImageViewHolder>()
);

// 混合添加不同类型的数据
rv_data_set->add(std::make_shared<TextData>());
rv_data_set->add(std::make_shared<ImageData>());
rv_data_set->add(std::make_shared<TextData>());
```

### 3. 响应式数据绑定

```cpp
class MyReactiveData : public ReactiveData<MyReactiveData> {
public:
    void bind_reactive_vh(std::shared_ptr<IReactiveViewHolder<MyReactiveData>> vh) override {
        view_holder_ = vh;
    }

    void unbind_reactive_vh() override {
        view_holder_.reset();
    }

    void set_title(const std::string& title) {
        title_ = title;
        // 通知 ViewHolder 更新
        if (auto vh = view_holder_.lock()) {
            vh->on_property_changed(shared_from_this(), PROPERTY_TITLE);
        }
    }

private:
    std::weak_ptr<IReactiveViewHolder<MyReactiveData>> view_holder_;
    std::string title_;
    static constexpr int PROPERTY_TITLE = 1;
};

class MyReactiveViewHolder : public IReactiveViewHolder<MyReactiveData> {
public:
    void on_property_changed(std::shared_ptr<MyReactiveData> data, int property_id) override {
        // 只更新改变的属性
        if (property_id == MyReactiveData::PROPERTY_TITLE) {
            // 更新标题 UI
        }
    }
    
    std::shared_ptr<ReactiveData<MyReactiveData>> get_reactive_data_if_exist() override {
        return data_;
    }

private:
    std::shared_ptr<MyReactiveData> data_;
};
```

### 4. 数据变化通知

```cpp
// 添加观察者
class MyAdapter : public DataObserverBase {
public:
    void on_data_set_changed() override {
        // 刷新整个列表
    }
    
    void notify_item_changed(int position) override {
        // 更新单个项目
    }
};

auto observer = std::make_shared<MyAdapter>();
rv_data_set->add_data_observer(observer);

// 修改数据后通知
rv_data_set->add(new_data);
rv_data_set->notify_item_inserted(rv_data_set->get_count() - 1);

rv_data_set->remove_item_at(0);
rv_data_set->notify_item_removed(0);
```

### 5. 层次数据结构（WrapperDataSet）

```cpp
// 创建 Wrapper DataSet
auto wrapper_ds = std::make_shared<WrapperDataSet<MyData>>();
auto rv_data_set = std::make_shared<PandoraWrapperRvDataSet<MyData>>(wrapper_ds);

// 创建子适配器
auto header_ds = std::make_shared<RealDataSet<MyData>>();
auto content_ds = std::make_shared<RealDataSet<MyData>>();
auto footer_ds = std::make_shared<RealDataSet<MyData>>();

// 添加子适配器
rv_data_set->add_sub(header_ds);
rv_data_set->add_sub(content_ds);
rv_data_set->add_sub(footer_ds);

// 分别操作各个部分
header_ds->add(header_data);
content_ds->add_items(content_items);
footer_ds->add(footer_data);

// WrapperDataSet 自动合并所有子适配器的数据
int total_count = rv_data_set->get_count();  // header + content + footer
```

## 与 Java 版本的主要差异

| 特性 | Java | C++ |
|------|------|-----|
| 类型系统 | 泛型 + 反射 | 模板 + type_index |
| 内存管理 | GC | 智能指针 (shared_ptr/weak_ptr) |
| 观察者引用 | WeakReference | std::weak_ptr |
| 容器 | ArrayList, SparseArray | std::vector, std::unordered_map |
| 创建器 | 匿名类/Lambda | Lambda/函数对象 |
| ViewGroup | Android ViewGroup | void* (平台无关) |
| 线程安全 | synchronized | std::mutex |

## 性能考虑

- **类型查找**: 使用 `std::type_index` 和 `unordered_map` 实现 O(1) 查找
- **观察者清理**: 自动清理过期的 `weak_ptr`，无需手动管理
- **内存开销**: 智能指针有轻微开销，但提供了内存安全保证
- **编译时优化**: 模板在编译时展开，无运行时类型擦除开销

## 编译要求

- C++17 或更高版本
- 支持 `<memory>`, `<vector>`, `<unordered_map>` 等标准库
- CMake 3.10 或更高版本（用于构建）

## 最佳实践

1. **使用 weak_ptr 作为观察者**: 避免循环引用导致内存泄漏
2. **在事务中批量操作**: 使用 `start_transaction()` 和 `end_transaction()` 减少通知次数
3. **合理使用响应式绑定**: 仅在需要细粒度更新时使用
4. **类型注册放在初始化阶段**: 避免运行时动态注册影响性能
5. **ViewHolder 复用**: 充分利用 ViewHolder 池化机制

