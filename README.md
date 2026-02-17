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
├── ui_pages.h/cpp         # UI 页面渲染
├── app_state.h            # 应用状态管理
├── pins.h                 # 引脚定义
├── wifi_config.h/cpp      # WiFi连接实现
├── api_client.h/cpp       # API客户端
├── config/                # 配置文件目录
│   ├── config.h           # 实际配置文件（不提交到Git）
│   └── config_example.h   # 配置示例文件
└── ...
```

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

## 开发状态

🚧 项目正在开发中...

## License

待定

