struct VsIn {
	float2 pos : POSITION;
	float2 coord: TEXCOORD;
};

struct VsToPs {
	float4 pos : SV_POSITION;
	float2 coord : TEXCOORD;
};

VsToPs VSMain(VsIn vsIn) {
	VsToPs vsOut;
	vsOut.pos = float4(vsIn.pos, 0, 1);
	vsOut.coord = vsIn.coord;
	return vsOut;
}

SamplerState gSampler : register(s0);
Texture2D gTexture : register(t0);
float4 PSMain(VsToPs psIn) : SV_TARGET
{
	return gTexture.Sample(gSampler, psIn.coord);
}
