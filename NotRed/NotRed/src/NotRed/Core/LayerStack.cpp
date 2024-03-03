#include "nrpch.h"
#include "LayerStack.h"

namespace NR
{
    NR::LayerStack::LayerStack()
    {

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
        mLayers.emplace(mLayers.begin() + mLayerInsertIndex, layer);
        ++mLayerInsertIndex;
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
            --mLayerInsertIndex;
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
