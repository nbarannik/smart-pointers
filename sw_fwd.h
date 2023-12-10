#pragma once

#include <exception>

class ControlBlockBase {
public:
    ControlBlockBase() : shared_count_(1) {
    }
    virtual ~ControlBlockBase() {
    }
    virtual void DeleteData() {
    }

    size_t shared_count_ = 0;
    size_t weak_count_ = 0;
};

template <typename T>
class ControlBlockPointer : public ControlBlockBase {
public:
    ControlBlockPointer(T* ptr) : ptr_(ptr) {
    }
    virtual ~ControlBlockPointer() = default;
    virtual void DeleteData() override {
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

    virtual ~ControlBlockInPlace() = default;

    virtual void DeleteData() override {
        GetPtr()->~T();
    }

    T* GetPtr() {
        return reinterpret_cast<T*>(&buffer_);
    }

private:
    alignas(T) char buffer_[sizeof(T)];
};

class BadWeakPtr : public std::exception {};