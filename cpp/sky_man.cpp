#include <stdafx.h>

SkyMan skyMan;

static const SamplerType samplers[] = { AFST_LINEAR_WRAP };

SkyMan::~SkyMan()
{
	assert(!texId);
	assert(!renderStates.IsReady());
}

void SkyMan::Create(const char *texFileName, const char* shader)
{
	Destroy();
	texId = afLoadTexture(texFileName, texDesc);
	renderStates.Create(shader, 0, nullptr, BM_NONE, DSM_DEPTH_CLOSEREQUAL_READONLY, CM_DISABLE, dimof(samplers), samplers);
}

void SkyMan::Draw()
{
	if (!renderStates.IsReady()) {
		return;
	}
	renderStates.Apply();

	Mat matV, matP;
	matrixMan.Get(MatrixMan::VIEW, matV);
	matrixMan.Get(MatrixMan::PROJ, matP);
	matV._41 = matV._42 = matV._43 = 0;
	Mat invVP = inv(matV * matP);

	AFCbvBindToken token;
	token.Create(&invVP, sizeof(invVP));
	afBindCbvs(&token, 1);
	(texDesc.isCubeMap ? afBindCubeMapToBindingPoint : afBindTextureToBindingPoint)(texId, 1);
	afDraw(PT_TRIANGLESTRIP, 4);
}

void SkyMan::Destroy()
{
	afSafeDeleteTexture(texId);
	renderStates.Destroy();
}
