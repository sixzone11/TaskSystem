#include "pch.h"


using namespace std;

struct BindingSlot
{
	template<typename T> operator T && () const;
	template<typename T> operator T& () const;
};

template<typename T>	struct is_binding_slot : false_type {};
template<>				struct is_binding_slot<BindingSlot> : true_type {};


template<typename T>
constexpr bool is_binding_slot_v = is_binding_slot<T>::value;

struct pseudo_void {};

template<typename Callable>
struct CallableInternalTypes;

template<typename Callable, typename Ret, typename...Params>
struct CallableSignature;

template<typename Ret, typename... Params>
struct CallableInternalTypes<Ret(*)(Params...)>
{
	using RetType = conditional_t<is_void_v<Ret>, pseudo_void, Ret>;
	using ParamTypeTuple = tuple<Params... >;

	using OriginalSignature = CallableSignature<Ret(*)(Params...), Ret, Params...>;
};

template<size_t Index, typename SlotIdxSeq, typename Tuple>
struct select_binding_slots;

template<typename Tuple>
struct SelectBindingSlots : select_binding_slots<0, index_sequence<>, Tuple> {};

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

template<typename Callable, typename Ret, typename... Params>
struct CallableSignature
{
	using RetType = typename CallableInternalTypes<Callable>::RetType;
	using ParamTypeTuple = tuple<Params...>;

	using OriginalSignature = typename CallableInternalTypes<Callable>::OriginalSignature;
	auto getOriginalSignature() { return OriginalSignature{}; }

	using BindingSlotIndexSequence = typename SelectBindingSlots<ParamTypeTuple>::ReturnIndexSequence;
};

template<typename Func>
auto makeCallableSignature(Func&& func);

template<typename Ret, typename... Params>
auto makeCallableSignature(Ret(*f)(Params...))
{
	return CallableSignature< remove_reference_t<decltype(f)>, Ret, Params...> {};
}

template<typename Ret, typename... Params, typename... Args>
auto makeCallableSignature(Ret(*f)(Params...), Args...)
{
	return CallableSignature<remove_reference_t<decltype(f)>, Ret, Args... > {};
}

template <typename Tuple, size_t... Seqs>
auto mapTuple(Tuple&& t, index_sequence<Seqs...>) {
	return std::make_tuple(std::get<Seqs>(t)...);
}

template<typename OriginalTypeTupleT, typename ReturnTypeTupleT>
struct BindingSlotTypeChecker;

template<typename... ArgsOriginal, typename... ArgsGiven>
struct BindingSlotTypeChecker<tuple<ArgsOriginal...>, tuple<ArgsGiven...>>
{
	constexpr static bool check()
	{
		static_assert(sizeof...(ArgsOriginal) == sizeof...(ArgsGiven), "Argument tuple size is wrong.");
		static_assert(sizeof...(ArgsOriginal) == 0 || (is_same_v<ArgsOriginal, ArgsGiven> && ...), "Arg");
		return true;
	}
};


template<typename ReturnTypeTuple, typename... CallableSignatureTs>
struct CallableInfo;

#define CurrentReturnTypeTuple decltype(tuple_cat(ReturnTypeTupleT {}, tuple<typename CallableSignatureT::RetType>{}))

template<typename ReturnTypeTupleT, typename CallableSignatureT>
struct CallableInfo<ReturnTypeTupleT, CallableSignatureT>
{
	using ReturnTypeTuple = CurrentReturnTypeTuple;
};

template<typename ReturnTypeTupleT, typename CallableSignatureT, typename... CallableSignatureTs>
struct CallableInfo<ReturnTypeTupleT, CallableSignatureT, CallableSignatureTs...>
	: CallableInfo<CurrentReturnTypeTuple, CallableSignatureTs...>
{
	using ReturnTypeTuple = CurrentReturnTypeTuple;
};

template<typename ReturnTypeTupleT, typename CallableSignatureT, size_t... Seqs>
struct CallableInfo<ReturnTypeTupleT, CallableSignatureT, index_sequence<Seqs...>>
{
	using OrderedReturnTypeTupleT = decltype(mapTuple(ReturnTypeTupleT{}, index_sequence<Seqs...>{}));
	using OrderedBindingSlotTypeTupleT = decltype(mapTuple(typename CallableSignatureT::OriginalSignature::ParamTypeTuple{}, typename CallableSignatureT::BindingSlotIndexSequence{}));
	//tuple<tuple_element<Seqs..., ReturnTypeTupleT>>::type>;
	CallableInfo()
	{
		//BindingSlotTypeChecker<0, typename CallableSignatureT::ParamTypeTuple, typename CallableSignatureT::OriginalSignature::ParamTypeTuple, OrderedReturnTypeTupleT>::check();
		BindingSlotTypeChecker<OrderedBindingSlotTypeTupleT, OrderedReturnTypeTupleT>::check();
	}

	using ReturnTypeTuple = CurrentReturnTypeTuple;
};

template<typename ReturnTypeTupleT, typename CallableSignatureT, size_t... Seqs, typename... CallableSignatureTs>
struct CallableInfo<ReturnTypeTupleT, CallableSignatureT, index_sequence<Seqs...>, CallableSignatureTs...>
	: CallableInfo<CurrentReturnTypeTuple, CallableSignatureTs...>
{
	using ReturnTypeTuple = CurrentReturnTypeTuple;
};

template<typename... CallableSignatures>
auto makeCallableInfo(CallableSignatures&&... signatures)
{
	return CallableInfo<tuple<>, remove_reference_t<decltype(signatures)>...>{};
}

int testIntRet() { return 0; }
float testFloatRet() { return 2.f; }
void testVoidRet(int, float) {}

void tes2222t()
{
	auto callableSignature0 = makeCallableSignature(testFloatRet);
	auto callableSignature1 = makeCallableSignature(testIntRet);
	auto callableSignature2 = makeCallableSignature(testVoidRet);
	auto callableSignature2_binding = makeCallableSignature(testVoidRet, BindingSlot(), BindingSlot());
	auto callableSignature2_pure = callableSignature2_binding.getOriginalSignature();

	auto callableInfo1 = makeCallableInfo(callableSignature1, callableSignature2);
	auto callableInfo2 = makeCallableInfo(callableSignature0, callableSignature1, makeCallableSignature(testVoidRet, BindingSlot(), BindingSlot()), index_sequence<1, 0>{});
}