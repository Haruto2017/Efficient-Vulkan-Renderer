#version 460

#extension GL_NV_mesh_shader: require

layout(location = 0) in vec3 fragColor;
// layout(location = 1) perprimitiveNV in vec3 triangleNormal;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(fragColor, 1.0);
}