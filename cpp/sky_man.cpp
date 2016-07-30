#include <stdafx.h>

SkyMan skyMan;

static const SamplerType samplers[] = { AFST_LINEAR_WRAP };

SkyMan::~SkyMan()
{
	assert(!texId);
	assert(!rootSignature);
	assert(!pipelineState);
}

void SkyMan::Create(const char *texFileName, const char* shader)
{
	Destroy();
	rootSignature = afCreateRootSignature(AFDL_CBV0_SRV0, dimof(samplers), samplers);
	pipelineState = afCreatePSO(shader, nullptr, 0, BM_NONE, DSM_DEPTH_CLOSEREQUAL_READONLY, CM_DISABLE, rootSignature);
	texId = afLoadTexture(texFileName, texDesc);
}

void SkyMan::Draw()
{
	if (!texId) {
		return;
	}
	if (!pipelineState) {
		return;
	}

	afSetPipeline(pipelineState, rootSignature);

	Mat matV, matP;
	matrixMan.Get(MatrixMan::VIEW, matV);
	matrixMan.Get(MatrixMan::PROJ, matP);
	matV._41 = matV._42 = matV._43 = 0;
	Mat invVP = inv(matV * matP);
	afBindCbv0Srv0(&invVP, sizeof(invVP), texId);
	afDraw(PT_TRIANGLESTRIP, 4);
}

void SkyMan::Destroy()
{
	afSafeDeleteTexture(texId);
	pipelineState.Reset();
	rootSignature.Reset();
}
