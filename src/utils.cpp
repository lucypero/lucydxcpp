#define HR(a) {HRESULT __hres = (a); assert(__hres == 0);}

enum LucyResult {
    LRES_OK = 0,
    LRES_FAIL = 1
};

struct ProgramState {
    void *base_mem;
    u64 total_mem_size;
};

struct Arena {
    u8 *buf;
    u64 offset;//arena offset pointer. it offsets to the first free byte. starts at 0
    u64 size;
};

struct Buf {
    void *buf;
    u64 size;
};

// lucy's assert
// stops execution if false as if it is a debugger breakpoint
fn void lassert() {

}

fn void log(const char *format, ...) {
    va_list argp;
    char buf[200];
    va_start(argp, format);
    i32 written = vsprintf_s(buf, 200, format, argp);
    va_end(argp);
    buf[written] = '\n';
    buf[written + 1] = '\0';
    OutputDebugStringA(buf);
}

fn void *arena_push(Arena *arena, u64 size) {
    void *ret_buf = arena->buf + arena->offset;

    arena->offset += size;

    // checking that u have enough mem in the arena
    assert(arena->offset <= arena->size);

    return ret_buf;
}

fn void arena_clear(Arena *arena) {
    arena->offset = 0;
}

fn void arena_zero(Arena *arena) {
    arena->offset = 0;
    memset(arena->buf, 0, arena->size);
}

fn u64 arena_save(Arena *arena) {
    return arena->offset;
}

fn void arena_restore(Arena *arena, u64 checkpoint) {
    arena->offset = checkpoint;
}

//this allocs a buffer w the file contents
fn LucyResult read_whole_file(Arena *arena, const char *file_path, Buf *out_buf) {
    HANDLE hTextFile = CreateFileA(file_path, GENERIC_READ, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    LucyResult ret = LRES_OK;

    if (hTextFile == INVALID_HANDLE_VALUE) {
        return LRES_FAIL;
    }

    DWORD dwFileSize = GetFileSize(hTextFile, &dwFileSize);

    if (dwFileSize == 0) {
        ret = LRES_FAIL;
    }

    DWORD dwBytesRead;
    void *file_buffer = arena_push(arena, dwFileSize);
    BOOL res = ReadFile(hTextFile, file_buffer, dwFileSize, &dwBytesRead, NULL);

    if (res == 0) {
        ret = LRES_FAIL;
    }

    CloseHandle(hTextFile);

    out_buf->buf = file_buffer;
    out_buf->size = (int) dwFileSize;

    return ret;
}

namespace GeometryGenerator {

    struct Vertex {
        XMFLOAT3 Position;
        XMFLOAT3 Normal;
        XMFLOAT3 tangent_u;
        XMFLOAT3 tex_c;
    };

    struct MeshData {
        std::vector<Vertex> Vertices;
        std::vector<u32> Indices;
    };

    Vertex vertex_new( float px, float py, float pz,
            float nx, float ny, float nz,
            float tx, float ty, float tz,
            float u, float v) {

            return Vertex {
                    .Position = XMFLOAT3(px, py, pz),
                    .Normal = XMFLOAT3(nx,ny,nz),
                    .tangent_u = XMFLOAT3(tx, ty, tz),
                    .tex_c = XMFLOAT3(u,v, 1.0)
            };
    }

    // m = amount of vertices along the x axis
    // n = amount of vertices along the z axis
    // total vertices: m * n
    void create_grid(f32 width, f32 depth, u32 m, u32 n, MeshData *mesh_data) {

        u32 vertex_count = m * n;
        // total amount of triangles
        u32 face_count = (m - 1) * (n - 1) * 2;

        //creating the vertices
        f32 half_width = 0.5f * width;
        f32 half_depth = 0.5f * depth;
        f32 dx = width / (n - 1);
        f32 dz = depth / (m - 1);
        f32 du = 1.0f / (n - 1);
        f32 dv = 1.0f / (m - 1);

        mesh_data->Vertices.resize(vertex_count);
        for (u32 i = 0; i < m; ++i) {
            float z = half_depth - i * dz;
            for (u32 j = 0; j < n; ++j) {
                float x = -half_width + j * dx;
                mesh_data->Vertices[i * n + j].Position = XMFLOAT3(x, 0.0f, z);
                // Ignore for now, used for lighting.
                mesh_data->Vertices[i * n + j].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
                mesh_data->Vertices[i * n + j].tangent_u = XMFLOAT3(1.0f, 0.0f, 0.0f);
                // Ignore for now, used for texturing.
                mesh_data->Vertices[i * n + j].tex_c.x = j * du;
                mesh_data->Vertices[i * n + j].tex_c.y = i * dv;
            }
        }

        mesh_data->Indices.resize(face_count * 3);// 3 indices per face
        // Iterate over each quad and compute indices.
        u32 k = 0;
        for (u32 i = 0; i < m - 1; ++i) {
            for (u32 j = 0; j < n - 1; ++j) {
                mesh_data->Indices[k] = i * n + j;
                mesh_data->Indices[k + 1] = i * n + j + 1;
                mesh_data->Indices[k + 2] = (i + 1) * n + j;
                mesh_data->Indices[k + 3] = (i + 1) * n + j;
                mesh_data->Indices[k + 4] = i * n + j + 1;
                mesh_data->Indices[k + 5] = (i + 1) * n + j + 1;
                k += 6;// next quad
            }
        }
    }

    void build_cylinder_top_cap(f32 bottom_radius, f32 top_radius, f32 height, u32 slice_count, u32 stack_count, MeshData *mesh_data) {

        u32 baseIndex = (u32) mesh_data->Vertices.size();
        f32 y = 0.5f * height;
        f32 dTheta = 2.0f * XM_PI / slice_count;
        // Duplicate cap ring vertices because the texture coordinates
        // and normals differ.
        for (u32 i = 0; i <= slice_count; ++i) {
            f32 x = top_radius * cosf(i * dTheta);
            f32 z = top_radius * sinf(i * dTheta);
            // Scale down by the height to try and make top cap
            // texture coord area proportional to base.
            f32 u = x / height + 0.5f;
            f32 v = z / height + 0.5f;
            mesh_data->Vertices.push_back(Vertex{
                    .Position = XMFLOAT3(x, y, z),
                    .Normal = XMFLOAT3(0.0f, 1.0f, 0.0f),
                    .tangent_u = XMFLOAT3(1.0f, 0.0f, 0.0f),
                    .tex_c = XMFLOAT3(u, v, 1.0f),
            });
        }
        // Cap center vertex.
        mesh_data->Vertices.push_back(Vertex{
                .Position = XMFLOAT3(0.0f, y, 0.0f),
                .Normal = XMFLOAT3(0.0f, 1.0f, 0.0f),
                .tangent_u = XMFLOAT3(1.0f, 0.0f, 0.0f),
                .tex_c = XMFLOAT3(0.5f, 0.5f, 1.0f),
        });
        // Index of center vertex.
        u32 centerIndex = (u32) mesh_data->Vertices.size() - 1;
        for (u32 i = 0; i < slice_count; ++i) {
            mesh_data->Indices.push_back(centerIndex);
            mesh_data->Indices.push_back(baseIndex + i + 1);
            mesh_data->Indices.push_back(baseIndex + i);
        }
    }

    void build_cylinder_bottom_cap(f32 bottom_radius, f32 top_radius, f32 height, u32 slice_count, u32 stack_count, MeshData *mesh_data) {

        u32 baseIndex = (u32) mesh_data->Vertices.size();
        f32 y = -0.5f * height;
        f32 dTheta = 2.0f * XM_PI / slice_count;
        // Duplicate cap ring vertices because the texture coordinates
        // and normals differ.
        for (u32 i = 0; i <= slice_count; ++i) {
            f32 x = bottom_radius * cosf(i * dTheta);
            f32 z = bottom_radius * sinf(i * dTheta);
            // Scale down by the height to try and make top cap
            // texture coord area proportional to base.
            f32 u = x / height + 0.5f;
            f32 v = z / height + 0.5f;
            mesh_data->Vertices.push_back(Vertex{
                    .Position = XMFLOAT3(x, y, z),
                    .Normal = XMFLOAT3(0.0f, -1.0f, 0.0f),
                    .tangent_u = XMFLOAT3(1.0f, 0.0f, 0.0f),
                    .tex_c = XMFLOAT3(u, v, 1.0f),
            });
        }
        // Cap center vertex.
        mesh_data->Vertices.push_back(Vertex{
                .Position = XMFLOAT3(0.0f, y, 0.0f),
                .Normal = XMFLOAT3(0.0f, -1.0f, 0.0f),
                .tangent_u = XMFLOAT3(1.0f, 0.0f, 0.0f),
                .tex_c = XMFLOAT3(0.5f, 0.5f, 1.0f),
        });
        // Index of center vertex.
        u32 centerIndex = (u32) mesh_data->Vertices.size() - 1;
        for (u32 i = 0; i < slice_count; ++i) {
            mesh_data->Indices.push_back(centerIndex);
            mesh_data->Indices.push_back(baseIndex + i + 1);
            mesh_data->Indices.push_back(baseIndex + i);
        }
    }

    void create_cylinder(f32 bottom_radius, f32 top_radius, f32 height, u32 slice_count, u32 stack_count, MeshData *mesh_data) {

        mesh_data->Vertices.clear();
        mesh_data->Indices.clear();

        //
        // Build Stacks.
        //

        f32 stackHeight = height / stack_count;
        // Amount to increment radius as we move up each stack
        // level from bottom to top.
        f32 radiusStep = (top_radius - bottom_radius) / stack_count;
        u32 ringCount = stack_count + 1;

        // Compute vertices for each stack ring starting at
        // the bottom and moving up.

        for (u32 i = 0; i < ringCount; ++i) {
            f32 y = -0.5f * height + i * stackHeight;
            f32 r = bottom_radius + i * radiusStep;
            // vertices of ring
            f32 dTheta = 2.0f * XM_PI / slice_count;

            for (u32 j = 0; j <= slice_count; ++j) {
                Vertex vertex;
                f32 c = cosf(j * dTheta);
                f32 s = sinf(j * dTheta);
                vertex.Position = XMFLOAT3(r * c, y, r * s);
                vertex.tex_c.x = (f32) j / slice_count;
                vertex.tex_c.y = 1.0f - (f32) i / stack_count;
                // Cylinder can be parameterized as follows, where we
                // introduce v parameter that goes in the same direction
                // as the v tex-coord so that the bitangent goes in the
                // same direction as the v tex-coord.
                // Let r0 be the bottom radius and let r1 be the
                // top radius.
                // y(v) = h - hv for v in [0,1].
                // r(v) = r1 + (r0-r1)v
                //
                // x(t, v) = r(v)*cos(t)
                // y(t, v) = h - hv
                // z(t, v) = r(v)*sin(t)
                //
                // dx/dt = -r(v)*sin(t)
                // dy/dt = 0
                // dz/dt = +r(v)*cos(t)
                //
                // dx/dv = (r0-r1)*cos(t)
                // dy/dv = -h
                // dz/dv = (r0-r1)*sin(t)
                // tangent_u us unit length.
                vertex.tangent_u = XMFLOAT3(-s, 0.0f, c);
                float dr = bottom_radius - top_radius;
                XMFLOAT3 bitangent(dr * c, -height, dr * s);
                XMVECTOR T = XMLoadFloat3(&vertex.tangent_u);
                XMVECTOR B = XMLoadFloat3(&bitangent);
                XMVECTOR N = XMVector3Normalize(XMVector3Cross(T, B));
                XMStoreFloat3(&vertex.Normal, N);
                mesh_data->Vertices.push_back(vertex);
            }
        }

        // generating indices

        // Add one because we duplicate the first and last vertex per ring
        // since the texture coordinates are different.
        UINT ringVertexCount = slice_count + 1;
        // Compute indices for each stack.
        for (UINT i = 0; i < stack_count; ++i) {
            for (UINT j = 0; j < slice_count; ++j) {
                mesh_data->Indices.push_back(i * ringVertexCount + j);
                mesh_data->Indices.push_back((i + 1) * ringVertexCount + j);
                mesh_data->Indices.push_back((i + 1) * ringVertexCount + j + 1);
                mesh_data->Indices.push_back(i * ringVertexCount + j);
                mesh_data->Indices.push_back((i + 1) * ringVertexCount + j + 1);
                mesh_data->Indices.push_back(i * ringVertexCount + j + 1);
            }
        }
        build_cylinder_top_cap(bottom_radius, top_radius, height, slice_count, stack_count, mesh_data);
        build_cylinder_bottom_cap(bottom_radius, top_radius, height, slice_count, stack_count, mesh_data);
    }

    void subdivide(MeshData *mesh_data)
    {
        // Save a copy of the input geometry.
        MeshData input_copy = *mesh_data;

        mesh_data->Vertices.resize(0);
        mesh_data->Indices.resize(0);

        //       v1
        //       *
        //      / \
	    //     /   \
	    //  m0*-----*m1
        //   / \   / \
	    //  /   \ /   \
	    // *-----*-----*
        // v0    m2     v2

        u32 numTris = (u32)input_copy.Indices.size()/3u;
        for(u32 i = 0; i < numTris; ++i)
        {
            Vertex v0 = input_copy.Vertices[input_copy.Indices[i*3+0] ];
            Vertex v1 = input_copy.Vertices[input_copy.Indices[i*3+1] ];
            Vertex v2 = input_copy.Vertices[input_copy.Indices[i*3+2] ];

            //
            // Generate the midpoints.
            //

            Vertex m0, m1, m2;

            // For subdivision, we just care about the position component.  We derive the other
            // vertex components in CreateGeosphere.

            m0.Position = XMFLOAT3(
                    0.5f*(v0.Position.x + v1.Position.x),
                    0.5f*(v0.Position.y + v1.Position.y),
                    0.5f*(v0.Position.z + v1.Position.z));

            m1.Position = XMFLOAT3(
                    0.5f*(v1.Position.x + v2.Position.x),
                    0.5f*(v1.Position.y + v2.Position.y),
                    0.5f*(v1.Position.z + v2.Position.z));

            m2.Position = XMFLOAT3(
                    0.5f*(v0.Position.x + v2.Position.x),
                    0.5f*(v0.Position.y + v2.Position.y),
                    0.5f*(v0.Position.z + v2.Position.z));

            //
            // Add new geometry.
            //

            mesh_data->Vertices.push_back(v0); // 0
            mesh_data->Vertices.push_back(v1); // 1
            mesh_data->Vertices.push_back(v2); // 2
            mesh_data->Vertices.push_back(m0); // 3
            mesh_data->Vertices.push_back(m1); // 4
            mesh_data->Vertices.push_back(m2); // 5

            mesh_data->Indices.push_back(i*6+0);
            mesh_data->Indices.push_back(i*6+3);
            mesh_data->Indices.push_back(i*6+5);

            mesh_data->Indices.push_back(i*6+3);
            mesh_data->Indices.push_back(i*6+4);
            mesh_data->Indices.push_back(i*6+5);

            mesh_data->Indices.push_back(i*6+5);
            mesh_data->Indices.push_back(i*6+4);
            mesh_data->Indices.push_back(i*6+2);

            mesh_data->Indices.push_back(i*6+3);
            mesh_data->Indices.push_back(i*6+1);
            mesh_data->Indices.push_back(i*6+4);
        }
    }

    void create_box(float width, float height, float depth, MeshData *meshData) {
        //
        // Create the vertices.
        //

        Vertex v[24];

        float w2 = 0.5f*width;
        float h2 = 0.5f*height;
        float d2 = 0.5f*depth;

        // Fill in the front face vertex data.
        v[0] = vertex_new(-w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
        v[1] = vertex_new(-w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        v[2] = vertex_new(+w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
        v[3] = vertex_new(+w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

        // Fill in the back face vertex data.
        v[4] = vertex_new(-w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
        v[5] = vertex_new(+w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
        v[6] = vertex_new(+w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        v[7] = vertex_new(-w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

        // Fill in the top face vertex data.
        v[8]  = vertex_new(-w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
        v[9]  = vertex_new(-w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        v[10] = vertex_new(+w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
        v[11] = vertex_new(+w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

        // Fill in the bottom face vertex data.
        v[12] = vertex_new(-w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
        v[13] = vertex_new(+w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
        v[14] = vertex_new(+w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        v[15] = vertex_new(-w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

        // Fill in the left face vertex data.
        v[16] = vertex_new(-w2, -h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
        v[17] = vertex_new(-w2, +h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
        v[18] = vertex_new(-w2, +h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
        v[19] = vertex_new(-w2, -h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

        // Fill in the right face vertex data.
        v[20] = vertex_new(+w2, -h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
        v[21] = vertex_new(+w2, +h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
        v[22] = vertex_new(+w2, +h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
        v[23] = vertex_new(+w2, -h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

        meshData->Vertices.assign(&v[0], &v[24]);

        //
        // Create the indices.
        //

        UINT i[36];

        // Fill in the front face index data
        i[0] = 0; i[1] = 1; i[2] = 2;
        i[3] = 0; i[4] = 2; i[5] = 3;

        // Fill in the back face index data
        i[6] = 4; i[7]  = 5; i[8]  = 6;
        i[9] = 4; i[10] = 6; i[11] = 7;

        // Fill in the top face index data
        i[12] = 8; i[13] =  9; i[14] = 10;
        i[15] = 8; i[16] = 10; i[17] = 11;

        // Fill in the bottom face index data
        i[18] = 12; i[19] = 13; i[20] = 14;
        i[21] = 12; i[22] = 14; i[23] = 15;

        // Fill in the left face index data
        i[24] = 16; i[25] = 17; i[26] = 18;
        i[27] = 16; i[28] = 18; i[29] = 19;

        // Fill in the right face index data
        i[30] = 20; i[31] = 21; i[32] = 22;
        i[33] = 20; i[34] = 22; i[35] = 23;

        meshData->Indices.assign(&i[0], &i[36]);
    }

    void create_sphere(float radius, UINT sliceCount, UINT stackCount, MeshData *mesh_data)
    {
        mesh_data->Vertices.clear();
        mesh_data->Indices.clear();

        //
        // Compute the vertices stating at the top pole and moving down the stacks.
        //

        // Poles: note that there will be texture coordinate distortion as there is
        // not a unique point on the texture map to assign to the pole when mapping
        // a rectangular texture onto a sphere.
        Vertex topVertex = vertex_new(0.0f, +radius, 0.0f, 0.0f, +1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        Vertex bottomVertex = vertex_new(0.0f, -radius, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

        mesh_data->Vertices.push_back( topVertex );

        float phiStep   = XM_PI/stackCount;
        float thetaStep = 2.0f*XM_PI/sliceCount;

        // Compute vertices for each stack ring (do not count the poles as rings).
        for(UINT i = 1; i <= stackCount-1; ++i)
        {
            float phi = i*phiStep;

            // Vertices of ring.
            for(UINT j = 0; j <= sliceCount; ++j)
            {
                float theta = j*thetaStep;

                Vertex v;

                // spherical to cartesian
                v.Position.x = radius*sinf(phi)*cosf(theta);
                v.Position.y = radius*cosf(phi);
                v.Position.z = radius*sinf(phi)*sinf(theta);

                // Partial derivative of P with respect to theta
                v.tangent_u.x = -radius*sinf(phi)*sinf(theta);
                v.tangent_u.y = 0.0f;
                v.tangent_u.z = +radius*sinf(phi)*cosf(theta);

                XMVECTOR T = XMLoadFloat3(&v.tangent_u);
                XMStoreFloat3(&v.tangent_u, XMVector3Normalize(T));

                XMVECTOR p = XMLoadFloat3(&v.Position);
                XMStoreFloat3(&v.Normal, XMVector3Normalize(p));

                v.tex_c.x = theta / XM_2PI;
                v.tex_c.y = phi / XM_PI;

                mesh_data->Vertices.push_back( v );
            }
        }

        mesh_data->Vertices.push_back( bottomVertex );

        //
        // Compute indices for top stack.  The top stack was written first to the vertex buffer
        // and connects the top pole to the first ring.
        //

        for(UINT i = 1; i <= sliceCount; ++i)
        {
            mesh_data->Indices.push_back(0);
            mesh_data->Indices.push_back(i+1);
            mesh_data->Indices.push_back(i);
        }

        //
        // Compute indices for inner stacks (not connected to poles).
        //

        // Offset the indices to the index of the first vertex in the first ring.
        // This is just skipping the top pole vertex.
        UINT baseIndex = 1;
        UINT ringVertexCount = sliceCount+1;
        for(UINT i = 0; i < stackCount-2; ++i)
        {
            for(UINT j = 0; j < sliceCount; ++j)
            {
                mesh_data->Indices.push_back(baseIndex + i*ringVertexCount + j);
                mesh_data->Indices.push_back(baseIndex + i*ringVertexCount + j+1);
                mesh_data->Indices.push_back(baseIndex + (i+1)*ringVertexCount + j);

                mesh_data->Indices.push_back(baseIndex + (i+1)*ringVertexCount + j);
                mesh_data->Indices.push_back(baseIndex + i*ringVertexCount + j+1);
                mesh_data->Indices.push_back(baseIndex + (i+1)*ringVertexCount + j+1);
            }
        }

        //
        // Compute indices for bottom stack.  The bottom stack was written last to the vertex buffer
        // and connects the bottom pole to the bottom ring.
        //

        // South pole vertex was added last.
        UINT southPoleIndex = (UINT)mesh_data->Vertices.size()-1;

        // Offset the indices to the index of the first vertex in the last ring.
        baseIndex = southPoleIndex - ringVertexCount;

        for(UINT i = 0; i < sliceCount; ++i)
        {
            mesh_data->Indices.push_back(southPoleIndex);
            mesh_data->Indices.push_back(baseIndex+i);
            mesh_data->Indices.push_back(baseIndex+i+1);
        }
    }

    void create_geosphere(float radius, u32 numSubdivisions, MeshData *mesh_data) {
        // Put a cap on the number of subdivisions.
        numSubdivisions = math::_min(numSubdivisions, 5u);
        // Approximate a sphere by tessellating an icosahedron.
        const float X = 0.525731f;
        const float Z = 0.850651f;
        XMFLOAT3 pos[12] =
                {
                        XMFLOAT3(-X, 0.0f, Z), XMFLOAT3(X, 0.0f, Z),
                        XMFLOAT3(-X, 0.0f, -Z), XMFLOAT3(X, 0.0f, -Z),
                        XMFLOAT3(0.0f, Z, X), XMFLOAT3(0.0f, Z, -X),
                        XMFLOAT3(0.0f, -Z, X), XMFLOAT3(0.0f, -Z, -X),
                        XMFLOAT3(Z, X, 0.0f), XMFLOAT3(-Z, X, 0.0f),
                        XMFLOAT3(Z, -X, 0.0f), XMFLOAT3(-Z, -X, 0.0f),
                };
        DWORD k[60] =
                {
                        1, 4, 0, 4, 9, 0, 4, 5, 9, 8, 5, 4, 1, 8, 4,
                        1, 10, 8, 10, 3, 8, 8, 3, 5, 3, 2, 5, 3, 7, 2,
                        3, 10, 7, 10, 6, 7, 6, 11, 7, 6, 0, 11, 6, 1, 0,
                        10, 1, 6, 11, 0, 9, 2, 11, 9, 5, 2, 9, 11, 2, 7};

        mesh_data->Vertices.resize(12);
        mesh_data->Indices.resize(60);
        for (size_t i = 0; i < 12; ++i)
            mesh_data->Vertices[i].Position = pos[i];
        for (size_t i = 0; i < 60; ++i)
            mesh_data->Indices[i] = k[i];
        for (size_t i = 0; i < numSubdivisions; ++i)
            subdivide(mesh_data);
        // Project vertices onto sphere and scale.
        for (size_t i = 0; i < mesh_data->Vertices.size(); ++i) {
            // Project onto unit sphere.
            XMVECTOR n = XMVector3Normalize(XMLoadFloat3(
                    &mesh_data->Vertices[i].Position));
            // Project onto sphere.
            XMVECTOR p = radius * n;
            XMStoreFloat3(&mesh_data->Vertices[i].Position, p);
            XMStoreFloat3(&mesh_data->Vertices[i].Normal, n);
            // Derive texture coordinates from spherical coordinates.
            float theta = math::angle_from_xy(
                    mesh_data->Vertices[i].Position.x,
                    mesh_data->Vertices[i].Position.z);
            float phi = acosf(mesh_data->Vertices[i].Position.y / radius);
            mesh_data->Vertices[i].tex_c.x = theta / XM_2PI;
            mesh_data->Vertices[i].tex_c.y = phi / XM_PI;
            // Partial derivative of P with respect to theta
            mesh_data->Vertices[i].tangent_u.x = -radius * sinf(phi) * sinf(theta);
            mesh_data->Vertices[i].tangent_u.y = 0.0f;
            mesh_data->Vertices[i].tangent_u.z = +radius * sinf(phi) * cosf(theta);
            XMVECTOR T = XMLoadFloat3(&mesh_data->Vertices[i].tangent_u);
            XMStoreFloat3(&mesh_data->Vertices[i].tangent_u,
                          XMVector3Normalize(T));
        }
    }
}// namespace GeometryGenerator


// todo: this doesn't compile
//  make a substruct w shader state and use that instead of a macro...

LucyResult setup_color_shader(Arena *arena, RenderContext *rctx, ShaderFile shader_file, Shader *out_shader) {

    u64 checkpoint = arena_save(arena);

    Buf color_fx_buf;
    LucyResult lres; 

    switch (shader_file){
        case ShaderFile::Color: {
            lres = read_whole_file(arena, "build\\color.fxo", &color_fx_buf);
        }; break;
        case ShaderFile::ColorTrippy: {
            lres = read_whole_file(arena, "build\\color_trippy.fxo", &color_fx_buf);
        }; break;
    };
    
    assert(lres == LRES_OK);

    ID3DX11Effect *effect;

    HRESULT hres = D3DX11CreateEffectFromMemory(
            color_fx_buf.buf,
            color_fx_buf.size,
            0, rctx->device,
            &effect);
    assert(hres == 0);

    //getting tech and WVP matrix from effect
    ID3DX11EffectTechnique *tech = effect->GetTechniqueByName("ColorTech");
    assert(tech->IsValid());

    ID3DX11EffectMatrixVariable *wvp_mat_var = effect->GetVariableByName("gWorldViewProj")->AsMatrix();
    assert(wvp_mat_var->IsValid());

    ID3DX11EffectScalarVariable *time_var = effect->GetVariableByName("gTime")->AsScalar();
    assert(time_var->IsValid());

    // shader input layout

    ID3D11InputLayout *input_layout = nullptr;
    D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };

    D3DX11_PASS_DESC pass_desc;
    tech->GetPassByIndex(0)->GetDesc(&pass_desc);

    hres = rctx->device->CreateInputLayout(
            inputElementDesc,
            arrsize(inputElementDesc),
            pass_desc.pIAInputSignature,
            pass_desc.IAInputSignatureSize,
            &input_layout);
    assert(hres == 0);

    // don't need the shader buffer anymore. color_fx_buf is invalid now.
    arena_restore(arena, checkpoint);

    out_shader->tech = tech;
    out_shader->mInputLayout = input_layout;
    out_shader->wvp_mat_var = wvp_mat_var;
    out_shader->time_var = time_var;

    return LRES_OK;
}



namespace imgui_help {
    void float4_edit(const char *label, XMFLOAT4 *val) {
        f32 a[4];
        a[0] = val->x;
        a[1] = val->y;
        a[2] = val->z;
        a[3] = val->w;
        ImGui::ColorEdit4(label, a);

        *val = XMFLOAT4(a);
    }

    void float3_edit(const char *label, XMFLOAT3 *val) {
        f32 a[3];
        a[0] = val->x;
        a[1] = val->y;
        a[2] = val->z;
        ImGui::InputFloat3(label, a);
        *val = XMFLOAT3(a);
    }
}

