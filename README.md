# 人数カウントWebアプリ

## アプリケーション構成

### C++実装
- **コア実装**: `src/person_counter.cpp`, `src/person_counter.h`
- **推論エンジン**: `src/inference.cpp`, `src/inference.h`
- **C++実行バイナリ**: `src/main.cpp` → `PersonCounting`（単体実行用）
- **Pythonバインディング**: `src/person_counter_bind.cpp`（pybind11で.so生成）

### Python実装
- **WebAPIサーバー**: `PersonCountServer.py`（Python標準ライブラリでREST API実装）
- **バインディングライブラリ**: `PersonCounterModule.so`（C++からコンパイル）


### 依存関係
- **モデルファイル**: `model/`

---

## PersonCountServer WebAPI 仕様

### エンドポイント
- `POST /person_count`
- Content-Type: multipart/form-data

### パラメータ
- `image`: JPEG画像ファイル (バイナリ)
- `rect`: 検出パラメータ(JSON文字列)
  - `modelConfidenceThreshold`: モデル信頼度閾値 (float)
  - `modelScoreThreshold`: スコア閾値 (float)
  - `modelNMSThreshold`: NMS閾値 (float)
  - `areas`: 検出エリア配列
    - `id`: エリアID (int)
    - `name`: エリア名 (string)
    - `vertexs`: 頂点座標配列 (x, y座標のリスト)

### リクエスト例 (curl)
```bash
curl -X POST http://localhost:8000/person_count \
  -F "image=@image.jpg;type=image/jpeg" \
  -F "rect={
    \"modelConfidenceThreshold\": 0.5,
    \"modelScoreThreshold\": 0.5,
    \"modelNMSThreshold\": 0.4,
    \"areas\": [
      {
        \"id\": 1,
        \"name\": \"TestArea\",
        \"vertexs\": [
          {\"x\": 0, \"y\": 0},
          {\"x\": 1279, \"y\": 0},
          {\"x\": 1279, \"y\": 719},
          {\"x\": 0, \"y\": 719}
        ]
      }
    ]
  };type=application/json"
```

### レスポンス例 (成功)
```json
{
  "error": "OK",
  "areas": [
    {
      "id": 1,
      "name": "TestArea",
      "count": 3,
      "results": [
        {
          "x": 100,
          "y": 200,
          "width": 50,
          "height": 80,
          "confidence": 0.95
        },
        {
          "x": 300,
          "y": 150,
          "width": 45,
          "height": 75,
          "confidence": 0.87
        },
        {
          "x": 500,
          "y": 250,
          "width": 48,
          "height": 78,
          "confidence": 0.82
        }
      ]
    }
  ]
}
```

### 複数エリア対応例
```json
{
  "error": "OK",
  "areas": [
    {
      "id": 1,
      "name": "LeftArea",
      "count": 2,
      "results": [...]
    },
    {
      "id": 2,
      "name": "RightArea",
      "count": 1,
      "results": [...]
    }
  ]
}
```

### エラーレスポンス例
- **400 Bad Request**: パラメータ不足や不正なJSON
```json
{
  "error": "Missing required field: areas"
}
```

- **404 Not Found**: 不正なパス
```json
{
  "error": "Not Found"
}
```

- **500 Internal Server Error**: サーバ内部エラー
```json
{
  "error": "Internal Server Error",
  "message": "...詳細..."
}
```

---

## ログ機能

### ログファイル
- **場所**: `logs/person_count_server.log`
- **ローテーション**: 毎日午前0時に自動ローテート
- **保持期間**: 最新30ファイルを保持、古いファイルは自動削除
- **フォーマット**: `YYYY-MM-DD HH:MM:SS - logger名 - レベル - メッセージ`

### ログレベル
- **INFO**: 通常動作（サーバー起動、リクエスト処理、レスポンス）
- **WARNING**: 警告（不明なエンドポイント、パラメータ不備）
- **ERROR**: エラー（バリデーション失敗、内部エラー）
- **DEBUG**: デバッグ情報（詳細パラメータ、処理時間）

---
## 実行方法

### サーバー起動
```bash
python PersonCountServer.py
```

### C++単体実行
```bash
./PersonCounting [jpg画像ファイル]
```

## ライセンス

このプロジェクトは GNU Affero General Public License v3 (AGPL v3) の下で公開されています。
詳細は [LICENSE](LICENSE) ファイルを参照してください。

### AGPL v3 の重要な条件

このソフトウェアを利用する場合、以下の条件に同意するものとします：

1. **ネットワークサービス提供時**:
   Web API やネットワーク経由でこのソフトウェアを利用する場合、利用者に対してソースコード（モデルファイル含む）へのアクセス権を提供する必要があります。

2. **ソース提供**:
   本リポジトリで提供されているソースコード、および変換前のモデル形式へのアクセスを継続的に提供してください。

3. **派生物の扱い**:
   このコードを改変・拡張する場合、派生物も同様に AGPL v3 で公開する必要があります。

