#include <nrpch.h>
#include "NodeEditorModel.h"

namespace NR
{
    void DemoNodeEditorModel::OnCompileGraph()
    {
        NR_CORE_INFO("Compiling graph...");

        SaveAll();

        if (onCompile)
        {
            if (onCompile(mGraphAsset.As<Asset>())) 
            {
                NR_CORE_INFO("Graph has been compiled.");
            }
            else                                     
            {
                NR_CORE_ERROR("Failed to compile graph!");
            }
        }
        else
        {
            NR_CORE_ERROR("Failed to compile graph!");
        }
    }
}