/*
 * MIT License
 *
 * Copyright (c) 2018 leobert-lan
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 */

#ifndef PANDORA_RV_I_REACTIVE_VIEW_HOLDER_H
#define PANDORA_RV_I_REACTIVE_VIEW_HOLDER_H

#include <memory>
#include "i_view_holder.h"
#include "reactive_data.h"
#include "../logger.h"

namespace pandora {
namespace rv {

/**
 * @brief ViewHolder interface with reactive data binding support
 *
 * This interface extends IViewHolder to support reactive data binding.
 * When bound data properties change, the ViewHolder is automatically
 * notified and can update its UI accordingly.
 *
 * @tparam DATA The data type
 *
 * Example:
 * @code
 * class MyReactiveViewHolder : public IReactiveViewHolder<MyReactiveData> {
 * public:
 *     void set_data(std::shared_ptr<MyReactiveData> data) override {
 *         data_ = data;
 *         update_ui();
 *     }
 *
 *     std::shared_ptr<ReactiveData<MyReactiveData>> get_reactive_data_if_exist() override {
 *         return data_;
 *     }
 *
 *     void on_property_changed(std::shared_ptr<MyReactiveData> data, int property_id) override {
 *         // Update only the changed property's UI
 *         switch (property_id) {
 *             case MyReactiveData::PROPERTY_NAME:
 *                 update_name_ui();
 *                 break;
 *         }
 *     }
 *
 * private:
 *     std::shared_ptr<MyReactiveData> data_;
 * };
 * @endcode
 */
template<typename DATA>
class IReactiveViewHolder : public IViewHolder<DATA> {
public:
    /**
     * @brief Get the reactive data if it exists
     *
     * @return The reactive data, or nullptr if not reactive or not bound
     */
    virtual std::shared_ptr<ReactiveData<DATA>> get_reactive_data_if_exist() = 0;

    /**
     * @brief Called when a property of the bound data changes
     *
     * This allows fine-grained UI updates when only specific properties change,
     * rather than re-rendering the entire ViewHolder.
     *
     * @param data The data that changed
     * @param property_id The ID of the property that changed
     */
    virtual void on_property_changed(std::shared_ptr<DATA> data, int property_id) = 0;
};

/**
 * @brief Visitor to ensure reactive ViewHolder unbinding
 *
 * This visitor ensures that reactive ViewHolders properly unbind from
 * their data before being recycled or bound to new data.
 */
class MakeSureUnbindVisitor : public IViewHolderVisitor {
public:
    template<typename DATA>
    void visit(IReactiveViewHolder<DATA>* holder) {
        if (holder) {
            auto old_binding = holder->get_reactive_data_if_exist();
            if (old_binding) {
                old_binding->unbind_reactive_vh();
            }
        }
    }
};

/**
 * @brief Visitor to ensure reactive ViewHolder binding
 *
 * This visitor ensures that reactive ViewHolders properly bind to
 * their new data after being assigned.
 */
class MakeSureBindVisitor : public IViewHolderVisitor {
public:
    template<typename DATA>
    void visit(IReactiveViewHolder<DATA>* holder) {
        if (holder) {
            auto reactive_data = holder->get_reactive_data_if_exist();
            if (reactive_data) {
                try {
                    auto holder_ptr = std::dynamic_pointer_cast<IReactiveViewHolder<DATA>>(
                        holder->shared_from_this()
                    );
                    if (holder_ptr) {
                        reactive_data->bind_reactive_vh(holder_ptr);
                    }
                } catch (const std::exception& e) {
                    Logger::e("MakeSureBindVisitor",
                             std::string("Exception when binding reactive data: ") + e.what());
                }
            }
        }
    }
};

// Global visitor instances for convenience
inline MakeSureUnbindVisitor MAKE_SURE_UNBIND_VISITOR;
inline MakeSureBindVisitor MAKE_SURE_BIND_VISITOR;

/**
 * @brief Helper function to set data to reactive ViewHolder
 *
 * This function ensures proper unbinding and binding when setting data
 * to a reactive ViewHolder. It should be used instead of directly calling
 * set_data() when reactive binding is involved.
 *
 * @tparam DATA The data type
 * @tparam VH The ViewHolder type
 * @param data The data to set
 * @param view_holder The ViewHolder to set data to
 */
template<typename DATA, typename VH>
void help_set_to_reactive_view_holder(std::shared_ptr<ReactiveData<DATA>> data,
                                     std::shared_ptr<VH> view_holder) {
    static_assert(std::is_base_of<IReactiveViewHolder<DATA>, VH>::value,
                  "VH must inherit from IReactiveViewHolder<DATA>");

    if (!view_holder) {
        return;
    }

    // Ensure unbind from old data
    view_holder->accept(MAKE_SURE_UNBIND_VISITOR);

    // Set the new data
    if (data) {
        auto data_ptr = std::dynamic_pointer_cast<DATA>(data);
        if (data_ptr) {
            view_holder->set_data(data_ptr);
        }
    }

    // Ensure bind to new data
    view_holder->accept(MAKE_SURE_BIND_VISITOR);
}

} // namespace rv
} // namespace pandora

#endif // PANDORA_RV_I_REACTIVE_VIEW_HOLDER_H

