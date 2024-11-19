// 「大正こンパクと」SubCore1
// メインコアの Kompactoクラスと通信し、音声信号をFFTしてピーク周波数を返す
// ssprocLib の YuruHorn_SubCore1.ino をベースに作成

// SPDX-License-Identifier: (Apache-2.0 OR LGPL-2.1-or-later)
// Copyright 2022 Sony Semiconductor Solutions Corporation

#include <MP.h>
#include <VoiceCapture.h>

// CMSISライブラリ
#define ARM_MATH_CM4
#define __FPU_PRESENT 1U
#include <arm_math.h>

#include "RingBuff.h"

/******************************
   プロトタイプ宣言 
 ******************************/
static void errorStop(int num);
static uint16_t getInputLevel(const int16_t *input, int length);
static void fft(float *pSrc, float *pDst, int fftLen);
static float getPeakFrequency(float *pData, int fftLen);

/******************************
   定数・変数 
 ******************************/

// FFTの長さ
#define FFTLEN 1024

// リングバッファ
#define INPUT_BUFFER (1024 * 4)
RingBuff ringbuf(INPUT_BUFFER);

// ヒープ確保
USER_HEAP_SIZE(64 * 1024);

// FFT用のバッファ
float pSrc[FFTLEN];
float pDst[FFTLEN];
float tmpBuf[FFTLEN];

// 検出パラメータ
const int SILENT_LEVEL = 100;  // 無音レベル (これ以下の振幅は無視)
const int VOLUME_THRESH = 800;  // 検出レベル (これ以上の音量なら有効と判定) 
const int FREQ_MIN = 800;      // 検出するピーク周波数の下限[Hz]
const int FREQ_MAX = 3500;     // 検出するピーク周波数の上限[Hz]

// エラーコード
enum { ERROR_MP_BEGIN = 2, ERROR_MP_SEND};

/******************************
   初期化とメインループ
 ******************************/

// 初期化
void setup()
{
    // マルチコア起動
    int ret = MP.begin();
    if (ret < 0) {
        MPLog("error: MP.begin => %d\n", ret);
        errorStop(ERROR_MP_BEGIN);
    }
    MP.RecvTimeout(MP_RECV_BLOCKING);

    // 初期化待ち合わせ
    while (true) {
        int8_t rcvid = -1;
        uint32_t msgdata = 0;
        ret = MP.Recv(&rcvid, &msgdata);
        if (rcvid == MSGID_INIT) {
            ret = MP.Send(MSGID_INIT_DONE, nullptr);
            if (ret < 0) {
                MPLog("MP.Send = %d\n", ret);
                errorStop(ERROR_MP_SEND);
            }
            break;
        }
    }

    MPLog("setup done\n");
}

// メインループ
void loop()
{
    uint16_t input_level = 0;

    // メインコアから音声信号を受信
    int8_t rcvid = -1;
    VoiceCapture::Capture *capture = nullptr;

    int ret = MP.Recv(&rcvid, &capture);
    if (ret < 0) {
        MPLog("MP.Recv = %d\n", ret);
        return;
    }
    if (rcvid != MSGID_SEND_CAPTURE) {
        MPLog("received = %d\n", rcvid);
        return;
    }
    if (capture == nullptr) {
        MPLog("received invalid data\n", rcvid);
        return;
    }

    ledOn(LED1);

    // 音声信号のレベル(音量)を計算
    input_level = getInputLevel((int16_t *)capture->data, CAP_FRAME_LENGTH);

    // 音声信号をリングバッファに入力
    ringbuf.put((q15_t *)capture->data, CAP_FRAME_LENGTH);

    // リングバッファからFFTサイズごとに取り出して処理
    while (ringbuf.stored() >= FFTLEN)
    {
        // リングバッファからFFTサイズぶん取り出す
        ringbuf.get(pSrc, FFTLEN, CAP_FRAME_LENGTH);

        // FFTを計算
        fft(pSrc, pDst, FFTLEN);

        // ピーク周波数を取得
        float peak = getPeakFrequency(pDst, FFTLEN);
        
        // メインコアに送信するメッセージ
        static VoiceCapture::Result result = {0, 0, 0, 0, 0, 0};

        result.id = capture->id;
        result.freq_denom = CAP_SAMPLING_FREQ;

        if (input_level > VOLUME_THRESH) {
            result.freq_numer = peak * capture->fs;
            result.volume = input_level;
        } else {
            result.freq_numer = 0.0f;
            result.volume = 0;
        }

        result.capture_time = capture->capture_time;
        result.result_time = millis();

        // デバッグ用
        if (capture->reserved == 1) {
            MPLog("frame=%d, capture_time=%d, freq=%d/%d, volume=%d, result_time=%d\n",  //
                  result.id,                                                             // frame
                  result.capture_time,                                                   // capture_time
                  result.freq_numer, result.freq_denom,                                  // freq (numer/denom)
                  result.volume,                                                         // volume
                  result.result_time);                                                   // result_time
        }

        // メインコアに送信
        MP.Send(MSGID_SEND_RESULT, &result);
    }
    ledOff(LED1);
}

/******************************
   内部関数
 ******************************/

// エラーで停止
// num : エラーコード
static void errorStop(int num)
{
    const int BLINK_TIME_MS = 300;
    const int INTERVAL_MS = 1000;
    while (true) {
        for (int i = 0; i < num; i++) {
            ledOn(LED1);
            delay(BLINK_TIME_MS);
            ledOff(LED1);
            delay(BLINK_TIME_MS);
        }
        delay(INTERVAL_MS);
    }
}

// 入力レベル(音量)を計算
// input : 入力信号のバッファ
// length : 入力信号のサンプル数
// return : 入力レベル
static uint16_t getInputLevel(const int16_t *input, int length)
{
    // 無音レベルを超える振幅(絶対値)の平均を計算する
    uint32_t sum = 0;
    int count = 0;

    for (int i = 0; i < length; i++) {
        if (input[i] < -SILENT_LEVEL) {
            sum = sum - input[i];
            count++;
        } else if (input[i] > SILENT_LEVEL) {
            sum = sum + input[i];
            count++;
        }
    }

    if (count > 0) {
        return (uint16_t)(sum / count);
    } else {
        return 0;
    }
}

// FFTの計算
// pSrc : 入力バッファ (時間領域)
// pDst : 出力バッファ (周波数領域)
// fftLen : FFTの長さ
static void fft(float *pSrc, float *pDst, int fftLen)
{
    arm_rfft_fast_instance_f32 S;

#if (FFTLEN == 512)
    arm_rfft_512_fast_init_f32(&S);
#elif (FFTLEN == 1024)
    arm_rfft_1024_fast_init_f32(&S);
#endif

    // 複素FFT ( {実, 虚, 実, 虚,...} の順に格納される )
    arm_rfft_fast_f32(&S, pSrc, tmpBuf, 0); // 0:順FFT / 1:逆FFT

    // 複素数の大きさ(振幅)を計算
    arm_cmplx_mag_f32(&tmpBuf[2], &pDst[1], fftLen / 2 - 1);
    pDst[0] = tmpBuf[0];
    pDst[fftLen / 2] = tmpBuf[1];
}

// ピーク周波数を計算
// pData : データバッファ(周波数領域)
// fftLen : FFTの長さ
// return : ピーク周波数[Hz]
static float getPeakFrequency(float *pData, int fftLen)
{
    float Fs = (float)CAP_SAMPLING_FREQ; // サンプリング周波数[Hz]

    // 検出する周波数の範囲をインデックスに換算
    uint32_t index_min = (uint32_t)(FREQ_MIN * fftLen / Fs);
    uint32_t index_max = (uint32_t)(FREQ_MAX * fftLen / Fs);
    uint32_t index_range = index_max - index_min;

    // 最大値とその位置を取得
    uint32_t index;
    float maxValue;
    arm_max_f32(&pData[index_min], index_range, &maxValue, &index);
    index += index_min;

    // より正確なピーク位置をサブサンプル補間
    float delta = 0.5 * (pData[index - 1] - pData[index + 1]) / 
                  (pData[index - 1] + pData[index + 1] - (2.0f * pData[index]));

    // 周波数に換算
    float peakFs = (index + delta) * Fs / (fftLen - 1);

    return peakFs;
}
