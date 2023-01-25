//obj loading

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

fn bool point_to_end_of_line(Buf *file, i64 *pointer) {
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

fn bool point_to_next_line(Buf *file, i64 *pointer) {
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

fn bool parse_mtl_line(Arena *arena, i64 *pointer, Buf *file, std::unordered_map<std::string, Material> *materials) {
    return true;
}

fn LucyResult load_mtl(Arena *arena, std::string *file_path, std::unordered_map<std::string, Material> *materials) {

    Buf file = {};

    LucyResult res = read_whole_file(arena, file_path->c_str(), &file);
    if(res != LRES_OK) {
        return res;
    }

    log("mtl file was succesfully loaded");

    i64 pointer = 0;
    bool is_end = false;

    // parse lines
    while(!is_end) {
        // parses line and moves pointer to next line
        is_end = parse_mtl_line(arena, &pointer, &file, materials);
    }

    return LRES_OK;
}

fn bool parse_line(Arena *arena, ObjParser *parser, Buf *file, ObjFile *obj) {

    i64 start = parser->pointer;
    bool file_ends = point_to_end_of_line(file, &parser->pointer);
    char *buf = (char*)file->buf;

    char *line = (char*)arena_push(arena, parser->pointer - start + 1 + 1);
    memcpy((void*)line, &buf[start], parser->pointer - start + 1);
    line[parser->pointer - start + 1] = '\0';

    char op_name_buf[10];
    i32 chars_read = 0;
    i32 read = sscanf_s(line, "%s%n", op_name_buf, 10, &chars_read);
    assert(read == 1);

    std::string op_name = op_name_buf;

    line = &line[chars_read + 1];

    if(op_name == "v") {
        // parsing vert
        f32 pos_1, pos_2, pos_3;
        read = sscanf_s(line, "%f %f %f", &pos_1, &pos_2, &pos_3);
        assert(read == 3);
        obj->positions.push_back(XMFLOAT3(pos_1, pos_2, pos_3));
    } else if (op_name == "vn") {
        // parsing normals
        f32 pos_1, pos_2, pos_3;
        read = sscanf_s(line, "%f %f %f", &pos_1, &pos_2, &pos_3);
        assert(read == 3);
        obj->normals.push_back(XMFLOAT3(pos_1, pos_2, pos_3));
    } else if (op_name == "vt") {
        // parsing uvs
        f32 pos_1, pos_2;
        read = sscanf_s(line, "%f %f", &pos_1, &pos_2);
        assert(read == 2);
        obj->uvs.push_back(XMFLOAT2(pos_1, pos_2));
    } else if (op_name == "f") {
        // parsing faces
        i32 p_1, p_2, p_3, n_1, n_2, n_3, u_1, u_2, u_3;
        read = sscanf_s(line, "%i/%i/%i %i/%i/%i %i/%i/%i", &p_1, &u_1, &n_1, &p_2, &u_2, &n_2, &p_3, &u_3, &n_3);
        assert(read == 9);

        parser->total_faces_parsed += 1;
        parser->faces_since_last_usemtl += 1;

        obj->position_indices.push_back(p_1);
        obj->position_indices.push_back(p_2);
        obj->position_indices.push_back(p_3);

        obj->uv_indices.push_back(u_1);
        obj->uv_indices.push_back(u_2);
        obj->uv_indices.push_back(u_3);

        obj->normal_indices.push_back(n_1);
        obj->normal_indices.push_back(n_2);
        obj->normal_indices.push_back(n_3);
    } else if (op_name == "mtllib") {
        //parsing mtl filename
        char mtl_filename[100];
        read = sscanf_s(line, "%s%n", mtl_filename, 100, &chars_read);
        assert(read == 1);

        log("mtl filename is %s", mtl_filename);
        obj->mtl_filename = mtl_filename;

        //now load the mtl data
    } else if (op_name == "usemtl") {
        char material_name[100];
        read = sscanf_s(line, "%s%n", material_name, 100, &chars_read);
        assert(read == 1);

        if (parser->faces_since_last_usemtl > 0) {
            // todo write something to obj parser
            // those indices are of mtl parser->usemtl

            log("faces %i to %i are of material %s", 
              parser->total_faces_parsed - parser->faces_since_last_usemtl,
              parser->total_faces_parsed - 1,
              parser->usemtl.c_str() );
        }

        parser->usemtl = material_name;
        parser->faces_since_last_usemtl = 0;
    }

    file_ends = point_to_next_line(file, &parser->pointer);

    return file_ends;
}

fn LucyResult load_obj(Arena *arena, const char *file_path, ObjFile *out) {

    u64 checkpoint = arena_save(arena);
    Buf file = {};
    LucyResult res = read_whole_file(arena, file_path, &file);
    if(res != LRES_OK) {
        arena_restore(arena, checkpoint);
        return res;
    }

    ObjParser parser = {};

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

        log("mtl path name is %s", mtl_dir.c_str());

        //concat name
        res = load_mtl(arena, &mtl_dir, &out->materials);
        assert(res == LRES_OK);
    }

    arena_restore(arena, checkpoint);

    return LRES_OK;
}