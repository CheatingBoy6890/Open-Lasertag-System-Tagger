/*
Copyright (c) 2024, Silas Hille
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.

*/
#include <Arduino.h>

#include <U8g2lib.h>

#include "AudioFileSourceSPIFFS.h"
// #include "AudioGeneratorMP3.h"
// #include "AudioGeneratorFLAC.h"
#include <AudioGeneratorWAV.h>
#include "AudioOutputI2SNoDAC.h"

#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <IRsend.h>

// #include <2g13ucheduler.h>
#include <ArduinoJson.h>
#include <painlessMesh.h>

#include <Adafruit_NeoPixel.h>

#include "weapon.h"
#include "neopixel_config.h"
#include "team_config.h"

#define DEBUG_MODE

const uint8_t SPEAKER_PIN = RX;

const uint8_t PIXEL_PIN = D7;

const uint8_t SHOOT_BUTTON = D3;
const uint8_t AIM_BUTTON = D4;

const uint8_t AIM_LASER = D8;
const uint8_t IR_LASER = D6;

const uint8_t IR_RECEIVER = D5;

const char *SCORE_MESSAGE PROGMEM = "Never gonna give you up!";
const char *VEST_CONNECT_ACKNOWLEDGE_MESSAGE PROGMEM = "bound";

const char *DEATH_SOUND PROGMEM = "/death.wav";
const char *RELOAD_SOUND PROGMEM = "/reload.wav";
const char *EMPTY_SOUND PROGMEM = "/empty.wav";
const char *SHOOT_SOUND PROGMEM = "/shoot.wav";

#define TEAM_SelTime 5000      // Time when turning on to select teams
#define VEST_CONNECTTIME 15000 // time to connect a vest

#define Down_Time 5000 // Time it takes to regen when you've been shot
// #define SHOOT_DELAY 150         // Firerate
// #define TIME_BEFORE_RELOAD 1000 // Time you have to hold the shoot button down before starting to reload
// #define RELOAD_INTERVAL 450
#define SEND_POINT_INTERVAL 5000 // The between sending people who killed you to score a point. Don't set too low, since it's pretty time intensive.

#define IR_CHECK_INTERVAL 100

#define CHECK_BATTERY // Check the voltage of the battery, comment this out if you've got PCB_v2 since it doesn't have the resistors to meassure the voltage.

// #define PISTOL_DAMMAGE 9 // The index of the Dammage array in team_config.h
// #define MAX_BULLETS 14   // With BULLET_WIDTH 10 and space 30 x 90 and Bullet_WIDTH 10 theres Space for 48 bullets
#define BULLET_WIDTH 10
#define SPACE_FOR_BULLETS_Y 90 // the y-space  from the bottom (128)of the Display. to  * bullets to.
#define SPACE_FOR_BULLETS_X 30 // The space for bullets in x direction counted from the left (0) of the Display.
// const uint8_t BULLET_HEIGHT = max(1,((min(SPACE_FOR_BULLETS / MAX_BULLETS, 7) + 1) >> 1 << 1) -1);  // set the bullet size so they fit on screen, max 7 pixels, because bigger looks stupid. Some magic to make it odd so the circle will fit. Minimum is 1 so there's no overflow.
const uint8_t BULLET_HEIGHT = 5; // must be odd
IRrecv receiver(IR_RECEIVER);
decode_results results;

IRsend transmitter(IR_LASER);

AudioFileSourceSPIFFS *file;
// AudioGeneratorMP3a *decoder;
AudioGeneratorWAV *decoder;
AudioOutputI2SNoDAC *out;

Scheduler userScheduler;
painlessMesh Lasermesh;

uint8_t myWeaponIndex = 0;

Adafruit_NeoPixel pixels(LED_NUM, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

void drawammunition(uint8_t x, uint8_t y);
void drawbullets(uint8_t first = 0, uint8_t last = weapons[myWeaponIndex].getMaxBullets());

void ammonition_led();
void updatePixels();

void TaskAmmoCallback();
void TaskRegenerateCallback();
void TaskShootCallback();

void playAudio(const char *path);
void loopAudio();
void mesassure_battery();

void printTeamInformation();
void teamselect();
void weaponSelect();
void draw_connected_icon();
void drawHP(void);
void drawScore(void);

void IRRecv();

void onShoot();
void sendMilesTag(uint8_t player, uint8_t team, uint8_t dammage);
void CheckIRresults(decode_type_t decode_type, uint8_t teamId, uint16_t playerIndex, uint8_t damage);
void IRAM_ATTR onLaserButtonChange();

void MeshReceivedCallback(uint32_t from, String &msg);
void MeshNewConnectionCallback(uint32_t nodeId);
void MeshChangedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);

void send_who_killed_me();
void updateScores();

// Task DisplayStuff(TASK_SECOND * 5, TASK_FOREVER, &display);
Task TaskLoopAudio(TASK_MILLISECOND * 10, TASK_FOREVER, &loopAudio);
Task TaskReload(0, weapons[myWeaponIndex].getMaxBullets(), &TaskAmmoCallback);
Task TaskRegenerate(Down_Time, 1, &TaskRegenerateCallback); // Specifying with Down_Time isn't really needed because it schouldn't have an impact. But better to be safe, right?
Task TaskSendScorePoints(SEND_POINT_INTERVAL, TASK_FOREVER, &send_who_killed_me);
Task TaskSyncPoints(10000, TASK_FOREVER, &updateScores);
Task TaskCheckIR(IR_CHECK_INTERVAL, TASK_FOREVER, &IRRecv);
Task TaskShoot(100, TASK_FOREVER, &TaskShootCallback);

uint8_t bullets = weapons[myWeaponIndex].getMaxBullets();

uint64_t debounce_shoot;

uint32_t boundvest = 0; // set to 0 to enable vest selection before game start.

uint8_t hp = 100;
bool alive = true;
bool isRunning = false;
bool enableAimLaser = true; // Todo: Maybe it's possible to debuff the other team so they cant use their lasers. Just a thought.
volatile bool shootButtonReleased = false;
/*
  ____       _
 / ___|  ___| |_ _   _ _ __
 \___ \ / _ \ __| | | | '_ \
  ___) |  __/ |_| |_| | |_) |
 |____/ \___|\__|\__,_| .__/
                      |_|
*/
void setup()
{
  Serial.begin(115200);
  Serial.println(F("setup"));
  pinMode(SDA, OUTPUT);
  pinMode(SCL, OUTPUT);
  // pinMode(IR_LASER, OUTPUT);
  pinMode(AIM_LASER, OUTPUT);
  pinMode(SHOOT_BUTTON, INPUT_PULLUP); // ESP8266 is buggy, so sometimes the Pin is only pulled up when the pinMode ist INPUT without pullup specified.
  pinMode(AIM_BUTTON, INPUT_PULLUP);   // ESP8266 is buggy, so sometimes the Pin is only pulled up when the pinMode ist INPUT without pullup specified.
  // pinMode(IR_RECEIVER,INPUT);
  pinMode(PIXEL_PIN, OUTPUT);

  //  digitalWrite(IR_LASER,LOW);
  //  delay(10000);

  // digitalWrite(IR_LASER,HIGH);
  // delay(10000);

  Display.begin();
  Display.setFont(u8g2_font_tinytim_tf);
  Display.setDrawColor(1);
  Display.setBitmapMode(1);
  SPIFFS.begin();
  pixels.begin();

  weaponSelect();
  // weapons[myWeaponIndex].DrawIcon(Display,20,20);
  weapons[myWeaponIndex].createShootTask(TaskShoot);
  weapons[myWeaponIndex].drawIcon(0, 0);
  TaskReload.setInterval(weapons[myWeaponIndex].getReloadInterval());
  Display.sendBuffer();
  delay(1000);
  // pixels.SetPixelColor(0,RgbColor(20,100,255));
  // pixels.Show();

  audioLogger = &Serial;

  file = new AudioFileSourceSPIFFS(DEATH_SOUND);
  decoder = new AudioGeneratorWAV();
  decoder->SetBufferSize(500);
  out = new AudioOutputI2SNoDAC();
  out->SetOutputModeMono(true);
  out->SetGain(1);
  // mp3->begin(file, out);
  // userScheduler.init();

  // digitalWrite(D8,LOW);
  // delay(5000);

  Lasermesh.setDebugMsgTypes(ERROR | DEBUG | STARTUP); // set before init() so that you can see error messages

  Lasermesh.init(F("Lasermesh"), F("Password"), &userScheduler, 5555);
  Lasermesh.onReceive(&MeshReceivedCallback);
  Lasermesh.onNewConnection(&MeshNewConnectionCallback);
  Lasermesh.onChangedConnections(&MeshChangedConnectionCallback);
  Lasermesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

#ifdef DEBUG_MODE
  Serial.print(F("NodeId = "));
  Serial.println(Lasermesh.getNodeId());
#endif
  userScheduler.addTask(TaskReload);
  userScheduler.addTask(TaskRegenerate);
  userScheduler.addTask(TaskSendScorePoints);
  userScheduler.addTask(TaskCheckIR);
  userScheduler.addTask(TaskLoopAudio);
  userScheduler.addTask(TaskShoot);
  userScheduler.addTask(TaskSyncPoints);
  // TaskSendScorePoints.enable();

  teamselect();

  // Remind players how to connect a vest
  Display.clearBuffer();
  Display.setCursor(0, 40);
  Display.print(F("Just shoot at\na vest to\nconnect to it"));
  Display.sendBuffer();
  delay(1000);




  pinMode(AIM_BUTTON, INPUT_PULLUP); // Set pinmode again because ESP8266audio blocks these for I2s which isn't needed. With redeclaration there's no problem.
  pinMode(AIM_LASER, OUTPUT);
  // pinMode(IR_LASER,OUTPUT);

  // for(int i = 0; i < 20; i++)
  // {
  //   digitalWrite(IR_LASER,HIGH);
  //   delay(1000);
  //   digitalWrite(IR_LASER,LOW);
  //   delay(1000);
  // }
  // transmitter.enableIROut(36, 50);
  receiver.enableIRIn(); // sets the Pin to INPUT automatically
  transmitter.begin();

#ifdef DEBUG_MODE
  Serial.println(F("Now connecting vest"));
#endif

#ifdef DEBUG_MODE
  Serial.println(F("finished Vest connection"));
#endif

  attachInterrupt(digitalPinToInterrupt(AIM_BUTTON), onLaserButtonChange, CHANGE);

  attachInterrupt(digitalPinToInterrupt(SHOOT_BUTTON), onShoot, FALLING);

  drawbullets(0, bullets);
  // drawHP();
  // drawScore();
  // Display.sendBuffer();

  alive = true;
  TaskRegenerate.enable();
  TaskCheckIR.enable();
  updatePixels();
  TaskLoopAudio.enable();
  TaskSyncPoints.enable();
  playAudio(DEATH_SOUND);
}

/*
  _
 | |    ___   ___  _ __
 | |   / _ \ / _ \| '_ \
 | |__| (_) | (_) | |_) |
 |_____\___/ \___/| .__/
                  |_|
*/

void loop()
{
  // digitalWrite(AIM_LASER,digitalRead(AIM_BUTTON));
  // loopAudio();
  // Serial.printf("D4 is %i\n", digitalRead(D4));

  // userScheduler.execute();

  Lasermesh.update();

  // delay(1000);
  // if (millis() >= nextTime)
  // {
  //   // int j = 119 - bullet_number * 9;
  //   // bullet_number++;
  //   // if (bullet_number > 8)
  //   // {
  //   //   bullet_number = 1;
  //   //   Display.clearBuffer();
  //   // }
  //   // drawammunition(j);
  //   // Display.sendBuffer();
  //   nextTime = millis() + 800;
  //   playAudio("/reload.mp3");
  // }
}

// put function definitions here:
void drawammunition(uint8_t x, uint8_t y)
{
  Display.drawBox(x, y, BULLET_WIDTH, BULLET_HEIGHT);
  // Display.drawFilledEllipse(BULLET_WIDTH /* +x */, y + ((int)BULLET_HEIGHT / 2), 4, ((int)BULLET_HEIGHT / 2), U8G2_DRAW_ALL); // y + Floored half of the height of the box
  Display.drawDisc(BULLET_WIDTH + x, y + BULLET_HEIGHT / 2, BULLET_HEIGHT / 2, U8G2_DRAW_LOWER_RIGHT | U8G2_DRAW_UPPER_RIGHT);
}

// Actually draws everything like team info and hp
void drawbullets(uint8_t first, uint8_t last)
{
  Display.clearBuffer();
  printTeamInformation();

  drawScore();

  if (boundvest != 0 && Lasermesh.isConnected(boundvest))
  {
    draw_connected_icon();
  }
  // uint16_t bullets_left = last;
  // for (int i = first; i < last; i++, bullets_left--)
  // {
  //   uint8_t y = SPACE_FOR_BULLETS - i * (BULLET_HEIGHT  +1);
  //   drawammunition(0,y); // leave blank space between bullets.
  //   if(y <= Display.getHeight() - SPACE_FOR_BULLETS){
  //     break;
  //   }
  // }
  uint8_t x = 0;
  uint8_t y = Display.getHeight() - BULLET_HEIGHT;

  for (uint8_t i = first; i < last; i++)
  {
    drawammunition(x, y);
    if (y >= (Display.getHeight() - SPACE_FOR_BULLETS_Y))
    {
      y = y - (BULLET_HEIGHT + 1);
    }
    else
    {
      x = x + (BULLET_WIDTH + 4);              // leave some space between the collumns
      y = Display.getHeight() - BULLET_HEIGHT; // Reset y to bottom of the screen - Height of bullets.
    }

    if (x > SPACE_FOR_BULLETS_X)
    {
      break;
    }
    // y = (y >= (128 - SPACE_FOR_BULLETS)) ? y - (BULLET_HEIGHT + 1) : (x += BULLET_WIDTH + 4, 128 - BULLET_HEIGHT);
  }

  drawHP();

#ifdef CHECK_BATTERY
  mesassure_battery();
#endif
  Display.sendBuffer();
}
// void fillAmmo(uint8_t amount)
// {
//   // for (uint8_t i = 119; i > 119 - amount * 9; i -= 9)
//   // {
//     drawammunition(i);
//     Serial.printf("reloading cartrige: %i\n", i / 9);
//     // playAudio("/reload.mp3");
//     Display.sendBuffer();
//     delay(700);
//   // }
// }

void TaskAmmoCallback()
{
#ifdef DEBUG_MODE
  Serial.println(F("TaskAmmo"));
#endif
  if (TaskReload.isFirstIteration() && digitalRead(SHOOT_BUTTON)) // If the Button is released when reloading starts disable the task
  {
    // TaskReload.restartDelayed(10000); // reset the runcounter to 0 the delay is completely unimportant.
    TaskReload.enable(); // Reset runcounter when Task ends.
    TaskReload.disable();
    return;
  }
  TaskShoot.setIterations(weapons[myWeaponIndex].getIterations());
  if (bullets >= weapons[myWeaponIndex].getMaxBullets()) // If Ammo is already full or higher due to a bug disable.
  {
    TaskReload.enable(); // Reset runcounter when Task ends.
    TaskReload.disable();
  }
  else
  {
    bullets++;
    drawbullets(0, bullets);
    Display.sendBuffer();
    updatePixels();
    // strip.SetPixelColor(0,RgbColor(255,255,255));
    // strip.Show();
    // if(t1.getRunCounter()==2){digitalWrite(BUILTIN_LED,LOW);}
    playAudio(RELOAD_SOUND);
  }

  // if(TaskReload.isLastIteration())
  // {TaskReload.restartDelayed(1000);
  //   TaskReload.disable();
  // }
  if (TaskReload.isLastIteration())
  {
    TaskReload.enable(); // Reset runcounter when Task ends.
  }
}

void TaskRegenerateCallback()
{
#ifdef DEBUG_MODE
  Serial.println(F("TaskRegenerate"));
#endif
  hp = 100;
  alive = true;
  bullets = weapons[myWeaponIndex].getMaxBullets();
  drawbullets(0, bullets);
  Display.sendBuffer();
  if (boundvest != 0)
  {
    Lasermesh.sendSingle(boundvest, "alive");
  }
}

void playAudio(const char *path)
{
  if (decoder->isRunning())
  {
    decoder->stop();
    // delete mp3;
    // out->flush();
    // file->close();

    // out->stop();

    // delete file;

    // delete out;

    // file = new AudioFileSourceSPIFFS(path.c_str());
    // mp3 = new AudioGeneratorMP3();
    // out = new AudioOutputI2SNoDAC();
    // out->SetOutputModeMono(true);
    // out->SetGain(1);
  }
  file->open(path);
  // out->begin();
  decoder->begin(file, out);
}

void loopAudio()
{
#ifdef DEBUG_MODE
  if (TaskLoopAudio.getRunCounter() % 100 == 1)
  {
    Serial.print(F("Free heap: "));
    Serial.println(ESP.getFreeHeap());
  }
#endif

  if (decoder->isRunning())
  {
    if (!decoder->loop())
    {
      decoder->stop();
      // file->close();
    }
  }
}

void printTeamInformation()
{
  // Display.clearBuffer();
  // Display.drawStr(0, 5, "Rot: 12");
  // Display.drawStr(0, 11, "Gr�n: 19");
  // Display.drawStr(0, 17, "Blau: 17");
  // Display.drawStr(0, 23, "Gelb: 9");
  Display.setCursor(0, 7);
  for (auto i : TeamList)
  { // iterate trough all teams
    Display.print((i.name + ": " + i.score).c_str());
    Display.setCursor(0, Display.getCursorY() + 7);
  }
  // for(int i = 128;i>23;i--){
  //   Display.drawBox(50,i,10,128);
  //   delay(10);
  //   Display.sendBuffer();
  // }
  // drawammunition(120);
  // fillAmmo(10);
}

void draw_connected_icon()
{

  Display.drawXBMP(Display.getWidth() - 15, 0, Cast_icon_width, Cast_icon_height, Cast_icon_bits);
}

// Needed for painless library
void MeshReceivedCallback(uint32_t from, String &msg)
{
  // Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
  // Serial.println(msg);
  Serial.print(F("--> New message from: "));
  Serial.println(from);
  Serial.print(F("Message: "));
  Serial.println(msg);

  if (msg == SCORE_MESSAGE)
  {
#ifdef DEBUG_MODE
    Serial.println(F("Got a point"));
#endif
    myPoints++;
    TeamList[myTeamId].score++; // Todo: sync points between players.
    drawbullets(0, bullets);
  }
  else if (msg.startsWith("IR-Data:"))
  {
    uint32_t value = strtoul(msg.substring(msg.indexOf(":") + 1).c_str(), nullptr, 10);
#ifdef DEBUG_MODE
    Serial.println(value);
#endif
    CheckIRresults(MILESTAG2, (value & 0B110000) >> 4, value >> 6, value & 0b001111);
  }
  else if (msg == VEST_CONNECT_ACKNOWLEDGE_MESSAGE)
  {
#ifdef DEBUG_MODE
    Serial.println(F("Connected Vest!"));
#endif
    boundvest = from;
  }
  else if (msg.startsWith("Sync\n"))
  {
    JsonDocument doc;
    Serial.print(F("Deserialing Json:\n"));
    Serial.println(msg.substring(msg.indexOf('{')));
    deserializeJson(doc, msg.substring(msg.indexOf('{')));
    doc[TeamList[myTeamId].name] = doc[TeamList[myTeamId].name] + myPoints;
    // If your the last player you don't relay the message but broadcast it.
    if (players.back() == Lasermesh.getNodeId())
    {
      String json;
      serializeJson(doc, json);
      String msg = "Points\n" + json;
      Lasermesh.sendBroadcast(msg, true);
    }
    else
    {
      String json;
      serializeJson(doc, json);
      String msg = "Sync\n" + json;
      Lasermesh.sendSingle(players[myPlayerId + 1], msg);
    }
  }

  if (msg.startsWith("Points\n"))
  {
    JsonDocument doc;
    deserializeJson(doc, msg.substring(msg.indexOf('{')));
    // You have to use references in order to change values
    for (team &t : TeamList)
    {
      t.score = doc[t.name];
    }
    drawbullets(0, bullets);
  }
}

void MeshNewConnectionCallback(uint32_t nodeId)
{
  // Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
  Serial.print(F("--> New Connection from: "));
  Serial.println(nodeId);
}

void MeshChangedConnectionCallback()
{
  Serial.println(F("Changed connections"));
  if (isRunning == false)
  {
    players.clear();
    for (const uint32_t &node : Lasermesh.getNodeList(true))
    {
      players.push_back(node);
    }
    players.shrink_to_fit();
    std::sort(players.begin(), players.end());
    Serial.printf("Player id is: %u \n", std::distance(players.begin(), std::find(players.begin(), players.end(), Lasermesh.getNodeId())));
    myPlayerId = std::distance(players.begin(), std::find(players.begin(), players.end(), Lasermesh.getNodeId()));
  }
}

void nodeTimeAdjustedCallback(int32_t offset)
{
  Serial.printf("Adjusted time %u. Offset = %d\n", Lasermesh.getNodeTime(), offset);
}

void IRAM_ATTR onLaserButtonChange()
{
  if (enableAimLaser)
  {
    digitalWrite(AIM_LASER, digitalRead(AIM_BUTTON)); // Turn on the Aimlaser when the Button is released. HIGH when button is open because of pullup.
  }
}

void IRAM_ATTR onShoot() // Send a Milestag2 Package, Play sounds and start the ReloadTask for later. Reload task is disabled when the Button is releasded before TIME_BEFORE_RELOAD
{
  // Serial.println("Interrupt!");
  // noInterrupts();
  if (alive && !TaskShoot.isEnabled())
  {
    // if (!digitalRead(SHOOT_BUTTON))
    // {
    TaskShoot.restartDelayed(weapons[myWeaponIndex].getTimeBeforeShot());
    // }
    // else
    // {
    //   if (TaskReload.getRunCounter() <= 0) // If the task is enabled, but hasn't run yet --> means: Button has been released to early, disable Reload task.
    //   {
    //     TaskReload.disable();

    //     // Serial.println("Disabled Reloading because of Button release!");
    //   }
    //   shootButtonReleased = true;
    //   // Serial.println("Shoot button released");
    //   if (TaskShoot.getIterations() <= 0)
    //   {
    //     TaskShoot.disable();
    //   }
    // }
  }
  // interrupts();
}

void sendMilesTag(uint8_t playerIndex, uint8_t team, uint8_t dammage)
{
  uint32_t data = 0;
  data = playerIndex;
  data = data << 2;
  data = data + team;
  data = data << 4;
  data = data + dammage;
  // transmitter.sendMilestag2(data);
  transmitter.sendMilestag2(data, 14, 0);
}

void IRRecv()
{

  // Serial.println("Trying to decode");

  if (receiver.decode(&results))
  {

#ifdef DEBUG_MODE // Print on serial only when DEBUG_MODE is defined at the top.
    Serial.println(F("#################"));
    Serial.print(F("Decode Type:"));
    Serial.println(results.decode_type);

    Serial.print(F("Team:"));
    Serial.println(results.command >> 4);

    Serial.print(F("Player:"));
    Serial.println(results.address);

    Serial.print(F("Dammage:"));
    Serial.println(results.command & 0xF);
#endif

    CheckIRresults(results.decode_type, results.command >> 4, results.address, results.command & 0xF);
    receiver.resume(); // Receive the next value
  }
}

inline void CheckIRresults(decode_type_t decode_type, uint8_t teamId, uint16_t playerIndex, uint8_t damage)
{

  // Serial.println("\nDecode Type:" + String(decode_type));

  //     Serial.print("Team:");
  //   Serial.println(teamId);

  //   Serial.print("Player:");
  //   Serial.println(playerIndex);

  if (decode_type == MILESTAG2 && teamId != myTeamId && alive)
  {
    Serial.print(F("Hp: "));
    hp = max(hp - Damage[damage], 0);
    Serial.println(hp);

    if (hp <= 0)
    {
      playAudio(DEATH_SOUND);
      alive = false;
      if (boundvest != 0)
      {
        Lasermesh.sendSingle(boundvest, F("dead"));
      }
      TaskRegenerate.restartDelayed(Down_Time);
      you_killed_me.push_back(players[playerIndex]);
      if (!TaskSendScorePoints.enable()) // Check if the task wasn't  aleready running
      {
        TaskSendScorePoints.forceNextIteration();
      }
    }
    drawbullets(0, bullets);
  }
}

void ammonition_led()
{
  // pixels.SetPixelColor(LED_AMMUNITION, pixels.ColorHSV(bullet_led_color, 255, brightness_per_bullet * bullets));
  uint8_t brightness = (pow(2, (float)bullets / (float)weapons[myWeaponIndex].getMaxBullets()) - 1) * 255;
  pixels.setPixelColor(LED_AMMUNITION, pixels.ColorHSV(bullet_led_color, 255, brightness));
}
void updatePixels()
{
  ammonition_led();

  pixels.setPixelColor(LED_VEST_CONNECTED, boundvest != 0 ? 0xFFFFFF : 0);

  pixels.show();
}

void send_who_killed_me()
{

  Serial.println(F("Now sending who killed me"));
  you_killed_me.shrink_to_fit();
  auto it = you_killed_me.begin();
  while (it != you_killed_me.end())
  {
    uint32_t killer = *it;
    if (Lasermesh.isConnected(killer) && Lasermesh.sendSingle(killer, SCORE_MESSAGE))
    {

      it = you_killed_me.erase(it); // Increments the iterator to next element automatically.
    }
    else
    {
      it++; // If there's an error go to the next element
    }
  }
  if (you_killed_me.empty())
  {
    TaskSendScorePoints.disable();
  }
}

void teamselect()
{
#ifdef DEBUG_MODE
  Serial.println(F("starting teamselection."));
#endif

  pixels.fill(TeamList[myTeamId].color, 0, 0); // Fill the entire strip with the Temcolor[0]
  pixels.show();

  Display.clearBuffer();
  Display.setCursor(0, 40);
  Display.print(F("Select your\nteam"));
  Display.sendBuffer();
  // Aim Button not pressed
  while (digitalRead(AIM_BUTTON))
  {
    if (digitalRead(SHOOT_BUTTON) == LOW)
    {
      myTeamId++;
      myTeamId = myTeamId % (sizeof(TeamList) / sizeof(team));
      pixels.fill(TeamList[myTeamId].color, 0, 0);
      pixels.show();

#ifdef DEBUG_MODE
      Serial.print(F("Team: "));
      Serial.println(myTeamId);
#endif
    }
    delay(200);
  }
  // alive = true;
  pixels.fill(); // turn off every pixel
  pixels.setPixelColor(LED_TEAM, TeamList[myTeamId].color);
  pixels.show();

#ifdef DEBUG_MODE
  Serial.print(F("finished Teamselection, Team:"));
  Serial.println(myTeamId);
#endif
}

void drawHP(void)
{
  Display.drawBox(48, Display.getHeight() - hp, 8, hp);
  // Display.sendBuffer();
}

void drawScore(void)
{
  Display.setCursor(30, 8);
  Display.printf("You: %i", myPoints);
  // Display.sendBuffer();
}

void TaskShootCallback()
{
  noInterrupts();
  if (millis() >= debounce_shoot && TaskReload.getRunCounter() < 1)
  {

    if (bullets > 0)
    {
      Serial.println(F("shooting"));
      sendMilesTag(myPlayerId, myTeamId, weapons[myWeaponIndex].getDammage());
      bullets--;
      pixels.setPixelColor(LED_SHOOTING, 0xFFFFFF);
      pixels.show();
      playAudio(SHOOT_SOUND);

      drawbullets(0, bullets);

      // You can't use Neopixels in interrupts!

      pixels.setPixelColor(LED_SHOOTING, 0);
      updatePixels();
    }
    else
    {
      playAudio(EMPTY_SOUND);
      // bullets = 0;
    }
    if (TaskShoot.getIterations() == 0 || bullets == 0) // starting the reloading when it's the last iteration of the task (for burst weapons) or the magazine is empty (for full auto)
    {

      Serial.println(F("starting Reload"));

      TaskReload.restartDelayed(weapons[myWeaponIndex].getTimeBeforeReload());
      if (TaskShoot.getIterations() == -1)
      {
        TaskShoot.setIterations(1);
      }
    }
    if (TaskShoot.isLastIteration())
    {
      debounce_shoot = millis() + weapons[myWeaponIndex].getTimebetwennshots();
    }
    if (TaskShoot.getIterations() == -1 && digitalRead(SHOOT_BUTTON))
    {
      TaskShoot.disable();
    }

    // shootButtonReleased = false;
  }
  interrupts();
}

void weaponSelect()
{

  uint8_t y = 0;
  for (auto gun : weapons)
  {
    gun.drawIcon(0, y);
    y += gun.getIconHeight();
  }
  Display.sendBuffer();
  while (digitalRead(AIM_BUTTON))
  {
    if (!digitalRead(SHOOT_BUTTON))
    {
      myWeaponIndex = (myWeaponIndex + 1) % (sizeof(weapons) / sizeof(weapon));

      y = 0;
      Display.clearBuffer();
      for (uint8_t j = 0; j < sizeof(weapons) / sizeof(weapon); j++)
      {
        if (j == myWeaponIndex)
        {
          Display.setDrawColor(0); // Inverses the color so you see which weapon is currently selected.
          Display.setBitmapMode(0);
        }
        else
        {
          Display.setDrawColor(1);
          Display.setBitmapMode(1);
        }
        weapons[j].drawIcon(0, y);
        y += weapons[j].getIconHeight() + 4;
        Display.sendBuffer();
      }
    }
    delay(100);
  }
  Display.setDrawColor(1);
  Display.setBitmapMode(1);
  Display.clear();

  TaskReload.setIterations(weapons[myWeaponIndex].getMaxBullets());
}

void updateScores()
{
  JsonDocument doc;
  if (players.size() > 1 && players[0] == Lasermesh.getNodeId()) // If your the first one in the player list.
  {
    Serial.println(F("Sending Syncing json!"));
    for (team t : TeamList)
    {
      doc[t.name] = 0;
    }

    doc[TeamList[myTeamId].name] = myPoints;
    String json;
    serializeJson(doc, json);
    String msg = "Sync\n" + json;
    // Send the message to the second player
    Lasermesh.sendSingle(players[1], msg);
  }
}

void mesassure_battery()
{
  float voltage = analogRead(A0) * 0.005394;
  Display.setCursor(40, 18);
  Display.printf("V=%.1f", voltage);
  if (voltage < (float)4)
  {
    Display.clearBuffer();
    Display.setCursor(0, 20);
    Display.print(F("Low \nBattery!"));
    playAudio(DEATH_SOUND);
    // delay(1000); // You actually should never use delay in a Task but I was to lazy to search a good way to pause anything printing to the display for one second.
  }
}
