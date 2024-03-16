#version 450

layout(location = 0) out vec4 outFragColor;

layout (std430, set = 1, binding = 0) readonly buffer sbShadows
{
	float shadows[];
};

layout(push_constant) uniform PushConstants {
    mat4 model;
    uint shadow_index;
    float opacity;
} pushConstants;

void main() { 
    outFragColor = vec4(vec3(clamp(shadows[pushConstants.shadow_index], 0.5, 1.0)), 1.0);
}