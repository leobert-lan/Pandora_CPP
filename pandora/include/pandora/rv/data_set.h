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

#ifndef PANDORA_RV_DATA_SET_H
#define PANDORA_RV_DATA_SET_H

#include <memory>
#include <vector>
#include <algorithm>
#include <mutex>
#include "data_observer.h"
#include "data_vh_mapping_pool.h"
#include "i_view_holder.h"
#include "../data_adapter.h"
#include "../pandora_exception.h"

namespace pandora {
namespace rv {

/**
 * @brief Abstract data set for RecyclerView-like adapters with Multi-Type support
 *
 * This is the base class for all data sets in the PandoraRV framework.
 * It manages data, observers, and the mapping between data types and ViewHolders.
 *
 * @tparam T The data type (must inherit from DataSet::Data)
 *
 * Key features:
 * - Multi-type support through DataVhMappingPool
 * - Observer pattern for change notifications
 * - Weak reference observers to avoid memory leaks
 * - Thread-safe operations
 *
 * Example:
 * @code
 * class MyDataSet : public DataSet<MyData> {
 * public:
 *     int get_count() const override { return data_.size(); }
 *     std::shared_ptr<MyData> get_item(int position) const override {
 *         return position < data_.size() ? data_[position] : nullptr;
 *     }
 * private:
 *     std::vector<std::shared_ptr<MyData>> data_;
 * };
 * @endcode
 */
template<typename T>
class DataSet : public DataAdapter<T> {
public:
    /**
     * @brief Interface for data that can be bound to ViewHolders
     *
     * @tparam DATA The actual data type
     * @tparam VH The ViewHolder type
     */
    template<typename DATA, typename VH>
    class D {
    public:
        virtual ~D() = default;

        /**
         * @brief Bind this data to a ViewHolder
         *
         * @param view_holder The ViewHolder to bind to
         */
        virtual void set_to_view_holder(std::shared_ptr<VH> view_holder) = 0;
    };

    /**
     * @brief Base data interface for Multi-Type support
     *
     * Use this interface when you have multiple data types in the same DataSet.
     */
    class Data : public D<Data, IViewHolder<Data>> {
    public:
        virtual ~Data() = default;
    };

    virtual ~DataSet() = default;

    /**
     * @brief Get the number of items in the data set
     */
    virtual int get_count() const = 0;

    /**
     * @brief Get item at the specified position
     *
     * @param position The position
     * @return The data item, or nullptr if out of range
     */
    virtual std::shared_ptr<T> get_item(int position) const = 0;

    /**
     * @brief Add a data observer
     *
     * @param observer The observer to add (held as weak_ptr)
     */
    void add_data_observer(std::shared_ptr<DataObserver> observer) {
        std::lock_guard<std::mutex> lock(observers_mutex_);
        observers_.push_back(observer);
    }

    /**
     * @brief Remove a data observer
     *
     * @param observer The observer to remove
     */
    void remove_data_observer(const std::shared_ptr<DataObserver>& observer) {
        std::lock_guard<std::mutex> lock(observers_mutex_);
        observers_.erase(
            std::remove_if(observers_.begin(), observers_.end(),
                [&observer](const std::weak_ptr<DataObserver>& weak) {
                    auto ptr = weak.lock();
                    return !ptr || ptr == observer;
                }),
            observers_.end()
        );
    }

    /**
     * @brief Get the mapping pool
     */
    DataVhMappingPool& get_data_vh_mapping_pool() {
        return mapping_pool_;
    }

    const DataVhMappingPool& get_data_vh_mapping_pool() const {
        return mapping_pool_;
    }

    /**
     * @brief Get item view type for position
     *
     * @param pos The position
     * @return The view type ID
     * @throws PandoraException if type is not registered
     */
    int get_item_view_type_v2(int pos) {
        auto data = get_item(pos);
        if (!data) {
            throw PandoraException("Data at position " + std::to_string(pos) + " is null");
        }

        try {
            return mapping_pool_.get_item_view_type(data);
        } catch (const std::exception& e) {
            throw PandoraException(std::string("Error getting view type: ") + e.what());
        }
    }

    /**
     * @brief Create ViewHolder for the given view type
     *
     * @param parent The parent view/container
     * @param view_type The view type ID
     * @return The created ViewHolder
     * @throws PandoraException if creation fails
     */
    std::shared_ptr<IViewHolderBase> create_view_holder_v2(void* parent, int view_type) {
        try {
            return mapping_pool_.create_view_holder(parent, view_type);
        } catch (const std::exception& e) {
            throw PandoraException(std::string("Error creating ViewHolder: ") + e.what());
        }
    }

    /**
     * @brief Get total view type count
     */
    int get_view_type_count() const {
        return mapping_pool_.get_view_type_count();
    }

    /**
     * @brief Register data-ViewHolder relation
     *
     * @tparam DataType The data type
     * @param creator The ViewHolder creator
     * @return Reference to this DataSet for chaining
     */
    template<typename DataType>
    DataSet<T>& register_dv_relation(std::shared_ptr<ViewHolderCreator> creator) {
        mapping_pool_.register_dv_relation<DataType>(creator);
        return *this;
    }

    /**
     * @brief Register custom DVRelation
     *
     * @tparam DataType The data type
     * @param relation The DVRelation
     * @return Reference to this DataSet for chaining
     */
    template<typename DataType>
    DataSet<T>& register_dv_relation(std::shared_ptr<DVRelation<DataType>> relation) {
        mapping_pool_.register_dv_relation<DataType>(relation);
        return *this;
    }

    /**
     * @brief Remove data-ViewHolder relation
     *
     * @tparam DataType The data type to remove
     * @return Reference to this DataSet for chaining
     */
    template<typename DataType>
    DataSet<T>& remove_dv_relation() {
        mapping_pool_.remove_dv_relation<DataType>();
        return *this;
    }

    // ========== Notification Methods ==========

    /**
     * @brief Notify all observers that the entire data set has changed
     */
    void notify_changed() {
        notify_observers([](std::shared_ptr<DataObserver> obs) {
            obs->on_data_set_changed();
        });
    }

    /**
     * @brief Notify that an item has changed
     */
    void notify_item_changed(int position) {
        notify_observers([position](std::shared_ptr<DataObserver> obs) {
            obs->notify_item_changed(position);
        });
    }

    /**
     * @brief Notify that an item has changed with payload
     */
    void notify_item_changed(int position, std::shared_ptr<void> payload) {
        notify_observers([position, payload](std::shared_ptr<DataObserver> obs) {
            obs->notify_item_changed(position, payload);
        });
    }

    /**
     * @brief Notify that a range of items has changed
     */
    void notify_item_range_changed(int position_start, int item_count) {
        notify_observers([position_start, item_count](std::shared_ptr<DataObserver> obs) {
            obs->notify_item_range_changed(position_start, item_count);
        });
    }

    /**
     * @brief Notify that a range of items has changed with payload
     */
    void notify_item_range_changed(int position_start, int item_count, std::shared_ptr<void> payload) {
        notify_observers([position_start, item_count, payload](std::shared_ptr<DataObserver> obs) {
            obs->notify_item_range_changed(position_start, item_count, payload);
        });
    }

    /**
     * @brief Notify that an item has been inserted
     */
    void notify_item_inserted(int position) {
        notify_observers([position](std::shared_ptr<DataObserver> obs) {
            obs->notify_item_inserted(position);
        });
    }

    /**
     * @brief Notify that an item has been moved
     */
    void notify_item_moved(int from_position, int to_position) {
        notify_observers([from_position, to_position](std::shared_ptr<DataObserver> obs) {
            obs->notify_item_moved(from_position, to_position);
        });
    }

    /**
     * @brief Notify that a range of items has been inserted
     */
    void notify_item_range_inserted(int position_start, int item_count) {
        notify_observers([position_start, item_count](std::shared_ptr<DataObserver> obs) {
            obs->notify_item_range_inserted(position_start, item_count);
        });
    }

    /**
     * @brief Notify that an item has been removed
     */
    void notify_item_removed(int position) {
        notify_observers([position](std::shared_ptr<DataObserver> obs) {
            obs->notify_item_removed(position);
        });
    }

    /**
     * @brief Notify that a range of items has been removed
     */
    void notify_item_range_removed(int position_start, int item_count) {
        notify_observers([position_start, item_count](std::shared_ptr<DataObserver> obs) {
            obs->notify_item_range_removed(position_start, item_count);
        });
    }

    /**
     * @brief Helper to set data to ViewHolder with reactive binding support
     *
     * @tparam DATA The data type
     * @tparam VH The ViewHolder type
     * @param data The data
     * @param view_holder The ViewHolder
     */
    template<typename DATA, typename VH>
    static void help_set_to_view_holder(std::shared_ptr<D<DATA, VH>> data,
                                       std::shared_ptr<VH> view_holder) {
        // This will be extended in reactive support
        if (data && view_holder) {
            data->set_to_view_holder(view_holder);
        }
    }

protected:
    DataSet() = default;

private:
    /**
     * @brief Notify all observers with a given action
     *
     * Automatically cleans up expired weak_ptr references.
     */
    template<typename Func>
    void notify_observers(Func&& func) {
        std::lock_guard<std::mutex> lock(observers_mutex_);

        // Remove expired observers and notify active ones
        observers_.erase(
            std::remove_if(observers_.begin(), observers_.end(),
                [&func](std::weak_ptr<DataObserver>& weak) {
                    auto obs = weak.lock();
                    if (!obs) {
                        return true;  // Remove expired
                    }
                    func(obs);
                    return false;  // Keep active
                }),
            observers_.end()
        );
    }

    DataVhMappingPool mapping_pool_;
    std::vector<std::weak_ptr<DataObserver>> observers_;
    mutable std::mutex observers_mutex_;
};

} // namespace rv
} // namespace pandora

#endif // PANDORA_RV_DATA_SET_H

