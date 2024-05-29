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


template<size_t>
struct LiteralBindingKey : BindingKey {};

void tes2222t()
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
	}

	{
		auto t1 = makeCallableSignature(testFloatRet);
		auto t2 = makeCallableSignature(test4, t1);
		auto t3 = makeCallableSignature(ProcessBlock()
		{
			float param = GetResult(decltype(t1));
			return param * 4.0f;
		});
		auto t4 = makeCallableSignature(ProcessBlock()
		{
			const float& param = GetResult(decltype(t1));
			const float& param2 = GetResult(decltype(t3));
			return param2 * param * 4.0f;
		});

		auto callableInfo1 = makeCallableInfo(
			t1,
			t2,
			t3,
			t4
		);
	}

#define MakeCallableSignature(KeyID, ...)	makeCallableSignature<LiteralBindingKey<KeyID>>(__VA_ARGS__)
#define GetResultFromID(KeyID)				std::get<find_type_in_tuple<true, LiteralBindingKey<KeyID>, decltype(info)>::value>(resultTuple)
#define BindingKeyID(KeyID)					LiteralBindingKey<KeyID>()

	{
		auto t0 = makeCallableSignature(testIntRet);
		auto t1 = makeCallableSignature(testFloatRet);
		auto t2 = makeCallableSignature(test4, decltype(t1)());
		auto t3 = makeCallableSignature(test3, decltype(t0)(), decltype(t1)());
		auto t4 = makeCallableSignature(testVoidRet, decltype(t0)(), decltype(t1)());
		auto t5 = makeCallableSignature(test5, 3, decltype(t3)());
		auto t6 = makeCallableSignature([](LambdaTaskIdentifier, auto info, auto&& resultTuple)
			{
				AutoBindResult(decltype(t1), param);
				AutoBindResult(decltype(t0), param2);
				AutoBindResult(decltype(t3), param3);
				return param * 4.0f;
			});
		auto t7 = makeCallableSignature(test4, decltype(t6)());

		auto callableInfo2 = makeCallableInfo(
			t0, t1, t2, t3, t4, t5, t6, t7
		);
	}

	{
		auto t1 = MakeCallableSignature(1, testFloatRet);
		auto t2 = MakeCallableSignature(2, test4, BindingKeyID(1));

		auto callableInfo2 = makeCallableInfo(
			MakeCallableSignature(0, testIntRet),
			//MakeCallableSignature(1, testFloatRet),
			t1,
			//MakeCallableSignature(2, test4, BindingKeyID(1)),
			t2,
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
}