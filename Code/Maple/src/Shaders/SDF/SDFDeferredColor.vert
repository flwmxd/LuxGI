//////////////////////////////////////////////////////////////////////////////
// This file is part of the Maple Engine                              		//
//////////////////////////////////////////////////////////////////////////////
#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(set = 0,binding = 0) uniform UniformBufferObject 
{    
	int placeholder;
} ubo;

layout(push_constant) uniform PushConsts
{
	mat4 projView;
	mat4 transform;
} pushConsts;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;


layout(location = 0) out vec4 fragPosition;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec4 fragColor;
layout(location = 3) out vec4 fragNormal;
layout(location = 4) out vec3 fragTangent;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main() 
{
	fragPosition = pushConsts.transform * vec4(inPosition, 1.0);
    vec4 pos =  pushConsts.projView * vec4(inPosition, 1.0);
	fragTexCoord = inTexCoord;
    fragColor = inColor;
    fragNormal.xyz =  transpose(inverse(mat3(pushConsts.transform))) * normalize(inNormal);
    fragTangent =  transpose(inverse(mat3(pushConsts.transform))) *normalize(inTangent);
    gl_Position = pos;
}