#version 450 core
layout(location = 0) in vec3 aPosition;

layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec3 aTangent;
layout(location = 3) in vec3 aBinormal;
layout(location = 4) in vec2 aTexCoord;

// Transform buffer
layout(location = 5) in vec4 aMRow0;
layout(location = 6) in vec4 aMRow1;
layout(location = 7) in vec4 aMRow2;

layout (std140, binding = 0) uniform Camera
{
	mat4 uViewProjection;
};

void main()
{
	mat4 transform = mat4(
		vec4(aMRow0.x, aMRow1.x, aMRow2.x, 0.0),
		vec4(aMRow0.y, aMRow1.y, aMRow2.y, 0.0),
		vec4(aMRow0.z, aMRow1.z, aMRow2.z, 0.0),
		vec4(aMRow0.w, aMRow1.w, aMRow2.w, 1.0)
	);

	gl_Position = uViewProjection * transform * vec4(aPosition, 1.0);
}
