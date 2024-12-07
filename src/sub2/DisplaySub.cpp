// OLEDの表示処理
#include <MP.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMono24pt7b.h>

// OLEDのI2Cアドレス
#define OLED_I2C_ADRS 0x3C

// OLEDデバイス
Adafruit_SH1106G OLED(128, 64, &Wire, -1);

// 表示コマンドデータ構造体
struct DisplayData{
    int command;
    char data[3][11];
};
static const int CMD_NORMAL = 0; // 通常表示
static const int CMD_LARGE  = 1; // デカ文字表示

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

// 初期化
void setup()
{
    // マルチコア起動
    int ret = MP.begin();
    if (ret < 0) {
      errorStop(2);
    }

    // OLED初期化
    if(!OLED.begin(OLED_I2C_ADRS, true)) {
        errorStop(5);
    }
//  OLED.display();
//  delay(2000);
    OLED.clearDisplay();
    OLED.setFont(&FreeMono9pt7b);
    OLED.setTextSize(1);
    OLED.setTextColor(SH110X_WHITE);
    OLED.setCursor(0,19);
    OLED.println("  Taisho");
    OLED.println(" KOmpacTO");
    OLED.display();
}

// メインループ
void loop()
{
    const int OK = 200;
    int8_t   msgid;
    DisplayData *rcvdata;

    // 受信待ち
    int ret = MP.Recv(&msgid, &rcvdata);
    if (ret < 0) {
        errorStop(3);
    }

    // 表示処理
    OLED.clearDisplay();
    if(rcvdata->command == CMD_NORMAL){
        OLED.setFont(&FreeMono9pt7b);
        OLED.setCursor(0,19);
        OLED.println(rcvdata->data[0]);
        OLED.println(rcvdata->data[1]);
        OLED.println(rcvdata->data[2]);
    }else if(rcvdata->command == CMD_LARGE){
        OLED.setFont(&FreeMono24pt7b);
        OLED.setCursor(30,50);
        OLED.println(rcvdata->data[0]);
    }else{
        OLED.setFont(&FreeMono9pt7b);
        OLED.setCursor(0,19);
        OLED.println("ERROR");
    }
    OLED.display();

    // 応答送信
    ret = MP.Send(msgid, OK);
    if (ret < 0) {
        errorStop(4);
    }
}
