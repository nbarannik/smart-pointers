#pragma once

#include "sw_fwd.h"  // Forward declaration
#include "weak.h"

#include <cstddef>  // std::nullptr_t
// https://en.cppreference.com/w/cpp/memory/shared_ptr

template <typename T>
class WeakPtr;

template <typename T>
class EnableSharedFromThis;

template <class T, class Y = void>
struct EnableIf {
    typedef Y IsEnabled;
};

template <class T, class Enable = void>
struct IsEnabledToShareFromThis : std::false_type {};

template <class T>
struct IsEnabledToShareFromThis<T, typename EnableIf<typename T::EnabledToShareFromThis>::IsEnabled>
    : std::true_type {};

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

    template <typename Y>
    explicit SharedPtr(Y* ptr) : ptr_(ptr), control_block_(new ControlBlockPointer<Y>(ptr)) {
        if constexpr (IsEnabledToShareFromThis<Y>()) {
            ptr->ptr_ = ptr_;
            ptr->control_block_ = control_block_;
        }
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
    explicit SharedPtr(const WeakPtr<T>& other) {
        if (other.Expired()) {
            throw BadWeakPtr();
        }
        ptr_ = other.ptr_;
        control_block_ = other.control_block_;
        IncrementSharedCount();
    }
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
            if (control_block_->shared_count_ == 0 && control_block_->weak_count_ == 0) {
                control_block_->DeleteData();
                delete control_block_;
                control_block_ = nullptr;
            } else if (control_block_->shared_count_ == 0) {
                control_block_->DeleteData();
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (control_block_) {
            control_block_->shared_count_--;
            if (control_block_->shared_count_ == 0 && control_block_->weak_count_ == 0) {
                control_block_->DeleteData();
                delete control_block_;
                control_block_ = nullptr;
            } else if (control_block_->shared_count_ == 0) {
                control_block_->DeleteData();
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
    inline void IncrementSharedCount() {
        if (control_block_) {
            ++control_block_->shared_count_;
        }
    }

    T* ptr_;
    ControlBlockBase* control_block_;

    template <typename Tp, typename... Args>
    friend SharedPtr<Tp> MakeShared(Args&&... args);

    template <typename Tp>
    friend class SharedPtr;

    template <typename Tp>
    friend class WeakPtr;

    template <typename Tp>
    friend class EnableSharedFromThis;

    template <typename Tp, typename Y>
    friend bool operator==(const SharedPtr<Tp>& sp1, const SharedPtr<Y>& sp2);
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

    if constexpr (IsEnabledToShareFromThis<T>()) {
        (*shared.ptr_).ptr_ = shared.ptr_;
        (*shared.ptr_).control_block_ = shared.control_block_;
    }

    return shared;
}

template <typename T, typename Y>
bool operator==(const SharedPtr<T>& sp1, const SharedPtr<Y>& sp2) {
    return (sp1.ptr_ == sp2.ptr_ && sp1.control_block_ == sp2.control_block_);
}

// Look for usage examples in tests
template <typename T>
class EnableSharedFromThis {
public:
    typedef void EnabledToShareFromThis;

    SharedPtr<T> SharedFromThis() {
        SharedPtr<T> sptr;
        sptr.ptr_ = ptr_;
        sptr.control_block_ = control_block_;
        sptr.IncrementSharedCount();
        return sptr;
    }

    SharedPtr<const T> SharedFromThis() const {
        SharedPtr<const T> sptr;
        sptr.ptr_ = ptr_;
        sptr.control_block_ = control_block_;
        sptr.IncrementSharedCount();
        return sptr;
    }

    WeakPtr<T> WeakFromThis() noexcept {
        WeakPtr<T> wptr;
        wptr.ptr_ = ptr_;
        wptr.control_block_ = control_block_;
        wptr.IncrementWeakCount();
        return wptr;
    }

    WeakPtr<const T> WeakFromThis() const noexcept {
        WeakPtr<const T> wptr;
        wptr.ptr_ = ptr_;
        wptr.control_block_ = control_block_;
        wptr.IncrementWeakCount();
        return wptr;
    }

private:
    ControlBlockBase* control_block_ = nullptr;
    T* ptr_ = nullptr;

    template <typename Tp>
    friend class SharedPtr;

    template <typename Tp, typename... Args>
    friend SharedPtr<Tp> MakeShared(Args&&... args);
};