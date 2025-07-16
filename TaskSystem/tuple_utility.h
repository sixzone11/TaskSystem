#pragma once

#include <tuple>

///////////////////////////////////////////////////////////////////////
//
// Elementary Types
//
///////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// value_type

template<size_t Value>
struct value_type { constexpr static size_t value = Value; };

template<size_t Value>
constexpr size_t value_type_v = value_type<Value>::value;

using value_type_invalid = value_type<~0ull>;

////////////////////////////////////////////////////////////////////////////////
/// check_type

template<bool AllowBaseType, typename Type1, typename Type2>
struct check_type : std::is_same<Type1, Type2> {};

template<typename Type1, typename Type2>
struct check_type<true, Type1, Type2> : std::is_base_of<Type1, Type2> {};

template<bool AllowBaseType, typename Type1, typename Type2>
constexpr bool check_type_v = check_type<AllowBaseType, Type1, Type2>::value;


///////////////////////////////////////////////////////////////////////
//
// Tuple Utilities
//
///////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// tuple_map

template <typename Tuple, size_t... Seqs>
constexpr auto mapTuple(Tuple&& t, std::index_sequence<Seqs...>) {
	static_assert(((std::tuple_size_v<Tuple> > Seqs) && ...), "Map tuple into Seq is failed since seq in Seqs is not less than size of tuple...");
	return std::make_tuple(std::get<Seqs>(t)...);
}

////////////////////////////////////////////////////////////////////////////////
// find_type_in_tuple

template<bool AllowBaseType, typename FindingType, size_t Index, typename... Types>
struct find_type_in_types;

template<bool AllowBaseType, typename FindingType, size_t Index, typename Type, typename... Types>
struct find_type_in_types<AllowBaseType, FindingType, Index, Type, Types...> : std::conditional_t<check_type_v<AllowBaseType, Type, FindingType>, value_type<Index>, find_type_in_types<AllowBaseType, FindingType, Index + 1, Types...>> {};

template<bool AllowBaseType, typename FindingType, size_t Index, typename Type>
struct find_type_in_types<AllowBaseType, FindingType, Index, Type> : std::conditional_t<check_type_v<AllowBaseType, Type, FindingType>, value_type<Index>, value_type_invalid> {};

template<bool AllowBaseType, typename FindingType, size_t Index>
struct find_type_in_types<AllowBaseType, FindingType, Index> : value_type_invalid {};

template<bool AllowBaseType, typename FindingType, typename TypeListTuple>
struct find_type_in_tuple;

template<bool AllowBaseType, typename FindingType, template<typename... TypeList> typename ListingType, typename... Types>
struct find_type_in_tuple<AllowBaseType, FindingType, ListingType<Types...>>
{
	constexpr static size_t value = find_type_in_types<AllowBaseType, FindingType, 0, Types...>::value;
};

////////////////////////////////////////////////////////////////////////////////
// find_types_in_tuple

template<bool AllowBaseType, typename TypeListTuple, typename FindingTypeTuple>
struct FindType;

template<bool AllowBaseType, typename TypeListTuple, template<typename... TypeList> typename ListingType, typename... FindingTypes>
struct FindType<AllowBaseType, TypeListTuple, ListingType<FindingTypes...>>
{
	using FoundIndexSeq = std::index_sequence< find_type_in_tuple<AllowBaseType, FindingTypes, TypeListTuple>::value ... >;
};

////////////////////////////////////////////////////////////////////////////////
// tuple_cat2

template<typename T1, typename T2>
struct tuple_cat2;

template<typename... T1Types, typename... T2Types>
struct tuple_cat2<std::tuple<T1Types...>, std::tuple<T2Types...>>
{
	using type = std::tuple<T1Types..., T2Types...>;
};

template<typename T1, typename T2>
using tuple_cat2_t = tuple_cat2<T1, T2>;

///////////////////////////////////////////////////////////
// tuple_distinct

template<typename TupleToDistinct, typename = std::make_index_sequence<std::tuple_size_v<TupleToDistinct>>>
struct tuple_distinct;

template<>
struct tuple_distinct<std::tuple<>, std::index_sequence<>>
{
	using type = std::tuple<>;
};

template<typename TupleToDistinct, size_t... Iseq>
struct tuple_distinct<TupleToDistinct, std::index_sequence<Iseq...>>
{
private:
	template<size_t I>
	struct find_distinct
	{
		using prev_tuple = typename find_distinct<I - 1>::tuple;
		using current_as_tuple = std::conditional_t<find_type_in_tuple<false, std::tuple_element_t<I, TupleToDistinct>, prev_tuple>::value == ~0ull,
			std::tuple<std::tuple_element_t<I, TupleToDistinct>>, std::tuple<>>;
		using tuple = typename tuple_cat2<prev_tuple, current_as_tuple>::type;
	};

	template<>
	struct find_distinct<0>
	{
		using tuple = std::tuple<std::tuple_element_t<0, TupleToDistinct>>;
	};

public:
	using type = typename find_distinct<sizeof...(Iseq) - 1>::tuple;
};

template<typename TupleToDistinct>
using tuple_distinct_t = typename tuple_distinct<TupleToDistinct>::type;


////////////////////////////////////////////////////////////////////////////////
// printTupleElement

template<typename T>
void printTupleElement(const T& t);

template<class Tuple, std::size_t N>
struct TuplePrinter
{
	static void print(const Tuple& t)
	{
		TuplePrinter<Tuple, N - 1>::print(t);
		printTupleElement(std::get<N - 1>(t));
	}
};

template<class Tuple>
struct TuplePrinter<Tuple, 1>
{
	static void print(const Tuple& t)
	{
		printTupleElement(std::get<0>(t));
	}
};

template<typename... Args, std::enable_if_t<sizeof...(Args) == 0, int> = 0>
void printTupleElement(const std::tuple<Args...>& t)
{
	std::cout << "()\n";
}

template<typename... Args, std::enable_if_t<sizeof...(Args) != 0, int> = 0>
void printTupleElement(const std::tuple<Args...>& t)
{
	std::cout << "(";
	TuplePrinter<decltype(t), sizeof...(Args)>::print(t);
	std::cout << ")\n";
}


///////////////////////////////////////////////////////////////////////
//
// Utilities
//
///////////////////////////////////////////////////////////////////////

template<size_t... Iseq1, size_t... Iseq2>
static constexpr auto index_sequence_cat(std::index_sequence<Iseq1...>, std::index_sequence<Iseq2...>) { return std::index_sequence<Iseq1..., Iseq2...>{}; }

template<typename T, uint32_t N>
struct AddByN
{
private:
	constexpr static const uint32_t Num = std::tuple_size<decltype(T::_var)>::value;

	template<typename T_, uint32_t N_, uint32_t RI>
	struct __AddByN
	{
		constexpr static const uint32_t I = std::tuple_size<decltype(T::_var)>::value - RI;
		constexpr static const auto _var = std::tuple_cat(std::make_tuple(std::get<I>(T::_var) + N), __AddByN<T_, N_, RI - 1>::_var);
	};

	template<typename T_, uint32_t N_>
	struct __AddByN<T_, N_, 0>
	{
		constexpr static const auto _var = std::make_tuple();
	};

public:
	constexpr static const auto _var = __AddByN<T, N, Num>::_var;
};

template<typename T1, typename T2>
struct AddTwo
{
private:
	constexpr static const uint32_t Num1 = std::tuple_size<decltype(T1::_var)>::value;
	constexpr static const uint32_t Num2 = std::tuple_size<decltype(T2::_var)>::value;
	static_assert(Num1 == Num2, "Size must be same");

	template<typename T1_, typename T2_, uint32_t RI>
	struct __AddTwo
	{
		constexpr static const uint32_t I = std::tuple_size<decltype(T1::_var)>::value - RI;
		constexpr static const auto _var = std::tuple_cat(std::make_tuple(std::get<I>(T1::_var) + std::get<I>(T2::_var)), __AddTwo<T1_, T2_, RI - 1>::_var);
	};

	template<typename T1_, typename T2_>
	struct __AddTwo<T1_, T2_, 0>
	{
		constexpr static const auto _var = std::make_tuple();
	};

public:
	constexpr static const auto _var = __AddTwo<T1, T2, Num1>::_var;
};

template<typename T, uint32_t N>
struct MultiplyByN
{
private:
	constexpr static const uint32_t Num = std::tuple_size<decltype(T::_var)>::value;

	template<typename T_, uint32_t N_, uint32_t RI>
	struct __MultiplyByN
	{
		constexpr static const uint32_t I = std::tuple_size<decltype(T::_var)>::value - RI;
		constexpr static const auto _var = std::tuple_cat(std::make_tuple(std::get<I>(T::_var) * N), __MultiplyByN<T_, N_, RI - 1>::_var);
	};

	template<typename T_, uint32_t N_>
	struct __MultiplyByN<T_, N_, 0>
	{
		constexpr static const auto _var = std::make_tuple();
	};

public:
	constexpr static const auto _var = __MultiplyByN<T, N, Num>::_var;
};

template<typename T1, typename T2>
struct MultiplyTwo
{
private:
	constexpr static const uint32_t Num1 = std::tuple_size<decltype(T1::_var)>::value;
	constexpr static const uint32_t Num2 = std::tuple_size<decltype(T2::_var)>::value;
	static_assert(Num1 == Num2, "Size must be same");

	template<typename T1_, typename T2_, uint32_t RI>
	struct __MultiplyTwo
	{
		constexpr static const uint32_t I = std::tuple_size<decltype(T1::_var)>::value - RI;
		constexpr static const auto _var = std::tuple_cat(std::make_tuple(std::get<I>(T1::_var) * std::get<I>(T2::_var)), __MultiplyTwo<T1_, T2_, RI - 1>::_var);
	};

	template<typename T1_, typename T2_>
	struct __MultiplyTwo<T1_, T2_, 0>
	{
		constexpr static const auto _var = std::make_tuple();
	};

public:
	constexpr static const auto _var = __MultiplyTwo<T1, T2, Num1>::_var;
};

template<uint32_t N, typename T>
struct MakeOffsetBias
{
private:
	constexpr static const uint32_t MAX = N - 1;

	template<typename T_, uint32_t I, uint32_t J>
	struct __MakeOffsetBias
	{
		constexpr static const auto _var = std::tuple_cat(std::make_tuple(J), __MakeOffsetBias<T, (I + 1), ((I < std::get<J>(T::_var)) ? J : (J + 1))>::_var);
	};

	template<typename T_, uint32_t J>
	struct __MakeOffsetBias<T_, MAX, J>
	{
		constexpr static const auto _var = std::make_tuple(J);
	};

public:
	constexpr static const auto _var = __MakeOffsetBias<T, 0, 0>::_var;
};