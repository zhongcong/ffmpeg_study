# ffmpeg_study
1. 依赖库
   1.1 依赖ffmpeg版本ffmpeg-4.3.1
   1.2 编译步骤：
       1.2.1 sudo ./configure --enable-shared
       1.2.2 sudo make
       1.2.3 sudo make install

2. 编译
   2.1 export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib
   2.2 gcc -o tutorial1 -g ffmpeg_test1_new.c  -lavutil -lavformat -lavcodec -lswresample -lswscale -lavutil -lz -lm
   2.3 gcc -o tutorial2 decode_sdl.c  -lavutil -lavformat -lavcodec -lswscale -lavutil -lz -lm `pkg-config --cflags --libs sdl`
