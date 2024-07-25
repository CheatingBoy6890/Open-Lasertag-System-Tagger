#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <painlessMesh.h>

#define LEDPIN D5
#define NUMLEDS 25
#define SendIntervall 200
#define IRreceiver D7
#define RCVintervall 200
#define TargetID 7
#define anmationIntervall 180
#define LEDCOLOR 0xFF00FF

int a1[26] = {0, 0, 9, 10, 19, 20, 1, 8, 11, 18, 21, 2, 7, 12, 17, 22, 3, 6, 13, 16, 23, 4, 5, 14, 15, 24};

Adafruit_NeoPixel strip(NUMLEDS, LEDPIN, NEO_GRB + NEO_KHZ800);
Scheduler runner;
IRrecv receiver(IRreceiver);
decode_results results;
painlessMesh Lasermesh;
JsonDocument document;

const uint32_t MeshReveiver = 360171193u;
void timeAdjust(uint32_t tmp);
void onRCV(uint32_t tmp2, String msg);
void onConnection(uint32_t node);
void onChange();
void Send();
void onHit();
void animation();
Task TaskSend(SendIntervall, TASK_FOREVER, &Send);
Task TaskOnHit(RCVintervall, TASK_FOREVER, &onHit);
Task LedTask(anmationIntervall, TASK_FOREVER, &animation);
void setup()
{

  Serial.begin(115200);
  strip.begin();
  strip.setBrightness(255);
  strip.fill(99999);
  strip.show();
  receiver.enableIRIn();
  runner.addTask(TaskSend);
  runner.addTask(TaskOnHit);
  runner.addTask(LedTask);
  LedTask.enable();
  TaskOnHit.enable();
  Lasermesh.setDebugMsgTypes(ERROR | DEBUG | STARTUP); // set before init() so that you can see error messages
  Lasermesh.onNewConnection(&onConnection);
  Lasermesh.onChangedConnections(&onChange);
  Lasermesh.onReceive(&onRCV);
  Lasermesh.onNodeTimeAdjusted(&timeAdjust);

  Lasermesh.init("Targetmesh", "Targetmesh", &runner, 5555);
  strip.clear();
  strip.show();
}

void loop()
{
  runner.execute();
  Lasermesh.update();
}

void Send()
{

  String output;
  serializeJson(document, output);
  if (Lasermesh.sendSingle(MeshReveiver, output))
    ;
  {

    TaskSend.disable();
  }
}
void onHit()
{
  if (receiver.decode(&results))
  {
    if (results.decode_type == MILESTAG2)
    {
      document["id"] = TargetID;
      document["team"] = (results.command >> 4);
      LedTask.disable();
      strip.fill(0xffffff);
      strip.show();
      LedTask.enableDelayed(7000);
      TaskSend.enable();
      strip.fill();
      strip.show();
    }
  }
  receiver.resume();
}
void animation()
{
  Serial.print("Changing pixel:");
  Serial.println(a1[((LedTask.getRunCounter()) % 27)]);

  strip.setPixelColor(a1[(LedTask.getRunCounter() % 26)], ((0xFFFFFF/12)*(LedTask.getRunCounter()%30))%0xFFFF );
  strip.setBrightness(255);
  strip.show();
  if (!(LedTask.getRunCounter() % 27))
  {

    strip.clear();
    strip.show();
  }
}
void onConnection(uint32_t node)
{
  Serial.print("New Connection from: ");
  Serial.println(node);
}
void onChange()
{
  Serial.println("Connection has changed!");
}
void onRCV(uint32_t tmp2, String msg)
{

  Serial.println("Received something over the Mesh");
}
void timeAdjust(uint32_t tmp)
{
  Serial.println("Adjusting time!");
}