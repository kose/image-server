// Copyright (C) 2020-2023 KOSEKI Yoshinori
///
/// @file  image-server.cpp
/// @brief 動画像サーバ
/// @author KOSEKI Yoshinori <koseki.y@saxa.co.jp>
///

#include <iostream>
using std::cerr;
using std::endl;

#include <zmq.hpp>
#include <opencv2/opencv.hpp>

#include "image-server.hpp"

#include <gflags/gflags.h>

DEFINE_string(port, "5555", "ZMQ server port number");
DEFINE_string(i, (std::string)HOME_DIR + "/GoogleDrive/movies/1024x600_shinyokohama2.mp4", "movie filename");
DEFINE_bool(loop, false, "Do loop?");
DEFINE_bool(flip, false, "Do flip?");

DEFINE_int32(width, 1024, "width of output image");
DEFINE_int32(height, 600, "width of output image"); // 1024x600: WSVGA(Wide-SVGA) 約16:10
DEFINE_double(rotate, 0.0, "rotate image");
DEFINE_double(scale, 1.0, "scale image");
DEFINE_int32(mx, 0, "move x axis");
DEFINE_int32(my, 0, "move y axis");
DEFINE_int32(start, 0, "start frame");
DEFINE_int32(frames, 1000000, "totale frames");


//
// main
//
int main(int argc, char *argv[])
{
  try {
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    ImageServe imageserve(FLAGS_port, FLAGS_loop, FLAGS_flip, FLAGS_i, FLAGS_width, FLAGS_height, FLAGS_rotate, FLAGS_scale,
                          FLAGS_mx, FLAGS_my, FLAGS_start, FLAGS_frames);

  
    for (int frame_number = 0; ;frame_number++) {

      if (imageserve.run() == false) {
        break;
      }
#if 0
      if (frame_number % 100 == 0) {
        cerr << "frame: " << frame_number << endl;
      }
#endif
    }
    
    return EXIT_SUCCESS;
  }

  // -*- error -*- //

  catch (std::bad_alloc &e) {
    std::cerr << "BAD ALLOC Exception : " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  catch (const std::exception& e) {
    std::cerr << "Error: "  << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  catch (...) {
    std::cerr << "unknown exception" << std::endl;
    return EXIT_FAILURE;
  }
}

/// Local Variables: ///
/// truncate-lines:t ///
/// End: ///
