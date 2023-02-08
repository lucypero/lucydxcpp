//obj loading
#pragma once

#include <vector>
#include <unordered_map>
#include <string>

#include "lucy_math.h"
#include "lucytypes.h"
#include "proj_types.h"
#include "utils.h"
#include "renderer.h"

struct ObjParser {
    i64 pointer;
    std::string usemtl; // last "usemtl" spotted
    std::string object; // last "o" spotted
    i32 total_faces_parsed;
    i32 faces_since_last_usemtl;

    // per obj parsed data
    i32 pos_parsed;
    i32 norm_parsed;
    i32 uv_parsed;

    // parsed BEFORE last bound o
    i32 pos_parsed_before_o;
    i32 norm_parsed_before_o;
    i32 uv_parsed_before_o;
};

struct Object {
    std::string name;
    std::vector<XMFLOAT3> positions;
    std::vector<XMFLOAT3> normals;
    std::vector<XMFLOAT2> uvs;
    std::vector<i64> position_indices;
    std::vector<i64> normal_indices;
    std::vector<i64> uv_indices;
};

// assumes that all faces are triangles.
struct ObjFile {
    std::string mtl_filename;
    std::unordered_map<std::string, Material> materials;
    std::unordered_map<std::string, Object> objects;
};

LucyResult load_obj(Arena *arena, const char *file_path, ObjFile *out);

// Fully processed thing

struct MatRange {
    std::string material;
    u32 start;
    u32 end;
};

struct ObjRange {
    std::string object;
    u32 start;
    u32 end;
};

struct RenderObjects {
    std::vector<Vertex> vertices; // all the vertices?
    // std::vector<u32> indices // when u figure out indexing from obj's...
    ID3D11Buffer *vb;

    std::vector<XMFLOAT4X4> obj_world_mats;

    std::unordered_map<std::string, Material> materials;
    std::vector<MatRange> mat_ranges;
    std::vector<ObjRange> obj_ranges;

    void draw(ID3DX11EffectTechnique *tech, XMMATRIX view, XMMATRIX proj, RenderContext *ctx);
};

LucyResult load_obj_for_rendering(Arena *arena, ID3D11Device1 *device, const char *file_path, RenderObjects *out);