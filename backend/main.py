from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from datetime import datetime, date
from calendar import monthrange
import httpx
import os
import random
import logging
import asyncio
from typing import Optional
from dotenv import load_dotenv
from PIL import Image, ImageDraw, ImageFont, ImageOps
import base64
import holidays
from zhdate import ZhDate
import io
from astral import LocationInfo
from astral.sun import sun
from astral.moon import phase as moon_phase_func
import math
from zoneinfo import ZoneInfo
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
# 默认经纬度（北京顺义区，用于计算日出日落）
DEFAULT_LATITUDE = 40.1289  # 纬度
DEFAULT_LONGITUDE = 116.6544  # 经度
DEFAULT_TIMEZONE = "Asia/Shanghai"


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
    lunar_str = "零月初零"
    solar_term = ""  # 节气
    try:
        if HAS_LUNAR_PYTHON:
            # 使用 lunar_python 库
            lunar = Lunar.fromDate(now)
            # 使用 toString() 然后 split('年')[-1] 获取月日
            lunar_str = f"{lunar.toString().split('年')[-1]}"
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
    
    # 并发获取天气信息和一言句子
    weather_service = get_weather_service()
    weather_data, air_quality_data, forecast_data, sentence = await asyncio.gather(
        weather_service.get_weather_now(DEFAULT_LOCATION),
        weather_service.get_air_quality(DEFAULT_LOCATION),
        weather_service.get_weather_forecast(DEFAULT_LOCATION, "3d"),
        get_hitokoto_sentence(),
        return_exceptions=False
    )
    
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
    
    # 计算年/月/日进度
    today = now.date()
    year = today.year
    month = today.month
    day = today.day
    
    # 年进度：今年过了多少天 / 今年总天数
    year_start = date(year, 1, 1)
    year_end = date(year, 12, 31)
    year_total_days = (year_end - year_start).days + 1
    year_passed_days = (today - year_start).days + 1
    year_progress = (year_passed_days / year_total_days) * 100
    
    # 月进度：本月过了多少天 / 本月总天数
    month_total_days = monthrange(year, month)[1]
    month_progress = (day / month_total_days) * 100
    
    # 日进度：今天过了多少小时 / 24小时
    hour = now.hour
    minute = now.minute
    day_progress = ((hour * 60 + minute) / (24 * 60)) * 100
    
    progress_data = {
        "year": round(year_progress, 1),
        "month": round(month_progress, 1),
        "day": round(day_progress, 1),
    }
    
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
        "progress": progress_data,
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


def get_moon_phase_name(phase_days: float) -> str:
    """根据月相天数获取中文名称
    
    Args:
        phase_days: 距新月天数（0~29.53）
    
    Returns:
        月相中文名称
    """
    _PHASE_BOUNDS = [
        (1.0, "新月"),
        (6.382, "娥眉月"),  # Waxing Crescent
        (8.382, "上弦月"),
        (13.765, "盈凸月"),  # Waxing Gibbous
        (15.765, "满月"),
        (21.148, "亏凸月"),  # Waning Gibbous
        (23.148, "下弦月"),
        (28.53, "残月"),  # Waning Crescent
    ]
    
    for bound, name in _PHASE_BOUNDS:
        if phase_days < bound:
            return name
    
    # 兜底当作新月
    return "新月"


def draw_sun_icon(draw: ImageDraw.Draw, x: int, y: int, size: int, is_sunrise: bool = True):
    """绘制太阳图标（带箭头）
    
    Args:
        draw: ImageDraw对象
        x, y: 图标左上角坐标
        size: 图标大小
        is_sunrise: True为日出（向上箭头），False为日落（向下箭头）
    """
    # 绘制圆形太阳
    center_x = x + size // 2
    center_y = y + size // 2
    radius = size // 3
    
    # 太阳外圆
    draw.ellipse(
        [center_x - radius, center_y - radius, center_x + radius, center_y + radius],
        outline=0,
        width=2,
        fill=1
    )
    
    # 绘制箭头
    arrow_size = size // 4
    if is_sunrise:
        # 向上箭头
        arrow_y = y + size - arrow_size - 2
        # 箭头三角形（向上）
        arrow_points = [
            (center_x, arrow_y),
            (center_x - arrow_size // 2, arrow_y + arrow_size),
            (center_x + arrow_size // 2, arrow_y + arrow_size),
        ]
    else:
        # 向下箭头
        arrow_y = y + 2
        # 箭头三角形（向下）
        arrow_points = [
            (center_x, arrow_y + arrow_size),
            (center_x - arrow_size // 2, arrow_y),
            (center_x + arrow_size // 2, arrow_y),
        ]
    
    draw.polygon(arrow_points, outline=0, fill=0)


def draw_moon_phase(img: Image.Image, center_x: int, center_y: int, radius: int, phase: float):
    """绘制月相图
    Args:
        img: PIL Image对象
        center_x, center_y: 圆心坐标
        radius: 半径
        phase: 月相值（0.0-1.0，0.0=新月/全黑，0.25=上弦月/右半黑，0.5=满月/全白，0.75=下弦月/左半黑）
    """
    draw = ImageDraw.Draw(img)
    
    # 先绘制白色圆（带黑色边框）
    bbox = [center_x - radius, center_y - radius, center_x + radius, center_y + radius]
    draw.ellipse(bbox, outline=0, width=2, fill=1)
    
    # 根据月相值计算需要涂黑的部分
    # 月相原理：在圆内绘制一个椭圆形状的黑色区域
    if phase < 0.5:
        # 从新月(0.0)到满月(0.5)：左侧是黑色，右侧是白色
        # 计算黑色椭圆的宽度（从2*radius逐渐减小到0）
        black_width = 2 * radius * (1 - 2 * phase)
        
        if black_width > 0:
            # 逐行绘制黑色椭圆区域
            for y in range(center_y - radius, center_y + radius + 1):
                dy = y - center_y
                if abs(dy) < radius:
                    # 计算圆的x范围
                    circle_x_offset = int(math.sqrt(radius * radius - dy * dy))
                    
                    # 计算黑色椭圆的x范围
                    # 椭圆在y处的宽度 = black_width * sqrt(1 - (y/radius)^2)
                    # 简化：使用线性插值
                    ellipse_x_offset = int(circle_x_offset * (black_width / (2 * radius)))
                    
                    # 绘制左侧黑色部分
                    x_start = center_x - circle_x_offset
                    x_end = center_x - circle_x_offset + ellipse_x_offset
                    if x_start < x_end:
                        draw.line([(x_start, y), (x_end, y)], fill=0, width=1)
    else:
        # 从满月(0.5)到新月(1.0)：右侧是黑色，左侧是白色
        # 计算黑色椭圆的宽度（从0逐渐增大到2*radius）
        black_width = 2 * radius * (2 * phase - 1)
        
        if black_width > 0:
            # 逐行绘制黑色椭圆区域
            for y in range(center_y - radius, center_y + radius + 1):
                dy = y - center_y
                if abs(dy) < radius:
                    circle_x_offset = int(math.sqrt(radius * radius - dy * dy))
                    ellipse_x_offset = int(circle_x_offset * (black_width / (2 * radius)))
                    
                    # 绘制右侧黑色部分
                    x_start = center_x + circle_x_offset - ellipse_x_offset
                    x_end = center_x + circle_x_offset
                    if x_start < x_end:
                        draw.line([(x_start, y), (x_end, y)], fill=0, width=1)


def render_cal_bitmap(cal_data: dict, width: int = 496, height: int = 120) -> bytes:
    """渲染日历信息为原始1-bit位图数据（用于ESP32 drawBitmap）
    
    新布局：三块区域
    - 左侧：日期、星期、农历、节假日/节气
    - 中间：日出日落时间（带图标）
    - 右侧：月相图（带名称）
    - 添加竖线分区（细线，不顶格）
    """
    img = Image.new('1', (width, height), 1)
    draw = ImageDraw.Draw(img)
    
    # 使用中文字体（适配496x120尺寸）
    font_large, stroke_large = load_chinese_font(32, bold=True)
    font_medium, stroke_medium = load_chinese_font(24, bold=True)
    font_small, stroke_small = load_chinese_font(20)
    font_sun, stroke_sun = load_chinese_font(24)
    
    # 边距（用于竖线不顶格）
    margin_y = 8
    
    # 三块区域的宽度分配
    left_width = int(width * 0.45)      # 左侧：45%
    middle_width = int(width * 0.30)   # 中间：30%
    right_width = width - left_width - middle_width  # 右侧：剩余部分
    
    # 分界线位置
    line1_x = left_width
    line2_x = left_width + middle_width
    
    # 绘制竖线分区（细线，不顶格，参考天气预报样式）
    draw.line([(line1_x, margin_y), (line1_x, height - margin_y)], fill=0, width=1)
    draw.line([(line2_x, margin_y), (line2_x, height - margin_y)], fill=0, width=1)
    
    # ========== 左侧：日历信息 ==========
    left_margin = 15
    y = 10
    
    # 第一行：日期
    date_str = cal_data.get("date", "")
    draw.text((left_margin, y), date_str, font=font_large, fill=0, stroke_width=stroke_large, stroke_fill=0)
    
    # 第二行：星期
    y = 50
    weekday_str = cal_data.get("weekday", "")
    if weekday_str:
        draw.text((left_margin, y), weekday_str, font=font_medium, fill=0, stroke_width=stroke_medium, stroke_fill=0)
    
    # 第三行：农历+节假日/节气
    y = 80
    lunar = cal_data.get("lunar", "")
    extra = cal_data.get("extra", "")
    if lunar or extra:
        draw.text((left_margin, y), lunar + " " + extra, font=font_small, fill=0, stroke_width=stroke_small, stroke_fill=0)
    
    # ========== 中间：日出日落 ==========
    middle_margin = line1_x + 10
    icon_size = 20  # 太阳图标大小
    
    # 获取日出日落时间和月相
    now = datetime.now()
    try:
        tz = ZoneInfo(DEFAULT_TIMEZONE)
        location = LocationInfo("Beijing", "China", DEFAULT_TIMEZONE, DEFAULT_LATITUDE, DEFAULT_LONGITUDE)
        s = sun(location.observer, date=now.date(), tzinfo=tz)
        sunrise = s["sunrise"].strftime("%H:%M")
        sunset = s["sunset"].strftime("%H:%M")
        
        # 计算月相
        phase_days = moon_phase_func(now.date())
        phase_normalized = (phase_days % 29.5) / 29.5
        moon_phase_name = get_moon_phase_name(phase_days)
        
        # 日出日落文字（在中间区域居中）
        middle_area_start = line1_x
        middle_area_width = middle_width
        
        # 第一行：日出时间（居中）
        sunrise_text = f"日出 {sunrise}"
        sunrise_bbox = draw.textbbox((0, 0), sunrise_text, font=font_sun)
        sunrise_width = sunrise_bbox[2] - sunrise_bbox[0]
        sunrise_x = middle_area_start + (middle_area_width - sunrise_width) // 2
        y = 40
        draw.text((sunrise_x, y), sunrise_text, font=font_sun, fill=0, stroke_width=stroke_sun, stroke_fill=0)
        
        # 第二行：日落时间（居中）
        sunset_text = f"日落 {sunset}"
        sunset_bbox = draw.textbbox((0, 0), sunset_text, font=font_sun)
        sunset_width = sunset_bbox[2] - sunset_bbox[0]
        sunset_x = middle_area_start + (middle_area_width - sunset_width) // 2
        y = 65
        draw.text((sunset_x, y), sunset_text, font=font_sun, fill=0, stroke_width=stroke_sun, stroke_fill=0)
        
        # ========== 右侧：月相 ==========
        right_margin = line2_x + 10
        right_area_width = width - line2_x
        
        # 月相图（在右侧区域居中，往上挪一点）
        moon_radius = 25
        moon_center_x = line2_x + right_area_width // 2
        moon_center_y = height // 2 - 10  # 往上挪10像素
        
        # 绘制月相图
        draw_moon_phase(img, moon_center_x, moon_center_y, moon_radius, phase_normalized)
        
        # 月相名称（月相图下方，居中）
        name_y = moon_center_y + moon_radius + 5
        name_bbox = draw.textbbox((0, 0), moon_phase_name, font=font_small)
        name_width = name_bbox[2] - name_bbox[0]
        name_x = moon_center_x - name_width // 2
        draw.text((name_x, name_y), moon_phase_name, font=font_small, fill=0, stroke_width=stroke_small, stroke_fill=0)
        
    except Exception as e:
        logger.warning(f"获取日出日落时间或月相失败: {e}", exc_info=True)
    
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
    font_text, stroke_text = load_chinese_font(22)  # 天气文字
    font_temp, stroke_temp = load_chinese_font(22, bold=True)  # 温度
    
    # 每天的高度
    day_height = height // 3
    icon_size = 40  # 图标大小
    
    # 边距
    margin_x = 18
    margin_y = 8
    
    for i, day_data in enumerate(forecast_list[:3]):  # 最多显示3天
        y_start = i * day_height
        
        # 日期（日）
        day = day_data.get("day", "")
        # 补齐到两位，注意day是str类型
        day = day.zfill(2)
        if day:
            draw.text((margin_x, y_start + margin_y), day+"日", font=font_day, fill=0, stroke_width=stroke_day, stroke_fill=0)
        
        # 天气图标（居中偏左）
        icon_code = day_data.get("icon", "")
        icon_x = margin_x + 70
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
        
        # 温度 右对齐
        temp_max = day_data.get("tempMax", "")
        temp_min = day_data.get("tempMin", "")
        if temp_max and temp_min:
            temp_str = f"{temp_min}~{temp_max}°C"
            temp_bbox = draw.textbbox((0, 0), temp_str, font=font_temp)
            temp_width = temp_bbox[2] - temp_bbox[0]
            temp_x = width - temp_width - margin_x
            temp_y = y_start + margin_y + 3 # 不知道为啥，需要往下点才能对齐
            draw.text((temp_x, temp_y), temp_str, font=font_temp, fill=0, stroke_width=stroke_temp, stroke_fill=0)
        
        # 绘制分隔线（除了最后一天）
        if i < 2:
            line_y = y_start + day_height - 1
            draw.line([(margin_x, line_y), (width - margin_x, line_y)], fill=0, width=1)
    
    # 存一下
    img.save("test_images/forecast.png")
    
    # 返回原始位图数据（1-bit位图的字节数组）
    return image_to_1bit_bitmap(img, invert=False)


def render_progress_bitmap(progress_data: dict, width: int = 496, height: int = 128) -> bytes:
    """渲染年/月/日进度条为原始1-bit位图数据（用于ESP32 drawBitmap）
    
    显示格式：
    - 年进度 [进度条] 百分比
    - 月进度 [进度条] 百分比
    - 日进度 [进度条] 百分比
    """
    img = Image.new('1', (width, height), 1)
    draw = ImageDraw.Draw(img)
    
    font_label, stroke_label = load_chinese_font(24)  # 标签字体
    font_percent, stroke_percent = load_chinese_font(22)  # 百分比字体
    
    # 边距和间距
    margin_x = 10
    margin_y = 10
    line_height = 36  # 每行高度
    progress_bar_height = 20  # 进度条高度
    progress_bar_width = width - 200  # 进度条宽度（留出标签和百分比的空间）
    
    # 进度条位置
    label_width = 80  # 标签宽度（"年进度"、"月进度"、"日进度"）
    bar_x = margin_x + label_width + 10
    bar_y_offset = (line_height - progress_bar_height) // 2
    
    # 进度条数据
    progress_items = [
        ("年进度", progress_data.get("year", 0)),
        ("月进度", progress_data.get("month", 0)),
        ("日进度", progress_data.get("day", 0)),
    ]
    
    for i, (label, percent) in enumerate(progress_items):
        y = margin_y + i * line_height
        
        # 绘制标签
        draw.text((margin_x, y), label, font=font_label, fill=0, stroke_width=stroke_label, stroke_fill=0)
        
        # 绘制进度条背景（白色背景上的黑色边框）
        bar_y = y + bar_y_offset
        draw.rectangle(
            [bar_x, bar_y, bar_x + progress_bar_width, bar_y + progress_bar_height],
            outline=0,
            width=2,
            fill=1
        )
        
        # 绘制进度条填充（黑色填充）
        fill_width = int(progress_bar_width * (percent / 100))
        if fill_width > 0:
            draw.rectangle(
                [bar_x, bar_y, bar_x + fill_width, bar_y + progress_bar_height],
                fill=0
            )
        
        # 绘制百分比（右对齐）
        percent_text = f"{percent:.1f}%"
        percent_bbox = draw.textbbox((0, 0), percent_text, font=font_percent)
        percent_width = percent_bbox[2] - percent_bbox[0]
        percent_x = width - percent_width - margin_x
        draw.text((percent_x, y + 5), percent_text, font=font_percent, fill=0, stroke_width=stroke_percent, stroke_fill=0)
    
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
    progress_bitmap = render_progress_bitmap(data.get("progress", {}))
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
            "buffer": base64.b64encode(progress_bitmap).decode('utf-8'),
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