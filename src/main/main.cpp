#include <SFZSink.h>
#include "Kompacto.h"
#include "SwEncoder.h"
#include "Display.h"

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

// スイッチ付きロータリーエンコーダ
SwEncoder encoder;

// OLED表示
Display display;

// UI処理 (仮)
static void ui_update();
static void ui_onNoteOn(int note);

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

    // OLEDとロータリーエンコーダの初期化
    display.begin();
    encoder.begin(PIN_D08, PIN_D09, PIN_D10);

    // 楽器の初期化
    if (!inst.begin()) {
        Serial.println("ERROR: init error.");
        while (true) {
            delay(1000);
        }
    }
    inst.onNoteOn = ui_onNoteOn;

    // 初期化完了の表示
    ledOn(LED2);
    Serial.println("START!");
    display.clear();
    display.printf(1, "  START!");
    display.sendCommand(CMD_NORMAL);
    delay(500);
    display.clear();
    display.sendCommand(CMD_NORMAL);
}

// メインループ
void loop()
{
    // 楽器の処理
    inst.update();

    // UIの処理(仮)
    ui_update();
}

/**************************************************
  UI処理(仮)
 **************************************************/
static bool _hasNoteOn = false;
static bool _isNoteOn = false;
static int _note;
static uint32_t _t_noteOn;

// UI処理更新(仮)
static void ui_update()
{
    const char NOTE[12][3] = {
        "C",  "C#", "D",  "D#", "E",  "F", 
        "F#", "G",  "G#", "A",  "A#", "B"
    };
    static int pressCnt = 0;
  
    // エンコーダのスイッチ押された？
    encoder.update();
    if(encoder.wasReleased()){
        printf("pressed\n");
        pressCnt++;
        display.clear();
        display.printf(0, "button cnt");
        display.printf(1, "%d", pressCnt);
        display.setCommand(CMD_NORMAL);
    }

    // エンコーダの回転あった？
    int diff = encoder.readDiff();
    static int cnt = 0;
    if(diff != 0){
        cnt = encoder.readCount();
        printf("%d %d\n", diff, cnt);
        display.clear();
        display.printf(0, "encoder");
        display.printf(1, "%d %d", diff, cnt);
        display.setCommand(CMD_NORMAL);
    }

    // ノートオンあった？
    if(_hasNoteOn && (_note > 0)){
        _hasNoteOn = false;
        _isNoteOn = true;
        if(display.isReady()){
            int note12 = _note % 12;
            int octave = _note / 12 - 1;
            display.clear();
            display.printf(0, "%s%d", NOTE[note12], octave);
            display.setCommand(CMD_LARGE);
        }
    }
    if(_isNoteOn){
        uint32_t now = millis();
        uint32_t elapsed = now - _t_noteOn;
        if(elapsed > 1000){
            _isNoteOn = false;
            if(display.isReady()){
                display.clear();
                display.setCommand(CMD_LARGE);
            }
        }
    }

    // OLED表示更新
    display.update();
}

// UI処理 (ノートオン時)
static void ui_onNoteOn(int note)
{
    _note = note;
    _t_noteOn = millis();
    _hasNoteOn = true;
}
