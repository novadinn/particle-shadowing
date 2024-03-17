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
    float shadow = max(shadows[pushConstants.shadow_index], 0.25);
    outFragColor = vec4(vec3(shadow), pushConstants.opacity);
}