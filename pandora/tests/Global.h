//
// Created by Lenovo on 2025/12/5.
//

#ifndef GLOBAL_H
#define GLOBAL_H

#include <string>
#include "pandora/pandora_traits.h"

struct TestData
{
    int value;
    std::string name;

    explicit TestData(const int v) : value(v), name("") {}
    TestData(const int v, const std::string& n) : value(v), name(n) {}

    bool operator==(const TestData& other) const {
        return value == other.value && name == other.name;
    }

    // Implement Hash() for content change detection
    size_t Hash() const {
        size_t seed = 0;
        pandora::HashCombine(seed, value);
        pandora::HashCombine(seed, name);
        return seed;
    }
};

#endif //GLOBAL_H
