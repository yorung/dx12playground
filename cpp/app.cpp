#include "stdafx.h"

App app;

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

App::App()
{
}

void App::Draw()
{
	afSetDescriptorHeap(heap);
	afSetPipeline(pipelineState, rootSignature);
	afSetVertexBuffer(vbo, sizeof(Vertex));
	afDraw(PT_TRIANGLELIST, 3);
}

void App::Init()
{
	GoMyDir();

	Descriptor descs[] = {
		CDescriptorCBV(0),
	};
	ubo = afCreateUBO(sizeof(Mat));
	vbo = afCreateVertexBuffer(sizeof(Vertex) * 3, vertices);
	rootSignature = afCreateRootSignature(1, descs, 0, nullptr);
	pipelineState = afCreatePSO("solid", elements, dimof(elements), BM_NONE, DSM_DISABLE, CM_DISABLE, rootSignature);
	SRVID srv[] = {
		ubo,
	};
	heap = afCreateDescriptorHeap(dimof(srv), srv);
}

void App::Destroy()
{
	vbo.Reset();
	ubo.Reset();
	rootSignature.Reset();
	pipelineState.Reset();
	heap.Reset();
}

void App::Update()
{
	Mat m;
	m = q2m(Quat(Vec3(0, 0, 1), (float)GetTime()));
	afWriteBuffer(ubo, &m, sizeof(m));
}
