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

#ifndef __PERSON_COUNTER_H__
#define __PERSON_COUNTER_H__
#include <string>
#include <vector>

#include <opencv2/opencv.hpp>

#include "inference.h"

// 頭部領域矩形データ構造体
struct Rect
{
    int x;            // 左上隅のX座標
    int y;            // 左上隅のY座標
    int width;        // 幅
    int height;       // 高さ
    float confidence; // 信頼度
    Rect() : x(0), y(0), width(0), height(0), confidence(0.0f)
    {
    }
    Rect(int x_, int y_, int w_, int h_, float c_)
        : x(x_), y(y_), width(w_), height(h_), confidence(c_)
    {
    }
};

// 頂点座標構造体
struct OBJPos
{
    int x; // X座標
    int y; // Y座標

    OBJPos()
    {
        x = 0, y = 0;
    }
    OBJPos(int x, int y)
    {
        this->x = x;
        this->y = y;
    }
};

// 頭部検出閾値構造体
struct Thresholds
{
    float confidenceThreshold; // 信頼度の閾値
    float scoreThreshold;      // スコアの閾値
    float nmsThreshold;        // 非最大抑制の閾値

    Thresholds(float conf = 0.2, float score = 0.2, float nms = 0.2)
        : confidenceThreshold(conf), scoreThreshold(score), nmsThreshold(nms)
    {
    }
};

class PersonCounter
{
  public:
    PersonCounter();
    ~PersonCounter();

    // 人物頭部検出実行
    std::vector<Rect> detectHeads(std::vector<unsigned char> &jpegData,
                                  std::vector<OBJPos> &vertices,
                                  Thresholds &thresholds);

  private:
    std::shared_ptr<Inference> inf; // yolov8 head detection class
};
#endif