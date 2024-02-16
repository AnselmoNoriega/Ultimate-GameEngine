#pragma once

#ifdef NR_PLATFORM_WINDOWS
	#ifdef  NR_BUILD_DLL
		#define NR_API __declspec(dllexport)
	#else
		#define NR_API __declspec(dllimport)
	#endif
#else
	#error This Engine only supports Windows.
#endif