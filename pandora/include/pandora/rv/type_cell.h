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

#ifndef PANDORA_RV_TYPE_CELL_H
#define PANDORA_RV_TYPE_CELL_H

#include <memory>
#include <vector>
#include <string>
#include <typeindex>
#include <functional>
#include "view_holder_creator.h"

namespace pandora {
namespace rv {

/**
 * @brief Represents a data-viewholder relationship
 *
 * This interface defines the contract for mapping data types to ViewHolder creators.
 * It supports both single-type (1:1) and multi-type (1:N) mappings.
 *
 * @tparam T The data type
 */
template<typename T>
class DVRelation {
public:
    static constexpr const char* SINGLE_TYPE_TOKEN = "type_one";

    virtual ~DVRelation() = default;

    /**
     * @brief Get the data class type index
     */
    virtual std::type_index get_data_type() const = 0;

    /**
     * @brief Get the number of sub-types (1 for single-type, N for multi-type)
     */
    virtual int one_to_n() const = 0;

    /**
     * @brief Get sub-type token for the given data instance
     *
     * @param data The data instance
     * @return A string token identifying the sub-type
     */
    virtual std::string sub_type_token(std::shared_ptr<T> data) const = 0;

    /**
     * @brief Get ViewHolder creator for the given sub-type token
     *
     * @param sub_type_token The sub-type token
     * @return The ViewHolder creator
     */
    virtual std::shared_ptr<ViewHolderCreator> get_vh_creator(const std::string& sub_type_token) const = 0;
};

/**
 * @brief Simple 1:1 data-viewholder relation
 *
 * This is the most common case where one data type maps to one ViewHolder type.
 */
template<typename T>
class DataVhRelation : public DVRelation<T> {
public:
    DataVhRelation(std::type_index data_type, std::shared_ptr<ViewHolderCreator> creator)
        : data_type_(data_type), creator_(std::move(creator)) {}

    std::type_index get_data_type() const override {
        return data_type_;
    }

    int one_to_n() const override {
        return 1;
    }

    std::string sub_type_token(std::shared_ptr<T> data) const override {
        return DVRelation<T>::SINGLE_TYPE_TOKEN;
    }

    std::shared_ptr<ViewHolderCreator> get_vh_creator(const std::string& sub_type_token) const override {
        return creator_;
    }

private:
    std::type_index data_type_;
    std::shared_ptr<ViewHolderCreator> creator_;
};

/**
 * @brief Type cell for managing view type calculations
 *
 * Internal helper class that maps data types to view types and manages
 * ViewHolder creation. Each TypeCell represents one DVRelation and is
 * responsible for calculating unique view type IDs.
 */
class TypeCell {
public:
    /**
     * @brief Construct a TypeCell
     *
     * @param index The base index for this cell (assigned by MappingPool)
     * @param data_type The type index of the data class
     * @param sub_type_count Number of sub-types this relation supports
     */
    TypeCell(int index, std::type_index data_type, int sub_type_count)
        : index_(index), data_type_(data_type), sub_type_count_(sub_type_count), max_size_(5) {}

    /**
     * @brief Check if this cell works for the given data type
     */
    bool work_for(std::type_index type) const {
        return data_type_ == type;
    }

    /**
     * @brief Update the max size for view type calculation
     */
    void update_max_size(int max_size) {
        max_size_ = max_size;
    }

    /**
     * @brief Get the number of sub-types
     */
    int get_sub_type_count() const {
        return sub_type_count_;
    }

    /**
     * @brief Get item view type for the given sub-type token
     *
     * @param token The sub-type token
     * @return The calculated view type ID
     */
    int get_item_view_type(const std::string& token) {
        // Find or add the token
        auto it = std::find(sub_type_tokens_.begin(), sub_type_tokens_.end(), token);
        if (it == sub_type_tokens_.end()) {
            sub_type_tokens_.push_back(token);
            it = sub_type_tokens_.end() - 1;
        }

        int sub_index = static_cast<int>(std::distance(sub_type_tokens_.begin(), it));
        return index_ * max_size_ + sub_index;
    }

    /**
     * @brief Get ViewHolder creator by sub-type index
     *
     * @param sub_type_index The index of the sub-type
     * @return The creator function
     */
    std::function<std::shared_ptr<ViewHolderCreator>()> get_vh_creator_func(int sub_type_index) const {
        if (sub_type_index >= 0 && sub_type_index < static_cast<int>(sub_type_tokens_.size())) {
            return creator_funcs_[sub_type_index];
        }
        return nullptr;
    }

    /**
     * @brief Register a creator function for a sub-type token
     */
    void register_creator(const std::string& token, std::function<std::shared_ptr<ViewHolderCreator>()> func) {
        auto it = std::find(sub_type_tokens_.begin(), sub_type_tokens_.end(), token);
        if (it == sub_type_tokens_.end()) {
            sub_type_tokens_.push_back(token);
            creator_funcs_.push_back(std::move(func));
        } else {
            int index = static_cast<int>(std::distance(sub_type_tokens_.begin(), it));
            creator_funcs_[index] = std::move(func);
        }
    }

    int get_index() const { return index_; }
    std::type_index get_data_type() const { return data_type_; }

private:
    int index_;                                                          // Base index assigned by pool
    std::type_index data_type_;                                          // Data type this cell works for
    int sub_type_count_;                                                 // Number of sub-types
    int max_size_;                                                       // Max size for view type calculation
    std::vector<std::string> sub_type_tokens_;                          // Registered sub-type tokens
    std::vector<std::function<std::shared_ptr<ViewHolderCreator>()>> creator_funcs_;  // Creator functions
};

/**
 * @brief Template TypeCell wrapper that holds a DVRelation
 *
 * This class bridges the gap between the type-erased TypeCell and
 * the templated DVRelation.
 */
template<typename T>
class TypedTypeCell {
public:
    TypedTypeCell(int index, std::shared_ptr<DVRelation<T>> relation)
        : cell_(index, relation->get_data_type(), relation->one_to_n()),
          relation_(std::move(relation)) {}

    TypeCell& get_cell() { return cell_; }
    const TypeCell& get_cell() const { return cell_; }

    std::shared_ptr<DVRelation<T>> get_relation() const { return relation_; }

    /**
     * @brief Get item view type for the given data
     */
    int get_item_view_type(std::shared_ptr<T> data) {
        std::string token = relation_->sub_type_token(data);

        // Register creator if not already registered
        if (!has_creator_for_token(token)) {
            register_creator_for_token(token);
        }

        return cell_.get_item_view_type(token);
    }

private:
    bool has_creator_for_token(const std::string& token) const {
        // Simple check - in real implementation, TypeCell would track this
        return false;  // Always register for simplicity
    }

    void register_creator_for_token(const std::string& token) {
        auto creator = relation_->get_vh_creator(token);
        cell_.register_creator(token, [creator]() { return creator; });
    }

    TypeCell cell_;
    std::shared_ptr<DVRelation<T>> relation_;
};

} // namespace rv
} // namespace pandora

#endif // PANDORA_RV_TYPE_CELL_H

