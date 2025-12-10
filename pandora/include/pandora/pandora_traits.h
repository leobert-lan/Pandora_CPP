#ifndef PANDORA_TRAITS_H_
#define PANDORA_TRAITS_H_

#include <cstddef>
#include <type_traits>
#include <functional>

namespace pandora {

/**
 * Hash combiner utility function
 * Used to combine multiple hash values into one
 */
template <typename T>
inline void HashCombine(size_t& seed, const T& val) {
    seed ^= std::hash<T>{}(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

/**
 * Type trait to check if a type has a Hash() member function
 */
template <typename T, typename = void>
struct HasHashMethod : std::false_type {};

template <typename T>
struct HasHashMethod<T, std::void_t<decltype(std::declval<T>().Hash())>> : std::true_type {};

/**
 * Type trait to check if a type has operator==
 */
template <typename T, typename = void>
struct HasEqualOperator : std::false_type {};

template <typename T>
struct HasEqualOperator<T, std::void_t<decltype(std::declval<T>() == std::declval<T>())>> : std::true_type {};

/**
 * Content hasher for Pandora types
 * Users can specialize this template for custom types
 *
 * Example specialization:
 *
 * template<>
 * struct ContentHasher<MyType> {
 *     size_t operator()(const MyType& obj) const {
 *         size_t seed = 0;
 *         HashCombine(seed, obj.field1);
 *         HashCombine(seed, obj.field2);
 *         return seed;
 *     }
 * };
 */
template <typename T, typename Enable = void>
struct ContentHasher {
    // Default implementation - compile error with helpful message
    static_assert(sizeof(T) == 0,
        "ContentHasher not specialized for this type. "
        "Please provide a specialization of pandora::ContentHasher<T> "
        "or implement a Hash() member function in your type.");

    size_t operator()(const T& obj) const {
        return 0; // Never reached
    }
};

/**
 * Specialization for types that have a Hash() member function
 */
template <typename T>
struct ContentHasher<T, std::enable_if_t<HasHashMethod<T>::value>> {
    size_t operator()(const T& obj) const {
        return obj.Hash();
    }
};

/**
 * Specialization for pointer types - hash based on pointer address
 */
template <typename T>
struct ContentHasher<T*> {
    size_t operator()(T* ptr) const {
        return std::hash<T*>{}(ptr);
    }
};

/**
 * Specialization for fundamental types (int, float, etc.)
 */
template <typename T>
struct ContentHasher<T, std::enable_if_t<std::is_fundamental_v<T>>> {
    size_t operator()(const T& obj) const {
        return std::hash<T>{}(obj);
    }
};

/**
 * Equality comparator for Pandora types
 * Users can specialize this template for custom types
 */
template <typename T, typename Enable = void>
struct ContentEquals {
    // Default implementation using operator==
    static_assert(HasEqualOperator<T>::value,
        "Type must have operator== or provide a specialization of pandora::ContentEquals<T>");

    bool operator()(const T& lhs, const T& rhs) const {
        return lhs == rhs;
    }
};

/**
 * Specialization for pointer types
 */
template <typename T>
struct ContentEquals<T*> {
    bool operator()(T* lhs, T* rhs) const {
        if (lhs == rhs) return true;
        if (lhs == nullptr || rhs == nullptr) return false;
        return ContentEquals<T>{}(*lhs, *rhs);
    }
};

/**
 * Pandora utility functions
 */
namespace Pandora {

/**
 * Calculate hash for any type that has ContentHasher specialization
 */
template <typename T>
size_t Hash(const T& obj) {
    return ContentHasher<T>{}(obj);
}

/**
 * Check equality for any type
 */
template <typename T>
bool Equals(const T& lhs, const T& rhs) {
    return ContentEquals<T>{}(lhs, rhs);
}

/**
 * Check equality for pointers (handles null pointers)
 */
template <typename T>
bool Equals(T* lhs, T* rhs) {
    if (lhs == rhs) return true;
    if (lhs == nullptr || rhs == nullptr) return false;
    return ContentEquals<T>{}(*lhs, *rhs);
}

} // namespace Pandora

} // namespace pandora

#endif // PANDORA_TRAITS_H_

