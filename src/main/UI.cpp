// UI処理
#include <Arduino.h>
#include <SDHCI.h>
#include <File.h>
#include "SwEncoder.h"
#include "Display.h"

SwEncoder encoder;  // スイッチ付きロータリーエンコーダ
Display display;    // OLED表示
SDClass SD;         // SDカード
File myFile;        // ファイル 

extern float FREQ_STRINGS[4];

static bool _hasNoteOn = false;
static bool _isNoteOn = false;
static int _note;
static uint32_t _t_noteOn;
static float _freq;

static bool  _tuning_mode = false;
static float _tuning_buff[4];
static int   _tuning_cnt = -1;
static int   _tuning_str = 0;

// チューニング表示
static void ui_display_tuning(float freq = 0)
{
    const char STRING[4] = {'G', 'D', 'A', 'E'};

    display.clear();
    display.printf(0, "Tuning");
    if(_tuning_cnt < 0){
        display.printf(1, "%c = %.0f", STRING[_tuning_str], FREQ_STRINGS[_tuning_str]);
    }else{
        display.printf(1, "%c : %d", STRING[_tuning_str], _tuning_cnt);
    } 
    if(freq > 0){
        display.printf(2, "%.0f", _freq);
    }
    display.setCommand(CMD_NORMAL);
}

// UI処理初期化
void ui_begin()
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

    // チューニング設定ファイルの読み込み
    if(!SD.begin()) {
        Serial.println("ERROR: No SD Card");
    }
    myFile = SD.open("tuning.dat", FILE_READ);
    if (myFile) {
        myFile.read(FREQ_STRINGS, sizeof(FREQ_STRINGS));
        myFile.close();
        for(int i = 0; i < 4; i++){
            Serial.printf("Freq[%d] = %.0f\n", i, FREQ_STRINGS[i]);
        }
    } else {
        myFile = SD.open("tuning.dat", FILE_WRITE);
        if (myFile) {
            Serial.println("No tuning.dat");
            myFile.write((uint8_t*)FREQ_STRINGS, sizeof(FREQ_STRINGS));
            myFile.close();
        }else{
            Serial.println("ERROR: can not create file");
        }
    }
}

// 初期化完了の表示
void ui_setupDone()
{
    ledOn(LED2);
    Serial.println("START!");
    display.clear();
    display.printf(1, "  START!");
    display.sendCommand(CMD_NORMAL);
    delay(500);
    display.clear();
    display.sendCommand(CMD_NORMAL);
}

// UI処理更新
void ui_update()
{
    const char NOTE[12][3] = {
        "C",  "C#", "D",  "D#", "E",  "F", 
        "F#", "G",  "G#", "A",  "A#", "B"
    };
  
    // エンコーダのスイッチ押された？
    encoder.update();
    // 長押し
    if(encoder.wasLongPushed()){
        // チューニング終了
        if(_tuning_mode){
            _tuning_mode = false;
            display.clear();
            display.printf(0, "Tuning");
            display.printf(1, "Completed");
            display.setCommand(CMD_NORMAL);

            myFile = SD.open("tuning.dat", FILE_WRITE);
            if (myFile) {
                myFile.seek(0);
                myFile.write((uint8_t*)FREQ_STRINGS, sizeof(FREQ_STRINGS));
                myFile.close();
            }else{
                Serial.println("ERROR: can not create file");
            }
        }
        // チューニング開始
        else{
            _tuning_mode = true;
            _tuning_str = 0;
            _tuning_cnt = -1;
            ui_display_tuning();
        }
    }
    // 短押し
    if(encoder.wasReleased()){
        if(_tuning_mode){
            if(_tuning_cnt < 0){
                _tuning_cnt = 0;
                ui_display_tuning();
            }else{
                _tuning_cnt = -1;
                ui_display_tuning();
            }
        }
    }

    // エンコーダの回転あった？
    int diff = encoder.readDiff();
    if(diff != 0){
        if(_tuning_mode){
            _tuning_str = (int)((unsigned int)(_tuning_str - diff) % 4);
            _tuning_cnt = -1;
            ui_display_tuning();
        }
    }

    // ノートオンあった？
    if(_hasNoteOn){
        _hasNoteOn = false;
        if(_tuning_mode == false) _isNoteOn = true;
        if(display.isReady()){
            if(_tuning_mode == false){
                // 通常モード
                if(_note > 0){
                    int note12 = _note % 12;
                    int octave = _note / 12 - 1;
                    display.clear();
                    display.printf(0, "%s%d", NOTE[note12], octave);
                    display.setCommand(CMD_LARGE);
                }
            }else{
                // チューニングモード
                if(_tuning_cnt >= 0){
                    _tuning_buff[_tuning_cnt] = _freq;
                    _tuning_cnt++;
                    if(_tuning_cnt >= 4){
                        float ave = 0;
                        for(int i = 0; i < 4; i++){
                            ave += _tuning_buff[i];
                        }
                        ave /= 4.0;
                        FREQ_STRINGS[_tuning_str] = ave;
                        _tuning_cnt = -1;
                    }
                }
                ui_display_tuning(_freq);
            }
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
void ui_onNoteOn(int note, float freq)
{
    _note = note;
    _freq = freq;
    _t_noteOn = millis();
    _hasNoteOn = true;
}
