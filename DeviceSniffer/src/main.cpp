#include <Arduino.h>
#include "FS.h"


// put function declarations here:
String readFile(fs::FS &fs, const char * path);
bool writeFile(fs::FS &fs, const char * path, const String message);
bool appendFile(fs::FS &fs, const char * path, const String message);
bool deleteFile(fs::FS &fs, const char * path);

void setup() {
  // put your setup code here, to run once:
}

void loop() {
  // put your main code here, to run repeatedly:
}

String readFile(fs::FS &fs, const char * path){
  File file = fs.open(path);
  if(!file || file.isDirectory()){
      Serial.println("- failed to open file for reading");
      return String();
  }
  Serial.println("Reading file");
  String content;
    while(file.available()){
      int c = file.read();
      if (c < 0) break;
      content += static_cast<char>(c);
    }
  file.close();
  return content;
}

bool writeFile(fs::FS &fs, const char * path, const String message){
  File file = fs.open(path, FILE_WRITE);
  if(!file){
      Serial.println("- failed to open file for writing");
      return false;
  }
  if(!file.print(message)){
      Serial.println("- write failed");
  }
  file.close();
  return true;
}

bool appendFile(fs::FS &fs, const char * path, const String message){
  if (message == "")
  {
    return true;
  }
  
  File file = fs.open(path, FILE_APPEND);
  if(!file){
      Serial.println("- failed to open file for appending");
      while (!writeFile(fs, path, message))
      
      return false;
  }
  if(!file.print(message)){
      Serial.println("- append failed");
  }
  file.close();
  return true;
}

bool deleteFile(fs::FS &fs, const char * path){
  if(!fs.remove(path)){
      Serial.println("- delete failed");
      return false;
  }
  return true;
}