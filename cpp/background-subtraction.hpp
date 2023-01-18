// Copyright (C) 2021, 2022 KOSEKI Yoshinori
///
/// @file  pipeline-BackgroundSubtraction.hpp
/// @brief 背景差分クラス
/// @author KOSEKI Yoshinori <koseki.y@saxa.co.jp>
///

#pragma once

#include <iostream>
#include <fstream>

#include <opencv2/opencv.hpp>


///
/// パイプライン：背景差分クラス
///
class BackgroundSubtraction {
public:

  /// コンストラクタ
  BackgroundSubtraction(const std::string filename_background) :
    filename_background(filename_background)
  {
    frame_number = 0;

    // model = cv::createBackgroundSubtractorKNN();
    model = cv::createBackgroundSubtractorMOG2();

    //
    // 背景画像初期値を使う
    //
    std::ifstream ifs(filename_background);

    if (ifs.is_open()) {
      image_background = cv::imread(filename_background);

      std::cerr << "load: " << filename_background << std::endl;

      // 初期背景を使ってモデルを初期化
      for (int i = 0; i < 30; i++) {
        frame_number = 0;       // FIXME: 毎回やるための(綺麗な実装じゃない）
        run(image_background);
        std::cerr << "+";
      }
      std::cerr << std::endl;
    }
    // std::cerr << "Construct: BackgroundSubtraction" << std::endl;
  }


  /// デストラクタ
  virtual ~BackgroundSubtraction()
  {
    if ( !image_background.empty() && !filename_background.empty()) {
      // スムージングして保存
      cv::blur(image_background, image_background, cv::Size(3, 3));
      cv::imwrite(filename_background, image_background);
      std::cerr << "save: " << filename_background << std::endl;
     }
    // std::cerr << "Destruct: BackgroundSubtraction" << std::endl;
  }


  cv::Mat& run(cv::Mat& image)
  {
    // 10回に1回だけ処理する (初期化時は毎回）
    if (frame_number++ % 10 != 0) {
      return image_background;
    }

    // pass the frame to background model
    model->apply(image, foregroundMask, doUpdateModel ? -1 : 0);

    // show foreground image and mask (with optional smoothing)
    if (doSmoothMask) {
      cv::GaussianBlur(foregroundMask, foregroundMask, cv::Size(11, 11), 3.5, 3.5);
      cv::threshold(foregroundMask, foregroundMask, 10, 255, cv::THRESH_BINARY);
    }

    if (image_foreground.empty()) {
      image_foreground.create(image.size(), image.type());
    }
    image_foreground = cv::Scalar::all(0);
    image.copyTo(image_foreground, foregroundMask);

    // show background image
    model->getBackgroundImage(image_background);

    return image_background;
  }


private:
  int frame_number;                       ///< フレーム番号
  const std::string filename_background; ///< backup BG image

  static const bool doSmoothMask = true;  ///< スムージング
  static const bool doUpdateModel = true; ///< 背景モデル更新

  cv::Ptr<cv::BackgroundSubtractor> model; ///< 差分モデル

  mutable cv::Mat foregroundMask;          ///< 前景マスク
  mutable cv::Mat image_foreground;        ///< 前景
  mutable cv::Mat image_background;        ///< 背景 FIXME:初期値：背景画像を読み込むようにする。
};

/// Local Variables: ///
/// truncate-lines:t ///
/// End: ///

