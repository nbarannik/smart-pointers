#pragma once

#include <type_traits>
#include <utility>
// Me think, why waste time write lot code, when few code do trick.

template <bool is_compressible, typename F>
class Proxy1 : F {
public:
    Proxy1() = default;

    Proxy1(F&&) {
    }

    Proxy1(const F&) {
    }

    F& GetEl() {
        return *this;
    }

    const F& GetEl() const {
        return *this;
    }
};

template <typename F>
class Proxy1<false, F> {
public:
    Proxy1() : value_() {
    }

    Proxy1(const F& f) : value_(f) {
    }

    Proxy1(F&& f) : value_(std::move(f)) {
    }

    F& GetEl() {
        return value_;
    }

    const F& GetEl() const {
        return value_;
    }

private:
    F value_;
};

template <bool is_compressible, typename S>
class Proxy2 : S {
public:
    Proxy2() = default;

    Proxy2(S&&) {
    }

    Proxy2(const S&) {
    }

    S& GetEl() {
        return *this;
    }

    const S& GetEl() const {
        return *this;
    }
};

template <typename S>
class Proxy2<false, S> {
public:
    Proxy2() : value_() {
    }

    Proxy2(const S& s) : value_(s) {
    }

    Proxy2(S&& s) : value_(std::move(s)) {
    }

    S& GetEl() {
        return value_;
    }

    const S& GetEl() const {
        return value_;
    }

private:
    S value_;
};

template <typename F>
using ProxyFirst = Proxy1<!(std::is_fundamental_v<F> || std::is_union_v<F> || std::is_final_v<F> ||
                            !std::is_empty_v<F>),
                          F>;

template <typename S>
using ProxySecond = Proxy2<!(std::is_fundamental_v<S> || std::is_union_v<S> || std::is_final_v<S> ||
                             !std::is_empty_v<S>),
                           S>;

template <typename F, typename S>
class CompressedPair : ProxyFirst<F>, ProxySecond<S> {
public:
    CompressedPair() = default;

    CompressedPair(F&& first, S&& second)
        : ProxyFirst<F>(std::move(first)), ProxySecond<S>(std::move(second)) {
    }

    CompressedPair(const F& first, S&& second)
        : ProxyFirst<F>(first), ProxySecond<S>(std::move(second)) {
    }

    CompressedPair(F&& first, const S& second)
        : ProxyFirst<F>(std::move(first)), ProxySecond<S>(second) {
    }

    CompressedPair(const F& first, const S& second) : ProxyFirst<F>(first), ProxySecond<S>(second) {
    }

    F& GetFirst() {
        return ProxyFirst<F>::GetEl();
    }

    const F& GetFirst() const {
        return ProxyFirst<F>::GetEl();
    }

    S& GetSecond() {
        return ProxySecond<S>::GetEl();
    };

    const S& GetSecond() const {
        return ProxySecond<S>::GetEl();
    };
};