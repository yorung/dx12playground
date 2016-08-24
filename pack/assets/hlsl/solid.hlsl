cbuffer perFrame : register(b0)
{
	float4x4 matVP;
};

struct VsInput {
	float3 pos : POSITION;
	float3 col : COLOR;
};

struct VsOutput {
	float4 pos : SV_POSITION;
	float3 col : COLOR;
};

#define RSDEF "RootFlags(ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT), CBV(b0)"

[RootSignature(RSDEF)]
VsOutput VSMain(VsInput inp)
{
	VsOutput outp;
	outp.pos = mul(matVP, float4(inp.pos, 1));
	outp.col = inp.col;
	return outp;
}

[RootSignature(RSDEF)]
float4 PSMain(VsOutput inp) : SV_TARGET
{
	return float4(inp.col, 0.5);
}
