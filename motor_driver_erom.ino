#include <Arduino.h>
#include <Preferences.h>

#define uS_TO_S_FACTOR 1000000ULL   // 秒→マイクロ秒

// --- スリープ時間用 DIPスイッチ ---
// 1時間, 2時間, 3時間 の重み
#define SLEEP_SW1 32   // +1時間　青線
#define SLEEP_SW2 33   // +2時間　緑線
#define SLEEP_SW3 14   // +3時間　白線

// --- モータON時間用 DIPスイッチ ---
// 1分, 2分, 3分 の重み
#define MOTOR_SW1 16   // +1分　青線
#define MOTOR_SW2 26   // +2分　緑線
#define MOTOR_SW3 27   // +3分　白線

// --- モータドライバ用ピン ---
#define IN1 25   // モータ入力1
// #define IN2 26   // 必要なら使用

// Flash(NVS)に保存する
Preferences prefs;
uint32_t bootCount = 0;

// ===== DIP読み取り用 共通関数 =====

// DIP 3bit から 「0〜6（単位は呼び出し側次第）」を計算
int calcFromDip(bool b1, bool b2, bool b3) {
  int v = 0;
  if (b1) v += 1;
  if (b2) v += 2;
  if (b3) v += 3;
  return v;        // 0〜6
}

// スリープ時間（時間）をDIPから決める
int getSleepHours() {
  bool s1 = (digitalRead(SLEEP_SW1) == LOW);  // ON=LOW
  bool s2 = (digitalRead(SLEEP_SW2) == LOW);
  bool s3 = (digitalRead(SLEEP_SW3) == LOW);

  int hours = calcFromDip(s1, s2, s3);  // 0〜6時間
  return hours;
}

// モータON時間（分）をDIPから決める
int getMotorMinutes() {
  bool m1 = (digitalRead(MOTOR_SW1) == LOW);
  bool m2 = (digitalRead(MOTOR_SW2) == LOW);
  bool m3 = (digitalRead(MOTOR_SW3) == LOW);

  int minutes = calcFromDip(m1, m2, m3); // 0〜6分
  return minutes;
}

// --- モータ制御 ---
void controlMotor(int motorTimeSec) {
  if (motorTimeSec <= 0) {
    Serial.println("Motor ON time = 0秒");
    digitalWrite(IN1, LOW);
    return;
  }

  Serial.println("Motor: ON");
  digitalWrite(IN1, HIGH);

  unsigned long ms = (unsigned long)motorTimeSec * 1000UL;
  delay(ms);

  digitalWrite(IN1, LOW);
  Serial.println("Motor: OFF");
}

void setup() {
  Serial.begin(115200);
  delay(500);

  // DIPスイッチ入力（内部プルアップ）
  pinMode(SLEEP_SW1, INPUT_PULLUP);
  pinMode(SLEEP_SW2, INPUT_PULLUP);
  pinMode(SLEEP_SW3, INPUT_PULLUP);

  pinMode(MOTOR_SW1, INPUT_PULLUP);
  pinMode(MOTOR_SW2, INPUT_PULLUP);
  pinMode(MOTOR_SW3, INPUT_PULLUP);

  // モータピン初期化
  pinMode(IN1, OUTPUT);

  // Flash(NVS)から読み出して更新して保存
  prefs.begin("counter", false);                 // namespace="counter"

  //リセット用
  // prefs.remove("bootCount");                   
  
  //カウント用
  bootCount = prefs.getUInt("bootCount", 0);     // 無ければ0
  bootCount++;
  prefs.putUInt("bootCount", bootCount);         // 保存
  
  prefs.end();

  Serial.println("Boot number (NVS): " + String(bootCount));

  // DIPスイッチから設定を読み取る
  int sleepHours   = getSleepHours();     // 0〜6 時間
  int motorMinutes = getMotorMinutes();   // 0〜6 分

  int sleepTimeSec = sleepHours * 3600;   // 時間 → 秒
  int motorTimeSec = motorMinutes * 60;   // 分 → 秒

  Serial.println("Sleep time: " + String(sleepHours) + " h (" + String(sleepTimeSec) + " sec)");
  Serial.println("Motor ON time: " + String(motorMinutes) + " min (" + String(motorTimeSec) + " sec)");

  // モータ制御を実行
  controlMotor(motorTimeSec);

  // スリープ時間 0時間のときはDeepSleepしない
  if (sleepTimeSec <= 0) {
    Serial.println("Sleep time = 0秒");
    while (true) {
      delay(1000);
    }
  }

  // DeepSleepの設定
  esp_sleep_enable_timer_wakeup((uint64_t)sleepTimeSec * uS_TO_S_FACTOR);
  Serial.println("ESP32 will sleep for " + String(sleepTimeSec) + " seconds");

  Serial.println("Going to sleep now...");
  Serial.flush();

  // DeepSleep開始
  esp_deep_sleep_start();
}

void loop() {
  // DeepSleep後は再起動するので、ここは実行されない
}
