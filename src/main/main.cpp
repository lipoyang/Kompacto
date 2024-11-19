#include <SFZSink.h>
#include "Kompacto.h"

// 鍵盤のキー1～8のピン番号
static const int PIN_KEYBOARD[] = {
    PIN_D00, PIN_D01, PIN_D02, PIN_D03,
    PIN_D04, PIN_D05, PIN_D06, PIN_D07
};

// 各弦の判定周波数[Hz] (音階とは特に関係なく、調整しやすい周波数でよい)
static const float FREQ_STRINGS[]={
    1125.0F,  // G線
    1453.0F,  // D線
    1593.0F,  // A線
    1687.0F,  // E線
};

// 琴のSFZデータを指定してSinkを生成
SFZSink sink("Koto.sfz");

// 定数とSinkを指定して楽器を生成
Kompacto inst(PIN_KEYBOARD, FREQ_STRINGS, sink);

// 初期化
void setup()
{
    // デバッグ用シリアル
    Serial.begin(115200);

    // メインボード上のLED
    pinMode(LED0, OUTPUT);
    pinMode(LED1, OUTPUT);
    pinMode(LED2, OUTPUT);
    pinMode(LED3, OUTPUT);

    // 楽器の初期化
    if (!inst.begin()) {
        Serial.println("ERROR: init error.");
        while (true) {
            delay(1000);
        }
    }

    ledOn(LED2);
    Serial.println("start!");
}

// メインループ
void loop()
{
    inst.update();
}
