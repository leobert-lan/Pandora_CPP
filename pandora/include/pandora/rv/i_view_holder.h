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

#ifndef PANDORA_RV_I_VIEW_HOLDER_H
#define PANDORA_RV_I_VIEW_HOLDER_H

#include <memory>

namespace pandora {
namespace rv {

// Forward declarations
class IViewHolderVisitor;

/**
 * @brief Abstract ViewHolder interface for RecyclerView-like components
 *
 * This interface defines the contract for ViewHolders in the Pandora RV framework.
 * It supports type-safe data binding and the Visitor pattern for extensibility.
 *
 * @tparam DATA The type of data this ViewHolder can display
 *
 * @note In C++, we use template instead of Java's generic to achieve type safety.
 *       The lifecycle callbacks allow proper resource management.
 *
 * Example usage:
 * @code
 * class MyViewHolder : public IViewHolder<MyData> {
 * public:
 *     void SetData(std::shared_ptr<MyData> data) override {
 *         data_ = data;
 *         // Update UI with data
 *     }
 *
 *     void OnViewAttachedToWindow() override {
 *         // Start animations, register listeners
 *     }
 *
 *     void OnViewDetachedFromWindow() override {
 *         // Stop animations, unregister listeners
 *     }
 * };
 * @endcode
 */
template<typename DATA>
class IViewHolder {
public:
    virtual ~IViewHolder() = default;

    /**
     * @brief Set data to this ViewHolder
     *
     * Invoked when the adapter binds this ViewHolder to display data.
     * This is analogous to RecyclerView.Adapter#onBindViewHolder().
     *
     * @param data The data to be displayed by this ViewHolder
     */
    virtual void SetData(std::shared_ptr<DATA> data) = 0;

    /**
     * @brief Called when the view is attached to the window
     *
     * This is the ideal place to start animations, register listeners,
     * or perform other operations that require the view to be visible.
     */
    virtual void OnViewAttachedToWindow() = 0;

    /**
     * @brief Called when the view is detached from the window
     *
     * This is the ideal place to stop animations, unregister listeners,
     * or perform cleanup to prevent memory leaks.
     */
    virtual void OnViewDetachedFromWindow() = 0;

    /**
     * @brief Accept a visitor for double dispatch
     *
     * This method implements the Visitor pattern, allowing external
     * operations to be performed on different ViewHolder types without
     * modifying their classes.
     *
     * @param visitor The visitor to accept
     */
    virtual void accept(IViewHolderVisitor& visitor) = 0;

protected:
    IViewHolder() = default;
    IViewHolder(const IViewHolder&) = default;
    IViewHolder& operator=(const IViewHolder&) = default;
    IViewHolder(IViewHolder&&) = default;
    IViewHolder& operator=(IViewHolder&&) = default;
};

/**
 * @brief Abstract visitor for ViewHolders
 *
 * Implements the Visitor pattern for type-safe operations on different
 * ViewHolder types. Subclass this to add new operations without modifying
 * ViewHolder implementations.
 *
 * Example:
 * @code
 * class MyVisitor : public IViewHolderVisitor {
 * public:
 *     void visit_reactive_view_holder(IReactiveViewHolder* holder) override {
 *         // Perform reactive-specific operations
 *     }
 * };
 * @endcode
 */
class IViewHolderVisitor {
public:
    virtual ~IViewHolderVisitor() = default;

    // Default implementation does nothing
    // Subclasses override specific visit methods they care about

protected:
    IViewHolderVisitor() = default;
};

/**
 * @brief Type-erased ViewHolder wrapper
 *
 * This class provides a type-erased interface to ViewHolder, useful when
 * you need to store ViewHolders of different data types in a single container.
 *
 * @note This is a workaround for C++ templates' type erasure at compile time,
 *       similar to how Java's generics work at runtime.
 */
class IViewHolderBase {
public:
    virtual ~IViewHolderBase() = default;

    virtual void OnViewAttachedToWindow() = 0;
    virtual void OnViewDetachedFromWindow() = 0;
    virtual void accept(IViewHolderVisitor& visitor) = 0;

protected:
    IViewHolderBase() = default;
};

/**
 * @brief Template implementation of type-erased ViewHolder
 *
 * This bridges the gap between templated IViewHolder<DATA> and
 * non-templated IViewHolderBase.
 */
template<typename DATA>
class ViewHolderWrapper : public IViewHolderBase {
public:
    explicit ViewHolderWrapper(std::shared_ptr<IViewHolder<DATA>> holder)
        : holder_(std::move(holder)) {}

    void OnViewAttachedToWindow() override {
        if (holder_) holder_->OnViewAttachedToWindow();
    }

    void OnViewDetachedFromWindow() override {
        if (holder_) holder_->OnViewDetachedFromWindow();
    }

    void accept(IViewHolderVisitor& visitor) override {
        if (holder_) holder_->accept(visitor);
    }

    std::shared_ptr<IViewHolder<DATA>> GetHolder() const {
        return holder_;
    }

private:
    std::shared_ptr<IViewHolder<DATA>> holder_;
};

} // namespace rv
} // namespace pandora

#endif // PANDORA_RV_I_VIEW_HOLDER_H

