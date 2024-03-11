#include <Arduino.h>
#include <M5StickCPlus.h>
#include <esp_now.h>
#include <WiFi.h>
#include <math.h>
#include <cstdlib>

// peer設定
esp_now_peer_info_t slave;


// 送信コールバック
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print("Last Packet Sent to: ");
  Serial.println(macStr);
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
// 受信コールバック
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.printf("Last Packet Recv from: %s\n", macStr);
  Serial.printf("Last Packet Recv Data(%d): ", data_len);
  for ( int i = 0 ; i < data_len ; i++ ) {
    Serial.print(data[i]);
    Serial.print(" ");
  }
  Serial.println("");
}

TFT_eSprite sprite = TFT_eSprite(&M5.Lcd);  //速度センサーで使う変数

const int M5LED = 10;   //LEDのピン番号

float OFFSET_X = 0, OFFSET_Y = 0, OFFSET_Z = -0.08;
float accX;
float accY;             //Y軸
float accZ;
float accU;             //計算後のX軸
float accV; 
float accW;             //計算後のZ軸
float oldU, oldY, oldW; //前回の値を残しておく変数
float x, y, z, theta;
int R1 = 20, R2 = 40, R3 = 60;
int X0 = 120, Y0 = 67;

int training_pattern;        //筋トレの種類を切り替えるための変数
int count = 0;          //筋トレした回数をカウントする変数
bool approval = false;  //部屋を開けるのを承認する変数
const int GOAL = 10;    //筋トレ達成目標回数

bool minus = false;     //判定がマイナスの時にtrueにする変数
bool plus = false;      //判定がプラスの時にtrueにする変数

void setup() {
  M5.begin();
  M5.IMU.Init();
  pinMode(M5LED, OUTPUT);     //LEDの初期設定
  digitalWrite(M5LED, HIGH);  //LEDを消す(念のため)

  M5.Lcd.setRotation(3);
  Serial.begin(115200);

  // ESP-NOW初期化
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (esp_now_init() == ESP_OK) {
    Serial.println("ESPNow Init Success");
  } else {
    Serial.println("ESPNow Init Failed");
    ESP.restart();
  }
  // マルチキャスト用Slave登録
  memset(&slave, 0, sizeof(slave));
  for (int i = 0; i < 6; ++i) {
    slave.peer_addr[i] = (uint8_t)0xff;
  }
  esp_err_t addStatus = esp_now_add_peer(&slave);
  if (addStatus == ESP_OK) {
    // Pair success
    Serial.println("Pair success");
  }
  // ESP-NOWコールバック登録
  esp_now_register_send_cb(OnDataSent);
  esp_now_register_recv_cb(OnDataRecv);

  delay(10);
}

/**
 * 加速度の値をとって計算するメソッド
*/
void measure(){
    // 加速度データ取得
  M5.IMU.getAccelData(&x, &y, &z);
  
  accZ = -(x + OFFSET_X);
  accY = -(y + OFFSET_Y);
  accX = -(z + OFFSET_Z);
  accU = accX * sin(theta*PI/180) + accZ * cos(theta*PI/180);   //X軸の加速度
  accV = accY;
  accW = - accX * cos(theta*PI/180) + accZ * sin(theta*PI/180);
  
  sprite.setCursor(0,5,2);
  sprite.fillCircle(X0 + (int)(accV * R3), Y0 - (int)(accU * R3), 8, RED);
}

/**
 * 筋トレの回数を記録するためのメソッド
*/
void training(){
  switch(training_pattern){
    case 0 : {  //スクワッドの処理(Y軸メイン)

      if(accY < oldY && fabs(oldY - accY) >= 0.4 && !minus) { //現在のY軸がマイナスかつ絶対値の誤差が0.4の時(一度も処理を通ってない場合)
        minus = true;
        delay(250);
      }
      if(accY > oldY && fabs(oldY - accY) >= 0.4 && !plus){   //現在のY軸がプラスかつ絶対値の誤差が0.4の時(一度も処理を通ってない場合)
        plus = true;
        delay(250);
      } 

      if(plus && minus){  //上下どちらも通ったら
        count++;
        plus = false;
        minus = false;

        measure();
        oldY = accY;
      }
      break;
    }case 1 : {  //腕立ての処理(Z軸メイン)
      //Serial.println(accW); //確認用

      if(accW < oldW && fabs(oldW - accW) >= 0.2 && !minus) { //現在のZ軸がマイナスかつ絶対値の誤差が0.3の時(一度も処理を通ってない場合)
        minus = true;
        delay(200);
      }
      if(accW > oldW && fabs(oldW - accW) >= 0.2 && !plus){   //現在のZ軸がプラスかつ絶対値の誤差が0.3の時(一度も処理を通ってない場合)
        plus = true;
        delay(200);
      }

      Serial.println(plus);
      Serial.println(minus);

      if(plus && minus){  //上下どちらも通ったら
        count++;
        plus = false;
        minus = false;

        measure();
        oldW = accW;
      }
      break;
    }case 2 : {  //腹筋の処理(Z軸メイン)
      //Serial.printf("X:%5.2fG\nY:%5.2fG\nZ:%5.2fG\n", accU, accY, accW);  //確認用
      
      if(fabs(oldY - accY) >= 0.3){ //Y軸の絶対値の誤差が0.4かつZ軸の絶対値の誤差が0.3
        if(accY < oldY && !minus) { //現在のY軸がマイナスの時(一度も処理を通ってない場合)
          minus = true;
          delay(400);
        }
        if(accY > oldY && !plus){   //現在のY軸がプラスの時(一度も処理を通ってない場合)
          plus = true;
          delay(400);
        }
      }

      if(plus && minus){  //上下どちらも通ったら
        count++;
        plus = false;
        minus = false;

        measure();
        oldW = accW;
      }
      break;
    }
    default : break;
  }

  Serial.println();
  Serial.println(count);
  Serial.println();
}

void loop() {
  M5.update();
  if(M5.BtnA.wasPressed()){ //Aボタンが押されたら計測開始
    digitalWrite(M5LED, HIGH);  //LEDを消す
    training_pattern = rand() % 3;  //ランダムに筋トレを選択

    while(true){
      measure();  //加速度計測メソッド呼び出し
      training();  //筋トレ回数計測メソッド呼び出し
      
      if(count >= GOAL){    //筋トレが「goal」回終わると、部屋を開けるための変数がtrueになる

        digitalWrite(M5LED,LOW);    //LEDをつける
        M5.Beep.beep();             //一秒間音を鳴らす
        delay(1000);
        M5.Beep.end();              //音を消す

        uint8_t data[1] = {1};
        esp_err_t result = esp_now_send(slave.peer_addr, data, sizeof(data));
        Serial.print("Send Status: ");
        if (result == ESP_OK) {
          Serial.println("Success");
        } else if (result == ESP_ERR_ESPNOW_NOT_INIT) {
          Serial.println("ESPNOW not Init.");
        } else if (result == ESP_ERR_ESPNOW_ARG) {
          Serial.println("Invalid Argument");
        } else if (result == ESP_ERR_ESPNOW_INTERNAL) {
          Serial.println("Internal Error");
        } else if (result == ESP_ERR_ESPNOW_NO_MEM) {
          Serial.println("ESP_ERR_ESPNOW_NO_MEM");
        } else if (result == ESP_ERR_ESPNOW_NOT_FOUND) {
          Serial.println("Peer not found.");
        } else {
          Serial.println("Not sure what happened");
        }

        count = 0;                  //回数の初期化
        break;                      //ループから出る
      }

      //前回の値を保存しておく処理
      measure();
      oldU = accU;
      oldY = accY;
      oldW = accW;

      delay(90);   //判定の時間を少し緩める
    }
  }
  delay(10);
}
//Serial.printf("X:%5.2fG\nY:%5.2fG\nZ:%5.2fG", accU, accY, accW);  //加速度を確認するためのコード