#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <SDL.h>
#include <SDL_thread.h>

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
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
    {
        fprintf(stderr, "Could not initialize SDL= %s\n", SDL_GetError());
        exit(1);
    }

    SDL_Surface *screen;

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

    screen = SDL_SetVideoMode(pCodecCtx->width, pCodecCtx->height, 0, 0);
    if (!screen)
    {
        fprintf(stderr, "SDL: could not set video mode - exiting\n");
        exit(1);
    }

    SDL_Overlay *bmp;
    bmp = SDL_CreateYUVOverlay(pCodecCtx->width, pCodecCtx->height, SDL_YV12_OVERLAY, screen);

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
    //AVFrame *pFrameRGB = av_frame_alloc();
    //if (pFrameRGB == NULL)
    //{
    //    return -1;
    //}

    int frameFinished;
    AVPacket packet;
    SDL_Event event;
    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height,
      pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height,
      AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

    i = 0;
    while(av_read_frame(pFormatCtx, &packet) >= 0)
    {
        //Is this a packet from the video stream?
        if (packet.stream_index==videoStream)
        {
          // Decode video frame
          avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
          // Did we get a video frame?

          SDL_Rect rect;
          if (frameFinished)
          {
              SDL_LockYUVOverlay(bmp);
              AVPicture pict;
              pict.data[0] = bmp->pixels[0];
              pict.data[1] = bmp->pixels[2];
              pict.data[2] = bmp->pixels[1];

              pict.linesize[0] = bmp->pitches[0];
              pict.linesize[1] = bmp->pitches[2];
              pict.linesize[2] = bmp->pitches[1];

#if 0
              //Convert the image from its native format to RGB
              img_convert((AVPicture *)pFrameRGB, AV_PIX_FMT_RGB24, (AVPicture*)pFrame, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
#else
              sws_scale(img_convert_ctx, (const uint8_t* const *)pFrame->data, pFrame->linesize,
                0, pCodecCtx->height, pict.data, pict.linesize);
#endif

              SDL_UnlockYUVOverlay(bmp);

              rect.x = 0;
              rect.y = 0;
              rect.w = pCodecCtx->width;
              rect.h = pCodecCtx->height;
              SDL_DisplayYUVOverlay(bmp, &rect);
          }
        }

        // Free the packet that was allocated by av read frame
        av_free_packet(&packet);

        SDL_PollEvent(&event);
        switch(event.type)
        {
        case SDL_QUIT:
            SDL_Quit();
            exit(0);
            break;
        default:
            break;
        }
    }

    // Free the RGB image
    av_free(pFrame);
    avcodec_close(pCodecCtx);
    avformat_close_input(&pFormatCtx);
    return 0;
}
