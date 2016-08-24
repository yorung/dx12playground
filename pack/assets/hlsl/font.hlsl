struct VsIn {
	float2 pos : POSITION;
	float2 coord: TEXCOORD;
};

struct VsToPs {
	float4 pos : SV_POSITION;
	float2 coord : TEXCOORD;
};

#define RSDEF "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), DescriptorTable(SRV(t0)), StaticSampler(s0)"

[RootSignature(RSDEF)]
VsToPs VSMain(VsIn vsIn) {
	VsToPs vsOut;
	vsOut.pos = float4(vsIn.pos, 0, 1);
	vsOut.coord = vsIn.coord;
	return vsOut;
}

SamplerState gSampler : register(s0);
Texture2D gTexture : register(t0);

[RootSignature(RSDEF)]
float4 PSMain(VsToPs psIn) : SV_TARGET
{
	return gTexture.Sample(gSampler, psIn.coord);
}
