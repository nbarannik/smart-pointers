#pragma once

#include "sw_fwd.h"  // Forward declaration
#include "shared.h"

template <typename T>
class SharedPtr;

// https://en.cppreference.com/w/cpp/memory/weak_ptr
template <typename T>
class WeakPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    WeakPtr() {
        ptr_ = nullptr;
        control_block_ = nullptr;
    }

    WeakPtr(const WeakPtr& other) {
        ptr_ = other.ptr_;
        control_block_ = other.control_block_;
        IncrementWeakCount();
    }

    WeakPtr(WeakPtr&& other) {
        ptr_ = other.ptr_;
        control_block_ = other.control_block_;
        IncrementWeakCount();
        other.Reset();
    }

    // Demote `SharedPtr`
    // #2 from https://en.cppreference.com/w/cpp/memory/weak_ptr/weak_ptr
    WeakPtr(const SharedPtr<T>& other) {
        ptr_ = other.ptr_;
        control_block_ = other.control_block_;
        IncrementWeakCount();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    WeakPtr& operator=(const WeakPtr& other) {
        Reset();
        ptr_ = other.ptr_;
        control_block_ = other.control_block_;
        IncrementWeakCount();
        return *this;
    }

    WeakPtr& operator=(WeakPtr&& other) {
        Reset();
        ptr_ = other.ptr_;
        control_block_ = other.control_block_;
        IncrementWeakCount();
        other.Reset();
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~WeakPtr() {
        if (control_block_) {
            control_block_->weak_count_--;
            DeleteFromBlock();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (control_block_) {
            control_block_->weak_count_--;
            DeleteFromBlock();
        }

        ptr_ = nullptr;
        control_block_ = nullptr;
    }

    void Swap(WeakPtr& other) {
        std::swap(ptr_, other.ptr_);
        std::swap(control_block_, other.control_block_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    size_t UseCount() const {
        return (control_block_ != nullptr ? control_block_->shared_count_ : 0);
    }

    bool Expired() const {
        if (control_block_ == nullptr) {
            return true;
        }
        return (control_block_->shared_count_ == 0);
    }

    SharedPtr<T> Lock() const {
        if (Expired()) {
            return SharedPtr<T>();
        }
        SharedPtr<T> shared(*this);
        return shared;
    }

    T* Get() const {
        if (Expired()) {
            return nullptr;
        }
        return ptr_;
    }

private:
    inline void IncrementWeakCount() {
        if (control_block_) {
            ++control_block_->weak_count_;
        }
    }

    inline void DeleteFromBlock() {
        if (control_block_->shared_count_ == 0 && control_block_->weak_count_ == 0) {
            delete control_block_;
            control_block_ = nullptr;
        } else if (control_block_->shared_count_ == 0) {
            control_block_->DeleteData();
        }
    }

    T* ptr_;
    ControlBlockBase* control_block_;

    template <typename Tp>
    friend class SharedPtr;

    template <typename Tp>
    friend class EnableSharedFromThis;
};
