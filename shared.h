#pragma once

#include "sw_fwd.h"  // Forward declaration

#include <cstddef>  // std::nullptr_t

class ControlBlockBase {
public:
    ControlBlockBase() : shared_count_(1) {
    }
    virtual ~ControlBlockBase() {
    }
    virtual void DeleteData() {
    }

    size_t shared_count_ = 0;
};

template <typename T>
class ControlBlockPointer : public ControlBlockBase {
public:
    ControlBlockPointer(T* ptr) : ptr_(ptr) {
    }
    virtual ~ControlBlockPointer() {
        delete ptr_;
    }

private:
    T* ptr_;
};

template <typename T>
class ControlBlockInPlace : public ControlBlockBase {
public:
    template <typename... Args>
    ControlBlockInPlace(Args&&... args) {
        new (&buffer_) T(std::forward<Args>(args)...);
    }

    virtual ~ControlBlockInPlace() {
        GetPtr()->~T();
    }

    T* GetPtr() {
        return reinterpret_cast<T*>(&buffer_);
    }

private:
    alignas(T) char buffer_[sizeof(T)];
};

// https://en.cppreference.com/w/cpp/memory/shared_ptr
template <typename T>
class SharedPtr {
public:
    typedef T Type;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    SharedPtr() {
        ptr_ = nullptr;
        control_block_ = nullptr;
    }

    SharedPtr(std::nullptr_t) : SharedPtr() {
    }

    explicit SharedPtr(T* ptr) : ptr_(ptr), control_block_(new ControlBlockPointer<T>(ptr)) {
    }

    template <typename Y, std::enable_if_t<std::is_convertible_v<Y*, T*>, bool> = true>
    explicit SharedPtr(Y* ptr) : ptr_(ptr), control_block_(new ControlBlockPointer<Y>(ptr)) {
    }

    SharedPtr(const SharedPtr& other) {
        ptr_ = other.ptr_;
        control_block_ = other.control_block_;
        IncrementSharedCount();
    }

    template <typename Y,
              std::enable_if_t<std::is_convertible_v<typename Y::Type*, T*>, bool> = true>
    SharedPtr(const Y& other) {
        ptr_ = other.ptr_;
        control_block_ = other.control_block_;
        IncrementSharedCount();
    }

    SharedPtr(SharedPtr&& other) {
        ptr_ = other.ptr_;
        control_block_ = other.control_block_;
        IncrementSharedCount();
        other.Reset();
    }

    template <typename Y,
              std::enable_if_t<std::is_convertible_v<typename Y::Type*, T*>, bool> = true>
    SharedPtr(Y&& other) {
        ptr_ = other.ptr_;
        control_block_ = other.control_block_;
        IncrementSharedCount();
        other.Reset();
    }

    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other, T* ptr) {
        control_block_ = other.control_block_;
        IncrementSharedCount();
        ptr_ = ptr;
    }

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<T>& other);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr& operator=(const SharedPtr& other) {
        if (this == &other) {
            return *this;
        }

        Reset();
        ptr_ = other.ptr_;
        control_block_ = other.control_block_;
        IncrementSharedCount();
        return *this;
    }

    SharedPtr& operator=(SharedPtr&& other) {
        if (this == &other) {
            return *this;
        }

        Reset();
        ptr_ = other.ptr_;
        control_block_ = other.control_block_;
        other.control_block_ = nullptr;
        other.ptr_ = nullptr;
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~SharedPtr() {
        if (control_block_) {
            control_block_->shared_count_--;
            if (control_block_->shared_count_ == 0) {
                delete control_block_;
                control_block_ = nullptr;
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (control_block_) {
            control_block_->shared_count_--;
            if (control_block_->shared_count_ == 0) {
                delete control_block_;
                control_block_ = nullptr;
            }
        }
        control_block_ = nullptr;
        ptr_ = nullptr;
    }

    template <typename Y, std::enable_if_t<std::is_convertible_v<Y*, T*>, bool> = true>
    void Reset(Y* ptr) {
        Reset();
        control_block_ = new ControlBlockPointer<Y>(ptr);
        ptr_ = ptr;
    }

    void Swap(SharedPtr& other) {
        std::swap(ptr_, other.ptr_);
        std::swap(control_block_, other.control_block_);
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
        if (control_block_) {
            return control_block_->shared_count_;
        }
        return 0;
    }

    explicit operator bool() const {
        return (ptr_ != nullptr);
    }

private:
    void IncrementSharedCount() {
        if (control_block_) {
            ++control_block_->shared_count_;
        }
    }

    T* ptr_;

    template <typename Tp, typename... Args>
    friend SharedPtr<Tp> MakeShared(Args&&... args);

    template <typename Tp>
    friend class SharedPtr;

    ControlBlockBase* control_block_;
};

template <typename T, typename U>
inline bool operator==(const SharedPtr<T>& left, const SharedPtr<U>& right);

// Allocate memory only once
template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    ControlBlockInPlace<T>* ptr = new ControlBlockInPlace<T>(std::forward<Args>(args)...);
    SharedPtr<T> shared;
    shared.ptr_ = ptr->GetPtr();
    shared.control_block_ = ptr;
    return shared;
}

// Look for usage examples in tests
template <typename T>
class EnableSharedFromThis {
public:
    SharedPtr<T> SharedFromThis();
    SharedPtr<const T> SharedFromThis() const;

    WeakPtr<T> WeakFromThis() noexcept;
    WeakPtr<const T> WeakFromThis() const noexcept;
};
