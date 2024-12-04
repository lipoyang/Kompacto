// スイッチ付きロータリーエンコーダ (割り込み使用)

#include <Arduino.h>
#include "SwEncoder.h"

#define MAX_SW_ENCODER  4   // インスタンスは最大4個まで

static SwEncoder *_sw_encoder[MAX_SW_ENCODER];
static int _sw_encoder_index = 0;

static void isr0(){ SwEncoder::isr(_sw_encoder[0]); }
static void isr1(){ SwEncoder::isr(_sw_encoder[1]); }
static void isr2(){ SwEncoder::isr(_sw_encoder[2]); }
static void isr3(){ SwEncoder::isr(_sw_encoder[3]); }
static void (*isr_table[MAX_SW_ENCODER])() = { isr0, isr1, isr2, isr3 };

void SwEncoder::isr(SwEncoder *encoder)
{
  if(encoder->_initializing) return; // trick

  if(digitalRead(encoder->_pinA) == digitalRead(encoder->_pinB)){
    encoder->_cnt++;
  }else{
    encoder->_cnt--;
  }
}

void SwEncoder::begin(int pinA, int pinB, int pinSW)
{
    _pinA = pinA;
    _pinB = pinB;
    _pinSW = pinSW;
    _cnt = 0;
    _cnt_prev = 0;
    _sw_prev = false;
    _has_sw = (pinSW > 0) ? true : false;
    _wasReloased = false;

    pinMode(pinA,  INPUT_PULLUP);
    pinMode(pinB,  INPUT_PULLUP);
    if(_has_sw){
        pinMode(pinSW, INPUT_PULLUP);
    }
    delay(100);

    _initializing = true; // trick
    if(_sw_encoder_index < MAX_SW_ENCODER){
        _sw_encoder[_sw_encoder_index] = this;
        attachInterrupt(pinA, isr_table[_sw_encoder_index], RISING);
        _sw_encoder_index++;
    }
    _initializing = false; // trick
}

void SwEncoder::update()
{
    if(_has_sw)
    {
        bool sw = (digitalRead(_pinSW) == LOW) ? true : false;
        if((_sw_prev == true) && (sw == false)){
            _wasReloased = true;
        }
        _sw_prev = sw;
    }
}

void SwEncoder::reset()
{
    _cnt = 0;
    _cnt_prev = 0;
}

int  SwEncoder::readCount()
{
    return _cnt;
}

int  SwEncoder::readDiff()
{
    int val = _cnt - _cnt_prev;
    _cnt_prev = _cnt;
    return val;
}

bool SwEncoder::isPressed()
{
    bool ret = (_has_sw && digitalRead(_pinSW) == LOW) ? true : false;
    return ret;
}

bool SwEncoder::wasReleased()
{
    bool ret = _wasReloased;
    _wasReloased = false;
    return ret;
}

