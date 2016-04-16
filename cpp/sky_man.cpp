#include <stdafx.h>

SkyMan skyMan;

SkyMan::~SkyMan()
{
	assert(!uboId);
	assert(!texId);
	assert(!rootSignature);
	assert(!pipelineState);
	assert(!heap);
}

void SkyMan::Create(const char *texFileName, const char* shader)
{
	Destroy();
	texId = afLoadTexture(texFileName, texDesc);
	uboId = afCreateUBO(sizeof(Mat));

	static Descriptor descriptors[] = {
		CDescriptorCBV(0),
		CDescriptorSRV(0),
	};
	static Sampler samplers[] = {
		CSampler(0, SF_LINEAR, SW_REPEAT),
	};
	SRVID srvs[] = {
		uboId,
		texId,
	};
	rootSignature = afCreateRootSignature(dimof(descriptors), descriptors, dimof(samplers), samplers);
	pipelineState = afCreatePSO(shader, nullptr, 0, BM_NONE, DSM_DEPTH_CLOSEREQUAL_READONLY, CM_DISABLE, rootSignature);
	heap = afCreateDescriptorHeap(dimof(srvs), srvs);
}

void SkyMan::Draw()
{
	if (!texId) {
		return;
	}
	if (!uboId) {
		return;
	}
	if (!pipelineState) {
		return;
	}

	afSetPipeline(pipelineState, rootSignature);
	afSetDescriptorHeap(heap);

	Mat matV, matP;
	matrixMan.Get(MatrixMan::VIEW, matV);
	matrixMan.Get(MatrixMan::PROJ, matP);
	matV._41 = matV._42 = matV._43 = 0;
	Mat invVP = inv(matV * matP);

	afWriteBuffer(uboId, &invVP, sizeof(invVP));
	afDraw(PT_TRIANGLESTRIP, 4);
}

void SkyMan::Destroy()
{
	afSafeDeleteBuffer(uboId);
	afSafeDeleteTexture(texId);
	pipelineState.Reset();
	rootSignature.Reset();
	heap.Reset();
}