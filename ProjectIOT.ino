#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
#include <WiFiClientSecure.h>
#include <LiquidCrystal_I2C.h>
#include <ESP8266Firebase.h>
#include "HTTPSRedirect.h"

const char* ssid = "duynhat";
const char* password = "1234567890";
const char* host = "script.google.com";//url connect to Google Apps Script
const int httpsPort = 443;//Port
String GAS_ID = "AKfycbwZ53oZGJGLHyxY9ndAANT-UjdCp8E-KNcjYMHuEpWkLSr_liz3aD5l5SfhzCLvyrBe5A";//Key Google Apps Script
// Enter command (insert_row or append_row) and your Google Sheets sheet name (default is Sheet1):
String payload_base =  "{\"command\": \"insert_row\", \"sheet_name\": \"Sheet1\", \"values\": ";
String payload = "";
// Google Sheets setup (do not edit)
String url = String("/macros/s/") + GAS_ID + "/exec";
HTTPSRedirect* client = nullptr;
#define REFERENCE_URL "iotproject-470fe-default-rtdb.asia-southeast1.firebasedatabase.app"  // Your Firebase project reference url
#define RX_PIN D4  // Chân RX của cổng nối tiếp mềm
#define TX_PIN D3  // Chân TX của cổng nối tiếp mềm
Firebase firebase(REFERENCE_URL);
SoftwareSerial mySerial(RX_PIN, TX_PIN);
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
ESP8266WebServer server(80);
LiquidCrystal_I2C lcd(0x27, 16, 2);
int status = 0;
int FingerID = 0;
bool checkAtten = false;
char webpage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="en">

<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Attendance Check </title>
    <style>
        /* Reset các style mặc định */
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        /* Common form styling */
        .form-style-5 {
            max-width: 700px;
            margin: 50px auto;
            padding: 20px;
            border: 1px solid #ccc;
            border-radius: 10px;
            background: #f9f9f9;
            font-family: Arial, sans-serif;
        }

        /* Form-specific styling */
        .form-style-5 legend {
            font-size: 20px;
            font-weight: bold;
            color: #333;
        }

        .form-style-5 fieldset {
            border: none;
            margin-bottom: 20px;
        }

        .form-style-5 label {
            display: block;
            font-weight: bold;
            margin-bottom: 5px;
        }

        .form-style-5 input[type="text"],
        .form-style-5 input[type="email"],
        .form-style-5 input[type="number"] {
            width: 100%;
            padding: 8px;
            margin-bottom: 10px;
            border: 1px solid #ccc;
            border-radius: 5px;
        }

        .form-style-5 button {
            width: 100%;
            padding: 10px;
            border: none;
            border-radius: 5px;
            background-color: #4c90af;
            color: white;
            cursor: pointer;
            transition: background-color 0.3s;
        }

        .form-style-5 button:hover {
            background-color: #45a049;
        }

        .slider-container {
            display: flex;
            align-items: center;
            margin-top: 20px;
            margin-bottom: 20px;
        }

        .switch {
            position: relative;
            display: inline-block;
            width: 60px;
            height: 34px;
        }

        .switch input {
            opacity: 0;
            width: 0;
            height: 0;
        }

        .slider {
            position: absolute;
            cursor: pointer;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background-color: #ccc;
            transition: .4s;
            border-radius: 34px;
        }

        .slider:before {
            position: absolute;
            content: "";
            height: 26px;
            width: 26px;
            left: 4px;
            bottom: 4px;
            background-color: white;
            transition: .4s;
            border-radius: 50%;
        }

        input:checked+.slider {
            background-color: #2196F3;
        }

        input:focus+.slider {
            box-shadow: 0 0 1px #2196F3;
        }

        input:checked+.slider:before {
            transform: translateX(26px);
        }

        /* Styles for the attendance label */
        .slider-label {
            margin-left: 20px;
            font-size: 30px;
        }
        h1 {
            margin-top: 10px;
            text-align: center;
            color: #008000;
        }
    </style>
</head>

<body>
    <h1>
        Attendance Check 
    </h1>
    <div class="form-style-5 slideInDown animated">
        <div class="alert">
            <label id="alert"></label>
        </div>
        <form onsubmit="enrollFingerprint(); return false;">
            <fieldset>
                <legend><span class="number">1</span> User Fingerprint ID:</legend>
                <label>Enter Fingerprint ID between 1 & 127:</label>
                <input type="number" name="fingerid" id="fingerid" placeholder="User Fingerprint ID...">
            </fieldset>
            <fieldset>
                <legend><span class="number">2</span> User Info</legend>
                <input type="text" name="name" id="name" placeholder="User Name...">
                <input type="text" name="number" id="number" placeholder="Serial Number...">
                <input type="email" name="email" id="email" placeholder="User Email...">
            </fieldset>
            <button type="submit" class="user_add">Add User</button>
        </form>
        <div class="slider-container">
            <label class="switch">
                <input type="checkbox" id="attendanceToggle" onchange="checkAttendance()">
                <span class="slider round"></span>
            </label>
            <span class="slider-label">Attendance</span>
        </div>

        <form onsubmit="deleteFingerprint(); return false;">
            <fieldset>
                <legend><span class="number">3</span> ID fingerprint Delete:</legend>
                <input type="text" name="txtInput" id="iddelete" value="" />
                <button type="submit" class="user_add">DELETE</button>
        </form>
        </fieldset>

        <script>
            var baseUrl = '/';  // Đường dẫn cơ bản, không cần địa chỉ IP
            function deleteFingerprint() {
                var fingerid = document.getElementById('iddelete').value;
                if (!fingerid) {
                    console.log("Please enter a fingerprint ID.");
                    return;
                }
                var xhr = new XMLHttpRequest();
                xhr.onreadystatechange = function () {
                    if (xhr.readyState == 4 && xhr.status == 200) {
                        var response = xhr.responseText;
                        console.log(response);
                    }
                };
                var deleteUrl = baseUrl + 'deleteFingerprint?fingerid=' + fingerid;
                xhr.open('GET', deleteUrl, true);
                xhr.send();
            }

            function enrollFingerprint() {
                var fingerid = document.getElementById('fingerid').value;
                var name = document.getElementById('name').value;
                var number = document.getElementById('number').value;
                var email = document.getElementById('email').value;
                var xhr = new XMLHttpRequest();
                xhr.onreadystatechange = function () {
                    if (xhr.readyState == 4 && xhr.status == 200) {
                        var response = xhr.responseText;
                        console.log(response);
                    }
                };
                var url = baseUrl + 'enrollFingerprint?fingerid=' + fingerid + '&name=' + name + '&number=' + number + '&email=' + email;
                xhr.open('GET', url, true);
                xhr.send();
            }

            function checkAttendance() {
                var checkbox = document.getElementById("attendanceToggle");
                var xhr = new XMLHttpRequest();
                xhr.onreadystatechange = function () {
                    if (xhr.readyState == 4) {
                        if (xhr.status == 200) {
                            var response = xhr.responseText;
                            console.log(response);
                        } else {
                            console.log("Error: " + xhr.status);
                        }
                    }
                };
                var checkAttendanceUrl = '';
                if (checkbox.checked) {
                    checkAttendanceUrl = baseUrl + 'checkAttendance?action=attendance';
                }
                else {
                    checkAttendanceUrl = baseUrl + 'checkAttendance?action=endattendance';
                }
                xhr.open('GET', checkAttendanceUrl, true);
                xhr.send();
            }
        </script>
    </div>
</body>

</html>
)=====";

void handleRoot() {
  server.send(200, "text/html", webpage);
}

void setup() {
  lcd.init();                      // initialize the lcd
  lcd.init();
  lcd.backlight();
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  lcd.clear();
  lcd.print("Connecting to ");
  lcd.setCursor(1, 0);
  lcd.print(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.print("Successfully connected to : ");
  Serial.println(ssid);
  lcd.clear();
  lcd.print("Connected, IP: ");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  mySerial.begin(57600);
  Serial.println("Waiting for the fingerprint sensor...");
  lcd.clear();
  lcd.print("Waiting ");
  lcd.setCursor(0, 1);
  lcd.print("finger sensor...");
  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
    lcd.clear();
    lcd.print("Found finger");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    lcd.clear();
    lcd.print("Not found finger");
    while (1);
  }
  delay(1000);
  server.on("/", HTTP_GET, handleRoot);
  server.on("/enrollFingerprint", HTTP_GET, handleEnrollFingerprint);
  server.on("/deleteFingerprint", HTTP_GET, handleDeleteFingerprint);
  server.on("/checkAttendance", HTTP_GET, handleCheckAttendance);
  server.begin();
  lcd.clear();
  lcd.print("Server on");
  lcd.setCursor(0, 1);
  lcd.print("IP:");
  lcd.setCursor(4,1);
  lcd.print(WiFi.localIP());
}

void loop() {
  server.handleClient();
  
}
void handleDeleteFingerprint() {
  String fingerid = server.arg("fingerid");
  int id = fingerid.toInt();
  Serial.print("Received Data: ");
  Serial.print("Fingerprint ID: " + fingerid);
  Serial.println("");
  deleteFingerprint(id);
  deleteData(fingerid);
}
void handleCheckAttendance() {
  String action = server.arg("action");
  if (action.equals("attendance")) {
    Serial.println("Attendace now");
    lcd.clear();
    lcd.print("Attendace now");
    int id = getFingerprintID();
    while(id <= 0 && action.equals("attendance")){
      id = getFingerprintID();
      delay(2000);
    }
    if (action.equals("attendance") && id > 0) {
      getData(id);
    }
  }
  if (action.equals("endattendance")) {
    Serial.println("End attendace");
    lcd.clear();
    lcd.print("End attendace");
  }
}
void handleEnrollFingerprint() {
  String fingerid = server.arg("fingerid");
  String name = server.arg("name");
  String number = server.arg("number");
  String email = server.arg("email");
  int id = fingerid.toInt();
  // Xử lý dữ liệu nhận được từ ESP8266
  Serial.print("Received Data: ");
  Serial.print("Fingerprint ID: " + fingerid);
  Serial.print(", Name: " + name);
  Serial.print(", Number: " + number);
  Serial.print(", Email: " + email);
  Serial.println();
  enrollFingerprint(id);
  sendData(fingerid, name, number, email);
  // Thực hiện các xử lý khác nếu cần

  // Phản hồi cho ESP8266 (nếu cần)
  server.send(200, "text/plain", "Data received successfully");
}
void deleteData(String fingerid) {
  String path ="/Enroll/" + fingerid;
  firebase.deleteData(path);
}
void sendData(String fingerid, String name, String number, String email) {
  String path = "/Enroll/" + fingerid;
  firebase.pushString(path, fingerid);
  firebase.pushString(path + "/name", name);
  firebase.pushString(path + "/number", number);
  firebase.pushString(path + "/email", email);
}

void getData(int fingerId) {
  String path = "/Enroll/" + String(fingerId) + "/"; // Đường dẫn đến Firebase dựa trên fingerId
  String name = firebase.getString(path + "name");
  String name1 = extractValue(name);
  String number = firebase.getString(path + "number");
  String number1 = extractValue(number);
  String email = firebase.getString(path + "email");
  String email1 = extractValue(email);
  Serial.print("Data from firebase: "); 
  Serial.println("fingerid=" + String(fingerId) + ", name=" + name1 + ", number=" + number1 + ", email="+email1); 
  Serial.println();
  //connectToWiFi();
  ++status;
  updatesheet(fingerId, name1, number1, email1);
}

String extractValue(String input) {
  // Tìm vị trí của dấu hai chấm
  int colonIndex = input.indexOf(':');

  // Tìm vị trí của dấu ngoặc kép bắt đầu và kết thúc
  int startQuoteIndex = input.indexOf('"', colonIndex);
  int endQuoteIndex = input.indexOf('"', startQuoteIndex + 1);

  // Trích xuất chuỗi con từ vị trí bắt đầu đến kết thúc
  String result = input.substring(startQuoteIndex + 1, endQuoteIndex);

  return result;
}

void updatesheet(int fingerid, String name, String number, String email) {
  String status1;
  if(status == 1) {
    status1 = "Check in";
    lcd.clear();
    lcd.print("Welcome, " + number);
  }
  if(status == 2) {
    status1 = "Check out";
    lcd.clear();
    lcd.print("Googbye, " + number);
    status = 0;
  }
  static bool flag = false;
  while (!flag) {
    client = new HTTPSRedirect(httpsPort);
    client->setInsecure();
    flag = true;
    client->setPrintResponseBody(true);
    client->setContentTypeHeader("application/json");
  }
  if (client != nullptr) {
    if (!client->connected()) {
      client->connect(host, httpsPort);
    }
  }
  else {
    Serial.println("Error creating client object!");
  }
  // Create json object string to send to Google Sheets
  payload = payload_base + "\"" + String(fingerid) + "," + name + "," + number  + "," + email + "," + status1 + "\"}";

  // Publish data to Google Sheets
  Serial.println("Publishing data...");
  Serial.println(payload);
  if (client->POST(url, host, payload)) {
    // do stuff here if publish was successful
  }
  else {
    // do stuff here if publish was not successful
    Serial.println("Error while connecting");
  }
  // a delay of several seconds is required before publishing again
  delay(5000);
}

void enrollFingerprint(int fingerId) {
  //int fingerId = -1;

  Serial.println("Place your finger on the sensor...");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Put your finger ");
  lcd.setCursor(0, 1);
  lcd.print("on the sensor");
  while (true) {
    int p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        lcd.clear();
        lcd.print("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.print(".");
        delay(500);
        continue;
      default:
        Serial.println("Unknown error");
        return;
    }

    p = finger.image2Tz(1);
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image converted");
        lcd.clear();
      lcd.print("Image converted");
        break;
      case FINGERPRINT_IMAGEMESS:
        Serial.println("Image too messy");
        continue;
      default:
        Serial.println("Unknown error");
        return;
    }

    Serial.println("Remove your finger...");

    delay(2000);
    p = 0;
    while (p != FINGERPRINT_NOFINGER) {
      p = finger.getImage();
    }

    Serial.println("Place your finger again...");
    lcd.clear();
  lcd.print("Place again");
    p = -1;
    while (p != FINGERPRINT_OK) {
      p = finger.getImage();
    }

    p = finger.image2Tz(2);
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image converted");
        break;
      case FINGERPRINT_IMAGEMESS:
        Serial.println("Image too messy");
        continue;
      default:
        Serial.println("Unknown error");
        return;
    }

    Serial.println("Creating model...");

    p = finger.createModel();
    if (p == FINGERPRINT_OK) {
      Serial.println("Prints matched!");
      lcd.clear();
    lcd.print("Prints matched!");
    } else {
      Serial.println("Fingerprints did not match");
      continue;
    }

    p = finger.storeModel(fingerId);
    if (p == FINGERPRINT_OK) {
      lcd.clear();
      lcd.print("Success");
      lcd.setCursor(0, 1);
      lcd.print("Enrolling id:" + fingerId);
      Serial.println("Fingerprint enrolled successfully!");
      Serial.print("Finger ID: ");
      Serial.println(fingerId);
      return;
    } else {
      Serial.println("Error storing fingerprint");
    }
  }
}

int getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      lcd.clear();
      lcd.print("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
      return 0;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return -2;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return -2;
    default:
      Serial.println("Unknown error");
      return -2;
  }
  // OK success!
  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      lcd.clear();
      lcd.print("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return -1;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return -2;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return -2;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return -2;
    default:
      Serial.println("Unknown error");
      return -2;
  }
  // OK converted!
  p = finger.fingerFastSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return -2;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    return -1;
  } else {
    Serial.println("Unknown error");
    return -2;
  }
  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);
  lcd.clear();
  lcd.print("Found ID:" + finger.fingerID);
  return finger.fingerID;
}

uint8_t deleteFingerprint(int id) {
  uint8_t p = -1;

  p = finger.deleteModel(id);

  if (p == FINGERPRINT_OK) {
    Serial.println("Deleted!");
    lcd.clear();
    lcd.print("ID: " + id + " Deleted");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    lcd.clear();
    lcd.print("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not delete in that location");
    lcd.clear();
    lcd.print("Could not delete");
    lcd.setCursor(0,1);
    lcd.print("in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    lcd.clear();
    lcd.print("Error writing");
    lcd.setCursor(0,1);
    lcd.print("to flash");
    return p;
  } else {
    Serial.print("Unknown error: 0x"); Serial.println(p, HEX);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Unknown error: 0x");
    return p;
  }
  return p;
}

void connectToWiFi() {
  WiFi.mode(WIFI_OFF);
  delay(1000);
  WiFi.mode(WIFI_STA);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  lcd.clear();
  lcd.print("Connecting to ");
  lcd.setCursor(1, 0);
  lcd.print(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("Connected");
  lcd.clear();
  lcd.print("Connected, IP: ");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());  //IP address assigned to your ESP

}
