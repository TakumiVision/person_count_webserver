/*
 * This file is part of [Head Count Web Application].
 *
 * Copyright (C) 2025 TakumiVision co., ltd. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "person_counter.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>

PersonCounter::PersonCounter()
{
    // ログの初期化
    auto logger =
        spdlog::rotating_logger_mt("headcount_logger", "log.txt", 1048576, 5);
    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::info);

    // Inferenceクラスのインスタンスを作成
    inf =
        std::make_shared<Inference>("./model/yolov8x_head.onnx", // ONNXモデルのパス
                                    cv::Size(640, 640),    // モデルの入力サイズ
                                    "./model/classes.txt", // クラス名のファイルパス
                                    true                   // CUDAを使用するかどうか
        );
}

PersonCounter::~PersonCounter()
{
    // ログをフラッシュ
    spdlog::get("headcount_logger")->flush();
}

// 多角形を内包する矩形取得
static cv::Rect getTgtRect(std::vector<OBJPos> &vertices, int cam_width,
                           int cam_height)
{
    cv::Rect tgtrect;

    int ltx = cam_width;
    int lty = cam_height;
    int rbx = 0;
    int rby = 0;
    for (auto &&p : vertices) {
        if (ltx > p.x) {
            ltx = p.x;
        }
        if (lty > p.y) {
            lty = p.y;
        }
        if (rbx < p.x) {
            rbx = p.x;
        }
        if (rby < p.y) {
            rby = p.y;
        }
    }
    tgtrect.x = ltx;
    tgtrect.y = lty;
    tgtrect.width = rbx - ltx;
    tgtrect.height = rby - lty;

    return tgtrect;
}

/**
 * @brief JPEG画像から人物の頭部を検出します。
 *
 * @param jpegData      JPEG 形式の画像データ（バイナリ形式）
 * @param vertices      検出対象領域を示す多角形頂点の座標（OBJPos型の vector）
 * @param thresholds    検出処理に用いる各種しきい値パラメータ（構造体）
 *
 * @return              検出された頭部領域の矩形（Rect型）の vector
 */
std::vector<Rect> PersonCounter::detectHeads(std::vector<unsigned char> &jpegData,
                                             std::vector<OBJPos> &vertices,
                                             Thresholds &thresholds)
{
    cv::Mat img = cv::imdecode(jpegData, cv::IMREAD_COLOR);
    if (img.empty()) {
        spdlog::error("Failed to decode JPEG data.");
        return std::vector<Rect>();
    }

    int width = img.cols;
    int height = img.rows;

    cv::Rect tgtRect = getTgtRect(vertices, width, height);

    cv::Mat src = img(tgtRect).clone();

    // set thresholds
    inf->setThresholds(thresholds.confidenceThreshold, thresholds.scoreThreshold,
                       thresholds.nmsThreshold);

    // Inference starts here...
    std::vector<Detection> output = inf->runInference(src);

    int detections = output.size();
    spdlog::trace("Number of detections: {}", detections);

    std::vector<Rect> results;

    for (int i = 0; i < detections; ++i) {
        Detection detection = output[i];

        cv::Rect box = detection.box;

        Rect result;
        result.x = box.x + tgtRect.x; // Adjust for the cropped region
        result.y = box.y + tgtRect.y; // Adjust for the cropped region
        result.width = box.width;
        result.height = box.height;
        result.confidence = detection.confidence;
        // cv::Scalar color = detection.color;

        results.push_back(result);
    }

    return results;
}
