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

#include <fstream>
#include <vector>
#include <iostream>
#include <string>
#include <memory>

#include <opencv2/opencv.hpp>

int main(int argc, char *argv[])
{
    std::string imagepath;
    // コマンドライン引数の処理
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--version") {
            std::cout << "Person Counter Version 1.0" << std::endl;
            return 0;
        }
        else if (arg == "--help") {
            std::cout << "Usage: person_counter [--version | --help | imagepath]"
                      << std::endl;
            return 0;
        }
        else {
            imagepath = arg;
            std::cout << "Image path provided: " << imagepath << std::endl;
        }
    }

    // メインの処理

    // jpeg画像の読み込み
    std::ifstream file(imagepath, std::ios::binary);
    if (!file) {
        std::cerr << "can not open file: " << imagepath << std::endl;
        return 1;
    }

    // ファイルサイズを取得
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    // バッファを確保して読み込む
    std::vector<unsigned char> jpgdata(size);
    if (file.read(reinterpret_cast<char *>(jpgdata.data()), size)) {
        std::cout << "Loading successful. Bytes: " << size << std::endl;
    }
    else {
        std::cerr << "Loading failed." << std::endl;
    }

    // 画像サイズの取得
    cv::Mat img = cv::imdecode(jpgdata, cv::IMREAD_COLOR);
    if (img.empty()) {
        std::cerr << "Failed to decode image." << std::endl;
        return 1;
    }
    int width = img.cols;
    int height = img.rows;

    // 対象領域の頂点座標を設定
    std::vector<OBJPos> vertices;
    vertices.emplace_back(0, 0);          // 左上
    vertices.emplace_back(width, 0);      // 右上
    vertices.emplace_back(width, height); // 右下
    vertices.emplace_back(0, height);     // 左下

    // 閾値の設定
    Thresholds thresholds(0.2f, 0.2f, 0.2f);

    // 頭部検出実行
    std::cout << "Person Counter is running..." << std::endl;
    auto personCounter = std::make_shared<PersonCounter>();
    std::vector<Rect> heads =
        personCounter->detectHeads(jpgdata, vertices, thresholds);

    for (auto &&head : heads) {
        cv::rectangle(img, cv::Point(head.x, head.y),
                      cv::Point(head.x + head.width, head.y + head.height),
                      cv::Scalar(0, 255, 0), 2);
        char text[256];
        snprintf(text, sizeof(text), "%.2f", head.confidence);
        cv::putText(img, text, cv::Point(head.x, head.y - 10),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 0), 1);
    }

    cv::imwrite("output.jpg", img);
    std::cout << "Output saved to output.jpg" << std::endl;

    return 0;
}