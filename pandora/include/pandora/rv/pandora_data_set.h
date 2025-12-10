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
 * @tparam DS The PandoraBoxAdapter type
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
template<typename T, typename DS>
class PandoraDataSet : public DataSet<T> {
    static_assert(std::is_base_of<PandoraBoxAdapter<T>, DS>::value,
                  "DS must inherit from PandoraBoxAdapter<T>");

public:
    /**
     * @brief Construct with a PandoraBoxAdapter
     *
     * @param data_set The underlying data adapter
     */
    explicit PandoraDataSet(std::shared_ptr<DS> data_set)
        : data_set_(std::move(data_set)) {
        if (!data_set_) {
            throw PandoraException("PandoraDataSet: data_set cannot be null");
        }
    }

    ~PandoraDataSet() override = default;

    /**
     * @brief Get the underlying data adapter
     */
    std::shared_ptr<DS> get_data_set() const {
        return data_set_;
    }

    // ========== DataSet Interface Implementation ==========

    int get_count() const override {
        return data_set_->GetDataCount();
    }

    std::shared_ptr<T> get_item(int position) const override {
        T* raw_ptr = data_set_->GetDataByIndex(position);
        // Non-owning shared_ptr for compatibility with DataSet interface
        return raw_ptr ? std::shared_ptr<T>(raw_ptr, [](T*){}) : nullptr;
    }

    // ========== PandoraBoxAdapter Delegation ==========

    /**
     * @brief Start a transaction for batch operations
     */
    void StartTransaction() {
        data_set_->StartTransaction();
    }

    /**
     * @brief End transaction and notify observers
     */
    void EndTransaction() {
        data_set_->EndTransaction();
    }

    /**
     * @brief End transaction silently without notifications
     */
    void EndTransactionSilently() {
        data_set_->EndTransactionSilently();
    }

    /**
     * @brief Get the alias of this adapter
     */
    std::string GetAlias() const {
        return data_set_->GetAlias();
    }

    /**
     * @brief Set the alias of this adapter
     */
    void SetAlias(const std::string& alias) {
        try {
            data_set_->SetAlias(alias);
        } catch (const PandoraException& e) {
            Logger::e("PandoraDataSet", std::string("Error setting alias: ") + e.what());
            throw;
        }
    }

    /**
     * @brief Retrieve adapter by data index
     */
    PandoraBoxAdapter<T>* RetrieveAdapterByDataIndex(int index) {
        return data_set_->RetrieveAdapterByDataIndex(index);
    }

    /**
     * @brief Retrieve adapter and local index by data index
     */
    std::pair<PandoraBoxAdapter<T>*, int>
    RetrieveAdapterByDataIndex2(int index) {
        return data_set_->RetrieveAdapterByDataIndex2(index);
    }

    /**
     * @brief Get the start index in parent
     */
    int GetStartIndex() const {
        return data_set_->GetStartIndex();
    }

    /**
     * @brief Find child adapter by alias
     */
    PandoraBoxAdapter<T>* FindByAlias(const std::string& target_alias) {
        return data_set_->FindByAlias(target_alias);
    }

    /**
     * @brief Run foreach action on all data items
     */
    template<typename Consumer>
    void RunForeach(Consumer&& action) {
        data_set_->RunForeach(std::forward<Consumer>(action));
    }

    /**
     * @brief Get group index
     */
    int GetGroupIndex() const {
        return data_set_->GetGroupIndex();
    }

    /**
     * @brief Add a child adapter (using unique_ptr as per PandoraBoxAdapter API)
     */
    void AddChild(std::unique_ptr<PandoraBoxAdapter<T>> sub) {
        data_set_->AddChild(std::move(sub));
    }

    /**
     * @brief Check if bound to parent
     */
    bool HasBindToParent() const {
        return data_set_->HasBindToParent();
    }

    /**
     * @brief Remove from original parent
     */
    void RemoveFromOriginalParent() {
        data_set_->NotifyHasRemoveFromParent();
    }

    /**
     * @brief Remove a child adapter (using raw pointer as per PandoraBoxAdapter API)
     */
    void RemoveChild(PandoraBoxAdapter<T>* sub) {
        data_set_->RemoveChild(sub);
    }

    // ========== DataAdapter Interface (Modern shared_ptr wrapper) ==========
    // These methods provide a modern shared_ptr interface while delegating to
    // the underlying PandoraBoxAdapter's raw pointer API

    /**
     * @brief Get the number of data items
     */
    int GetDataCount() const override
    {
        return data_set_->GetDataCount();
    }

    /**
     * @brief Get data by index (returns raw pointer wrapped in shared_ptr for safety)
     */
    T* GetDataByIndex(int index) override
    {
        return data_set_->GetDataByIndex(index);
    }

    /**
     * @brief Clear all data
     */
    void ClearAllData() override
    {
        data_set_->ClearAllData();
    }

    /**
     * @brief Add an item
     */
    void Add(const T& item) override
    {
        data_set_->Add(item);
    }

    /**
     * @brief Add an item at specific position
     */
    void Add(int pos, const T& item) override
    {
        data_set_->Add(pos, item);
    }

    /**
     * @brief Add multiple items
     */
    void AddAll(const std::vector<T>& collection) override
    {
        data_set_->AddAll(collection);
    }

    /**
     * @brief Remove an item
     */
    void Remove(const T& item) override
    {
        data_set_->Remove(item);
    }

    /**
     * @brief Remove item at position
     */
    void RemoveAtPos(int position) override
    {
        data_set_->RemoveAtPos(position);
    }

    /**
     * @brief Replace item at position if exists
     */
    bool ReplaceAtPosIfExist(int position, const T& item) override
    {
        return data_set_->ReplaceAtPosIfExist(position, item);
    }

    /**
     * @brief Set data collection
     */
    void SetData(const std::vector<T>& collection) override
    {
        data_set_->SetData(collection);
    }

    /**
     * @brief Find index of item
     */
    int IndexOf(const T& item) const override
    {
        return data_set_->IndexOf(item);
    }

protected:
    std::shared_ptr<DS> data_set_;
};

} // namespace rv
} // namespace pandora

#endif // PANDORA_RV_PANDORA_DATA_SET_H

