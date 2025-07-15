#!/usr/bin/env python3
"""
This file is part of Head Count Web Application.

Copyright (C) 2025 TakumiVision co., ltd. All rights reserved.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
"""

import sys
import os
import socketserver
import ctypes
import json
import http.server
import logging
import logging.handlers
from datetime import datetime
from typing import Dict, List, Any, Union

PORT = 8000
APP_VERSION = "1.0.0"

# ログの設定
def setup_logging():
    """ログの設定を行う"""
    log_dir = os.path.join(os.path.abspath(os.path.dirname(__file__)), "logs")
    os.makedirs(log_dir, exist_ok=True)

    # ログファイルのパス
    log_file = os.path.join(log_dir, "person_count_server.log")

    # ログフォーマットの設定
    formatter = logging.Formatter(
        '%(asctime)s - %(name)s - %(levelname)s - %(message)s',
        datefmt='%Y-%m-%d %H:%M:%S'
    )

    # デイリーローテーションハンドラーの設定
    handler = logging.handlers.TimedRotatingFileHandler(
        filename=log_file,
        when='midnight',           # 毎日午前0時にローテート
        interval=1,                # 1日間隔
        backupCount=30,            # 30ファイルまで保持
        encoding='utf-8',
        delay=False,
        utc=False
    )
    handler.setFormatter(formatter)

    # コンソール出力ハンドラーの設定
    console_handler = logging.StreamHandler()
    console_handler.setFormatter(formatter)

    # ルートロガーの設定
    logger = logging.getLogger()
    logger.setLevel(logging.INFO)
    logger.addHandler(handler)
    logger.addHandler(console_handler)

    return logger

# ログ設定を初期化
logger = setup_logging()
logger.info("PersonCountServer starting up...")
logger.info(f"Log files will be saved to: {os.path.join(os.path.abspath(os.path.dirname(__file__)), 'logs')}")
logger.info(f"Log rotation: daily, keeping 30 files")

# カレントディレクトリに.soがある場合はパスを追加
sys.path.insert(0, os.path.abspath(os.path.dirname(__file__)))

# 依存ライブラリパスを追加（PersonCounterModule.soが./libの依存ライブラリを見つけられるように）
lib_path = os.path.join(os.path.abspath(os.path.dirname(__file__)), "lib")
if 'LD_LIBRARY_PATH' in os.environ:
    os.environ['LD_LIBRARY_PATH'] = lib_path + ":" + os.environ['LD_LIBRARY_PATH']
else:
    os.environ['LD_LIBRARY_PATH'] = lib_path

logger.info(f"Library path set to: {lib_path}")

# 重要な依存ライブラリを事前にロード
import ctypes
try:
    # OpenCVの主要ライブラリを事前ロード
    ctypes.CDLL(os.path.join(lib_path, "libopencv_core.so.411"), mode=ctypes.RTLD_GLOBAL)
    ctypes.CDLL(os.path.join(lib_path, "libopencv_imgproc.so.411"), mode=ctypes.RTLD_GLOBAL)
    ctypes.CDLL(os.path.join(lib_path, "libopencv_imgcodecs.so.411"), mode=ctypes.RTLD_GLOBAL)
    ctypes.CDLL(os.path.join(lib_path, "libopencv_highgui.so.411"), mode=ctypes.RTLD_GLOBAL)
    ctypes.CDLL(os.path.join(lib_path, "libopencv_dnn.so.411"), mode=ctypes.RTLD_GLOBAL)
    logger.info("✓ 依存ライブラリの事前ロード完了")
except OSError as e:
    logger.warning(f"依存ライブラリの事前ロードに失敗: {e}")
    logger.info("LD_LIBRARY_PATHでの解決を試行します...")

try:
    # C++で定義したクラスを含むモジュールのインポート
    from PersonCounterModule import Rect, OBJPos, Thresholds, PersonCounter
    logger.info("✓ PersonCounterModule のインポート完了")
except ImportError as e:
    logger.error(f"モジュールのimportに失敗しました: {e}")
    sys.exit(1)

# サーバー用の型定義クラス（C++バインディングとは別）
class Area:
    def __init__(self, id: int, name: str, vertexs: List[OBJPos]):
        self.id = id
        self.name = name
        self.vertexs = vertexs

    def to_objpos_list(self) -> List[OBJPos]:
        """頂点リストをC++のOBJPosリストに変換"""
        return self.vertexs

class DetectionParams:
    def __init__(self,
                 thresholds: Thresholds = None,
                 areas: List[Area] = None):
        self.thresholds = thresholds if thresholds is not None else Thresholds()
        self.areas = areas if areas is not None else []

# レスポンス用の型クラス
class AreaResult:
    """エリア毎の検出結果"""
    def __init__(self, area_id: int, name: str, count: int, results: List[Rect]):
        self.id = area_id
        self.name = name
        self.count = count
        self.results = results

    def to_dict(self) -> Dict[str, Any]:
        return {
            "id": self.id,
            "name": self.name,
            "count": self.count,
            "results": [result.to_dict() for result in self.results]
        }

class PersonCountResponse:
    """レスポンス全体の構造"""
    def __init__(self, error: str, area_results: List[AreaResult]):
        self.error = error
        self.area_results = area_results

    def to_dict(self) -> Dict[str, Any]:
        return {
            "error": self.error,
            "areas": [area_result.to_dict() for area_result in self.area_results]
        }

# インスタンス生成とメソッド呼び出し
obj: PersonCounter = PersonCounter()
logger.info("✓ PersonCounter インスタンス生成完了")

class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def log_message(self, format, *args):
        """HTTPサーバーのログメッセージをカスタムロガーにリダイレクト"""
        logger.info(f"{self.client_address[0]} - {format % args}")

    def parse_detection_params(self, data: Dict[str, Any]) -> DetectionParams:
        """
        検出パラメータをパースする
        {
            "modelConfidenceThreshold": 0.2,
            "modelScoreThreshold": 0.2,
            "modelNMSThreshold": 0.2,
            "areas": [
                { "id": 1, "name": "Area1", "vertexs": [ { "x": 10, "y": 20 }, ...]},
            ],
        }
        """
        logger.debug(f"パラメータ解析開始: {data}")

        # 必須フィールドのチェック
        required_fields = ["areas", "modelConfidenceThreshold", "modelScoreThreshold", "modelNMSThreshold"]
        for field in required_fields:
            if field not in data:
                logger.error(f"必須フィールド '{field}' が見つかりません")
                raise ValueError(f"{field} field is required")

        areas_data = data["areas"]

        # バリデーション
        if not isinstance(areas_data, list):
            logger.error("areas は配列である必要があります")
            raise ValueError("areas must be an array")

        areas = []
        for i, area_data in enumerate(areas_data):
            if not isinstance(area_data, dict):
                logger.error(f"エリア {i} はオブジェクトである必要があります")
                raise ValueError("Each area must be an object")
            if "id" not in area_data or "name" not in area_data or "vertexs" not in area_data:
                logger.error(f"エリア {i} に必須フィールドが不足しています")
                raise ValueError("Area must have id, name, and vertexs fields")
            if not isinstance(area_data["vertexs"], list):
                logger.error(f"エリア {i} の vertexs は配列である必要があります")
                raise ValueError("vertexs must be an array")

            vertexs: List[OBJPos] = []
            for j, vertex_data in enumerate(area_data["vertexs"]):
                if not isinstance(vertex_data, dict) or "x" not in vertex_data or "y" not in vertex_data:
                    logger.error(f"エリア {i} の頂点 {j} に座標が不足しています")
                    raise ValueError("Each vertex must have x and y coordinates")
                vertexs.append(OBJPos(vertex_data["x"], vertex_data["y"]))

            areas.append(Area(area_data["id"], area_data["name"], vertexs))
            logger.debug(f"エリア {area_data['id']} ({area_data['name']}) を追加: {len(vertexs)} 個の頂点")

        logger.info(f"パラメータ解析完了: {len(areas)} エリア")
        return DetectionParams(
            thresholds=Thresholds(
                data["modelConfidenceThreshold"],
                data["modelScoreThreshold"],
                data["modelNMSThreshold"]
            ),
            areas=areas
        )

    def do_GET(self):
        logger.info(f"GET request from {self.client_address[0]}: {self.path}")
        self.send_response(404)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(json.dumps({"error": "Not Found"}).encode("utf-8"))

    def handle_person_count(self):
        start_time = datetime.now()
        logger.info(f"Person count request started from {self.client_address[0]}")

        ctype = self.headers.get('Content-Type')
        if not (ctype and ctype.startswith('multipart/form-data')):
            logger.error("Invalid Content-Type, expected multipart/form-data")
            raise ValueError("Bad Request")

        import cgi
        form = cgi.FieldStorage(
            fp=self.rfile,
            headers=self.headers,
            environ={'REQUEST_METHOD': 'POST', 'CONTENT_TYPE': ctype}
        )

        # 画像データ
        if 'image' in form:
            image_data = form['image'].file.read()
            logger.info(f"画像データ受信: {len(image_data)} bytes")
        else:
            logger.error("画像データが見つかりません")
            raise ValueError("Missing image data")

        # JSONデータ
        if 'rect' in form:
            rect_json = form['rect'].value
            logger.debug(f"検出パラメータ受信: {rect_json}")
        else:
            logger.error("検出パラメータが見つかりません")
            raise ValueError("Missing rect data")

        try:
            rect_data = json.loads(rect_json)
        except json.JSONDecodeError as e:
            logger.error(f"JSONデコードエラー: {e}")
            raise ValueError("Invalid rect JSON")

        # パラメータの解析
        params = self.parse_detection_params(rect_data)

        # 各エリアで頭部検出実行
        area_results: List[AreaResult] = []
        total_detections = 0

        if params.areas:
            # 指定されたエリア毎に検出
            for area in params.areas:
                logger.info(f"エリア {area.id} ({area.name}) の検出開始")
                area_start_time = datetime.now()

                detection_rects: List[Rect] = obj.detectHeads(image_data, area.vertexs, params.thresholds)

                area_end_time = datetime.now()
                area_duration = (area_end_time - area_start_time).total_seconds()

                logger.info(f"エリア {area.id} ({area.name}) の検出完了: {len(detection_rects)} 個検出 ({area_duration:.3f}秒)")
                total_detections += len(detection_rects)

                # C++のRectをそのまま使用
                area_results.append(AreaResult(
                    area_id=area.id,
                    name=area.name,
                    count=len(detection_rects),
                    results=detection_rects
                ))

        # レスポンス作成
        response = PersonCountResponse(
            error="OK",
            area_results=area_results
        )

        end_time = datetime.now()
        duration = (end_time - start_time).total_seconds()
        logger.info(f"Person count request completed: {total_detections} 個検出, 処理時間: {duration:.3f}秒")

        # レスポンス送信
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.end_headers()
        self.wfile.write(json.dumps(response.to_dict()).encode("utf-8"))

    def do_POST(self):
        try:
            if self.path.startswith('/person_count'):
                self.handle_person_count()
            else:
                logger.warning(f"Unknown POST endpoint: {self.path}")
                raise FileNotFoundError()
        except FileNotFoundError:
            logger.error(f"404 Not Found: {self.path}")
            self.send_response(404)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps({"error": "Not Found"}).encode("utf-8"))
        except ValueError as e:
            logger.error(f"400 Bad Request: {e}")
            self.send_response(400)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps({"error": str(e)}).encode("utf-8"))
        except Exception as e:
            logger.error(f"500 Internal Server Error: {e}", exc_info=True)
            self.send_response(500)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps({"error": "Internal Server Error", "message": str(e)}).encode("utf-8"))

if __name__ == "__main__":
    try:
        logger.info(f"Person Count Server Version {APP_VERSION}")
        with socketserver.TCPServer(("", PORT), Handler) as httpd:
            logger.info(f"Server starting on http://localhost:{PORT}")
            logger.info(f"Press Ctrl+C to stop the server")
            httpd.serve_forever()
    except KeyboardInterrupt:
        logger.info("Server stopped by user")
    except Exception as e:
        logger.error(f"Server error: {e}", exc_info=True)
        sys.exit(1)