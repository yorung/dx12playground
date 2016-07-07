#include "stdafx.h"

Triangle triangle;

static InputElement elements[] = {
	CInputElement("POSITION", SF_R32G32B32_FLOAT, 0),
	CInputElement("COLOR", SF_R32G32B32_FLOAT, 12),
};

struct Vertex {
	Vec3 pos;
	Vec3 color;
};

static Vertex vertices[] = {
	{ Vec3(0, 1, 0), Vec3(1, 1, 0) },
	{ Vec3(-1, -1, 0), Vec3(0, 1, 1) },
	{ Vec3(1, -1, 0), Vec3(1, 0, 1) },
};
static AFIndex indices[] = {
	0, 1, 2
};

void Triangle::Draw()
{
	if (!vbo) {
		return;
	}
	Mat m = q2m(Quat(Vec3(0, 0, 1), (float)GetTime()));
	int descriptorHeapIndex = deviceMan.AssignDescriptorHeap(1);
	deviceMan.AssignConstantBuffer(descriptorHeapIndex, &m, sizeof(m));
	afSetPipeline(pipelineState, rootSignature);
	deviceMan.SetAssignedDescriptorHeap(descriptorHeapIndex);
	afSetVertexBuffer(vbo, sizeof(Vertex));
	afSetIndexBuffer(ibo);
	afDrawIndexed(PT_TRIANGLELIST, 3);
}

void Triangle::Create()
{
	D3D12_DESCRIPTOR_RANGE descs[] = {
		CDescriptorCBV(0),
	};
	vbo = afCreateVertexBuffer(sizeof(Vertex) * 3, vertices);
	ibo = afCreateIndexBuffer(indices, _countof(indices));
	rootSignature = afCreateRootSignature(1, descs, 0, nullptr);
	pipelineState = afCreatePSO("solid", elements, dimof(elements), BM_NONE, DSM_DEPTH_ENABLE, CM_DISABLE, rootSignature);
}

void Triangle::Destroy()
{
	vbo.Reset();
	ibo.Reset();
	rootSignature.Reset();
	pipelineState.Reset();
}
