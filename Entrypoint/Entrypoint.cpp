#include "pch.h"

void entrypoint_test();
void constexpr_str_test();
void threadguard_test();

int main()
{
	std::cout << "Hello TaskSystem!" << std::endl;

	entrypoint_test();
	constexpr_str_test();
	threadguard_test();
}