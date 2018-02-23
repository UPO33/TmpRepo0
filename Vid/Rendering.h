#pragma once

#include "Base.h"
#include "../Core/SmartPointers.h"
#include <d3dcompiler.h>
#include <vector>

#define D3D_COMPILE_STANDARD_FILE_INCLUDE ((ID3DInclude*)(UINT_PTR)1)

using namespace Veena;

//////////////////////////////////////////////////////////////////////////
template<typename TShader>  TShader* VCrateShader(const wchar_t* filename, const char* entryPoint, TSPtr<ID3DBlob>* outoptByteCode = nullptr)
{
	TSPtr<ID3DBlob> shaderBytes, errorBytes;

	const char* target = nullptr;

	if (std::is_same<TShader, ID3D11VertexShader>::value)
		target = "vs_4_0";
	if (std::is_same<TShader, ID3D11PixelShader>::value)
		target = "ps_4_0";

	ID3D11DeviceChild* shader = nullptr;

	HRESULT hr = D3DCompileFromFile(filename, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, entryPoint, target,
		D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, shaderBytes.GetAddress(), errorBytes.GetAddress());

	if (outoptByteCode)
		*outoptByteCode = shaderBytes;
		

	if (SUCCEEDED(hr))
	{
		if(std::is_same<TShader, ID3D11VertexShader>::value)
			hr = gDXDevice->CreateVertexShader(shaderBytes->GetBufferPointer(), shaderBytes->GetBufferSize(), nullptr, (ID3D11VertexShader**)&shader);
		if (std::is_same<TShader, ID3D11PixelShader>::value)
			hr = gDXDevice->CreatePixelShader(shaderBytes->GetBufferPointer(), shaderBytes->GetBufferSize(), nullptr, (ID3D11PixelShader**)&shader);

		VASSERT(SUCCEEDED(hr));
	}
	else
	{
		const char* pErrors = (const char*)errorBytes->GetBufferPointer();
		VLOG_ERROR("compiling shader failed: %", pErrors);
	}
	return (TShader*)shader;
}

//helper structure to map a dx resource easily
template<typename T, UINT subresource> struct D3D11MapScoped
{
	union
	{
		D3D11_MAPPED_SUBRESOURCE mMappedSubresource;
		struct
		{
			T*   mData; //pointer to the mapped data
			UINT mRowPitch;
			UINT mDepthPitch;
		};
	};

	ID3D11Resource* mResource;

	D3D11MapScoped(ID3D11Resource* resource, D3D11_MAP mapType = D3D11_MAP_WRITE_DISCARD)
		: mResource(resource)
	{
		HRESULT hr = gDXDeviceCtx->Map(resource, subresource, mapType, 0, &mMappedSubresource);
		VASSERT(SUCCEEDED(hr));
	}
	~D3D11MapScoped()
	{
		gDXDeviceCtx->Unmap(mResource, subresource);
	}

	D3D11MapScoped(const D3D11MapScoped&) = delete;
	D3D11MapScoped& operator =(const D3D11MapScoped&) = delete;
};


template<typename TClass> class TRef
{
public:
	TRef() : mPtr(nullptr) {}
	TRef(const TRef& copy)
	{
		mPtr = copy.mPtr;
		if (mPtr) mPtr->AddRef();
	}
	TRef(const TRef&& move)
	{
		mPtr = move.mPtr;
		move.mPtr = nullptr;
	}
	TRef(TClass* ptr)
	{
		mPtr = ptr;
		if (mPtr) mPtr->AddRef();
	}
	TRef& operator = (const TRef& copy)
	{
		if (copy.mPtr)
			copy.mPtr->AddRef();
		if (mPtr)
			mPtr->Release();

		mPtr = copy.mPtr;

		return *this;
	}
	~TRef()
	{
		if (mPtr) mPtr->Release();
		mPtr = nullptr;
	}

	TClass* mPtr;
	TClass* Get() { return mPtr; }
	TClass** GetAddress() { return &mPtr; }
};

//////////////////////////////////////////////////////////////////////////
struct Graphics
{
	TSPtr<ID3D11SamplerState> mDefaultSampler;
	TSPtr<ID3D11BlendState> mDefaultBlendState;

	static Graphics* Sing();

	Graphics();

	ID3D11SamplerState* GetDefaultSampler() { return mDefaultSampler; }
	ID3D11BlendState* GetDefaultBlendState() { return mDefaultBlendState; }
};