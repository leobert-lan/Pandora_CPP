/**
 * @file template_type_recognition_example.cpp
 * @brief 演示 C++ 模板如何在编译时识别不同的具体类型
 *
 * 这个示例展示了：
 * 1. PandoraDataSet 中 data_set_ 被识别为 PandoraBoxAdapter<T>
 * 2. 在子类 PandoraWrapperRvDataSet 中，data_set_ 被识别为 WrapperDataSet<T>
 * 3. 在子类 PandoraRealRvDataSet 中，data_set_ 被识别为 RealDataSet<T>
 */

#include <iostream>
#include <memory>
#include <type_traits>
#include <typeinfo>

// 简化的示例结构，演示模板类型识别原理

namespace example {

// 基类
template<typename T>
class PandoraBoxAdapter {
public:
    virtual ~PandoraBoxAdapter() = default;
    virtual int GetDataCount() const = 0;
    virtual void PrintType() const {
        std::cout << "PandoraBoxAdapter base" << std::endl;
    }
};

// 具体实现类1
template<typename T>
class WrapperDataSet : public PandoraBoxAdapter<T> {
public:
    int GetDataCount() const override { return 10; }
    void PrintType() const override {
        std::cout << "WrapperDataSet" << std::endl;
    }
    void WrapperSpecificMethod() {
        std::cout << "This is WrapperDataSet specific method!" << std::endl;
    }
};

// 具体实现类2
template<typename T>
class RealDataSet : public PandoraBoxAdapter<T> {
public:
    int GetDataCount() const override { return 5; }
    void PrintType() const override {
        std::cout << "RealDataSet" << std::endl;
    }
    void RealSpecificMethod() {
        std::cout << "This is RealDataSet specific method!" << std::endl;
    }
};

// ========== 关键点：基类使用模板参数 DS ==========
template<typename T, typename DS>
class PandoraDataSet {
    static_assert(std::is_base_of<PandoraBoxAdapter<T>, DS>::value,
                  "DS must inherit from PandoraBoxAdapter<T>");

protected:
    std::shared_ptr<DS> data_set_;  // ← 这里的类型由模板参数 DS 决定

public:
    explicit PandoraDataSet(std::shared_ptr<DS> data_set)
        : data_set_(std::move(data_set)) {}

    // 通用方法，可以调用 PandoraBoxAdapter 的接口
    int GetCount() const {
        return data_set_->GetDataCount();
    }

    void ShowType() const {
        data_set_->PrintType();
    }

    // 获取底层 data_set_，供子类使用
    std::shared_ptr<DS> GetDataSet() const {
        return data_set_;
    }
};

// ========== 子类1：使用 WrapperDataSet ==========
template<typename T>
class PandoraWrapperRvDataSet : public PandoraDataSet<T, WrapperDataSet<T>> {
public:
    explicit PandoraWrapperRvDataSet(std::shared_ptr<WrapperDataSet<T>> wrapper)
        : PandoraDataSet<T, WrapperDataSet<T>>(std::move(wrapper)) {}

    // ✅ 在这里，this->data_set_ 的类型是 std::shared_ptr<WrapperDataSet<T>>
    // 编译器知道它是 WrapperDataSet，所以可以调用 WrapperDataSet 特有的方法
    void CallWrapperSpecificMethod() {
        this->data_set_->WrapperSpecificMethod();  // ✅ 编译通过！

        // 下面这行会编译失败，因为 WrapperDataSet 没有这个方法
        // this->data_set_->RealSpecificMethod();  // ❌ 编译错误
    }
};

// ========== 子类2：使用 RealDataSet ==========
template<typename T>
class PandoraRealRvDataSet : public PandoraDataSet<T, RealDataSet<T>> {
public:
    explicit PandoraRealRvDataSet(std::shared_ptr<RealDataSet<T>> real)
        : PandoraDataSet<T, RealDataSet<T>>(std::move(real)) {}

    // ✅ 在这里，this->data_set_ 的类型是 std::shared_ptr<RealDataSet<T>>
    // 编译器知道它是 RealDataSet，所以可以调用 RealDataSet 特有的方法
    void CallRealSpecificMethod() {
        this->data_set_->RealSpecificMethod();  // ✅ 编译通过！

        // 下面这行会编译失败，因为 RealDataSet 没有这个方法
        // this->data_set_->WrapperSpecificMethod();  // ❌ 编译错误
    }
};

} // namespace example

// ========== 测试代码 ==========
int main() {
    using namespace example;

    std::cout << "=== C++ 模板类型识别演示 ===" << std::endl << std::endl;

    // 创建 WrapperDataSet 实例
    auto wrapper = std::make_shared<WrapperDataSet<int>>();
    auto wrapper_rv = std::make_shared<PandoraWrapperRvDataSet<int>>(wrapper);

    std::cout << "1. PandoraWrapperRvDataSet 中的 data_set_ 类型：" << std::endl;
    wrapper_rv->ShowType();
    std::cout << "   数据数量：" << wrapper_rv->GetCount() << std::endl;
    std::cout << "   调用 WrapperDataSet 特有方法：" << std::endl;
    wrapper_rv->CallWrapperSpecificMethod();
    std::cout << std::endl;

    // 创建 RealDataSet 实例
    auto real = std::make_shared<RealDataSet<int>>();
    auto real_rv = std::make_shared<PandoraRealRvDataSet<int>>(real);

    std::cout << "2. PandoraRealRvDataSet 中的 data_set_ 类型：" << std::endl;
    real_rv->ShowType();
    std::cout << "   数据数量：" << real_rv->GetCount() << std::endl;
    std::cout << "   调用 RealDataSet 特有方法：" << std::endl;
    real_rv->CallRealSpecificMethod();
    std::cout << std::endl;

    // 类型验证（编译时）
    std::cout << "3. 编译时类型验证：" << std::endl;
    std::cout << "   WrapperDataSet 是 PandoraBoxAdapter？ "
              << std::is_base_of<PandoraBoxAdapter<int>, WrapperDataSet<int>>::value << std::endl;
    std::cout << "   RealDataSet 是 PandoraBoxAdapter？ "
              << std::is_base_of<PandoraBoxAdapter<int>, RealDataSet<int>>::value << std::endl;

    return 0;
}

