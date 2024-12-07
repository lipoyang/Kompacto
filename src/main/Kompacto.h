#pragma once

#include <VoiceCapture.h>

// 「大正こンパクと」クラス
// VoiceCaptureクラスを継承 (マイク入力からピーク周波数と音量を取得できる)
class Kompacto : public VoiceCapture
{
public:
    Kompacto(const int* pin_keyboard, const float* f_strings, Filter& filter)
        : VoiceCapture(filter),
          PIN_KEYBOARD(pin_keyboard),
          FREQ_STRINGS(f_strings),
          onNoteOn(nullptr)
        {};

    bool begin() override;

    void (*onNoteOn)(int);

protected:
    void onCapture(unsigned int freq_numer, unsigned int freq_denom, unsigned int volume) override;

private:
    const int* PIN_KEYBOARD;    // キーボードのピン番号のテーブル
    const float* FREQ_STRINGS;  // 弦の周波数のテーブル
    unsigned int _prev_volume;  // 前回の音量
    int _detect_cnt;            // 有効な検出の連続カウンタ
    int _last_note;             // 各弦の最後の音符
};

