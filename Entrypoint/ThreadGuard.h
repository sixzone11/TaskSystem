#pragma once

#include <type_traits>
#include <memory>
#include <unordered_map>
#include <string>

#include "TaskSystem/tuple_utility.h"

///////////////////////////////////////////////////////////////////////
//
// struct to tuple
//
///////////////////////////////////////////////////////////////////////

#if 1

template <class T, class... TArgs>  decltype(void(T{ std::declval<TArgs>()... }), std::true_type{})     test_is_braces_constructible(int);
template <class, class...>          std::false_type                                                     test_is_braces_constructible(...);
template <class T, class... TArgs>  using is_braces_constructible = decltype(test_is_braces_constructible<T, TArgs...>(0));

struct any_type {
	template<class T>
	constexpr operator T(); // non explicit
};

template<class T>
auto to_tuple(T&& object) noexcept {
	using type = std::decay_t<T>;
	if constexpr (is_braces_constructible<type, any_type, any_type, any_type, any_type, any_type, any_type, any_type>{}) {
		auto&& [p1, p2, p3, p4, p5, p6, p7] = std::forward<T>(object);
		return std::make_tuple(std::forward<decltype(p1)>(p1), std::forward<decltype(p2)>(p2), std::forward<decltype(p3)>(p3), std::forward<decltype(p4)>(p4), std::forward<decltype(p5)>(p5), std::forward<decltype(p6)>(p6), std::forward<decltype(p7)>(p7));
	}
	else if constexpr (is_braces_constructible<type, any_type, any_type, any_type, any_type, any_type, any_type>{}) {
		auto&& [p1, p2, p3, p4, p5, p6] = std::forward<T>(object);
		return std::make_tuple(std::forward<decltype(p1)>(p1), std::forward<decltype(p2)>(p2), std::forward<decltype(p3)>(p3), std::forward<decltype(p4)>(p4), std::forward<decltype(p5)>(p5), std::forward<decltype(p6)>(p6));
	}
	else if constexpr (is_braces_constructible<type, any_type, any_type, any_type, any_type, any_type>{}) {
		auto&& [p1, p2, p3, p4, p5] = std::forward<T>(object);
		return std::make_tuple(std::forward<decltype(p1)>(p1), std::forward<decltype(p2)>(p2), std::forward<decltype(p3)>(p3), std::forward<decltype(p4)>(p4), std::forward<decltype(p5)>(p5));
	}
	else if constexpr (is_braces_constructible<type, any_type, any_type, any_type, any_type>{}) {
		auto&& [p1, p2, p3, p4] = std::forward<T>(object);
		return std::make_tuple(std::forward<decltype(p1)>(p1), std::forward<decltype(p2)>(p2), std::forward<decltype(p3)>(p3), std::forward<decltype(p4)>(p4));
	}
	else if constexpr (is_braces_constructible<type, any_type, any_type, any_type>{}) {
		auto&& [p1, p2, p3] = std::forward<T>(object);
		return std::make_tuple(std::forward<decltype(p1)>(p1), std::forward<decltype(p2)>(p2), std::forward<decltype(p3)>(p3));
	}
	else if constexpr (is_braces_constructible<type, any_type, any_type>{}) {
		auto&& [p1, p2] = std::forward<T>(object);
		return std::make_tuple(std::forward<decltype(p1)>(p1), std::forward<decltype(p2)>(p2));
	}
	else if constexpr (is_braces_constructible<type, any_type>{}) {
		auto&& [p1] = std::forward<T>(object);
		return std::make_tuple(std::forward<decltype(p1)>(p1));
	}
	else {
		return std::make_tuple();
	}
}

#else
template <class T, class... TArgs>  decltype(void(T{ std::declval<TArgs>()... }), std::true_type{})     test_is_braces_constructible(int);
template <class, class...>          std::false_type                                                     test_is_braces_constructible(...);
template <class T, class... TArgs>  using is_braces_constructible = decltype(test_is_braces_constructible<std::decay_t<T>, TArgs...>(0));

struct any_type {
	template<class T>
	constexpr operator T(); // non explicit
};


#define PARAMS_MACRO_1(macro) macro(1)
#define PARAMS_MACRO_2(macro) PARAMS_MACRO_1(macro), macro(2)
#define PARAMS_MACRO_3(macro) PARAMS_MACRO_2(macro), macro(3)
#define PARAMS_MACRO_4(macro) PARAMS_MACRO_3(macro), macro(4)
#define PARAMS_MACRO_5(macro) PARAMS_MACRO_4(macro), macro(5)
#define PARAMS_MACRO_6(macro) PARAMS_MACRO_5(macro), macro(6)
#define PARAMS_MACRO_7(macro) PARAMS_MACRO_6(macro), macro(7)
#define PARAMS_MACRO_8(macro) PARAMS_MACRO_7(macro), macro(8)
#define PARAMS_MACRO_9(macro) PARAMS_MACRO_8(macro), macro(9)

#define GENERATE_MULTIPARAMS(macro) \
    macro(PARAMS_MACRO_1) \
    macro(PARAMS_MACRO_2) \
    macro(PARAMS_MACRO_3) \
    macro(PARAMS_MACRO_4) \
    macro(PARAMS_MACRO_5) \
    macro(PARAMS_MACRO_6) \
    macro(PARAMS_MACRO_7) \
    macro(PARAMS_MACRO_8) \
    macro(PARAMS_MACRO_9)


#define TYPE_MACRO(v) any_type
#define ARGS_MACRO(v) p##v

#define TO_TUPLE_MACRO(macro) \
    template<typename T, std::enable_if_t < \
        is_braces_constructible<T, macro(TYPE_MACRO)>::value \
        && !is_braces_constructible<T, macro(TYPE_MACRO), any_type>::value \
        , int> = 0> \
        auto to_tuple(T&& object) noexcept \
    { \
        auto&&[macro(ARGS_MACRO)] = object; \
        return std::make_tuple(macro(ARGS_MACRO)); \
    }

GENERATE_MULTIPARAMS(TO_TUPLE_MACRO)
#endif

///////////////////////////////////////////////////////////////////////
//
// ThreadGuard
//
///////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// has_finding_field

template<typename T, typename = void>
struct has_finding_field : std::false_type { };

template<typename T>
struct has_finding_field<T, std::void_t<decltype(T::___this_is_a_finding_field)>> : std::true_type { };

template<typename T>
constexpr bool has_finding_field_v = has_finding_field<T>::value;


////////////////////////////////////////////////////////////////////////////////
// is_complete

template <typename T, typename = void>
struct is_complete : std::false_type {};

template <typename T>
struct is_complete<T, std::void_t<decltype(sizeof(T) != 0)>> : std::true_type {};

template<typename T>
constexpr bool is_complete_v = is_complete<T>::value;


////////////////////////////////////////////////////////////////////////////////
// Test

struct IntermediateType;

struct TypeA
{
	int a;
	float b;
	char c;
	double d;
	IntermediateType* e;
	std::unique_ptr<IntermediateType> f;
	int g;
	//std::unordered_map<std::string, IntermediateType> h;
};


///////////////////////////////////////////////////////////////////////
//
// Traverse Struct
//
///////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// FindTypeTraverseInStruct

template<typename Struct, typename Finding, bool IsStruct>
struct FindTypeTraverseInStruct;

template<typename Tuple, typename Finding, size_t... Iseq>
constexpr bool FindTypeTraverseImpl(std::index_sequence<Iseq...>)
{
	return (... || (std::is_same_v<std::tuple_element_t<Iseq, Tuple>, Finding> || FindTypeTraverseInStruct<std::tuple_element_t<Iseq, Tuple>, Finding, std::is_class_v<std::tuple_element_t<Iseq, Tuple>>>::result));
}

template<typename Tuple, typename Finding>
constexpr bool FindTypeTraverseInTuple()
{
	return FindTypeTraverseImpl<Tuple, Finding>(std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}

template<typename Struct, typename Finding>
struct FindTypeTraverseInStruct<Struct, Finding, false>
{
	constexpr static bool result = std::is_same_v<Struct, Finding>;
};

template<typename Struct, typename Finding>
struct FindTypeTraverseInStruct<Struct, Finding, true>
{
	constexpr static bool result = FindTypeTraverseInTuple<decltype(to_tuple(std::declval<Struct>())), Finding>();
};

template<template<typename... Ts> typename AnyTemplateType, typename... AnyTemplateArguments, typename Finding>
struct FindTypeTraverseInStruct<AnyTemplateType<AnyTemplateArguments...>, Finding, true>
{
	constexpr static bool result = (... || (std::is_same_v<std::decay_t<AnyTemplateArguments>, Finding> || FindTypeTraverseInStruct<std::decay_t<AnyTemplateArguments>, Finding, std::is_class_v<std::decay_t<AnyTemplateArguments>>>::result));
};


////////////////////////////////////////////////////////////////////////////////
// FindFieldTraverseInStruct

template<typename Struct, bool IsStruct>
struct FindFieldTraverseInStruct;

template<typename Tuple, size_t... Iseq>
constexpr bool FindFieldTraverseImpl(std::index_sequence<Iseq...>)
{
	return (... || (has_finding_field_v<std::tuple_element_t<Iseq, Tuple>> || FindFieldTraverseInStruct<std::tuple_element_t<Iseq, Tuple>, std::is_class_v<std::tuple_element_t<Iseq, Tuple>>>::result));
}

template<typename Tuple>
constexpr bool FindFieldTraverseInTuple()
{
	return FindFieldTraverseImpl<Tuple>(std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}

template<typename Struct>
struct FindFieldTraverseInStruct<Struct, false>
{
	constexpr static bool result = has_finding_field_v<Struct>;
};

template<typename Struct>
struct FindFieldTraverseInStruct<Struct*, false>
{
	constexpr static bool result = has_finding_field_v<Struct> || FindFieldTraverseInStruct<Struct, std::is_class_v<Struct>>::result;
};

template<typename Struct>
struct FindFieldTraverseInStruct<Struct&, false>
{
	constexpr static bool result = has_finding_field_v<Struct> || FindFieldTraverseInStruct<Struct, std::is_class_v<Struct>>::result;
};

template<typename Struct>
struct FindFieldTraverseInStruct<Struct, true>
{

	constexpr static bool result = FindFieldTraverseInTuple<decltype(to_tuple(std::declval<Struct>()))>();
};

template<template<typename... Ts> typename AnyTemplateType, typename... AnyTemplateArguments>
struct FindFieldTraverseInStruct<AnyTemplateType<AnyTemplateArguments...>, true>
{
	constexpr static bool result = (... || (has_finding_field_v<std::decay_t<AnyTemplateArguments>> || FindFieldTraverseInStruct<std::decay_t<AnyTemplateArguments>, std::is_class_v<std::decay_t<AnyTemplateArguments>>>::result));
};

////////////////////////////////////////////////////////////////////////////////
// TraverseInStruct

template<typename Struct, bool IsStruct = std::is_class_v<Struct>>
struct TraverseInStruct;


template<typename Tuple, typename = std::make_index_sequence<std::tuple_size_v<Tuple>>>
struct TraverseInTuple;

template<typename Tuple, size_t... Iseq>
struct TraverseInTuple<Tuple, std::index_sequence<Iseq...>>
{
	using found_tuple = decltype(
		std::tuple_cat(std::declval<
			std::conditional_t<
				has_finding_field_v<std::tuple_element_t<Iseq, Tuple>>,
				std::tuple<std::tuple_element_t<Iseq, Tuple>>,
				typename TraverseInStruct<std::tuple_element_t<Iseq, Tuple>, std::is_class_v<std::tuple_element_t<Iseq, Tuple>>>::has_finding_field_tuple
			>
		>() ...));

	using has_finding_field_tuple = tuple_distinct_t<found_tuple>;
};

template<typename Struct>
struct TraverseInStruct<Struct, false>
{
	using has_finding_field_tuple = std::conditional_t<has_finding_field_v<Struct>, std::tuple<Struct>, std::tuple<>>;
};

template<typename Struct>
struct TraverseInStruct<Struct*, false>
{
	using has_finding_field_tuple = std::conditional_t<has_finding_field_v<Struct>, std::tuple<Struct>, typename TraverseInStruct<Struct, std::is_class_v<Struct>>::has_finding_field_tuple>;
};

template<typename Struct>
struct TraverseInStruct<Struct&, false>
{
	using has_finding_field_tuple = std::conditional_t<has_finding_field_v<Struct>, std::tuple<Struct>, typename TraverseInStruct<Struct, std::is_class_v<Struct>>::has_finding_field_tuple>;
};

template<typename Struct>
struct TraverseInStruct<Struct, true>
{
	using has_finding_field_tuple = typename TraverseInTuple<decltype(to_tuple(std::declval<Struct>()))>::has_finding_field_tuple;
};

template<template<typename... Ts> typename AnyTemplateType, typename... AnyTemplateArguments>
struct TraverseInStruct<AnyTemplateType<AnyTemplateArguments...>, true>
{
	using found_tuple = decltype(
		std::tuple_cat(
			std::declval<std::conditional_t<
				has_finding_field_v<std::decay_t<AnyTemplateArguments>>,
				std::tuple<std::decay_t<AnyTemplateArguments>>,
				typename TraverseInStruct<std::decay_t<AnyTemplateArguments>, std::is_class_v<std::decay_t<AnyTemplateArguments>>>::has_finding_field_tuple>
			>() ...) );

	using has_finding_field_tuple = tuple_distinct_t<found_tuple>;
};

///////////////////////////////////////////////////////////////////////
//
// ThreadGuard
//
///////////////////////////////////////////////////////////////////////

template<typename TypeToGuard>
struct ThreadGuard;

template<typename TypeToGuard, typename = void>
struct ReadOnlyAccessor
{
	ReadOnlyAccessor(ThreadGuard<TypeToGuard>& guard) : _guard(guard) {}
	operator const TypeToGuard& () const { return _guard; }
	const TypeToGuard& get() const { return _guard; }

private:
	ThreadGuard<TypeToGuard>& _guard;
};

template<typename TypeToGuard>
struct ReadOnlyAccessor<std::unique_ptr<TypeToGuard>, void/*, std::void_t<decltype(&PtrType<TypeToGuard>::operator->)>*/>
{
	ReadOnlyAccessor(ThreadGuard<std::unique_ptr<TypeToGuard>>& guard) : _guard(guard) {}
	operator const std::unique_ptr<TypeToGuard>& () const { return _guard; }
	const TypeToGuard* get() const { return _guard.get(); }

	const TypeToGuard* operator -> () const { return _guard.get(); }

private:
	ThreadGuard<std::unique_ptr<TypeToGuard>>& _guard;
};

template<typename TypeToGuard>
struct ReadOnlyAccessor<std::shared_ptr<TypeToGuard>, void/*, std::void_t<decltype(&PtrType<TypeToGuard>::operator->)>*/>
{
	ReadOnlyAccessor(ThreadGuard<std::shared_ptr<TypeToGuard>>& guard) : _guard(guard) {}
	operator const std::shared_ptr<TypeToGuard>& () const { return _guard; }
	const TypeToGuard* get() const { return _guard.get(); }

	const TypeToGuard* operator -> () const { return _guard.get(); }

private:
	ThreadGuard<std::shared_ptr<TypeToGuard>>& _guard;
};


//template<typename TypeToGuard, template<typename... Types> typename PtrType>
//struct ReadOnlyAccessor<PtrType<TypeToGuard>, std::void_t<decltype(&PtrType<TypeToGuard>::operator->)>>
//{
//ReadOnlyAccessor(ThreadGuard<PtrType<TypeToGuard>>& guard) : _guard(guard) {}
//operator const PtrType<TypeToGuard>& () { return _guard; }
//const TypeToGuard* get() { return _guard.get(); }
//
//const TypeToGuard* operator -> () { return _guard.get(); }
//
//private:
//	ThreadGuard<PtrType<TypeToGuard>>& _guard;
//};

template<typename TypeToGuard>
struct ReadWriteAccessor
{
	ReadWriteAccessor(ThreadGuard<TypeToGuard>& guard) : _guard(guard)
	{
		TraverseInStruct<TypeToGuard>::has_finding_field_tuple;
	}
	~ReadWriteAccessor()
	{

	}

	operator TypeToGuard& () { return _guard; }
	TypeToGuard& get() { return _guard; }

private:
	ThreadGuard<TypeToGuard>& _guard;
};

template<typename TypeToGuard>
struct ReadWriteAccessor<std::unique_ptr<TypeToGuard>>
{
	ReadWriteAccessor(ThreadGuard<std::unique_ptr<TypeToGuard>>& guard) : _guard(guard) {}
	operator const std::unique_ptr<TypeToGuard>& () const { return _guard; }
	const TypeToGuard* get() const { return _guard.get(); }

	const TypeToGuard* operator -> () const { return _guard.get(); }

private:
	ThreadGuard<std::unique_ptr<TypeToGuard>>& _guard;
};

template<typename TypeToGuard>
struct ReadWriteAccessor<std::shared_ptr<TypeToGuard>>
{
	ReadWriteAccessor(ThreadGuard<std::shared_ptr<TypeToGuard>>& guard) : _guard(guard) {}
	operator const std::shared_ptr<TypeToGuard>& () const { return _guard; }
	const TypeToGuard* get() const { return _guard.get(); }

	const TypeToGuard* operator -> () const { return _guard.get(); }

private:
	ThreadGuard<std::shared_ptr<TypeToGuard>>& _guard;
};

template<typename TypeToGuard>
struct OwnerOf
{
	// inline static since c++17
	// Ref: https://en.cppreference.com/w/cpp/language/static#Static_data_members
	static inline uint32_t ___this_is_a_finding_field = 0;
};

template<typename TypeToGuard>
struct ThreadGuardInternal
{
	// inline static since c++17
	// Ref: https://en.cppreference.com/w/cpp/language/static#Static_data_members
	static inline uint32_t ___this_is_a_finding_field = 0;
};

template<typename TypeToGuard>
struct ThreadGuard : protected TypeToGuard
{
	friend ReadOnlyAccessor<TypeToGuard>;
	friend ReadWriteAccessor<TypeToGuard>;

	using TypeToGuard::TypeToGuard;

private:
	ThreadGuardInternal<TypeToGuard> _internal;
};

template<typename TypeToGuard>
struct ReadOnlyAccessorForGroup
{
	// inline static since c++17
	// Ref: https://en.cppreference.com/w/cpp/language/static#Static_data_members
	static inline ThreadGuardInternal<TypeToGuard> _internal;

	ReadOnlyAccessorForGroup()
	{
	}

	~ReadOnlyAccessorForGroup()
	{
	}
};

template<typename TypeToGuard>
struct ReadWriteAccessorForGroup
{
	// inline static since c++17
	// Ref: https://en.cppreference.com/w/cpp/language/static#Static_data_members
	static inline ThreadGuardInternal<TypeToGuard> _internal;

	ReadWriteAccessorForGroup()
	{
	}

	~ReadWriteAccessorForGroup()
	{
	}
};

template<typename T>
struct FindingType
{
	static uint32_t ___this_is_a_finding_field;
};

struct IntermediateType
{
	int a;
	FindingType<int> f;
};

template<typename TypeToGuard, bool ForwardDeclared>
struct DeferredGuard
{

};

struct TestTypeToCheck;

#define SET_AS_OWNER_OF(type) static OwnerOf<type> _ownerOf##type