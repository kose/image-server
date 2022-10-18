#include <iostream>
using std::cerr;
using std::endl;

#include <opencv2/opencv.hpp>
#include <zmq.hpp>

#include <gflags/gflags.h>

#include "image-server.hpp"

DEFINE_string(i, (std::string)HOME_DIR + "/GoogleDrive/movies/1024x600_shinyokohama2.mp4", "movie filename");

DEFINE_int32(width, 1024, "width of output image");
DEFINE_int32(height, 600, "width of output image"); // 1024x600: WSVGA(Wide-SVGA) 約16:10
DEFINE_double(rotate, 0.0, "rotate image");
DEFINE_double(scale, 1.0, "scale image");
DEFINE_int32(mx, 0, "move x axis");
DEFINE_int32(my, 0, "move y axis");


//
// main
//
int main(int argc, char *argv[])
{
  try {
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    ImageServe imageserve(FLAGS_i, FLAGS_width, FLAGS_height, FLAGS_rotate, FLAGS_scale, FLAGS_mx, FLAGS_my);

    for (int frame_number = 0; ;frame_number++) {

      if (imageserve.run() == false) {
        break;
      }

      if (frame_number % 1000 == 0) {
        cerr << "frame: " << frame_number << endl;
      }
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
