//
// Created by Lenovo on 2025/12/5.
//

#ifndef GLOBAL_H
#define GLOBAL_H

struct TestData
{
    int value;
    explicit TestData(const int v) : value(v) {}

    bool operator==(const TestData& other) const {
        return value == other.value;
    }
};

#endif //GLOBAL_H
