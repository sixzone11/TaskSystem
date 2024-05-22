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

void tes2222t()
{
	struct SampleKey : BindingKey {};
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