//obj loading

#include "obj_loader.h"


static bool point_to_end_of_line(Buf *file, i64 *pointer) {
    char *buf = (char*)file->buf;
    char c = buf[*pointer];
    while(c != '\n' && c != '\r') {

        *pointer += 1;

        if(*pointer >= (i64)file->size) {
            return true;
        }

        c = buf[*pointer];
    }

    return false;
}

static bool point_to_next_line(Buf *file, i64 *pointer) {
    char *buf = (char*)file->buf;
    char c = buf[*pointer];
    while(c != '\n' && c != '\r') {

        *pointer += 1;

        if(*pointer >= (i64)file->size) {
            return true;
        }

        c = buf[*pointer];
    }

    while(c == '\n' || c == '\r') {
        *pointer += 1;

        if(*pointer >= (i64)file->size) {
            return true;
        }

        c = buf[*pointer];
    }

    return false;
}

static bool parse_mtl_line(Arena *arena, i64 *pointer, Buf *file, std::unordered_map<std::string, Material> *materials) {
    return true;
}

static LucyResult load_mtl(Arena *arena, std::string *file_path, std::unordered_map<std::string, Material> *materials) {

    Buf file = {};

    LucyResult res = read_whole_file(arena, file_path->c_str(), &file);
    if(res != LRES_OK) {
        return res;
    }


    i64 pointer = 0;
    bool is_end = false;

    // parse lines
    while(!is_end) {
        // parses line and moves pointer to next line
        is_end = parse_mtl_line(arena, &pointer, &file, materials);
    }

    return LRES_OK;
}

static bool parse_line(Arena *arena, ObjParser *parser, Buf *file, ObjFile *obj) {

    i64 start = parser->pointer;
    bool file_ends = point_to_end_of_line(file, &parser->pointer);
    char *buf = (char*)file->buf;

    char *line = (char*)arena_push(arena, parser->pointer - start + 1 + 1);
    memcpy((void*)line, &buf[start], parser->pointer - start + 1);
    line[parser->pointer - start + 1] = '\0';

    // check for current obj

    auto it = obj->objects.find(parser->object);

    if(it == obj->objects.end()) {
        // initializing obj


        Object o = {};
        o.name = parser->object;

        obj->objects[parser->object] = o;
    } 

    Object *the_obj = &obj->objects[parser->object];

    char op_name_buf[10];
    i32 chars_read = 0;
    i32 read = sscanf_s(line, "%s%n", op_name_buf, 10, &chars_read);
    assert(read == 1);

    std::string op_name = op_name_buf;

    line = &line[chars_read + 1];

    if(op_name == "o") {
        char object_name[100];

        //todo: what if it has spaces?? haha
        read = sscanf_s(line, "%s%n", object_name, 100, &chars_read);
        assert(read == 1);


        parser->object = object_name;

        parser->pos_parsed_before_o = parser->pos_parsed;
        parser->norm_parsed_before_o = parser->norm_parsed;
        parser->uv_parsed_before_o = parser->uv_parsed;
    }
    else if(op_name == "v") {
        // parsing vert
        f32 pos_1, pos_2, pos_3;
        read = sscanf_s(line, "%f %f %f", &pos_1, &pos_2, &pos_3);
        assert(read == 3);
        the_obj->positions.push_back(XMFLOAT3(pos_1, pos_2, pos_3));
        ++parser->pos_parsed;
    } else if (op_name == "vn") {
        // parsing normals
        f32 pos_1, pos_2, pos_3;
        read = sscanf_s(line, "%f %f %f", &pos_1, &pos_2, &pos_3);
        assert(read == 3);
        the_obj->normals.push_back(XMFLOAT3(pos_1, pos_2, pos_3));
        ++parser->norm_parsed;
    } else if (op_name == "vt") {
        // parsing uvs
        f32 pos_1, pos_2;
        read = sscanf_s(line, "%f %f", &pos_1, &pos_2);
        assert(read == 2);
        the_obj->uvs.push_back(XMFLOAT2(pos_1, pos_2));
        ++parser->uv_parsed;
    } else if (op_name == "f") {
        // parsing faces
        i32 p_1, p_2, p_3, n_1, n_2, n_3, u_1, u_2, u_3;
        read = sscanf_s(line, "%i/%i/%i %i/%i/%i %i/%i/%i", &p_1, &u_1, &n_1, &p_2, &u_2, &n_2, &p_3, &u_3, &n_3);
        assert(read == 9);

        parser->total_faces_parsed += 1;
        parser->faces_since_last_usemtl += 1;

        the_obj->position_indices.push_back(p_1 - parser->pos_parsed_before_o - 1);
        the_obj->position_indices.push_back(p_2 - parser->pos_parsed_before_o - 1);
        the_obj->position_indices.push_back(p_3 - parser->pos_parsed_before_o - 1);

        the_obj->uv_indices.push_back(u_1 - parser->uv_parsed_before_o - 1);
        the_obj->uv_indices.push_back(u_2 - parser->uv_parsed_before_o - 1);
        the_obj->uv_indices.push_back(u_3 - parser->uv_parsed_before_o - 1);

        the_obj->normal_indices.push_back(n_1 - parser->norm_parsed_before_o - 1);
        the_obj->normal_indices.push_back(n_2 - parser->norm_parsed_before_o - 1);
        the_obj->normal_indices.push_back(n_3 - parser->norm_parsed_before_o - 1);
    } else if (op_name == "mtllib") {
        //parsing mtl filename
        char mtl_filename[100];
        read = sscanf_s(line, "%s%n", mtl_filename, 100, &chars_read);
        assert(read == 1);

        obj->mtl_filename = mtl_filename;

        //now load the mtl data
    } else if (op_name == "usemtl") {
        char material_name[100];
        read = sscanf_s(line, "%s%n", material_name, 100, &chars_read);
        assert(read == 1);

        parser->usemtl = material_name;
        parser->faces_since_last_usemtl = 0;
    }

    file_ends = point_to_next_line(file, &parser->pointer);

    return file_ends;
}

LucyResult load_obj(Arena *arena, const char *file_path, ObjFile *out) {

    u64 checkpoint = arena_save(arena);
    Buf file = {};
    LucyResult res = read_whole_file(arena, file_path, &file);
    if(res != LRES_OK) {
        arena_restore(arena, checkpoint);
        return res;
    }

    ObjParser parser = {};
    parser.object = "unnammed";

    bool is_end = false;

    // parse lines
    while(!is_end) {
        // parses line and moves pointer to next line
        is_end = parse_line(arena, &parser, &file, out);
    }

    // load mtl
    if (out->mtl_filename.size() > 0) {

        std::string mtl_dir(file_path);

        // u have to cut the dir until the last \
        // then add the mtl filename
        std::size_t found = mtl_dir.find_last_of("\\");
        mtl_dir = mtl_dir.substr(0, found);
        mtl_dir += "\\";
        mtl_dir += out->mtl_filename;

        //concat name
        res = load_mtl(arena, &mtl_dir, &out->materials);
        assert(res == LRES_OK);
    }

    //todo: filter empty objects

    arena_restore(arena, checkpoint);

    return LRES_OK;
}

//TODO: materials (use your own material for now)
LucyResult load_obj_for_rendering(Arena *arena, ID3D11Device1 *device, const char *file_path, RenderObjects *out) {

    ObjFile obj_file = {};

    LR(load_obj(arena, file_path, &obj_file));

    u32 total_vertices = 0;

    for(const auto &obj: obj_file.objects) {
        total_vertices += (u32)obj.second.position_indices.size();
    }

    std::vector<Vertex> obj_vertices(total_vertices);
    std::vector<ObjRange> obj_ranges(obj_file.objects.size());
    std::vector<XMFLOAT4X4> obj_world_mats(obj_file.objects.size());

    //constructing vertices
    u32 vertices_processed = 0;

    u32 obj_i = 0;
    for(auto const &obj_pair: obj_file.objects) {

        auto &obj = obj_pair.second;

        for(u32 i = 0; i < obj.position_indices.size(); ++i) {
            assert(obj.position_indices[i] >= 0);
            assert(obj.normal_indices[i] >= 0);
            assert(obj.uv_indices[i] >= 0);
            assert(obj.normal_indices.size() == obj.position_indices.size());
            assert(obj.uv_indices.size() == obj.position_indices.size());

            obj_vertices[vertices_processed + i].Pos = obj.positions[obj.position_indices[i]];
            obj_vertices[vertices_processed + i].Normal = obj.normals[obj.normal_indices[i]];
            obj_vertices[vertices_processed + i].Tex = obj.uvs[obj.uv_indices[i]];
        }

        ObjRange obj_range = {};
        obj_range.object = obj.name;
        obj_range.start = vertices_processed;
        vertices_processed += (u32)obj.position_indices.size();
        obj_range.end = vertices_processed;

        obj_ranges[obj_i] = obj_range;

        // initializing world mat
        XMMATRIX obj_scale = XMMatrixScaling(1.0f, 1.0f, 1.0f);
        XMMATRIX obj_rot = XMMatrixRotationX(0.0f);
        XMMATRIX obj_offset = XMMatrixTranslation(0.0f, 0.0f, 0.0f);

        XMFLOAT4X4 world_mat;
        XMStoreFloat4x4(&world_mat, XMMatrixMultiply(obj_rot, XMMatrixMultiply(obj_scale, obj_offset)));

        obj_world_mats[obj_i] = world_mat;

        ++obj_i;
    }

    ID3D11Buffer *vb;
    D3D11_BUFFER_DESC vbd = {};
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = sizeof(Vertex) * (u32)obj_vertices.size();
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
    D3D11_SUBRESOURCE_DATA vinitData = {};
    vinitData.pSysMem = &obj_vertices[0];
    HR(device->CreateBuffer(&vbd, &vinitData, &vb));

    //todo mat ranges. for now just make a default material.

    // initializing material
    Material mat = {
        .Ambient = XMFLOAT4(0.1f, 0.1f, 0.1f, 0.1f),
        .Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
        .Specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 32.0f),
        // what is this
        .Reflect = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
    };

    std::unordered_map<std::string, Material> materials;
    std::string mat_name = "DefMat";
    materials[mat_name] = mat;

    std::vector<MatRange> mat_ranges(1);
    MatRange mat_range = {};
    mat_range.material = mat_name;
    mat_range.start = 0;
    mat_range.end = total_vertices;
    mat_ranges[0] = mat_range;

    // end todo temporary

    out->vertices = obj_vertices;
    out->vb = vb;
    out->obj_world_mats = obj_world_mats;
    out->materials = materials;
    out->mat_ranges = mat_ranges;
    out->obj_ranges = obj_ranges;

    return LRES_OK;
}

void RenderObjects::draw(ID3DX11EffectTechnique *tech, XMMATRIX view, XMMATRIX proj, RenderContext *ctx) {
    D3DX11_TECHNIQUE_DESC techDesc;
    tech->GetDesc(&techDesc);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    ctx->device_context->IASetVertexBuffers(0, 1, &this->vb, &stride, &offset);

    // this should only be 1 pass so don't get scared by this loop
    for (u32 p = 0; p < techDesc.Passes; ++p) {

        // this could be in a render_obj method

        u32 obj_i = 0;

        for(auto const &obj_range: this->obj_ranges) {
            // set all the shader vars
            XMMATRIX world = XMLoadFloat4x4(&this->obj_world_mats[obj_i]);
            XMMATRIX worldInvTranspose = math::InverseTranspose(world);
            XMMATRIX worldViewProj = world*view*proj;

            ctx->basic_effect.SetWorld(world);
            ctx->basic_effect.SetWorldInvTranspose(worldInvTranspose);
            ctx->basic_effect.SetWorldViewProj(worldViewProj);

            for(auto const &mat_range: this->mat_ranges) {

                Material mat = this->materials[mat_range.material];
		        ctx->basic_effect.SetMaterial(&mat);

                u32 start = math::clamp(mat_range.start, obj_range.start, obj_range.end);
                u32 end = math::clamp(mat_range.end, obj_range.start, obj_range.end);

                // apply
                tech->GetPassByIndex(p)->Apply(0, ctx->device_context);

                // we will not use indexed drawing bc i still don't know how to parsse obj's.
                ctx->device_context->Draw(end - start, start);
            }

            ++obj_i;
        }
    }
}