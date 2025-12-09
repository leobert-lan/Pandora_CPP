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

#ifndef PANDORA_RV_H
#define PANDORA_RV_H

/**
 * @file pandora_rv.h
 * @brief Main header file for PandoraRV library
 *
 * Include this file to get access to all PandoraRV functionality.
 * PandoraRV provides a C++ implementation of multi-type RecyclerView adapter
 * framework with reactive data binding support.
 */

// Core interfaces
#include "rv/i_view_holder.h"
#include "rv/view_holder_creator.h"
#include "rv/data_observer.h"

// Type system
#include "rv/type_cell.h"
#include "rv/data_vh_mapping_pool.h"

// DataSet implementations
#include "rv/data_set.h"
#include "rv/pandora_data_set.h"
#include "rv/pandora_real_rv_data_set.h"
#include "rv/pandora_wrapper_rv_data_set.h"

// Reactive support
#include "rv/reactive_data.h"
#include "rv/i_reactive_view_holder.h"

/**
 * @namespace pandora::rv
 * @brief RecyclerView adapter framework with multi-type support
 *
 * This namespace contains all classes and interfaces for building
 * RecyclerView-like adapters with multiple item types, reactive data binding,
 * and efficient change notifications.
 *
 * Quick Start Example:
 * @code
 * using namespace pandora::rv;
 *
 * // 1. Define your data type
 * class MyData : public DataSet<MyData>::Data {
 * public:
 *     std::string name;
 *     int age;
 * };
 *
 * // 2. Define your ViewHolder
 * class MyViewHolder : public IViewHolder<MyData> {
 * public:
 *     void set_data(std::shared_ptr<MyData> data) override {
 *         // Update UI with data
 *     }
 *     void on_view_attached_to_window() override {}
 *     void on_view_detached_from_window() override {}
 *     void accept(IViewHolderVisitor& visitor) override {}
 * };
 *
 * // 3. Create DataSet and register mapping
 * auto real_ds = std::make_shared<RealDataSet<MyData>>();
 * auto rv_data_set = std::make_shared<PandoraRealRvDataSet<MyData>>(real_ds);
 *
 * rv_data_set->register_dv_relation<MyData>(
 *     make_lambda_creator<MyData>([](void* parent) {
 *         return std::make_shared<MyViewHolder>(parent);
 *     })
 * );
 *
 * // 4. Add data
 * auto data = std::make_shared<MyData>();
 * data->name = "John";
 * data->age = 25;
 * rv_data_set->add(data);
 *
 * // 5. Use in adapter
 * int view_type = rv_data_set->get_item_view_type_v2(0);
 * auto holder = rv_data_set->create_view_holder_v2(parent, view_type);
 * @endcode
 */

#endif // PANDORA_RV_H

