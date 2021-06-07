#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Wire.h>
#include <OneWire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <SPIFFS.h>

//Датчик BME280
Adafruit_BME280 bme;                   // Подключаем датчик в режиме I2C

// Задаем сетевые настройки
const char* ssid = "*******";
const char* password = "*********";
IPAddress local_IP(192, 168, 1, 68);   // Задаем статический IP-адрес:
IPAddress gateway(192, 168, 1, 102);   // Задаем IP-адрес сетевого шлюза:
IPAddress subnet(255, 255, 255, 0);    // Задаем маску сети:
IPAddress primaryDNS(8, 8, 8, 8);      // Основной ДНС (опционально)
IPAddress secondaryDNS(8, 8, 4, 4);    // Резервный ДНС (опционально)
AsyncWebServer server(80);             // Создаем сервер через 80 порт

// Default Threshold Temperature Value
String inputMessage = "25.0";          // пороговое значение температуры
String lastTemperature;                // ПеременнаяlastTemperature будет содержать последние показания температуры, которые будут сравниваться с пороговым значением.
String enableArmChecked = "checked";   // ПеременнаяenableArmChecked сообщит нам, установлен ли флажок для автоматического управления выводом или нет.
String inputMessage2 = "true";         // В случае, если он установлен, значение, сохраненное на inputMessage2, должно быть установлено в true.


void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

// BME280 - Универсальный датчик
String getTemperature2() {
  float IN4 = bme.readTemperature()-1.04;
  Serial.print("BME280- Температура: ");
  Serial.print(IN4);
  Serial.print(F(" °C ")); 
  return String(IN4);
}

String getPressure() {
  float IN5 = bme.readPressure()/133.3;
  Serial.print(" Давление: ");
  Serial.print(IN5);
  Serial.print(" мм.рт.ст ");
  return String(IN5);
}
  
String getHumidity() {
  float IN6 = bme.readHumidity();
  Serial.print(" Влажность: ");
  Serial.print(IN6);
  Serial.println(" %");
  return String(IN6);
}

//*****************************************************

//Заменяем placeholder значениями BME280
String processor(const String& var){
  //Serial.println(var);
  if(var == "DATA4"){
    return lastTemperature;
  }
  else if(var == "THRESHOLD1"){
    return inputMessage;
  }
  else if(var == "AUTO1"){
    return enableArmChecked;
  }
  return String();
}

// Переменная флага для отслеживания того, были ли активированы триггеры или нет
bool triggerActive = false;
// Следующие переменные будут использоваться для проверки того, получили ли мы HTTP-запрос GET из этих полей ввода, и сохранения значений в переменные соответственно.
const char* PARAM_INPUT_1 = "threshold_input";
const char* PARAM_INPUT_2 = "enable_arm_input";

// Интервал между показаниями датчиков. Узнайте больше о таймерах ESP 32: https://RandomNerdTutorials.com/esp32-pir-motion-sensor-interrupts-timers/
unsigned long previousMillis = 0;     
const long interval = 5000;    

void setup() {
  Serial.begin(115200);

  // Настройки WiFI
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("WiFi Failed!");
    return;
  }
  Serial.println();
  Serial.print("ESP IP Address: http://");
  Serial.println(WiFi.localIP());
  
// Установите GPIO 32 в качестве выходного сигнала и установите его на НИЗКИЙ уровень при первом запуске ESP.    
  pinMode(32, OUTPUT);
  digitalWrite(32, LOW);
  
  // Инициализация и запуск датчика BME280
  if (!bme.begin(0x76)) {
	Serial.println("Не обнаружен датчик BME280, проверьте соеденение!");
    while (1);
 }
  
    // Инициализируем SPIFFS:
  if (!SPIFFS.begin()) {
    Serial.println("При монтировании SPIFFS произошла ошибка");
    return;
  }

  // Настраиваем статический IP-адрес:
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("Режим клиента не удалось настроить");
  }

// Отправить веб-страницу в браузер
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/index.html", "text/html", false, processor);
  });

// Итак, мы проверяем, содержит ли запрос входные параметры, и сохраняем эти параметры в переменные:
  // Получите HTTP GET запрос по адресу <ESP_IP>/get?threshold_input=<inputMessage>&enable_arm_input=<inputMessage2>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
/* Это часть кода, где переменные будут заменены значениями, представленными на форме. Переменная inputMessage 
сохраняет пороговое значение температуры, а переменная inputMessage2 сохраняет, установлен ли флажок или нет 
(если мы должны контролировать GPIO или нет)*/
    // ПОЛУЧИТЬ значение threshold_input on <ESP_IP>/get?threshold_input=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      // ПОЛУЧИТЬ значение enable_arm_input on<ESP_IP>/get?enable_arm_input=<inputMessage2>
      if (request->hasParam(PARAM_INPUT_2)) {
        inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
        enableArmChecked = "checked";
      }
      else {
        inputMessage2 = "false";
        enableArmChecked = "";
      }
    }
    Serial.println(inputMessage);
    Serial.println(inputMessage2);
// После отправки значений в форме отображается новая страница с сообщением, что запрос был успешно отправлен в ESP32 со ссылкой для возврата на домашнюю страницу.
    request->send(200, "text/html", "HTTP GET request sent to your ESP.<br><a href=\"/\">Return to Home Page</a>");
  });
  server.onNotFound(notFound);
// Запуск веб сервера
  server.begin();
}

void loop() {
// таймеры для получения новых показаний температуры каждые 5 секунд.
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
  //  sensors.requestTemperatures();
  //  float IN4 = sensors.getTempCByIndex(0);
  float IN4 = bme.readTemperature()-1.04;
    Serial.print(IN4);
    Serial.println(" *C");
    lastTemperature = String(IN4);
    
/* Получив новое значение температуры, мы проверяем, находится ли она выше или ниже порогового значения, 
и соответственно включаем или выключаем выход.В этом примере мы устанавливаем выходное состояние на ВЫСОКОЕ, если выполняются все эти условия:
Текущая температура выше порога; Включено автоматическое управление выводом (флажок установлен на веб-странице);
Если выход еще не был запущен.*/
    
    // Проверьте, если температура выше порога и если она должна вызвать выход
    if(IN4 > inputMessage.toFloat() && inputMessage2 == "true" && !triggerActive){
      String message = String("Температура выше порога. Текущая температура: ") + 
        String(IN4) + String("C");
      Serial.println(message);
      triggerActive = true;
      digitalWrite(32, HIGH);
    }
// Затем, если температура опускается ниже порога, установите выход на НИЗКИЙуровень .
// Проверьте, не находится ли температура ниже порогового значения и не нужно ли вызвать выход
    else if((IN4 < inputMessage.toFloat()) && inputMessage2 == "true" && triggerActive) {
      String message = String("Температура ниже порога. Текущая температура: ") + 
                            String(IN4) + String(" C");
// В зависимости от вашего приложения вы можете изменить выход на НИЗКИЙ, когда температура выше порога , и на ВЫСОКИЙ, когда выход ниже порога.
      Serial.println(message);
      triggerActive = false;
      digitalWrite(32, LOW);
    }
  }
}
