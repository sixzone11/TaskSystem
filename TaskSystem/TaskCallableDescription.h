﻿#pragma once

#include "lambda_details.h"
#include "tuple_utility.h"

///////////////////////////////////////////////////////////////////////
// Type Checker
template<typename T> void type_checker() { T a = -1; }

///////////////////////////////////////////////////////////////////////
// BindingSlot

#if defined(_MSC_VER) && (_MSC_VER < 1930) // https://learn.microsoft.com/ko-kr/cpp/overview/compiler-versions?view=msvc-170#version-macros
#define IS_NOT_REQUIRED { static_assert(false, "why is this required?"); }
#else // _MSC_VER
#define IS_NOT_REQUIRED
#endif // _MSC_VER

struct BindingSlot
{
	// Note: 링크 시 아래 casting operator overloading의 기호가 필요하면 argument passing을 대응하는 부분이 잘못됐을 것.
	template<typename T> operator T && () const IS_NOT_REQUIRED;
	template<typename T> operator T& () const IS_NOT_REQUIRED;
};

template<typename T>
struct is_binding_slot : std::conditional_t<std::is_base_of_v<BindingSlot, T>, std::true_type, std::false_type> {};


template<typename T>
constexpr bool is_binding_slot_v = is_binding_slot<T>::value;

///////////////////////////////////////////////////////////////////////
// BindingKey

#define DefineBindingKey(KeyName)	struct KeyName : BindingKey { using KeyType = KeyName; }

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
constexpr bool is_deferred_substitution_v = std::is_same_v<T, deferred_substitution>;

///////////////////////////////////////////////////////////////////////
// pseudo_void

struct pseudo_void {};

template<typename T>
constexpr bool is_pseudo_void_v = std::is_same_v<T, pseudo_void>;


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

	using RetType = std::conditional_t<std::is_void_v<Ret>, pseudo_void, Ret>;
	using ParamTypeTuple = std::tuple<Params... >;

	using OriginalSignature = CallableSignature<Ret(*)(Params...), Ret, Params...>;
};

// Note(jiman): non-static member function의 cv-qualifier 중 const만 대응됨.

template<typename Type, typename Ret, typename... Params>
struct CallableInternalTypes<Ret(Type::*)(Params...) const>
{
	constexpr static bool is_resolved = true;
	using Callable = Ret(Type::*)(Params...) const;

	using RetType = std::conditional_t<std::is_void_v<Ret>, pseudo_void, Ret>;
	using ParamTypeTuple = std::tuple<const Type*, Params... >;

	using OriginalSignature = CallableSignature<Ret(Type::*)(Params...) const, Ret, Params...>;
};

template<typename Type, typename Ret, typename... Params>
struct CallableInternalTypes<Ret(Type::*)(Params...)>
{
	constexpr static bool is_resolved = true;
	using Callable = Ret(Type::*)(Params...);

	using RetType = std::conditional_t<std::is_void_v<Ret>, pseudo_void, Ret>;
	using ParamTypeTuple = std::tuple<Type*, Params... >;

	using OriginalSignature = CallableSignature<Ret(Type::*)(Params...), Ret, Params...>;
};

template<typename _Callable>
struct CallableInternalTypes<_Callable,
	std::void_t<decltype(&_Callable::operator())>> // Note(jiman): [SFINAE] function-call operator가 decltype으로 확정 가능한 형태만 허용.
{
	constexpr static bool is_resolved = true;
	using Callable = _Callable;

	using RetType = typename CallableInternalTypes<decltype(&_Callable::operator())>::RetType;
	using ParamTypeTuple = typename CallableInternalTypes<decltype(&_Callable::operator())>::ParamTypeTuple;

	using OriginalSignature = typename CallableInternalTypes<decltype(&_Callable::operator())>::OriginalSignature;
};

struct DefaultTaskIdentifier {};
struct LambdaTaskIdentifier {};

template<typename Type, typename Ret, typename KeyTuple, typename ResultTuple>
struct CallableInternalTypes<Ret(Type::*)(LambdaTaskIdentifier, KeyTuple, ResultTuple&&) const>
{
	constexpr static bool is_resolved = true;

	using RetType = std::conditional_t<std::is_void_v<Ret>, pseudo_void, Ret>;
	using ParamTypeTuple = std::tuple<const Type*, LambdaTaskIdentifier, KeyTuple, ResultTuple&&>;

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

#define CurrentIndexSequence std::conditional_t<is_binding_slot_v<std::remove_reference_t<Arg>>, std::index_sequence<Indices..., Index>, std::index_sequence<Indices...>>

template<size_t Index, size_t... Indices, typename Arg, typename... Args>
struct select_binding_slots<Index, std::index_sequence<Indices...>, std::tuple<Arg, Args...>> : select_binding_slots<Index + 1, CurrentIndexSequence, std::tuple<Args...>> {};

template<size_t Index, size_t... Indices, typename Arg>
struct select_binding_slots<Index, std::index_sequence<Indices...>, std::tuple<Arg>>
{
	using BindingSlotIndexSequence = CurrentIndexSequence;
};

template<size_t Index, size_t... Indices>
struct select_binding_slots<Index, std::index_sequence<Indices...>, std::tuple<>>
{
	using BindingSlotIndexSequence = std::index_sequence<Indices...>;
};

template<typename Tuple>
struct SelectBindingSlots : select_binding_slots<0, std::index_sequence<>, Tuple> {};

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
	using ArgTypeTuple = std::tuple<Args...>;

	using OriginalSignature = typename CallableInternalTypes<Callable>::OriginalSignature;
	auto getOriginalSignature() { return OriginalSignature{}; }

	using BindingSlotIndexSequence = typename SelectBindingSlots<ArgTypeTuple>::BindingSlotIndexSequence;

	Callable _callable;
	ArgTypeTuple _args;
};

//template<typename Callable, typename = std::enable_if_t<std::is_class_v<std::remove_reference_t<Callable>>>, typename... Args>
//struct CallableSignature<Callable, typename CallableInternalTypes<Callable>::RetType, Args...>

///////////////////////////////////////////////////////////////////////
// CallableSignatureWithKey

template<typename Key, typename Callable, typename Ret, typename... Args>
struct CallableSignatureWithKey : CallableSignature<Callable, Ret, Args...>
{
	static_assert(std::is_base_of_v<BindingKey, Key>, "Given Key is not a KeyType");
	using KeyType = std::conditional_t<std::is_same_v<BindingKey_None, Key>, typename CallableSignature<Callable, Ret, Args...>::KeyType, Key>;
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
template<typename Key = BindingKey_None, typename Ret, typename... Params, typename = std::enable_if_t<std::is_base_of_v<BindingKey, Key>>>
constexpr auto makeCallableSignature(Ret(*f)(Params...)) {
	return CallableSignatureWithKey<Key, Ret(*)(Params...), Ret> {
		CallableSignature<Ret(*)(Params...), Ret> {
			std::forward< Ret(*)(Params...)>(f),
			std::tuple<>{}
		}
	};
}

template<typename Key = BindingKey_None, typename Ret, typename... Params, typename... Args, typename = std::enable_if_t<std::is_base_of_v<BindingKey, Key>>>
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
	typename = std::enable_if_t< std::is_base_of_v<BindingKey, Key> && std::is_class_v<Type> && (std::is_base_of_v<BindingKey, Type> == false) >>
constexpr auto makeCallableSignature(Ret(Type::*f)(Params...)) {
	return CallableSignatureWithKey<Key, Ret(Type::*)(Params...), Ret> {
		CallableSignature<Ret(Type::*)(Params...), Ret> {
			std::forward< Ret(Type::*)(Params...)>(f),
			std::tuple<>{}
		}
	};
}

template<typename Key = BindingKey_None, typename Type, typename Ret, typename... Params, typename... Args,
	typename = std::enable_if_t< std::is_base_of_v<BindingKey, Key> && std::is_class_v<Type> && (std::is_base_of_v<BindingKey, Type> == false) >>
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
	typename = std::enable_if_t< std::is_base_of_v<BindingKey, Key> && std::is_class_v<Type> && (std::is_base_of_v<BindingKey, Type> == false)>>
constexpr auto makeCallableSignature(Ret(Type::*f)(Params...) const) {
	return CallableSignatureWithKey<Key, Ret(Type::*)(Params...) const, Ret> {
		CallableSignature<Ret(Type::*)(Params...) const, Ret> {
			std::forward< Ret(Type::*)(Params...) const>(f),
			std::tuple<>{}
		}
	};
}

template<typename Key = BindingKey_None, typename Type, typename Ret, typename... Params, typename... Args,
	typename = std::enable_if_t< std::is_base_of_v<BindingKey, Key> && std::is_class_v<Type> && (std::is_base_of_v<BindingKey, Type> == false)>>
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
		std::is_class_v<std::remove_reference_t<Callable>> && std::is_base_of_v<BindingKey, Key>
	>>
	constexpr auto makeCallableSignature(Callable&& callable, Args&&... args)
{
	using Ret = typename CallableInternalTypes<std::remove_reference_t<Callable>>::RetType;
	return CallableSignatureWithKey<Key, std::remove_reference_t<Callable>, typename CallableInternalTypes<std::remove_reference_t<Callable>>::RetType, Args...> {
		CallableSignature<Callable, Ret, Args...> {
			std::forward<Callable>(callable),
			std::tuple<Args...>{ std::forward<Args>(args)... }
		}
	};
}


///////////////////////////////////////////////////////////////////////
// NullCallableSignature

inline static void nothing() {}
using NullCallableSignature = decltype(makeCallableSignature(nothing));


///////////////////////////////////////////////////////////////////////
// is_callable_signature

template<typename T>
struct is_callable_signature : std::false_type {};

template<typename Callable, typename Ret, typename... Args>
struct is_callable_signature<CallableSignature<Callable, Ret, Args...>> : std::true_type {};

template<typename Key, typename Callable, typename Ret, typename... Args>
struct is_callable_signature<CallableSignatureWithKey<Key, Callable, Ret, Args...>> : is_callable_signature<CallableSignature<Callable, Ret, Args...>> {};

template<typename... Keys>
struct is_callable_signature<BindingKeyList<Keys...>> : std::true_type {};

template<typename T>
constexpr bool is_callable_signature_v = is_callable_signature<T>::value;


///////////////////////////////////////////////////////////////////////
// BindingSlotTypeChecker

template<typename OriginalTypeTupleT, typename ReturnTypeTupleT>
struct BindingSlotTypeChecker;

template<typename... ArgsOriginal, typename... ArgsGiven>
struct BindingSlotTypeChecker<std::tuple<ArgsOriginal...>, std::tuple<ArgsGiven...>>
{
	constexpr static bool check()
	{
		static_assert(sizeof...(ArgsOriginal) == sizeof...(ArgsGiven), "Argument tuple size is wrong.");
		static_assert(sizeof...(ArgsOriginal) == 0 || (std::is_same_v<ArgsOriginal, ArgsGiven> && ...), "Argument type is mismatched");
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

#define CurrentReturnTypeTupleSelf	decltype(std::tuple_cat(std::declval<ReturnTypeTupleT>(), std::declval<std::tuple<typename CallableSignatureT::RetType>>()))
#define CurrentReturnTypeTuple		decltype(std::tuple_cat(std::declval<ReturnTypeTupleT>(), std::declval<std::tuple<typename CallableInfo<IsResolved, ReturnTypeTupleT, KeyTypeTupleT, CallableSignatureT>::CallableSignatureResolved::RetType>>()))
#define CurrentKeyTypeTuple			decltype(std::tuple_cat(std::declval<KeyTypeTupleT>(), std::declval<std::tuple<typename CallableSignatureT::KeyType>>()))

template<typename CallableSignatureT, typename... CallableSignatureTs>
constexpr bool isFirstSignatureResolved()
{
	return std::remove_reference_t<CallableSignatureT>::is_resolved;
}

template<typename ParamTypeTuple, typename ArgTypeTuple, size_t ArgIndex, typename BindingSlotIndexSequence>
struct ResolveArgTuple;

template<typename ParamTypeTuple, typename ArgTypeTuple, typename BindingSlotIndexSequence>
struct ResolveArgTupleType;

template<typename ParamTypeTuple, typename ArgTypeTuple>
struct ResolveArgTupleType<ParamTypeTuple, ArgTypeTuple, std::index_sequence<>>
{
	using ArgTypeTupleResolved = ParamTypeTuple;
};

template<typename ParamTypeTuple, typename ArgTypeTuple, typename BindingSlotIndexSequence>
struct ResolveArgTupleType
{
	using ArgTypeTupleResolved = typename ResolveArgTuple<ParamTypeTuple, ArgTypeTuple, 0, BindingSlotIndexSequence>::ArgTypeTupleResolved;
};

template<typename ParamTypeTuple, typename ArgTypeTuple, size_t ArgIndex, size_t BindingSlotIndex, size_t... BindingSlotIndexSeq>
struct ResolveArgTuple<ParamTypeTuple, ArgTypeTuple, ArgIndex, std::index_sequence<BindingSlotIndex, BindingSlotIndexSeq...>>
{
	using ArgTypeTupleResolved = decltype( std::tuple_cat(
		std::declval<std::tuple<std::conditional_t<ArgIndex == BindingSlotIndex, std::tuple_element_t<ArgIndex, ParamTypeTuple>&, std::tuple_element_t<ArgIndex, ParamTypeTuple>>>>(),
		std::declval<typename std::conditional_t<
		ArgIndex == BindingSlotIndex,
		ResolveArgTuple<ParamTypeTuple, ArgTypeTuple, ArgIndex + 1, std::index_sequence<BindingSlotIndexSeq...>>,
		ResolveArgTuple<ParamTypeTuple, ArgTypeTuple, ArgIndex + 1, std::index_sequence<BindingSlotIndex, BindingSlotIndexSeq...>>
		>::ArgTypeTupleResolved>()
		));
	
	//std::conditional_t<ArgIndex == BindingSlotIndex, std::tuple_element_t<ArgIndex, ParamTypeTuple>&, std::tuple_element_t<ArgIndex, ArgTypeTuple>>;
	//typename std::conditional_t<
	//	ArgIndex == BindingSlotIndex,
	//	ResolveArgTuple<ParamTypeTuple, ArgTypeTuple, ArgIndex + 1, std::index_sequence<BindingSlotIndexSeq...>>,
	//	ResolveArgTuple<ParamTypeTuple, ArgTypeTuple, ArgIndex + 1, std::index_sequence<BindingSlotIndex, BindingSlotIndexSeq...>>
	//>::ArgTypeTupleResolved;
};

template<typename ParamTypeTuple, typename ArgTypeTuple, size_t ArgIndex>
struct ResolveArgTuple<ParamTypeTuple, ArgTypeTuple, ArgIndex, std::index_sequence<>>
{
	template<size_t BeginArgIndex, typename RemainIndexSeq>
	struct RemainArgTuple;

	template<size_t BeginArgIndex, size_t... ArgIndices>
	struct RemainArgTuple<BeginArgIndex, std::index_sequence<ArgIndices...>>
	{
		using ArgTypeTupleResolved = std::tuple<std::tuple_element_t<(BeginArgIndex + ArgIndices), ArgTypeTuple> ... >;
	};

	constexpr static size_t NumRemains = std::tuple_size_v<ArgTypeTuple> - ArgIndex;
	using ArgTypeTupleResolved = typename RemainArgTuple<ArgIndex, std::make_index_sequence<NumRemains>>::ArgTypeTupleResolved;
};

template<typename CallableSignatureT, typename ArgTypeTupleResolved>
struct ResolveCallableSignature;

template<typename CallableSignatureT, typename... Args>
struct ResolveCallableSignature<CallableSignatureT, std::tuple<Args...>>
{
	using CallableSignatureResolved = CallableSignatureWithKey<typename CallableSignatureT::KeyType, typename CallableSignatureT::Callable, typename CallableSignatureT::RetType, Args...>;
};

///////////////////////////////////////////////////////////////////////
// CallableInfo

template<bool IsResolved, typename ReturnTypeTupleT, typename KeyTypeTupleT, typename CallableSignatureT>
struct CallableInfo<IsResolved, ReturnTypeTupleT, KeyTypeTupleT, CallableSignatureT>
{
	//static_assert(is_same_v<typename CallableSignatureT::KeyType, BindingKey_None> ||
	//	tuple_size_v<KeyTypeTupleT> == 0 ||
	//	find_type_in_tuple<true, typename CallableSignatureT::KeyType, KeyTypeTupleT>::value == ~0ull, "Failed to makeCallableInfo since key is duplicated.");

	using ArgTupleTypeResolved = typename ResolveArgTupleType<typename CallableSignatureT::ParamTypeTuple, typename CallableSignatureT::ArgTypeTuple, typename CallableSignatureT::BindingSlotIndexSequence>::ArgTypeTupleResolved;

	using CallableSignatureResolved = typename ResolveCallableSignature<CallableSignatureT, ArgTupleTypeResolved>::CallableSignatureResolved;
	using CallableSignatureResolvedTuple = std::tuple<CallableSignatureResolved>;

	using ReturnTypeTuple			= CurrentReturnTypeTupleSelf;
	using KeyTypeTuple				= CurrentKeyTypeTuple;

	CallableInfo()
	{
		using OrderedBindingSlotArgTypeTupleT		= decltype(mapTuple(std::declval<typename CallableSignatureT::ArgTypeTuple>(), std::declval<typename CallableSignatureT::BindingSlotIndexSequence>()));
		using OrderedBindingKeyIndexSequence		= typename FindType<true, CurrentKeyTypeTuple, OrderedBindingSlotArgTypeTupleT>::FoundIndexSeq;

		using OrderedReturnTypeTupleT				= decltype(mapTuple(std::declval<ReturnTypeTupleT>(), OrderedBindingKeyIndexSequence{}));
		using OrderedBindingSlotParamTypeTupleT		= decltype(mapTuple(std::declval<typename CallableSignatureT::ParamTypeTuple>(), std::declval<typename CallableSignatureT::BindingSlotIndexSequence>()));

		BindingSlotTypeChecker<OrderedBindingSlotParamTypeTupleT, OrderedReturnTypeTupleT>::check();
	}
};

template<typename ReturnTypeTupleT, typename KeyTypeTupleT, typename CallableSignatureT>
struct CallableInfo<false, ReturnTypeTupleT, KeyTypeTupleT, CallableSignatureT>
#define ResolvedCallableInfo \
	CallableInfo<true, ReturnTypeTupleT, KeyTypeTupleT, decltype(makeCallableSignature< \
		typename CallableSignatureT::KeyType, \
		typename CallableSignatureT::Callable, \
		decltype(std::declval<typename CallableSignatureT::Callable>()(std::declval<LambdaTaskIdentifier>(), std::declval<KeyTypeTupleT>(), std::declval<ReturnTypeTupleT>())), \
		LambdaTaskIdentifier, KeyTypeTupleT, ReturnTypeTupleT&& \
	>(&CallableSignatureT::Callable::operator())) >
	: ResolvedCallableInfo
{
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
auto decl_CallableInfo(CallableSignatureTs&&...)
{
	return CallableInfo< isFirstSignatureResolved<CallableSignatureTs...>(), std::tuple<>, std::tuple<>, std::remove_reference_t<CallableSignatureTs>... > {};
}

template<typename... CallableSignatures>
auto decl_CallableInfoByTuple(std::tuple<CallableSignatures...>&&)
{
	return decltype(decl_CallableInfo(std::declval<CallableSignatures>() ...)) {};
}

///////////////////////////////////////////////////////////////////////
// ProcessBlock

struct __Task_Control {};
struct __Task_Condition : __Task_Control {};
struct __Task_ConditionCancel : __Task_Control {};
struct __Task_WaitWhile : __Task_Control {};

struct __Task_SwitchDefault {};

#define ProcessBlock(...)			__task_expand_1(TASK_PROCESS_SWITCH, ProcessBlock, __VA_ARGS__)
#define ConditionExpression(...)	[=] (LambdaTaskIdentifier, auto info, auto&& resultTuple) -> const bool { return (__VA_ARGS__) ; }
#define Condition_Skip(...)			__Task_Condition{}, ConditionExpression(__VA_ARGS__)
#define Condition_Cancel(...)		__Task_ConditionCancel{}, ConditionExpression(__VA_ARGS__)
#define WaitWhile(...)				__Task_WaitWhile{}, ConditionExpression(__VA_ARGS__)

/*
#define GetReturnOfTask(Task)		std::get<find_type_in_tuple<true, std::tuple_element_t<0, std::remove_reference_t<decltype(std::get<IndexTaskCallable>(Task))>>::KeyType, decltype(info)>::value>(resultTuple)
#define GetReturnByKey(Key)			std::get<find_type_in_tuple<true, std::remove_reference_t<Key>, decltype(info)>::value>(resultTuple)
*/

// Get a return from preceding task.
#define GetReturn(TaskOrKey)		__task_expand_1(TASK_PROCESS_SWITCH, GetReturn, TaskOrKey)
#define BindReturn(Key, Var)		Var = GetReturn(Key)
#define AutoBindReturn(Key, Var)	auto BindReturn(Key, Var)

#define TaskSwitchDefault			__Task_SwitchDefault{}



#define IsEmptyArgs__(_1, _2, _3, _4, _5, _6, _7, _8, _9, ...) _9
#define IsEmptyArgs(...) IsEmptyArgs__(__VA_ARGS__, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1)

#define CommaConnection_0() ,
#define CommaConnection_1()
#define CommaConnection_Select(N) CommaConnection_ ## N()
#define CommaConnection_Expand(N) CommaConnection_Select(N)
#define CommaConnection(...) CommaConnection_Expand(IsEmptyArgs(__VA_ARGS__))

#define TaskProcessBegin(task_name, ...)					__task_expand_0(TASK_PROCESS_SWITCH, TaskProcessBegin, task_name, __VA_ARGS__)
#define TaskProcessNext(task_name, ...)						__task_expand_0(TASK_PROCESS_SWITCH, TaskProcessNext, task_name, __VA_ARGS__)
#define TaskProcessEnd()									__task_expand_2(TASK_PROCESS_SWITCH, TaskProcessEnd )

#define Capture(...)										ProcessBlock(__VA_ARGS__)
#define TaskProcessBeginCapture(task_name, ...)				__task_expand_0(TASK_PROCESS_SWITCH, TaskProcessBeginCapture, task_name, __VA_ARGS__)
#define TaskProcessNextCapture(task_name, ...)				__task_expand_0(TASK_PROCESS_SWITCH, TaskProcessNextCapture, task_name, __VA_ARGS__)

#define create_task(...)									__task_expand_1(TASK_PROCESS_SWITCH, create_task, __VA_ARGS__)

#define TASK_PROCESS_SWITCH									__enable__
//#define TASK_PROCESS_SWITCH								__disable__
#define IS_TASK_SWITCH_ENABLED								__task_expand_2(TASK_PROCESS_SWITCH, TaskSwitch)

#define __task_concat(x, y)									x##y
#define __task_expand_0(x, y, task_name, ...)				__task_concat(x, y)(task_name, __VA_ARGS__)
#define __task_expand_1(x, y, ...)							__task_concat(x, y)(__VA_ARGS__)
#define __task_expand_2(x, y)								__task_concat(x, y)()

#define __enable__TaskSwitch()								1
#define __enable__TaskProcessBegin(task_name, ...)			auto task_name = TaskWriter::task(__VA_ARGS__ ProcessBlock()
#define __enable__TaskProcessNext(task_name, ...)			); auto task_name = TaskWriter::task(__VA_ARGS__ ProcessBlock()
#define __enable__TaskProcessEnd()							)
#define __enable__TaskProcessBeginCapture(task_name, ...)	auto task_name = TaskWriter::task(__VA_ARGS__
#define __enable__TaskProcessNextCapture(task_name, ...)	); auto task_name = TaskWriter::task(__VA_ARGS__
#define __enable__create_task(...)							ITaskManager::createTask(__VA_ARGS__)

#define __enable__ProcessBlock(...)							[ __VA_ARGS__ ](LambdaTaskIdentifier, auto info, auto&& resultTuple)
#define __enable__GetReturn(TaskOrKey)						std::get<find_type_in_tuple<true, typename decltype(TaskOrKey())::KeyType, decltype(info)>::value>(resultTuple)


#define __disable__TaskSwitch()								0
#define __disable__TaskProcessBegin(task_name, ...)			auto task_name = [&]()
#define __disable__TaskProcessNext(task_name, ...)			(); auto task_name = [&]()
#define __disable__TaskProcessEnd()							()
#define __disable__TaskProcessBeginCapture(task_name, ...)	auto task_name = __VA_ARGS__
#define __disable__TaskProcessNextCapture(task_name, ...)	(); auto task_name = __VA_ARGS__
#define __disable__create_task(...)							((ITask*)(nullptr))

#define __disable__ProcessBlock(...)						[ &, __VA_ARGS__ ]()
#define __disable__GetReturn(TaskOrKey)						TaskOrKey
