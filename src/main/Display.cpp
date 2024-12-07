// 表示処理クラス (SubCore2に指令を送る)

#include <Arduino.h>
#include <MP.h>
#include "Display.h"

// OLEDを制御するサブコアの番号
static const int SUBCORE = 2;

// メッセージID 
static const int8_t SEND_ID = 100;

// 初期化する
void Display::begin()
{
    _ready = true;
    int ret = MP.begin(SUBCORE);
    if (ret < 0) {
        ::printf("Display: MP.begin error = %d\n", ret);
    }
}

// 更新する
void Display::update()
{
    const int OK = 200;

    MP.RecvTimeout(MP_RECV_POLLING);

    int8_t rcvid;
    int rcvdata;
    int ret = MP.Recv(&rcvid, &rcvdata, SUBCORE);
    if (ret < 0 && ret != -11) {
        ::printf("Display: MP.Recv error = %d\n", ret);
    }else if (ret > 0) {
        if(rcvid == SEND_ID && rcvdata == OK){
            // printf("Display: received\n");
            _ready = true;
        }else{
            ::printf("Display: Error rcvid = %d, rcvdata = %d\n", rcvid, rcvdata);
        }
    }
}

// レディか？ (描画中でないか？)
bool Display::isReady()
{
    return _ready;
}

// バッファのクリア
void Display::clear()
{
    _displayData.data[0][0] = '\0';
    _displayData.data[1][0] = '\0';
    _displayData.data[2][0] = '\0';
}

// コマンド送信
void Display::send(int command)
{
    _displayData.command = command;

    int ret = MP.Send(SEND_ID, &_displayData, SUBCORE);
    if (ret < 0) {
      ::printf("MP.Send error = %d\n", ret);
    }
    _ready = false;
}
