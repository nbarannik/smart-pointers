#pragma once

#include "compressed_pair.h"

#include <cstddef>  // std::nullptr_t
#include <type_traits>

template <typename T>
struct DefaultDeleter {
public:
    DefaultDeleter() = default;

    template <class Derived, std::enable_if_t<std::is_convertible_v<Derived*, T*>, bool> = true>
    DefaultDeleter(const DefaultDeleter<Derived>&) {
    }

    void operator()(T* ptr) {
        delete ptr;
    }
};

template <typename T>
struct DefaultDeleter<T[]> {
public:
    DefaultDeleter() = default;

    template <class Derived, std::enable_if_t<std::is_convertible_v<Derived*, T*>, bool> = true>
    DefaultDeleter(const DefaultDeleter<Derived>&) {
    }

    void operator()(T* ptr) {
        delete[] ptr;
    }
};

// Primary template
template <typename T, typename Deleter = DefaultDeleter<T>>
class UniquePtr : public CompressedPair<T*, Deleter> {
public:
    using Compressed = CompressedPair<T*, Deleter>;
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    explicit UniquePtr(T* ptr = nullptr) : CompressedPair<T*, Deleter>(ptr, Deleter()) {
    }

    UniquePtr(T* ptr, Deleter&& deleter) : CompressedPair<T*, Deleter>(ptr, std::move(deleter)) {
    }

    UniquePtr(T* ptr, const Deleter& deleter) : CompressedPair<T*, Deleter>(ptr, deleter) {
    }

    UniquePtr(UniquePtr&& other) noexcept
        : CompressedPair<T*, Deleter>(other.Get(), std::move(other.GetDeleter())) {
        other.Release();
    }

    template <class Derived, std::enable_if_t<std::is_convertible_v<Derived*, T*>, bool> = true>
    UniquePtr(UniquePtr<Derived>&& other) noexcept
        : CompressedPair<T*, Deleter>(other.Get(), std::move(other.GetDeleter())) {
        other.Release();
    }

    template <class Derived, class DerivedDeleter,
              std::enable_if_t<std::is_convertible_v<Derived*, T*> &&
                                   std::is_convertible_v<DerivedDeleter*, Deleter*>,
                               bool> = true>
    UniquePtr(UniquePtr<Derived, DerivedDeleter>&& other) noexcept
        : CompressedPair<T*, Deleter>(other.Get(), std::move(other.GetDeleter())) {
        other.Release();
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        Reset(other.Get());
        GetDeleter() = std::move(other.GetDeleter());
        other.Release();
        return *this;
    }

    UniquePtr& operator=(std::nullptr_t) {
        Reset();
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        Reset();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() {
        T* return_ptr = Get();
        Compressed::GetFirst() = nullptr;
        Compressed::GetSecond() = Deleter();
        return return_ptr;
    }

    void Reset(T* ptr = nullptr) {
        T* tmp = Get();
        Compressed::GetFirst() = ptr;
        if (tmp != ptr && tmp != nullptr) {
            GetDeleter()(tmp);
        }
    }

    void Swap(UniquePtr& other) {
        std::swap(Compressed::GetFirst(), other.Compressed::GetFirst());
        std::swap(GetDeleter(), other.GetDeleter());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return Compressed::GetFirst();
    }

    Deleter& GetDeleter() {
        return Compressed::GetSecond();
    }

    const Deleter& GetDeleter() const {
        return Compressed::GetSecond();
    }

    explicit operator bool() const {
        return (Get() != nullptr);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    template <typename U = T, std::enable_if_t<!std::is_void_v<U>, bool> = true>
    U& operator*() {
        return *Compressed::GetFirst();
    }

    template <typename U = T, std::enable_if_t<std::is_void_v<U>, bool> = true>
    void operator*() = delete;

    template <typename U = T, std::enable_if_t<!std::is_void_v<U>, bool> = true>
    U* operator->() {
        return Compressed::GetFirst();
    }

    template <typename U = T, std::enable_if_t<!std::is_void_v<U>, bool> = true>
    U* operator->() const {
        return Compressed::GetFirst();
    }

    template <typename U = T, std::enable_if_t<std::is_void_v<U>, bool> = true>
    void operator->() = delete;
};

// Specialization for arrays
template <typename T, typename Deleter>
class UniquePtr<T[], Deleter> : public CompressedPair<T*, Deleter> {
public:
    using Compressed = CompressedPair<T*, Deleter>;
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    explicit UniquePtr(T* ptr = nullptr) : CompressedPair<T*, Deleter>(ptr, Deleter()) {
    }

    UniquePtr(T* ptr, Deleter&& deleter) : CompressedPair<T*, Deleter>(ptr, std::move(deleter)) {
    }

    UniquePtr(T* ptr, const Deleter& deleter) : CompressedPair<T*, Deleter>(ptr, deleter) {
    }

    UniquePtr(UniquePtr&& other) noexcept
        : CompressedPair<T*, Deleter>(other.Get(), std::move(other.GetDeleter())) {
        other.Release();
    }

    template <class Derived, std::enable_if_t<std::is_convertible_v<Derived*, T*>, bool> = true>
    UniquePtr(UniquePtr<Derived>&& other) noexcept
        : CompressedPair<T*, Deleter>(other.Get(), std::move(other.GetDeleter())) {
        other.Release();
    }

    template <class Derived, class DerivedDeleter,
              std::enable_if_t<std::is_convertible_v<Derived*, T*> &&
                                   std::is_convertible_v<DerivedDeleter*, Deleter*>,
                               bool> = true>
    UniquePtr(UniquePtr<Derived, DerivedDeleter>&& other) noexcept
        : CompressedPair<T*, Deleter>(other.Get(), std::move(other.GetDeleter())) {
        other.Release();
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        Reset(other.Get());
        GetDeleter() = std::move(other.GetDeleter());
        other.Release();
        return *this;
    }

    UniquePtr& operator=(std::nullptr_t) {
        Reset();
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        Reset();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() {
        T* return_ptr = Get();
        Compressed::GetFirst() = nullptr;
        Compressed::GetSecond() = Deleter();
        return return_ptr;
    }

    void Reset(T* ptr = nullptr) {
        T* tmp = Get();
        Compressed::GetFirst() = ptr;
        if (tmp != ptr && tmp != nullptr) {
            GetDeleter()(tmp);
        }
    }

    void Swap(UniquePtr& other) {
        std::swap(Compressed::GetFirst(), other.Compressed::GetFirst());
        std::swap(GetDeleter(), other.GetDeleter());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return Compressed::GetFirst();
    }

    Deleter& GetDeleter() {
        return Compressed::GetSecond();
    }

    const Deleter& GetDeleter() const {
        return Compressed::GetSecond();
    }

    explicit operator bool() const {
        return (Get() != nullptr);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    template <typename U = T, std::enable_if_t<!std::is_void_v<U>, bool> = true>
    U& operator*() {
        return *Compressed::GetFirst();
    }

    template <typename U = T, std::enable_if_t<!std::is_void_v<U>, bool> = true>
    const U& operator*() const {
        return *Compressed::GetFirst();
    }

    template <typename U = T, std::enable_if_t<std::is_void_v<U>, bool> = true>
    void operator*() = delete;

    template <typename U = T, std::enable_if_t<!std::is_void_v<U>, bool> = true>
    U* operator->() {
        return Compressed::GetFirst();
    }

    template <typename U = T, std::enable_if_t<!std::is_void_v<U>, bool> = true>
    const U* operator->() const {
        return Compressed::GetFirst();
    }

    template <typename U = T, std::enable_if_t<std::is_void_v<U>, bool> = true>
    void operator->() = delete;

    T& operator[](size_t i) {
        return *(Compressed::GetFirst() + i);
    }

    const T& operator[](size_t i) const {
        return *(Compressed::GetFirst() + i);
    }
};
