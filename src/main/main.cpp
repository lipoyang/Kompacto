#include <SFZSink.h>
#include "Kompacto.h"

// 鍵盤のキー1～8のピン番号
static const int PIN_KEYBOARD[] = {
    PIN_D00, PIN_D01, PIN_D02, PIN_D03,
    PIN_D04, PIN_D05, PIN_D06, PIN_D07
};

// 各弦の判定周波数[Hz] (音階とは特に関係なく、調整しやすい周波数でよい)
float FREQ_STRINGS[]={
    1000.0F,  // G線
    2000.0F,  // D線
    2500.0F,  // A線
    1500.0F,  // E線
};

// 琴のSFZデータを指定してSinkを生成
SFZSink sink("Koto.sfz");

// 定数とSinkを指定して楽器を生成
Kompacto inst(PIN_KEYBOARD, FREQ_STRINGS, sink);

// UI処理
void ui_begin();
void ui_setupDone();
void ui_update();
void ui_onNoteOn(int note, float freq);

// 初期化
void setup()
{
    // UI処理初期化
    ui_begin();

    // 楽器の初期化
    if (!inst.begin()) {
        Serial.println("ERROR: init error.");
        while (true) {
            delay(1000);
        }
    }
    inst.onNoteOn = ui_onNoteOn;

    // 初期化完了表示
    ui_setupDone();
}

// メインループ
void loop()
{
    // 楽器の処理
    inst.update();

    // UIの処理
    ui_update();
}
