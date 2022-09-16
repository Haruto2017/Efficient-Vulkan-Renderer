#version 460

#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require
#extension GL_EXT_shader_explicit_arithmetic_types: require

layout(location = 0) in vec3 inPosition;
layout(location = 1) in uvec4 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;

void main() {
    vec3 inNormal = vec3(inNormal.xyz) / 127.0 - 1.0;

    gl_Position = vec4(inPosition + vec3(0, 0, 0.5), 1.0);
    fragColor = normalize(inNormal) * 0.5 + 0.5;
}