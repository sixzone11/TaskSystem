#include "pch.h"

#include "TaskCallableDescription.h"


///////////////////////////////////////////////////////////////////////
//
// Test code
//
///////////////////////////////////////////////////////////////////////

int testIntRet() { return 0; }
float testFloatRet() { return 2.f; }
void testVoidRet(int, float) {}
double test3(int, float) { return 3.5; }
void test4(float) {}
int test5(int, double) { return 9; }

namespace KeyA {
	struct First : BindingKey {};
	struct Second : BindingKey {};
	struct Third : BindingKey {};
	struct Forth : BindingKey {};
}

void test_callableSignature()
{
	struct SampleKey : BindingKey {};

	{
		auto callableSignature0 = makeCallableSignature<KeyA::First>(testFloatRet);
		auto callableSignature1 = makeCallableSignature<SampleKey>(testIntRet);
		auto callableSignature2 = makeCallableSignature(testVoidRet);
		auto callableSignature2_binding = makeCallableSignature(testVoidRet, SampleKey(), KeyA::First());
		auto callableSignature2_pure = callableSignature2_binding.getOriginalSignature();
		auto callableSignature3_binding = makeCallableSignature<KeyA::Second>(test3, SampleKey(), KeyA::First());
		auto callableSignature4_binding = makeCallableSignature(test4, KeyA::First());
		auto callableSignature5_binding = makeCallableSignature<KeyA::Third>(test5, 3, KeyA::Second());

		auto callableSignature6_binding = makeCallableSignature<KeyA::Forth>([](LambdaTaskIdentifier, auto info, auto&& resultTuple)
			{
				AutoBindResult(KeyA::First, param);
				AutoBindResult(SampleKey, param2);
				AutoBindResult(KeyA::Second, param3);
				return param * 4.0f;
			});

		auto callableSignature7_binding = makeCallableSignature(test4, KeyA::Forth());

		auto callableInfo = makeCallableInfo(
			callableSignature1,
			callableSignature0,
			callableSignature4_binding,
			callableSignature3_binding,
			callableSignature2_binding,
			callableSignature5_binding,
			callableSignature6_binding,
			callableSignature7_binding
		);

		constexpr size_t sizeofReturnTypeTuple = sizeof(typename decltype(callableInfo)::ReturnTypeTuple);

		typename decltype(callableInfo)::ReturnTypeTuple retTuple;
		auto& [v0, v1, v2, v3, v4, v5, v6, v7] = retTuple;
		// auto& [v0, v1, v2, v3, v4, v5, v6] = retTuple; // error: less
		// auto& [v0, v1, v2, v3, v4, v5, v6, v7, v8] = retTuple; // error: more
	}
}

#define GetResultVar(Var)				std::get<find_type_in_tuple<true, typename std::remove_reference_t<decltype(Var)>::KeyType, decltype(info)>::value>(resultTuple)
#define BindingKeyVar(Var)				typename std::remove_reference_t<decltype(Var)>::KeyType()
#define BindResultVar(Key, Var)			Var = GetResultVar(Key)
#define AutoBindResultVar(Key, Var)		auto BindResultVar(Key, Var)

void test_VariableAsBindingKey()
{
	auto t1 = makeCallableSignature(testFloatRet);
	auto t2 = makeCallableSignature(test4, BindingKeyVar(t1));
	auto t3 = makeCallableSignature(ProcessBlock()
	{
		float param = GetResultVar(t1);
		return param * 4.0f;
	});
	auto t4 = makeCallableSignature(ProcessBlock()
	{
		const float& param = GetResultVar(t1);
		const float& param2 = GetResultVar(t3);
		return param2 * param * 4.0f;
	});

	auto callableInfo1 = makeCallableInfo(
		t1,
		t2,
		t3,
		t4
	);
	
}

void testSignature()
{
	
	auto t0 = makeCallableSignature(testIntRet);
	auto t1 = makeCallableSignature(testFloatRet);
	auto t2 = makeCallableSignature(test4, BindingKeyVar(t1));
	auto t3 = makeCallableSignature(test3, BindingKeyVar(t0), BindingKeyVar(t1));
	auto t4 = makeCallableSignature(testVoidRet, BindingKeyVar(t0), BindingKeyVar(t1));
	auto t5 = makeCallableSignature(test5, 3, BindingKeyVar(t3));
	auto t6 = makeCallableSignature([](LambdaTaskIdentifier, auto info, auto&& resultTuple)
		{
			AutoBindResultVar(t1, param);
			AutoBindResultVar(t0, param2);
			AutoBindResultVar(t3, param3);
			return param * 4.0f;
		});
	auto t7 = makeCallableSignature(test4, BindingKeyVar(t6));

	auto callableInfo2 = makeCallableInfo(
		t0, t1, t2, t3, t4, t5, t6, t7
	);
	
}


template<size_t>
struct LiteralBindingKey : BindingKey {};

void testIntegralSignature()
{
#define MakeCallableSignature(KeyID, ...)	makeCallableSignature<LiteralBindingKey<KeyID>>(__VA_ARGS__)
#define GetResultFromID(KeyID)				std::get<find_type_in_tuple<true, LiteralBindingKey<KeyID>, decltype(info)>::value>(resultTuple)
#define BindingKeyID(KeyID)					LiteralBindingKey<KeyID>()
	
	auto t1 = MakeCallableSignature(1, testFloatRet);
	auto t2 = MakeCallableSignature(2, test4, BindingKeyID(1));

	auto callableInfo2 = makeCallableInfo(
		MakeCallableSignature(0, testIntRet),			
		t1, //MakeCallableSignature(1, testFloatRet),
		t2, //MakeCallableSignature(2, test4, BindingKeyID(1)),
		MakeCallableSignature(3, test3, BindingKeyID(0), BindingKeyID(1)),
		MakeCallableSignature(4, testVoidRet, BindingKeyID(0), BindingKeyID(1)),
		MakeCallableSignature(5, test5, 3, BindingKeyID(3)),
		MakeCallableSignature(6, [](LambdaTaskIdentifier, auto info, auto&& resultTuple)
			{
				auto param = GetResultFromID(1);
				auto param2 = GetResultFromID(0);
				auto param3 = GetResultFromID(3);
				return param * 4.0f;
			}),
		MakeCallableSignature(7, test4, BindingKeyID(6))
	);
	
}

template<const char ... chars>
struct CharsBindingKey : BindingKey {};

template<const char... CharPack>
constexpr auto operator "" _chars()
{
	return CharsBindingKey<CharPack...>{};
}

void testCharPackSignature()
{
#define TriConCat(x, y, z)						x##y##z
#define MakeCallableSignatureCP(CharPack, ...)	makeCallableSignature<decltype(TriConCat(0x, CharPack, _chars))>(__VA_ARGS__)
#define GetResultFromCP(CharPack)				std::get<find_type_in_tuple<true, decltype(TriConCat(0x, CharPack, _chars)), decltype(info)>::value>(resultTuple)
#define BindingKeyCP(CharPack)					decltype(TriConCat(0x, CharPack, _chars))()

	auto t1 = MakeCallableSignatureCP(1a2bc31dfe3489, testFloatRet);
	auto t2 = MakeCallableSignatureCP(eeeeeeee, test4, BindingKeyCP(1a2bc31dfe3489));

	auto callableInfo2 = makeCallableInfo(
		MakeCallableSignatureCP(abc0, testIntRet),
		t1, //MakeCallableSignatureCP(1a2bc31dfe3489, testFloatRet),
		t2, //MakeCallableSignatureCP(eeeeeeee, test4, BindingKeyCP(1a2bc31dfe3489)),
		MakeCallableSignatureCP(3edf, test3, BindingKeyCP(abc0), BindingKeyCP(1a2bc31dfe3489)),
		MakeCallableSignatureCP(6345634564, testVoidRet, BindingKeyCP(abc0), BindingKeyCP(1a2bc31dfe3489)),
		MakeCallableSignatureCP(ffdefe5, test5, 3, BindingKeyCP(3edf)),
		MakeCallableSignatureCP(aadefbd6, [](LambdaTaskIdentifier, auto info, auto&& resultTuple)
			{
				auto param = GetResultFromCP(1a2bc31dfe3489);
				auto param2 = GetResultFromCP(abc0);
				auto param3 = GetResultFromCP(3edf);
				return param * 4.0f;
			}),
		MakeCallableSignatureCP(def7, test4, BindingKeyCP(aadefbd6))
	);
}

#if _MSVC_LANG > 202000L || __cplusplus > 202000L

template <typename CharT, std::size_t N>
struct basic_fixed_string
{
	constexpr basic_fixed_string(const CharT(&foo)[N])
	{
		std::copy_n(foo, N, m_data);
	}
	auto operator<=>(const basic_fixed_string &) const = default;
	CharT m_data[N];
};

template<basic_fixed_string>
struct StringBindingKey : BindingKey {};

void testStringSignature()
{
#define MakeCallableSignatureStr(KeyStr, ...)	makeCallableSignature<StringBindingKey<#KeyStr>>(__VA_ARGS__)
#define GetResultFromStr(KeyStr)				std::get<find_type_in_tuple<true, StringBindingKey<#KeyStr>, decltype(info)>::value>(resultTuple)
#define BindingKeyStr(KeyStr)					StringBindingKey<#KeyStr>()
	
	auto t1 = MakeCallableSignatureStr(process_1, testFloatRet);
	auto t2 = MakeCallableSignatureStr(process_2, test4, BindingKeyStr(process_1));

	static_assert(is_same_v<decltype(BindingKeyVar(t1)), decltype(BindingKeyStr(process_1))>, "BindingKey(t1) and BindinigKey(process_1) is same");

	auto callableInfo2 = makeCallableInfo(
		MakeCallableSignatureStr(process_0, testIntRet),
		t1, //MakeCallableSignatureStr(process_1, testFloatRet),
		t2, //MakeCallableSignatureStr(process_2, test4, BindingKeyStr(process_1)),
		MakeCallableSignatureStr(process_3, test3, BindingKeyStr(process_0), BindingKeyStr(process_1)),
		MakeCallableSignatureStr(process_4, testVoidRet, BindingKeyStr(process_0), BindingKeyStr(process_1)),
		MakeCallableSignatureStr(process_5, test5, 3, BindingKeyStr(process_3)),
		MakeCallableSignatureStr(process_6, [](LambdaTaskIdentifier, auto info, auto&& resultTuple)
			{
				auto param = GetResultFromStr(process_1);
				auto param2 = GetResultFromStr(process_0);
				auto param3 = GetResultFromStr(process_3);
				return param * 4.0f;
			}),
		MakeCallableSignatureStr(process_7, test4, BindingKeyStr(process_6))
	);
	
}

#endif