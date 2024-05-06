#include "pch.h"

#include "TaskSystem.h"

decltype(std::cout)& FormatMeta<Format::utf8>::cout = std::cout;
decltype(std::wcout)& FormatMeta<Format::utf16>::cout = std::wcout;
