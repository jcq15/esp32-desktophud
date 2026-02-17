from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from datetime import datetime
import httpx
import os
from typing import Optional
from dotenv import load_dotenv

# 加载 .env 文件
load_dotenv()

app = FastAPI(title="DeskHUD Backend", version="1.0.0")

# 允许跨域请求
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# QWeather API 配置
QWEATHER_KEY = os.getenv("QWEATHER_KEY", "")
QWEATHER_BASE_URL = "https://devapi.qweather.com/v7"

# 默认位置（可以后续改为从配置或请求参数获取）
DEFAULT_LOCATION = os.getenv("DEFAULT_LOCATION", "101010100")  # 北京


async def get_weather(location: str = DEFAULT_LOCATION) -> Optional[dict]:
    """获取天气信息"""
    if not QWEATHER_KEY:
        return None
    
    try:
        async with httpx.AsyncClient(timeout=10.0) as client:
            # 获取实时天气
            url = f"{QWEATHER_BASE_URL}/weather/now"
            params = {
                "location": location,
                "key": QWEATHER_KEY
            }
            response = await client.get(url, params=params)
            response.raise_for_status()
            data = response.json()
            
            if data.get("code") == "200":
                return data.get("now", {})
            else:
                return None
    except Exception as e:
        print(f"获取天气信息失败: {e}")
        return None


@app.get("/")
async def root():
    """根路径"""
    return {"message": "DeskHUD Backend API"}


def get_sample_data():
    """返回示例测试数据"""
    now = datetime.now()
    
    return {
        "cal": {
            "date": now.strftime("%Y-%m-%d"),
            "weekday": now.strftime("%A"),
            "lunar": "Lunar 5th",  # 示例农历，后续可以集成农历转换
            "extra": "Minor Cold"
        },
        "wx": {
            "city": "Beijing",
            "t": 20,
            "text": "Sunny",
            "aqi": "45"
        },
        "note": [
            "Complete project documentation today",
            "Meeting at 3 PM",
            "Remember to buy milk",
            "Reply to client email"
        ]
    }


@app.get("/api/info")
async def get_info():
    """获取日期、时间、天气等信息"""
    # 返回写死的示例数据用于测试
    return get_sample_data()
    
    # 后续可以改为真实数据：
    # now = datetime.now()
    # weather = await get_weather()
    # 
    # weekday_map = {
    #     "Monday": "星期一",
    #     "Tuesday": "星期二",
    #     "Wednesday": "星期三",
    #     "Thursday": "星期四",
    #     "Friday": "星期五",
    #     "Saturday": "星期六",
    #     "Sunday": "星期日"
    # }
    # 
    # return {
    #     "cal": {
    #         "date": now.strftime("%Y-%m-%d"),
    #         "weekday": weekday_map.get(now.strftime("%A"), now.strftime("%A")),
    #         "lunar": "待实现",  # 需要集成农历转换库
    #         "extra": ""
    #     },
    #     "wx": {
    #         "city": "北京",
    #         "t": int(weather.get("temp", 0)) if weather else 0,
    #         "text": weather.get("text", "") if weather else "",
    #         "aqi": weather.get("aqi", "") if weather else ""
    #     },
    #     "note": []  # 笔记功能待实现
    # }


if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)

