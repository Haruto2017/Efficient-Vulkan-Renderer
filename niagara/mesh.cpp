#define TINYOBJLOADER_IMPLEMENTATION

#include "mesh.h"

void Mesh::loadMesh(std::string objpath)
{
	tinyobj::ObjReaderConfig reader_config;
	reader_config.mtl_search_path = "./";

	tinyobj::ObjReader reader;

	if (!reader.ParseFromFile(objpath, reader_config)) {
		if (!reader.Error().empty()) {
			std::cerr << "TinyObjReader: " << reader.Error();
		}
		exit(1);
	}

	if (!reader.Warning().empty()) {
		std::cout << "TinyObjReader: " << reader.Warning();
	}

	auto& attrib = reader.GetAttrib();
	auto& shapes = reader.GetShapes();
	auto& materials = reader.GetMaterials();

    std::atomic<size_t> num_triangles = 0;
    std::for_each(std::execution::par, shapes.begin(), shapes.end(), [&](auto i) {
        size_t curr = i.mesh.num_face_vertices.size();
        num_triangles.fetch_add(curr, std::memory_order_relaxed);
    });

    size_t num_indices = num_triangles * 3;
    std::vector<Vertex> vertices(num_indices);

    // Loop over shapes
    uint32_t curr_tri = 0;
    for (size_t s = 0; s < shapes.size(); s++) {
        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
            size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
            if (fv != 3)
            {
                throw std::runtime_error("Mesh contains triangles with not exactly three vertices");
            }

            // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++) {
                // access to vertex
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
                tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
                tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];
                vertices[(uint64_t)curr_tri * 3 + v].pos.x = vx;
                vertices[(uint64_t)curr_tri * 3 + v].pos.y = vy;
                vertices[(uint64_t)curr_tri * 3 + v].pos.z = vz;

                // Check if `normal_index` is zero or positive. negative = no normal data
                if (idx.normal_index >= 0) {
                    tinyobj::real_t nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
                    tinyobj::real_t ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
                    tinyobj::real_t nz = attrib.normals[3 * size_t(idx.normal_index) + 2];
                    vertices[(uint64_t)curr_tri * 3 + v].nx = uint8_t(nx * 127.f + 127.f);
                    vertices[(uint64_t)curr_tri * 3 + v].ny = uint8_t(ny * 127.f + 127.f);
                    vertices[(uint64_t)curr_tri * 3 + v].nz = uint8_t(nz * 127.f + 127.f);
                }

                // Check if `texcoord_index` is zero or positive. negative = no texcoord data
                if (idx.texcoord_index >= 0) {
                    tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
                    tinyobj::real_t ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
                    vertices[(uint64_t)curr_tri * 3 + v].texcoord.x = tx;
                    vertices[(uint64_t)curr_tri * 3 + v].texcoord.y = ty;
                }

                // Optional: vertex colors
                // tinyobj::real_t red   = attrib.colors[3*size_t(idx.vertex_index)+0];
                // tinyobj::real_t green = attrib.colors[3*size_t(idx.vertex_index)+1];
                // tinyobj::real_t blue  = attrib.colors[3*size_t(idx.vertex_index)+2];
            }
            curr_tri++;
            index_offset += fv;

            // per-face material
            shapes[s].mesh.material_ids[f];
        }
    }

    //m_vertices = vertices;
    //m_indices.resize(num_indices);
    //std::iota(std::begin(m_indices), std::end(m_indices), 0);
    std::vector<uint32_t> remap(num_indices);
    size_t num_unique_vertices = meshopt_generateVertexRemap((unsigned int *)remap.data(), 0, num_indices, vertices.data(), num_indices, sizeof(Vertex));

    m_vertices.resize(num_unique_vertices);
    m_indices.resize(num_indices);

    meshopt_remapVertexBuffer(m_vertices.data(), vertices.data(), num_indices, sizeof(Vertex), remap.data());
    meshopt_remapIndexBuffer(m_indices.data(), 0, num_indices, remap.data());
}

void Mesh::buildMeshlets()
{
    Meshlet meshlet = {};

    std::unordered_map<unsigned int, uint8_t> curr;

    for (size_t i = 0; i < m_indices.size(); i += 3)
    {
        unsigned int a = m_indices[i + 0];
        unsigned int b = m_indices[i + 1];
        unsigned int c = m_indices[i + 2];

        uint8_t av = 0xff;
        if (curr.find(a) != curr.end())
        {
            av = curr[a];
        }
        uint8_t bv = 0xff;
        if (curr.find(b) != curr.end())
        {
            bv = curr[b];
        }
        uint8_t cv = 0xff;
        if (curr.find(c) != curr.end())
        {
            cv = curr[c];
        }

        if (meshlet.vertexCount + (av == 0xff) + (bv == 0xff) + (cv == 0xff))
        {
            m_meshlets.push_back(meshlet);
            meshlet = {};
            curr.clear();
        }

        if (meshlet.indexCount + 3 > 126)
        {
            m_meshlets.push_back(meshlet);
            meshlet = {};
            curr.clear();
        }

        if (av == 0xff)
        {
            curr[a] = meshlet.vertexCount;
            meshlet.vertices[meshlet.vertexCount++] = a;
        }

        if (bv == 0xff)
        {
            curr[b] = meshlet.vertexCount;
            meshlet.vertices[meshlet.vertexCount++] = b;
        }

        if (cv == 0xff)
        {
            curr[c] = meshlet.vertexCount;
            meshlet.vertices[meshlet.vertexCount++] = c;
        }

        meshlet.indices[meshlet.indexCount++] = curr[a];
        meshlet.indices[meshlet.indexCount++] = curr[b];
        meshlet.indices[meshlet.indexCount++] = curr[c];
    }
    if (meshlet.indexCount)
    {
        m_meshlets.push_back(meshlet);
    }
}

void Mesh::generateRenderData(VkDevice device, const VkPhysicalDeviceMemoryProperties& memoryProperties)
{
    if (m_vertices.size() == 0 || m_indices.size() == 0)
    {
        throw std::runtime_error("mesh is not loaded");
    }
    createBuffer(vb, device, memoryProperties, sizeof(m_vertices[0]) * m_vertices.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    createBuffer(ib, device, memoryProperties, sizeof(m_indices[0]) * m_indices.size(), VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
#ifdef RTX
    buildMeshlets();
    createBuffer(mb, device, memoryProperties, sizeof(m_meshlets[0]) * m_meshlets.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    memcpy(mb.data, m_meshlets.data(), sizeof(m_meshlets[0]) * m_meshlets.size());
#endif

    memcpy(vb.data, m_vertices.data(), m_vertices.size() * sizeof(m_vertices[0]));
    memcpy(ib.data, m_indices.data(), m_indices.size() * sizeof(m_indices[0]));
}

void Mesh::destroyRenderData(VkDevice device)
{
    destroyBuffer(vb, device);
    destroyBuffer(ib, device);
    destroyBuffer(mb, device);
}