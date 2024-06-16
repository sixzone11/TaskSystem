#include "pch.h"

void entrypoint_test();
void threadguard_test();

int main()
{
	std::cout << "Hello TaskSystem!" << std::endl;

	entrypoint_test();
	threadguard_test();
}