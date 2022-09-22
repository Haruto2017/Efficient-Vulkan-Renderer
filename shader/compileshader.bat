glslc.exe --target-env=vulkan1.3 -fshader-stage=vert simple.vert.glsl -o ../compiledShader/simple.vert.spv
glslc.exe --target-env=vulkan1.3 -fshader-stage=frag simple.frag.glsl -o ../compiledShader/simple.frag.spv
glslc.exe --target-env=vulkan1.3 -fshader-stage=mesh meshlet.mesh.glsl -o ../compiledShader/meshlet.mesh.spv
glslc.exe --target-env=vulkan1.3 -fshader-stage=task meshlet.task.glsl -o ../compiledShader/meshlet.task.spv
pause