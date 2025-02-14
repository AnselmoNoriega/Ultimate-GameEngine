#include <nrpch.h>
#include "NodeEditorModel.h"

namespace NR
{
    bool DemoNodeEditorModel::SaveGraphState(const char* data, size_t size)
    {
        mGraphAsset->GraphState = data;
        return true;
    }

    const std::string& DemoNodeEditorModel::LoadGraphState()
    {
        return mGraphAsset->GraphState;
    }

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