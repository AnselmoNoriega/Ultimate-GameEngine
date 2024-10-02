#version 450 core

layout(location = 0) in vec3 aPosition;
layout(location = 16) out vec4 fragColor; 

layout (std140, binding = 0) uniform Camera
{
	mat4 uViewProjectionMatrix;
	mat4 uInverseViewProjectionMatrix;
	mat4 uViewMatrix;
};

layout (push_constant) uniform Transform
{
	mat4 Transform;
} uRenderer;

//---------------------------------------------------
struct Particle {
    vec4 position;
    vec4 velocity;
    float mass;
    int id;
};

layout(std430, binding = 8) buffer MyDataBuffer {
    Particle data[200];
};
//---------------------------------------------------

void main()
{
	if(data[0].id != 0)
	{
		fragColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
	}
	else
	{
		fragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
	}
	gl_Position = uViewProjectionMatrix  * uRenderer.Transform * vec4(aPosition, 1.0);
}