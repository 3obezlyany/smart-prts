#pragma once

#include "sw_fwd.h"  // Forward declaration
#include "shared.h"

#include <cstddef>  // std::nullptr_t
#include <memory>
#include <type_traits>
#include <utility>

// https://en.cppreference.com/w/cpp/memory/weak_ptr
template <typename T, bool is_array>
class WeakPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    WeakPtr() noexcept = default;

    WeakPtr(const WeakPtr& other) noexcept : ptr_(other.ptr_), control_(other.control_) {
        if (control_ != nullptr) {
            ++control_->weak_count_;
        }
    }
    WeakPtr(WeakPtr&& other) noexcept : ptr_(other.ptr_), control_(other.control_) {
        other.ptr_ = nullptr;
        other.control_ = nullptr;
    }

    // Demote `SharedPtr`
    // #2 from https://en.cppreference.com/w/cpp/memory/weak_ptr/weak_ptr
    template <typename U, bool B>
    requires (std::is_convertible_v<U*, T*> && B == is_array)
    WeakPtr(const SharedPtr<U, B>& other) noexcept : ptr_(other.ptr_), control_(other.control_) {
        if (control_ != nullptr) {
            ++control_->weak_count_;
        }
    }

    template <typename U, bool B>
    requires (std::is_convertible_v<U*, T*> && B == is_array)
    WeakPtr(const WeakPtr<U, B>& other) noexcept : ptr_(other.ptr_), control_(other.control_) {
        if (control_ != nullptr) {
            ++control_->weak_count_;
        }
    }

    template <typename U, bool B>
    requires (std::is_convertible_v<U*, T*> && B == is_array)
    WeakPtr(WeakPtr<U, B>&& other) noexcept : ptr_(other.ptr_), control_(other.control_) {
        other.ptr_ = nullptr;
        other.control_ = nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    WeakPtr& operator=(const WeakPtr& other) throw() {
        WeakPtr(other).Swap(*this);
        return *this;
    }
    WeakPtr& operator=(WeakPtr&& other) noexcept {
        if (this != &other) {
            WeakPtr(std::move(other)).Swap(*this);
        }
        return *this;
    }

    template <typename U, bool B>
    requires (std::is_convertible_v<U*, T*> && B == is_array)
    WeakPtr& operator=(const WeakPtr<U, B>& other) throw() {
        WeakPtr(other).Swap(*this);
        return *this;
    }
    template <typename U, bool B>
    requires (std::is_convertible_v<U*, T*> && B == is_array)
    WeakPtr& operator=(WeakPtr<U, B>&& other) noexcept {
        if (this != &other) {
            WeakPtr(std::move(other)).Swap(*this);
        }
        return *this;
    }

    template <typename U, bool B>
    requires (std::is_convertible_v<U*, T*> && B == is_array)
    WeakPtr& operator=(const SharedPtr<U, B>& other) noexcept {
        WeakPtr(other).Swap(*this);
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~WeakPtr() {
        if (control_ != nullptr && --control_->weak_count_ == 0) {
            delete control_;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        WeakPtr().Swap(*this);
    }
    void Swap(WeakPtr& other) {
        std::swap(ptr_, other.ptr_);
        std::swap(control_, other.control_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    size_t UseCount() const {
        if (control_ == nullptr) return 0;
        return control_->strong_count_;
    }
    bool Expired() const {
        return UseCount() == 0;
    }
    SharedPtr<T, is_array> Lock() const {
        if (Expired()) {
            return SharedPtr<T, is_array>();
        }
        return SharedPtr<T, is_array>(*this);
    }
private:
    T* ptr_ = nullptr;
    ControlBlock* control_ = nullptr;

    template <typename, bool>
    friend class SharedPtr;

    template <typename, bool>
    friend class WeakPtr;
};


template <typename T>
class WeakPtr<T[]> : public WeakPtr<T, true> {
    using Base = WeakPtr<T, true>;
public:
    using Base::Base;
    using Base::operator=;
};