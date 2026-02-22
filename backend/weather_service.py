"""和风天气API服务
使用JWT认证方式调用和风天气API
"""
import os
import time
import jwt
import logging
import traceback
from datetime import datetime, timedelta
from typing import Optional, Dict
import httpx
from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.backends import default_backend

# 配置日志
log_dir = os.path.join(os.path.dirname(__file__), "logs")
os.makedirs(log_dir, exist_ok=True)
log_file = os.path.join(log_dir, "weather_service.log")

# 创建logger
logger = logging.getLogger("weather_service")
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


class QWeatherService:
    """和风天气API服务类"""
    
    def __init__(self):
        """初始化服务，从环境变量读取配置"""
        self.private_key_path = os.getenv("QW_PRIVATE_KEY_PATH", "")
        self.key_id = os.getenv("QW_KEY_ID", "")
        self.sub_id = os.getenv("QW_SUB_ID", "")
        # 从环境变量读取API host，如果没有则使用默认值
        api_host = os.getenv("QW_API_HOST", "")
        if not api_host:
            logger.warning("警告: QW_API_HOST 未设置，将无法调用API")
        self.base_url = f"https://{api_host}/v7"
        self._private_key = None
        self._jwt_token = None
        self._token_expires_at = 0
        # 缓存最近一次成功的天气数据
        self._weather_cache: Optional[Dict] = None
        self._weather_cache_time: float = 0
        self._cache_valid_duration = 3600  # 缓存有效期：1小时（秒）
        
    def _load_private_key(self) -> Optional[str]:
        """加载私钥文件，返回PEM格式的字符串"""
        if not self.private_key_path:
            logger.warning("警告: QW_PRIVATE_KEY_PATH 未设置")
            return None
        
        if not os.path.exists(self.private_key_path):
            logger.warning(f"警告: 私钥文件不存在: {self.private_key_path}")
            return None
        
        try:
            # 直接读取PEM格式的私钥文件（文本格式）
            with open(self.private_key_path, 'r', encoding='utf-8') as f:
                private_key_pem = f.read()
            
            # 验证是否是有效的PEM格式
            if not private_key_pem.strip().startswith('-----BEGIN'):
                logger.warning("警告: 私钥文件格式不正确，应该以 -----BEGIN 开头")
                return None
            
            # 验证私钥是否是Ed25519格式
            if 'BEGIN PRIVATE KEY' in private_key_pem:
                try:
                    private_key_obj = serialization.load_pem_private_key(
                        private_key_pem.encode('utf-8'),
                        password=None,
                        backend=default_backend()
                    )
                    key_type = type(private_key_obj).__name__
                    if 'Ed25519' not in key_type:
                        logger.warning(f"警告: 私钥类型不是Ed25519，而是 {key_type}")
                except Exception as e:
                    logger.warning(f"警告: 无法解析私钥: {e}")
            
            return private_key_pem
        except UnicodeDecodeError:
            # 如果UTF-8解码失败，尝试用二进制方式读取然后解码
            try:
                with open(self.private_key_path, 'rb') as f:
                    private_key_data = f.read()
                private_key_pem = private_key_data.decode('utf-8')
                if not private_key_pem.strip().startswith('-----BEGIN'):
                    logger.warning("警告: 私钥文件格式不正确")
                    return None
                return private_key_pem
            except Exception as e:
                logger.error(f"读取私钥文件失败: {e}", exc_info=True)
                return None
        except Exception as e:
            logger.error(f"读取私钥文件失败: {e}", exc_info=True)
            return None
    
    def _generate_jwt_token(self) -> Optional[str]:
        """生成JWT token
        根据和风天气官方文档：https://dev.qweather.com/docs/configuration/authentication/#json-web-token
        - 算法必须是 EdDSA（Ed25519）
        - Header 必须包含 kid（凭据ID）
        - Payload 只需要 sub（项目ID）、iat（签发时间）、exp（过期时间）
        - iat 建议设置为当前时间前30秒，防止时间误差
        """
        if not self.key_id or not self.sub_id:
            logger.warning("警告: QW_KEY_ID 或 QW_SUB_ID 未设置")
            return None
        
        if not self._private_key:
            self._private_key = self._load_private_key()
            if not self._private_key:
                return None
        
        try:
            # 和风天气只支持 EdDSA 算法（Ed25519）
            algorithm = "EdDSA"
            
            # JWT payload（根据官方文档，只需要 sub, iat, exp）
            # 使用time.time()获取UTC时间戳（更准确，不受时区影响）
            current_timestamp = time.time()
            # iat 设置为当前时间前30秒，防止时间误差
            iat = int(current_timestamp) - 30
            # exp 设置为 iat + 15分钟（900秒），可以根据需要调整，最长24小时
            exp = iat + 900  # 15分钟
            
            payload = {
                "sub": self.sub_id,  # subject: 项目ID
                "iat": iat,  # issued at: 签发时间（当前时间前30秒）
                "exp": exp,  # expires: 过期时间
            }
            
            # Header 必须包含 kid（凭据ID）
            headers = {
                "kid": self.key_id
            }
            
            # 生成JWT token
            token = jwt.encode(
                payload,
                self._private_key,
                algorithm=algorithm,
                headers=headers
            )
            
            # 设置token过期时间（提前5分钟刷新）
            self._token_expires_at = exp - 300  # 提前5分钟刷新
            
            return token
        except Exception as e:
            logger.error(f"生成JWT token失败: {e}", exc_info=True)
            return None
    
    def _get_jwt_token(self) -> Optional[str]:
        """获取JWT token（带缓存和自动刷新）"""
        current_time = int(time.time())
        
        # 如果token已过期或即将过期，重新生成
        if not self._jwt_token or current_time >= self._token_expires_at:
            self._jwt_token = self._generate_jwt_token()
        
        return self._jwt_token
    
    async def get_weather_now(self, location_id: str = "101010400") -> Optional[Dict]:
        """获取实时天气
        
        Args:
            location_id: 位置ID，默认101010400（北京顺义区）
        
        Returns:
            天气数据字典，如果失败且缓存有效则返回缓存数据，否则返回None
        """
        token = self._get_jwt_token()
        if not token:
            logger.warning("无法获取JWT token")
            # 尝试使用缓存
            return self._get_cached_weather()
        
        try:
            async with httpx.AsyncClient(timeout=10.0) as client:
                url = f"{self.base_url}/weather/now"
                headers = {
                    "Authorization": f"Bearer {token}"
                }
                params = {
                    "location": location_id
                }
                
                response = await client.get(url, headers=headers, params=params)
                
                # 如果状态码不是200，打印详细错误信息
                if response.status_code != 200:
                    try:
                        error_data = response.json()
                        error_msg = f"和风天气API返回错误 (状态码 {response.status_code}): {error_data}"
                        logger.error(error_msg)
                    except Exception as parse_err:
                        error_msg = f"和风天气API返回错误 (状态码 {response.status_code}): {response.text}"
                        logger.error(f"{error_msg}, 解析响应失败: {parse_err}")
                
                response.raise_for_status()
                data = response.json()
                
                if data.get("code") == "200":
                    weather_data = data.get("now", {})
                    # 保存成功的请求结果到缓存
                    self._weather_cache = weather_data
                    self._weather_cache_time = time.time()
                    logger.debug(f"成功获取天气数据: {weather_data}")
                    return weather_data
                else:
                    error_msg = f"和风天气API返回错误: code={data.get('code')}, message={data.get('message', '')}"
                    logger.error(error_msg)
                    # 尝试使用缓存
                    return self._get_cached_weather()
        except httpx.HTTPStatusError as e:
            # 处理HTTP状态错误
            error_details = []
            error_details.append(f"HTTP状态错误: {e.response.status_code}")
            try:
                error_data = e.response.json()
                error_details.append(f"响应数据: {error_data}")
            except Exception as parse_err:
                try:
                    error_text = e.response.text[:500]  # 限制长度
                    error_details.append(f"响应文本: {error_text}")
                except:
                    error_details.append("无法读取响应内容")
                error_details.append(f"解析响应失败: {type(parse_err).__name__}: {parse_err}")
            
            error_msg = f"请求和风天气API失败 ({' | '.join(error_details)})"
            logger.error(error_msg, exc_info=True)
            # 尝试使用缓存
            return self._get_cached_weather()
        except httpx.HTTPError as e:
            error_type = type(e).__name__
            error_str = str(e) if str(e) else repr(e)
            error_msg = f"请求和风天气API失败: {error_type}: {error_str}"
            logger.error(error_msg, exc_info=True)
            # 尝试使用缓存
            return self._get_cached_weather()
        except Exception as e:
            error_type = type(e).__name__
            error_str = str(e) if str(e) else repr(e)
            error_msg = f"获取天气信息失败: {error_type}: {error_str}"
            logger.error(error_msg, exc_info=True)
            # 尝试使用缓存
            return self._get_cached_weather()
    
    def _get_cached_weather(self) -> Optional[Dict]:
        """获取缓存的天气数据（如果缓存有效）
        
        Returns:
            缓存的天气数据，如果缓存无效或不存在则返回None
        """
        if self._weather_cache is None:
            return None
        
        current_time = time.time()
        elapsed_time = current_time - self._weather_cache_time
        
        # 如果缓存在1小时内，返回缓存数据
        if elapsed_time < self._cache_valid_duration:
            logger.info(f"使用缓存的天气数据（缓存时间: {elapsed_time:.0f}秒前）")
            return self._weather_cache
        else:
            logger.warning(f"缓存已过期（{elapsed_time:.0f}秒前），无法使用")
            return None
    
    async def get_air_quality(self, location_id: str = "101010400") -> Optional[Dict]:
        """获取空气质量
        
        Args:
            location_id: 位置ID，默认101010400（北京顺义区）
        
        Returns:
            空气质量数据字典，如果失败返回None
        """
        token = self._get_jwt_token()
        if not token:
            logger.warning("无法获取JWT token (空气质量)")
            return None
        
        try:
            async with httpx.AsyncClient(timeout=10.0) as client:
                url = f"{self.base_url}/air/now"
                headers = {
                    "Authorization": f"Bearer {token}"
                }
                params = {
                    "location": location_id
                }
                
                response = await client.get(url, headers=headers, params=params)
                
                # 如果状态码不是200，打印详细错误信息
                if response.status_code != 200:
                    try:
                        error_data = response.json()
                        error_msg = f"和风天气API返回错误 (状态码 {response.status_code}): {error_data}"
                        logger.error(error_msg)
                    except Exception as parse_err:
                        error_msg = f"和风天气API返回错误 (状态码 {response.status_code}): {response.text}"
                        logger.error(f"{error_msg}, 解析响应失败: {parse_err}")
                
                response.raise_for_status()
                data = response.json()
                
                if data.get("code") == "200":
                    return data.get("now", {})
                else:
                    error_msg = f"和风天气API返回错误: code={data.get('code')}, message={data.get('message', '')}"
                    logger.error(error_msg)
                    return None
        except httpx.HTTPStatusError as e:
            # 处理HTTP状态错误
            error_details = []
            error_details.append(f"HTTP状态错误: {e.response.status_code}")
            try:
                error_data = e.response.json()
                error_details.append(f"响应数据: {error_data}")
            except Exception as parse_err:
                try:
                    error_text = e.response.text[:500]  # 限制长度
                    error_details.append(f"响应文本: {error_text}")
                except:
                    error_details.append("无法读取响应内容")
                error_details.append(f"解析响应失败: {type(parse_err).__name__}: {parse_err}")
            
            error_msg = f"请求和风天气API失败 (空气质量) ({' | '.join(error_details)})"
            logger.error(error_msg, exc_info=True)
            return None
        except httpx.HTTPError as e:
            error_type = type(e).__name__
            error_str = str(e) if str(e) else repr(e)
            error_msg = f"请求和风天气API失败 (空气质量): {error_type}: {error_str}"
            logger.error(error_msg, exc_info=True)
            return None
        except Exception as e:
            error_type = type(e).__name__
            error_str = str(e) if str(e) else repr(e)
            error_msg = f"获取空气质量信息失败: {error_type}: {error_str}"
            logger.error(error_msg, exc_info=True)
            return None
    
    async def get_weather_forecast(self, location_id: str = "101010400", days: str = "3d") -> Optional[list]:
        """获取天气预报
        
        Args:
            location_id: 位置ID，默认101010400（北京顺义区）
            days: 预报天数，可选值：3d, 7d, 10d, 15d, 30d，默认3d
        
        Returns:
            天气预报列表，如果失败返回None
        """
        token = self._get_jwt_token()
        if not token:
            logger.warning("无法获取JWT token (天气预报)")
            return None
        
        try:
            async with httpx.AsyncClient(timeout=10.0) as client:
                url = f"{self.base_url}/weather/{days}"
                headers = {
                    "Authorization": f"Bearer {token}",
                    "Accept-Encoding": "gzip"  # 接受gzip压缩
                }
                params = {
                    "location": location_id
                }
                
                response = await client.get(url, headers=headers, params=params)
                
                # 如果状态码不是200，打印详细错误信息
                if response.status_code != 200:
                    try:
                        error_data = response.json()
                        error_msg = f"和风天气API返回错误 (状态码 {response.status_code}): {error_data}"
                        logger.error(error_msg)
                    except Exception as parse_err:
                        error_msg = f"和风天气API返回错误 (状态码 {response.status_code}): {response.text}"
                        logger.error(f"{error_msg}, 解析响应失败: {parse_err}")
                
                response.raise_for_status()
                # httpx会自动处理gzip解压
                data = response.json()
                
                if data.get("code") == "200":
                    daily_list = data.get("daily", [])
                    return daily_list[:3] if days == "3d" else daily_list  # 只返回前3天
                else:
                    error_msg = f"和风天气API返回错误: code={data.get('code')}, message={data.get('message', '')}"
                    logger.error(error_msg)
                    return None
        except httpx.HTTPStatusError as e:
            # 处理HTTP状态错误
            error_details = []
            error_details.append(f"HTTP状态错误: {e.response.status_code}")
            try:
                error_data = e.response.json()
                error_details.append(f"响应数据: {error_data}")
            except Exception as parse_err:
                try:
                    error_text = e.response.text[:500]  # 限制长度
                    error_details.append(f"响应文本: {error_text}")
                except:
                    error_details.append("无法读取响应内容")
                error_details.append(f"解析响应失败: {type(parse_err).__name__}: {parse_err}")
            
            error_msg = f"请求和风天气API失败 (天气预报) ({' | '.join(error_details)})"
            logger.error(error_msg, exc_info=True)
            return None
        except httpx.HTTPError as e:
            error_type = type(e).__name__
            error_str = str(e) if str(e) else repr(e)
            error_msg = f"请求和风天气API失败 (天气预报): {error_type}: {error_str}"
            logger.error(error_msg, exc_info=True)
            return None
        except Exception as e:
            error_type = type(e).__name__
            error_str = str(e) if str(e) else repr(e)
            error_msg = f"获取天气预报失败: {error_type}: {error_str}"
            logger.error(error_msg, exc_info=True)
            return None


# 全局服务实例
_weather_service = None


def get_weather_service() -> QWeatherService:
    """获取天气服务实例（单例模式）"""
    global _weather_service
    if _weather_service is None:
        _weather_service = QWeatherService()
    return _weather_service

