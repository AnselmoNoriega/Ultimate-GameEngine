#include "nrpch.h"
#include "Camera.h"

namespace NR
{
	Camera::Camera(const glm::mat4& projectionMatrix)
		: mProjection(projectionMatrix)
	{

	}
}