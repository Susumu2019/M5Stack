#include <M5Stack.h>
#include <WiFi.h>
#include <time.h>
#include "AudioFileSourceSD.h"
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2S.h"
//#include <SD.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <EMailSender.h>

#define DA_SCK 3    // クロック信号出力ピン
#define DA_CS 1     // 選択動作出力ピン
#define DA_SDI 16   // データ信号出力ピン
#define DA_LDAC 17  // ラッチ動作出力ピン

// #include <sys/statfs.h>
// #include <SDHCI.h>
// SDClass SD;

//system
String ver = "0.9 20240408";

// Time
char ntpServer[] = "ntp.jst.mfeed.ad.jp";
const long gmtOffset_sec = 9 * 3600;
const int  daylightOffset_sec = 0;
struct tm timeinfo;
String dateStr;
String timeStr;
int8_t count_timer_10 = 0;
int8_t count_timer_10_one = 0;
int8_t count_timer_100 = 0;
int8_t count_timer_100_one = 0;
int8_t flicker_100 = 0;
int16_t count_timer_1000 = 0;
int16_t count_timer_1000_one = 0;
int16_t count_timer_LCD_one = 0;
int8_t count_timer_record_60000 = 0;//1秒単位でカウントアップ
int8_t count_timer_record_one = 0;
int16_t count_flicker_1000 = 0;
int16_t count_beep_100 = 0;
float millis_re = 0;
hw_timer_t * timer = NULL;

//volatile uint32_t counter = 0;
//volatile uint32_t current_time = 0;

//e-mail
char myMailAdr[100]  = "example@gmail.com";
char myMailPass[100] = "";
char toMailAdr[100]  = "example@gmail.com";//
EMailSender emailSend(myMailAdr, myMailPass);
//EMailSender emailSend("smtp.account@gmail.com", "mpdvpfpofvyaygfj");
//EMailSender(myMailAdr,myMailPass,myMailAdr,"smtp.account@gmail.com",25);
//EMailSender(const char* email_login, const char* email_password, const char* email_from, const char* smtp_server, uint16_t smtp_port);
int8_t sendmail_number = 0;//メールの送信回数

//wifi
String wifi_ssid = "";//32文字まで
String wifi_pass = "";//64文字まで
String tmp_wifi_ssid;
String tmp_wifi_pass;
int8_t wifi_status = 0;
int16_t wifi_count = 0;
int8_t wifi_timecheck = 0;
long wifi_rssi = 0;
String wifi_ip = "000.000.000.000";
int8_t wifi_lastip = 0;
int8_t wifi_timeout_count = 0;
String wifi_ssid_letter = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ -_";
String wifi_pass_letter = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!-_<>()[]{}#+=~$@|&^*/\\\"'`.,:;";
int8_t wifi_n = 0;
int8_t wifi_scan_count = 0;
int8_t wifi_scan_finish = 0;
int8_t wifissid_array_max = 0;

class wifi_scan_status {
  public:
  //int8_t no;
  String ssid;
  //int8_t channel;
  //long rssi;
  //String encryption;
};
wifi_scan_status get_ssid[8];

//Audio
AudioGeneratorWAV *wav;
AudioFileSourceSD *file;
AudioOutputI2S *out;
int8_t wav_play1 = 0;
char wav_file[] = "/test_sound_22.05k_16bit_mo.wav";

//File
File testfile;
File rootdir;
String getdirlist[100];
const char* btnevent_log_filepath = "/btnevent_log.csv";
const char* record_filepath = "/record.csv";
int8_t record_on = 0;//

//Web
WebServer server(80); // WebServerオブジェクト作成

//SW
//※長押し処理を入れるとややこしくなるので要注意
//長押しをやめたときに0を入れてもすぐに1に戻ってしまい、0にするタイミングが難しい。
int8_t btnA_on = 0;
int8_t btnB_on = 0;
int8_t btnC_on = 0;
int8_t btnA_one = 0;
//int8_t btnA_L_one = 0;
int8_t btnB_one = 0;
//int8_t btnB_L_one = 0;
int8_t btnC_one = 0;
//int8_t btnC_L_one = 0;

//GPIO
int16_t gpio_a35_in = 0;;         // アナログ入力値格納用
//float ad_val;         // アナログ入力値格納用
int8_t gpio_a26_out = 100;         // アナログ出力値指定用
//float da_val;         // アナログ出力値指定用
float v_in = 0;       // アナログ入力電圧換算値
float v_out = 0;      // アナログ出力電圧換算用
int8_t gpio_d2_in = 0;
int8_t gpio_d5_out = 0;

//SPI
int8_t MCP4921_on = 0;//DACを有効化
int16_t MCP4921_outa = 0;//OUT Aの出力値設定
int16_t MCP4921_outb = 0;//OUT Bの出力値設定
//MCP4822 MCP(255, 255, &SPI1);

//その他
int16_t sys_step = 0;
int8_t lcd_rewriting = 0;
int8_t startup_sound = 1;
int16_t cursor_index = 0;
//int8_t cursor_edit_index = 0;//編集する文字位置
int8_t letter_index = 0;//修正選択中の文字番号
int8_t letter_edit = 0;//修正中
int8_t get_letter_number = 0;//
int32_t error_count = 0;

void onTimer(){
  count_timer_10_one = 1;
  if(count_timer_100==9){count_timer_100 = 0;
    count_timer_100_one = 1;
    count_timer_LCD_one = 1;
    if(flicker_100 == 0){flicker_100 = 1;}else{flicker_100 = 0;}
    if(count_beep_100 == 0){}else{count_beep_100--;}//ビープ音長さ
  }else{                  count_timer_100++;  }

  if(count_timer_1000==99){count_timer_1000 = 0;
    count_timer_1000_one = 1;
    //Serial.printf("millis= %u ms\n", millis());//デバッグ用

    if(count_timer_record_60000<59){
      count_timer_record_60000++;
    }else{
      count_timer_record_60000 = 0;
      count_timer_record_one = 1;
    }

    if(wifi_timeout_count>0){wifi_timeout_count--;}
    if(wifi_scan_count>0){wifi_scan_count--;}

    if(count_flicker_1000 == 0){  count_flicker_1000 = 1;
    }else{                        count_flicker_1000 = 0;}
  }else{                    count_timer_1000++;      }

}

void writeData_testfile(char *paramStr) {
  File file;
  // SDカードへの書き込み処理（ファイル追加モード）
  // SD.beginはM5.begin内で処理されているので不要
  file = SD.open(btnevent_log_filepath, FILE_APPEND);

  file.printf("%04d/%02d/%02d,%02d:%02d:%02d,",
        timeinfo.tm_year+1900,  timeinfo.tm_mon + 1,
        timeinfo.tm_mday,       timeinfo.tm_hour,
        timeinfo.tm_min,        timeinfo.tm_sec);
  file.println(paramStr);
  //file.println(dateStr + "," + timeStr + "," + paramStr);
  //file.println(dateStr + "," + timeStr + "test code.");
  file.close();
}
void writeData_recordfile(char *paramStr) {
  File file;
  // SDカードへの書き込み処理（ファイル追加モード）
  // SD.beginはM5.begin内で処理されているので不要
  file = SD.open(record_filepath, FILE_APPEND);
  
  file.printf("%04d/%02d/%02d,%02d:%02d:%02d,",
        timeinfo.tm_year+1900,  timeinfo.tm_mon + 1,
        timeinfo.tm_mday,       timeinfo.tm_hour,
        timeinfo.tm_min,        timeinfo.tm_sec);

  file.println(paramStr);
  file.close();
}

void setup() {
  M5.begin();
  SD.begin();
  Serial.begin(115200);
  //M5.Power.begin();
  WiFi.mode(WIFI_STA);//Wifiは子機モード
  //WiFi.mode(WIFI_OFF); 

  //GPIO設定
  //I/O関係
  pinMode(2, INPUT_PULLUP);
  pinMode(5, OUTPUT); digitalWrite(5, LOW);
  pinMode(35, ANALOG);    // アナログ入力
  pinMode(26, OUTPUT);    // 出力端子に設定
  dacWrite(26, 0);        // 初期値0設定
  //SPI関係
  pinMode(DA_CS, OUTPUT);     // CS 出力端子に設定
  pinMode(DA_SCK, OUTPUT);     // SCK 出力端子に設定
  pinMode(DA_SDI, OUTPUT);    // SDI 出力端子に設定
  pinMode(DA_LDAC, OUTPUT);    // LDAC 出力端子に設定
  //digitalWrite(DA_CS,LOW) ; // CS
  digitalWrite(DA_CS,HIGH) ; // CS
  digitalWrite(DA_SCK,LOW) ;  // SCK
  digitalWrite(DA_LDAC,HIGH) ; // LDAC  

  //割り込み処理の初期化
  timer = timerBegin(0, 80, true);// タイマ作成
  timerAttachInterrupt(timer, &onTimer, true); //タイマ割り込みサービス・ルーチン onTimer を登録
  timerAlarmWrite(timer, 10000, true);// 割り込みタイミングの設定 10000=10msec
  timerAlarmEnable(timer);// タイマ有効化

  //LCD設定
  M5.Lcd.setBrightness(5);//0-255
  M5.Lcd.fillScreen(BLACK);

  //Web
  server.on("/", handleResponse); // URLにアクセスされた際の動作を登録
  server.onNotFound(handleNotFound); // server.onで登録されていないURLにアクセスされた際の動作を登録
  server.begin(); // クライアントからの接続応答待ちを開始
  M5.Lcd.println("HTTP server started");  

  LCD_Process();

  //起動音
  if(startup_sound!=0){
    M5.Speaker.begin();       // 
    M5.Speaker.setVolume(2);  //0-10
    M5.Speaker.tone(2000);  delay(100);
    M5.Speaker.tone(1000);  delay(100);
    M5.Speaker.mute();
  }

  Serial.begin(115200);//無くてもよい
  Serial.println("Start!");

  //send_mail("test e-mail.");
}

void loop() {

  //常に処理
  if(0==0){
    M5.update();
    server.handleClient();
    serial_Process();
    SW_Process();

    //Wifiの状態をチェック
    if(WiFi.status()==WL_CONNECTED){
      wifi_status = 1;    wifi_rssi = WiFi.RSSI();  wifi_ip = WiFi.localIP().toString().c_str();
      wifi_lastip = wifi_ip.substring(wifi_ip.lastIndexOf(".")+1).toInt();
    }else{
      wifi_status = 0;    wifi_rssi = 0;  wifi_ip = ""; wifi_lastip = 0;
    }

    if(wifi_status == 1 && wifi_timecheck == 0){
      //時間取得
      getTimeFromNTP();    wifi_timecheck = 1;
    }else{
      if(wifi_timeout_count==0){wifi_timeout_count = 10;
        WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());
      }
    }
   
    //GPIO
    gpio_d2_in = digitalRead(2);//デジタル入力
    digitalWrite(5, gpio_d5_out);//デジタル出力
    gpio_a35_in = analogRead(35);
    v_in = gpio_a35_in * (3.3 / 4095); // 3.3Vへ換算
    dacWrite(26, gpio_a26_out);
    v_out = gpio_a26_out * (3.3 / 255); // 3.3Vへ換算
     
    //WAV再生
    if(wav_play1 == 1){
      if (wav->isRunning()) {
        if(!wav->loop()){  //wav->stop();←いる？
        wav_play1 = 0;}
      }

    }

    //LCD処理
    if(count_timer_LCD_one!=0){count_timer_LCD_one = 0;
      LCD_Process();
    } 

    //Beep処理
    if(count_beep_100==0){
      M5.Speaker.mute();
    }

    //Record
    if(record_on == 1 && count_timer_record_one!=0){count_timer_record_one = 0;
      char tmp10[100];
      sprintf(tmp10,"gpio_a35_in,%lu",gpio_a35_in);
      writeData_recordfile(tmp10);

    }

  }

  if(sys_step >= 0 && sys_step < 100){
    if(sys_step == 0){
      //起動
    
      sys_step = 10;
    }else if(sys_step == 10){
      //初期化

      //Wifi初期化
      // char tmp_wifi_ssid[32];
      // char tmp_wifi_pass[32];
      // wifi_ssid.toCharArray(tmp_wifi_ssid,8);
      // wifi_pass.toCharArray(tmp_wifi_pass,8);
      sys_step = 100;
      //WiFi.begin(tmp_wifi_ssid, tmp_wifi_pass);    sys_step = 100;
    }
  }else if(sys_step >= 100 && sys_step < 200){
    if(sys_step == 100){
      M5.Lcd.fillScreen(BLACK); sys_step = 101; lcd_rewriting = 0;
    }else if(sys_step == 101){
      //待機

      if(btnA_one!=0){btnA_one = 0;//ENTER Key
        if(cursor_index == 0){//Exit
        }else if(cursor_index == 1){//Sys info
          sys_step = 200; cursor_index = 0;
        }else if(cursor_index == 2){//Wifi
          sys_step = 300; cursor_index = 0;
        }else if(cursor_index == 3){//Sound
          sys_step = 400; cursor_index = 0;
        }else if(cursor_index == 4){//I/O
          sys_step = 500; cursor_index = 0;
        }else if(cursor_index == 5){//SD
          sys_step = 600; cursor_index = 0;
        }else if(cursor_index == 6){//e-mail
          sys_step = 700; cursor_index = 0;
        }
      }

      cursor_control(6);
    }  
  }else if(sys_step >= 200 && sys_step < 300){
    if(sys_step == 200){//sys info
      M5.Lcd.fillScreen(BLACK); sys_step = 201; lcd_rewriting = 0;
    }else if(sys_step == 201){// 1/3
      if(btnA_one!=0){btnA_one = 0;//ENTER Key
        if(cursor_index == 0){        sys_step = 100; cursor_index = 1;//Exit
        }else if(cursor_index == 1){  sys_step = 205; cursor_index = 0;//Sys info
        }
      }
      cursor_control(1);
    }else if(sys_step == 205){
      M5.Lcd.fillScreen(BLACK); sys_step = 206; lcd_rewriting = 0;
    }else if(sys_step == 206){// 2/3
      if(btnA_one!=0){btnA_one = 0;//ENTER Key
        if(cursor_index == 0){        sys_step = 100; cursor_index = 1;//Exit
        }else if(cursor_index == 1){  sys_step = 210; cursor_index = 0;//URL QR code
        }else if(cursor_index == 2){  sys_step = 200; cursor_index = 0;//BACK
        }
      }
      cursor_control(2);
    }else if(sys_step == 210){
      M5.Lcd.fillScreen(BLACK); sys_step = 211; lcd_rewriting = 0;
    }else if(sys_step == 211){// 3/3
      if(btnA_one!=0){btnA_one = 0;//ENTER Key
        if(cursor_index == 0){        sys_step = 100; cursor_index = 1;//Exit
        }else if(cursor_index == 1){  sys_step = 205; cursor_index = 0;//BACK
        }
      }
      cursor_control(1);
    
    }
  }else if(sys_step >= 300 && sys_step < 400){
    if(sys_step == 300){//Wifi
      M5.Lcd.fillScreen(BLACK); sys_step = 301; lcd_rewriting = 0;
    }else if(sys_step == 301){
      if(btnA_one!=0){btnA_one = 0;//ENTER Key
        if(cursor_index == 0){        sys_step = 100; cursor_index = 2;//Exit
        }else if(cursor_index == 1){  sys_step = 305; cursor_index = 0;//SSID
        }else if(cursor_index == 2){  sys_step = 310; cursor_index = 0;//PASS
        }else if(cursor_index == 3){  sys_step = 315; cursor_index = 0;//SCAN
        }
      }
      cursor_control(3);
    }else if(sys_step == 305){//SSIDの編集
      M5.Lcd.fillScreen(BLACK); sys_step = 306; lcd_rewriting = 0;
      tmp_wifi_ssid = wifi_ssid;
      //strcpy(tmp_wifi_ssid,wifi_ssid);
    }else if(sys_step == 306){//
      if(btnA_one!=0){btnA_one = 0;//ENTER Key
        if(cursor_index == 0){
          wifi_ssid = tmp_wifi_ssid;        
          sys_step = 300; cursor_index = 1;//Exit
        }else if(cursor_index>=1 && cursor_index<=32){//32桁とする
          if(letter_edit==0){ letter_edit = 1;
            //選択決定した時の文字番号を取得
            get_letter_number = wifi_ssid_letter.indexOf(tmp_wifi_ssid[cursor_index-1]);
          }else{              letter_edit = 0;};//修正文字選択の切り替え
        }
      }
      if(letter_edit == 0){//編集文字選択中
        if(btnB_one!=0){//ButtonB Key
          M5.Lcd.fillRect(10,74,16*18,4,BLACK);//カーソル消去
          M5.Lcd.fillRect(10,109,16*18,4,BLACK);//カーソル消去
          //cursor_edit_index
        }
        if(btnC_one!=0){//ButtonC Key
          M5.Lcd.fillRect(10,74,16*18,4,BLACK);
          M5.Lcd.fillRect(10,109,16*18,4,BLACK);//カーソル消去
        }
        cursor_control(32);
      }else{//選択文字編集中      
        if(btnB_one!=0){btnB_one = 0;//ButtonB Key
          if(get_letter_number==64){  get_letter_number = 0;
          }else{                      get_letter_number++;}
        }
        if(btnC_one!=0){btnC_one = 0;//ButtonC Key
          if(get_letter_number==0){ get_letter_number = wifi_ssid_letter.length()-1;// = 64;
          }else{                    get_letter_number--;}
        }
      }
    }else if(sys_step == 310){//PASSの編集
      M5.Lcd.fillScreen(BLACK); sys_step = 311; lcd_rewriting = 0;
      tmp_wifi_pass=wifi_pass;
      //strcpy(tmp_wifi_ssid,wifi_pass);
    }else if(sys_step == 311){//
      if(btnA_one!=0){btnA_one = 0;//ENTER Key
        if(cursor_index == 0){
          wifi_pass = tmp_wifi_pass;     
          //wifi_ssid = tmp_wifi_ssid;     
          sys_step = 300; cursor_index = 2;//Exit
        }else if(cursor_index>=1 && cursor_index<=63){//63桁とする
          if(letter_edit==0){ letter_edit = 1;
            //選択決定した時の文字番号を取得
            get_letter_number = wifi_pass_letter.indexOf(tmp_wifi_pass[cursor_index-1]);
          }else{              letter_edit = 0;};//修正文字選択の切り替え
        }
      }

      if(letter_edit == 0){//編集文字選択中
        if(btnB_one!=0){//ButtonB Key
          M5.Lcd.fillRect(10,74,16*18,4,BLACK);//カーソル消去
          M5.Lcd.fillRect(10,109,16*18,4,BLACK);//カーソル消去
          M5.Lcd.fillRect(10,144,16*18,4,BLACK);//カーソル消去
          M5.Lcd.fillRect(10,179,16*18,4,BLACK);//カーソル消去
          //cursor_edit_index
        }
        if(btnC_one!=0){//ButtonC Key
          M5.Lcd.fillRect(10,74,16*18,4,BLACK);
          M5.Lcd.fillRect(10,109,16*18,4,BLACK);//カーソル消去
          M5.Lcd.fillRect(10,144,16*18,4,BLACK);
          M5.Lcd.fillRect(10,179,16*18,4,BLACK);//カーソル消去
        }
        cursor_control(64);
      }else{//選択文字編集中     
        if(btnB_one!=0){btnB_one = 0;//ButtonB Key
          if(get_letter_number==64){  get_letter_number = 0;//
          }else{                      get_letter_number++;}
        }
        if(btnC_one!=0){btnC_one = 0;//ButtonC Key
          if(get_letter_number==0){ get_letter_number = wifi_pass_letter.length()-1;
          }else{                    get_letter_number--;}
        }
      }

      if(btnA_one!=0){btnA_one = 0;//ENTER Key
        if(cursor_index == 0){        sys_step = 300; cursor_index = 2;//Exit
        // }else if(cursor_index == 1){  sys_step = 305; cursor_index = 0;//
        // }else if(cursor_index == 2){  sys_step = 310; cursor_index = 0;//
        }
      }
    }else if(sys_step == 315){//SCAN
      M5.Lcd.fillScreen(BLACK); sys_step = 316; lcd_rewriting = 0;

      //Wifiネットワークをスキャンするとそこで少し止まるからここで画面処理
      SetFromat2(WHITE,80, 110,2); M5.Lcd.printf("SSID Scan...");

      //先に取得済みのSSIDと関係する変数を初期化
      for (int8_t i = 0; i < 8; ++i) {
        get_ssid[i].ssid = "";
      }
      wifissid_array_max = 0;
      wifi_scan_finish = 0;

      //wifi_scan_count = 2;
      WiFi.disconnect();
      wifi_n = WiFi.scanNetworks();
    }else if(sys_step == 316){//
      if(btnA_one!=0){btnA_one = 0;//ENTER Key
        if(cursor_index == 0){        sys_step = 300; cursor_index = 3;//Exit
        }else{
          wifi_ssid = get_ssid[cursor_index-1].ssid;
          sys_step = 300; cursor_index = 3;//Exit
        }
      }
      cursor_control(wifissid_array_max);
    }
  }else if(sys_step >= 400 && sys_step < 500){
  
    if(sys_step == 400){
      M5.Lcd.fillScreen(BLACK); sys_step = 401; lcd_rewriting = 0;
    }else if(sys_step == 401){//Sound
      if(btnA_one!=0){btnA_one = 0;//ENTER Key
        if(cursor_index == 0){  sys_step = 100; cursor_index = 3;//Exit
        }else if(cursor_index == 1){  sys_step = 405; cursor_index = 0;//
        }else if(cursor_index == 2){  sys_step = 410; cursor_index = 0;//
        }
      }
      cursor_control(2);
    }else if(sys_step == 405){//BEEP
      M5.Lcd.fillScreen(BLACK); sys_step = 406; lcd_rewriting = 0;
    }else if(sys_step == 406){//BEEP
      if(btnA_one!=0){btnA_one = 0;//ENTER Key
        if(cursor_index == 0){  sys_step = 400; cursor_index = 1;//Exit
        }else if(cursor_index == 1){
          M5.Speaker.begin();
          M5.Speaker.tone(2000);count_beep_100 = 10;//1秒鳴らす
        }
      }
      cursor_control(1);
    }else if(sys_step == 410){//FILE
      M5.Lcd.fillScreen(BLACK); sys_step = 411; lcd_rewriting = 0;
    }else if(sys_step == 411){//FILE
      if(btnA_one!=0){btnA_one = 0;//ENTER Key
        if(cursor_index == 0){  sys_step = 400; cursor_index = 2;//Exit
        }else if(cursor_index == 1){

          file = new AudioFileSourceSD(wav_file);
          //file = new AudioFileSourceSD("/test_sound_22.05k_16bit_mo.wav");
          out = new AudioOutputI2S(0, 1); // Output to builtInDAC
          out->SetOutputModeMono(true);
          //out->SetGain((float)OUTPUT_GAIN/100.0);
          wav = new AudioGeneratorWAV();
          wav->begin(file, out);

          wav_play1 = 1;

          // 再生中の場合は、停止する
          // if(wav != NULL){
          //   wav->stop();
          // }
        }
      }
      cursor_control(1);
    }
  }else if(sys_step >= 500 && sys_step < 600){
    if(sys_step == 500){//I/O
      M5.Lcd.fillScreen(BLACK); sys_step = 501; lcd_rewriting = 0;
    }else if(sys_step == 501){//
      if(btnA_one!=0){btnA_one = 0;//ENTER Key
        if(cursor_index==0){      sys_step = 100; cursor_index = 4;//Exit
        }else if(cursor_index==1){sys_step = 505; cursor_index = 0;//Button
        }else if(cursor_index==2){sys_step = 510; cursor_index = 0;//GPIO
        }else if(cursor_index==3){sys_step = 530; cursor_index = 0;//SPI
        }
      }
      cursor_control(3);
    }else if(sys_step == 505){//Button
      M5.Lcd.fillScreen(BLACK); sys_step = 506; lcd_rewriting = 0;
      btnA_one = 0;   btnB_one = 0;   btnC_one = 0;
      // = 0; btnB_L_one = 0; btnC_L_one = 0;
    }else if(sys_step == 506){
      // if(btnB_on==1 && btnC_on==1){//Reset
      //   btnA_one = 0;   btnB_one = 0;   btnC_one = 0;
      //   btnA_L_one = 0; btnB_L_one = 0; btnC_L_one = 0;
      // }
      if(btnB_on==1 && btnC_on==1){
        sys_step = 500; cursor_index = 1;//Exit
      }
    }else if(sys_step == 510){//Button
      M5.Lcd.fillScreen(BLACK); sys_step = 511; lcd_rewriting = 0;
    }else if(sys_step == 511){
      if(btnA_one!=0){btnA_one = 0;//ENTER Key
        if(cursor_index==0){        sys_step = 500; cursor_index = 2;//Exit
        }else if(cursor_index==1){  sys_step = 515; cursor_index = 0;//
        }else if(cursor_index==2){  sys_step = 520; cursor_index = 0;//
        }
      }
      cursor_control(2);
    }else if(sys_step == 515){
      M5.Lcd.fillScreen(BLACK); sys_step = 516; lcd_rewriting = 0;
    }else if(sys_step == 516){
      if(btnA_one!=0){btnA_one = 0;//ENTER Key
        if(cursor_index==0){  sys_step = 510; cursor_index = 1;}//Exit
      }
      if(btnB_one!=0){btnB_one = 0;//B Key
        if(gpio_d5_out == 0){ gpio_d5_out = 1;
        }else{                gpio_d5_out = 0;}
      }
      if(btnC_one!=0){btnC_one = 0;//C Key
        if(gpio_d5_out == 0){ gpio_d5_out = 1;
        }else{                gpio_d5_out = 0;}
      }
      //cursor_control(1);
    }else if(sys_step == 520){
      M5.Lcd.fillScreen(BLACK); sys_step = 521; lcd_rewriting = 0;
    }else if(sys_step == 521){
      if(btnA_one!=0){btnA_one = 0;//ENTER Key
        if(cursor_index==0){  sys_step = 510; cursor_index = 2;}//Exit
      }
      if(btnB_one!=0){btnB_one = 0;//B Key
        if(gpio_a26_out == 255){  gpio_a26_out = 0;
        }else{                    gpio_a26_out++;}
      }
      if(btnC_one!=0){btnC_one = 0;//C Key
        if(gpio_a26_out == 0){  gpio_a26_out = 255;
        }else{                  gpio_a26_out--;}
      }
      //cursor_control(1);
    }else if(sys_step == 530){
      M5.Lcd.fillScreen(BLACK); sys_step = 531; lcd_rewriting = 0;
    }else if(sys_step == 531){
      if(btnA_one!=0){btnA_one = 0;//ENTER Key
        if(cursor_index==0){  sys_step = 500; cursor_index = 3;
        }else if(cursor_index==1){  sys_step = 535; cursor_index = 0;
        }//Exit
      }
      cursor_control(1);
    }else if(sys_step == 535){
      M5.Lcd.fillScreen(BLACK); sys_step = 536; lcd_rewriting = 0;
    }else if(sys_step == 536){
      if(btnA_one!=0){btnA_one = 0;//ENTER Key
        if(cursor_index==0){  sys_step = 530; cursor_index = 1;
        }else if(cursor_index==1){
          if(MCP4921_on==0){  MCP4921_on = 1; }else{  MCP4921_on = 0;}
        }//Exit
      }
      cursor_control(1);

      //
      if(MCP4921_on == 1){
        if(MCP4921_outa>4095){MCP4921_outa = 0;}else{MCP4921_outa++;}
        if(MCP4921_outb>4095){MCP4921_outb = 0;}else{MCP4921_outb++;}

        digitalWrite(DA_LDAC,HIGH); digitalWrite(DA_CS,LOW);
        DACout(DA_SDI,DA_SCK,0,MCP4921_outa);   //OUTAの値設定
        digitalWrite(DA_CS,HIGH);   digitalWrite(DA_LDAC,LOW);//出力
        
        digitalWrite(DA_LDAC,HIGH); digitalWrite(DA_CS,LOW);
        DACout(DA_SDI,DA_SCK,1,MCP4921_outb);   //OUTBの値設定
        digitalWrite(DA_CS,HIGH) ;  digitalWrite(DA_LDAC,LOW);//出力

      }
    }
  }else if(sys_step >= 600 && sys_step < 700){//SD
    
    if(sys_step == 600){//
      M5.Lcd.fillScreen(BLACK); sys_step = 601; lcd_rewriting = 0;
      
    }else if(sys_step == 601){
      if(btnA_one!=0){btnA_one = 0;//ENTER Key
        if(cursor_index==0){        sys_step = 100; cursor_index = 0;//Exit
        }else if(cursor_index==1){  sys_step = 605; cursor_index = 0;//Dir
        }else if(cursor_index==2){  sys_step = 610; cursor_index = 0;//TestFile
        }else if(cursor_index==3){  sys_step = 620; cursor_index = 0;//RecordFile
        }
      }
      cursor_control(3);
    }else if(sys_step == 605){
      M5.Lcd.fillScreen(BLACK); sys_step = 606; lcd_rewriting = 0;
      getdirlist_cls();//変数の中身を削除

      rootdir = SD.open("/");
      printDirectory(rootdir);
    }else if(sys_step == 606){
      if(btnA_one!=0){btnA_one = 0;//ENTER Key
        if(cursor_index==0){  sys_step = 600; cursor_index = 1;}//Exit
      }

      //cursor_control(2);
    }else if(sys_step == 610){
      M5.Lcd.fillScreen(BLACK); sys_step = 611; lcd_rewriting = 0;
      testfile.close();
    }else if(sys_step == 611){
      if(btnA_one!=0){btnA_one = 0;//ENTER Key
        if(cursor_index==0){  sys_step = 600; cursor_index = 2;//Exit
        }else if(cursor_index==1){  sys_step = 615; cursor_index = 0;//Operation record
        }else if(cursor_index==2){  SD.remove(btnevent_log_filepath);//Delete
        }
      }
      cursor_control(2);
    }else if(sys_step == 615){
      M5.Lcd.fillScreen(BLACK); sys_step = 616; lcd_rewriting = 0;
    }else if(sys_step == 616){
      if(btnA_one!=0){btnA_one = 0;//ENTER Key
        if(cursor_index==0){        sys_step = 610; cursor_index = 1;//Exit
        // }else if(cursor_index==1){  sys_step = 615; cursor_index = 2;
        // }else if(cursor_index==2){  sys_step = 615; cursor_index = 2;
        }
      }
      if(btnB_one!=0){btnB_one = 0;
        writeData_testfile("b");
      }
      if(btnC_one!=0){btnC_one = 0;
        writeData_testfile("c");
      }
    }else if(sys_step == 620){
      M5.Lcd.fillScreen(BLACK); sys_step = 621; lcd_rewriting = 0;
    }else if(sys_step == 621){
      if(btnA_one!=0){btnA_one = 0;//ENTER Key
        if(cursor_index==0){        sys_step = 600; cursor_index = 3;//Exit
        }else if(cursor_index==1){
          if(record_on == 0){record_on = 1;}else{record_on = 0;}
        }
      }
      cursor_control(1);
    }
  }else if(sys_step >= 700 && sys_step < 800){//e-mail
    if(sys_step == 700){//
      M5.Lcd.fillScreen(BLACK); sys_step = 701; lcd_rewriting = 0;
    }else if(sys_step == 701){
      if(btnA_one!=0){btnA_one = 0;//ENTER Key
        if(cursor_index==0){        sys_step = 100; cursor_index = 6;//Exit
        }else if(cursor_index==1){
          //メッセージタイトル作成
          EMailSender::EMailMessage message;
          message.subject = "Measured value by IoT unit";
          char tmp10[100];
          sprintf(tmp10,"date_time=%04d/%02d/%02d %02d:%02d:%02d<br>",
          timeinfo.tm_year+1900,  timeinfo.tm_mon + 1,
          timeinfo.tm_mday,       timeinfo.tm_hour,
          timeinfo.tm_min,        timeinfo.tm_sec);
          sprintf(tmp10,"%sgpio_a35_in=%u",tmp10,gpio_a35_in);
          message.message = tmp10;
          //添付ファイルの準備
          if (SD.exists(record_filepath)) {
            EMailSender::FileDescriptior fileDescriptor[1];
            fileDescriptor[0].filename = F("record.csv");
            fileDescriptor[0].url = F("/record.csv");
            fileDescriptor[0].mime = MIME_TEXT_PLAIN;
            fileDescriptor[0].storageType = EMailSender::EMAIL_STORAGE_TYPE_SD;
            EMailSender::Attachments attachs = {1, fileDescriptor};

            //送信
            EMailSender::Response resp = emailSend.send(toMailAdr, message, attachs);
          }else{
            //送信
            EMailSender::Response resp = emailSend.send(toMailAdr, message);
          }

          sendmail_number++;
        }
      }
      cursor_control(1);
    }
  }else if(sys_step >= 900 && sys_step < 1000){

    if(sys_step == 999){
      //エラー処理
      //cursor_control(wifissid_array_max);
    }
  }else {
    error_count++;
  }
}

void LCD_Process(){
  int16_t cursor_color[80] = {//cursor_indexの範囲に合わせる
    WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,
    WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,
    WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,
    WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,
    WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,
    WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,
    WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,
    WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,};
  //ushort dark_gray = getColor(80, 80, 80);
  // = M5.Lcd.color565(R,G,B)
  cursor_color[cursor_index] = RED;
  
  //Wifiのステータス表示
  int16_t wifi_icon_x = 302;
  int16_t wifi_icon_y = 20;
  int16_t wifi_icon[4] = {DARKGREY,DARKGREY,DARKGREY,DARKGREY};//TFT_LIGHTGREY,TFT_DARKGREY

  //常に処理
  if(0==0){

    //Wifiアンテナ表示
    if(wifi_rssi != 0){
      if(wifi_rssi >= -80){ wifi_icon[0] = WHITE;}
      if(wifi_rssi >= -75){ wifi_icon[1] = WHITE;}
      if(wifi_rssi >= -65){ wifi_icon[2] = WHITE;}
      if(wifi_rssi >= -50){ wifi_icon[3] = WHITE;}
    }
    M5.Lcd.fillRect(wifi_icon_x+15,wifi_icon_y-20,3,20,wifi_icon[3]);
    M5.Lcd.fillRect(wifi_icon_x+10,wifi_icon_y-15,3,15,wifi_icon[2]);
    M5.Lcd.fillRect(wifi_icon_x+5,wifi_icon_y-10,3,10,wifi_icon[1]);
    M5.Lcd.fillRect(wifi_icon_x,wifi_icon_y-5,3,5,wifi_icon[0]);
    SetFromat2(WHITE,260, 10,1);M5.Lcd.printf("IP:%03d",wifi_lastip);

    //日時表示
    if(wifi_timecheck==0){
      if(lcd_rewriting == 0){
        SetFromat(80,80,80,0, 0,2);
        M5.Lcd.printf("yyyy/mm/dd hh:nn:ss");
      }
    }else{
      //時刻取得と表示
      getLocalTime(&timeinfo);
      SetFromat(255,255,255,0, 0,2);
      M5.Lcd.printf("%04d/%02d/%02d %02d:%02d:%02d",
        timeinfo.tm_year+1900,  timeinfo.tm_mon + 1,
        timeinfo.tm_mday,       timeinfo.tm_hour,
        timeinfo.tm_min,        timeinfo.tm_sec);
    }

    //グラフィック描画
    M5.Lcd.drawLine(0,20,300,20,TFT_WHITE);
    M5.Lcd.drawLine(0,219,320,219,TFT_WHITE);

    //変数表示
    SetFromat2(WHITE,0, 220,1);
    M5.Lcd.printf("sys_step=%03d,cursor_index=%03d,error_count=%06d",
      sys_step,cursor_index,error_count);
    SetFromat2(WHITE,0, 230,1);
    M5.Lcd.printf("wifi_timeout_count=%02d,btnX_one=%01d%01d%01d",wifi_timeout_count,btnA_one,btnB_one,btnC_one);

  }

  if(sys_step == 0){
    SetFromat(255,255,255,160-120, 120-10,2);   M5.Lcd.printf("Start UP....");    
  }else if(sys_step >= 100 && sys_step < 200){
    //Menu表示
    if(lcd_rewriting == 0){lcd_rewriting = 1;
      SetFromat2(WHITE,0, 25,2); M5.Lcd.printf("Top menu");
    }
    SetFromat2(cursor_color[1],10, 45,2);  M5.Lcd.printf("Sys info");
    SetFromat2(cursor_color[2],10, 65,2);  M5.Lcd.printf("Wifi");
    SetFromat2(cursor_color[3],10, 85,2);  M5.Lcd.printf("Sound");
    SetFromat2(cursor_color[4],10, 105,2); M5.Lcd.printf("I/O");
    SetFromat2(cursor_color[5],10, 125,2); M5.Lcd.printf("SD");
    SetFromat2(cursor_color[6],10, 145,2); M5.Lcd.printf("e-mail");
    
    //

    //SetFromat2(WHITE,10, 145,2); M5.Lcd.printf("=%02d\n",wifi_pass_letter.length());

    SetFromat2(cursor_color[0],10, 200,2); M5.Lcd.printf("Exit");
  }else if(sys_step >= 200 && sys_step < 300){
    if(sys_step == 201){
      if(lcd_rewriting == 0){
        SetFromat2(WHITE,0, 25,2); M5.Lcd.printf("Top menu>Sys info 1/3");
        SetFromat2(WHITE,10, 45,2); M5.Lcd.printf("Template ver:%s",ver);
        SetFromat2(WHITE,10, 65,1); M5.Lcd.printf("Product:ESP32 Basic Core IoT Development Kit V2.6");
        SetFromat2(WHITE,10, 75,1); M5.Lcd.printf("ESP32-D0WDQ6-V3:");
        SetFromat2(WHITE,10, 85,1); M5.Lcd.printf(" 240MHz dual core, 600 DMIPS, 520KB SRAM, Wi-Fi");
        SetFromat2(WHITE,10, 95,1); M5.Lcd.printf("Flash:16MB");
        SetFromat2(WHITE,10, 105,1); M5.Lcd.printf("Input power:5V @ 500mA");
        SetFromat2(WHITE,10, 115,1); M5.Lcd.printf("Interface:TypeC x1, I2C x1");
        SetFromat2(WHITE,10, 125,1); M5.Lcd.printf("IO:G21,G22,G23,G19,G18,G3,G1,G16,G17,");
        SetFromat2(WHITE,10, 135,1); M5.Lcd.printf(" G2,G5,G25,G26,G35,G36");
        SetFromat2(WHITE,10, 145,1); M5.Lcd.printf("Button:Physical button x 3");
      }
      SetFromat2(cursor_color[1],10, 180,2); M5.Lcd.printf("NEXT");
      SetFromat2(cursor_color[0],10, 200,2); M5.Lcd.printf("EXIT");
    }else if(sys_step == 206){
      if(lcd_rewriting == 0){
        SetFromat2(WHITE,0, 25,2); M5.Lcd.printf("Top menu>Sys info 2/3");
        SetFromat2(WHITE,10, 45,1); M5.Lcd.printf("LCD screen:2.0 @320*240 ILI9342C IPS panel,");
        SetFromat2(WHITE,10, 55,1); M5.Lcd.printf(" maximum brightness 853nit");
        SetFromat2(WHITE,10, 65,1); M5.Lcd.printf("Speaker:1W-0928");
        SetFromat2(WHITE,10, 75,1); M5.Lcd.printf("USB chip:CH9102F");
        SetFromat2(WHITE,10, 85,1); M5.Lcd.printf("Antenna:2.4G 3D Antenna");
        SetFromat2(WHITE,10, 95,1); M5.Lcd.printf("Battery:110mAh @ 3.7V");
        SetFromat2(WHITE,10, 105,1); M5.Lcd.printf("Net weight:47.2g");
        SetFromat2(WHITE,10, 115,1); M5.Lcd.printf("Gross weight:93g");
        SetFromat2(WHITE,10, 125,1); M5.Lcd.printf("Product size:54mm x 54mm x 18mm");
      }
      SetFromat2(cursor_color[1],10, 160,2); M5.Lcd.printf("URL QR code\n");
      SetFromat2(cursor_color[2],10, 180,2); M5.Lcd.printf("BACK");
      SetFromat2(cursor_color[0],10, 200,2); M5.Lcd.printf("Exit");
    }else if(sys_step == 211){
      if(lcd_rewriting == 0){lcd_rewriting = 1;
        SetFromat2(WHITE,0, 25,2); M5.Lcd.printf("Top menu>Sys info 3/3");
        SetFromat2(WHITE,10, 50,1); M5.Lcd.printf("https://m5stack.com/");
        SetFromat2(WHITE,10, 60,1); M5.Lcd.printf("ESP32 Basic Core IoT");
        SetFromat2(WHITE,10, 70,1); M5.Lcd.printf(" Development Kit V2.6");
        M5.Lcd.qrcode("https://shop.m5stack.com/collections/m5-controllers/products/esp32-basic-core-iot-development-kit-v2-6",
          150, 50, 160, 5); //QRコード
        
      }
      SetFromat2(cursor_color[1],10, 180,2); M5.Lcd.printf("BACK");
      SetFromat2(cursor_color[0],10, 200,2); M5.Lcd.printf("Exit");
    }
  }else if(sys_step >= 300 && sys_step < 400){
    if(sys_step == 301){
      if(lcd_rewriting == 0){
        SetFromat2(WHITE,0, 25,2); M5.Lcd.printf("Top menu>Wifi");
        SetFromat2(cursor_color[1],10, 45,2); M5.Lcd.printf("SSID:%s",wifi_ssid.c_str());
        SetFromat2(cursor_color[2],10, 65,2); M5.Lcd.printf("PASS:%s",wifi_pass.c_str());
        SetFromat2(WHITE,10, 85,2);M5.Lcd.printf("IP:");M5.Lcd.print(WiFi.localIP());
        SetFromat2(WHITE,10, 105,2);M5.Lcd.printf("MAC:");M5.Lcd.print(WiFi.macAddress());
        SetFromat2(WHITE,5, 160,2); M5.Lcd.printf("*This wifi is 2.4ghz only.");
        SetFromat2(cursor_color[3],10, 180,2); M5.Lcd.printf("SCAN");
      }
      SetFromat2(WHITE,10, 130,2); M5.Lcd.printf("RSSI:");M5.Lcd.print(wifi_rssi);M5.Lcd.printf("  ");

      SetFromat2(cursor_color[0],10, 200,2); M5.Lcd.printf("Exit\n");
    }else if(sys_step == 306){//SSIDの編集
      if(lcd_rewriting == 0){
        SetFromat2(WHITE,0, 25,2); M5.Lcd.printf("Top menu>Wifi>SSID");
      }
      //SetFromat2(cursor_color[1],10, 45,2); M5.Lcd.printf(wifi_ssid);
      SetFromat2(WHITE,10,50,3);           M5.Lcd.printf(tmp_wifi_ssid.c_str());//
      //SetFromat2(WHITE,10,85,3);           M5.Lcd.printf("1234567890123456");//後々にフル桁数をテストするときに修正

      SetFromat2(WHITE,10,115,2);
      if(cursor_index != 0 && letter_edit == 1){
        //デバッグ用
        // M5.Lcd.printf("%01c ",tmp_wifi_ssid[cursor_index-1]);
        // M5.Lcd.printf("%01u ",wifi_ssid_letter.indexOf(tmp_wifi_ssid[cursor_index-1]));//指定文字を探す
        // M5.Lcd.printf("%02u ",get_letter_number);
        // M5.Lcd.println(wifi_ssid_letter.substring(get_letter_number,get_letter_number+1));//指定位置の文字を返す
        // M5.Lcd.printf("wifi_ssid_letter.length=%02u ",wifi_ssid_letter.length());
        // M5.Lcd.printf("get_letter_number=%02u ",get_letter_number);

        String tmp5 = wifi_ssid_letter.substring(get_letter_number,get_letter_number+1);
        int8_t tmp_len = tmp5.length()+1; 
        char tmp6[tmp_len];
        tmp5.toCharArray(tmp6,tmp_len);
        tmp_wifi_ssid.setCharAt(cursor_index-1,tmp6[0]);
      }else{
        // M5.Lcd.printf("Null");//デバッグ用
      }

      int16_t cursor_step_x = 18;
      //int16_t cursor_step_y = 30;

      if(letter_edit == 1 && (count_flicker_1000 == 0 || cursor_index == 0)){
        //修正中  点滅
        M5.Lcd.fillRect(10,74,16*18,4,BLACK);//カーソル消去
        M5.Lcd.fillRect(10,109,16*18,4,BLACK);//カーソル消去
      }else{
        //選択中
        if(cursor_index>0 && cursor_index<=16){
          M5.Lcd.fillRect((cursor_index*cursor_step_x)-8,74,16,4,RED);//カーソル表示
        }else if(cursor_index>=17 && cursor_index<=32){
          M5.Lcd.fillRect(((cursor_index-16)*cursor_step_x)-8,109,16,4,RED);//カーソル表示
        }
      }

      SetFromat2(WHITE,5, 140,2); M5.Lcd.printf("*Check the Internet for");
      SetFromat2(WHITE,5, 160,2); M5.Lcd.printf(" acceptable characters.");

      SetFromat2(cursor_color[0],10, 200,2); M5.Lcd.printf("Exit\n");
    }else if(sys_step == 311){//PASSの編集
      if(lcd_rewriting == 0){
        SetFromat2(WHITE,0, 25,2); M5.Lcd.printf("Top menu>Wifi>PASS");
      }
      SetFromat2(WHITE,10,50,3);  M5.Lcd.printf(tmp_wifi_pass.c_str());//
      // SetFromat2(WHITE,10,85,3);  M5.Lcd.printf("1234567890123456");//後々にフル桁数をテストするときに修正

      SetFromat2(WHITE,10,190,2);
      if(cursor_index != 0 && letter_edit == 1){
        //デバッグ用
        // M5.Lcd.printf("%01c ",tmp_wifi_pass[cursor_index-1]);
        // M5.Lcd.printf("%01u ",wifi_pass_letter.indexOf(tmp_wifi_pass[cursor_index-1]));//指定文字を探す
        // M5.Lcd.printf("%02u ",get_letter_number);
        // M5.Lcd.println(wifi_pass_letter.substring(get_letter_number,get_letter_number+1));//指定位置の文字を返す
        // M5.Lcd.printf("wifi_pass_letter.length=%02u ",wifi_pass_letter.length());
        // M5.Lcd.printf("get_letter_number=%02u ",get_letter_number);

        String tmp5 = wifi_pass_letter.substring(get_letter_number,get_letter_number+1);
        int8_t tmp_len = tmp5.length()+1; 
        char tmp6[tmp_len];
        tmp5.toCharArray(tmp6,tmp_len);
        tmp_wifi_pass.setCharAt(cursor_index-1,tmp6[0]);
      }else{
        //M5.Lcd.printf("Null");//デバッグ用
      }

      int16_t cursor_step_x = 18;
      //int16_t cursor_step_y = 30;

      if(letter_edit == 1 && (count_flicker_1000 == 0 || cursor_index == 0)){
        //修正中  点滅
        M5.Lcd.fillRect(10,74,16*18,4,BLACK);//カーソル消去
        M5.Lcd.fillRect(10,109,16*18,4,BLACK);//カーソル消去
      }else{
        //選択中
        if(cursor_index>0 && cursor_index<=16){
          M5.Lcd.fillRect((cursor_index*cursor_step_x)-8,74,16,4,RED);//カーソル表示
        }else if(cursor_index>=17 && cursor_index<=32){
          M5.Lcd.fillRect(((cursor_index-16)*cursor_step_x)-8,109,16,4,RED);//カーソル表示
        }else if(cursor_index>=33 && cursor_index<=48){
          M5.Lcd.fillRect(((cursor_index-32)*cursor_step_x)-8,144,16,4,RED);//カーソル表示
        }else if(cursor_index>=49 && cursor_index<=64){
          M5.Lcd.fillRect(((cursor_index-48)*cursor_step_x)-8,179,16,4,RED);//カーソル表示
        }
      }
      // SetFromat2(WHITE,5, 140,2); M5.Lcd.printf("*Check the Internet for");
      // SetFromat2(WHITE,5, 160,2); M5.Lcd.printf(" acceptable characters.");      
      SetFromat2(cursor_color[0],10, 200,2); M5.Lcd.printf("Exit\n");
    }else if(sys_step == 316){//SCAN
      SetFromat2(WHITE,0, 25,2); M5.Lcd.printf("Top menu>Wifi>SCAN");
    
      if(lcd_rewriting == 0 && wifi_n != 0){
        lcd_rewriting = 1;
        //scanの文字を削除
        SetFromat2(WHITE,80, 110,2); M5.Lcd.printf("            ");
      }
      
      if(wifi_n == 0){  
        SetFromat2(WHITE,80, 110,2); M5.Lcd.printf("  No WiFi.  ");
      }else{
        if(wifi_scan_finish == 0){wifi_scan_finish = 1;
          int8_t ssid_select_count = 0;
          int8_t ssid_add;
          for (int8_t i = 0; i < wifi_n; ++i) {
            ssid_add = wifissid_array(WiFi.SSID(i));
            if(ssid_add == 0){
              ssid_select_count++;
            }
            if(ssid_select_count==8){break;}
          }
          wifissid_array_max = ssid_add+1;
        }

        SetFromat2(WHITE,0,45,2);
        for (int8_t i = 0; i < 8; ++i) {
          SetFromat2(cursor_color[i+1],10, 40+(i*20),2); M5.Lcd.print(get_ssid[i].ssid);
        }

      }

      SetFromat2(cursor_color[0],10, 200,2); M5.Lcd.printf("Exit ");
      // if(cursor_index != 0){
      //   M5.Lcd.print(get_ssid[cursor_index-1].ssid);M5.Lcd.printf("   ");
      // }
    }

    //SetFromat2(cursor_color[0],20, 200,2); M5.Lcd.printf("Exit\n");
  }else if(sys_step >= 400 && sys_step < 500){//Sound
    if(sys_step == 401){
      SetFromat2(WHITE,0, 25,2); M5.Lcd.printf("Top menu>Sound");
      SetFromat2(cursor_color[1],10, 45,2);  M5.Lcd.printf("BEEP");
      SetFromat2(cursor_color[2],10, 65,2);  M5.Lcd.printf("FILE");
    }else if(sys_step == 406){
      SetFromat2(WHITE,0, 25,2); M5.Lcd.printf("Top menu>Sound>BEEP");
      SetFromat2(cursor_color[1],10, 45,2);  M5.Lcd.printf("PLAY 1sec");
      SetFromat2(WHITE,10, 160,2);  M5.Lcd.printf("*It cannot be used when");
      SetFromat2(WHITE,10, 180,2);  M5.Lcd.printf(" playing Wav files.");
    }else if(sys_step == 411){
      SetFromat2(WHITE,0, 25,2); M5.Lcd.printf("Top menu>Sound>FILE");
      SetFromat2(cursor_color[1],10, 45,2);  M5.Lcd.printf("PLAY");
      SetFromat2(WHITE,10, 70,1);  M5.Lcd.printf(wav_file);
      //SetFromat2(cursor_color[1],10, 45,2);  M5.Lcd.printf("PLAY test_sound.wav");
      // SetFromat2(cursor_color[2],10, 70,2);  M5.Lcd.printf("FILE\n");
    }

    SetFromat2(cursor_color[0],10, 200,2); M5.Lcd.printf("Exit");
  }else if(sys_step >= 500 && sys_step < 600){//I/O
    if(sys_step == 501){
      SetFromat2(WHITE,0, 25,2); M5.Lcd.printf("Top menu>I/O");
      SetFromat2(cursor_color[1],10, 45,2);  M5.Lcd.printf("Button\n");
      SetFromat2(cursor_color[2],10, 65,2);  M5.Lcd.printf("GPIO\n");
      SetFromat2(cursor_color[3],10, 85,2);  M5.Lcd.printf("SPI\n");

      SetFromat2(cursor_color[0],10, 200,2); M5.Lcd.printf("Exit\n");
    }else if(sys_step == 506){
      SetFromat2(WHITE,0, 25,2); M5.Lcd.printf("Top menu>I/O>Button");

      SetFromat2(WHITE,5, 45,2); M5.Lcd.printf("ButtonA");
      SetFromat2(WHITE,100, 45,2); M5.Lcd.printf("btnA_on = %01d",btnA_on);
      SetFromat2(WHITE,100, 65,2); M5.Lcd.printf("btnA_one = %01d",btnA_one);
      //SetFromat2(WHITE,100, 65,1); M5.Lcd.printf("btnA_L_one = %01d",btnA_L_one);

      SetFromat2(WHITE,5, 95,2); M5.Lcd.printf("ButtonB");
      SetFromat2(WHITE,100, 95,2); M5.Lcd.printf("btnB_on = %01d",btnB_on);
      SetFromat2(WHITE,100, 115,2); M5.Lcd.printf("btnB_one = %01d",btnB_one);
      //SetFromat2(WHITE,100, 105,1); M5.Lcd.printf("btnB_L_one = %01d",btnB_L_one);

      SetFromat2(WHITE,5, 145,2); M5.Lcd.printf("ButtonC");
      SetFromat2(WHITE,100, 145,2); M5.Lcd.printf("btnC_on = %01d",btnC_on);
      SetFromat2(WHITE,100, 165,2); M5.Lcd.printf("btnC_one = %01d",btnC_one);
      //SetFromat2(WHITE,100, 145,1); M5.Lcd.printf("btnC_L_one = %01d",btnC_L_one);
      
      //SetFromat2(WHITE,5, 160,2); M5.Lcd.printf("*Press button B+C to reset.");
      SetFromat2(WHITE,5, 200,2); M5.Lcd.printf("*Press button B+C to Exit.");
      //SetFromat2(cursor_color[0],10, 200,2); M5.Lcd.printf("Exit\n");
    }else if(sys_step == 511){
      SetFromat2(WHITE,0, 25,2); M5.Lcd.printf("Top menu>I/O>GPIO");

      SetFromat2(WHITE,10, 45,2); M5.Lcd.printf("DIN 2=%01d",gpio_d2_in);
      SetFromat2(cursor_color[1],10, 65,2); M5.Lcd.printf("DOUT 5=%01d",gpio_d5_out);
      SetFromat2(WHITE,10, 85,2); M5.Lcd.printf("AIN 35=%1.2f(%03d) ",v_in,gpio_a35_in);
      SetFromat2(cursor_color[2],10, 105,2); M5.Lcd.printf("AOUT 26=%1.2f(%03d) ",v_out,gpio_a26_out);
      SetFromat2(cursor_color[0],10, 200,2); M5.Lcd.printf("Exit\n");
    }else if(sys_step == 516){
      SetFromat2(WHITE,0, 25,2); M5.Lcd.printf("Top menu>I/O>GPIO>DOUT 5");
      SetFromat2(cursor_color[0],10, 45,2); M5.Lcd.printf("DOUT 5=%01d",gpio_d5_out);
      SetFromat2(WHITE,10, 200,2); M5.Lcd.printf("Press A button to exit");
    }else if(sys_step == 521){
      SetFromat2(WHITE,0, 25,2); M5.Lcd.printf("Top menu>I/O>GPIO>AOUT 26");
      SetFromat2(cursor_color[0],10, 45,2); M5.Lcd.printf("AOUT 26=%1.2f(%03d) ",v_out,gpio_a26_out);
      SetFromat2(WHITE,10, 200,2); M5.Lcd.printf("Press A button to exit");
    }else if(sys_step == 531){
      SetFromat2(WHITE,0, 25,2); M5.Lcd.printf("Top menu>I/O>SPI");

      SetFromat2(cursor_color[1],10, 45,2); M5.Lcd.printf("MCP4921 12bit DAC");
      SetFromat2(cursor_color[0],10, 200,2); M5.Lcd.printf("Exit\n");
    }else if(sys_step == 536){
      SetFromat2(WHITE,0, 25,2); M5.Lcd.printf("Top menu>I/O>SPI>MCP4921");
      SetFromat2(WHITE,10, 45,2); M5.Lcd.printf("OUT A=%04lu  ",MCP4921_outa);
      SetFromat2(WHITE,10, 65,2); M5.Lcd.printf("OUT B=%04lu  ",MCP4921_outb);
      SetFromat2(cursor_color[1],10, 85,2); M5.Lcd.printf("OUT ");
      if(MCP4921_on == 1){M5.Lcd.printf("ON ");}else{M5.Lcd.printf("OFF");}
      SetFromat2(WHITE,100, 105,2); M5.Lcd.printf("Connection");
      SetFromat2(WHITE,100, 125,2); M5.Lcd.printf("3. SCK  ");
      SetFromat2(WHITE,100, 145,2); M5.Lcd.printf("1. CS   ");
      SetFromat2(WHITE,100, 165,2); M5.Lcd.printf("16.SDI  ");
      SetFromat2(WHITE,100, 185,2); M5.Lcd.printf("17.LDAC ");
      
      SetFromat2(cursor_color[0],10, 200,2); M5.Lcd.printf("Exit\n");
    }
  }else if(sys_step >= 600 && sys_step < 700){//SD
    if(sys_step == 601){
      SetFromat2(WHITE,0, 25,2); M5.Lcd.printf("Top menu>SD");
      SetFromat2(cursor_color[1],10, 45,2);  M5.Lcd.printf("Dir\n");
      SetFromat2(cursor_color[2],10, 65,2);  M5.Lcd.printf("TestFile\n");
      SetFromat2(cursor_color[3],10, 85,2);  M5.Lcd.printf("RecordFile\n");

      SetFromat2(cursor_color[0],10, 200,2); M5.Lcd.printf("Exit\n");
    }else if(sys_step == 606){//Dir一覧
      int8_t tmp1 = 0;
      SetFromat2(WHITE,0, 25,2); M5.Lcd.printf("Top menu>SD>Dir\n");
      SetFromat2(WHITE,0, 45,2);
      for(tmp1=0;tmp1<7;tmp1++){
        M5.Lcd.print(" ");
        M5.Lcd.print(getdirlist[tmp1].substring(0,22));
        if(getdirlist[tmp1].length()>22){
          M5.Lcd.print("...");
        }
        M5.Lcd.print("\n");
      }
      if(tmp1>6){
        SetFromat2(WHITE,0, 160,2); M5.Lcd.printf("*There are many other");
        SetFromat2(WHITE,0, 180,2); M5.Lcd.printf(" files.");
      }
      SetFromat2(cursor_color[0],10, 200,2); M5.Lcd.printf("Exit\n");
    }else if(sys_step == 611){//
      SetFromat2(WHITE,0, 25,2); M5.Lcd.printf("Top menu>SD>TestFile");

      SetFromat2(cursor_color[1],10, 45,2); M5.Lcd.printf("Operation record");
      SetFromat2(cursor_color[2],10, 65,2); M5.Lcd.printf("Delete");
      if (SD.exists(btnevent_log_filepath)) {
        SetFromat2(WHITE,10, 85,2); M5.Lcd.printf("Existence:YES");
        SetFromat2(WHITE,10, 105,2); M5.Lcd.printf("Size:");
        File testfile = SD.open(btnevent_log_filepath);M5.Lcd.print(testfile.size());//ファイルサイズ表示
        M5.Lcd.printf("byte");
      }else{
        SetFromat2(WHITE,10, 85,2); M5.Lcd.printf("Existence:NO ");
        SetFromat2(WHITE,10, 105,2); M5.Lcd.printf("Size:-byte     ");
      }
      
      SetFromat2(cursor_color[0],10, 200,2); M5.Lcd.printf("Exit\n");
    }else if(sys_step == 616){//
      SetFromat2(WHITE,0, 25,2); M5.Lcd.printf("Top menu>SD>TestFile>");
      SetFromat2(WHITE,0, 45,2); M5.Lcd.printf("Operation record");

      if (SD.exists(btnevent_log_filepath)) {//ファイルサイズ表示
        M5.Lcd.printf("FileSize:");
        File testfile = SD.open(btnevent_log_filepath);
        SetFromat2(WHITE,20, 105,2);
        M5.Lcd.printf("  ");
        M5.Lcd.print(testfile.size());
        M5.Lcd.printf("byte          ");
      }else{
        SetFromat2(WHITE,20, 105,2); M5.Lcd.printf("*There is no TestFile.");
      }

      SetFromat2(WHITE,10, 175,2); M5.Lcd.printf("Record button presses.");
      SetFromat2(cursor_color[0],10, 200,2); M5.Lcd.printf("Exit\n");
    }else if(sys_step == 621){//
      SetFromat2(WHITE,0, 25,2); M5.Lcd.printf("Top menu>SD>RecordFile>");
      SetFromat2(cursor_color[1],10, 45,2); M5.Lcd.printf("Record ");
      if(record_on == 1){ M5.Lcd.printf("ON     ");}else{ M5.Lcd.printf("OFF    ");}
      SetFromat2(WHITE,10, 85,2); M5.Lcd.printf("Count=%03lu",count_timer_record_60000);

      if (SD.exists(record_filepath)) {
        SetFromat2(WHITE,10, 105,2); M5.Lcd.printf("Existence:YES");
        SetFromat2(WHITE,10, 125,2); M5.Lcd.printf("Size:");
        File testfile = SD.open(record_filepath);M5.Lcd.print(testfile.size());//ファイルサイズ表示
        M5.Lcd.printf("byte");
      }else{
        SetFromat2(WHITE,10, 105,2); M5.Lcd.printf("Existence:NO ");
        SetFromat2(WHITE,10, 125,2); M5.Lcd.printf("Size:-byte     ");
      }
      
      SetFromat2(WHITE,10, 145,2); M5.Lcd.printf("gpio_a35_in=%04lu",gpio_a35_in);

      SetFromat2(cursor_color[0],10, 200,2); M5.Lcd.printf("Exit\n");

    }
    
  }else if(sys_step >= 700 && sys_step < 800){//e-mail
    if(sys_step == 701){//
      SetFromat2(WHITE,0, 25,2); M5.Lcd.printf("Top menu>e-mail");
      SetFromat2(cursor_color[1],10, 45,2);  M5.Lcd.printf("Now send mail\n");
      SetFromat2(WHITE,20, 85,2);  M5.Lcd.printf("Send number=%u",sendmail_number);
      SetFromat2(WHITE,20, 105,2);  M5.Lcd.printf("gpio_a35_in=%u",gpio_a35_in);
      SetFromat2(cursor_color[0],10, 200,2); M5.Lcd.printf("Exit\n");
    }
  }else{
     SetFromat2(WHITE,0, 45,3); M5.Lcd.printf("sys_step error.");
  }
}

//MCP4922 DAC
void DACout(int dataPin,int clockPin,int destination,int value){
  int i ;

  // コマンドデータの出力
  digitalWrite(dataPin,destination) ;// 出力するピン(OUTA/OUTB)を選択する
  digitalWrite(clockPin,HIGH) ;
  digitalWrite(clockPin,LOW) ;
  digitalWrite(dataPin,LOW) ;        // VREFバッファは使用しない
  digitalWrite(clockPin,HIGH) ;
  digitalWrite(clockPin,LOW) ;
  digitalWrite(dataPin,HIGH) ;       // 出力ゲインは１倍とする
  digitalWrite(clockPin,HIGH) ;
  digitalWrite(clockPin,LOW) ;
  digitalWrite(dataPin,HIGH) ;       // アナログ出力は有効とする
  digitalWrite(clockPin,HIGH) ;
  digitalWrite(clockPin,LOW) ;
  // ＤＡＣデータビット出力
  for (i=11 ; i>=0 ; i--) {
    if (((value >> i) & 0x1) == 1) digitalWrite(dataPin,HIGH) ;
    else                           digitalWrite(dataPin,LOW) ;
    digitalWrite(clockPin,HIGH) ;
    digitalWrite(clockPin,LOW) ;
  }
}

//シリアル処理
void serial_Process(){
  
  while(Serial.available()) {
    String checkcommand;
    String getserial = Serial.readStringUntil(0x0a);

    checkcommand = "?";
    if (getserial.length() > 0 && getserial.startsWith(checkcommand)) {
      Serial.printf("template Ver.%s\n",ver);
      Serial.printf("Command list.\n");
      Serial.printf("wifi_ssid,wifi_pass,\n");
    }
    checkcommand = "wifi_ssid";
    if (getserial.length() > 0 && getserial.startsWith(checkcommand)) {
      Serial.print("WIFI SSID:");
      Serial.println(wifi_ssid);
    }
    checkcommand = "wifi_pass";
    if (getserial.length() > 0 && getserial.startsWith(checkcommand)) {
      Serial.print("WIFI Pass:");
      Serial.println(wifi_pass);
    }
    checkcommand = "wifi_ssid,";
    if (getserial.length() > 0 && getserial.startsWith(checkcommand)) {
      wifi_ssid = getserial.substring(checkcommand.length());
      Serial.println(wifi_ssid);
      Serial.println("wifi_ssid set ok.");
    }
    checkcommand = "wifi_pass,";
    if (getserial.length() > 0 && getserial.startsWith(checkcommand)) {
      wifi_ssid = getserial.substring(checkcommand.length());
      Serial.println(wifi_pass);
      Serial.println("wifi_pass set ok.");
    }
  } 
}

void handleResponse(){
  char buf[400];
  //temp = TempMeasurement();
  
  sprintf(buf, 
    "<html>\
     <head>\
        <title>M5Stack Webserver test</title>\
     </head>\
     <body>\
        <p>ok</p>\
        <p>%01d</p>\
        <p>date:%04d/%02d/%02d %02d:%02d:%02d</p>\
     </body>\
     </html>",
    1,
    timeinfo.tm_year+1900,  timeinfo.tm_mon + 1,
    timeinfo.tm_mday,       timeinfo.tm_hour,
    timeinfo.tm_min,        timeinfo.tm_sec
    );

      // M5.Lcd.printf("%04d/%02d/%02d %02d:%02d:%02d",
      //   timeinfo.tm_year+1900,  timeinfo.tm_mon + 1,
      //   timeinfo.tm_mday,       timeinfo.tm_hour,
      //   timeinfo.tm_min,        timeinfo.tm_sec);

  server.send(200, "text/html", buf);
  SetFromat2(BLUE,0, 0,2);  M5.Lcd.printf("accessed on");
}

void handleNotFound(){
  server.send(404, "text/plain", "File Not Found\n\n");
  M5.Lcd.println("File Not Found");
}

void getdirlist_cls(){
  for(int8_t i=0;i<100;i++){
    getdirlist[i] = "";
  }
}

void printDirectory(File dir) {
  int8_t tmp1=0;
  while(true) {
    File entry =  dir.openNextFile();
    if (! entry) {
      dir.rewindDirectory();
      break;
    }
    getdirlist[tmp1] = entry.name();
    
    //M5.Lcd.print(entry.name());
    if (entry.isDirectory()) { 
      getdirlist[tmp1] = "/";
      //M5.Lcd.println("/");//ディレクトリなら最後にスラッシュ追加
    } else {                   //M5.Lcd.println(entry.size(), DEC);ファイルサイズ取得
    }
    tmp1++;
  }
}

//Scan時のwifiのSSIDを8個まで記録
int8_t wifissid_array(String checkssid){
  int8_t have = 0;
  int8_t add_index = 0;

  for(int8_t i=0;i<8;++i){
    if(get_ssid[i].ssid.length()==0){
      add_index = i;
      get_ssid[i].ssid = checkssid;
      break;
    }else{
      if(get_ssid[i].ssid == checkssid){
        have = 1;break;
      }
    }
  }

  //if(have == 0){  }
  //M5.Lcd.println(have);
  return add_index;//have;
} 

void SW_Process(){
  if(M5.BtnA.wasReleased()==1){  btnA_one = 1;}//単押しで離したとき
  btnA_on = M5.BtnA.isPressed();

  if(M5.BtnB.wasReleased()==1){  btnB_one = 1;}//単押しで離したとき
  btnB_on = M5.BtnB.isPressed();

  if(M5.BtnC.wasReleased()==1){  btnC_one = 1;}//単押しで離したとき
  btnC_on = M5.BtnC.isPressed();

  //※長押し処理を入れるとややこしくなるので要注意
  //長押しをやめたときに0を入れてもすぐに1に戻ってしまい、0にするタイミングが難しい。
  // if(M5.BtnA.pressedFor(1000)==1){                        btnA_L_one = 1;//長押し
  // }else if(M5.BtnA.wasReleased()==1 && btnA_L_one == 0){  btnA_one =1;}//単押し
  // btnA_on = M5.BtnA.isPressed();

  // if(M5.BtnB.pressedFor(1000)==1){                        btnB_L_one = 1;//長押し
  // }else if(M5.BtnB.wasReleased()==1 && btnB_L_one == 0){  btnB_one =1;}//単押し
  // btnB_on = M5.BtnB.isPressed();

  // if(M5.BtnC.pressedFor(1000)==1){                        btnC_L_one = 1;//長押し
  // }else if(M5.BtnC.wasReleased()==1 && btnC_L_one == 0){  btnC_one =1;}//単押し
  // btnC_on = M5.BtnC.isPressed();
}

//色と位置とサイズを指定
void SetFromat(int32_t c_r,int32_t c_g,int32_t c_b,int32_t c_x,int32_t c_y,int32_t c_s){
  M5.Lcd.setTextSize(c_s);
  M5.Lcd.setTextColor(M5.Lcd.color565(c_r,c_g,c_b),BLACK);
  M5.Lcd.setCursor(c_x, c_y);
}

void SetFromat2(int32_t c_rgb,int32_t c_x,int32_t c_y,int32_t c_s){
  M5.Lcd.setTextSize(c_s);
  M5.Lcd.setTextColor(c_rgb,BLACK);
  M5.Lcd.setCursor(c_x, c_y);
}

void cursor_control(int8_t cursor_max){
    if(btnB_one!=0){btnB_one = 0;//UP Key
      if(cursor_index == 0){  cursor_index = cursor_max;
      }else{                  cursor_index--;}
    } 
    if(btnC_one!=0){btnC_one = 0;//DOWN Key
      if(cursor_index == cursor_max){  cursor_index = 0;
      }else{                  cursor_index++;}
    }
}

// NTPサーバと時刻を同期
void getTimeFromNTP(){
  
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  while (!getLocalTime(&timeinfo)) {
    delay(1000);
  }
}

//カラー番号取得
uint16_t getColor(uint8_t red, uint8_t green, uint8_t blue){
  //M5.Lcd.color565(c_r,c_g,c_b)と同じ？
  return ((red>>3)<<11) | ((green>>2)<<5) | (blue>>3);
}

/*  memo

lcdリファレンス
https://github.com/m5stack/m5-docs/blob/master/docs/ja/api/lcd.md


M5.Lcd.qrcode("吾輩わがはいは猫である。名前はまだ無い。",40, 0, 240, 15);  

グラフィック描画
tft.drawPixel(4,7,WHITE);//ドット描画
tft.drawLine(4,7,140,170,WHITE);//線描画
tft.drawRect(100,100,10,10,getColor(200,200,0));//四角線の描画
tft.fillRect(120,100,10,20,WHITE);//四角の描画
tft.drawRoundRect(5,2,100,90,10,WHITE);//アール付き四角線の描画
tft.fillRoundRect(5,2,100,90,10,WHITE);//アール付き四角の描画  
tft.drawTriangle(10,100,60,20,110,100,WHITE);//三角線の描画
tft.fillTriangle(10,100,60,20,110,100,WHITE);//三角の描画
tft.drawCircle(120,100,10,WHITE);//円線の描画
tft.fillCircle(120,100,10,WHITE);//円の描画

テキスト描画
tft.setTextColor(WHITE,TFT_BLACK);
float fnumber = 123.45;
tft.println((long)3735928559, HEX); // Should print DEADBEEF
tft.print("Float = ");
tft.println(fnumber);           // Print floating point number
tft.print("Binary = ");
tft.println((int)fnumber, BIN); // Print as integer value in binary
tft.print("Hexadecimal = ");
tft.println((int)fnumber, HEX); // Print as integer number in Hexadecimal

色指定
TFT_WHITE,TFT_BLACK,TFT_BLUE

  x = 0;                          // x座標リセット
  for (int i = 0 ; i < 19; i++) { // 全19色（TFT_付は除く）分表示繰り返し
    switch (i) {
      case 0:   color = BLACK      ;  break;  //   0,   0,   0
      case 1:   color = NAVY       ;  break;  //   0,   0, 128
      case 2:   color = DARKGREEN  ;  break;  //   0, 128,   0
      case 3:   color = DARKCYAN   ;  break;  //   0, 128, 128
      case 4:   color = MAROON     ;  break;  // 128,   0,   0
      case 5:   color = PURPLE     ;  break;  // 128,   0, 128
      case 6:   color = OLIVE      ;  break;  // 128, 128,   0
      case 7:   color = LIGHTGREY  ;  break;  // 192, 192, 192
      case 8:   color = DARKGREY   ;  break;  // 128, 128, 128
      case 9:   color = BLUE       ;  break;  //   0,   0, 255
      case 10:  color = GREEN      ;  break;  //   0, 255,   0
      case 11:  color = CYAN       ;  break;  //   0, 255, 255
      case 12:  color = RED        ;  break;  // 255,   0,   0
      case 13:  color = MAGENTA    ;  break;  // 255,   0, 255
      case 14:  color = YELLOW     ;  break;  // 255, 255,   0
      case 15:  color = WHITE      ;  break;  // 255, 255, 255
      case 16:  color = ORANGE     ;  break;  // 255, 165,   0
      case 17:  color = GREENYELLOW;  break;  // 173, 255,  47
      case 18:  color = PINK       ;  break;  // 255, 0  , 255
    }
    canvas.fillRect(x , 115, 12, 20, color);  // 塗り潰し四角でcolorを表示
    x = x + 12;                               // x座標を+12
  }
*/
