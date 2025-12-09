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

#ifndef PANDORA_RV_PANDORA_WRAPPER_RV_DATA_SET_H
#define PANDORA_RV_PANDORA_WRAPPER_RV_DATA_SET_H

#include "pandora_data_set.h"
#include "../wrapper_data_set.h"

namespace pandora {
namespace rv {

/**
 * @brief DataSet implementation that wraps a WrapperDataSet
 *
 * This class combines the RecyclerView DataSet interface with Pandora's
 * WrapperDataSet, which can contain multiple child adapters and manage
 * complex hierarchical data structures.
 *
 * @tparam T The data type (must inherit from DataSet::Data)
 *
 * Example:
 * @code
 * auto wrapper_ds = std::make_shared<WrapperDataSet<MyData>>();
 * auto rv_data_set = std::make_shared<PandoraWrapperRvDataSet<MyData>>(wrapper_ds);
 *
 * // Create child adapters
 * auto child1 = std::make_shared<RealDataSet<MyData>>();
 * auto child2 = std::make_shared<RealDataSet<MyData>>();
 *
 * // Add children to wrapper
 * rv_data_set->add_sub(child1);
 * rv_data_set->add_sub(child2);
 *
 * // Register ViewHolder mappings
 * rv_data_set->register_dv_relation<MyData>(creator);
 *
 * // Merge another mapping pool (useful for complex multi-type scenarios)
 * DataVhMappingPool other_pool;
 * rv_data_set->merge(other_pool);
 * @endcode
 */
template<typename T>
class PandoraWrapperRvDataSet : public PandoraDataSet<T, WrapperDataSet<T>> {
public:
    /**
     * @brief Construct with a WrapperDataSet
     *
     * @param wrapper_data_set The underlying WrapperDataSet
     */
    explicit PandoraWrapperRvDataSet(std::shared_ptr<WrapperDataSet<T>> wrapper_data_set)
        : PandoraDataSet<T, WrapperDataSet<T>>(std::move(wrapper_data_set)) {}

    /**
     * @brief Set the group index
     *
     * @param group_index The group index
     */
    void set_group_index(int group_index) {
        this->data_set_->set_group_index(group_index);
    }

    /**
     * @brief Add a sub adapter
     *
     * @param sub The child adapter to add
     */
    void add_sub(std::shared_ptr<PandoraBoxAdapter<T>> sub) {
        this->data_set_->add_child(sub);
    }

    /**
     * @brief Merge another mapping pool into this one
     *
     * This is useful when different child adapters have their own
     * ViewHolder type mappings that need to be combined.
     *
     * @param pool The mapping pool to merge
     */
    void merge(const DataVhMappingPool& pool) {
        this->get_data_vh_mapping_pool().merge(pool);
    }

    /**
     * @brief Remove a sub adapter
     *
     * @param sub The child adapter to remove
     */
    void remove_sub(std::shared_ptr<PandoraBoxAdapter<T>> sub) {
        this->data_set_->remove_child(sub);
    }

    /**
     * @brief Get child adapter at index
     *
     * @param index The child index
     * @return The child adapter, or nullptr if out of range
     */
    std::shared_ptr<PandoraBoxAdapter<T>> get_child(int index) {
        return this->data_set_->get_child(index);
    }

    /**
     * @brief Clear all child adapters
     */
    void clear_all_children() {
        this->data_set_->clear_all_children();
    }
};

} // namespace rv
} // namespace pandora

#endif // PANDORA_RV_PANDORA_WRAPPER_RV_DATA_SET_H

