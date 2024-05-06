#pragma once
#include <tuple>


struct any_argument
{
	template <typename T> operator T && () const;
	template <typename T> operator T& () const;

	any_argument& operator ++();
	any_argument& operator ++(int);
	any_argument& operator --();
	any_argument& operator --(int);

	template <typename T> friend any_argument operator + (const any_argument&, const T&);
	template <typename T> friend any_argument operator + (const T&, const any_argument&);
	template <typename T> friend any_argument operator - (const any_argument&, const T&);
	template <typename T> friend any_argument operator - (const T&, const any_argument&);
	template <typename T> friend any_argument operator * (const any_argument&, const T&);
	template <typename T> friend any_argument operator * (const T&, const any_argument&);
	template <typename T> friend any_argument operator / (const any_argument&, const T&);
	template <typename T> friend any_argument operator / (const T&, const any_argument&);
};

template <typename Lambda, typename Is, typename = void>
struct can_accept_impl : std::false_type
{};

template <typename Lambda, std::size_t ...Is>
struct can_accept_impl < Lambda, std::index_sequence<Is...>, std::void_t<decltype(std::declval<Lambda>()((Is, any_argument{})...)) >> : std::true_type
{
	using RetType = decltype(std::declval<Lambda>()((Is, any_argument{})...));
};

template <typename Lambda, std::size_t N>
struct can_accept : can_accept_impl<Lambda, std::make_index_sequence<N>>
{};

template <typename Lambda, std::size_t N, size_t Max, typename = void>
struct lambda_details_maximum
{
	static constexpr size_t maximum_argument_count = N - 1;
	static constexpr bool is_variadic = false;
};

template <typename Lambda, std::size_t N, size_t Max>
struct lambda_details_maximum<Lambda, N, Max, std::enable_if_t<can_accept<Lambda, N>::value && (N <= Max)>> : lambda_details_maximum<Lambda, N + 1, Max>
{};

template <typename Lambda, std::size_t N, size_t Max>
struct lambda_details_maximum<Lambda, N, Max, std::enable_if_t<can_accept<Lambda, N>::value && (N > Max)>>
{
	static constexpr bool is_variadic = true;
};

template <typename Lambda, std::size_t N, size_t Max, typename = void>
struct lambda_details_minimum : lambda_details_minimum<Lambda, N + 1, Max>
{
	static_assert(N <= Max, "Argument limit reached");
};

template <typename Lambda, std::size_t N, size_t Max>
struct lambda_details_minimum<Lambda, N, Max, std::enable_if_t<can_accept<Lambda, N>::value>> : lambda_details_maximum<Lambda, N, Max>
{
	static constexpr size_t minimum_argument_count = N;
	using RetType = typename can_accept<Lambda, N>::RetType;
};

template <typename Lambda, size_t Max = 50>
struct lambda_details : lambda_details_minimum<Lambda, 0, Max>
{};