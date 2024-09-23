#include "painlessMesh.h"
#include <Arduino_JSON.h>

#define MESH_PREFIX "new_red123"
#define MESH_PASSWORD "new_red123"
#define MESH_PORT 5555

painlessMesh mesh;
Scheduler userScheduler;
Task taskReadLED(TASK_SECOND * 1, TASK_FOREVER, &readLED);  
Task taskSendMessage(TASK_SECOND * 5, TASK_FOREVER, &sendMessage); 

int ledPin = 5;  
int nodeNumber = 1;
int brightnessSum = 0;
int readCount = 0;
long wakeTime = 0;
int brightness = 0;  
bool increasing = true;  

/*
  Function: readLED
  Description: Reads the brightness level from the LED pin, 
  accumulates the readings, and increments the read count.
  Parameters: None
  Return: None
*/
void readLED() {
  int brightness = analogRead(ledPin);  
  brightnessSum += brightness;  
  readCount++;
  Serial.printf("Brightness read: %d\n", brightness);
}

/*
  Function: getLEDData
  Description: Calculates the average brightness from accumulated readings,
  constructs a JSON object with the average brightness, and resets the accumulators.
  Parameters: None
  Return: A string in JSON format containing the node number and average brightness.
*/
String getLEDData() {
  int avgBrightness = brightnessSum / readCount;  
  JSONVar jsonReadings;
  jsonReadings["node"] = nodeNumber;
  jsonReadings["brightness_avg"] = avgBrightness;
  String readings = JSON.stringify(jsonReadings);
  brightnessSum = 0;
  readCount = 0;
  
  return readings;
}

/*
  Function: sendMessage
  Description: Sends the LED data over the mesh network if the current node time 
  is greater than or equal to the wake time. 
  Parameters: None
  Return: None
*/
void sendMessage() {
  if (mesh.getNodeTime() >= wakeTime) {
    String msg = getLEDData();  
    mesh.sendBroadcast(msg);
    Serial.println("LED message sent: " + msg);
  } else {
    Serial.println("It's not time to wake up yet, waiting for synchronization...");
  }
}

/*
  Function: receivedCallback
  Description: Handles incoming messages from other nodes. If the message contains 
  a temperature reading, it prints a confirmation message. If the message is a sync time, 
  it updates the wake time.
  Parameters: 
    - from: ID of the node that sent the message.
    - msg: Received message.
  Return: None
*/
void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Received from node %u message: %s\n", from, msg.c_str());
  
  JSONVar myObject = JSON.parse(msg.c_str());

  if (myObject.hasOwnProperty("temp")) {
    Serial.println("Temperature reading received.");
  } else {
    long syncTime = atol(msg.c_str());
    if (syncTime > 0) {
      wakeTime = syncTime;
      Serial.printf("Wake time synchronized: %ld\n", wakeTime);
    } else {
      Serial.println("Synchronization failed. Retrying...");
    }
  }
}

/*
  Function: setup
  Description: Initializes the device, sets up the mesh network, configures 
  the LED pin, and enables the tasks for reading and sending messages.
  Parameters: None
  Return: None
*/
void setup() {
  Serial.begin(115200);
  
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  pinMode(ledPin, INPUT);  
  userScheduler.addTask(taskReadLED);
  taskReadLED.enable();
  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();
  pinMode(ledPin, OUTPUT); 
}

/*
  Function: loop
  Description: Main loop that updates the mesh network, 
  adjusts the brightness of the LED, and executes scheduled tasks.
  Parameters: None
  Return: None
*/
void loop() {
  mesh.update(); 
  
  if (increasing) {
    brightness++;
    if (brightness >= 255) increasing = false; 
  } else {
    brightness--;
    if (brightness <= 0) increasing = true;  
  }
  
  analogWrite(ledPin, brightness);  
  delay(15); 
  userScheduler.execute(); 
}

