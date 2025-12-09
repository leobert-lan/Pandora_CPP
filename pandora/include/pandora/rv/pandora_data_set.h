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

#ifndef PANDORA_RV_PANDORA_DATA_SET_H
#define PANDORA_RV_PANDORA_DATA_SET_H

#include <memory>
#include "data_set.h"
#include "../pandora_box_adapter.h"
#include "../type_visitor.h"

namespace pandora {
namespace rv {

/**
 * @brief Base DataSet that wraps a PandoraBoxAdapter
 *
 * This class bridges the gap between Pandora's core data structures
 * (PandoraBoxAdapter) and the RecyclerView-specific DataSet interface.
 * It delegates data operations to the underlying PandoraBoxAdapter.
 *
 * @tparam T The data type (must inherit from DataSet::Data)
 * @tparam D The PandoraBoxAdapter type
 *
 * Example:
 * @code
 * auto real_adapter = std::make_shared<RealDataSet<MyData>>();
 * auto data_set = std::make_shared<PandoraDataSet<MyData, RealDataSet<MyData>>>(real_adapter);
 *
 * // Use both interfaces
 * data_set->add(my_data);  // PandoraBoxAdapter method
 * int count = data_set->get_count();  // DataSet method
 * @endcode
 */
template<typename T, typename D>
class PandoraDataSet : public DataSet<T> {
    static_assert(std::is_base_of<PandoraBoxAdapter<T>, D>::value,
                  "D must inherit from PandoraBoxAdapter<T>");

public:
    /**
     * @brief Construct with a PandoraBoxAdapter
     *
     * @param data_set The underlying data adapter
     */
    explicit PandoraDataSet(std::shared_ptr<D> data_set)
        : data_set_(std::move(data_set)) {
        if (!data_set_) {
            throw PandoraException("PandoraDataSet: data_set cannot be null");
        }
    }

    virtual ~PandoraDataSet() = default;

    /**
     * @brief Get the underlying data adapter
     */
    std::shared_ptr<D> get_data_set() const {
        return data_set_;
    }

    // ========== DataSet Interface Implementation ==========

    int get_count() const override {
        return get_data_count();
    }

    std::shared_ptr<T> get_item(int position) const override {
        return data_set_->get_data_by_index(position);
    }

    // ========== PandoraBoxAdapter Delegation ==========

    /**
     * @brief Start a transaction for batch operations
     */
    void start_transaction() {
        data_set_->start_transaction();
    }

    /**
     * @brief End transaction and notify observers
     */
    void end_transaction() {
        data_set_->end_transaction();
    }

    /**
     * @brief End transaction silently without notifications
     */
    void end_transaction_silently() {
        data_set_->end_transaction_silently();
    }

    /**
     * @brief Get the alias of this adapter
     */
    std::string get_alias() const {
        return data_set_->get_alias();
    }

    /**
     * @brief Set the alias of this adapter
     */
    void set_alias(const std::string& alias) {
        try {
            data_set_->set_alias(alias);
        } catch (const PandoraException& e) {
            Logger::e("PandoraDataSet", std::string("Error setting alias: ") + e.what());
            throw;
        }
    }

    /**
     * @brief Retrieve adapter by data index
     */
    std::shared_ptr<PandoraBoxAdapter<T>> retrieve_adapter_by_data_index(int index) {
        return data_set_->retrieve_adapter_by_data_index(index);
    }

    /**
     * @brief Retrieve adapter and local index by data index
     */
    std::pair<std::shared_ptr<PandoraBoxAdapter<T>>, int>
    retrieve_adapter_by_data_index2(int index) {
        return data_set_->retrieve_adapter_by_data_index2(index);
    }

    /**
     * @brief Get the start index in parent
     */
    int get_start_index() const {
        return data_set_->get_start_index();
    }

    /**
     * @brief Find child adapter by alias
     */
    std::shared_ptr<PandoraBoxAdapter<T>> find_by_alias(const std::string& target_alias) {
        return data_set_->find_by_alias(target_alias);
    }

    /**
     * @brief Run foreach action on all data items
     */
    template<typename Consumer>
    void run_foreach(Consumer&& action) {
        data_set_->run_foreach(std::forward<Consumer>(action));
    }

    /**
     * @brief Accept a type visitor
     */
    template<typename R>
    R accept(int pos, TypeVisitor<R>& type_visitor) {
        return data_set_->accept(pos, type_visitor);
    }

    /**
     * @brief Get group index
     */
    int get_group_index() const {
        return data_set_->get_group_index();
    }

    /**
     * @brief Add a child adapter
     */
    void add_child(std::shared_ptr<PandoraBoxAdapter<T>> sub) {
        data_set_->add_child(sub);
    }

    /**
     * @brief Check if bound to parent
     */
    bool has_bind_2_parent() const {
        return data_set_->has_bind_2_parent();
    }

    /**
     * @brief Remove from original parent
     */
    void remove_from_original_parent() {
        data_set_->remove_from_original_parent();
    }

    /**
     * @brief Remove a child adapter
     */
    void remove_child(std::shared_ptr<PandoraBoxAdapter<T>> sub) {
        data_set_->remove_child(sub);
    }

    // ========== DataAdapter Interface ==========

    int get_data_count() const override {
        return data_set_->get_data_count();
    }

    std::shared_ptr<T> get_data_by_index(int index) override {
        return data_set_->get_data_by_index(index);
    }

    void clear_all_data() override {
        data_set_->clear_all_data();
    }

    void add(std::shared_ptr<T> item) override {
        data_set_->add(item);
    }

    void add_at(int index, std::shared_ptr<T> item) override {
        data_set_->add_at(index, item);
    }

    void add_items(const std::vector<std::shared_ptr<T>>& items) override {
        data_set_->add_items(items);
    }

    void add_items_at(int index, const std::vector<std::shared_ptr<T>>& items) override {
        data_set_->add_items_at(index, items);
    }

    void remove_item(std::shared_ptr<T> item) override {
        data_set_->remove_item(item);
    }

    void remove_item_at(int index) override {
        data_set_->remove_item_at(index);
    }

    void remove_items(int start, int count) override {
        data_set_->remove_items(start, count);
    }

    void set_item(int index, std::shared_ptr<T> item) override {
        data_set_->set_item(index, item);
    }

    int index_of(std::shared_ptr<T> item) const override {
        return data_set_->index_of(item);
    }

    bool contains(std::shared_ptr<T> item) const override {
        return data_set_->contains(item);
    }

    std::vector<std::shared_ptr<T>> get_all_data() const override {
        return data_set_->get_all_data();
    }

protected:
    std::shared_ptr<D> data_set_;
};

} // namespace rv
} // namespace pandora

#endif // PANDORA_RV_PANDORA_DATA_SET_H

