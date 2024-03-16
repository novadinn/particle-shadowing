#version 450

layout(location = 0) in vec3 inPosition;

layout(set = 0, binding = 0) uniform GlobalUBO {
    mat4 projection;
    mat4 view;
} globalUBO;

layout(push_constant) uniform PushConstants {
    mat4 model;
    uint shadow_index;
} pushConstants;

void main() {
    gl_Position = globalUBO.projection * globalUBO.view * pushConstants.model * vec4(inPosition, 1.0);
}