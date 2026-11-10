export module Cast;

export template <class To, class From>
constexpr auto scast(From&& Value) noexcept(noexcept(static_cast<To>(static_cast<From&&>(Value))))
    -> decltype(static_cast<To>(static_cast<From&&>(Value)))
{
    return static_cast<To>(static_cast<From&&>(Value));
}

export template <class To, class From>
constexpr auto rcast(From&& Value) noexcept(noexcept(reinterpret_cast<To>(static_cast<From&&>(Value))))
    -> decltype(reinterpret_cast<To>(static_cast<From&&>(Value)))
{
    return reinterpret_cast<To>(static_cast<From&&>(Value));
}

export template <class To, class From>
constexpr auto ccast(From&& Value) noexcept
    -> decltype(const_cast<To>(static_cast<From&&>(Value)))
{
    return const_cast<To>(static_cast<From&&>(Value));
}
