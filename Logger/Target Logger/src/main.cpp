#include <TFT_eSPI.h>
#include <SPI.h>
#include <painlessMesh.h>

#define BUTTON_RED 13
#define LED_RED 12

#define BUTTON_BLUE 14
#define LED_BLUE 27

#define BUTTON_YELLOW 26
#define LED_YELLOW 25

#define BUTTON_GREEN 33
#define LED_GREEN 32

#define TFT_ORANGE 0xfa64

const char * SSID = "Targetmesh";
const char * PASSWORD = "Targetmesh";

void IRAM_ATTR redButtonPress(void);
void IRAM_ATTR blueButtonPress(void);
void IRAM_ATTR yellowButtonPress(void);
void IRAM_ATTR  greenButtonPress();

// Prototypes
void sendMessage(); 
void receivedCallback(uint32_t from, String & msg);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback(); 
void nodeTimeAdjustedCallback(int32_t offset); 
void delayReceivedCallback(uint32_t from, int32_t delay);


TFT_eSPI tft = TFT_eSPI();

Scheduler userScheduler;
painlessMesh parcourMesh;


struct target
{
  uint32_t nodeIp;
  int32_t points;
  bool gotHit = false;
  target(uint32_t Ip, int32_t p): nodeIp(Ip), points(p) {}
  /* data */
};



class Timer{
  private:
    bool running = false;
    uint32_t startTime;
    int32_t offset = 0; //this is added to the time
    uint32_t stopTime = 0;
    target targets[7] = {
      {0,5000},
      {0,3500},
      {0,3500},
      {0,4000},
      {0,4000},
      {0,4000},
      {0,4000},
     
    };
  public:
    Timer(){}
    //starts the timer by setting start time to millis() returna true when timer started, false when timer was already running
    bool start(){
      if(!running)
      {
        startTime = millis();
        running = true;
        return true;
      }
      return false;
    }

    int64_t getTime(bool useOffset = true){
      if(running){
      if(useOffset){
        return (int64_t)millis() -startTime + offset;;
      }else {
        return (millis() - startTime);
      }
      } else {
        // return ((lastTime -startTime) + useOffset ? offset : 0);
        if(useOffset){
          return stopTime -startTime + offset;
        } else {
          return stopTime -startTime;
        }
        
      }

    }

    void stop(){
      stopTime = millis();
      running = false;
    }

    void reset(){
      running = false;
      startTime = NULL;
      offset = 0;

      for(int i = 0; i < sizeof(targets)/sizeof(target); i++){
        targets[i].gotHit = false;
      }
    }

    bool isRunning(){
      return running;
    }

    void hitTarget(uint32_t index){
      if(running){
      if(targets[index].gotHit == false){

        offset -= targets[index].points;
        Serial.println("Changed offset!");
        Serial.printf("Offset is: %i", offset);

        targets[index].gotHit = true;
      }
      }
    }

    int32_t getOffset(){
      return offset;
    }


};

Timer redTimer = Timer();
Timer blueTimer = Timer();
Timer yellowTimer = Timer();
Timer greenTimer = Timer();

void setup(){
  Serial.begin(115200);
  Serial.println("setup");
  tft.init();
  tft.setRotation(1);
  tft.setTextFont(4);
  tft.fillScreen(0);
  pinMode(BUTTON_RED,INPUT_PULLUP);
  pinMode(LED_RED,OUTPUT);

  pinMode(BUTTON_BLUE,INPUT_PULLUP);
  pinMode(LED_BLUE,OUTPUT);

  pinMode(BUTTON_YELLOW,INPUT_PULLUP);
  pinMode(LED_YELLOW,OUTPUT);

  pinMode(BUTTON_GREEN,INPUT_PULLUP);
  pinMode(LED_GREEN,OUTPUT);

parcourMesh.setDebugMsgTypes(ERROR | DEBUG);  // set before init() so that you can see error messages

  parcourMesh.init(SSID, PASSWORD, &userScheduler, 5555);
  parcourMesh.onReceive(&receivedCallback);
  parcourMesh.onNewConnection(&newConnectionCallback);
  parcourMesh.onChangedConnections(&changedConnectionCallback);
  parcourMesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  parcourMesh.onNodeDelayReceived(&delayReceivedCallback);

  Serial.printf("Nodeip: %u", parcourMesh.getNodeId());

  attachInterrupt(digitalPinToInterrupt(BUTTON_RED),redButtonPress,FALLING);
   attachInterrupt(digitalPinToInterrupt(BUTTON_BLUE),blueButtonPress,FALLING);
   attachInterrupt(digitalPinToInterrupt(BUTTON_YELLOW),yellowButtonPress,FALLING);
    attachInterrupt(digitalPinToInterrupt(BUTTON_GREEN),greenButtonPress,FALLING);
}

void loop(){
  // digitalWrite(LED_RED,!digitalRead(BUTTON_RED));
  // digitalWrite(LED_BLUE,!digitalRead(BUTTON_BLUE));
  // digitalWrite(LED_YELLOW,!digitalRead(BUTTON_YELLOW));
  // digitalWrite(LED_GREEN,!digitalRead(BUTTON_GREEN));
  // if(!digitalRead(BUTTON_RED) && redTimer.start())
  // {
  //   // redTimer.start();
  //   tft.fillScreen(0);
  // }

  parcourMesh.update();


  tft.setTextColor(TFT_RED,TFT_BLACK);
  tft.setOrigin(0,0);
  tft.drawString("Rot",10,0);
  tft.setCursor(0,0);
  tft.print("\nZeit:");
  tft.println((double)redTimer.getTime(false) / 1000 , 2);
  tft.setTextColor(TFT_ORANGE,TFT_BLACK);
  tft.printf("Bonus: %.2f\n", (float)abs(redTimer.getOffset()) / 1000);
  tft.setTextColor(TFT_WHITE,TFT_BLACK);
  tft.printf("Gesamt: %.2f",(double)redTimer.getTime(true) / 1000);


  tft.setTextColor(TFT_BLUE,TFT_BLACK);
  tft.setOrigin(170,0);

  tft.drawString("Blau",10,0);
  tft.setCursor(0,0);
  tft.print("\n");
  tft.println((double)blueTimer.getTime(false) / 1000 , 2);
  tft.setTextColor(TFT_ORANGE,TFT_BLACK);
  tft.printf(" %.2f\n", (float)abs(blueTimer.getOffset()) / 1000);
  tft.setTextColor(TFT_WHITE,TFT_BLACK);
  tft.printf(" %.2f",(double)blueTimer.getTime(true) / 1000);


    tft.setTextColor(TFT_YELLOW,TFT_BLACK);
  tft.setOrigin(0,110);
  tft.drawString("Gelb",10,0);
  tft.setCursor(0,0);
  tft.print("\nZeit:");
  tft.println((double)yellowTimer.getTime(false) / 1000 , 2);
  tft.setTextColor(TFT_ORANGE,TFT_BLACK);
  tft.printf("Bonus: %.2f\n", (float)abs(yellowTimer.getOffset()) / 1000);
  tft.setTextColor(TFT_WHITE,TFT_BLACK);
  tft.printf("Gesamt: %.2f",(double)yellowTimer.getTime(true) / 1000);


      tft.setTextColor(TFT_GREEN,TFT_BLACK);
  tft.setOrigin(160,110);
  tft.drawString("Gruen",10,0);
  tft.setCursor(0,0);
  tft.print("\n");
  tft.println((double)greenTimer.getTime(false) / 1000 , 2);
  tft.setTextColor(TFT_ORANGE,TFT_BLACK);
  tft.printf("%.2f\n", (float)abs(greenTimer.getOffset()) / 1000);
  tft.setTextColor(TFT_WHITE,TFT_BLACK);
  tft.printf(" %.2f",(double)greenTimer.getTime(true) / 1000);




  if(Serial.available()){
    Serial.println("There's data available, yum!");
      JsonDocument doc;

  DeserializationError error = deserializeJson(doc, Serial);


  if (error) {
  Serial.print("deserializeJson() failed: ");
  Serial.println(error.c_str());
  return;
  }

  if(doc["team"] == "1"){
  redTimer.hitTarget(atoll(doc["id"]));
  }
  if(doc["team"]=="2"){
blueTimer.hitTarget(atoll(doc["id"]));

  }
  }
 
  
  // delay(10);
}

void IRAM_ATTR redButtonPress(void){
  static uint32_t debounce;
  if(debounce < millis()){
    if(redTimer.isRunning()){
      redTimer.stop();
      digitalWrite(LED_RED,LOW);
    } else {
      tft.fillScreen(TFT_BLACK);
      redTimer.reset();
      redTimer.start();
      digitalWrite(LED_RED,HIGH);
    }

    debounce = millis() + 300;
  }

}
void IRAM_ATTR blueButtonPress(void){
  static uint32_t debounce;
  if(debounce < millis()){
    if(blueTimer.isRunning()){
      blueTimer.stop();
      digitalWrite(LED_BLUE,LOW);
    } else {
      tft.fillScreen(TFT_BLACK);
      blueTimer.reset();
      blueTimer.start();
      digitalWrite(LED_BLUE,HIGH);
    }

    debounce = millis() + 300;
  }}

  void IRAM_ATTR yellowButtonPress(void){
  static uint32_t debounce;
  if(debounce < millis()){
    if(yellowTimer.isRunning()){
      yellowTimer.stop();
      digitalWrite(LED_YELLOW,LOW);
    } else {
      tft.fillScreen(TFT_BLACK);
      yellowTimer.reset();
      yellowTimer.start();
      digitalWrite(LED_YELLOW,HIGH);
    }

    debounce = millis() + 300;
  }}


    void IRAM_ATTR greenButtonPress(void){
  static uint32_t debounce;
  if(debounce < millis()){
    if(greenTimer.isRunning()){
      greenTimer.stop();
      digitalWrite(LED_GREEN,LOW);
    } else {
      tft.fillScreen(TFT_BLACK);
      greenTimer.reset();
      greenTimer.start();
      digitalWrite(LED_GREEN,HIGH);
    }

    debounce = millis() + 300;
  }}



void receivedCallback(uint32_t from, String & msg) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());


  JsonDocument doc;

DeserializationError error = deserializeJson(doc, msg);


if (error) {
  Serial.print("deserializeJson() failed: ");
  Serial.println(error.c_str());
  return;
}

int team = doc["team"];

if(team == 0){
  redTimer.hitTarget(doc["id"].as<int>());
}

// const char* id = doc["id"]; // "1"
// const char* team = doc["team"]; // "red"
}

void newConnectionCallback(uint32_t nodeId) {
  // Reset blink task
 
 
  Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
  Serial.printf("--> startHere: New Connection, %s\n", parcourMesh.subConnectionJson(true).c_str());
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
  // Reset blink task
  
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", parcourMesh.getNodeTime(), offset);
}

void delayReceivedCallback(uint32_t from, int32_t delay) {
  Serial.printf("Delay to node %u is %d us\n", from, delay);
}
