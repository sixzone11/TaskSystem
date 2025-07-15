#pragma once

#include <stdint.h>
#include <stdio.h>
#include <iostream>

#include <vector>
#include <tuple>
#include <functional>
#include <array>

#if _MSVC_LANG < 202000L || __cplusplus < 202000L
namespace std
{
	template<class T>
	struct remove_cvref
	{
		using type = std::remove_cv_t<std::remove_reference_t<T>>;
	};

	template <class _Ty>
	using remove_cvref_t = typename remove_cvref<_Ty>::type;
}
#endif