// 「大正こンパクと」クラス
#include "Kompacto.h"
#include "note.h"

// 撥弦判定の閾値
static const int VOLUME_THRESH = 800;  // 音量閾値
static const int LENGTH_THRESH = 1;    // 時間閾値

// 撥弦判定の周波数の許容誤差[Hz]
static const float F_ERR = 50.0F;

// 音程のテーブル
static const int NOTE_TABLE[4][9]={
    // 開放   キー1,     キー2    キー3     キー4     キー5     キー6     キー7     キー8
    {NOTE_G3, NOTE_GS3, NOTE_A3, NOTE_AS3, NOTE_B3, REST,     NOTE_C4, NOTE_CS4, NOTE_D4}, // G線
    {NOTE_D4, NOTE_DS4, NOTE_E4, REST,     NOTE_F4, NOTE_FS4, NOTE_G4, NOTE_GS4, NOTE_A4}, // D線
    {NOTE_A4, NOTE_AS4, NOTE_B4, REST,     NOTE_C5, NOTE_CS5, NOTE_D5, NOTE_DS5, NOTE_E5}, // A線
    {NOTE_E5, REST,     NOTE_F5, NOTE_FS5, NOTE_G5, NOTE_GS5, NOTE_A5, NOTE_AS5, NOTE_B5}, // E線
};

// 開始する
bool Kompacto::begin()
{
    // 変数の初期化
    _last_note = INVALID_NOTE_NUMBER;

    // 鍵盤入力の初期化
    for(int i = 0; i < 8; i++){
        pinMode(PIN_KEYBOARD[i], INPUT_PULLUP);
    }

    // 親クラスのbegin()
    return VoiceCapture::begin();
}

// サブコアからピーク周波数検出を受けるコールバック
// freq_number : 周波数の分子
// freq_denom  : 周波数の分母  (freq_number / freq_denom が Hz単位になる)
// volume : 音量
void Kompacto::onCapture(unsigned int freq_number, unsigned int freq_denom, unsigned int volume)
{
    // 立ち上がり
    if (_prev_volume < VOLUME_THRESH && volume >= VOLUME_THRESH) {
        _detect_cnt = 0;
    }
    // 持続
    else if (volume >= VOLUME_THRESH) {
        // 時間閾値に達したら有効な撥弦とみなす
        _detect_cnt++;
        if(_detect_cnt == LENGTH_THRESH){
            // 周波数
            float freq = (float)freq_number / (float)freq_denom;
            printf("Detect freq %f, volume %d\n", freq, volume);

            // 弦の判定
            int pick = -1;
            for(int i = 0; i < 4; i++){
                if( (freq > FREQ_STRINGS[i] - F_ERR) && 
                    (freq < FREQ_STRINGS[i] + F_ERR) )
                {
                    pick = i;
                    break;
                }
            }
            if(pick >= 0){
                // 音量
                int velocity = volume / 20;
                if(velocity > 127) velocity = 127;

                // 鍵盤の判定
                int key = 0; // 0:開放弦
                for(int i = 0; i < 8; i++){
                    if(digitalRead(PIN_KEYBOARD[7 - i]) == LOW){
                        key = 8 - i; // 1～8 : キー1～8押下
                        break;
                    }
                }
                // 出力
                int note = NOTE_TABLE[pick][key];
                if(_last_note != INVALID_NOTE_NUMBER){
                    VoiceCapture::sendNoteOff(_last_note, DEFAULT_VELOCITY, DEFAULT_CHANNEL);
                }
                VoiceCapture::sendNoteOn(note, DEFAULT_VELOCITY, DEFAULT_CHANNEL);
                _last_note = note;
                if(onNoteOn != nullptr) onNoteOn(note);
                
                printf("string %d, note %d, velocity %d \n", pick, note, velocity);
            }
        }
    }
    // 立ち下がり
    if (_prev_volume >= VOLUME_THRESH && volume < VOLUME_THRESH) {
        //VoiceCapture::sendNoteOff(note, DEFAULT_VELOCITY, DEFAULT_CHANNEL);
    }
    _prev_volume = volume;
}