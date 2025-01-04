#pragma once

#include "NotRed/Core/Core.h"
#include "Layer.h"

#include <vector>

namespace NR
{
	class LayerStack
	{
	public:
		LayerStack();
		~LayerStack();

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);
		void PopLayer(Layer* layer);
		void PopOverlay(Layer* overlay);

		Layer* operator[](size_t index)
		{
			NR_CORE_ASSERT(index >= 0 && index < mLayers.size());
			return mLayers[index];
		}
		const Layer* operator[](size_t index) const
		{
			NR_CORE_ASSERT(index >= 0 && index < mLayers.size());
			return mLayers[index];
		}

		size_t Size() const { return mLayers.size(); }

		std::vector<Layer*>::iterator begin() { return mLayers.begin(); }
		std::vector<Layer*>::iterator end() { return mLayers.end(); }

	private:
		std::vector<Layer*> mLayers;
		uint32_t mLayerInsertIndex = 0;
	};
}