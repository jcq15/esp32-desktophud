# Desktop HUD

桌面墨水屏显示项目，使用 ESP32-S3 控制 Waveshare 7.5 英寸墨水屏。

## 硬件

- **主控**: ESP32-S3
- **显示屏**: Waveshare 7.5 英寸墨水屏

## 项目结构

```
firmware/
├── firmware.ino           # 主程序入口
├── display_hal.h/cpp      # 显示硬件抽象层
├── region.h/cpp           # 区域定义（Region枚举和Rect结构）
├── data_hub.h/cpp         # 数据中心（统一管理所有数据）
├── widget.h               # Widget基类
├── widget_*.h/cpp         # 具体Widget实现（time/calendar/weather/note/status）
├── scheduler.h/cpp        # 调度器（统一管理Widget更新和渲染）
├── pins.h                 # 引脚定义
├── wifi_config.h/cpp      # WiFi连接实现
├── config/                # 配置文件目录
│   ├── config.h           # 实际配置文件（不提交到Git）
│   └── config_example.h   # 配置示例文件
└── ...
```

### 架构说明

项目采用模块化架构：

- **Region（区域）**: 定义屏幕的固定显示区域（TIME、CALENDAR、WEATHER、NOTE、STATUS）
- **Widget（组件）**: 每个区域对应一个Widget，负责维护显示模型和渲染
- **DataHub（数据中心）**: 统一管理所有数据，支持版本号机制
- **Scheduler（调度器）**: 统一调度所有Widget的更新和渲染，实现智能刷新策略

## 快速开始

### 1. 安装依赖库

在 Arduino IDE 中安装以下库：
- **ArduinoJson** (用于 JSON 解析)

### 2. 配置敏感信息

所有敏感配置（WiFi、API、Token等）都统一放在 `firmware/config/config.h` 文件中：

1. 复制 `firmware/config/config_example.h` 为 `firmware/config/config.h`
2. 编辑 `firmware/config/config.h`，填入你的实际配置：
   ```cpp
   // WiFi配置
   #define WIFI_SSID "你的WiFi名称"
   #define WIFI_PASSWORD "你的WiFi密码"
   
   // API服务器配置
   #define API_SERVER_URL "http://你的服务器地址/api"
   #define API_UPDATE_INTERVAL_MS 300000
   
   // 如果需要API认证
   // #define API_TOKEN "your_api_token"
   
   // 如果将来需要SSH配置
   // #define SSH_HOST "your-ssh-host.com"
   // #define SSH_USER "your_username"
   // #define SSH_PASSWORD "your_password"
   ```

**注意**: 
- `config/config.h` 文件包含所有敏感信息，已被 `.gitignore` 忽略，不会提交到 Git
- `config/config_example.h` 是配置模板，可以安全提交到 Git
- 所有配置项都集中在一个文件中，方便管理

### 3. 编译和上传

使用 Arduino IDE 编译并上传到 ESP32-S3。

## 服务器API数据格式

设备通过HTTP GET请求从服务器获取数据，服务器需要返回JSON格式的数据。

### API端点

- **URL**: 在 `config/config.h` 中配置的 `API_SERVER_URL`
- **方法**: GET
- **认证**: 可选，支持 `Authorization: Bearer <token>` 或 `X-API-Key: <key>`

### 响应格式

服务器应返回JSON格式的数据，包含以下字段：

#### 必需字段

所有字段都是可选的，但建议至少包含版本号以便设备判断数据是否更新。

#### 版本号（强烈推荐）

版本号用于判断数据是否更新，设备端会比较版本号来决定是否需要刷新显示。

```json
{
  "wx_ver": 123,      // 天气数据版本号（uint32）
  "cal_ver": 456,     // 日历数据版本号（uint32）
  "note_ver": 789,    // 笔记数据版本号（uint32）
  "global_ver": 1000  // 全局版本号（可选，uint32）
}
```

#### 天气数据（wx）

```json
{
  "wx": {
    "city": "北京",        // 城市名称（字符串）
    "t": 25,              // 温度（整数，摄氏度）
    "text": "多云",        // 天气描述（字符串）
    "aqi": "良(58)"       // 空气质量（字符串）
  }
}
```

#### 日历数据（cal）

```json
{
  "cal": {
    "date": "2026-02-17",    // 日期（字符串，格式：YYYY-MM-DD）
    "weekday": "周二",        // 星期（字符串）
    "lunar": "正月初一",      // 农历（字符串）
    "extra": "春节"          // 额外信息，如节日（字符串，可选）
  }
}
```

#### 笔记数据（note）

```json
{
  "note": [
    "喝水",
    "把外壳设计一下",
    "晚上刷全屏去残影"
  ]
}
```

**注意**: 最多支持10条笔记，超出部分会被忽略。

#### TTL（可选）

```json
{
  "ttl": 300  // 下次获取数据的间隔（秒，整数）
}
```

如果未提供TTL，设备将使用默认值（5分钟）。

### 完整示例

```json
{
  "wx_ver": 123,
  "cal_ver": 456,
  "note_ver": 789,
  "wx": {
    "city": "北京",
    "t": 25,
    "text": "多云",
    "aqi": "良(58)"
  },
  "cal": {
    "date": "2026-02-17",
    "weekday": "周二",
    "lunar": "正月初一",
    "extra": "春节"
  },
  "note": [
    "喝水",
    "把外壳设计一下",
    "晚上刷全屏去残影"
  ],
  "ttl": 300
}
```

### 版本号机制

设备端使用版本号来判断数据是否更新：

1. **首次请求**: 设备会获取所有数据并记录版本号
2. **后续请求**: 
   - 如果版本号未变化，设备不会刷新对应区域
   - 如果版本号变化，设备会更新对应区域的数据并刷新显示
3. **分区版本**: 每个数据类型（天气、日历、笔记）有独立的版本号，可以单独更新

### 最佳实践

1. **版本号管理**: 
   - 每次数据更新时递增对应的版本号
   - 可以使用时间戳、序列号或哈希值作为版本号
   - 建议使用递增的整数，便于调试

2. **TTL设置**: 
   - 根据数据更新频率设置合适的TTL
   - 天气数据建议30-60分钟
   - 日历数据建议每天更新一次
   - 笔记数据根据实际需求设置

3. **数据格式**: 
   - 所有字符串字段使用UTF-8编码
   - 温度使用整数类型
   - 日期格式建议使用 ISO 8601 (YYYY-MM-DD)

4. **错误处理**: 
   - 如果某个字段缺失，设备会使用默认值或保持上次的值
   - 建议服务器始终返回完整的JSON结构

5. **性能优化**: 
   - 如果数据未更新，可以返回较小的JSON（只包含版本号）
   - 使用HTTP缓存头（如ETag）可以减少不必要的传输

## 开发状态

🚧 项目正在开发中...

## License

待定

