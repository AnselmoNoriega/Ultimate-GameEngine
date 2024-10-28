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
struct Particle
{
	vec2 pos;
	float height;
	float angle;
	float tiltAngle;
	float angleVel;
	float opacity;
	float temp;
};

layout(std430, binding = 16) buffer Particles
{
    Particle data[];
};
//---------------------------------------------------

void main()
{
	if(data[0].height != 0)
	{
		fragColor = vec4(0.0f, 1.0f, 0.0f, 1.0f);
	}
	else
	{
		fragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
	}
	gl_Position = uViewProjectionMatrix  * uRenderer.Transform * vec4(aPosition, 1.0);
}