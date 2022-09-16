glslc.exe -fshader-stage=vert meshfvf.vert.glsl -o ../compiledShader/meshfvf.vert.spv
glslc.exe -fshader-stage=vert simple.vert.glsl -o ../compiledShader/simple.vert.spv
glslc.exe -fshader-stage=frag simple.frag.glsl -o ../compiledShader/simple.frag.spv
glslc.exe -fshader-stage=mesh meshlet.mesh.glsl -o ../compiledShader/meshlet.mesh.spv
pause