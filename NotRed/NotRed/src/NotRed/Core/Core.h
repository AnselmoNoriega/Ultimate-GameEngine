#pragma once

namespace NR
{
	void InitializeCore();
	void ShutdownCore();
}

#ifdef NOT_RED_BUILD_DLL
#define NOT_RED_API __declspec(dllexport)
#else
#define NOT_RED_API __declspec(dllimport)
#endif