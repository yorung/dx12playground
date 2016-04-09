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
	afSetPipeline(pipelineState, rootSignature);
	afSetVertexBuffer(vbo, sizeof(Vertex));
	afDraw(PT_TRIANGLELIST, 3);
}

void App::Init()
{
	GoMyDir();

	vbo = afCreateVertexBuffer(sizeof(Vertex) * 3, vertices);
	rootSignature = afCreateRootSignature(0, nullptr, 0, nullptr);
	pipelineState = afCreatePSO("solid", elements, dimof(elements), BM_NONE, DSM_DISABLE, CM_DISABLE, rootSignature);
}

void App::Destroy()
{
	vbo.Reset();
	rootSignature.Reset();
	pipelineState.Reset();
}

void App::Update()
{
}
