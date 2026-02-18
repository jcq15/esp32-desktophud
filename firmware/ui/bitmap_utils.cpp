#include "bitmap_utils.h"

// Base64字符到值的映射表
static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Base64字符解码
int base64_decode_char(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    if (c == '=') return -2;  // padding标记
    return -1;  // 无效字符
}

// Base64解码 - 重写为更可靠的实现
bool base64_decode(const String& input, uint8_t* output, size_t* output_len) {
    // 移除所有空白字符
    String clean_input = "";
    clean_input.reserve(input.length());
    for (size_t i = 0; i < input.length(); i++) {
        char c = input[i];
        if (c != ' ' && c != '\n' && c != '\r' && c != '\t') {
            clean_input += c;
        }
    }
    
    size_t input_len = clean_input.length();
    
    if (input_len == 0) {
        *output_len = 0;
        return false;
    }
    
    size_t out_idx = 0;
    size_t i = 0;
    
    // 处理所有4字符组
    while (i < input_len) {
        if (i + 4 > input_len) {
            // 处理最后一个不完整的组
            break;
        }
        
        int val1 = base64_decode_char(clean_input[i]);
        int val2 = base64_decode_char(clean_input[i + 1]);
        int val3 = base64_decode_char(clean_input[i + 2]);
        int val4 = base64_decode_char(clean_input[i + 3]);
        
        // 检查padding
        bool has_padding = (val3 == -2 || val4 == -2);
        
        if (val1 < 0 || val2 < 0) {
            i += 4;
            continue;  // 跳过无效字符组
        }
        
        uint32_t combined = (val1 << 18) | (val2 << 12);
        
        // 第一个字节总是输出
        output[out_idx++] = (combined >> 16) & 0xFF;
        
        // 第二个字节：如果val3不是padding
        if (val3 >= 0) {
            combined |= (val3 << 6);
            output[out_idx++] = (combined >> 8) & 0xFF;
            
            // 第三个字节：如果val4不是padding
            if (val4 >= 0) {
                combined |= val4;
                output[out_idx++] = combined & 0xFF;
            }
        }
        
        i += 4;
    }
    
    // 处理剩余字符（如果有）
    if (i < input_len) {
        int val1 = base64_decode_char(clean_input[i]);
        int val2 = (i + 1 < input_len) ? base64_decode_char(clean_input[i + 1]) : -1;
        int val3 = (i + 2 < input_len) ? base64_decode_char(clean_input[i + 2]) : -1;
        int val4 = (i + 3 < input_len) ? base64_decode_char(clean_input[i + 3]) : -1;
        
        if (val1 >= 0 && val2 >= 0) {
            uint32_t combined = (val1 << 18) | (val2 << 12);
            output[out_idx++] = (combined >> 16) & 0xFF;
            
            if (val3 >= 0 && val3 != -2) {  // val3不是padding
                combined |= (val3 << 6);
                output[out_idx++] = (combined >> 8) & 0xFF;
                
                if (val4 >= 0 && val4 != -2) {  // val4不是padding
                    combined |= val4;
                    output[out_idx++] = combined & 0xFF;
                }
            }
        }
    }
    
    *output_len = out_idx;
    return true;
}

// 绘制位图（1bit位图，每个字节8个像素）
bool drawBitmapFromBase64(const String& base64Data, 
                          GxEPD2_BW<GxEPD2_750_T7, GxEPD2_750_T7::HEIGHT>& display,
                          int x, int y, int w, int h) {
    if (base64Data.length() == 0) {
        return false;
    }
    
    // 计算位图数据大小（1bit位图，每8个像素1字节）
    // 注意：服务器端已经将宽度对齐到8的倍数（例如500→504, 300→304）
    // 所以实际数据宽度 = ((w + 7) / 8) * 8
    int actual_width = ((w + 7) / 8) * 8;  // 对齐到8的倍数
    size_t bitmap_bytes = (actual_width / 8) * h;  // 每行字节数 * 行数
    
    // 分配缓冲区（动态分配可能不够，使用静态缓冲区）
    // 最大位图大小：800x480 = 48000字节，但实际区域更小
    // 根据实际需要调整：512x128 = 8192, 304x128 = 4864, 更大的可能需要更多
    const size_t MAX_BITMAP_SIZE = 16384;  // 增加到16KB，支持更大的位图
    static uint8_t bitmap_buffer[MAX_BITMAP_SIZE];
    
    if (bitmap_bytes > MAX_BITMAP_SIZE) {
        Serial.print("Bitmap too large: ");
        Serial.print(bitmap_bytes);
        Serial.print(" bytes, max: ");
        Serial.println(MAX_BITMAP_SIZE);
        return false;
    }
    
    // 解码base64
    Serial.print("[BitmapUtils] Base64 input length: ");
    Serial.println(base64Data.length());
    
    size_t decoded_len = 0;
    if (!base64_decode(base64Data, bitmap_buffer, &decoded_len)) {
        Serial.println("[BitmapUtils] Base64 decode failed");
        return false;
    }
    
    Serial.print("[BitmapUtils] Decoded bitmap: ");
    Serial.print(decoded_len);
    Serial.print(" bytes, expected: ");
    Serial.print(bitmap_bytes);
    Serial.print(" (display_w=");
    Serial.print(w);
    Serial.print(", actual_data_w=");
    Serial.print(actual_width);
    Serial.print(", h=");
    Serial.print(h);
    Serial.print(", bytes_per_row=");
    Serial.print(actual_width / 8);
    Serial.print(", total_rows=");
    Serial.println(h);
    
    // 如果解码长度不匹配，使用实际解码的长度（可能前端压缩了数据）
    // 但需要确保不超过预期大小
    if (decoded_len != bitmap_bytes) {
        Serial.print("[BitmapUtils] Warning: Bitmap size mismatch. Expected: ");
        Serial.print(bitmap_bytes);
        Serial.print(", got: ");
        Serial.println(decoded_len);
        
        if (decoded_len > bitmap_bytes) {
            Serial.println("[BitmapUtils] Decoded data too large, truncating");
            decoded_len = bitmap_bytes;
        } else if (decoded_len < bitmap_bytes) {
            // 如果解码的数据少于预期，可能是：
            // 1. base64解码不完整（需要修复）
            // 2. 前端发送的数据不完整
            // 3. 数据被压缩了
            Serial.print("[BitmapUtils] Decoded data smaller than expected. Missing: ");
            Serial.print(bitmap_bytes - decoded_len);
            Serial.println(" bytes");
            // 用0填充剩余部分（或者不填充，直接使用实际长度）
            // 这里我们使用实际解码的长度，让drawBitmap处理
        }
    }
    
    // 使用实际解码的长度（可能小于预期，但这是实际数据）
    size_t actual_bitmap_bytes = decoded_len;
    
    // 如果黑白反了，可以反转位图数据的每一位
    // 取消下面的注释来反转位图数据
    // for (size_t i = 0; i < decoded_len; i++) {
    //     bitmap_buffer[i] = ~bitmap_buffer[i];
    // }
    
    // 使用GxEPD2的drawBitmap函数绘制
    // drawBitmap(x, y, bitmap, w, h, foreground_color, background_color)
    // 对于1bit位图，1对应前景色，0对应背景色
    // 注意：位图数据的实际宽度是actual_width（8的倍数），但显示宽度是w
    // drawBitmap会自动处理这种情况，只显示前w个像素
    display.drawBitmap(x, y, bitmap_buffer, actual_width, h, GxEPD_WHITE, GxEPD_BLACK);
    
    return true;
}

