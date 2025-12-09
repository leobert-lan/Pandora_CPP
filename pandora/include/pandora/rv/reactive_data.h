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

#ifndef PANDORA_RV_REACTIVE_DATA_H
#define PANDORA_RV_REACTIVE_DATA_H

#include <memory>
#include "data_set.h"

namespace pandora {
namespace rv {

// Forward declaration
template<typename DA>
class IReactiveViewHolder;

/**
 * @brief Interface for reactive data that can notify ViewHolders of changes
 *
 * This interface extends DataSet::Data to support reactive data binding.
 * When data properties change, the bound ViewHolder is automatically notified.
 *
 * @tparam DA The actual data type
 *
 * Example:
 * @code
 * class MyReactiveData : public ReactiveData<MyReactiveData> {
 * public:
 *     void bind_reactive_vh(std::shared_ptr<IReactiveViewHolder<MyReactiveData>> vh) override {
 *         view_holder_ = vh;
 *         // Register for property change notifications
 *     }
 *
 *     void unbind_reactive_vh() override {
 *         view_holder_.reset();
 *         // Unregister from property change notifications
 *     }
 *
 *     void set_name(const std::string& name) {
 *         name_ = name;
 *         notify_property_changed(PROPERTY_NAME);
 *     }
 *
 * private:
 *     void notify_property_changed(int property_id) {
 *         if (auto vh = view_holder_.lock()) {
 *             vh->on_property_changed(shared_from_this(), property_id);
 *         }
 *     }
 *
 *     std::weak_ptr<IReactiveViewHolder<MyReactiveData>> view_holder_;
 *     std::string name_;
 *     static constexpr int PROPERTY_NAME = 1;
 * };
 * @endcode
 */
template<typename DA>
class ReactiveData : public DataSet<DA>::Data {
public:
    virtual ~ReactiveData() = default;

    /**
     * @brief Bind this data to a reactive ViewHolder
     *
     * This is called when the ViewHolder is bound to display this data.
     * The data should hold a weak reference to the ViewHolder to send
     * property change notifications.
     *
     * @param view_holder The reactive ViewHolder to bind to
     */
    virtual void bind_reactive_vh(std::shared_ptr<IReactiveViewHolder<DA>> view_holder) = 0;

    /**
     * @brief Unbind from the current reactive ViewHolder
     *
     * This is called when the ViewHolder is recycled or destroyed.
     * The data should clear its reference to the ViewHolder.
     */
    virtual void unbind_reactive_vh() = 0;
};

} // namespace rv
} // namespace pandora

#endif // PANDORA_RV_REACTIVE_DATA_H

