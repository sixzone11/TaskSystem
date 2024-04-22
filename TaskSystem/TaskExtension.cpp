#include "pch.h"


using namespace std;

struct Bindingslot
{
	template<typename T> operator T && () const;
	template<typename T> operator T& () const;
};

template<typename T>	struct isBindingSlot : false_type {};
template<>				struct isBindingSlot<Bindingslot> : true_type {};


template<typename T>
constexpr bool isBindingSlotV = isBindingSlot<T>::value;

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

template<typename Callable, typename Ret, typename... Params>
struct CallableSignature
{
	using CallableTuple = tuple<Callable, Ret, tuple<Params...>>;

	using CallableType = Callable;
	using RetType = typename CallableInternalTypes<Callable>::RetType;
	using ParamTypeTuple = tuple<Params...>;

	using OriginalSignature = typename CallableInternalTypes<Callable>::OriginalSignature;
	auto getOriginalSignature() { return OriginalSignature{}; }
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

template<size_t ArgIndex, typename ParamTypeTupleT, typename OriginalTypeTupleT, typename ReturnTypeTupleT>
struct BindingSlotTypeChecker;

template<size_t ArgIndex, typename ParamTypeTupleT, typename OriginalTypeTupleT, typename Arg, typename... Args>
struct BindingSlotTypeChecker<ArgIndex, ParamTypeTupleT, OriginalTypeTupleT, tuple<Arg, Args...>>
{
	constexpr static bool check()
	{
		static_assert(ArgIndex < tuple_size_v<ParamTypeTupleT>, "Argument tuple size is wrong.");

		constexpr bool foundBindingSlot = isBindingSlotV<tuple_element_t<ArgIndex, ParamTypeTupleT>>;
		constexpr bool matched = conditional_t<foundBindingSlot,
			is_same<tuple_element_t<ArgIndex, OriginalTypeTupleT>, Arg>,
			true_type>::value;

		static_assert(matched, "Argument type is mismatched.");

		if constexpr (ArgIndex < tuple_size_v<ParamTypeTupleT>)
		{
			if constexpr (foundBindingSlot)
				return BindingSlotTypeChecker<ArgIndex + 1, ParamTypeTupleT, OriginalTypeTupleT, tuple<Args...>>::check();
			else
				return BindingSlotTypeChecker<ArgIndex + 1, ParamTypeTupleT, OriginalTypeTupleT, tuple<Arg, Args...>>::check();
		}
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
	//CallableSignatureT: :ParamTypeTuple* //tuple_element_t<Seqs, ReturnTypeTupleT>;
	using OrderedReturnTypeTupleT = decltype(mapTuple(ReturnTypeTupleT{}, index_sequence<Seqs...>{}));
	//tuple<tuple_element<Seqs..., ReturnTypeTupleT>>::type>;
	CallableInfo()
	{
		BindingSlotTypeChecker<0, typename CallableSignatureT::ParamTypeTuple, typename CallableSignatureT::OriginalSignature::ParamTypeTuple, OrderedReturnTypeTupleT>::check();
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
void testVoidRet(int) {}


void tes2222t()
{
	auto callableSignature1 = makeCallableSignature(testIntRet);
	auto callableSignature2 = makeCallableSignature(testVoidRet);
	auto callableSignature2_binding = makeCallableSignature(testVoidRet, Bindingslot());
	auto callableSignature2_pure = callableSignature2_binding.getOriginalSignature();

	auto callableInfo1 = makeCallableInfo(callableSignature1, callableSignature2);
	auto callableInfo2 = makeCallableInfo(callableSignature1, callableSignature2_binding, index_sequence<0>{});
}