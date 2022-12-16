// 4.7 EXERCISES ----------------
fn void enum_adapters(Arena *arena, IDXGIFactory2 *dxgi_factory) {

    u64 checkpoint = arena_save(arena);

    IDXGIAdapter *dxgi_adapter_i;
    for (int i = 0; dxgi_factory->EnumAdapters(i, &dxgi_adapter_i) != DXGI_ERROR_NOT_FOUND; ++i) {
        log("found adapter %i", i);
        DXGI_ADAPTER_DESC desc;
        dxgi_adapter_i->GetDesc(&desc);
        log("adapter description: %ls", desc.Description);

        // does it support dx 11?

        LARGE_INTEGER p_umd_ver;
        HRESULT res = dxgi_adapter_i->CheckInterfaceSupport(__uuidof(IDXGIDevice), &p_umd_ver);
        log("does the adapter support dxgiDevice? %i, number returned: %i", res == 0, p_umd_ver);

        // enumerating outputs
        IDXGIOutput *output_i;
        i32 num_outputs = 0;
        while (dxgi_adapter_i->EnumOutputs(num_outputs, &output_i) != DXGI_ERROR_NOT_FOUND){

            //getting output1
            IDXGIOutput1 *output_1_i;
            output_i->QueryInterface(__uuidof(IDXGIOutput1), reinterpret_cast<void **>(&output_1_i));

            // calling the thing twice
            u32 p_num_modes = 0;
            output_1_i->GetDisplayModeList1(DXGI_FORMAT_R8G8B8A8_UNORM, 0, &p_num_modes, 0);
            DXGI_MODE_DESC1 *p_desc_arr = (DXGI_MODE_DESC1*)arena_push(arena, p_num_modes * sizeof(DXGI_MODE_DESC1));
            output_1_i->GetDisplayModeList1(DXGI_FORMAT_R8G8B8A8_UNORM, 0, &p_num_modes, p_desc_arr);

            // describing all the display modes per output
            for(int j = 0; j < p_num_modes; ++j) {
                DXGI_MODE_DESC1 mode = p_desc_arr[j];

                log("describing display mode %i for output %i", j, num_outputs);
                log("width: %i, h: %i, refresh rate: %i/%i", mode.Width, mode.Height, mode.RefreshRate.Numerator, mode.RefreshRate.Denominator);
            }

            ++num_outputs;
        }

        log("num outputs of adapter (i think these are monitors?) %i", num_outputs);
    }

    arena_restore(arena, checkpoint);
}