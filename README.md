# Efficient-Vulkan-Renderer

* Learning how to write an efficiency-driven Vulkan renderer with [Arseny Kapoulkine's niagara renderer video series](https://youtu.be/BR2my8OE1Sc)

* Implementing an optimized version of object culling done on the GPU

* Serves as an experimental ground for any interesting rendering algorithm


# Implemented Features

## Geometry Rendering Optimization & GPU-based Rendering Pipeline Goals:
    1. Minimize the amount of triangles sent to the vertex shader & (more importantly) rasterization stage
    2. Save the number of draw calls submitted from the CPU side
    3. Memory-cache friendly buffers & utilization

* Mesh rendering with meshlet generation & optimization and mesh shader. 

    ![Mesh_Shader_Correct](images/Simple_Mesh_Front.png)

    * Visualized meshlets

    ![Meshlet_Visualized](images/Meshlet_Visualized.png)

* Meshlet culling

    * Meshlet culling done by cone testing. The idea is learnt from Arseny Kapoulkine and a siggraph sharing from Assassin's Creed: Unity team. The idea is such:
        1. For a meshlet with many triangles, we first compute an average normal and a max degree angle 'a' that a 
        single triangle's normal can be away from the average normal.
        2. Then, a backfacing cone should have an angle betweeen avg normal & view dir less than (-a + 90 degrees) 
        such that even the farthest normal direction from the view dir in the meshlet is backfacing
        3. We store cos value only instead of degree angle. In shader, since the greater the cos value is the less the angle is in between, dot(avg normal, view dir) > cos(-a + 90) means the meshlet is backfacing
        4. Math: cos(-a + 90) = sin(a) = sqrt(1 - cos(a)^2)
        5. Corner case 1: if the meshlet has triangles with normal distributing over a range of over 180 degrees, no matter what the view direction is, we can see some triangles so the meshlet must be kept.
        6. Corner case 2: Perpective projection requires an apex or a bounding sphere to be sent to the shader to compute the correct viewing direction or a conservative cutoff

    * Implementation is done with a combination of task & mesh shader where task shader only dispatches mesh shaders if a meshlet is not culled

    * This image is rendered by enabling meshlet culling and also culling front-facing triangles in rasterization stage to show the result.

    ![Meshlet_Culling](images/Backface_Cone_Culling.png)

    * This image is the result without meshlet culling

    ![Mesh_Shader_Correct](images/Mesh_Shader_Correct.png)

* Indirect draw command

    * Draws a lot of meshes with only one draw call and an additional data buffer. The buffer contains the necessary info to give each mesh its own constants

    ![Mesh_Draw_Indirect](images/Mesh_Draw_Indirect.png)