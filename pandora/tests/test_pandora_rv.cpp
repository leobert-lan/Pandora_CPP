/*
 * PandoraRV C++ Example - Demonstrates basic and advanced usage
 */

#include <pandora/pandora_rv.h>
#include <pandora/real_data_set.h>
#include <pandora/wrapper_data_set.h>
#include <iostream>
#include <cassert>

using namespace pandora;
using namespace pandora::rv;

// ========== Example 1: Simple Single-Type List ==========

class SimpleData : public DataSet<SimpleData>::Data {
public:
    std::string title;
    int value;

    SimpleData(const std::string& t, int v) : title(t), value(v) {}

    void set_to_view_holder(std::shared_ptr<IViewHolder<SimpleData>> vh) override {
        vh->set_data(shared_from_this());
    }
};

class SimpleViewHolder : public IViewHolder<SimpleData> {
public:
    explicit SimpleViewHolder(void* parent) {
        std::cout << "SimpleViewHolder created" << std::endl;
    }

    void set_data(std::shared_ptr<SimpleData> data) override {
        data_ = data;
        if (data_) {
            std::cout << "Binding data: " << data_->title << " = " << data_->value << std::endl;
        }
    }

    void on_view_attached_to_window() override {
        std::cout << "SimpleViewHolder attached" << std::endl;
    }

    void on_view_detached_from_window() override {
        std::cout << "SimpleViewHolder detached" << std::endl;
    }

    void accept(IViewHolderVisitor& visitor) override {}

private:
    std::shared_ptr<SimpleData> data_;
};

void example_simple_list() {
    std::cout << "\n========== Example 1: Simple Single-Type List ==========\n";

    // Create DataSet
    auto real_ds = std::make_shared<RealDataSet<SimpleData>>();
    auto rv_data_set = std::make_shared<PandoraRealRvDataSet<SimpleData>>(real_ds);

    // Register ViewHolder creator
    rv_data_set->register_dv_relation<SimpleData>(
        make_lambda_creator<SimpleData>([](void* parent) {
            return std::make_shared<SimpleViewHolder>(parent);
        })
    );

    // Add data
    rv_data_set->add(std::make_shared<SimpleData>("Item 1", 100));
    rv_data_set->add(std::make_shared<SimpleData>("Item 2", 200));
    rv_data_set->add(std::make_shared<SimpleData>("Item 3", 300));

    std::cout << "Total items: " << rv_data_set->get_count() << std::endl;

    // Simulate adapter behavior
    for (int i = 0; i < rv_data_set->get_count(); i++) {
        int view_type = rv_data_set->get_item_view_type_v2(i);
        auto holder = rv_data_set->create_view_holder_v2(nullptr, view_type);

        auto data = rv_data_set->get_item(i);
        if (auto simple_holder = std::dynamic_pointer_cast<ViewHolderWrapper<SimpleData>>(holder)) {
            simple_holder->get_holder()->set_data(data);
        }
    }
}

// ========== Example 2: Multi-Type List ==========

class BaseData : public DataSet<BaseData>::Data {
public:
    virtual ~BaseData() = default;
    void set_to_view_holder(std::shared_ptr<IViewHolder<BaseData>> vh) override {
        vh->set_data(shared_from_this());
    }
};

class TextData : public BaseData {
public:
    std::string text;
    explicit TextData(const std::string& t) : text(t) {}
};

class ImageData : public BaseData {
public:
    std::string url;
    int width, height;
    ImageData(const std::string& u, int w, int h) : url(u), width(w), height(h) {}
};

class TextViewHolder : public IViewHolder<TextData> {
public:
    explicit TextViewHolder(void* parent) {
        std::cout << "TextViewHolder created" << std::endl;
    }

    void set_data(std::shared_ptr<TextData> data) override {
        if (data) {
            std::cout << "Text: " << data->text << std::endl;
        }
    }

    void on_view_attached_to_window() override {}
    void on_view_detached_from_window() override {}
    void accept(IViewHolderVisitor& visitor) override {}
};

class ImageViewHolder : public IViewHolder<ImageData> {
public:
    explicit ImageViewHolder(void* parent) {
        std::cout << "ImageViewHolder created" << std::endl;
    }

    void set_data(std::shared_ptr<ImageData> data) override {
        if (data) {
            std::cout << "Image: " << data->url << " (" << data->width << "x" << data->height << ")" << std::endl;
        }
    }

    void on_view_attached_to_window() override {}
    void on_view_detached_from_window() override {}
    void accept(IViewHolderVisitor& visitor) override {}
};

void example_multi_type() {
    std::cout << "\n========== Example 2: Multi-Type List ==========\n";

    auto real_ds = std::make_shared<RealDataSet<BaseData>>();
    auto rv_data_set = std::make_shared<PandoraRealRvDataSet<BaseData>>(real_ds);

    // Register multiple types
    rv_data_set->register_dv_relation<TextData>(
        make_typed_creator<TextData, TextViewHolder>()
    );

    rv_data_set->register_dv_relation<ImageData>(
        make_typed_creator<ImageData, ImageViewHolder>()
    );

    // Add mixed data
    rv_data_set->add(std::make_shared<TextData>("Hello World"));
    rv_data_set->add(std::make_shared<ImageData>("image1.jpg", 800, 600));
    rv_data_set->add(std::make_shared<TextData>("Another text"));
    rv_data_set->add(std::make_shared<ImageData>("image2.png", 1024, 768));

    std::cout << "Total items: " << rv_data_set->get_count() << std::endl;
    std::cout << "View type count: " << rv_data_set->get_view_type_count() << std::endl;
}

// ========== Example 3: Observer Pattern ==========

class TestObserver : public DataObserverBase {
public:
    void on_data_set_changed() override {
        std::cout << "Observer: Data set changed!" << std::endl;
    }

    void notify_item_inserted(int position) override {
        std::cout << "Observer: Item inserted at position " << position << std::endl;
    }

    void notify_item_removed(int position) override {
        std::cout << "Observer: Item removed at position " << position << std::endl;
    }
};

void example_observer() {
    std::cout << "\n========== Example 3: Observer Pattern ==========\n";

    auto real_ds = std::make_shared<RealDataSet<SimpleData>>();
    auto rv_data_set = std::make_shared<PandoraRealRvDataSet<SimpleData>>(real_ds);

    // Add observer
    auto observer = std::make_shared<TestObserver>();
    rv_data_set->add_data_observer(observer);

    // Add data and notify
    rv_data_set->add(std::make_shared<SimpleData>("Item 1", 1));
    rv_data_set->notify_item_inserted(0);

    rv_data_set->add(std::make_shared<SimpleData>("Item 2", 2));
    rv_data_set->notify_item_inserted(1);

    // Remove and notify
    rv_data_set->remove_item_at(0);
    rv_data_set->notify_item_removed(0);

    // Notify full change
    rv_data_set->notify_changed();
}

// ========== Example 4: Wrapper DataSet (Hierarchical) ==========

void example_wrapper_dataset() {
    std::cout << "\n========== Example 4: Wrapper DataSet ==========\n";

    // Create wrapper
    auto wrapper_ds = std::make_shared<WrapperDataSet<SimpleData>>();
    auto rv_data_set = std::make_shared<PandoraWrapperRvDataSet<SimpleData>>(wrapper_ds);

    // Create child adapters
    auto header_ds = std::make_shared<RealDataSet<SimpleData>>();
    auto content_ds = std::make_shared<RealDataSet<SimpleData>>();
    auto footer_ds = std::make_shared<RealDataSet<SimpleData>>();

    // Add data to children
    header_ds->add(std::make_shared<SimpleData>("Header", 0));
    content_ds->add(std::make_shared<SimpleData>("Content 1", 1));
    content_ds->add(std::make_shared<SimpleData>("Content 2", 2));
    content_ds->add(std::make_shared<SimpleData>("Content 3", 3));
    footer_ds->add(std::make_shared<SimpleData>("Footer", 99));

    // Add children to wrapper
    rv_data_set->add_sub(header_ds);
    rv_data_set->add_sub(content_ds);
    rv_data_set->add_sub(footer_ds);

    std::cout << "Total items in wrapper: " << rv_data_set->get_count() << std::endl;

    // Access all items
    for (int i = 0; i < rv_data_set->get_count(); i++) {
        auto item = rv_data_set->get_item(i);
        if (item) {
            std::cout << "Position " << i << ": " << item->title << std::endl;
        }
    }
}

// ========== Example 5: Transaction ==========

void example_transaction() {
    std::cout << "\n========== Example 5: Transaction ==========\n";

    auto real_ds = std::make_shared<RealDataSet<SimpleData>>();
    auto rv_data_set = std::make_shared<PandoraRealRvDataSet<SimpleData>>(real_ds);

    auto observer = std::make_shared<TestObserver>();
    rv_data_set->add_data_observer(observer);

    std::cout << "Adding items in transaction (should notify once):" << std::endl;

    // Start transaction
    rv_data_set->start_transaction();

    // Batch operations
    for (int i = 0; i < 5; i++) {
        rv_data_set->add(std::make_shared<SimpleData>("Item " + std::to_string(i), i));
    }

    // End transaction (triggers single notification)
    rv_data_set->end_transaction();

    std::cout << "Total items: " << rv_data_set->get_count() << std::endl;
}

// ========== Main ==========

int main() {
    std::cout << "PandoraRV C++ Examples\n";
    std::cout << "=====================\n";

    try {
        example_simple_list();
        example_multi_type();
        example_observer();
        example_wrapper_dataset();
        example_transaction();

        std::cout << "\n✅ All examples completed successfully!\n";

    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

