#version 450 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

layout (std140, binding = 0) uniform Camera
{
	mat4 uViewProjectionMatrix;
	mat4 uInverseViewProjection;
};

#ifdef OPENGL
layout (std140, binding = 1) uniform Transform {
	mat4 Transform;
} uRenderer;
#else
layout (push_constant) uniform Transform
{
	mat4 Transform;
} uRenderer;
#endif

layout (location = 0) out vec2 vTexCoord;

void main()
{
	vec4 position = uViewProjectionMatrix * uRenderer.Transform * vec4(aPosition, 1.0);
	gl_Position = position;

	vTexCoord = aTexCoord;
}