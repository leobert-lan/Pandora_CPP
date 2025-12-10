/*
 * PandoraRV C++ Example - Demonstrates basic and advanced usage
 */

#include <pandora/pandora_rv.h>
#include <pandora/real_data_set.h>
#include <pandora/wrapper_data_set.h>
#include <iostream>
#include <cassert>
#include <filesystem>

using namespace pandora;
using namespace pandora::rv;

// ========== Example 1: Simple Single-Type List ==========

class SimpleData final : public DataSet<SimpleData>::Data
{
public:
    std::string title;
    int value;

    explicit SimpleData(const std::string& t, const int v) : title(t), value(v)
    {
    }

    void SetToViewHolder(const std::shared_ptr<IViewHolder<Data>> view_holder) override
    {
        view_holder->SetData(std::shared_ptr<SimpleData>(this));
    }

    // 实现 Hash() 成员函数
    size_t Hash() const
    {
        size_t seed = 0;
        HashCombine(seed, title);
        HashCombine(seed, value);
        return seed;
    }

    bool operator==(const SimpleData& other) const
    {
        return title == other.title && value == other.value;
    }
};

class SimpleViewHolder : public IViewHolder<SimpleData>
{
public:
    explicit SimpleViewHolder(void* parent)
    {
        std::cout << "SimpleViewHolder created" << std::endl;
    }

    void SetData(const std::shared_ptr<SimpleData> data) override
    {
        data_ = data;
        if (data_)
        {
            std::cout << "Binding data: " << data_->title << " = " << data_->value << std::endl;
        }
    }

    void OnViewAttachedToWindow() override
    {
        std::cout << "SimpleViewHolder attached" << std::endl;
    }

    void OnViewDetachedFromWindow() override
    {
        std::cout << "SimpleViewHolder detached" << std::endl;
    }

    void accept(IViewHolderVisitor& visitor) override
    {
    }

private:
    std::shared_ptr<SimpleData> data_;
};

void example_simple_list()
{
    std::cout << "\n========== Example 1: Simple Single-Type List ==========\n";

    // Create DataSet
    auto real_ds = std::make_shared<RealDataSet<SimpleData>>();
    const auto rv_data_set = std::make_shared<PandoraRealRvDataSet<SimpleData>>(real_ds);

    // Register ViewHolder creator
    rv_data_set->register_dv_relation<SimpleData>(
        make_lambda_creator<SimpleData>([](void* parent)
        {
            return std::make_shared<SimpleViewHolder>(parent);
        })
    );

    // Add data
    rv_data_set->Add(SimpleData("Item 1", 100));
    rv_data_set->Add(SimpleData("Item 2", 200));
    rv_data_set->Add(SimpleData("Item 3", 300));

    std::cout << "Total items: " << rv_data_set->GetCount() << std::endl;

    // Simulate adapter behavior
    for (int i = 0; i < rv_data_set->GetCount(); i++)
    {
        const int view_type = rv_data_set->GetItemViewTypeV2(i);
        auto holder = rv_data_set->CreateViewHolderV2(nullptr, view_type);

        const auto data = rv_data_set->GetItem(i);
        if (const auto simple_holder = std::dynamic_pointer_cast<ViewHolderWrapper<SimpleData>>(holder))
        {
            simple_holder->GetHolder()->SetData(data);
        }
    }
}

// ========== Example 2: Multi-Type List ==========

class BaseData : public DataSet<BaseData>::Data
{
public:
    ~BaseData() override = default;

    void SetToViewHolder(const std::shared_ptr<IViewHolder<Data>> view_holder) override
    {
        view_holder->SetData(std::shared_ptr<BaseData>(this));
    }

    size_t Hash() const
    {
        std::size_t seed = 0x5C0A28E4;
        seed ^= (seed << 6) + (seed >> 2) + 0x2A691606;
        return seed;
    }

    bool operator==(const BaseData& other) const
    {
        return Hash() == other.Hash();
    }
};

class TextData : public BaseData
{
public:
    std::string text;

    explicit TextData(const std::string& t) : text(t)
    {
    }

    // 实现 Hash() 成员函数
    size_t Hash() const
    {
        size_t seed = 0;
        HashCombine(seed, text);
        return seed;
    }

    // 同时也需要实现 operator==
    bool operator==(const TextData& other) const
    {
        return text == other.text;
    }
};

class ImageData : public BaseData
{
public:
    std::string url;
    int width, height;

    ImageData(const std::string& u, const int w, const int h) : url(u), width(w), height(h)
    {
    }

    // 实现 Hash() 成员函数
    size_t Hash() const
    {
        size_t seed = 0;
        HashCombine(seed, url);
        HashCombine(seed, width);
        HashCombine(seed, height);
        return seed;
    }

    // 同时也需要实现 operator==
    bool operator==(const ImageData& other) const
    {
        return url == other.url && width == other.width && height == other.height;
    }
};

class TextViewHolder final : public IViewHolder<TextData>
{
public:
    explicit TextViewHolder(void* parent)
    {
        std::cout << "TextViewHolder created" << std::endl;
    }

    void SetData(const std::shared_ptr<TextData> data) override
    {
        if (data)
        {
            std::cout << "Text: " << data->text << std::endl;
        }
    }

    void OnViewAttachedToWindow() override
    {
    }

    void OnViewDetachedFromWindow() override
    {
    }

    void accept(IViewHolderVisitor& visitor) override
    {
    }
};

class ImageViewHolder final : public IViewHolder<ImageData>
{
public:
    explicit ImageViewHolder(void* parent)
    {
        std::cout << "ImageViewHolder created" << std::endl;
    }

    void SetData(const std::shared_ptr<ImageData> data) override
    {
        if (data)
        {
            std::cout << "Image: " << data->url << " (" << data->width << "x" << data->height << ")" << std::endl;
        }
    }

    void OnViewAttachedToWindow() override
    {
    }

    void OnViewDetachedFromWindow() override
    {
    }

    void accept(IViewHolderVisitor& visitor) override
    {
    }
};

void example_multi_type()
{
    std::cout << "\n========== Example 2: Multi-Type List ==========\n";

    auto real_ds = std::make_shared<RealDataSet<BaseData>>();
    const auto rv_data_set = std::make_shared<PandoraRealRvDataSet<BaseData>>(real_ds);

    // Register multiple types
    rv_data_set->register_dv_relation<TextData>(
        make_typed_creator<TextData, TextViewHolder>()
    );

    rv_data_set->register_dv_relation<ImageData>(
        make_typed_creator<ImageData, ImageViewHolder>()
    );

    // Add mixed data
    rv_data_set->Add(TextData("Hello World"));
    rv_data_set->Add(ImageData("image1.jpg", 800, 600));
    rv_data_set->Add(TextData("Another text"));
    rv_data_set->Add(ImageData("image2.png", 1024, 768));

    std::cout << "Total items: " << rv_data_set->GetCount() << std::endl;
    std::cout << "View type count: " << rv_data_set->GetViewTypeCount() << std::endl;
}

// ========== Example 3: Observer Pattern ==========

class TestObserver : public DataObserverBase
{
public:
    void OnDataSetChanged() override
    {
        std::cout << "Observer: Data set changed!" << std::endl;
    }

    void NotifyItemInserted(const int position) override
    {
        std::cout << "Observer: Item inserted at position " << position << std::endl;
    }

    void NotifyItemRemoved(const int position) override
    {
        std::cout << "Observer: Item removed at position " << position << std::endl;
    }
};

void example_observer()
{
    std::cout << "\n========== Example 3: Observer Pattern ==========\n";

    auto real_ds = std::make_shared<RealDataSet<SimpleData>>();
    const auto rv_data_set = std::make_shared<PandoraRealRvDataSet<SimpleData>>(real_ds);

    // Add observer
    const auto observer = std::make_shared<TestObserver>();
    rv_data_set->AddDataObserver(observer);

    // Add data and notify
    rv_data_set->Add(SimpleData("Item 1", 1));
    rv_data_set->NotifyItemInserted(0);

    rv_data_set->Add(SimpleData("Item 2", 2));
    rv_data_set->NotifyItemInserted(1);

    // Remove and notify
    rv_data_set->RemoveAtPos(0);
    rv_data_set->NotifyItemRemoved(0);

    // Notify full change
    rv_data_set->NotifyChanged();
}

// ========== Example 4: Wrapper DataSet (Hierarchical) ==========

void example_wrapper_dataset()
{
    std::cout << "\n========== Example 4: Wrapper DataSet ==========\n";

    // Create wrapper
    auto wrapper_ds = std::make_shared<WrapperDataSet<SimpleData>>();
    const auto rv_data_set = std::make_shared<PandoraWrapperRvDataSet<SimpleData>>(wrapper_ds);

    // Create child adapters
    const auto header_ds = std::make_shared<RealDataSet<SimpleData>>();
    const auto content_ds = std::make_shared<RealDataSet<SimpleData>>();
    const auto footer_ds = std::make_shared<RealDataSet<SimpleData>>();

    // Add data to children
    header_ds->Add(SimpleData("Header", 0));
    content_ds->Add(SimpleData("Content 1", 1));
    content_ds->Add(SimpleData("Content 2", 2));
    content_ds->Add(SimpleData("Content 3", 3));
    footer_ds->Add(SimpleData("Footer", 99));

    // Add children to wrapper
    rv_data_set->AddSub(std::unique_ptr<RealDataSet<SimpleData>>(header_ds.get()));
    rv_data_set->AddSub(std::unique_ptr<RealDataSet<SimpleData>>(content_ds.get()));
    rv_data_set->AddSub(std::unique_ptr<RealDataSet<SimpleData>>(footer_ds.get()));

    std::cout << "Total items in wrapper: " << rv_data_set->GetCount() << std::endl;

    // Access all items
    for (int i = 0; i < rv_data_set->GetCount(); i++)
    {
        auto item = rv_data_set->GetItem(i);
        if (item)
        {
            std::cout << "Position " << i << ": " << item->title << std::endl;
        }
    }
}

// ========== Example 5: Transaction ==========

void example_transaction()
{
    std::cout << "\n========== Example 5: Transaction ==========\n";

    auto real_ds = std::make_shared<RealDataSet<SimpleData>>();
    const auto rv_data_set = std::make_shared<PandoraRealRvDataSet<SimpleData>>(real_ds);

    const auto observer = std::make_shared<TestObserver>();
    rv_data_set->AddDataObserver(observer);

    std::cout << "Adding items in transaction (should notify once):" << std::endl;

    // Start transaction
    rv_data_set->StartTransaction();

    // Batch operations
    for (int i = 0; i < 5; i++)
    {
        rv_data_set->Add(SimpleData("Item " + std::to_string(i), i));
    }

    // End transaction (triggers single notification)
    rv_data_set->EndTransaction();

    std::cout << "Total items: " << rv_data_set->GetCount() << std::endl;
}

// ========== Main ==========

int main()
{
    std::cout << "PandoraRV C++ Examples\n";
    std::cout << "=====================\n";

    try
    {
        example_simple_list();
        example_multi_type();
        example_observer();
        example_wrapper_dataset();
        example_transaction();

        std::cout << "\n✅ All examples completed successfully!\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "\n❌ Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
