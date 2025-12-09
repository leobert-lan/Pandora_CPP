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

#ifndef PANDORA_RV_PANDORA_REAL_RV_DATA_SET_H
#define PANDORA_RV_PANDORA_REAL_RV_DATA_SET_H

#include "pandora_data_set.h"
#include "../real_data_set.h"

namespace pandora {
namespace rv {

/**
 * @brief DataSet implementation that wraps a RealDataSet
 *
 * This class combines the RecyclerView DataSet interface with Pandora's
 * RealDataSet, which manages a single linear list of data items.
 *
 * @tparam T The data type (must inherit from DataSet::Data)
 *
 * Example:
 * @code
 * auto real_ds = std::make_shared<RealDataSet<MyData>>();
 * auto rv_data_set = std::make_shared<PandoraRealRvDataSet<MyData>>(real_ds);
 *
 * // Register ViewHolder mapping
 * rv_data_set->register_dv_relation<MyData>(
 *     make_lambda_creator<MyData>([](void* parent) {
 *         return std::make_shared<MyViewHolder>(parent);
 *     })
 * );
 *
 * // Add data
 * rv_data_set->add(std::make_shared<MyData>());
 *
 * // Use with RecyclerView adapter
 * int view_type = rv_data_set->get_item_view_type_v2(0);
 * auto holder = rv_data_set->create_view_holder_v2(parent, view_type);
 * @endcode
 */
template<typename T>
class PandoraRealRvDataSet : public PandoraDataSet<T, RealDataSet<T>> {
public:
    /**
     * @brief Construct with a RealDataSet
     *
     * @param real_data_set The underlying RealDataSet
     */
    explicit PandoraRealRvDataSet(std::shared_ptr<RealDataSet<T>> real_data_set)
        : PandoraDataSet<T, RealDataSet<T>>(std::move(real_data_set)) {}

    /**
     * @brief Get the underlying RealDataSet
     *
     * @deprecated Use get_data_set() instead
     */
    [[deprecated("Use get_data_set() instead")]]
    std::shared_ptr<RealDataSet<T>> get_real_data_set() const {
        return this->get_data_set();
    }

    /**
     * @brief Set the group index
     *
     * @param group_index The group index
     */
    void set_group_index(int group_index) {
        this->data_set_->set_group_index(group_index);
    }
};

} // namespace rv
} // namespace pandora

#endif // PANDORA_RV_PANDORA_REAL_RV_DATA_SET_H

