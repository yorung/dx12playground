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

	skyMan.Draw();
	triangle.Draw();
}

void App::Init()
{
	GoMyDir();

	triangle.Create();
	skyMan.Create("hakodate.jpg", "sky_photosphere");
}

void App::Destroy()
{
	deviceMan.Flush();
	triangle.Destroy();
	skyMan.Destroy();
}

void App::Update()
{
	matrixMan.Set(MatrixMan::VIEW, devCamera.CalcViewMatrix());
	triangle.Update();
}
