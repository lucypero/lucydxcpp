struct vs_in {
    float3 position_local : POS;
    float3 color : COL;
};

struct vs_out {
    float4 position_clip : SV_POSITION; // required output of VS
    float4 color : COL;
};

vs_out vs_main(vs_in input) {
  vs_out output = (vs_out)0; // zero the memory first
  output.position_clip = float4(input.position_local, 1.0);
  output.color = float4(input.color, 1.0);
  return output;
}

float4 ps_main(vs_out input) : SV_TARGET {
  return input.color;
}