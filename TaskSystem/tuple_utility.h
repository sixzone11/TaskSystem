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