#include "pch.h"

#include <tuple>
#include <type_traits>
#include <memory>
#include <unordered_map>
#include <string>

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


template<typename T, typename = void>
struct has_finding_field : std::false_type { };

template<typename T>
struct has_finding_field<T, std::void_t<decltype(T::___this_is_a_finding_field)>>
    : std::true_type { };

template<typename T>
constexpr bool has_finding_field_v = has_finding_field<T>::value;

template<typename TypeToGuard>
struct ThreadGuard;

template<typename TypeToGuard>
struct ReadOnlyAccessor
{
    ReadOnlyAccessor(ThreadGuard<TypeToGuard>& guard) : _guard(guard) {}
    operator const TypeToGuard& () { return _guard; }
    const TypeToGuard& get() { return _guard; }

    ThreadGuard<TypeToGuard>& _guard;
};

template<typename TypeToGuard>
struct ReadWriteAccessor
{
    ReadWriteAccessor(ThreadGuard<TypeToGuard>& guard) : _guard(guard) {}
    operator TypeToGuard& () { return _guard; }
    TypeToGuard& get() { return _guard; }

    ThreadGuard<TypeToGuard>& _guard;
};

template<typename TypeToGuard>
struct ThreadGuardInternal
{
    static uint32_t ___this_is_a_finding_field;
};

template<typename TypeToGuard>
uint32_t ThreadGuardInternal<TypeToGuard>::___this_is_a_finding_field = 0;

template<typename TypeToGuard>
struct ThreadGuard : protected TypeToGuard
{
    friend ReadOnlyAccessor<TypeToGuard>;
    friend ReadWriteAccessor<TypeToGuard>;

    ThreadGuardInternal<TypeToGuard> _internal;
};

template<typename TypeToGuard, bool ReadOnly>
struct GlobalThreadGuard
{
    ThreadGuardInternal<TypeToGuard> _internal;

    GlobalThreadGuard()
    {
    }

    ~GlobalThreadGuard()
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

struct TypeA
{
    int a;
    float b;
    char c;
    double d;
    //IntermediateType e;
    //std::unique_ptr<IntermediateType> e;
    int f;
    std::unordered_map<std::string, IntermediateType> g;
};


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
    constexpr static bool result = false;
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
    constexpr static bool result = false;
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

void threadguard_test()
{
    //static_assert(FindTypeTraverseInStruct < TypeA, FindingType, true>::result, "Has no FindingType");
    static_assert(FindFieldTraverseInStruct<TypeA, true>::result, "Has no FindingType");
}