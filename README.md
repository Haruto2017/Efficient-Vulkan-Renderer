# Efficient-Vulkan-Renderer

* Learning how to write an efficiency-driven Vulkan renderer with [Arseny Kapoulkine's niagara renderer video series](https://youtu.be/BR2my8OE1Sc)

* Implementing an optimized version of object culling done on the GPU

* Serves as an experimental ground for any interesting rendering algorithm


# Implemented Features

* Mesh rendering with meshlet generation & optimization and mesh shader. 

    ![Mesh_Shader_Correct](images/Simple_Mesh_Front.png)

    * Visualized meshlets

    ![Meshlet_Visualized](images/Meshlet_Visualized.png)

* Meshlet culling

    * Meshlet culling done by cone testing. The idea is originated from a siggraph sharing from Assassin's Creed: Unity team. The idea is such:
        1. For a meshlet with many triangles, we first compute an average normal and a max degree angle 'a' that a 
        single triangle's normal can be away from the average normal.
        2. Then, a backfacing cone should have an angle betweeen avg normal & view dir less than (-a + 90 degrees) 
        such that even the farthest normal direction from the view dir in the meshlet is backfacing
        3. We store cos value only instead of degree angle. In shader, since the greater the cos value is the less the angle is in between, dot(avg normal, view dir) > cos(-a + 90) means the meshlet is backfacing
        4. Math: cos(-a + 90) = sin(a) = sqrt(1 - cos(a)^2)
        5. Corner case: if the meshlet has triangles with normal distributing over a range of over 180 degrees, no matter what the view direction is, we can see some triangles so the meshlet must be kept.

    * Implementation is done with a combination of task & mesh shader where task shader only dispatches mesh shaders if a meshlet is not culled

    * This image is rendered by enabling meshlet culling and also culling front-facing triangles in rasterization stage to show the result.

    ![Meshlet_Culling](images/Backface_Cone_Culling.png)

    * This image is the result without meshlet culling

    ![Mesh_Shader_Correct](images/Mesh_Shader_Correct.png)