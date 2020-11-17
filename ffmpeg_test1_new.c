#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#define CODEC_TYPE_VIDEO AVMEDIA_TYPE_VIDEO
#define CODEC_TYPE_AUDIO AVMEDIA_TYPE_AUDIO

static struct SwsContext *img_convert_ctx;

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame)
{
    FILE *pFile;
    char szFilename[32];
    int y;

    //Open file
    sprintf(szFilename, "frame%d.ppm", iFrame);
    pFile = fopen(szFilename, "wb");
    if (pFile == NULL)
    {
        return;
    }

    // Write header
    fprintf(pFile, "P6\n%d %d\n255\n", width, height);

    // Write pixel data
    for (y=0; y<height; y++)
    {
        fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);
    }

    // Close file
    fclose(pFile);
}

int main(int argc, char* argv[])
{
    av_register_all();

    AVFormatContext *pFormatCtx;

    if (avformat_open_input(&pFormatCtx, argv[1], NULL, NULL) != 0)
    {
        printf("could not open file\n");
        return -1;
    }

    //Retrieve stream information
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
    {
        printf("could not find stream info.\n");
        return -1;
    }

    //Dump information about file onto standard error
    av_dump_format(pFormatCtx, 0, argv[1], 0);

    int i;
    AVCodecContext *pCodecCtx;

    // FInd the first video stream
    int videoStream = -1;
    for (i = 0; i < pFormatCtx->nb_streams; i++)
    {
        if (pFormatCtx->streams[i]->codec->codec_type == CODEC_TYPE_VIDEO)
        {
            videoStream=i;
            break;
        }
    }

    if (videoStream == -1)
    {
        printf("did not find a video stream\n");
        return -1;
    }

    // Get a pointer to the codec context for the video stream
    pCodecCtx = pFormatCtx->streams[videoStream]->codec;
    AVCodec *pCodec;

    // Find the decoder for the video stream
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL)
    {
        fprintf(stderr, "Unsupported codec!\n");
        return -1;
    }

    // Open codec
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
    {
        printf("could not open codec\n");
        return -1;
    }

    AVFrame *pFrame;

    // allocate video frame
    pFrame = av_frame_alloc();

    // allocate an AVFrame structure
    AVFrame *pFrameRGB = av_frame_alloc();
    if (pFrameRGB == NULL)
    {
        return -1;
    }

    uint8_t *buffer;
    int numBytes;
    // Determine required buffer size and allocate buffer
    numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);
    buffer = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

    avpicture_fill((AVPicture*)pFrameRGB, buffer, AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);
    
    int frameFinished;
    AVPacket packet;
    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
      pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
      AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);

    i = 0;
    while(av_read_frame(pFormatCtx, &packet) >= 0)
    {
        //Is this a packet from the video stream?
        if (packet.stream_index==videoStream)
        {
          // Decode video frame
          avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
          // Did we get a video frame?
          if (frameFinished)
          {
#if 0
              //Convert the image from its native format to RGB
              img_convert((AVPicture *)pFrameRGB, AV_PIX_FMT_RGB24, (AVPicture*)pFrame, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
#else
              sws_scale(img_convert_ctx, (const uint8_t* const *)pFrame->data, pFrame->linesize,
                0, pCodecCtx->height, pFrameRGB->data, pFrameRGB->linesize);
#endif
              // Save the frame to disk
              if (++i<=5)
              {
                  SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height, i);
              }
          }
        }

        // Free the packet that was allocated by av read frame
        av_free_packet(&packet);
    }

    // Free the RGB image
    av_free(buffer);
    av_frame_unref(pFrameRGB);
    av_frame_free(pFrameRGB);
    av_frame_unref(pFrame);
    av_frame_free(pFrame);
    avcodec_close(pCodecCtx);
    //av_close_input_file(pFormatCtx);
    avformat_close_input(&pFormatCtx);
    return 0;
}
