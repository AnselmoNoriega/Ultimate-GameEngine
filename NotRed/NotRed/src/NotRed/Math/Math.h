#pragma once

namespace NR::Math 
{
	bool DecomposeTransform(const glm::mat4& transform, glm::vec3& translation, glm::vec3& rotation, glm::vec3& scale);
	glm::mat4 MakeInfReversedZProjRH(const float fovY_radians, const float aspectWbyH, const float zNear);
}