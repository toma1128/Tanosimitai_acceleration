#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
esp_now_peer_info_t slave;

int catch_data = 0;       //M5stickから送られてきたら1になる変数
#define LOCKER 26     //ドアロック装置のピン番号
#define DOOR_SENS 25  //ドアセンサーのピン番号

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
  catch_data = data_len;
  for ( int i = 0 ; i < data_len ; i++ ) {
    Serial.print(data[i]);
    Serial.print(" ");
  }
  Serial.println("");
}
void setup() {
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

  pinMode(LOCKER, OUTPUT);
  pinMode(DOOR_SENS, INPUT_PULLUP);
  digitalWrite(LOCKER, HIGH);
  delay(10);
}

int count = 0;      //ドアが何回開いたか数える変数
int count2 = 0;     //ドアが開かれた回数を4回に一回カウントする変数
int old_count2 = 0; //前回のカウント2を保持する変数
int old_sens = 0;   //前回のセンサーの記録を保持する変数
int sens = 0;       //センサーの記録を読み取る変数

void loop() {
  sens = digitalRead(DOOR_SENS);
  if(sens != old_sens){
    count++;
    count2 = count / 4;
  }

  if(count % 4 == 0 && catch_data != 1){
    digitalWrite(LOCKER, HIGH);
    Serial.println("lock");
  }else if(catch_data == 1) {
    digitalWrite(LOCKER, LOW);
    Serial.println("open");
  }
  Serial.print("ドアセンサーカウント");
  Serial.println(sens);
  Serial.print("カウント :");
  Serial.println(count);
  Serial.print("カウント２ :");
  Serial.println(count2);
  Serial.println();

  if(count2 != old_count2) catch_data = 0;  //入って出たらロックするために通信で来た変数をリセットする

  old_count2 = count2;
  old_sens = sens;
  delay(100);
}
//お借りしたコード https://lang-ship.com/blog/work/m5stickc-esp-now-1/#toc10