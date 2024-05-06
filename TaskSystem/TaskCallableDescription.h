#pragma once

#include <tuple>
#include "lambda_details.h"

using namespace std;

///////////////////////////////////////////////////////////////////////
// BindingSlot

struct BindingSlot
{
	template<typename T> operator T && () const;
	template<typename T> operator T& () const;
};

template<typename T>	struct is_binding_slot : false_type {};
template<>				struct is_binding_slot<BindingSlot> : true_type {};


template<typename T>
constexpr bool is_binding_slot_v = is_binding_slot<T>::value;

///////////////////////////////////////////////////////////////////////
// BindingKey

struct BindingKey {};
struct BindingKey_None : BindingKey {};

#define BindingKeys(Key, ...) tuple<Key, __VA_ARGS__>{}

///////////////////////////////////////////////////////////////////////
// pseudo_void

struct pseudo_void {};



///////////////////////////////////////////////////////////////////////
//
// Tuple Utilities
//
///////////////////////////////////////////////////////////////////////

template <typename Tuple, size_t... Seqs>
auto mapTuple(Tuple&& t, index_sequence<Seqs...>) {
	static_assert(((tuple_size_v<Tuple> > Seqs) && ...), "Map tuple into Seq is failed since seq in Seqs is not less than size of tuple...");
	return std::make_tuple(std::get<Seqs>(t)...);
}

template<size_t Value>
struct value_type { constexpr static size_t value = Value; };

template<size_t Value>
constexpr size_t value_type_v = value_type<Value>::value;

using value_type_invalid = value_type<~0ull>;

template<typename FindingType, size_t Index, typename... Types>
struct find_type_in_types;

template<typename FindingType, size_t Index, typename Type, typename... Types>
struct find_type_in_types<FindingType, Index, Type, Types...> : conditional_t<is_same_v<FindingType, Type>, value_type<Index>, find_type_in_types<FindingType, Index + 1, Types...>> {};

template<typename FindingType, size_t Index, typename Type>
struct find_type_in_types<FindingType, Index, Type> : conditional_t<is_same_v<FindingType, Type>, value_type<Index>, value_type_invalid> {};

template<typename FindingType, size_t Index>
struct find_type_in_types<FindingType, Index> : value_type_invalid {};

template<typename FindingType, typename TypeListTuple>
struct find_type_in_tuple;

template<typename FindingType, typename... Types>
struct find_type_in_tuple<FindingType, tuple<Types...>>
{
	constexpr static size_t value = find_type_in_types<FindingType, 0, Types...>::value;
};

template<typename TypeListTuple, typename FindingTypeTuple>
struct FindType;

template<typename TypeListTuple, typename... FindingTypes>
struct FindType<TypeListTuple, tuple<FindingTypes...>>
{
	using FoundIndexTuple = index_sequence< find_type_in_tuple<FindingTypes, TypeListTuple>::value ... >;
};

///////////////////////////////////////////////////////////////////////
//
// CallableSignature Family
//
///////////////////////////////////////////////////////////////////////

template<typename Callable, typename Ret, typename...Args>
struct CallableSignature;

///////////////////////////////////////////////////////////////////////
// CallableInternalTypes

template<typename Callable>
struct CallableInternalTypes;

template<typename Ret, typename... Params>
struct CallableInternalTypes<Ret(*)(Params...)>
{
	using RetType = conditional_t<is_void_v<Ret>, pseudo_void, Ret>;
	using ParamTypeTuple = tuple<Params... >;

	using OriginalSignature = CallableSignature<Ret(*)(Params...), Ret, Params...>;
};

///////////////////////////////////////////////////////////////////////
// SelectBindingSlots

template<size_t Index, typename SlotIdxSeq, typename Tuple>
struct select_binding_slots;

#define CurrentIndexSequence conditional_t<is_binding_slot_v<remove_reference_t<Arg>>, index_sequence<Indices..., Index>, index_sequence<Indices...>>

template<size_t Index, size_t... Indices, typename Arg, typename... Args>
struct select_binding_slots<Index, index_sequence<Indices...>, tuple<Arg, Args...>> : select_binding_slots<Index + 1, CurrentIndexSequence, tuple<Args...>> {};

template<size_t Index, size_t... Indices, typename Arg>
struct select_binding_slots<Index, index_sequence<Indices...>, tuple<Arg>>
{
	using ReturnIndexSequence = CurrentIndexSequence;
};

template<size_t Index, size_t... Indices>
struct select_binding_slots<Index, index_sequence<Indices...>, tuple<>>
{
	using ReturnIndexSequence = index_sequence<Indices...>;
};

template<typename Tuple>
struct SelectBindingSlots : select_binding_slots<0, index_sequence<>, Tuple> {};

///////////////////////////////////////////////////////////////////////
// CallableSignature

template<typename Callable, typename Ret, typename... Args>
struct CallableSignature
{
	using RetType = typename CallableInternalTypes<Callable>::RetType;
	using ParamTypeTuple = typename CallableInternalTypes<Callable>::ParamTypeTuple;
	using ArgTypeTuple = tuple<Args...>;

	using OriginalSignature = typename CallableInternalTypes<Callable>::OriginalSignature;
	auto getOriginalSignature() { return OriginalSignature{}; }

	using BindingSlotIndexSequence = typename SelectBindingSlots<ArgTypeTuple>::ReturnIndexSequence;
};

///////////////////////////////////////////////////////////////////////
// CallableSignatureWithKey

template<typename Key, typename Callable, typename Ret, typename... Args>
struct CallableSignatureWithKey : CallableSignature<Callable, Ret, Args...>
{
	static_assert(is_base_of_v<BindingKey, Key>, "Given Key is not a KeyType");
	using KeyType = Key;
};

///////////////////////////////////////////////////////////////////////
// makeCallableSignature utility

template<typename Func>
auto makeCallableSignature(Func&& func);

template<typename Ret, typename... Params>
auto makeCallableSignature(Ret(*f)(Params...))
{
	return CallableSignatureWithKey<BindingKey_None, remove_reference_t<decltype(f)>, Ret, Params...> {};
}

template<typename Ret, typename... Params, typename... Args>
auto makeCallableSignature(Ret(*f)(Params...), Args...)
{
	return CallableSignatureWithKey<BindingKey_None, remove_reference_t<decltype(f)>, Ret, Args...> {};
}

template<typename Key, typename Ret, typename... Params, typename = enable_if_t<is_base_of_v<BindingKey, Key>>>
auto makeCallableSignature(Ret(*f)(Params...))
{
	return CallableSignatureWithKey<Key, remove_reference_t<decltype(f)>, Ret, Params...> {};
}

template<typename Key, typename Ret, typename... Params, typename... Args, typename = enable_if_t<is_base_of_v<BindingKey, Key>>>
auto makeCallableSignature(Ret(*f)(Params...), Args...)
{
	return CallableSignatureWithKey<Key, remove_reference_t<decltype(f)>, Ret, Args...> {};
}

template<typename OriginalTypeTupleT, typename ReturnTypeTupleT>
struct BindingSlotTypeChecker;

template<typename... ArgsOriginal, typename... ArgsGiven>
struct BindingSlotTypeChecker<tuple<ArgsOriginal...>, tuple<ArgsGiven...>>
{
	constexpr static bool check()
	{
		static_assert(sizeof...(ArgsOriginal) == sizeof...(ArgsGiven), "Argument tuple size is wrong.");
		static_assert(sizeof...(ArgsOriginal) == 0 || (is_same_v<ArgsOriginal, ArgsGiven> && ...), "Argument type is mismatched");
		return true;
	}
};


///////////////////////////////////////////////////////////////////////
//
// CallableInfo Family
//
///////////////////////////////////////////////////////////////////////

template<typename ReturnTypeTuple, typename KeyTypeTuple, typename... CallableSignatureTs>
struct CallableInfo;

#define CurrentReturnTypeTuple decltype(tuple_cat(ReturnTypeTupleT {}, tuple<typename CallableSignatureT::RetType>{}))
#define CurrentKeyTypeTuple decltype(tuple_cat(KeyTypeTupleT {}, tuple<typename CallableSignatureT::KeyType>{}))

///////////////////////////////////////////////////////////////////////
// CallableInfo

template<typename ReturnTypeTupleT, typename KeyTypeTupleT, typename CallableSignatureT>
struct CallableInfo<ReturnTypeTupleT, KeyTypeTupleT, CallableSignatureT>
{
	static_assert(is_same_v<typename CallableSignatureT::KeyType, BindingKey_None> ||
		tuple_size_v<KeyTypeTupleT> == 0 ||
		find_type_in_tuple<typename CallableSignatureT::KeyType, KeyTypeTupleT>::value == ~0ull, "Failed to makeCallableInfo since key is duplicated.");

	using ReturnTypeTuple = CurrentReturnTypeTuple;
	using KeyTypeTuple = CurrentKeyTypeTuple;
};

template<typename ReturnTypeTupleT, typename KeyTypeTupleT, typename CallableSignatureT, size_t... Seqs>
struct CallableInfo<ReturnTypeTupleT, KeyTypeTupleT, CallableSignatureT, index_sequence<Seqs...>>
	: CallableInfo<ReturnTypeTupleT, KeyTypeTupleT, CallableSignatureT>
{
	using OrderedReturnTypeTupleT = decltype(mapTuple(ReturnTypeTupleT{}, index_sequence<Seqs...>{}));
	using OrderedBindingSlotTypeTupleT = decltype(mapTuple(typename CallableSignatureT::ParamTypeTuple{}, typename CallableSignatureT::BindingSlotIndexSequence{}));
	//tuple<tuple_element<Seqs..., ReturnTypeTupleT>>::type>;
	CallableInfo()
	{
		//BindingSlotTypeChecker<0, typename CallableSignatureT::ParamTypeTuple, typename CallableSignatureT::OriginalSignature::ParamTypeTuple, OrderedReturnTypeTupleT>::check();
		BindingSlotTypeChecker<OrderedBindingSlotTypeTupleT, OrderedReturnTypeTupleT>::check();
	}
};

template<typename ReturnTypeTupleT, typename KeyTypeTupleT, typename CallableSignatureT, typename... KeyTs>
struct CallableInfo<ReturnTypeTupleT, KeyTypeTupleT, CallableSignatureT, tuple<KeyTs...>>
	: CallableInfo<ReturnTypeTupleT, KeyTypeTupleT, CallableSignatureT, typename FindType<CurrentKeyTypeTuple, tuple<KeyTs...>>::FoundIndexTuple> {};

template<typename ReturnTypeTupleT, typename KeyTypeTupleT, typename CallableSignatureT, typename... CallableSignatureTs>
struct CallableInfo<ReturnTypeTupleT, KeyTypeTupleT, CallableSignatureT, CallableSignatureTs...>
	: CallableInfo<ReturnTypeTupleT, KeyTypeTupleT, CallableSignatureT>
	, CallableInfo<CurrentReturnTypeTuple, CurrentKeyTypeTuple, CallableSignatureTs...> {};

template<typename ReturnTypeTupleT, typename KeyTypeTupleT, typename CallableSignatureT, size_t... Seqs, typename... CallableSignatureTs>
struct CallableInfo<ReturnTypeTupleT, KeyTypeTupleT, CallableSignatureT, index_sequence<Seqs...>, CallableSignatureTs...>
	: CallableInfo<ReturnTypeTupleT, KeyTypeTupleT, CallableSignatureT, index_sequence<Seqs...>>
	, CallableInfo<CurrentReturnTypeTuple, CurrentKeyTypeTuple, CallableSignatureTs...> {};

template<typename ReturnTypeTupleT, typename KeyTypeTupleT, typename CallableSignatureT, typename... KeyTs, typename... CallableSignatureTs>
struct CallableInfo<ReturnTypeTupleT, KeyTypeTupleT, CallableSignatureT, tuple<KeyTs...>, CallableSignatureTs...>
	: CallableInfo<ReturnTypeTupleT, KeyTypeTupleT, CallableSignatureT, typename FindType<CurrentKeyTypeTuple, tuple<KeyTs...>>::FoundIndexTuple, CallableSignatureTs...> {};

///////////////////////////////////////////////////////////////////////
// makeCallableInfo utility

template<typename... CallableSignatures>
auto makeCallableInfo(CallableSignatures&&... signatures)
{
	return CallableInfo<tuple<>, tuple<>, remove_reference_t<decltype(signatures)>...>{};
}