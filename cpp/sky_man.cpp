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
	static D3D12_DESCRIPTOR_RANGE descriptors[] = {
		CDescriptorCBV(0),
		CDescriptorSRV(0),
	};
	rootSignature = afCreateRootSignature(dimof(descriptors), descriptors, dimof(samplers), samplers);
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

	int descriptorHeapIndex = deviceMan.AssignDescriptorHeap(2);
	deviceMan.AssignConstantBuffer(descriptorHeapIndex, &invVP, sizeof(invVP));
	deviceMan.AssignSRV(descriptorHeapIndex + 1, texId);
	deviceMan.SetAssignedDescriptorHeap(descriptorHeapIndex);

	afDraw(PT_TRIANGLESTRIP, 4);
}

void SkyMan::Destroy()
{
	afSafeDeleteTexture(texId);
	pipelineState.Reset();
	rootSignature.Reset();
}
