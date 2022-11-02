// Copyright (C) 2020-2022 KOSEKI Yoshinori
///
/// @file  pipeline-mapping.hpp
/// @brief パイプライン：地図上にプロット
/// @author KOSEKI Yoshinori <koseki.y@saxa.co.jp>
///

#pragma once

#include <cstring>
#include "background-subtraction.hpp"

//
// イメージサーバー
//
class ImageServe {
public:

  // コンストラクタ
  ImageServe (const std::string port, const bool loop, const std::string moviefile, const int width, const int height,
              const double rotate, const double scale, const int mx, const int my, int start, int frames) :
    loop(loop), width(width), height(height), start(start), frames(frames)
  {
    context = new zmq::context_t(1);
    socket = new zmq::socket_t(*context, ZMQ_REP);
    socket->bind("tcp://*:" + port);

    capture.open(moviefile);

    if (!capture.isOpened()) {
      throw std::runtime_error("Can not open VideoCapture: ");
    }

    capture.set(cv::CAP_PROP_POS_FRAMES, start); // 巻き戻し

    affine_matrix = cv::getRotationMatrix2D(cv::Point2f(0.0, 0.0), rotate, scale);

    affine_matrix.at<double>(0, 2) = affine_matrix.at<double>(0, 0) * mx + affine_matrix.at<double>(0, 1) * my + affine_matrix.at<double>(0, 2);
    affine_matrix.at<double>(1, 2) = affine_matrix.at<double>(1, 0) * mx + affine_matrix.at<double>(1, 1) * my + affine_matrix.at<double>(1, 2);

    cerr << affine_matrix << endl;

    frame_number = 0;
    
   backgroundsubtraction = new BackgroundSubtraction("");
  }

  // デストラクタ
  ~ImageServe() {
    delete backgroundsubtraction;
  }

  // 1フレーム処理
  bool run()
  {
    background = false;
    
    // receive pull request
    {
      zmq::message_t message_recv;

      // receive: data
      socket->recv(&message_recv);

      int size = message_recv.size();
      char buff[size];
      memcpy(&buff[0], (unsigned char*)message_recv.data(), size);

      if (std::strcmp(buff, "background") == 0) {
        background = true;
        // cerr << "request: " << buff << endl;
      }
    }

    //
    // input image
    //
    if (background == false || image_in.empty()) {

      capture >> image_in;

      if (image_in.empty() || frame_number > frames) {
        if (loop) {
          // cerr << "rewind " << frame_number << endl;
          capture.set(cv::CAP_PROP_POS_FRAMES, start); // 巻き戻し
          frame_number = 0;
          capture >> image_in;
        } else {
          cerr << "------ end of movie -----" << endl;
          // send JPEG buffer
          zmq::message_t message_send(1);
          socket->send(message_send);
          return false;
        }
      }
  
      //
      // アフィン変換
      //
      cv::warpAffine(image_in, image_proc, affine_matrix, cv::Size(width, height)); // WSVGA(Wide-SVGA) 約16:10
    }

    //
    // send JPEG stream
    //
    cv::Mat image_send;

    if (background) {
      image_send = backgroundsubtraction->run(image_proc);
    } else {
      image_send = image_proc;
    }
      
    // make jpeg
    std::vector<unsigned char> buff;
    std::vector<int> param = std::vector<int>(2);
    param[0] = cv::IMWRITE_JPEG_QUALITY;
    param[1] = 90;                // default(95) 0-100
    imencode(".jpg", image_send, buff, param);

    // send JPEG buffer
    zmq::message_t message_send(buff.size());
    memcpy(message_send.data(), &buff[0], buff.size());

    socket->send(message_send);

#if _DEBUG_
    cv::imshow("server", image_send);

    if (cv::waitKey(1) == 27) {
      throw std::runtime_error("ESC: exit server");
    }
#endif

    frame_number++;
    
    return true;
  }

private:

  zmq::context_t *context;      ///< ZeroMQ context
  zmq::socket_t *socket;        ///< ZeroMQソケット

  cv::VideoCapture capture;     ///< ビデオキャプチャ
  cv::Mat image_in;             ///< 入力画像
  cv::Mat image_proc;           ///< アフィン変換後画像
   cv::Mat affine_matrix;       ///< アフィン変換行列

  const bool loop;
  const int width;
  const int height;
  const int start;
  const int frames;

  int frame_number; 
  
   BackgroundSubtraction *backgroundsubtraction; ///< 背景差分
  bool background;                               ///< 背景シェアフラグ
};

/// Local Variables: ///
/// truncate-lines:t ///
/// End: ///
