#include "VideoPlayerDX.h"
#include "imgui.h"
#include "Rendering.h"

const unsigned QueueSize = 32;


struct VQVertex
{
	float x, y;
	float u, v;
};


void VGenQuadVertices(float x, float y, float w, float h, VQVertex pVertices[6])
{
	VQVertex tl = { x, y, 0, 0 };
	VQVertex tr = { x + w, y, 1, 0 };
	VQVertex bl = { x, y + h, 0, 1 };
	VQVertex br = { x + w, y + h, 1, 1 };

	pVertices[0] = tl;
	pVertices[1] = tr;
	pVertices[2] = br;

	pVertices[3] = tl;
	pVertices[4] = br;
	pVertices[5] = bl;
}
//generates 6 vertices for a quad
void VGenQuadVertices(Vec2 xy, Vec2 wh, VQVertex pVertices[6])
{
	VGenQuadVertices(xy.x, xy.y, wh.x, wh.y, pVertices);
}

DXTestingTexture::DXTestingTexture()
{
	mDebugInputFilename[0] = 0;
	SetFile("res/stars.mkv", true);
}

void DXTestingTexture::FetchFrames()
{
	if (mQueueLength <= 2)
	{
		if (!mVideoPacket) mVideoPacket = av_packet_alloc();
		if (!mVideoFrame) mVideoFrame = av_frame_alloc();
		AVPacket* pPacket = mVideoPacket;
		AVFrame* pFrame = mVideoFrame;
		//Now we're going to read the packets from the stream and decode them into frames but first, we need to allocate memory for both components
		//AVPacket* pPacket = av_packet_alloc();
		//AVFrame* pFrame = av_frame_alloc();
		
		//Let's feed our packets from the streams with the function av_read_frame while it has packets.
		int ret = (av_read_frame(mFormatContext, pPacket));
		while(ret >= 0 && mQueueLength < (MaxFrame - 1))
		{
			if (pPacket->stream_index == 0)
			{
				ret = avcodec_send_packet(mVideoCodecContext, pPacket);
				if (ret < 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				{
					std::cout << "avcodec_send_packet: " << ret << std::endl;
					return;
				}
				while (ret >= 0 && (mQueueLength < (MaxFrame - 1)))
				{
					ret = avcodec_receive_frame(mVideoCodecContext, pFrame);

					if(ret == 0)
					{
						double bts = pFrame->best_effort_timestamp * av_q2d(mVideoStream->time_base);

						ID3D11Texture2D* pNewFrameTex = mRGBATextures[mQueueWriteIndex];
						ID3D11ShaderResourceView* pNewFrameSrv = mRGBATexturesView[mQueueWriteIndex];

						UpdateDXTexture(pFrame, pNewFrameTex);

						mQueueFrames[mQueueWriteIndex].mTexture = pNewFrameSrv;
						mQueueFrames[mQueueWriteIndex].mTime = bts;

						mQueueWriteIndex = (mQueueWriteIndex + 1) % MaxFrame;
						mQueueLength++;
					}
					else if(ret == AVERROR(EAGAIN))
					{
						VASSERT(true);
					}
					else if (ret == AVERROR_EOF)
					{
						VASSERT(true);
					}
					else if (ret == AVERROR(AVERROR_EOF))
					{
						VASSERT(true);
					}
				}
			}


			//av_packet_unref(pPacket);

			ret = (av_read_frame(mFormatContext, pPacket));
		}
		if (ret == AVERROR_EOF)
		{
			VLOG_MESSAGE("Finished");
			mState = ENull;
		}
		else if (ret != 0)
		{
			constexpr int eof = AVERROR_EOF;
			char errBuff[256];
			av_strerror(ret, errBuff, sizeof(errBuff));
			VLOG_MESSAGE("%", errBuff);
		}
		//av_frame_free(&pFrame);
		//av_packet_free(&pPacket);
	}
}

void DXTestingTexture::Release()
{
	for (size_t i = 0; i < MaxFrame; i++)
	{
		mRGBATexturesView[i] = nullptr;
		mRGBATextures[i] = nullptr;
		mQueueFrames[i].mTexture = nullptr;
		mQueueFrames[i].mTime = 0;
	}

	mState = EState::ENull;
	mVideoStream = nullptr;
	mAudioStream = nullptr;
	mSecondsSincePlay = 0;
	mNumFramePresented = 0;
	mCurrentPlayingSRV = nullptr;
	mQueueReadIndex = mQueueWriteIndex = mQueueLength = 0;
	mFrameWidth = mFrameHeight = 0;

	if (mSwsContext) sws_freeContext(mSwsContext);
	if (mVideoCodecContext) avcodec_free_context(&mVideoCodecContext);
	if (mAudioCodecContext) avcodec_free_context(&mAudioCodecContext);
	if (mFormatContext) avformat_close_input(&mFormatContext);
}

void DXTestingTexture::CreateGFXResources()
{
	HRESULT hr = S_OK;

	for (size_t iTexture = 0; iTexture < MaxFrame; iTexture++)
	{
		D3D11_TEXTURE2D_DESC textureDesc = {};
		textureDesc.Width = mFrameWidth;
		textureDesc.Height = mFrameHeight;
		textureDesc.MipLevels = 1;
		textureDesc.ArraySize = 1;
		textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		textureDesc.Usage = D3D11_USAGE_DYNAMIC;
		textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		textureDesc.MiscFlags = 0;

		hr = gDXDevice->CreateTexture2D(&textureDesc, nullptr, mRGBATextures[iTexture].GetAddress());
		VASSERT(SUCCEEDED(hr));


		hr = gDXDevice->CreateShaderResourceView(mRGBATextures[iTexture], nullptr, mRGBATexturesView[iTexture].GetAddress());
		VASSERT(SUCCEEDED(hr));
	}
}

void DXTestingTexture::Update(float delta)
{
	if(mState == EState::EPlaying)
		mSecondsSincePlay += delta * mTimeScale;

	if (mState == EState::EPlaying)
	{
		FetchFrames();

		while (mQueueLength != 0 && mQueueFrames[mQueueReadIndex].mTime <= mSecondsSincePlay)
		{
			mCurrentPlayingSRV = mQueueFrames[mQueueReadIndex].mTexture;

			mQueueReadIndex = (mQueueReadIndex + 1) % MaxFrame;
			mQueueLength--;

			mNumFramePresented++;
		}
	}
	
	if (ImGui::Begin("Video Rendering"))
	{
		ImGui::Text("Seconds Sine Play %f", mSecondsSincePlay);
		ImGui::Text("QLen %d", mQueueLength);
		ImGui::Separator();

		bool bInputFilenameChanged = ImGui::InputText("File", mDebugInputFilename, sizeof(mDebugInputFilename), ImGuiInputTextFlags_EnterReturnsTrue);
		bool bPause = ImGui::Button("Pause"); ImGui::SameLine();
		bool bResume = ImGui::Button("Resume"); ImGui::SameLine();
		bool bRewind = ImGui::Button("Rewind"); ImGui::SameLine();
		ImGui::SliderFloat("TimeScale", &mTimeScale, 0.1f, 10);
		ImGui::Separator();

		if(mCurrentPlayingSRV)
			ImGui::Image(mCurrentPlayingSRV, ImGui::GetContentRegionAvail());

		ImGui::End();

		if (bPause) Pause();
		if (bRewind) Rewind();
		if (bResume) Resume();
		if (bInputFilenameChanged) SetFile(mDebugInputFilename, true);
	}
}


void DXTestingTexture::Render(float delta)
{
	/*if(0)
	{
		{
			D3D11MapScoped<VQVertex, 0> mapped(mVBVideoQuad, D3D11_MAP_WRITE_DISCARD);
			VGenQuadVertices(mVideoViewportXY, mVideoViewportWH, mapped.mData);
		}

		gDXDeviceCtx->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
		UINT strides[] = { sizeof(VQVertex) };
		UINT offsets[] = { 0 };
		gDXDeviceCtx->IASetVertexBuffers(0, 1, mVBVideoQuad.GetAddress(), strides, offsets);
		gDXDeviceCtx->VSSetShader(mVSVideoQuad, nullptr, 0);
		gDXDeviceCtx->PSSetShader(mPSVideoQuad, nullptr, 0);
		gDXDeviceCtx->IASetInputLayout(mILVideoQuad);
		gDXDeviceCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		ID3D11SamplerState* samplers[] = { Graphics::Sing()->GetDefaultSampler() };
		gDXDeviceCtx->PSSetSamplers(0, VCOUNTOF(samplers), samplers);
		ID3D11ShaderResourceView* srvs[] = { nullptr, nullptr, mCurrentPlayingSRV };
		gDXDeviceCtx->PSSetShaderResources(0, VCOUNTOF(srvs), srvs);
		gDXDeviceCtx->Draw(6, 0);
	}*/

}

void YPUV444ToRGB888(char Y, char U, char V, char* outRGB)
{
	auto C = Y - 16;
	auto D = U - 128;
	auto E = V - 128;

#define clip3(v) (v & 0xFF)

	outRGB[0] = clip3((298 * C + 409 * E + 128) >> 8);
	outRGB[1] = clip3((298 * C - 100 * D - 208 * E + 128) >> 8);
	outRGB[2] = clip3((298 * C + 516 * D + 128) >> 8);
};

void YPUV420ToRGB888(char* ys, char* us, char* vs, unsigned posY, unsigned posX, unsigned w, char* outRGB)
{
	//size.total = size.width * size.height;
	//y = yuv[position.y * size.width + position.x];
	//u = yuv[(position.y / 2) * (size.width / 2) + (position.x / 2) + size.total];
	//v = yuv[(position.y / 2) * (size.width / 2) + (position.x / 2) + size.total + (size.total / 4)];
	//rgb = Y′UV444toRGB888(y, u, v);

	auto y = ys[posX];
	auto u = us[0];
	auto v = vs[0];
	YPUV444ToRGB888(y, u, v, outRGB);
}

void DXTestingTexture::UpdateDXTexture(AVFrame* frame, ID3D11Texture2D* pTextrure)
{
	//copying the frame data to dx texture
	//#TODO using native texture format for video to avoid conversion
	{
		D3D11MapScoped<uint8_t, 0> mapped(pTextrure);
		//const int dstSize = av_image_get_buffer_size((AVPixelFormat)frame->format, frame->width, frame->height, 16);
		const int lineSize = mapped.mRowPitch; //av_image_get_linesize(AV_PIX_FMT_RGBA, mFrameWidth, 0);
		int h = sws_scale(mSwsContext, frame->data, frame->linesize, 0, frame->height, &mapped.mData, &lineSize);
	}
}
//////////////////////////////////////////////////////////////////////////
DXGI_FORMAT GetEquivalentDXFormat(AVPixelFormat pf)
{
	switch (pf)
	{
	case AV_PIX_FMT_YUV420P: return DXGI_FORMAT_NV12;
	};

	return DXGI_FORMAT_UNKNOWN;
}
#if 0
//////////////////////////////////////////////////////////////////////////
void DXTestingTexture::InitVideo()
{
	int err = 0;
	av_register_all();

	
	//the video container is also known as format

	const char* videoFile = "res/gogo.mp4";
	//const char* videoFile = "res/page18.avi";
	//const char* videoFile = "res/star_trails.mkv";
	err = avformat_open_input(&mFormatContext, videoFile, nullptr, nullptr);
	VASSERT(err == 0);
	err = avformat_find_stream_info(mFormatContext, nullptr);
	VASSERT(err == 0);
	for (size_t iStream = 0; iStream < mFormatContext->nb_streams; iStream++)
	{
		AVStream* pStream = mFormatContext->streams[iStream];
		AVCodecParameters* pCodecParams = mFormatContext->streams[iStream]->codecpar;
		
		AVCodec* pCodec = avcodec_find_decoder(pCodecParams->codec_id);
		
		VLOG_MESSAGE("Stream Index:% CodecName:%", iStream, pCodec->long_name);
		

		if (pCodecParams->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			mVideoCodecContext = avcodec_alloc_context3(pCodec);
			err = avcodec_parameters_to_context(mVideoCodecContext, pCodecParams);
			VASSERT(err == 0);
			err = avcodec_open2(mVideoCodecContext, pCodec, nullptr);

			VASSERT(err == 0);

			AVPixelFormat pixelFormat = (AVPixelFormat)pCodecParams->format;
			VLOG_MESSAGE("W:%  H:%  Pixel Format %", pCodecParams->width, pCodecParams->height, pixelFormat);

			mVideoWidth = pCodecParams->width;
			mVideoHeight = pCodecParams->height;
			auto tpf = mVideoCodecContext->ticks_per_frame;
			mSwsContext = sws_getContext(mVideoWidth, mVideoHeight, pixelFormat, mVideoWidth, mVideoHeight
				, AV_PIX_FMT_RGBA, 0, nullptr, nullptr, nullptr );
			VASSERT(mSwsContext);
			
			mVideoStram = pStream;

			
			double durationInSecond0 = mFormatContext->duration / (double)AV_TIME_BASE;
			double durationInSecondS = mVideoStram->duration / (double)AV_TIME_BASE;
			double durationInSecond1 = mFormatContext->duration * av_q2d(pStream->time_base);
			
			double frameDelayMS = av_q2d(pStream->time_base) * 1000;

			mChronoStart.Start();

			return;
		}
	}
}


void DXTestingTexture::InitGFXResources()
{

	HRESULT hr = S_OK;

	//NV12 textures and views
	{
		D3D11_TEXTURE2D_DESC textureDesc = {};
		textureDesc.Width = mVideoWidth;
		textureDesc.Height = mVideoHeight;
		textureDesc.MipLevels = 1;
		textureDesc.ArraySize = 1;
		textureDesc.Format = DXGI_FORMAT_NV12;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		textureDesc.Usage = D3D11_USAGE_DYNAMIC;
		textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		textureDesc.MiscFlags = 0;

		hr = gDXDevice->CreateTexture2D(&textureDesc, nullptr, &mTextureYUV);
		VASSERT(SUCCEEDED(hr));

		D3D11_SHADER_RESOURCE_VIEW_DESC descChrominanceView = {};
		D3D11_SHADER_RESOURCE_VIEW_DESC descLumaView = {};

		descLumaView.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		descLumaView.Format = DXGI_FORMAT_R8_UNORM;
		descLumaView.Texture2D.MipLevels = 1;
		descLumaView.Texture2D.MostDetailedMip = 0;

		// DirectX specifies the view format to be DXGI_FORMAT_R8_UNORM for NV12 luminance channel.
		// Luminance is 8 bits per pixel. DirectX will handle converting 8-bit integers into normalized
		// floats for use in the shader.
		hr = gDXDevice->CreateShaderResourceView(mTextureYUV, &descLumaView, &mLumaView);
		VASSERT(SUCCEEDED(hr));


		// DirectX specifies the view format to be DXGI_FORMAT_R8G8_UNORM for NV12 chrominance channel.
		// Chrominance has 4 bits for U and 4 bits for V per pixel. DirectX will handle converting 4-bit
		// integers into normalized floats for use in the shader.
		descChrominanceView.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		descChrominanceView.Format = DXGI_FORMAT_R8G8_UNORM;
		descChrominanceView.Texture2D.MipLevels = 1;
		descChrominanceView.Texture2D.MostDetailedMip = 0;
		hr = gDXDevice->CreateShaderResourceView(mTextureYUV, &descChrominanceView, &mChominanceView);
		VASSERT(SUCCEEDED(hr));

	}


	TSPtr<ID3DBlob> vsByteCode;

	mVSVideoQuad = VCrateShader<ID3D11VertexShader>(L"../../../Shaders/VideoScreenQuad.hlsl", "MainVS", &vsByteCode);
	mPSVideoQuad = VCrateShader<ID3D11PixelShader>(L"../../../Shaders/VideoScreenQuad.hlsl", "MainPS");

	{
		D3D11_INPUT_ELEMENT_DESC elements[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{"UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(float[2]), D3D11_INPUT_PER_VERTEX_DATA, 0},
		};

		hr = gDXDevice->CreateInputLayout(elements, VCOUNTOF(elements), vsByteCode->GetBufferPointer(), vsByteCode->GetBufferSize(), mILVideoQuad.GetAddress());
		VASSERT(SUCCEEDED(hr));

		hr = gDXDevice->CreateBuffer(&CD3D11_BUFFER_DESC(1024, D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE),
			nullptr, mVBVideoQuad.GetAddress());
		VASSERT(SUCCEEDED(hr));
	}
}
#endif

DXTestingTexture::~DXTestingTexture()
{
	Release();
}


void DXTestingTexture::Pause()
{
	if (mState == EPlaying)
	{
		mState = EState::EPaused;
	}
}

void DXTestingTexture::Resume()
{
	if (mState == EState::EPaused)
	{
		mState = EState::EPlaying;
	}
}

void DXTestingTexture::Rewind()
{
}

#define CHECK_AVERR(err) 										\
if (err != 0)													\
{																\
	char zzzErrBuf[512];										\
	av_strerror(err, zzzErrBuf, sizeof(zzzErrBuf));				\
	VLOG_ERROR("%", err);										\
}																\



bool DXTestingTexture::SetFile(const char* url, bool bPlayImmediately)
{
	if (mFormatContext && strcmp(mFormatContext->url, url) == 0) //the same file requested again?
	{
		if (mState == EState::EPaused && bPlayImmediately)
			Resume();
		
		return true;
	}

	Release();

	int err = 0;
	err = avformat_open_input(&mFormatContext, url, nullptr, nullptr);
	if (err != 0)
	{
		CHECK_AVERR(err);
		return false;
	}
	err = avformat_find_stream_info(mFormatContext, nullptr);
	if (err != 0)
	{
		CHECK_AVERR(err);

		avformat_close_input(&mFormatContext);
		return false;
	}
	
	for (size_t iStream = 0; iStream < mFormatContext->nb_streams; iStream++)
	{
		AVStream* pStream = mFormatContext->streams[iStream];
		AVCodecParameters* pCodecParams = mFormatContext->streams[iStream]->codecpar;
		if (pStream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			mVideoStream = pStream;
		}
		else if (pStream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			mAudioStream = pStream;
		}
	}
	
	if (mVideoStream == nullptr)
	{
		avformat_close_input(&mFormatContext);
		VLOG_MESSAGE("video stream not found");
		return false;
	}
	if (mAudioStream == nullptr)
	{
		VLOG_MESSAGE("audio stream not found");
	}

	//open video stream
	{
		AVCodec* pVideoCodec = avcodec_find_decoder(mVideoStream->codecpar->codec_id);
		mVideoCodecContext = avcodec_alloc_context3(pVideoCodec);
		err = avcodec_parameters_to_context(mVideoCodecContext, mVideoStream->codecpar);
		VASSERT(err == 0);
		err = avcodec_open2(mVideoCodecContext, pVideoCodec, nullptr);
		VASSERT(err == 0);

		mFrameWidth = mVideoStream->codecpar->width;
		mFrameHeight = mVideoStream->codecpar->height;

		AVPixelFormat pixelFormat = (AVPixelFormat)mVideoStream->codecpar->format; //pf of the video
		mSwsContext = sws_getContext(mFrameWidth, mFrameHeight, pixelFormat, mFrameWidth, mFrameHeight, AV_PIX_FMT_RGBA, 0, nullptr, nullptr, nullptr);
		VASSERT(mSwsContext);

		double durationInSecond0 = mFormatContext->duration / (double)AV_TIME_BASE;
	}

	//opwn audio stream
	{

	}

	mState = bPlayImmediately ? EState::EPlaying : EState::EPaused;
	CreateGFXResources();
}

DXTestDecoding::DXTestDecoding()
{
#ifdef ___ZXC
	HRESULT hr = S_OK;

	{
		D3D11_TEXTURE2D_DESC textureDesc = {};
		textureDesc.Width = 512;
		textureDesc.Height = 512;
		textureDesc.Usage = D3D11_USAGE_DEFAULT;
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DECODER;

		hr = gDXDevice->CreateTexture2D(&textureDesc, nullptr, &mOutputResource);
		VASSERT(SUCCEEDED(hr));
	}
	//creating decoder
	{
		D3D11_VIDEO_DECODER_DESC decoderDesc = { D3D11_DECODER_PROFILE_H264_VLD_NOFGT , 512, 512, DXGI_FORMAT_NV12 };

		unsigned configCount = 0;
		hr = gDXVideoDevice->GetVideoDecoderConfigCount(&decoderDesc, &configCount);
		VASSERT(SUCCEEDED(hr));
		VLOG_MESSAGE("Num Config %", configCount);

		for (unsigned iConfig = 0; iConfig < configCount; iConfig++)
		{
			D3D11_VIDEO_DECODER_CONFIG decoderConfig = {};
			hr = gDXVideoDevice->GetVideoDecoderConfig(&decoderDesc, iConfig, &decoderConfig);
			VASSERT(SUCCEEDED(hr));

			hr = gDXVideoDevice->CreateVideoDecoder(&decoderDesc, &decoderConfig, &mVideoDecoder);
			VASSERT(SUCCEEDED(hr));

			D3D11_VIDEO_DECODER_OUTPUT_VIEW_DESC ovDesc;
			ovDesc.ViewDimension = D3D11_VDOV_DIMENSION_TEXTURE2D;
			ovDesc.DecodeProfile = D3D11_DECODER_PROFILE_H264_VLD_NOFGT;
			ovDesc.Texture2D.ArraySlice = 0;

			hr = gDXVideoDevice->CreateVideoDecoderOutputView(nullptr, &ovDesc, &mOutputView);
			VASSERT(SUCCEEDED(hr));

			break;
		}

	}

	unsigned numDecoder = gDXVideoDevice->GetVideoDecoderProfileCount();
	VLOG_MESSAGE("Num Video Decoder %", numDecoder);
	for (unsigned iDecoder = 0; iDecoder < numDecoder; iDecoder++)
	{
		GUID decoderGUID;
		hr = gDXVideoDevice->GetVideoDecoderProfile(iDecoder, &decoderGUID);
		VASSERT(SUCCEEDED(hr));

		D3D11_VIDEO_DECODER_DESC vdDesc = {};
		vdDesc.Guid = decoderGUID;
		vdDesc.SampleWidth = 256;
		vdDesc.SampleHeight = 256;
		vdDesc.OutputFormat = DXGI_FORMAT_R8G8B8A8_UNORM;;
		unsigned configCount = 0;
		hr = gDXVideoDevice->GetVideoDecoderConfigCount(&vdDesc, &configCount);
		VASSERT(SUCCEEDED(hr));
		for (unsigned iConfig = 0; iConfig < configCount; iConfig++)
		{
			D3D11_VIDEO_DECODER_CONFIG vdConfig = {};
			hr = gDXVideoDevice->GetVideoDecoderConfig(&vdDesc, iConfig, &vdConfig);
			VASSERT(SUCCEEDED(hr));
		}
	}
#endif // ___ZXC
}

DXTestDecoding::~DXTestDecoding()
{
	
}
