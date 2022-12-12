// my types
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float f32;
typedef double f64;

#define arrsize(arr) (sizeof(arr) / sizeof(arr[0]))

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
    u64 offset; //arena offset pointer. it offsets to the first free byte. starts at 0
    u64 size;
};

struct Buf {
    void *buf;
    u64 size;
};

void log(const char* format, ...) {
    va_list argp;
    char buf[200];
    va_start(argp, format);
    i32 written = vsprintf_s(buf, 200, format, argp);
    va_end(argp);
    buf[written] = '\n';
    buf[written+1] = '\0';
    OutputDebugStringA(buf);
}

void *arena_push(Arena *arena, u64 size) {
    void* ret_buf = arena->buf + arena->offset;

    arena->offset += size;

    // checking that u have enough mem in the arena
    assert(arena->offset <= arena->size);

    return ret_buf;
}

void arena_clear(Arena *arena) {
    arena->offset = 0;
}

void arena_zero(Arena *arena) {
    arena->offset = 0;
    memset(arena->buf, 0, arena->size);
}

u64 arena_save(Arena *arena) {
    return arena->offset;
}

void arena_restore(Arena *arena, u64 checkpoint) {
    arena->offset = checkpoint;
}

//this allocs a buffer w the file contents
LucyResult read_whole_file(Arena *arena, const char *file_path, Buf *out_buf) {
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
