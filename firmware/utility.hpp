#pragma once
#include <stdint.h>
#include <stddef.h>

template<size_t...Is>
struct IndexSeq {};

template<size_t I, size_t...Is>
struct impl_seq : impl_seq<I - 1, I - 1, Is...> {};

template<size_t...Is>
struct impl_seq<0, Is...> {
    using type = IndexSeq<Is...>;
};

template<size_t I>
constexpr auto MakeSeq() -> typename impl_seq<I>::type {
    return {};
}
