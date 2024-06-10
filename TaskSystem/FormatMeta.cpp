#include "pch.h"

#include "FormatMeta.h"

decltype(std::cout)& FormatMeta<Format::utf8>::cout = std::cout;
decltype(std::wcout)& FormatMeta<Format::utf16>::cout = std::wcout;


//template<Format F, typename T>
//void convertToStringList(const T& var)
//{
//	FormatMeta<F>::cout << "Default" << std::endl;
//}

template<Format F>
void convertToStringList(const int& var)
{
	typename FormatMeta<F>::type out_buffer[256];
	FormatMeta<F>::sprintf(out_buffer, FStr(F, "this is a integer %d"), var);

	//FormatMeta<F>::sprintf(out_buffer, FormatMeta<F>::F("this is a integer: %d"), var);

	FormatMeta<F>::cout << out_buffer << std::endl;
}
template TASKSYSTEM_API void convertToStringList<Format::utf8>(const int& var);
template TASKSYSTEM_API void convertToStringList<Format::utf16>(const int& var);