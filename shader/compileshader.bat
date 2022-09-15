glslc.exe simple_vert.vert -o ../compiledShader/simple_vert.spv
glslc.exe simple_frag.frag -o ../compiledShader/simple_frag.spv
glslc.exe -fshader-stage=mesh meshlet.mesh.glsl -o ../compiledShader/meshlet.mesh.spv
pause