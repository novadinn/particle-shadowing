#version 450

layout(location = 0) out vec4 outFragColor;

layout (std430, set = 1, binding = 0) readonly buffer sbShadows
{
	float shadows[];
};

layout(push_constant) uniform PushConstants {
    mat4 model;
    uint shadow_index;
} pushConstants;

void main() { 
    outFragColor = vec4(1.0);
}