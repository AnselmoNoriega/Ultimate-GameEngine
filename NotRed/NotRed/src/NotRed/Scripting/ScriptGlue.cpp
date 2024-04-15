#include "nrpch.h"
#include "ScriptGlue.h"

#include "mono/metadata/object.h"

namespace NR
{
#define NR_ADD_INTERNAL_CALL(Name) mono_add_internal_call("NR.InternalCalls::" #Name, Name)

	static void NativeLog(MonoString* string, int parameter)
	{
		char* cStr = mono_string_to_utf8(string);
		std::string str(cStr);
		mono_free(cStr);
		std::cout << str << ", " << parameter << std::endl;
	}

    void ScriptGlue::RegisterFunctions()
    {
        NR_ADD_INTERNAL_CALL(NativeLog);
    }
}