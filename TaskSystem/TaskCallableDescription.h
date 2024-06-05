#pragma once

#include <tuple>
#include "lambda_details.h"


///////////////////////////////////////////////////////////////////////
// Type Checker
template<typename T> void type_checker() { static_assert(false, ""); }

using namespace std;

///////////////////////////////////////////////////////////////////////
// BindingSlot

struct BindingSlot
{
	template<typename T> operator T && () const;
	template<typename T> operator T& () const;
};

template<typename T>	struct is_binding_slot : conditional_t<is_base_of_v<BindingSlot, T>, true_type, false_type> {};


template<typename T>
constexpr bool is_binding_slot_v = is_binding_slot<T>::value;

///////////////////////////////////////////////////////////////////////
// BindingKey

struct BindingKey : public BindingSlot {};
struct BindingKey_None : BindingKey {};

template<typename... Keys>
struct BindingKeyList {};
#define BindingKeys(Key, ...) BindingKeyList<Key, __VA_ARGS__>{}
//#define BindingKeys(Key, ...) tuple<Key, __VA_ARGS__>{}


///////////////////////////////////////////////////////////////////////
// deferred_substitution

struct deferred_substitution {};

template<typename T>
constexpr bool is_deferred_substitution_v = is_same_v<T, deferred_substitution>;

///////////////////////////////////////////////////////////////////////
// pseudo_void

struct pseudo_void {};

template<typename T>
constexpr bool is_pseudo_void_v = is_same_v<T, pseudo_void>;


///////////////////////////////////////////////////////////////////////
//
// Tuple Utilities
//
///////////////////////////////////////////////////////////////////////

template <typename Tuple, size_t... Seqs>
constexpr auto mapTuple(Tuple&& t, index_sequence<Seqs...>) {
	static_assert(((tuple_size_v<Tuple> > Seqs) && ...), "Map tuple into Seq is failed since seq in Seqs is not less than size of tuple...");
	return std::make_tuple(std::get<Seqs>(t)...);
}

template<size_t Value>
struct value_type { constexpr static size_t value = Value; };

template<size_t Value>
constexpr size_t value_type_v = value_type<Value>::value;

using value_type_invalid = value_type<~0ull>;

template<bool AllowBaseType, typename Type1, typename Type2>
struct check_type : is_same<Type1, Type2> {};

template<typename Type1, typename Type2>
struct check_type<true, Type1, Type2> : is_base_of<Type1, Type2> {};

template<bool AllowBaseType, typename Type1, typename Type2>
constexpr bool check_type_v = check_type<AllowBaseType, Type1, Type2>::value;

template<bool AllowBaseType, typename FindingType, size_t Index, typename... Types>
struct find_type_in_types;

template<bool AllowBaseType, typename FindingType, size_t Index, typename Type, typename... Types>
struct find_type_in_types<AllowBaseType, FindingType, Index, Type, Types...> : conditional_t<check_type_v<AllowBaseType, Type, FindingType>, value_type<Index>, find_type_in_types<AllowBaseType, FindingType, Index + 1, Types...>> {};

template<bool AllowBaseType, typename FindingType, size_t Index, typename Type>
struct find_type_in_types<AllowBaseType, FindingType, Index, Type> : conditional_t<check_type_v<AllowBaseType, Type, FindingType>, value_type<Index>, value_type_invalid> {};

template<bool AllowBaseType, typename FindingType, size_t Index>
struct find_type_in_types<AllowBaseType, FindingType, Index> : value_type_invalid {};

template<bool AllowBaseType, typename FindingType, typename TypeListTuple>
struct find_type_in_tuple;

template<bool AllowBaseType, typename FindingType, template<typename... TypeList> typename ListingType, typename... Types>
struct find_type_in_tuple<AllowBaseType, FindingType, ListingType<Types...>>
{
	constexpr static size_t value = find_type_in_types<AllowBaseType, FindingType, 0, Types...>::value;
};

template<bool AllowBaseType, typename TypeListTuple, typename FindingTypeTuple>
struct FindType;

template<bool AllowBaseType, typename TypeListTuple, template<typename... TypeList> typename ListingType, typename... FindingTypes>
struct FindType<AllowBaseType, TypeListTuple, ListingType<FindingTypes...>>
{
	using FoundIndexSeq = index_sequence< find_type_in_tuple<AllowBaseType, FindingTypes, TypeListTuple>::value ... >;
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

template<typename Callable, typename = void>
struct CallableInternalTypes;

template<typename Ret, typename... Params>
struct CallableInternalTypes<Ret(*)(Params...)>
{
	constexpr static bool is_resolved = true;
	using Callable = Ret(*)(Params...);

	using RetType = conditional_t<is_void_v<Ret>, pseudo_void, Ret>;
	using ParamTypeTuple = tuple<Params... >;

	using OriginalSignature = CallableSignature<Ret(*)(Params...), Ret, Params...>;
};

// Note(jiman): non-static member function의 cv-qualifier 중 const만 대응됨.

template<typename Type, typename Ret, typename... Params>
struct CallableInternalTypes<Ret(Type::*)(Params...) const>
{
	constexpr static bool is_resolved = true;
	using Callable = Ret(Type::*)(Params...) const;

	using RetType = conditional_t<is_void_v<Ret>, pseudo_void, Ret>;
	using ParamTypeTuple = tuple<Params... >;

	using OriginalSignature = CallableSignature<Ret(Type::*)(Params...) const, Ret, Params...>;
};

template<typename Type, typename Ret, typename... Params>
struct CallableInternalTypes<Ret(Type::*)(Params...)>
{
	constexpr static bool is_resolved = true;
	using Callable = Ret(Type::*)(Params...);

	using RetType = conditional_t<is_void_v<Ret>, pseudo_void, Ret>;
	using ParamTypeTuple = tuple<Params... >;

	using OriginalSignature = CallableSignature<Ret(Type::*)(Params...), Ret, Params...>;
};

template<typename _Callable>
struct CallableInternalTypes<_Callable,
	void_t<decltype(&_Callable::operator())>> // Note(jiman): [SFINAE] function-call operator가 decltype으로 확정 가능한 형태만 허용.
{
	constexpr static bool is_resolved = true;
	using Callable = _Callable;

	using RetType = typename CallableInternalTypes<decltype(&_Callable::operator())>::RetType;
	using ParamTypeTuple = typename CallableInternalTypes<decltype(&_Callable::operator())>::ParamTypeTuple;

	using OriginalSignature = typename CallableInternalTypes<decltype(&_Callable::operator())>::OriginalSignature;
};

struct LambdaTaskIdentifier {};

template<typename Type, typename Ret, typename KeyTuple, typename ResultTuple>
struct CallableInternalTypes<Ret(Type::*)(LambdaTaskIdentifier, KeyTuple, ResultTuple&&) const>
{
	constexpr static bool is_resolved = true;

	using RetType = conditional_t<is_void_v<Ret>, pseudo_void, Ret>;
	using ParamTypeTuple = tuple<KeyTuple, ResultTuple&>;

	using OriginalSignature = CallableSignature<Ret(Type::*)(LambdaTaskIdentifier, KeyTuple, ResultTuple&&) const, Ret, LambdaTaskIdentifier, KeyTuple, ResultTuple&&>;
};

template<typename Type, typename KeyTuple, typename ResultTuple, typename Ret>
constexpr auto makeCallableInternalTypeByFunction(Ret(Type::*f)(LambdaTaskIdentifier, KeyTuple, ResultTuple&&) const)
{
	return CallableInternalTypes<Ret(Type::*)(LambdaTaskIdentifier, KeyTuple, ResultTuple&&) const> {};
}

template<typename _Callable, typename>
struct CallableInternalTypes
//	: CallableInternalTypes<Callable, decltype(&Callable::operator())> {};
{
	constexpr static bool is_resolved = false;
	using Callable = _Callable;

	using RetType = deferred_substitution;
	using ParamTypeTuple = deferred_substitution;
	using OriginalSignature = deferred_substitution;

	//using RetType = typename decltype(makeCallableInternalTypeByFunction<
	//	Callable, tuple<KeyA::Second, KeyA::First>, tuple<int, float>,
	//	decltype(declval<Callable>()(LambdaTaskIdentifier(), tuple<KeyA::Second, KeyA::First>{}, tuple<int, float>{})) > (&Callable::operator()))::RetType;
	//using RetType = typename decltype(makeCallableInternalTypeByFunction<Callable, tuple<BindingKey_None, BindingKey_None>, tuple<>>(&(Callable::operator())))::RetType;
	
	//using RetType = typename decltype(makeCallableInternalTypeByFunction(&(Callable::operator())))::RetType;
	//using ParamTypeTuple = typename decltype(makeCallableInternalTypeByFunction(&(Callable::operator())))::ParamTypeTuple;
	//using OriginalSignature = typename decltype(makeCallableInternalTypeByFunction(&(Callable::operator())))::OriginalSignature;
	
	//using RetType = typename CallableInternalTypes<&(Callable::operator())>::RetType;
	//using ParamTypeTuple = typename CallableInternalTypes<&(Callable::operator())>::ParamTypeTuple;
	//using OriginalSignature = typename CallableInternalTypes<&(Callable::operator())>::OriginalSignature;
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
	using BindingSlotIndexSequence = CurrentIndexSequence;
};

template<size_t Index, size_t... Indices>
struct select_binding_slots<Index, index_sequence<Indices...>, tuple<>>
{
	using BindingSlotIndexSequence = index_sequence<Indices...>;
};

template<typename Tuple>
struct SelectBindingSlots : select_binding_slots<0, index_sequence<>, Tuple> {};

///////////////////////////////////////////////////////////////////////
// CallableSignature

template<typename _Callable, typename Ret, typename... Args>
struct CallableSignature
{
	using Callable = _Callable;
	constexpr static bool is_resolved = CallableInternalTypes<Callable>::is_resolved;

	struct KeyType : public BindingKey {};
	using RetType = typename CallableInternalTypes<Callable>::RetType;
	using ParamTypeTuple = typename CallableInternalTypes<Callable>::ParamTypeTuple;
	using ArgTypeTuple = tuple<Args...>;

	using OriginalSignature = typename CallableInternalTypes<Callable>::OriginalSignature;
	auto getOriginalSignature() { return OriginalSignature{}; }

	using BindingSlotIndexSequence = typename SelectBindingSlots<ArgTypeTuple>::BindingSlotIndexSequence;

	Callable _callable;
	ArgTypeTuple _args = tuple<Args...>{ Args{}... };
};

//template<typename Callable, typename = std::enable_if_t<std::is_class_v<std::remove_reference_t<Callable>>>, typename... Args>
//struct CallableSignature<Callable, typename CallableInternalTypes<Callable>::RetType, Args...>

///////////////////////////////////////////////////////////////////////
// CallableSignatureWithKey

template<typename Key, typename Callable, typename Ret, typename... Args>
struct CallableSignatureWithKey : CallableSignature<Callable, Ret, Args...>
{
	static_assert(is_base_of_v<BindingKey, Key>, "Given Key is not a KeyType");
	using KeyType = conditional_t<is_same_v<BindingKey_None, Key>, typename CallableSignature<Callable, Ret, Args...>::KeyType, Key>;
};

///////////////////////////////////////////////////////////////////////
// makeCallableSignature utility

// Guide: 함수 오버로딩에선 원형이랄 것의 선언을 따로 둘 필요 없음.
// 템플릿 특수화와 달리 함수 오버로딩인 관계로 타입 추론의 방법 자체가 다름을 인지해야할 것.

//template<typename Func>
//constexpr auto makeCallableSignature(Func&& func);

//template<typename Func, typename = void>
//constexpr auto makeCallableSignature(Func&& func);

// Functions
template<typename Key = BindingKey_None, typename Ret, typename... Params, typename = enable_if_t<is_base_of_v<BindingKey, Key>>>
constexpr auto makeCallableSignature(Ret(*f)(Params...)) {
	return CallableSignatureWithKey<Key, Ret(*)(Params...), Ret, Params...> {
		CallableSignature<Ret(*)(Params...), Ret, Params...> {
			std::forward< Ret(*)(Params...)>(f),
		}
	};
}

template<typename Key = BindingKey_None, typename Ret, typename... Params, typename... Args, typename = enable_if_t<is_base_of_v<BindingKey, Key>>>
constexpr auto makeCallableSignature(Ret(*f)(Params...), Args&&... args) {
	return CallableSignatureWithKey<Key, Ret(*)(Params...), Ret, Args...> {
		CallableSignature<Ret(*)(Params...), Ret, Args...> {
			std::forward< Ret(*)(Params...)>(f),
			std::tuple<Args...>{ std::forward<Args>(args)... }
		}
	};
}

// Non-static Member Functions (non-const)
template<typename Key = BindingKey_None, typename Type, typename Ret, typename... Params,
	typename = enable_if_t< is_base_of_v<BindingKey, Key> && is_class_v<Type> && (is_base_of_v<BindingKey, Type> == false) >>
constexpr auto makeCallableSignature(Ret(Type::*f)(Params...)) {
	return CallableSignatureWithKey<Key, Ret(Type::*)(Params...), Ret, Params...> {
		CallableSignature<Ret(Type::*)(Params...), Ret, Params...> {
			std::forward< Ret(Type::*)(Params...)>(f),
		}
	};
}

template<typename Key = BindingKey_None, typename Type, typename Ret, typename... Params, typename... Args,
	typename = enable_if_t< is_base_of_v<BindingKey, Key> && is_class_v<Type> && (is_base_of_v<BindingKey, Type> == false) >>
constexpr auto makeCallableSignature(Ret(Type::*f)(Params...), Args&&... args) {
	return CallableSignatureWithKey<Key, Ret(Type::*)(Params...), Ret, Args...> {
		CallableSignature<Ret(Type::*)(Params...), Ret, Args...> {
			std::forward< Ret(Type::*)(Params...)>(f),
			std::tuple<Args...>{ std::forward<Args>(args)... }
		}
	};
}

// Non-static Member Functions (const)

template<typename Key = BindingKey_None, typename Type, typename Ret, typename... Params,
	typename = enable_if_t< is_base_of_v<BindingKey, Key> && is_class_v<Type> && (is_base_of_v<BindingKey, Type> == false)>>
constexpr auto makeCallableSignature(Ret(Type::*f)(Params...) const) {
	return CallableSignatureWithKey<Key, Ret(Type::*)(Params...) const, Ret, Params...> {
		CallableSignature<Ret(Type::*)(Params...) const, Ret, Params...> {
			std::forward< Ret(Type::*)(Params...) const>(f),
		}
	};
}

template<typename Key = BindingKey_None, typename Type, typename Ret, typename... Params, typename... Args,
	typename = enable_if_t< is_base_of_v<BindingKey, Key> && is_class_v<Type> && (is_base_of_v<BindingKey, Type> == false)>>
constexpr auto makeCallableSignature(Ret(Type::*f)(Params...) const, Args&&... args) {
	return CallableSignatureWithKey<Key, Ret(Type::*)(Params...) const, Ret, Args...> {
		CallableSignature<Ret(Type::*)(Params...) const, Ret, Args...> {
			std::forward< Ret(Type::*)(Params...) const>(f),
			std::tuple<Args...>{ std::forward<Args>(args)... }
		}
	};
}

// Callable Class (Functor, Lambda)
template<typename Key = BindingKey_None, typename Callable, typename... Args,
	typename = std::enable_if_t<
		std::is_class_v<std::remove_reference_t<Callable>> && is_base_of_v<BindingKey, Key>
	>>
	constexpr auto makeCallableSignature(Callable&& callable, Args&&... args)
{
	using Ret = typename CallableInternalTypes<remove_reference_t<Callable>>::RetType;
	return CallableSignatureWithKey<Key, remove_reference_t<Callable>, typename CallableInternalTypes<remove_reference_t<Callable>>::RetType, Args...> {
		CallableSignature<Callable, Ret, Args...> {
			std::forward<Callable>(callable),
			std::tuple<Args...>{ std::forward<Args>(args)... }
		}
	};
}

static void nothing() {}
using NullCallableSignature = decltype(makeCallableSignature(nothing));

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

template<bool IsResolved, typename ReturnTypeTuple, typename KeyTypeTuple, typename... CallableSignatureTs>
struct CallableInfo;

#define CurrentReturnTypeTupleSelf	decltype(tuple_cat(std::declval<ReturnTypeTupleT>(), std::declval<tuple<typename CallableSignatureT::RetType>>()))
#define CurrentReturnTypeTuple		decltype(tuple_cat(std::declval<ReturnTypeTupleT>(), std::declval<tuple<typename CallableInfo<IsResolved, ReturnTypeTupleT, KeyTypeTupleT, CallableSignatureT>::CallableSignatureResolved::RetType>>()))
#define CurrentKeyTypeTuple			decltype(tuple_cat(std::declval<KeyTypeTupleT>(), std::declval<tuple<typename CallableSignatureT::KeyType>>()))

template<typename CallableSignatureT, typename... CallableSignatureTs>
constexpr bool isFirstSignatureResolved()
{
	return remove_reference_t<CallableSignatureT>::is_resolved;
}

///////////////////////////////////////////////////////////////////////
// CallableInfo

template<bool IsResolved, typename ReturnTypeTupleT, typename KeyTypeTupleT, typename CallableSignatureT>
struct CallableInfo<IsResolved, ReturnTypeTupleT, KeyTypeTupleT, CallableSignatureT>
{
	//static_assert(is_same_v<typename CallableSignatureT::KeyType, BindingKey_None> ||
	//	tuple_size_v<KeyTypeTupleT> == 0 ||
	//	find_type_in_tuple<true, typename CallableSignatureT::KeyType, KeyTypeTupleT>::value == ~0ull, "Failed to makeCallableInfo since key is duplicated.");

	using CallableSignatureResolved	= CallableSignatureT;
	using CallableSignatureResolvedTuple = std::tuple<CallableSignatureResolved>;

	using ReturnTypeTuple			= CurrentReturnTypeTupleSelf;
	using KeyTypeTuple				= CurrentKeyTypeTuple;

	CallableInfo()
	{
		using OrderedBindingSlotArgTypeTupleT		= decltype(mapTuple(std::declval<typename CallableSignatureT::ArgTypeTuple>(), std::declval<typename CallableSignatureT::BindingSlotIndexSequence>()));
		using OrderedBindingSlotArgIndexSequence	= typename FindType<true, CurrentKeyTypeTuple, OrderedBindingSlotArgTypeTupleT>::FoundIndexSeq;

		using OrderedReturnTypeTupleT				= decltype(mapTuple(std::declval<ReturnTypeTupleT>(), OrderedBindingSlotArgIndexSequence{}));
		using OrderedBindingSlotParamTypeTupleT		= decltype(mapTuple(std::declval<typename CallableSignatureT::ParamTypeTuple>(), std::declval<typename CallableSignatureT::BindingSlotIndexSequence>()));

		type_checker<CallableSignatureT>();
		//type_checker<typename CallableSignatureT::ParamTypeTuple>();

		BindingSlotTypeChecker<OrderedBindingSlotParamTypeTupleT, OrderedReturnTypeTupleT>::check();
	}
};

template<typename ReturnTypeTupleT, typename KeyTypeTupleT, typename CallableSignatureT>
struct CallableInfo<false, ReturnTypeTupleT, KeyTypeTupleT, CallableSignatureT>
#define ResolvedCallableInfo \
	CallableInfo<true, ReturnTypeTupleT, KeyTypeTupleT, decltype(makeCallableSignature< \
		typename CallableSignatureT::KeyType, \
		typename CallableSignatureT::Callable, \
		decltype(declval<typename CallableSignatureT::Callable>()(declval<LambdaTaskIdentifier>(), declval<KeyTypeTupleT>(), declval<ReturnTypeTupleT>())), \
		LambdaTaskIdentifier, KeyTypeTupleT, ReturnTypeTupleT&& \
	>(&CallableSignatureT::Callable::operator())) >
	: ResolvedCallableInfo
{
	//static_assert(CallableSignatureT::is_resolved == true, "substitution is not finished");

	CallableInfo()
	{
		//type_checker<decltype(makeCallableSignature<
		//	typename CallableSignatureT::KeyType,
		//	typename CallableSignatureT::Callable,
		//	decltype(declval<typename CallableSignatureT::Callable>()(declval<LambdaTaskIdentifier>(), declval<KeyTypeTupleT>(), declval<ReturnTypeTupleT>())),
		//	LambdaTaskIdentifier, KeyTypeTupleT, ReturnTypeTupleT&&
		//>(&CallableSignatureT::Callable::operator()))>();
	}

	//static_assert(std::is_same_v<LambdaTaskIdentifier, std::tuple_element_t<0, typename ResolvedCallableInfo::CallableSignatureResolved::ParamTypeTuple>>, "Why Fail?");
	using ReturnTypeTuple = typename ResolvedCallableInfo::ReturnTypeTuple;
	using KeyTypeTuple = typename ResolvedCallableInfo::KeyTypeTuple;
#undef ResolvedCallableInfo
};

template<bool IsResolved, typename ReturnTypeTupleT, typename KeyTypeTupleT, typename CallableSignatureT, typename... CallableSignatureTs>
struct CallableInfo<IsResolved, ReturnTypeTupleT, KeyTypeTupleT, CallableSignatureT, CallableSignatureTs...>
	: CallableInfo<IsResolved, ReturnTypeTupleT, KeyTypeTupleT, CallableSignatureT>
#define NextCallableInfo CallableInfo<isFirstSignatureResolved<CallableSignatureTs...>(), CurrentReturnTypeTuple, CurrentKeyTypeTuple, CallableSignatureTs...>
	, NextCallableInfo
{
	using CallableSignatureResolved = typename CallableInfo<IsResolved, ReturnTypeTupleT, KeyTypeTupleT, CallableSignatureT>::CallableSignatureResolved;
	using CallableSignatureResolvedTuple = decltype(std::tuple_cat(
		std::declval<std::tuple<CallableSignatureResolved>>(),
		std::declval<typename NextCallableInfo::CallableSignatureResolvedTuple>()));

	using ReturnTypeTuple = typename NextCallableInfo::ReturnTypeTuple;
	using KeyTypeTuple = typename NextCallableInfo::KeyTypeTuple;
#undef NextCallableInfo
};

#if 0 // 미사용 코드 정리: 기록해둘만한 인터페이스 패턴
/* Example: CallableInfo<CallableSignature, CallableSignature, AnyOtherType, CallableSignature, ...>
 * 초안에서의 경우처럼 CallableSignature 인자 사이에 다른 타입을 끼워넣어서 별도의 처리를 할 수 있음.
 * 
 * #초안
 * ```
 *	Dependency(
 *	    Task<Key0>(func0),                                                     // CallableSignature<Key0>
 *	    Task<Key1>(func1, binding_slot),               BindingKeys(Key0)       // CallableSignature<Key1>, BindingKeyList<Key0>
 *	    Task      (func2, binding_slot, binding_slot), BindingKeys(Key1, Key0) // CallableSignature<None>, BindingKeyList<Key1, Key0>
 *	)
 * ```
 * 
 * 위 코드는 아래의 CallableInfo를 생성함. (CS: CallableSignature, K: Key, B: BindingKeyList)
 * CallableInfo< CS<K0>, CS<K1>, B<K0>, CS<None>, B<K1, K0> >
 */

template<bool IsResolved, typename ReturnTypeTupleT, typename KeyTypeTupleT, typename CallableSignatureT, size_t... Seqs>
struct CallableInfo<IsResolved, ReturnTypeTupleT, KeyTypeTupleT, CallableSignatureT, index_sequence<Seqs...>>
	: CallableInfo<IsResolved, ReturnTypeTupleT, KeyTypeTupleT, CallableSignatureT>
{
	CallableInfo()
	{
		using CallableSignatureResolved = typename CallableInfo<IsResolved, ReturnTypeTupleT, KeyTypeTupleT, CallableSignatureT>::CallableSignatureResolved;
		using OrderedReturnTypeTupleT = decltype(mapTuple(std::declval<ReturnTypeTupleT>(), index_sequence<Seqs...>{}));
		using OrderedBindingSlotTypeTupleT = decltype(mapTuple(std::declval<typename CallableSignatureResolved::ParamTypeTuple>(), std::declval<typename CallableSignatureResolved::BindingSlotIndexSequence>()));

		//BindingSlotTypeChecker<0, typename CallableSignatureT::ParamTypeTuple, typename CallableSignatureT::OriginalSignature::ParamTypeTuple, OrderedReturnTypeTupleT>::check();
		BindingSlotTypeChecker<OrderedBindingSlotTypeTupleT, OrderedReturnTypeTupleT>::check();
	}
};

template<bool IsResolved, typename ReturnTypeTupleT, typename KeyTypeTupleT, typename CallableSignatureT, typename... KeyTs>
struct CallableInfo<IsResolved, ReturnTypeTupleT, KeyTypeTupleT, CallableSignatureT, BindingKeyList<KeyTs...>>
	: CallableInfo<IsResolved, ReturnTypeTupleT, KeyTypeTupleT, CallableSignatureT, typename FindType<true, CurrentKeyTypeTuple, BindingKeyList<KeyTs...>>::FoundIndexSeq> {};

template<bool IsResolved, typename ReturnTypeTupleT, typename KeyTypeTupleT, typename CallableSignatureT, size_t... Seqs, typename... CallableSignatureTs>
struct CallableInfo<IsResolved, ReturnTypeTupleT, KeyTypeTupleT, CallableSignatureT, index_sequence<Seqs...>, CallableSignatureTs...>
	: CallableInfo<IsResolved, ReturnTypeTupleT, KeyTypeTupleT, CallableSignatureT, index_sequence<Seqs...>>
	, CallableInfo<isFirstSignatureResolved<CallableSignatureTs...>(), CurrentReturnTypeTuple, CurrentKeyTypeTuple, CallableSignatureTs...> {};

template<bool IsResolved, typename ReturnTypeTupleT, typename KeyTypeTupleT, typename CallableSignatureT, typename... KeyTs, typename... CallableSignatureTs>
struct CallableInfo<IsResolved, ReturnTypeTupleT, KeyTypeTupleT, CallableSignatureT, BindingKeyList<KeyTs...>, CallableSignatureTs...>
	: CallableInfo<IsResolved, ReturnTypeTupleT, KeyTypeTupleT, CallableSignatureT, typename FindType<true, CurrentKeyTypeTuple, BindingKeyList<KeyTs...>>::FoundIndexSeq, CallableSignatureTs...> {};

#endif

///////////////////////////////////////////////////////////////////////
// makeCallableInfo utility

template<typename... CallableSignatureTs>
auto makeCallableInfo(CallableSignatureTs&&...)
{
	return CallableInfo< isFirstSignatureResolved<CallableSignatureTs...>(), tuple<>, tuple<>, remove_reference_t<CallableSignatureTs>... > {};
}

///////////////////////////////////////////////////////////////////////
// ProcessBlock

struct __Task_Control {};
struct __Task_Condition : __Task_Control {};
struct __Task_ConditionCancel : __Task_Control {};
struct __Task_WaitWhile : __Task_Control {};

struct __Task_SwitchDefault {};

#define ProcessBlock(...)			[ __VA_ARGS__ ] (LambdaTaskIdentifier, auto info, auto&& resultTuple)
#define ConditionExpression(...)	[=] (LambdaTaskIdentifier, auto info, auto&& resultTuple) -> const bool { return (__VA_ARGS__) ; }
#define Condition_Skip(...)			__Task_Condition{}, ConditionExpression(__VA_ARGS__)
#define Condition_Cancel(...)		__Task_ConditionCancel{}, ConditionExpression(__VA_ARGS__)
#define WaitWhile(...)				__Task_WaitWhile{}, ConditionExpression(__VA_ARGS__)

#define GetResult(Key)				std::get<find_type_in_tuple<true, std::remove_reference_t<Key>, decltype(info)>::value>(resultTuple)
#define BindResult(Key, Var)		Var = GetResult(Key)
#define AutoBindResult(Key, Var)	auto BindResult(Key, Var)

#define TaskSwitchDefault			__Task_SwitchDefault{}