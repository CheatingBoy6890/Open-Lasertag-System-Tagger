#include <Arduino.h>

#include <U8g2lib.h>

#include "AudioFileSourceSPIFFS.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2SNoDAC.h"

#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <IRsend.h>

// #include <TaskScheduler.h>
#include <painlessMesh.h>

#include <Adafruit_NeoPixel.h>

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

const char *SCORE_MESSAGE = "Never gonna give you up!";

#define TEAM_SelTime 5000         // Time when turning on to select teams
#define Down_Time 5000            // Time it takes to regen when you've been shot
#define VEST_CONNECTTIME 7000     // time to connact a vest
#define SHOOT_DELAY 400           // Firerate
#define TIME_BEFORE_RELOAD 2000   // Time you have to hold the shoot button down before starting to reload
#define SEND_POINT_INTERVAL 10000 // The between sending people who killed you to score a point. Don't set too low, since it's pretty time intensive.

#define MESH_SSID "Lasertag"
#define MESH_PASSWORD "PWA_Lasertag"
#define MESH_PORT 5555

#define MAX_BULLETS 8
// #define SPACE_FOR_BULLETS 120 // the y-space to draw bullets to.
// const uint8_t BULLET_HEIGHT = min(SPACE_FOR_BULLETS / MAX_BULLETS, 8);  // set the bullet size so they fit on screen, max 8 pixels, because bigger looks stupid.

IRrecv receiver(IR_RECEIVER);
decode_results results;

IRsend transmitter(IR_LASER);

AudioFileSourceSPIFFS *file;
AudioGeneratorMP3 *mp3;
AudioOutputI2SNoDAC *out;

Scheduler userScheduler;
painlessMesh Lasermesh;

U8G2_SH1106_128X64_NONAME_F_HW_I2C Display(U8G2_R3);
Adafruit_NeoPixel pixels(LED_NUM, PIXEL_PIN, NEO_GRB + NEO_KHZ800);

void drawammunition(uint8_t y);
void drawbullets(uint8_t first = 0, uint8_t last = MAX_BULLETS);

void ammonition_led();
void updatePixels();

void TaskAmmoCallback();
void TaskRegenerateCallback();

void playAudio(String path);
void loopAudio();

void printTeamInformation();
void teamselect();

void IRRecv();

void onShoot();
void sendMilesTag(uint8_t player, uint8_t team, uint8_t dammage);
void CheckIRresults(decode_type_t decode_type, uint8_t teamId, uint16_t playerIndex);
void IRAM_ATTR onLaserButtonChange();

void MeshReceivedCallback(uint32_t from, String &msg);
void MeshNewConnectionCallback(uint32_t nodeId);
void MeshChangedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);

void send_who_killed_me();

// Task DisplayStuff(TASK_SECOND * 5, TASK_FOREVER, &display);
// Task audioLoop(TASK_MILLISECOND, TASK_FOREVER, &loopAudio);
Task TaskReload(700, MAX_BULLETS, &TaskAmmoCallback);
Task TaskRegenerate(Down_Time, 1, &TaskRegenerateCallback); // Specifying with Down_Time isn't really needed because it schouldn't have an impact. But better to be safe, right?
Task TaskSendScorePoints(SEND_POINT_INTERVAL, TASK_FOREVER, &send_who_killed_me);
Task TaskCheckIR(150, TASK_FOREVER, &IRRecv);

uint8_t bullets = MAX_BULLETS;
uint8_t bullet_offset = 119; // The y-Offset where to draw bullets. // Start value will be overwritten ,just to avoid Null-errors.

uint64_t debounce_shoot;

bool alive = true;
bool isRunning = false;
bool enableAimLaser = true; // Todo: Maybe it's possible to debuff the other team so they cant use their lasers. Just a thought.

void setup()
{
  Serial.begin(115200);
  pinMode(SDA, OUTPUT);
  pinMode(SCL, OUTPUT);
  pinMode(IR_LASER, OUTPUT);
  pinMode(AIM_LASER, OUTPUT);
  pinMode(SHOOT_BUTTON, INPUT_PULLUP); // ESP8266 is buggy, so sometimes the Pin is only pulled up when the pinMode ist INPUT without pullup specified.
  pinMode(AIM_BUTTON, INPUT_PULLUP);   // ESP8266 is buggy, so sometimes the Pin is only pulled up when the pinMode ist INPUT without pullup specified.
  // pinMode(IR_RECEIVER,INPUT);
  pinMode(PIXEL_PIN, OUTPUT);

  receiver.enableIRIn(); // sets the Pin to INPUT automatically
  Display.begin();
  Display.setFont(u8g2_font_tinytim_tf);
  SPIFFS.begin();
  pixels.begin();
  // pixels.SetPixelColor(0,RgbColor(20,100,255));
  // pixels.Show();

  audioLogger = &Serial;

  file = new AudioFileSourceSPIFFS("/reload.mp3");
  mp3 = new AudioGeneratorMP3();
  out = new AudioOutputI2SNoDAC();
  out->SetOutputModeMono(true);
  out->SetGain(1);
  // mp3->begin(file, out);
  // userScheduler.init();

  // digitalWrite(D8,LOW);
  // delay(5000);
  Lasermesh.setDebugMsgTypes(ERROR | DEBUG | STARTUP); // set before init() so that you can see error messages

  Lasermesh.init("Lasermesh", "Password", &userScheduler, 5555);
  Lasermesh.onReceive(&MeshReceivedCallback);
  Lasermesh.onNewConnection(&MeshNewConnectionCallback);
  Lasermesh.onChangedConnections(&MeshChangedConnectionCallback);
  Lasermesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

#ifdef DEBUG_MODE
  Serial.println(Lasermesh.getNodeId());
#endif
  userScheduler.addTask(TaskReload);
  userScheduler.addTask(TaskRegenerate);
  userScheduler.addTask(TaskSendScorePoints);
  userScheduler.addTask(TaskCheckIR);
  TaskCheckIR.enable();
  // TaskSendScorePoints.enable();

  teamselect();
  pinMode(AIM_BUTTON, INPUT_PULLUP); // Set pinmode again because ESP8266audio blocks these for I2s which isn't needed. With redeclaration there's no problem.
  pinMode(AIM_LASER, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(AIM_BUTTON), onLaserButtonChange, CHANGE);
  attachInterrupt(digitalPinToInterrupt(SHOOT_BUTTON), onShoot, CHANGE);
  drawbullets(0, bullets);

  alive = true;
  updatePixels();
}

void loop()
{
  // digitalWrite(AIM_LASER,digitalRead(AIM_BUTTON));
  loopAudio();
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
void drawammunition(uint8_t y)
{
  Display.drawBox(0, y, 15, 7);
  Display.drawFilledEllipse(15, y + 3, 4, 3, U8G2_DRAW_ALL); // y + Floored half of the height of the box
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

  if (bullets >= MAX_BULLETS) // If Ammo is already full or higher due to a bug disable.
  {
    TaskReload.disable();
  }
  else
  {
    if (TaskReload.isFirstIteration())
    {
      drawbullets(0, bullets);
      bullet_offset = 128 - bullets * 9;
    }
    int i = bullet_offset - (TaskReload.getRunCounter() * 9);
    bullets++;
    updatePixels();
    drawammunition(i);
// strip.SetPixelColor(0,RgbColor(255,255,255));
// strip.Show();
#ifdef DEBUG_MODE
    Serial.printf("reloading cartrige: %i\n", i / 9);
#endif
    // if(t1.getRunCounter()==2){digitalWrite(BUILTIN_LED,LOW);}
    playAudio("/reload.mp3");
    Display.sendBuffer();
  }
}

void TaskRegenerateCallback()
{
  alive = true;
}

void playAudio(String path)
{
  if (mp3->isRunning())
  {
    mp3->stop();
    file->close();
  }
  file->open(path.c_str());
  mp3->begin(file, out);
}

void loopAudio()
{
  if (mp3->isRunning())
  {
    if (!mp3->loop())
    {
      mp3->stop();
      file->close();
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

// Needed for painless library
void MeshReceivedCallback(uint32_t from, String &msg)
{
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
  if (msg = SCORE_MESSAGE)
  {
    myPoints++;
    TeamList[myTeamId].score++; // Todo: sync points between players.
  }
}

void MeshNewConnectionCallback(uint32_t nodeId)
{
  Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void MeshChangedConnectionCallback()
{
  Serial.printf("Changed connections\n");
  if (isRunning == false)
  {
    players.clear();
    for (uint32_t node : Lasermesh.getNodeList(true))
    {
      players.push_back(node);
    }
    players.shrink_to_fit();
    std::sort(players.begin(), players.end());
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
  if (alive)
  {

    if (millis() >= debounce_shoot && !TaskReload.isEnabled() && !digitalRead(SHOOT_BUTTON))
    {

      if (bullets > 0)
      {
        bullets--;
        sendMilesTag(std::distance(players.begin(), std::find(players.begin(), players.end(), Lasermesh.getNodeId())), myTeamId, 15);
        playAudio("/shoot.mp3");
        drawbullets(0, bullets);
        updatePixels();
      }
      else
      {
        playAudio("/empty.mp3");
        bullets = 0;
      }

      TaskReload.restartDelayed(TIME_BEFORE_RELOAD);

      debounce_shoot = millis() + SHOOT_DELAY;
    }
    else if (TaskReload.getRunCounter() <= 0) // If the task is enabled, but hasn't run yet --> means: Button has been released to early, disable Reload task.
    {
      TaskReload.disable();
#ifdef DEBUG_MODE
      Serial.println("Disabled Reloading because of Button release!");
#endif
    }
  }
}

void drawbullets(uint8_t first, uint8_t last)
{
  Display.clearBuffer();
  printTeamInformation();
  for (int i = first; i < last; i++)
  {
    drawammunition(119 - i * 9);
  }
  Display.sendBuffer();
}

void sendMilesTag(uint8_t playerIndex, uint8_t team, uint8_t dammage)
{
  uint64_t data = 0;
  data = playerIndex;
  data = data << 2;
  data = data + team;
  data = data << 4;
  data = data + dammage;
  transmitter.sendMilestag2(data);
}

void IRRecv()
{

  // Serial.println("Trying to decode");

  if (receiver.decode(&results))
  {

#ifdef DEBUG_MODE // Print on serial only when DEBUG_MODE is defined at the top.
    Serial.println("#################");
    Serial.print("Decode Type:");
    Serial.println(results.decode_type);

    Serial.print("Team:");
    Serial.println(results.command >> 4);

    Serial.print("Player:");
    Serial.println(results.address);

    Serial.print("Dammage:");
    Serial.println(results.command & 0xF);
#endif

    CheckIRresults(results.decode_type, results.command >> 4, results.address);
    receiver.resume(); // Receive the next value
  }
}

inline void CheckIRresults(decode_type_t decode_type, uint8_t teamId, uint16_t playerIndex)
{
  if (decode_type == MILESTAG2 && teamId != myTeamId)
  {
    alive = false;
    playAudio("/death.mp3");
    TaskRegenerate.restartDelayed(Down_Time);
    you_killed_me.push_back(players[playerIndex]);
    if (!TaskSendScorePoints.enable()) // Check if the task wasn't  aleready running
    {
      TaskSendScorePoints.forceNextIteration();
    }
  }
}

void ammonition_led()
{
  // pixels.SetPixelColor(LED_AMMUNITION, pixels.ColorHSV(bullet_led_color, 255, brightness_per_bullet * bullets));
  uint8_t brightness = (pow(2, (float)bullets / (float)MAX_BULLETS) - 1) * 255;
  pixels.setPixelColor(LED_AMMUNITION, pixels.ColorHSV(bullet_led_color, 255, brightness));
}
void updatePixels()
{
  ammonition_led();

  pixels.show();
}

void send_who_killed_me()
{
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
  Serial.println("starting teamselection.");
#endif

  unsigned long seltime = millis() + TEAM_SelTime;
  pixels.fill(TeamList[myTeamId].color, 0, 0); // Fill the entire strip with the Temcolor[0]
  pixels.show();
  while (seltime > millis())
  {
    if (digitalRead(SHOOT_BUTTON) == LOW)
    {
      myTeamId++;
      myTeamId = myTeamId % (sizeof(TeamList) / sizeof(team));
      pixels.fill(TeamList[myTeamId].color, 0, 0);
      pixels.show();

#ifdef DEBUG_MODE
      Serial.print("Team: ");
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
  Serial.print("finished Teamselection, Team:");
  Serial.println(myTeamId);
#endif
}

void Connect_Vest()
{
  uint64_t connect_time = millis() + VEST_CONNECTTIME;
  while (connect_time < millis())
  {
    if (digitalRead(SHOOT_BUTTON) == LOW)
    {
      sendMilesTag(std::distance(players.begin(), std::find(players.begin(), players.end(), Lasermesh.getNodeId())), myTeamId, 15);
    }
  }
}