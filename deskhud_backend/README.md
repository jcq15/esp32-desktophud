# DeskHUD Backend

简单的 FastAPI 后端服务，提供日期、时间和天气信息。

## 安装

```bash
pip install -r requirements.txt
```

## 配置

1. 复制 `.env.example` 为 `.env`
2. 在 [和风天气开发者平台](https://dev.qweather.com/) 注册并获取免费 API Key
3. 将 API Key 填入 `.env` 文件中的 `QWEATHER_KEY`

## 运行

```bash
# 开发模式
uvicorn main:app --reload

# 生产模式
uvicorn main:app --host 0.0.0.0 --port 8000
```

## API 端点

### GET /api/info

返回日期、时间、天气等信息。

**响应示例：**
```json
{
  "date": "2024-01-15",
  "time": "14:30:00",
  "datetime": "2024-01-15T14:30:00",
  "weekday": "Monday",
  "weather": {
    "temp": "20",
    "text": "晴",
    "feelsLike": "18",
    "humidity": "65",
    "windDir": "东北",
    "windScale": "2"
  },
  "timestamp": 1705297800
}
```

## 注意事项

- 免费版 QWeather API 有调用频率限制，但 5-60 分钟一次的频率完全足够
- 如果没有配置 API Key，天气信息将返回 `null`

