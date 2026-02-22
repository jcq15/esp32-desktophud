from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from datetime import datetime, date
import httpx
import os
import random
import logging
from typing import Optional
from dotenv import load_dotenv
from PIL import Image, ImageDraw, ImageFont, ImageOps
import base64
import holidays
from zhdate import ZhDate
import io
try:
    from lunar_python import Lunar
    HAS_LUNAR_PYTHON = True
except ImportError:
    HAS_LUNAR_PYTHON = False

from weather_service import get_weather_service

# 加载 .env 文件
load_dotenv()

# 配置日志
log_dir = os.path.join(os.path.dirname(__file__), "logs")
os.makedirs(log_dir, exist_ok=True)
log_file = os.path.join(log_dir, "main.log")

# 创建logger
logger = logging.getLogger("main")
logger.setLevel(logging.DEBUG)

# 如果logger还没有handler，添加handler
if not logger.handlers:
    # 文件handler（追加模式）
    file_handler = logging.FileHandler(log_file, encoding='utf-8')
    file_handler.setLevel(logging.DEBUG)
    
    # 控制台handler
    console_handler = logging.StreamHandler()
    console_handler.setLevel(logging.INFO)
    
    # 格式化
    formatter = logging.Formatter(
        '%(asctime)s - %(name)s - %(levelname)s - %(message)s',
        datefmt='%Y-%m-%d %H:%M:%S'
    )
    file_handler.setFormatter(formatter)
    console_handler.setFormatter(formatter)
    
    logger.addHandler(file_handler)
    logger.addHandler(console_handler)

app = FastAPI(title="DeskHUD Backend", version="1.0.0")

# 允许跨域请求
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# 默认位置（北京顺义区）
DEFAULT_LOCATION = "101010400"


def get_real_date_info():
    """获取真实的日期信息，包括公历、农历、星期、节假日、节气"""
    today = date.today()
    now = datetime.now()
    
    # 1. 公历年月日
    solar_date = today.strftime("%Y-%m-%d")
    
    # 2. 星期（中文）
    weekdays = ['星期一', '星期二', '星期三', '星期四', '星期五', '星期六', '星期日']
    weekday = weekdays[today.weekday()]
    
    # 3. 农历月日和节气
    lunar_str = "农历 未知"
    solar_term = ""  # 节气
    try:
        if HAS_LUNAR_PYTHON:
            # 使用 lunar_python 库
            lunar = Lunar.fromDate(now)
            # 使用 toString() 然后 split('年')[-1] 获取月日
            lunar_str = f"农历 {lunar.toString().split('年')[-1]}"
            # 获取节气
            jie_qi = lunar.getJieQi()
            if jie_qi:
                solar_term = jie_qi
        else:
            # 使用 zhdate 库（备用方案）
            zh_date = ZhDate.from_datetime(now)
            lunar_month = zh_date.lunar_month
            lunar_day = zh_date.lunar_day
            month_names = ['', '正', '二', '三', '四', '五', '六', '七', '八', '九', '十', '十一', '十二']
            day_names = ['', '初一', '初二', '初三', '初四', '初五', '初六', '初七', '初八', '初九', '初十',
                        '十一', '十二', '十三', '十四', '十五', '十六', '十七', '十八', '十九', '二十',
                        '廿一', '廿二', '廿三', '廿四', '廿五', '廿六', '廿七', '廿八', '廿九', '三十']
            month_str = month_names[lunar_month] + '月' if lunar_month <= 12 else f"{lunar_month}月"
            day_str = day_names[lunar_day] if lunar_day <= 30 else f"{lunar_day}日"
            lunar_str = f"农历 {month_str}{day_str}"
    except Exception as e:
        logger.warning(f"获取农历日期失败: {e}", exc_info=True)
    
    # 4. 节假日（使用 holidays 库）
    holiday_name = ""
    try:
        cn = holidays.country_holidays('CN', language='zh_CN')
        holiday_name = cn.get(today, "")
        # 去掉括号里的东西
        holiday_name = holiday_name.split("(")[0]
    except Exception as e:
        logger.warning(f"获取节假日失败: {e}", exc_info=True)

    # 组合 extra 信息（节假日和节气）
    extra_parts = []
    if holiday_name:
        extra_parts.append(holiday_name)
    if solar_term:
        extra_parts.append(solar_term)
    extra = " ".join(extra_parts) if extra_parts else ""
    
    return {
        "date": solar_date,
        "weekday": weekday,
        "lunar": lunar_str,
        "extra": extra
    }



@app.get("/")
async def root():
    """根路径"""
    return {"message": "DeskHUD Backend API"}


async def get_hitokoto_sentence() -> str:
    """从一言API获取句子
    
    Returns:
        句子文本，如果失败返回默认句子
    """
    default_sentences = [
        "今天是个好日子",
        "保持专注，持续进步",
        "每一天都是新的开始",
        "努力成为更好的自己",
        "时间是最好的老师"
    ]
    
    try:
        async with httpx.AsyncClient(timeout=5.0) as client:
            url = "https://v1.hitokoto.cn"
            params = {
                "encode": "json",
                "charset": "utf-8",
                "max_length": 26  # 限制最大长度为26
            }
            response = await client.get(url, params=params)
            response.raise_for_status()
            data = response.json()
            
            sentence = data.get("hitokoto", "")
            if sentence and len(sentence) <= 26:
                return sentence
            else:
                # 如果句子超过26字，使用默认句子
                return random.choice(default_sentences)
    except Exception as e:
        logger.warning(f"获取一言句子失败: {e}", exc_info=True)
        return random.choice(default_sentences)


async def get_sample_data():
    """返回示例测试数据"""
    now = datetime.now()
    
    # 获取真实的日期信息
    cal_info = get_real_date_info()
    
    # 获取真实的天气信息
    weather_service = get_weather_service()
    weather_data = await weather_service.get_weather_now(DEFAULT_LOCATION)
    air_quality_data = await weather_service.get_air_quality(DEFAULT_LOCATION)
    forecast_data = await weather_service.get_weather_forecast(DEFAULT_LOCATION, "3d")
    
    # 获取一言句子
    sentence = await get_hitokoto_sentence()
    
    # 构建天气信息
    wx_info = {
        "city": "顺义区",
        "t": 0,  # 温度
        "feelsLike": 0,  # 体感温度
        "text": "未知",
        "windDir": "",  # 风向
        "windScale": "",  # 风力等级
        "icon": "",  # 天气图标代码
        "aqi": "未知",
    }
    
    if weather_data:
        # 解析天气数据
        wx_info["t"] = int(weather_data.get("temp", 0))
        wx_info["feelsLike"] = int(weather_data.get("feelsLike", 0))
        wx_info["text"] = weather_data.get("text", "未知")
        wx_info["windDir"] = weather_data.get("windDir", "")
        wx_info["windScale"] = weather_data.get("windScale", "")
        wx_info["icon"] = weather_data.get("icon", "")
        wx_info["city"] = "顺义区"  # 可以根据需要从API获取城市名
    
    if air_quality_data:
        # 解析空气质量数据
        aqi_value = air_quality_data.get("aqi", "")
        category = air_quality_data.get("category", "")
        if aqi_value and category:
            wx_info["aqi"] = f"{category}({aqi_value})"
    
    # 处理天气预报数据
    forecast_list = []
    if forecast_data:
        for day_data in forecast_data:
            # 解析日期，只取日
            fx_date = day_data.get("fxDate", "")
            day = ""
            if fx_date:
                try:
                    # fxDate格式：2021-11-15，提取日
                    day = fx_date.split("-")[-1]
                except:
                    day = ""
            
            forecast_list.append({
                "day": day,
                "icon": day_data.get("iconDay", ""),  # 白天图标
                "text": day_data.get("textDay", ""),  # 白天天气描述
                "tempMax": day_data.get("tempMax", ""),  # 最高温度
                "tempMin": day_data.get("tempMin", ""),  # 最低温度
            })
    
    # 使用时间戳（秒级）模 10000 作为版本号
    timestamp = int(now.timestamp()) % 86389 # magic
    
    return {
        "sentence_ver": timestamp,
        "wx_ver": timestamp,
        "cal_ver": timestamp,
        "note_ver": timestamp,
        "forecast_ver": timestamp,
        "sentence": sentence,
        "wx": wx_info,
        "cal": cal_info,
        "forecast": forecast_list,
        "note": [
            "喝水",
            "设计外壳",
            f'我想吃 {random.choice(["苹果", "香蕉", "橙子", "梨", "菠萝"])}'
        ],
        #"ttl": 300
    }


def load_chinese_font(size: int, bold: bool = False):
    """加载思源黑体字体
    
    Args:
        size: 字体大小
        bold: 是否加粗（通过stroke参数实现，不依赖字体文件）
    
    Returns:
        (font, stroke_width): 字体对象和描边宽度（用于加粗效果）
    """
    # font_path = os.path.join(os.path.dirname(__file__), "fonts", "SourceHanSerifSC-VF.ttf")
    font_path = os.path.join(os.path.dirname(__file__), "fonts", "SourceHanSansSC-VF.ttf")
    
    if os.path.exists(font_path):
        try:
            font = ImageFont.truetype(font_path, size)
            # 使用stroke参数实现加粗效果，stroke_width根据字体大小调整
            # stroke_width = int(size * 0.08) if bold else 0
            stroke_width = 1 if bold else 0
            return font, stroke_width
        except Exception as e:
            logger.warning(f"警告: 加载字体失败 {e}，使用默认字体", exc_info=True)
            font = ImageFont.load_default()
            stroke_width = 1 if bold else 0
            return font, stroke_width
    else:
        logger.warning(f"警告: 字体文件不存在 {font_path}，使用默认字体")
        font = ImageFont.load_default()
        stroke_width = 1 if bold else 0
        return font, stroke_width


def load_weather_icon(icon_code: str, size: int = 64) -> Optional[Image.Image]:
    """加载天气图标PNG并转换为PIL Image（1-bit模式）
    
    注意：由于SVG渲染需要系统库，这里直接加载PNG文件。
    如果PNG不存在，会尝试加载SVG（需要预先转换）。
    
    Args:
        icon_code: 图标代码（如"502"）
        size: 目标尺寸（默认64x64）
    
    Returns:
        PIL Image对象（1-bit模式，0=黑色图标，1=白色背景），如果加载失败返回None
    """
    if not icon_code:
        return None
    
    # 图标文件路径
    icons_dir = os.path.join(os.path.dirname(__file__), "icons")
    
    # 优先尝试PNG文件（如果已预先转换）
    # 然后尝试-fill版本的PNG，最后尝试普通版本的PNG
    icon_files = [
        os.path.join(icons_dir, f"{icon_code}-fill.png"),
        os.path.join(icons_dir, f"{icon_code}.png"),
        os.path.join(icons_dir, f"{icon_code}-fill.svg"),
        os.path.join(icons_dir, f"{icon_code}.svg"),
    ]
    
    icon_path = None
    for path in icon_files:
        if os.path.exists(path):
            icon_path = path
            break
    
    if not icon_path:
        logger.warning(f"天气图标文件不存在: {icon_code}")
        return None
    
    try:
        # 如果是PNG文件，直接加载
        if icon_path.endswith('.png'):
            icon_img = Image.open(icon_path)
        else:
            # SVG文件需要系统库支持，这里返回None并提示
            logger.warning(f"SVG文件需要预先转换为PNG: {icon_path}")
            logger.info(f"提示：可以使用在线工具或脚本将 {icon_code}.svg 转换为 {icon_code}.png")
            return None
        
        # 确保是RGB模式
        if icon_img.mode == 'RGBA':
            # 处理透明背景：创建白色背景
            background = Image.new('RGB', icon_img.size, (255, 255, 255))
            background.paste(icon_img, mask=icon_img.split()[3] if icon_img.mode == 'RGBA' else None)
            icon_img = background
        elif icon_img.mode != 'RGB':
            icon_img = icon_img.convert('RGB')
        
        # 调整大小
        if icon_img.size[0] != size or icon_img.size[1] != size:
            icon_img = icon_img.resize((size, size), Image.Resampling.LANCZOS)
        
        # 转换为灰度
        icon_img = icon_img.convert('L')
        
        # 转换为1-bit：小于128的像素变为0（黑色），大于等于128的变为1（白色）
        # 这样图标部分（通常是黑色或深色）会变成0，背景（白色）会变成1
        icon_img = icon_img.point(lambda x: 0 if x < 128 else 255, mode='1')
        
        return icon_img
    except Exception as e:
        logger.error(f"加载天气图标失败 {icon_path}: {e}", exc_info=True)
        return None


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


def render_sentence_bitmap(sentence: str, width: int = 800, height: int = 56) -> bytes:
    """渲染句子为原始1-bit位图数据（用于ESP32 drawBitmap）
    黑底白字，需要反转颜色
    """
    # 创建1-bit位图，初始为黑色
    img = Image.new('1', (width, height), 0)
    draw = ImageDraw.Draw(img)
    
    font, stroke_width = load_chinese_font(28, bold=True)
    
    # 计算文字居中位置
    bbox = draw.textbbox((0, 0), sentence, font=font)
    text_width = bbox[2] - bbox[0]
    text_height = bbox[3] - bbox[1]
    x = (width - text_width) // 2
    y = (height - text_height) // 2 - bbox[1]
    
    # 绘制白色文字（在黑色背景上），使用stroke实现加粗
    draw.text((x, y), sentence, font=font, fill=1, stroke_width=stroke_width, stroke_fill=1)

    # 存一下
    img.save("test_images/sentence.png")
    
    return image_to_1bit_bitmap(img)


def render_cal_bitmap(cal_data: dict, width: int = 496, height: int = 120) -> bytes:
    """渲染日历信息为原始1-bit位图数据（用于ESP32 drawBitmap）"""
    img = Image.new('1', (width, height), 1)
    draw = ImageDraw.Draw(img)
    
    # 使用中文字体（适配496x120尺寸）
    font_large, stroke_large = load_chinese_font(32, bold=True)
    font_medium, stroke_medium = load_chinese_font(24)
    font_small, stroke_small = load_chinese_font(20)
    
    # 第一行：日期（左侧）+ 星期（右侧）
    y = 10
    date_str = cal_data.get("date", "")
    weekday_str = cal_data.get("weekday", "")
    
    # 日期（左侧）
    draw.text((10, y), date_str, font=font_large, fill=0, stroke_width=stroke_large, stroke_fill=0)
    
    # 星期（右侧对齐）
    weekday_bbox = draw.textbbox((0, 0), weekday_str, font=font_medium)
    weekday_width = weekday_bbox[2] - weekday_bbox[0]
    weekday_x = width - weekday_width - 10
    draw.text((weekday_x, y + 5), weekday_str, font=font_medium, fill=0, stroke_width=stroke_medium, stroke_fill=0)
    
    # 第二行：农历（左侧）
    y = 50
    lunar = cal_data.get("lunar", "")
    if lunar:
        draw.text((10, y), lunar, font=font_small, fill=0, stroke_width=stroke_small, stroke_fill=0)
    
    # 第三行：节假日/节气（如果有，显示在右侧或下方）
    extra = cal_data.get("extra", "")
    if extra:
        # 如果农历较短，可以放在同一行的右侧；否则放在下一行
        if lunar:
            lunar_bbox = draw.textbbox((0, 0), lunar, font=font_small)
            lunar_width = lunar_bbox[2] - lunar_bbox[0]
            # 如果农历较短，节假日放在同一行右侧
            if lunar_width < width // 2:
                extra_x = width // 2 + 20
                draw.text((extra_x, y), extra, font=font_small, fill=0, stroke_width=stroke_small, stroke_fill=0)
            else:
                # 否则放在下一行
                y += 28
                draw.text((10, y), extra, font=font_small, fill=0, stroke_width=stroke_small, stroke_fill=0)
        else:
            # 没有农历，直接显示在第二行
            draw.text((10, y), extra, font=font_small, fill=0, stroke_width=stroke_small, stroke_fill=0)
    
    # 存一下
    img.save("test_images/cal.png")
    
    # 返回原始位图数据（1-bit位图的字节数组）
    # 注意：PIL的'1'模式中，0=黑色，1=白色
    # 如果ESP32端需要反转，可以在image_to_1bit_bitmap中设置invert=True
    return image_to_1bit_bitmap(img, invert=False)


def render_wx_bitmap(wx_data: dict, width: int = 304, height: int = 176) -> bytes:
    """渲染天气信息为原始1-bit位图数据（用于ESP32 drawBitmap）
    
    优化后的布局：
    - 右上角：icon（64x64）
    - 左上角：城市名（28号字体）
    - 左侧：温度（42号大字体）
    - 温度下方：体感温度（24号字体）
    - 左侧中间：AQI（20号字体）
    - 右下角第一行：天气描述（22号字体，右对齐）
    - 右下角第二行：风向风力（20号字体，右对齐）
    """
    img = Image.new('1', (width, height), 1)
    draw = ImageDraw.Draw(img)
    
    # 使用更大的字体
    font_temp, stroke_temp = load_chinese_font(42, bold=True)  # 温度
    font_city, stroke_city = load_chinese_font(28, bold=True)  # 城市名
    font_feels, stroke_feels = load_chinese_font(24)  # 体感温度
    font_desc, stroke_desc = load_chinese_font(24, bold=True)  # 天气描述
    font_info, stroke_info = load_chinese_font(24)  # 风向、AQI等
    
    # 左右边距
    margin = 18

    # 预留icon位置（右上角，64x64）
    icon_size = 64
    icon_x = width - icon_size - margin
    icon_y = 10

    # 加载并绘制天气图标
    icon_code = wx_data.get("icon", "")
    if icon_code:
        icon_img = load_weather_icon(icon_code, icon_size)
        if icon_img:
            img.paste(icon_img, (icon_x, icon_y))
    
    # 第一行：城市名（左上角）
    y = 10
    city = wx_data.get("city", "")
    draw.text((margin, y), city, font=font_city, fill=0, stroke_width=stroke_city, stroke_fill=0)
    
    # 第二行：温度（左侧，超大字体）
    y = 45
    temp = f"{wx_data.get('t', 0)}°C"
    draw.text((margin, y), temp, font=font_temp, fill=0, stroke_width=stroke_temp, stroke_fill=0)
    
    # 第三行：体感温度（温度下方）
    feels_like = wx_data.get("feelsLike", 0)
    if feels_like:
        y = 95
        feels_like_str = f"体感 {feels_like}°C"
        draw.text((margin, y), feels_like_str, font=font_feels, fill=0, stroke_width=stroke_feels, stroke_fill=0)
    
    # 第四行：AQI（左侧）
    y = 125
    aqi = wx_data.get("aqi", "")
    if aqi and aqi != "未知":
        aqi_text = f"AQI: {aqi}"
        draw.text((margin, y), aqi_text, font=font_info, fill=0, stroke_width=stroke_info, stroke_fill=0)
    
    # 右下角第一行：天气描述（右对齐）
    y = 95
    text = wx_data.get("text", "")
    if text:
        text_bbox = draw.textbbox((0, 0), text, font=font_desc)
        text_width = text_bbox[2] - text_bbox[0]
        text_x = width - text_width - margin
        draw.text((text_x, y), text, font=font_desc, fill=0, stroke_width=stroke_desc, stroke_fill=0)
    
    # 右下角第二行：风向风力（右对齐）
    y = 125
    wind_dir = wx_data.get("windDir", "")
    wind_scale = wx_data.get("windScale", "")
    wind_info = ""
    if wind_dir and wind_scale:
        wind_info = f"{wind_dir} {wind_scale}级"
    elif wind_dir:
        wind_info = wind_dir
    elif wind_scale:
        wind_info = f"{wind_scale}级"
    
    if wind_info:
        wind_bbox = draw.textbbox((0, 0), wind_info, font=font_info)
        wind_width = wind_bbox[2] - wind_bbox[0]
        wind_x = width - wind_width - margin
        draw.text((wind_x, y), wind_info, font=font_info, fill=0, stroke_width=stroke_info, stroke_fill=0)
    
    # 存一下
    img.save("test_images/wx.png")
    
    # 返回原始位图数据（1-bit位图的字节数组）
    return image_to_1bit_bitmap(img, invert=False)


def render_forecast_bitmap(forecast_list: list, width: int = 304, height: int = 192) -> bytes:
    """渲染未来三天天气预报为原始1-bit位图数据（用于ESP32 drawBitmap）
    
    布局：竖着分成三份，每份是一天
    每天显示：日期（日）、天气图标、天气文字、温度（xx~xx）
    """
    img = Image.new('1', (width, height), 1)
    draw = ImageDraw.Draw(img)
    
    # 字体设置
    font_day, stroke_day = load_chinese_font(24, bold=True)  # 日期
    font_text, stroke_text = load_chinese_font(20)  # 天气文字
    font_temp, stroke_temp = load_chinese_font(22, bold=True)  # 温度
    
    # 每天的高度
    day_height = height // 3
    icon_size = 40  # 图标大小
    
    # 边距
    margin_x = 10
    margin_y = 8
    
    for i, day_data in enumerate(forecast_list[:3]):  # 最多显示3天
        y_start = i * day_height
        
        # 日期（日）
        day = day_data.get("day", "")
        if day:
            draw.text((margin_x, y_start + margin_y), day, font=font_day, fill=0, stroke_width=stroke_day, stroke_fill=0)
        
        # 天气图标（居中偏左）
        icon_code = day_data.get("icon", "")
        icon_x = margin_x + 40
        icon_y = y_start + margin_y
        if icon_code:
            icon_img = load_weather_icon(icon_code, icon_size)
            if icon_img:
                img.paste(icon_img, (icon_x, icon_y))
        
        # 天气文字（图标右侧）
        text = day_data.get("text", "")
        text_x = icon_x + icon_size + 10
        if text:
            draw.text((text_x, y_start + margin_y + 5), text, font=font_text, fill=0, stroke_width=stroke_text, stroke_fill=0)
        
        # 温度（底部，居中）
        temp_max = day_data.get("tempMax", "")
        temp_min = day_data.get("tempMin", "")
        if temp_max and temp_min:
            temp_str = f"{temp_min}~{temp_max}°C"
            temp_bbox = draw.textbbox((0, 0), temp_str, font=font_temp)
            temp_width = temp_bbox[2] - temp_bbox[0]
            temp_x = (width - temp_width) // 2
            temp_y = y_start + day_height - 30
            draw.text((temp_x, temp_y), temp_str, font=font_temp, fill=0, stroke_width=stroke_temp, stroke_fill=0)
        
        # 绘制分隔线（除了最后一天）
        if i < 2:
            line_y = y_start + day_height - 1
            draw.line([(margin_x, line_y), (width - margin_x, line_y)], fill=0, width=1)
    
    # 存一下
    img.save("test_images/forecast.png")
    
    # 返回原始位图数据（1-bit位图的字节数组）
    return image_to_1bit_bitmap(img, invert=False)


def render_note_bitmap(note_list: list, width: int = 496, height: int = 128) -> bytes:
    """渲染笔记列表为原始1-bit位图数据（用于ESP32 drawBitmap）"""
    img = Image.new('1', (width, height), 1)
    draw = ImageDraw.Draw(img)
    
    font, stroke_width = load_chinese_font(24)
    
    y = 10
    for i, note in enumerate(note_list[:5]):  # 最多显示5条（适配128高度）
        draw.text((10, y), f"{i+1}. {note}", font=font, fill=0, stroke_width=stroke_width, stroke_fill=0)
        y += 24
        if y > height - 24:
            break
    
    # 存一下
    img.save("test_images/note.png")
    
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
    data = await get_sample_data()
    
    # 返回原始位图数据（用于ESP32 drawBitmap）
    sentence_bitmap = render_sentence_bitmap(data["sentence"])
    cal_bitmap = render_cal_bitmap(data["cal"])
    wx_bitmap = render_wx_bitmap(data["wx"])
    note_bitmap = render_note_bitmap(data["note"])
    forecast_bitmap = render_forecast_bitmap(data.get("forecast", []))
    
    return {
        "sentence_ver": data["sentence_ver"],
        "wx_ver": data["wx_ver"],
        "cal_ver": data["cal_ver"],
        "note_ver": data["note_ver"],
        "forecast_ver": data.get("forecast_ver", data["wx_ver"]),
        "sentence": {
            "buffer": base64.b64encode(sentence_bitmap).decode('utf-8'),
            "width": 800,
            "height": 56,
            "format": "bitmap"
        },
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
            "height": 128,
            "format": "bitmap"
        },
        "forecast": {
            "buffer": base64.b64encode(forecast_bitmap).decode('utf-8'),
            "width": 304,
            "height": 192,
            "format": "bitmap"
        }
    }


if __name__ == "__main__":
    import uvicorn
    uvicorn.run('main:app', host="0.0.0.0", port=8000, reload=True)