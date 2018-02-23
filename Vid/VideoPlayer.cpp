
#include "Base.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/avutil.h>

};


int main3(int argc, char** argv)
{
	int err = 0;
	av_register_all();
	//the video container is also known as format
	AVFormatContext* formatCtx = nullptr;
	err = avformat_open_input(&formatCtx, "res/star_trails.mkv", nullptr, nullptr);
	VASSERT(err == 0);
	err = avformat_find_stream_info(formatCtx, nullptr);
	VASSERT(err == 0);
	
	for (size_t iStream = 0; iStream < formatCtx->nb_streams; iStream++)
	{
		AVCodecParameters* pCodecParams = formatCtx->streams[iStream]->codecpar;
		AVCodec* pCodec = avcodec_find_decoder(pCodecParams->codec_id);
		if (pCodecParams->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			VLOG_MESSAGE("W % H %", pCodecParams->width, pCodecParams->height);
		}
		else if (pCodecParams->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			VLOG_MESSAGE("Num Channels: %  Sample Rate: %", pCodecParams->channels, pCodecParams->sample_rate);
		}
		VLOG_MESSAGE("CodecName:% StreamIndex:%", pCodec->long_name, iStream);

		AVCodecContext* pCodecCtx = avcodec_alloc_context3(pCodec);
		err = avcodec_parameters_to_context(pCodecCtx, pCodecParams);
		VASSERT(err == 0);
		err = avcodec_open2(pCodecCtx, pCodec, nullptr);
		VASSERT(err == 0);
		//Now we're going to read the packets from the stream and decode them into frames but first, we need to allocate memory for both components
		AVPacket* pPacket = av_packet_alloc();
		AVFrame* pFrame = av_frame_alloc();
		//Let's feed our packets from the streams with the function av_read_frame while it has packets.
		while (av_read_frame(formatCtx, pPacket) >= 0)
		{
			VLOG_MESSAGE("Stream index %", pPacket->stream_index);
			if (pPacket->stream_index == 0)
			{
				int ret = avcodec_send_packet(pCodecCtx, pPacket);
				if (ret < 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
					std::cout << "avcodec_send_packet: " << ret << std::endl;
					break;
				}
				while (ret >= 0) {
					ret = avcodec_receive_frame(pCodecCtx, pFrame);
					if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
						//std::cout << "avcodec_receive_frame: " << ret << std::endl;
						break;
					}
					std::cout << "frame: " << pCodecCtx->frame_number << std::endl;
				}
			}
			av_packet_unref(pPacket);

// 				//send the raw data packet (compressed frame) to the decoder, through the codec context,
// 				err = avcodec_send_packet(pCodecCtx, pPacket);
// 				VASSERT(err == 0);
// 				//receive the raw data frame
// 				err = avcodec_receive_frame(pCodecCtx, pFrame);
// 				//
// 
// 				char errBuf[128];
// 				av_strerror(err, errBuf, sizeof(errBuf));
// 				VASSERT(err == 0);
		}
		av_frame_free(&pFrame);
		av_packet_free(&pPacket);
		avcodec_free_context(&pCodecCtx);
	}
	avformat_close_input(&formatCtx);

	return 0;
}


#if 0 // encoding smaple
static void encode(AVCodecContext *enc_ctx, AVFrame *frame, AVPacket *pkt,
	FILE *outfile)
{
	int ret;

	/* send the frame to the encoder */
	if (frame)
		printf("Send frame %i64\n", frame->pts);

	ret = avcodec_send_frame(enc_ctx, frame);
	if (ret < 0) {
		fprintf(stderr, "Error sending a frame for encoding\n");
		exit(1);
	}

	while (ret >= 0) {
		ret = avcodec_receive_packet(enc_ctx, pkt);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return;
		else if (ret < 0) {
			fprintf(stderr, "Error during encoding\n");
			exit(1);
		}

		printf("Write packet %d (size=%5d)\n", pkt->pts, pkt->size);
		fwrite(pkt->data, 1, pkt->size, outfile);
		av_packet_unref(pkt);
	}
}

int main(int argc, char **argv)
{

	
	const char *filename, *codec_name;
	const AVCodec *codec;
	AVCodecContext *c = NULL;
	int i, ret, x, y;
	FILE *f;
	AVFrame *frame;
	AVPacket *pkt;
	uint8_t endcode[] = { 0, 0, 1, 0xb7 };

	filename = "capture.mpeg";

	/* find the mpeg1video encoder */
	codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	VASSERT(codec);

	c = avcodec_alloc_context3(codec);
	VASSERT(c);

	pkt = av_packet_alloc();
	VASSERT(pkt);

	/* put sample parameters */
	c->bit_rate = 400000;
	/* resolution must be a multiple of two */
	c->width = 352;
	c->height = 288;
	/* frames per second */
	c->time_base = AVRational{ 1, 25 };
	c->framerate = AVRational{ 25, 1 };

	/* emit one intra frame every ten frames
	* check frame pict_type before passing frame
	* to encoder, if frame->pict_type is AV_PICTURE_TYPE_I
	* then gop_size is ignored and the output of encoder
	* will always be I frame irrespective to gop_size
	*/
	c->gop_size = 10;
	c->max_b_frames = 1;
	c->pix_fmt = AV_PIX_FMT_YUV420P;

	if (codec->id == AV_CODEC_ID_H264)
		av_opt_set(c->priv_data, "preset", "slow", 0);

	/* open it */
	ret = avcodec_open2(c, codec, NULL);
	VASSERT(0 == ret);

	f = fopen(filename, "wb");
	VASSERT(f);

	frame = av_frame_alloc();
	VASSERT(frame);

	frame->format = c->pix_fmt;
	frame->width = c->width;
	frame->height = c->height;

	ret = av_frame_get_buffer(frame, 32);
	VASSERT(ret >= 0);

	/* encode 1 second of video */
	for (i = 0; i < 25; i++) {
		fflush(stdout);

		/* make sure the frame data is writable */
		ret = av_frame_make_writable(frame);
		if (ret < 0)
			exit(1);

		/* prepare a dummy image */
		/* Y */
		for (y = 0; y < c->height; y++) {
			for (x = 0; x < c->width; x++) {
				frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
			}
		}

		/* Cb and Cr */
		for (y = 0; y < c->height / 2; y++) {
			for (x = 0; x < c->width / 2; x++) {
				frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;
				frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 5;
			}
		}

		frame->pts = i;

		/* encode the image */
		encode(c, frame, pkt, f);
	}

	/* flush the encoder */
	encode(c, NULL, pkt, f);

	/* add sequence end code to have a real MPEG file */
	fwrite(endcode, 1, sizeof(endcode), f);
	fclose(f);

	avcodec_free_context(&c);
	av_frame_free(&frame);
	av_packet_free(&pkt);

	return 0;
}
#endif // _DEBUG
