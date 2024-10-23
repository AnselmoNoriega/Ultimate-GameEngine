#pragma once

#define GLM_ENABLE_EXPERIMENTAL

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#ifdef NR_PLATFORM_WINDOWS
#include <Windows.h>
#endif

#include <algorithm>
#include <array>
#include <fstream>
#include <functional>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include <NotRed/Core/Core.h>
#include <NotRed/Core/Events/Event.h>
#include <NotRed/Core/Log.h>

#include <NotRed/Math/Matrix4x4.h>