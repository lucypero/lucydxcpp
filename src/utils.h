#pragma once

#include <vector>

#include "lucytypes.h"
#include "proj_types.h"

#define HR(a) {HRESULT __hres = (a); assert(__hres == 0);}

// Convenience macro for releasing COM objects.
#define ReleaseCOM(x) { if(x){ x->Release(); x = 0; } }

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

void log(const char *format, ...);

void *arena_push(Arena *arena, u64 size);

void arena_clear(Arena *arena);

void arena_zero(Arena *arena);

u64 arena_save(Arena *arena);

void arena_restore(Arena *arena, u64 checkpoint);

//this allocs a buffer w the file contents
LucyResult read_whole_file(Arena *arena, const char *file_path, Buf *out_buf);

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
            float u, float v);

    // m = amount of vertices along the x axis
    // n = amount of vertices along the z axis
    // total vertices: m * n
    void create_grid(f32 width, f32 depth, u32 m, u32 n, MeshData *mesh_data);
    void build_cylinder_top_cap(f32 bottom_radius, f32 top_radius, f32 height, u32 slice_count, u32 stack_count, MeshData *mesh_data);
    void build_cylinder_bottom_cap(f32 bottom_radius, f32 top_radius, f32 height, u32 slice_count, u32 stack_count, MeshData *mesh_data);
    void create_cylinder(f32 bottom_radius, f32 top_radius, f32 height, u32 slice_count, u32 stack_count, MeshData *mesh_data);
    void subdivide(MeshData *mesh_data);
    void create_box(float width, float height, float depth, MeshData *meshData);
    void create_sphere(float radius, u32 sliceCount, u32 stackCount, MeshData *mesh_data);
    void create_geosphere(float radius, u32 numSubdivisions, MeshData *mesh_data);
}// namespace GeometryGenerator


namespace imgui_help {
    void float4_edit(const char *label, XMFLOAT4 *val);
    void float3_edit(const char *label, XMFLOAT3 *val);
}
