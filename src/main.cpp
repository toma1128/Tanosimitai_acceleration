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

int minY,minU,minW; //下の加速度の値

int pattern = 2;        //筋トレの種類を切り替えるための変数
float count = 0;         //筋トレした回数をカウントする変数(エラーで回数+1されるため-1スタート)
bool approval = false;  //部屋を開けるのを承認する変数
int goal = 20;          //筋トレ達成目標回数

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
  sprite.fillCircle(X0 + (int)(accV * R3), Y0 - (int)(accU * R3), 8, RED);

  // 傾き補正
  M5.update();
  if(M5.BtnA.wasPressed()){
    accZ = max(min(accZ, 1.0), -1.0);
    theta = 180 / PI * asin(accZ);
  }
  
}
/**
 * 
*/
void counter(){
  switch(pattern){
    case 0 : {  //スクワッドの処理(Y軸メイン)
      Serial.println(accY);   //確認用
      if(fabs(oldY - accY) >= 0.3){
        count += 0.5;
        delay(425);  //判定後、少し待つ

        //エラーで回数を追加されるのを阻止するため現在の値を入れる
        measure();
        oldY = accY;
      }
      break;
    }
    case 1 : {  //腕立ての処理(Z軸メイン)
      Serial.println(accW); //確認用
      if(fabs(oldW - accW) >= 0.3){
        count++;
        delay(1000);

        //エラーで回数を追加されるのを阻止するため現在の値を入れる
        measure();
         oldW = accW;
      }
      break;
    }
    case 2 : {  //腹筋の処理()
      Serial.printf("X:%5.2fG\nY:%5.2fG\nZ:%5.2fG", accU, accY, accW);  //確認用
      if(fabs(oldY - accY) >= 0.3 && fabs(oldW - accW) >= 0.4){
        count++;
        delay(1500);

        //エラーで回数を追加されるのを阻止するため現在の値を入れる
        measure();
        oldY = accY;
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
    measure();  //加速度計測メソッド呼び出し
    counter();  //回数計測メソッド呼び出し

    if(count >= goal){    //筋トレが「goal」回終わると、部屋を開けるための変数がtrueになる
      approval = true;
      if(approval) {
        Serial.println("クリア");
        Serial.println();
      }
    }

    //前回の値を保存しておく処理
    oldU = accU;
    oldY = accY;
    oldW = accW;

    delay(150);   //  チャタリング防止&判定の時間を少し緩める
}

//Serial.printf("X:%5.2fG\nY:%5.2fG\nZ:%5.2fG", accU, accY, accW);  //加速度を確認するためのコード