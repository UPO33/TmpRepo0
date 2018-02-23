#pragma once

#include "../Core/Core.h"
#include <d3d11.h>
#include <dxgi.h>

extern ID3D11Device*            gDXDevice;
extern ID3D11DeviceContext*     gDXDeviceCtx;
extern ID3D11VideoContext*      gDXVideoDeviceCtx;
extern IDXGISwapChain*          gSwapChain;
extern ID3D11RenderTargetView*  gMainRenderTargetView;
extern ID3D11VideoDevice*		gDXVideoDevice;

