#version 450

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV0;

layout (set = 0, binding = 0) uniform UBO 
{
	mat4 modelMatrix;
	mat4 viewMatrix;
	mat4 projectionMatrix;
	vec4 depth;
} uboMVP;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outUV0;

out gl_PerVertex 
{
    vec4 gl_Position;   
};

void main() 
{
	outUV0      = vec3(inUV0, uboMVP.depth.x);
	outNormal   = inNormal;
	gl_Position = uboMVP.projectionMatrix * uboMVP.viewMatrix * uboMVP.modelMatrix * vec4(inPosition.xyz, 1.0);
}
