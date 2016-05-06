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

void Triangle::Draw()
{
	if (!ubo) {
		return;
	}
	afSetPipeline(pipelineState, rootSignature);
	afSetDescriptorHeap(heap);
	afSetVertexBuffer(vbo, sizeof(Vertex));
	afDraw(PT_TRIANGLELIST, 3);
}

void Triangle::Create()
{
	Descriptor descs[] = {
		CDescriptorCBV(0),
	};
	ubo = afCreateUBO(sizeof(Mat));
	vbo = afCreateVertexBuffer(sizeof(Vertex) * 3, vertices);
	rootSignature = afCreateRootSignature(1, descs, 0, nullptr);
	pipelineState = afCreatePSO("solid", elements, dimof(elements), BM_NONE, DSM_DEPTH_ENABLE, CM_DISABLE, rootSignature);
	SRVID srv[] = {
		ubo,
	};
	heap = afCreateDescriptorHeap(dimof(srv), srv);
}

void Triangle::Destroy()
{
	vbo.Reset();
	ubo.Reset();
	rootSignature.Reset();
	pipelineState.Reset();
	heap.Reset();
}

void Triangle::Update()
{
	if (!ubo) {
		return;
	}
	Mat m;
	m = q2m(Quat(Vec3(0, 0, 1), (float)GetTime()));
	afWriteBuffer(ubo, &m, sizeof(m));
}
