// Copyright (C) 2020-2023 KOSEKI Yoshinori
///
/// @file  image-server.hpp
/// @brief 動画像サーバ
/// @author KOSEKI Yoshinori <koseki.y@saxa.co.jp>
///

#pragma once

#include <cstring>
#include "background-subtraction.hpp"
#include "inference-engine/hpe_model_openpose.h"
#include "inference-engine/human-pose-estimation.hpp"

//
// イメージサーバー
//
class ImageServe {
public:

  // コンストラクタ
  ImageServe (const std::string port, const bool loop, const bool flip, const std::string moviefile, const int width, const int height,
              const double rotate, const double scale, const int mx, const int my, int start_frame, int end_frame) :
    loop(loop), flip(flip), width(width), height(height), start_frame(start_frame), end_frame(end_frame), num_queue(1)
  {
    context = new zmq::context_t(1);
    socket = new zmq::socket_t(*context, ZMQ_REP);
    socket->bind("tcp://*:" + port);

    capture.open(moviefile);

    if (!capture.isOpened()) {
      throw std::runtime_error("Can not open VideoCapture: ");
    }

    capture.set(cv::CAP_PROP_POS_FRAMES, start_frame); // 巻き戻し

    capture >> image_in;
    
    affine_matrix = cv::getRotationMatrix2D(cv::Point2f(0.0, 0.0), rotate, scale);

    affine_matrix.at<double>(0, 2) = affine_matrix.at<double>(0, 0) * mx + affine_matrix.at<double>(0, 1) * my + affine_matrix.at<double>(0, 2);
    affine_matrix.at<double>(1, 2) = affine_matrix.at<double>(1, 0) * mx + affine_matrix.at<double>(1, 1) * my + affine_matrix.at<double>(1, 2);

    cerr << affine_matrix << endl;

    frame_number = start_frame;

    backgroundsubtraction = new BackgroundSubtraction("");

    detector_bone = new BoneDetection(core, "CPU", (std::string)HOME_DIR + "/openvino/IR/FP16/human-pose-estimation-0001.xml", num_queue);
  }

  // デストラクタ
  ~ImageServe() {
    delete backgroundsubtraction;
    delete detector_bone;
  }

  //
  // モード: C++ に enemerate ってないの？
  //
  int get_mode(const char* str)
  {
    std::vector<std::string> mode_table {"foreground", "background", "bonedetect", "boneBlob"};

    for (int i = 0; i <  mode_table.size(); i++) {
      if (std::strcmp(str, mode_table.at(i).c_str()) == 0) {
        return i;
      }
    }
    return 0; // if none, ret 0
  }

  // 1フレーム処理
  bool run()
  {
    int mode = -1;

    float pafsBlob[HPEOpenPose::n_pafs];        ///< keypoint pairwise relations (part affinity fields)
    float heatMapsBlob[HPEOpenPose::n_heatMap]; ///< keypoint heatmaps
    HPEOpenPose hpe("");

    // receive pull request
    {
      zmq::message_t message_recv;

      // receive: data
      socket->recv(&message_recv);

      int size = message_recv.size();
      char buff[size];
      memcpy(&buff[0], (unsigned char*)message_recv.data(), size);
      mode = get_mode(buff);
    }

    //
    // input image
    //
    if (image_in.empty() || mode == get_mode("foreground") || mode == get_mode("bonedetect")) {

      if (image_in.empty() || frame_number > end_frame) {
        if (loop) {
          // cerr << "rewind " << frame_number << endl;
          capture.set(cv::CAP_PROP_POS_FRAMES, start_frame); // 巻き戻し
          capture >> image_in;
        } else {
          // cerr << "------ end of movie: " << frame_number <<  " ----- " << endl;
          // send JPEG buffer (empty image)
          zmq::message_t message_send(1);
          socket->send(message_send);
          return false;
        }
      }

      //
      // アフィン変換
      //
      cv::warpAffine(image_in, image_proc, affine_matrix, cv::Size(width, height)); // WSVGA(Wide-SVGA) 約16:10

      if (flip) {
        cv::flip(image_proc, image_proc, 1);
      }

      image_send = image_proc;
    }

    // bonedetect
    if (mode == get_mode("bonedetect")) {
      detector_bone->Inference(image_proc);
      image_send = detector_bone->getResults(pafsBlob, heatMapsBlob);
    }

    // background
    if (mode == get_mode("background")) {
      image_send = backgroundsubtraction->run(image_proc);
    }

    //
    // send JPEG stream: foreground, background, bonedetect
    //

    // make jpeg
    std::vector<unsigned char> buff;
    std::vector<int> param = std::vector<int>(2);
    param[0] = cv::IMWRITE_JPEG_QUALITY;
    param[1] = 90;                // default(95) 0-100
    imencode(".jpg", image_send, buff, param);

    // make JPEG buffer
    zmq::message_t message_send(buff.size());
    memcpy(message_send.data(), &buff[0], buff.size());

    // send JPEG
    if (mode == get_mode("foreground") || mode == get_mode("background")) {
      socket->send(message_send);
    }

    // send JPEG, detect bone heatmap
    if (mode == get_mode("bonedetect")) {

      socket->send(message_send, ZMQ_SNDMORE); // 続く（マルチパート）

      {
        int length = HPEOpenPose::n_pafs * sizeof(float);
        zmq::message_t message_send(length);
        memcpy(message_send.data(), &pafsBlob[0], length);
        socket->send(message_send, ZMQ_SNDMORE); // 続く（マルチパート）
      }
      {
        int length = HPEOpenPose::n_heatMap * sizeof(float);
        zmq::message_t message_send(length);
        memcpy(message_send.data(), &heatMapsBlob[0], length);
        socket->send(message_send);
      }
    }

#if 0
    if (mode == get_mode("bonedetect")) {

      cv::Mat image_bone = image_send.clone();

      std::vector<HumanPose> poses = hpe.extractPoses(pafsBlob, heatMapsBlob, image_proc.cols, image_proc.rows);
      hpe.renderHumanPose(poses, image_bone);

      cv::imshow("server bone", image_bone);
    } else {
      cv::imshow("server foreground", image_proc);
    }

    if (cv::waitKey(1) == 27) {
      throw std::runtime_error("ESC: exit server");
    }
#endif


    if (mode != get_mode("background")) {
      capture >> image_in;
      frame_number++;
    }
    
    return true;
  }

private:

  zmq::context_t *context;      ///< ZeroMQ context
  zmq::socket_t *socket;        ///< ZeroMQソケット

  cv::VideoCapture capture;     ///< ビデオキャプチャ
  cv::Mat image_in;             ///< 入力画像
  cv::Mat image_proc;           ///< アフィン変換後画像
  cv::Mat image_send;           ///< 送信画像
  cv::Mat affine_matrix;        ///< アフィン変換行列

  const bool loop;              ///< ループするか?
  const bool flip;              ///< フリップするか?
  const int width;              ///< 出力画像幅
  const int height;             ///< 出力画像高さ
  const int start_frame;        ///< スタートフレーム
  const int end_frame;          ///< エンドフレーム
  int num_queue;                ///< OpenVINO推論のキューの数

  int frame_number;             ///< 出力フレーム番号

  ov::Core core;                ///< OpenVINO core
  BoneDetection* detector_bone; ///< DNN bone Detector
  BackgroundSubtraction *backgroundsubtraction; ///< 背景差分
};

/// Local Variables: ///
/// truncate-lines:t ///
/// End: ///
