// dummy.hlsl

float4 VSMain(uint id : SV_VertexID) : SV_POSITION
{
    // Generate a Full-Screen Triangle to force 100% Rasterization / ROP activity
    // Adrenalin looks for significant onscreen 3D pixel generation.
    float x = (id == 2) ?  3.0f : -1.0f;
    float y = (id == 0) ? -3.0f :  1.0f;
    return float4(x, y, 0.5f, 1.0f);
}

float4 PSMain() : SV_TARGET
{
    // Return a solid color (pinkish-red)
    return float4(1.0f, 0.2f, 0.5f, 1.0f);
}
