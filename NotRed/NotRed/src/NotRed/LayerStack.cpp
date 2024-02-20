#include "nrpch.h"
#include "LayerStack.h"

namespace NR
{
    NR::LayerStack::LayerStack()
    {
        mLayerInsert = mLayers.begin();
    }

    NR::LayerStack::~LayerStack()
    {
        for (Layer* layer : mLayers)
        {
            delete layer;
            layer = nullptr;
        }
    }

    void NR::LayerStack::PushLayer(Layer* layer)
    {
        mLayerInsert = mLayers.emplace(mLayerInsert, layer);
    }

    void NR::LayerStack::PushOverlay(Layer* overlay)
    {
        mLayers.emplace_back(overlay);
    }

    void NR::LayerStack::PopLayer(Layer* layer)
    {
        auto it = std::find(mLayers.begin(), mLayers.end(), layer);
        if (it != mLayers.end())
        {
            mLayers.erase(it);
            --mLayerInsert;
        }
    }

    void NR::LayerStack::PopOverlay(Layer* overlay)
    {
        auto it = std::find(mLayers.begin(), mLayers.end(), overlay);
        if (it != mLayers.end())
        {
            mLayers.erase(it);
        }
    }
}
