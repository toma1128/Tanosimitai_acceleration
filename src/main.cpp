#include <Arduino.h>
#include <M5StickCPlus.h>
#include <math.h>

TFT_eSprite sprite = TFT_eSprite(&M5.Lcd);

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

int pattern = 0;    //筋トレの種類を切り替えるための変数
int count = 0;      //筋トレした回数をカウントする変数
bool approval = false;  //部屋を開けるのを承認する変数

void setup() {
  M5.begin();
  M5.IMU.Init();
  M5.Lcd.setRotation(3);
  Serial.begin(115200);
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
  //Serial.printf("X:%5.2fG\nY:%5.2fG\nZ:%5.2fG", accU, accY, accW);

  sprite.fillCircle(X0 + (int)(accV * R3), Y0 - (int)(accU * R3), 8, RED);

  // 傾き補正
  M5.update();
  if(M5.BtnA.wasPressed()){
    accZ = max(min(accZ, 1.0), -1.0);
    theta = 180 / PI * asin(accZ);
  }
  
}


void loop() {
    measure();  //計測メソッド呼び出し

    //筋トレの種類によって処理を分ける
    switch(pattern){
        case 0 : {  //スクワット
            Serial.println(accY);   //確認用
            if(fabs(oldY - accY) >= 0.4){   //前回との誤差が0.4以上あれば
                count++;
            }
            break;
        }

        default : break;
    }

    if(count >= 20){    //筋トレ20回終われば、部屋を開けるための変数がtrueになる
        approval = true;
    }

    //前回の値を保存しておく処理
    oldU = accU;
    oldY = accY;
    oldW = accW;

  delay(350);   //  チャタリング防止&判定の時間を少し緩める

}