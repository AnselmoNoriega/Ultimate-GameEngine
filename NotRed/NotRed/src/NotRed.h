#pragma once

#include "NotRed/Core/Application.h"
#include "NotRed/Core/Input.h"
#include "NotRed/Core/Log.h"

#include "NotRed/Core/TimeFrame.h"
#include "NotRed/Core/Timer.h"

#include "NotRed/Core/Events/Event.h"
#include "NotRed/Core/Events/ApplicationEvent.h"
#include "NotRed/Core/Events/MouseEvent.h"
#include "NotRed/Core/Events/KeyEvent.h"

#include "NotRed/Math/AABB.h"
#include "NotRed/Math/Ray.h"

#include "imgui/imgui.h"

// --- NotRed Render API -----------------------
#include "NotRed/Renderer/Camera.h"
#include "NotRed/Renderer/FrameBuffer.h"
#include "NotRed/Renderer/IndexBuffer.h"
#include "NotRed/Renderer/Material.h"
#include "NotRed/Renderer/Mesh.h"
#include "NotRed/Renderer/Pipeline.h"
#include "NotRed/Renderer/Renderer.h"
#include "NotRed/Renderer/RenderPass.h"
#include "NotRed/Renderer/SceneRenderer.h"
#include "NotRed/Renderer/Shader.h"
#include "NotRed/Renderer/Texture.h"
#include "NotRed/Renderer/VertexBuffer.h"
// ---------------------------------------------

// Scenes----------------------------------
#include "NotRed/Scene/Entity.h"
#include "NotRed/Scene/Scene.h"
#include "NotRed/Scene/SceneCamera.h"
#include "NotRed/Scene/SceneSerializer.h"
#include "NotRed/Scene/Components.h"