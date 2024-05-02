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
}

void tes2222t()
{
	struct SampleKey : BindingKey {};
	auto callableSignature0 = makeCallableSignature<KeyA::First>(testFloatRet);
	auto callableSignature1 = makeCallableSignature<SampleKey>(testIntRet);
	auto callableSignature2 = makeCallableSignature(testVoidRet);
	auto callableSignature2_binding = makeCallableSignature(testVoidRet, BindingSlot(), BindingSlot());
	auto callableSignature2_pure = callableSignature2_binding.getOriginalSignature();
	auto callableSignature3_binding = makeCallableSignature<KeyA::Second>(test3, BindingSlot(), BindingSlot());
	auto callableSignature4_binding = makeCallableSignature(test4, BindingSlot());
	auto callableSignature5_binding = makeCallableSignature<KeyA::Third>(test5, 3, BindingSlot());

	auto callableInfo1 = makeCallableInfo(
		callableSignature0,
		callableSignature1,
		callableSignature2_binding, index_sequence<1, 0>{});


	auto callableInfo2 = makeCallableInfo(
		callableSignature0,
		callableSignature1,
		makeCallableSignature(testVoidRet, BindingSlot(), 5.f), index_sequence<1>{});


	auto callableInfo3 = makeCallableInfo(
		callableSignature1,
		callableSignature0,
		callableSignature4_binding, BindingKeys(KeyA::First),
		callableSignature3_binding, BindingKeys(SampleKey, KeyA::First),
		callableSignature2_binding, BindingKeys(SampleKey, KeyA::First),
		callableSignature5_binding, BindingKeys(KeyA::Second));
}