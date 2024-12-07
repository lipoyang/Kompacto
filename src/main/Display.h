#pragma once
#include <iostream>
#include <cstdarg>

// 表示コマンドデータ構造体
struct DisplayData{
    int command;
    char data[3][11];
};
static const int CMD_NORMAL = 0; // 通常表示
static const int CMD_LARGE  = 1; // デカ文字表示

// 表示処理クラス (SubCore2に指令を送る)
class Display{
public:
    void begin();
    void update();
    bool isReady();
    void clear();
    void printf(int row, const char* format, ...)
    {
        va_list args;
        va_start(args, format);
        vsprintf(_displayData.data[row], format, args);
        va_end(args);
    }
    void setCommand(int command);
    void sendCommand(int command);
private:
    void send();
    
    DisplayData _displayData;
    DisplayData _sendData;
    bool _ready;
    bool _redraw;
};