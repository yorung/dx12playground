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
	afSetPipeline(pipelineState, rootSignature);
	afSetVertexBuffer(vbo, sizeof(Vertex));
	afSetIndexBuffer(ibo);
	Mat matWorld[] = {
		q2m(Quat(Vec3(0, 0, 1), (float)GetTime())) * translate(-1, 0, 0),
		q2m(Quat(Vec3(0, 1, 0), (float)GetTime())),
		q2m(Quat(Vec3(1, 0, 0), (float)GetTime())) * translate(1, 0, 0)
	};
	Mat matProj = ortho(-2, 2, -2, 2, 0.001f, 10.0f);
	Mat matView = lookatLH(Vec3(0, 0, -1), Vec3(0, 0, 0), Vec3(0, 1, 0));
	for (int i = 0; i < dimof(matWorld); i++) {
		Mat m = matWorld[i] * matView * matProj;
		int cbTop = deviceMan.AssignConstantBuffer(&m, sizeof(m));
		deviceMan.GetCommandList()->SetGraphicsRootConstantBufferView(0, deviceMan.GetConstantBufferGPUAddress(cbTop));
		afDrawIndexed(PT_TRIANGLELIST, 3);
	}
}

void Triangle::Create()
{
	vbo = afCreateVertexBuffer(sizeof(Vertex) * 3, vertices);
	ibo = afCreateIndexBuffer(indices, _countof(indices));
	vbo->SetName(L"Triangle vertex bugger");
	ibo->SetName(L"Triangle index bugger");
	rootSignature = afCreateRootSignature(AFDL_ROOTCBV0, 0, nullptr);
	pipelineState = afCreatePSO("solid", elements, dimof(elements), BM_NONE, DSM_DEPTH_ENABLE, CM_DISABLE, rootSignature, D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
}

void Triangle::Destroy()
{
	vbo.Reset();
	ibo.Reset();
	rootSignature.Reset();
	pipelineState.Reset();
}
