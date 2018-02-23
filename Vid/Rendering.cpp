#include "Rendering.h"

Graphics* Graphics::Sing()
{
	static Graphics Ins;
	return &Ins;
}

Graphics::Graphics()
{
	HRESULT hr = S_OK;

	hr = gDXDevice->CreateSamplerState(&CD3D11_SAMPLER_DESC(CD3D11_DEFAULT()), mDefaultSampler.GetAddress());
	VASSERT(SUCCEEDED(hr));

	hr = gDXDevice->CreateBlendState(&CD3D11_BLEND_DESC(CD3D11_DEFAULT()), mDefaultBlendState.GetAddress());
	VASSERT(SUCCEEDED(hr));
}
