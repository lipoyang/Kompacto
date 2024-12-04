#pragma once

#include <stdbool.h>

// スイッチ付きロータリーエンコーダ
class SwEncoder{
public:
    void begin(int pinA, int pinB, int pinSW = -1);
    void update();
    void reset();
    int  readCount();
    int  readDiff();
    bool isPressed();
    bool wasReleased();
    static void isr(SwEncoder *encoder);
private:
    int _pinA;
    int _pinB;
    int _pinSW;
    bool _has_sw;
    int _cnt;
    int _cnt_prev;
    bool _sw_prev;
    bool _wasReloased;
    bool _initializing;
};