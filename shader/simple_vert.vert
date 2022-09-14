#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = vec4(inPosition + vec3(0, 0, 0.5), 1.0);
    fragColor = normalize(inNormal) * 0.5 + 0.5;
}