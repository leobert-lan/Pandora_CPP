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

#ifndef PANDORA_RV_DATA_VH_MAPPING_POOL_H
#define PANDORA_RV_DATA_VH_MAPPING_POOL_H

#include <memory>
#include <mutex>
#include <unordered_map>
#include <typeindex>
#include <stdexcept>
#include "type_cell.h"
#include "view_holder_creator.h"
#include "../logger.h"

namespace pandora {
namespace rv {

/**
 * @brief Pool for managing data-to-ViewHolder mappings
 *
 * This class maintains the registry of data types to ViewHolder creators.
 * It calculates unique view type IDs and creates ViewHolder instances.
 *
 * The pool uses a multiplier strategy to ensure each data type gets a range
 * of view type IDs, allowing for multi-type support (1:N mappings).
 *
 * Example:
 * @code
 * DataVhMappingPool pool;
 *
 * // Register single-type mapping
 * pool.register_dv_relation<MyData>(
 *     make_lambda_creator<MyData>([](void* parent) {
 *         return std::make_shared<MyViewHolder>(parent);
 *     })
 * );
 *
 * // Get view type
 * auto data = std::make_shared<MyData>();
 * int view_type = pool.get_item_view_type(data);
 *
 * // Create ViewHolder
 * auto holder = pool.create_view_holder(parent, view_type);
 * @endcode
 */
class DataVhMappingPool {
public:
    DataVhMappingPool() : max_size_(5), type_cell_key_(0) {}

    /**
     * @brief Register a data-ViewHolder relation
     *
     * @tparam T The data type
     * @param creator The ViewHolder creator
     */
    template<typename T>
    void register_dv_relation(std::shared_ptr<ViewHolderCreator> creator) {
        auto relation = std::make_shared<DataVhRelation<T>>(
            std::type_index(typeid(T)), creator);
        register_dv_relation_internal(relation);
    }

    /**
     * @brief Register a custom DVRelation
     *
     * @tparam T The data type
     * @param relation The custom DVRelation
     */
    template<typename T>
    void register_dv_relation(std::shared_ptr<DVRelation<T>> relation) {
        register_dv_relation_internal(relation);
    }

    /**
     * @brief Remove data-ViewHolder relation for a data type
     *
     * @tparam T The data type to remove
     */
    template<typename T>
    void remove_dv_relation() {
        remove_dv_relation(std::type_index(typeid(T)));
    }

    /**
     * @brief Remove data-ViewHolder relation by type index
     *
     * @param type_idx The type index to remove
     */
    void remove_dv_relation(std::type_index type_idx) {
        std::lock_guard<std::mutex> lock(mutex_);

        for (auto it = view_type_cache_.begin(); it != view_type_cache_.end();) {
            if (it->second && it->second->WorkFor(type_idx)) {
                Logger::i("DataVhMappingPool", "Removing relation for type");
                it = view_type_cache_.erase(it);
            } else {
                ++it;
            }
        }

        // Remove from typed cache as well
        typed_cells_.erase(type_idx);
    }

    /**
     * @brief Get item view type for data
     *
     * @tparam T The data type
     * @param data The data instance
     * @return The view type ID
     * @throws std::runtime_error if type is not registered
     */
    template<typename T>
    int GetItemViewType(std::shared_ptr<T> data) {
        std::type_index type_idx(typeid(T));

        // Find the typed cell for this data type
        auto typed_it = typed_cells_.find(type_idx);
        if (typed_it != typed_cells_.end()) {
            auto typed_cell = std::static_pointer_cast<TypedTypeCell<T>>(typed_it->second);
            return typed_cell->GetItemViewType(data);
        }

        // Fallback: search in view_type_cache_
        for (const auto& pair : view_type_cache_) {
            if (pair.second && pair.second->WorkFor(type_idx)) {
                // Found it, but we need the typed version for proper handling
                // This shouldn't happen in normal usage
                Logger::w("DataVhMappingPool", "Type found in cache but not in typed_cells");
                return pair.second->GetItemViewType(DVRelation<T>::SINGLE_TYPE_TOKEN);
            }
        }

        // Not registered
        std::string error = "Type not registered: " + std::string(type_idx.name());
        Logger::e("DataVhMappingPool", error);
        throw std::runtime_error(error);
    }

    /**
     * @brief Get total view type count
     */
    int GetViewTypeCount() const {
        std::lock_guard<std::mutex> lock(mutex_);

        int count = 0;
        for (const auto& pair : view_type_cache_) {
            if (pair.second) {
                count += pair.second->GetSubTypeCount();
            }
        }

        if (internal_error_cell_) {
            count += 1;
        }

        return count;
    }

    /**
     * @brief Create ViewHolder for the given view type
     *
     * @param parent The parent view/container
     * @param view_type The view type ID
     * @return The created ViewHolder
     */
    std::shared_ptr<IViewHolderBase> CreateViewHolder(void* parent, int view_type) {
        try {
            int index = view_type / max_size_;
            int sub_index = view_type % max_size_;

            Logger::v("DataVhMappingPool",
                     "createViewHolder: index=" + std::to_string(index) +
                     ", subIndex=" + std::to_string(sub_index) +
                     ", viewType=" + std::to_string(view_type));

            auto it = view_type_cache_.find(index);
            if (it != view_type_cache_.end() && it->second) {
                auto creator_func = it->second->GetVhCreatorFunc(sub_index);
                if (creator_func) {
                    auto creator = creator_func();
                    if (creator) {
                        return creator->CreateViewHolder(parent);
                    }
                }
            }

            throw std::runtime_error("No creator found for view type: " + std::to_string(view_type));

        } catch (const std::exception& e) {
            Logger::e("DataVhMappingPool", std::string("Error creating ViewHolder: ") + e.what());

            // Use error cell if available
            if (internal_error_cell_) {
                auto creator_func = internal_error_cell_->GetVhCreatorFunc(0);
                if (creator_func) {
                    auto creator = creator_func();
                    if (creator) {
                        return creator->CreateViewHolder(parent);
                    }
                }
            }

            throw;
        }
    }

    /**
     * @brief Set error ViewHolder creator for internal errors
     *
     * @param creator The error ViewHolder creator
     */
    void when_internal_error(std::shared_ptr<ViewHolderCreator> creator) {
        internal_error_cell_ = std::make_shared<TypeCell>(
            std::numeric_limits<int>::max(),
            std::type_index(typeid(void)),
            1
        );
        internal_error_cell_->RegisterCreator(
            DVRelation<void>::SINGLE_TYPE_TOKEN,
            [creator]() { return creator; }
        );
    }

    /**
     * @brief Merge another pool into this one
     *
     * @param other The pool to merge from
     */
    void merge(const DataVhMappingPool& other) {
        std::lock_guard<std::mutex> lock(mutex_);

        // Copy all cells from other pool
        for (const auto& pair : other.view_type_cache_) {
            if (pair.second) {
                // Re-register with new index
                // Note: This is simplified; in production, you'd preserve the DVRelation
                view_type_cache_[type_cell_key_] = pair.second;
                type_cell_key_++;
            }
        }

        // Update max_size if needed
        if (other.max_size_ > max_size_) {
            max_size_ = other.max_size_;
            for (auto& pair : view_type_cache_) {
                if (pair.second) {
                    pair.second->UpdateMaxSize(max_size_);
                }
            }
        }
    }

private:
    template<typename T>
    void register_dv_relation_internal(std::shared_ptr<DVRelation<T>> relation) {
        std::lock_guard<std::mutex> lock(mutex_);

        int n = relation->one_to_n();

        // Expand max_size if needed
        while (n > max_size_) {
            max_size_ *= 2;
            for (auto& pair : view_type_cache_) {
                if (pair.second) {
                    pair.second->UpdateMaxSize(max_size_);
                }
            }
        }

        // Create TypeCell
        auto typed_cell = std::make_shared<TypedTypeCell<T>>(type_cell_key_, relation);

        // Store in both caches
        view_type_cache_[type_cell_key_] = std::make_shared<TypeCell>(typed_cell->get_cell());
        typed_cells_[relation->get_data_type()] = typed_cell;

        Logger::i("DataVhMappingPool",
                 "Registered DV relation with key: " + std::to_string(type_cell_key_));

        type_cell_key_++;
    }

    mutable std::mutex mutex_;
    std::unordered_map<int, std::shared_ptr<TypeCell>> view_type_cache_;
    std::unordered_map<std::type_index, std::shared_ptr<void>> typed_cells_;  // Type-erased TypedTypeCell
    std::shared_ptr<TypeCell> internal_error_cell_;
    int max_size_;
    int type_cell_key_;
};

} // namespace rv
} // namespace pandora

#endif // PANDORA_RV_DATA_VH_MAPPING_POOL_H

