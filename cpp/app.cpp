#include "stdafx.h"

App app;

App::App()
{
}

void App::Draw()
{
	IVec2 scrSize = systemMisc.GetScreenSize();
	float f = 1000;
	float n = 1;
	float aspect = (float)scrSize.x / scrSize.y;
	Mat proj = perspectiveLH(45.0f * (float)M_PI / 180.0f, aspect, n, f);
	matrixMan.Set(MatrixMan::PROJ, proj);

	rt.BeginRenderToThis();

	triangle.Draw();
	skyMan.Draw();

	rsPostProcess.Apply();
	deviceMan.SetRenderTarget();
	afBindSrv0(rt.GetTexture());
	afDraw(PT_TRIANGLESTRIP, 4);

	fontMan.Render();
}

void App::Init()
{
	GoMyDir();

	triangle.Create();
	skyMan.Create("yangjae.dds", "sky_photosphere");
	fontMan.Init();

	IVec2 scrSize = systemMisc.GetScreenSize();
	rt.Init(scrSize, AFDT_R8G8B8A8_UNORM, AFDT_DEPTH_STENCIL);

	SamplerType sampler = AFST_POINT_CLAMP;
	rsPostProcess.Create(AFDL_SRV0, "vivid", 0, nullptr, BM_NONE, DSM_DISABLE, CM_DISABLE, 1, &sampler);
}

void App::Destroy()
{
	deviceMan.Flush();
	triangle.Destroy();
	skyMan.Destroy();
	fontMan.Destroy();
	rsPostProcess.Destroy();
	rt.Destroy();
}

void App::Update()
{
	matrixMan.Set(MatrixMan::VIEW, devCamera.CalcViewMatrix());
	fps.Update();
	fontMan.DrawString(Vec2(20, 40), 20, SPrintf("FPS: %f", fps.Get()));
}
