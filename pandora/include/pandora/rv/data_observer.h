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

#ifndef PANDORA_RV_DATA_OBSERVER_H
#define PANDORA_RV_DATA_OBSERVER_H

#include <memory>

namespace pandora {
namespace rv {

/**
 * @brief Observer interface for DataSet changes
 *
 * Implement this interface to receive notifications when the underlying
 * data in a DataSet changes. This follows the Observer pattern and is
 * typically used by adapters to update UI components.
 *
 * @note Observers are held with weak_ptr to avoid circular references.
 *       Ensure the observer's lifetime is managed properly.
 *
 * Example:
 * @code
 * class MyAdapter : public DataObserver {
 * public:
 *     void on_data_set_changed() override {
 *         // Refresh entire view
 *     }
 *
 *     void NotifyItemChanged(int position) override {
 *         // Update single item at position
 *     }
 * };
 * @endcode
 */
class DataObserver {
public:
    virtual ~DataObserver() = default;

    /**
     * @brief Called when the entire data set has changed
     *
     * This is invoked when DataSet::NotifyChanged() is called.
     * The observer should refresh its entire view.
     */
    virtual void OnDataSetChanged() = 0;

    /**
     * @brief Called when a single item has changed
     *
     * @param position The position of the changed item
     */
    virtual void NotifyItemChanged(int position) = 0;

    /**
     * @brief Called when a single item has changed with payload
     *
     * @param position The position of the changed item
     * @param payload Additional data about what changed (optional)
     */
    virtual void NotifyItemChanged(int position, std::shared_ptr<void> payload) = 0;

    /**
     * @brief Called when a range of items has changed
     *
     * @param position_start The starting position
     * @param item_count The number of items that changed
     */
    virtual void NotifyItemRangeChanged(int position_start, int item_count) = 0;

    /**
     * @brief Called when a range of items has changed with payload
     *
     * @param position_start The starting position
     * @param item_count The number of items that changed
     * @param payload Additional data about what changed (optional)
     */
    virtual void NotifyItemRangeChanged(int position_start, int item_count,
                                          std::shared_ptr<void> payload) = 0;

    /**
     * @brief Called when a new item has been inserted
     *
     * @param position The position where the item was inserted
     */
    virtual void NotifyItemInserted(int position) = 0;

    /**
     * @brief Called when an item has been moved
     *
     * @param from_position The original position
     * @param to_position The new position
     */
    virtual void NotifyItemMoved(int from_position, int to_position) = 0;

    /**
     * @brief Called when a range of items has been inserted
     *
     * @param position_start The starting position
     * @param item_count The number of items inserted
     */
    virtual void NotifyItemRangeInserted(int position_start, int item_count) = 0;

    /**
     * @brief Called when an item has been removed
     *
     * @param position The position of the removed item
     */
    virtual void NotifyItemRemoved(int position) = 0;

    /**
     * @brief Called when a range of items has been removed
     *
     * @param position_start The starting position
     * @param item_count The number of items removed
     */
    virtual void NotifyItemRangeRemoved(int position_start, int item_count) = 0;

protected:
    DataObserver() = default;
};

/**
 * @brief Base implementation of DataObserver with default no-op implementations
 *
 * Subclass this if you only want to override specific notification methods.
 * All methods have default empty implementations.
 *
 * Example:
 * @code
 * class MySimpleAdapter : public DataObserverBase {
 * public:
 *     void OnDataSetChanged() override {
 *         // Only handle full refresh
 *     }
 *     // Other methods use default no-op implementation
 * };
 * @endcode
 */
class DataObserverBase : public DataObserver {
public:
    void OnDataSetChanged() override {}
    void NotifyItemChanged(int position) override {}
    void NotifyItemChanged(int position, std::shared_ptr<void> payload) override {}
    void NotifyItemRangeChanged(int position_start, int item_count) override {}
    void NotifyItemRangeChanged(int position_start, int item_count,
                                  std::shared_ptr<void> payload) override {}
    void NotifyItemInserted(int position) override {}
    void NotifyItemMoved(int from_position, int to_position) override {}
    void NotifyItemRangeInserted(int position_start, int item_count) override {}
    void NotifyItemRemoved(int position) override {}
    void NotifyItemRangeRemoved(int position_start, int item_count) override {}
};

} // namespace rv
} // namespace pandora

#endif // PANDORA_RV_DATA_OBSERVER_H

