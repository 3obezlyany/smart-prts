#pragma once

#include "compressed_pair.h"

#include <cstddef>  // std::nullptr_t
#include <utility>
#include <type_traits>

struct Slug {};

// Primary template
template <typename T, typename Deleter = Slug, bool is_array = false>
class UniquePtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    explicit UniquePtr(T* ptr = nullptr) : data_(ptr, Deleter()) {}

    UniquePtr(T* ptr, Deleter deleter) : data_(ptr, std::move(deleter)) {}

    UniquePtr(UniquePtr&& other) noexcept : data_(other.Release(), std::move(other.GetDeleter())) {}

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this != &other) {
            Reset(other.Release());
            data_.GetSecond() = std::move(other.GetDeleter());
        }
        return *this;
    }

    UniquePtr& operator=(std::nullptr_t) throw() {
        Reset();
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() throw() {
        if (data_.GetFirst()) {
            Delete(data_.GetFirst());
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Delete(T* ptr) {
        if (ptr == nullptr) {
            return;
        }
        if constexpr (std::is_same_v<Deleter, Slug>) {
            if constexpr (is_array) {
                delete[] ptr;
            } else {
                delete ptr;
            }
        } else {
            data_.GetSecond()(ptr);
        }
    }

    T* Release() {
        T* ptr = data_.GetFirst();
        data_.GetFirst() = nullptr;
        return ptr;
    }
    void Reset(T* ptr = nullptr) {
        T* old = data_.GetFirst();
        data_.GetFirst() = ptr;
        if (old) {
            Delete(old);
        }
    }
    void Swap(UniquePtr& other) {
        std::swap(data_.GetFirst(), other.data_.GetFirst());
        std::swap(data_.GetSecond(), other.data_.GetSecond());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return data_.GetFirst();
    }
    Deleter& GetDeleter() {
        return data_.GetSecond();
    }
    const Deleter& GetDeleter() const {
        return data_.GetSecond();
    }
    explicit operator bool() const {
        return data_.GetFirst() != nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    T& operator*() const {
        return *data_.GetFirst();
    }
    T* operator->() const {
        return data_.GetFirst();
    }
private:
    CompressedPair<T*, Deleter> data_;
};

// Specialization for arrays
template <typename T, typename Deleter>
class UniquePtr<T[], Deleter> : public UniquePtr<T, Deleter, true> {
    using Base = UniquePtr<T, Deleter, true>;
public:
    using Base::Base;
    using Base::operator=;
    T& operator[](size_t i) const {
        return this->Get()[i];
    }
private:
    using Base::operator*;
    using Base::operator->;
};
