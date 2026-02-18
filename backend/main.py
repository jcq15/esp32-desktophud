from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from datetime import datetime
import httpx
import os
import random
from typing import Optional
from dotenv import load_dotenv
from PIL import Image, ImageDraw, ImageFont, ImageOps
import base64

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
        "wx_ver": random.randint(100, 999),
        "cal_ver": random.randint(100, 999),
        "note_ver": random.randint(100, 999),
        "wx": {
            "city": "北京",
            "t": random.randint(10, 30),
            "text": random.choice(["多云", "晴", "阴", "雪"]),
            "aqi": random.choice(["Good(58)", "Moderate(100)", "Unhealthy(150)", "Very Unhealthy(200)", "Hazardous(300)"]),
        },
        "cal": {
            "date": now.strftime("%Y-%m-%d"),
            "weekday": now.strftime("%A"),
            "lunar": "农历 1 月 1 日",  # 示例农历，后续可以集成农历转换
            "extra": "春节"
        },
        "note": [
            "喝水",
            "设计外壳",
            f'我想吃 {random.choice(["苹果", "香蕉", "橙子", "梨", "菠萝"])}'
        ],
        #"ttl": 300
    }


def load_chinese_font(size: int, bold: bool = False):
    """加载思源宋体字体"""
    font_path = os.path.join(os.path.dirname(__file__), "fonts", "SourceHanSerifSC-VF.ttf")
    
    if os.path.exists(font_path):
        try:
            return ImageFont.truetype(font_path, size)
        except Exception as e:
            print(f"警告: 加载字体失败 {e}，使用默认字体")
            return ImageFont.load_default()
    else:
        print(f"警告: 字体文件不存在 {font_path}，使用默认字体")
        return ImageFont.load_default()


def image_to_1bit_bitmap(img: Image.Image, invert: bool = False) -> bytes:
    """
    将PIL Image转换为1bit位图的原始数据（用于ESP32 drawBitmap）
    
    Args:
        img: PIL Image对象（必须是'1'模式）
        invert: 是否反转颜色（PIL的'1'模式：0=黑色，1=白色；ESP32可能需要反转）
    
    Returns:
        1bit位图的原始字节数据（每个字节8个像素，从左到右，从上到下）
    """
    # 确保是1bit模式
    if img.mode != '1':
        img = img.convert('1')
    
    # 如果需要反转颜色（ESP32端可能需要）
    if invert:
        img = ImageOps.invert(img)
    
    # 获取位图原始数据
    # PIL的'1'模式：每个字节8个像素，从左到右，从上到下
    # 宽度会自动向上取整到8的倍数
    return img.tobytes()


def render_cal_bitmap(cal_data: dict, width: int = 496, height: int = 120) -> bytes:
    """渲染日历信息为原始1-bit位图数据（用于ESP32 drawBitmap）"""
    img = Image.new('1', (width, height), 1)
    draw = ImageDraw.Draw(img)
    
    # 使用中文字体（适配496x120尺寸）
    font_large = load_chinese_font(32, bold=True)
    font_medium = load_chinese_font(24)
    font_small = load_chinese_font(20)
    
    y = 10
    draw.text((10, y), cal_data.get("date", ""), font=font_large, fill=0)
    y += 40
    draw.text((10, y), cal_data.get("weekday", ""), font=font_medium, fill=0)
    y += 30
    lunar = cal_data.get("lunar", "")
    extra = cal_data.get("extra", "")
    if lunar:
        draw.text((10, y), lunar, font=font_small, fill=0)
    if extra:
        draw.text((250, y), extra, font=font_small, fill=0)
    
    # 返回原始位图数据（1-bit位图的字节数组）
    # 注意：PIL的'1'模式中，0=黑色，1=白色
    # 如果ESP32端需要反转，可以在image_to_1bit_bitmap中设置invert=True
    return image_to_1bit_bitmap(img, invert=False)


def render_wx_bitmap(wx_data: dict, width: int = 304, height: int = 176) -> bytes:
    """渲染天气信息为原始1-bit位图数据（用于ESP32 drawBitmap）"""
    img = Image.new('1', (width, height), 1)
    draw = ImageDraw.Draw(img)
    
    font_large = load_chinese_font(36, bold=True)
    font_medium = load_chinese_font(24, bold=True)
    font_small = load_chinese_font(18)
    
    y = 10
    draw.text((10, y), wx_data.get("city", ""), font=font_medium, fill=0)
    y += 35
    temp = f"{wx_data.get('t', 0)}°C"
    draw.text((10, y), temp, font=font_large, fill=0)
    y += 40
    draw.text((10, y), wx_data.get("text", ""), font=font_small, fill=0)
    draw.text((10, y + 25), f"AQI: {wx_data.get('aqi', '')}", font=font_small, fill=0)
    
    # 返回原始位图数据（1-bit位图的字节数组）
    return image_to_1bit_bitmap(img, invert=False)


def render_note_bitmap(note_list: list, width: int = 496, height: int = 184) -> bytes:
    """渲染笔记列表为原始1-bit位图数据（用于ESP32 drawBitmap）"""
    img = Image.new('1', (width, height), 1)
    draw = ImageDraw.Draw(img)
    
    font = load_chinese_font(24)
    
    y = 10
    for i, note in enumerate(note_list[:7]):  # 最多显示7条（适配184高度）
        draw.text((10, y), f"{i+1}. {note}", font=font, fill=0)
        y += 26
        if y > height - 26:
            break
    
    # 返回原始位图数据（1-bit位图的字节数组）
    return image_to_1bit_bitmap(img, invert=False)


@app.get("/api/info")
async def get_info():
    """获取日期、时间、天气等信息（返回1-bit位图数据用于ESP32）
    
    返回原始1-bit位图数据（用于drawBitmap）
    格式说明：
    - 每个字节8个像素，从左到右，从上到下
    - 宽度会自动向上取整到8的倍数
    - 数据大小 = ((width + 7) // 8) * height 字节
    - PIL的'1'模式：0=黑色，1=白色（如果ESP32需要反转，可在image_to_1bit_bitmap中设置invert=True）
    """
    data = get_sample_data()
    
    # 返回原始位图数据（用于ESP32 drawBitmap）
    cal_bitmap = render_cal_bitmap(data["cal"])
    wx_bitmap = render_wx_bitmap(data["wx"])
    note_bitmap = render_note_bitmap(data["note"])
    
    return {
        "wx_ver": data["wx_ver"],
        "cal_ver": data["cal_ver"],
        "note_ver": data["note_ver"],
        "wx": {
            "buffer": base64.b64encode(wx_bitmap).decode('utf-8'),
            "width": 304,
            "height": 176,
            "format": "bitmap"
        },
        "cal": {
            "buffer": base64.b64encode(cal_bitmap).decode('utf-8'),
            "width": 496,
            "height": 120,
            "format": "bitmap"
        },
        "note": {
            "buffer": base64.b64encode(note_bitmap).decode('utf-8'),
            "width": 496,
            "height": 184,
            "format": "bitmap"
        }
    }


if __name__ == "__main__":
    import uvicorn
    uvicorn.run(app, host="0.0.0.0", port=8000)

