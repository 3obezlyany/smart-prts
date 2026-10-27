#pragma once

#include "sw_fwd.h"  // Forward declaration

#include <cstddef>  // std::nullptr_t
#include <memory>
#include <type_traits>
#include <utility>

struct ControlBlock {
    size_t strong_count_ = 1;
    size_t weak_count_ = 1;
    virtual void Destroy() = 0;
    virtual ~ControlBlock() = default;
};

template <typename T, bool is_array>
struct ControlBlockT : ControlBlock {
    T* ptr_;
    ControlBlockT(T* ptr) : ptr_(ptr) {}
    void Destroy() override {
        if constexpr (is_array) {
            delete[] ptr_;
        } else {
            delete ptr_;
        }
    }
};

// https://en.cppreference.com/w/cpp/memory/shared_ptr
template <typename T, bool is_array>
class SharedPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    SharedPtr() = default;
    SharedPtr(std::nullptr_t) noexcept : SharedPtr() {}
    explicit SharedPtr(T* ptr) : ptr_(ptr), control_(new ControlBlockT<T, is_array>(ptr_)) {
    }

    SharedPtr(const SharedPtr& other) : ptr_(other.ptr_), control_(other.control_) {
        if (control_ != nullptr) {
            control_->strong_count_ += 1;
        }
    }
    SharedPtr(SharedPtr&& other) : ptr_(other.ptr_), control_(other.control_) {
        other.ptr_ = nullptr;
        other.control_ = nullptr;
    }

    template <typename U, bool B>
    requires (std::is_convertible_v<U*, T*> && B == is_array)
    SharedPtr(const SharedPtr<U, B>& other) : ptr_(other.ptr_), control_(other.control_) {
        if (control_ != nullptr) {
            ++control_->strong_count_;
        }
    }

    template <typename U, bool B>
    requires (std::is_convertible_v<U*, T*> && B == is_array)
    SharedPtr(SharedPtr<U, B>&& other) noexcept : ptr_(other.ptr_), control_(other.control_) {
        other.ptr_ = nullptr;
        other.control_ = nullptr;
    }

    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename Y, bool B>
    SharedPtr(const SharedPtr<Y, B>& other, T* ptr) : ptr_(ptr), control_(other.control_) {
        if (control_ != nullptr) {
            control_->strong_count_ += 1;
        }
    }

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename U, bool B>
    requires (std::is_convertible_v<U*, T*> && B == is_array)
    explicit SharedPtr(const WeakPtr<U, B>& other) : ptr_(other.ptr_), control_(other.control_) {
        if (control_ == nullptr || control_->strong_count_ == 0) {
            throw BadWeakPtr();
        }
        ++control_->strong_count_;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr& operator=(const SharedPtr& other) throw() {
        SharedPtr(other).Swap(*this);
        return *this;
    }
    SharedPtr& operator=(SharedPtr&& other) noexcept {
        if (this != &other) {
            SharedPtr(std::move(other)).Swap(*this);
        }
        return *this;
    }

    template <typename U, bool B>
    requires (std::is_convertible_v<U*, T*> && B == is_array)
    SharedPtr& operator=(const SharedPtr<U, B>& other) {
        SharedPtr(other).Swap(*this);
        return *this;
    }

    template <typename U, bool B>
    requires (std::is_convertible_v<U*, T*> && B == is_array)
    SharedPtr& operator=(SharedPtr<U, B>&& other) {
        SharedPtr(std::move(other)).Swap(*this);
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~SharedPtr() {
        if (control_ == nullptr) {
            return;
        }
        if (--control_->strong_count_ == 0) {
            control_->Destroy();
            if (--control_->weak_count_ == 0) {
                delete control_;
            }
        }
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        SharedPtr().Swap(*this);
    }
    template <typename U>
    void Reset(U* ptr) {
        SharedPtr(ptr).Swap(*this);
    }
    void Swap(SharedPtr& other) {
        std::swap(ptr_, other.ptr_);
        std::swap(control_, other.control_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return ptr_;
    }
    T& operator*() const {
        return *ptr_;
    }
    T* operator->() const {
        return ptr_;
    }
    size_t UseCount() const {
        if (control_ != nullptr) {
            return control_->strong_count_;
        }
        return 0;
    }
    explicit operator bool() const {
        return ptr_ != nullptr;
    }
private:
    T* ptr_ = nullptr;
    ControlBlock* control_ = nullptr;

    template <typename, bool>
    friend class SharedPtr;

    template <typename, bool>
    friend class WeakPtr;
};

template <typename T, bool B>
SharedPtr(const WeakPtr<T, B>&) -> SharedPtr<T, B>;

template <typename T, typename U>
inline bool operator==(const SharedPtr<T>& left, const SharedPtr<U>& right) {
    return left.Get() == right.Get();
}

// Allocate memory only once
template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    return SharedPtr<T>(new T(std::forward<Args>(args)...));
}

// Look for usage examples in tests
template <typename T>
class EnableSharedFromThis {
public:
    SharedPtr<T> SharedFromThis() {
        return SharedPtr<T>(this_);
    }
    SharedPtr<const T> SharedFromThis() const {
        return SharedPtr<const T>(this_);
    }

    WeakPtr<T> WeakFromThis() noexcept {
        return this_;
    }
    WeakPtr<const T> WeakFromThis() const noexcept {
        return this_;
    }
private:
    WeakPtr<T> this_;

    template <typename, bool>
    friend class SharedPtr;

    template <typename, bool>
    friend class WeakPtr;
};

template <typename T>
class SharedPtr<T[]> : public SharedPtr<T, true> {
    using Base = SharedPtr<T, true>;
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