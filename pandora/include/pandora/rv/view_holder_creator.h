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

#ifndef PANDORA_RV_VIEW_HOLDER_CREATOR_H
#define PANDORA_RV_VIEW_HOLDER_CREATOR_H

#include <memory>
#include <functional>
#include "i_view_holder.h"

namespace pandora {
namespace rv {

/**
 * @brief Abstract factory for creating ViewHolders
 *
 * This class defines the interface for ViewHolder creation. Subclass this
 * to implement custom creation logic, or use the provided helper classes
 * for common scenarios.
 *
 * Example:
 * @code
 * class MyViewHolderCreator : public ViewHolderCreator {
 * public:
 *     std::shared_ptr<IViewHolderBase> create_view_holder(void* parent) override {
 *         auto holder = std::make_shared<MyViewHolder>(parent);
 *         return std::make_shared<ViewHolderWrapper<MyData>>(holder);
 *     }
 * };
 * @endcode
 */
class ViewHolderCreator {
public:
    virtual ~ViewHolderCreator() = default;

    /**
     * @brief Create a ViewHolder instance
     *
     * @param parent The parent view/container (platform-specific)
     * @return A new ViewHolder instance wrapped in type-erased interface
     */
    virtual std::shared_ptr<IViewHolderBase> create_view_holder(void* parent) = 0;

protected:
    ViewHolderCreator() = default;
};

/**
 * @brief Function-based ViewHolder creator
 *
 * This class wraps a function or lambda for ViewHolder creation,
 * providing a convenient way to register creators without defining
 * new classes.
 *
 * @tparam DATA The data type the ViewHolder displays
 *
 * Example:
 * @code
 * auto creator = std::make_shared<LambdaViewHolderCreator<MyData>>(
 *     [](void* parent) {
 *         return std::make_shared<MyViewHolder>(parent);
 *     }
 * );
 * @endcode
 */
template<typename DATA>
class LambdaViewHolderCreator : public ViewHolderCreator {
public:
    using CreatorFunc = std::function<std::shared_ptr<IViewHolder<DATA>>(void*)>;

    /**
     * @brief Construct with a creator function
     * @param func Function that creates ViewHolder instances
     */
    explicit LambdaViewHolderCreator(CreatorFunc func)
        : func_(std::move(func)) {}

    std::shared_ptr<IViewHolderBase> create_view_holder(void* parent) override {
        auto holder = func_(parent);
        return std::make_shared<ViewHolderWrapper<DATA>>(holder);
    }

private:
    CreatorFunc func_;
};

/**
 * @brief Type-specific ViewHolder creator
 *
 * This template class provides type-safe ViewHolder creation for a specific
 * DATA and ViewHolder type. It uses perfect forwarding to pass arguments
 * to the ViewHolder constructor.
 *
 * @tparam DATA The data type the ViewHolder displays
 * @tparam VH The ViewHolder type to create (must inherit from IViewHolder<DATA>)
 *
 * Example:
 * @code
 * auto creator = std::make_shared<TypedViewHolderCreator<MyData, MyViewHolder>>();
 * @endcode
 */
template<typename DATA, typename VH>
class TypedViewHolderCreator : public ViewHolderCreator {
    static_assert(std::is_base_of<IViewHolder<DATA>, VH>::value,
                  "VH must inherit from IViewHolder<DATA>");

public:
    TypedViewHolderCreator() = default;

    std::shared_ptr<IViewHolderBase> create_view_holder(void* parent) override {
        auto holder = std::make_shared<VH>(parent);
        return std::make_shared<ViewHolderWrapper<DATA>>(holder);
    }
};

/**
 * @brief Helper function to create a lambda-based ViewHolder creator
 *
 * This is a convenience function that deduces template parameters automatically.
 *
 * @tparam DATA The data type (usually auto-deduced from lambda return type)
 * @tparam Func The function type (auto-deduced)
 * @param func The creator function
 * @return A shared pointer to the creator
 *
 * Example:
 * @code
 * auto creator = make_lambda_creator<MyData>([](void* parent) {
 *     return std::make_shared<MyViewHolder>(parent);
 * });
 * @endcode
 */
template<typename DATA, typename Func>
std::shared_ptr<ViewHolderCreator> make_lambda_creator(Func&& func) {
    return std::make_shared<LambdaViewHolderCreator<DATA>>(std::forward<Func>(func));
}

/**
 * @brief Helper function to create a typed ViewHolder creator
 *
 * This is a convenience function for creating TypedViewHolderCreator instances.
 *
 * @tparam DATA The data type
 * @tparam VH The ViewHolder type
 * @return A shared pointer to the creator
 *
 * Example:
 * @code
 * auto creator = make_typed_creator<MyData, MyViewHolder>();
 * @endcode
 */
template<typename DATA, typename VH>
std::shared_ptr<ViewHolderCreator> make_typed_creator() {
    return std::make_shared<TypedViewHolderCreator<DATA, VH>>();
}

} // namespace rv
} // namespace pandora

#endif // PANDORA_RV_VIEW_HOLDER_CREATOR_H

