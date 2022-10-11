// Copyright (C) 2020-2022 KOSEKI Yoshinori
///
/// @file  pipeline-mapping.hpp
/// @brief パイプライン：地図上にプロット
/// @author KOSEKI Yoshinori <koseki.y@saxa.co.jp>
///

#pragma once

//
// イメージサーバー
//
class ImageServe {
public:

  // コンストラクタ
  ImageServe (const std::string moviefile, const int width, const int height,
              const double rotate, const double scale, const int mx, const int my) :
    width(width), height(height)
  {
    context = new zmq::context_t(1);
    socket = new zmq::socket_t(*context, ZMQ_REP);
    socket->bind("tcp://*:5555");

    capture.open(moviefile);

    if (!capture.isOpened()) {
      throw std::runtime_error("Can not open VideoCapture: ");
    }

    affine_matrix = cv::getRotationMatrix2D(cv::Point2f(0.0, 0.0), rotate, scale);

    affine_matrix.at<double>(0, 2) = affine_matrix.at<double>(0, 0) * mx + affine_matrix.at<double>(0, 1) * my + affine_matrix.at<double>(0, 2);
    affine_matrix.at<double>(1, 2) = affine_matrix.at<double>(1, 0) * mx + affine_matrix.at<double>(1, 1) * my + affine_matrix.at<double>(1, 2);

    cerr << affine_matrix << endl;
  }

  // デストラクタ
  ~ImageServe() {}

  // 1フレーム処理
  void run()
  {
    // receive pull request
    {
      zmq::message_t message_recv;

      // receive: data
      socket->recv(&message_recv);

      int size = message_recv.size();
      unsigned char buff[size];
      memcpy(&buff[0], (unsigned char*)message_recv.data(), size);

#if _DEBUG_00000
      std::cerr << size << ": ";
      std::cerr << buff << std::endl;
#endif
    }
      
    // pinput image

    cv::Mat image_in;
    capture >> image_in;

    if (image_in.empty()) {
      cerr << "rewind" << endl;
      capture.set(cv::CAP_PROP_POS_FRAMES, 0); // 巻き戻し
      capture >> image_in;
    }

    cv::Mat image_proc;
    cv::warpAffine(image_in, image_proc, affine_matrix, cv::Size(width, height)); // WSVGA(Wide-SVGA) 約16:10
    
#if _DEBUG_
    cv::imshow("server", image_in);

    if (cv::waitKey(1) == 27) {
      throw std::runtime_error("ESC: exit server");
    }
#endif
      
    // send JPEG stream
    {
      // make jpeg
      std::vector<unsigned char> buff;
      std::vector<int> param = std::vector<int>(2);
      param[0] = cv::IMWRITE_JPEG_QUALITY;
      param[1] = 90;                // default(95) 0-100
      imencode(".jpg", image_proc, buff, param);

      // send JPEG buffer
      zmq::message_t message_send(buff.size());
      memcpy(message_send.data(), &buff[0], buff.size());

      socket->send(message_send);
    }
  }
  
private:

  zmq::context_t *context;      ///< ZeroMQ context
  zmq::socket_t *socket;        ///< ZeroMQソケット

  cv::VideoCapture capture;     ///< ビデオキャプチャ
  cv::Mat affine_matrix;       ///< アフィン変換行列

  const int width;
  const int height;
};

/// Local Variables: ///
/// truncate-lines:t ///
/// End: ///
