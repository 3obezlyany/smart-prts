#pragma once

#include <type_traits>
#include <utility>

// Me think, why waste time write lot code, when few code do trick.
template <typename T, bool = std::is_empty_v<T> && !std::is_final_v<T>>
struct FB : private T {
    FB() = default;

    template <typename U>
    explicit FB(U&& value) : T(std::forward<U>(value)) {}

    T& Get() {
        return *this;
    }

    const T& Get() const {
        return *this;
    }
};

template <typename T>
struct FB<T, false> {
    T value;

    FB() = default;

    template <typename U>
    explicit FB(U&& v) : value(std::forward<U>(v)) {}

    T& Get() {
        return value;
    }

    const T& Get() const {
        return value;
    }
};

template <typename T, bool = std::is_empty_v<T> && !std::is_final_v<T>>
struct SB : private T {
    SB() = default;

    template <typename U>
    explicit SB(U&& value) : T(std::forward<U>(value)) {}

    T& Get() {
        return *this;
    }

    const T& Get() const {
        return *this;
    }
};

template <typename T>
struct SB<T, false> {
    T value;

    SB() = default;

    template <typename U>
    explicit SB(U&& v) : value(std::forward<U>(v)) {}

    T& Get() {
        return value;
    }

    const T& Get() const {
        return value;
    }
};

template <typename F, typename S>
class CompressedPair : private FB<F>, private SB<S> {
public:
    CompressedPair() = default;

    template <typename A, typename B>
    CompressedPair(A&& first, B&& second)
        : FB<F>(std::forward<A>(first)),
          SB<S>(std::forward<B>(second)) {}

    F& GetFirst() {
        return FB<F>::Get();
    }

    const F& GetFirst() const {
        return FB<F>::Get();
    }

    S& GetSecond() {
        return SB<S>::Get();
    }

    const S& GetSecond() const {
        return SB<S>::Get();
    }
};