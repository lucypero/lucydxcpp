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
    std::string usemtl;
    i32 total_faces_parsed;
    i32 faces_since_last_usemtl;
};

// assumes that all faces are triangles.
struct ObjFile {
    std::vector<XMFLOAT3> positions;
    std::vector<XMFLOAT3> normals;
    std::vector<XMFLOAT2> uvs;
    std::vector<i64> position_indices;
    std::vector<i64> normal_indices;
    std::vector<i64> uv_indices;
    std::string mtl_filename;
    std::unordered_map<std::string, Material> materials;
};

LucyResult load_obj(Arena *arena, const char *file_path, ObjFile *out);