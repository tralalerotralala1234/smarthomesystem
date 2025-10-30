#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include "BluetoothSerial.h"
#include <ESP32Servo.h>

// --- Definiciones ---
#define DHTPIN 4
#define DHTTYPE DHT22
#define FC22_PIN 5
#define LED_PIN 2          // LED Baño
#define EXTRA_LED_PIN 18   // LED Habitación
#define SERVO_PIN 19

LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);
BluetoothSerial SerialBT;
Servo myServo;

String device_name = "ESP32_Sensores";
bool deviceConnected = false;
unsigned long previousMillis = 0;
const long interval = 2000;

bool showAction = false;
unsigned long actionMillis = 0;

bool led1State = false;
bool led2State = false;
bool servoOpen = false;
String currentProfile = "";
bool isDefaultMode = true;

// --- Mostrar mensajes temporales ---
void showLCDMessage(String msg) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(msg);
  showAction = true;
  actionMillis = millis();
}

// --- Pantalla principal ---
void showMainScreen(float t, float h) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(t, 1);
  lcd.print("C H:");
  lcd.print(h, 0);
  lcd.print("%");
  lcd.setCursor(0, 1);
  if (deviceConnected)
    lcd.print("Conectado      ");
  else
    lcd.print("Buscando...    ");
}

// --- Restaurar modo fábrica ---
void resetDefaults() {
  led1State = false;
  led2State = false;
  servoOpen = false;
  currentProfile = "";
  isDefaultMode = true;

  digitalWrite(LED_PIN, LOW);
  digitalWrite(EXTRA_LED_PIN, LOW);
  myServo.write(90);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Modo Default");
  lcd.setCursor(0, 1);
  lcd.print("Restablecido");

  delay(1500);
  lcd.clear();
}

// --- Setup ---
void setup() {
  Serial.begin(115200);
  dht.begin();

  pinMode(FC22_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(EXTRA_LED_PIN, OUTPUT);

  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Iniciando...");

  myServo.attach(SERVO_PIN);
  myServo.write(90);

  SerialBT.begin(device_name);
  lcd.clear();
  lcd.print("Buscando...");
  delay(1000);
}

// --- Loop ---
void loop() {
  unsigned long currentMillis = millis();

  if (SerialBT.connected() && !deviceConnected)
    deviceConnected = true;
  else if (!SerialBT.connected() && deviceConnected)
    deviceConnected = false;

  // --- Lectura sensores ---
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    int fc22Value = digitalRead(FC22_PIN);
    String fc22Status = (fc22Value == HIGH) ? "seguro" : "peligro";

    if (!isnan(h) && !isnan(t) && deviceConnected) {
      String dataStr = String(t, 1) + "," + String(h, 0) + "," + fc22Status;
      SerialBT.println(dataStr);
    }

    if (!showAction) showMainScreen(t, h);
  }

  if (showAction && millis() - actionMillis >= 1500) showAction = false;

  // --- Comandos Bluetooth ---
  if (SerialBT.available()) {
    String command = SerialBT.readStringUntil('\n');
    command.trim();

    if (command == "DEFAULT") {
      resetDefaults();
      return;
    }

    if (command == "ON") {
      led1State = true;
      digitalWrite(LED_PIN, HIGH);
      showLCDMessage("Baño Encendido");
    } else if (command == "OFF") {
      led1State = false;
      digitalWrite(LED_PIN, LOW);
      showLCDMessage("Baño Apagado");
    }

    else if (command == "ON2") {
      led2State = true;
      digitalWrite(EXTRA_LED_PIN, HIGH);
      showLCDMessage("Habitacion ON");
    } else if (command == "OFF2") {
      led2State = false;
      digitalWrite(EXTRA_LED_PIN, LOW);
      showLCDMessage("Habitacion OFF");
    }

    else if (command == "ABRIR") {
      servoOpen = true;
      myServo.write(180);
      showLCDMessage("Puerta Abierta");
    } else if (command == "CERRAR") {
      servoOpen = false;
      myServo.write(90);
      showLCDMessage("Puerta Cerrada");
    }

    else if (command == "PAPA") {
      currentProfile = "PAPA";
      isDefaultMode = false;
      digitalWrite(LED_PIN, HIGH);
      digitalWrite(EXTRA_LED_PIN, LOW);
      myServo.write(180);
      showLCDMessage("Perfil: Papa");
    } else if (command == "MAMA") {
      currentProfile = "MAMA";
      isDefaultMode = false;
      digitalWrite(LED_PIN, HIGH);
      digitalWrite(EXTRA_LED_PIN, HIGH);
      myServo.write(90);
      showLCDMessage("Perfil: Mama");
    } else if (command == "MACHETE") {
      currentProfile = "MACHETE";
      isDefaultMode = false;
      digitalWrite(LED_PIN, LOW);
      digitalWrite(EXTRA_LED_PIN, HIGH);
      myServo.write(180);
      showLCDMessage("Perfil: Machete");
    }
  }
}

