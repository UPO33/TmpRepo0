#pragma once

#include "Base.h"
#include "Rendering.h"
#include "../Core/SmartPointers.h"
#include "../Core/Vector.h"
#include "../Core/Timer.h"
#include "../Core/Queue.h"

using namespace Veena;

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libavfilter/avfilter.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
};

struct DXTestDecoding
{
	ID3D11VideoDecoder* mVideoDecoder = nullptr;
	ID3D11VideoDecoderOutputView* mOutputView = nullptr;
	ID3D11Resource* mOutputResource = nullptr;


	DXTestDecoding();
	~DXTestDecoding();
};


///
struct DXTestingTexture
{	
	static constexpr unsigned MaxFrame = 6;

	struct Frame
	{
		float mTime = 0;
		ID3D11ShaderResourceView* mTexture = nullptr;
	};

	enum EState
	{
		ENull,
		EPlaying,
		EPaused,
	};

	SwsContext*				mSwsContext = nullptr;
	AVFormatContext*		mFormatContext = nullptr;
	AVCodecContext*			mVideoCodecContext = nullptr;
	AVCodecContext*			mAudioCodecContext = nullptr;
	AVStream*				mVideoStream = nullptr;
	AVStream*				mAudioStream = nullptr;
	unsigned				mFrameWidth = 0;
	unsigned				mFrameHeight = 0;

	TSPtr<ID3D11Texture2D>				mRGBATextures[MaxFrame];
	TSPtr<ID3D11ShaderResourceView>		mRGBATexturesView[MaxFrame];
	ID3D11ShaderResourceView*			mCurrentPlayingSRV = nullptr;
	size_t								mQueueReadIndex = 0;
	size_t								mQueueWriteIndex = 0;
	size_t								mQueueLength = 0;
	Frame								mQueueFrames[MaxFrame];
	float								mSecondsSincePlay = 0;
	//number of frames of the video passed
	size_t								mNumFramePresented = 0;
	float								mTimeScale = 1;
	EState								mState = ENull;
	char								mDebugInputFilename[256];
	
	AVPacket* mVideoPacket = nullptr;
	AVFrame* mVideoFrame = nullptr;

	DXTestingTexture();
	~DXTestingTexture();


	

	void Pause();
	void Resume();
	void Rewind();
	bool SetFile(const char* url, bool playImmediately);
	
	void Update(float delta);
	void Render(float delta);
	void UpdateDXTexture(AVFrame* frame, ID3D11Texture2D* pTextrure);

private:

	void FetchFrames();
	void Release();


	void CreateGFXResources();
};

using Test0 = DXTestingTexture;



