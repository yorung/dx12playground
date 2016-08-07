#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#define NOMINMAX
#define _USE_MATH_DEFINES

#include <windows.h>

#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>

#include <wrl.h>
#include <shellapi.h>

#include <cmath>
#include <string>
#include <algorithm>
#include <vector>
#include <map>
#include <random>
#include <chrono>
#include <memory>

#include <assert.h>

using Microsoft::WRL::ComPtr;
#include "af_math.h"
#include "device_man_dx12.h"
#include "helper.h"
#include "helper_dx12.h"
#include "helper_text.h"
#include "matrix_man.h"
#include "dev_camera.h"
#include "system_misc.h"
#include "sky_man.h"
#include "dib.h"
#include "font_man.h"
#include "fps.h"

#include "triangle.h"
#include "app.h"
