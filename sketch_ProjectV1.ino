#include <ESP8266WiFi.h>
#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoJson.h>

#define BLINKING_TIME 100 //В миллисекундах
#define TIME_TO_LEAVE 5000 //В миллисекундах

bool answer = false;    //Ответ сервера, на поставленный вопрос
int idESP = 1;          //Идендификатор ESP
bool canWork = false;   //Возможность работать с сервером

//Для работы с Wi-Fi
const char *ssid = "SecPiServer";     //Имя сети Wi-Fi
const char *password = "21031999";  //пароль
const char *host = "192.168.4.1";  //Ip сервера
int port = 9090;                    //Порт
WiFiClient client;

//Для работы с RFID-RC522 
#define RST_PIN 0 
#define SS_PIN 15

MFRC522 rfid(SS_PIN, RST_PIN); //Инициализация переменной
unsigned long uidDec, uidDecTemp;  // для хранения номера метки в десятичном формате

//Данные для работы датчика обнаружения
#define PIR_PIN 5 
boolean pirValue;               // Переменная для сохранения значения из PIR

//Для работы со светодиодом
#define PLUS 2
#define MINUS 16

//Для работы с таймером
unsigned int oldTime = 0;
unsigned int nowTime = 0;  

//Для работы с JSON
 StaticJsonDocument<200> doc; 
 /////////////////////////////////////////////
 ////////////////ФУНКЦИИ//////////////////////
 /////////////////////////////////////////////
 
void readUID()
{
 // Выдача серийного номера метки.
  for (byte i = 0; i < rfid.uid.size; i++)
  {
    uidDecTemp = rfid.uid.uidByte[i];
    uidDec = uidDec * 256 + uidDecTemp;
  } 
}


bool checkErrorServer();
void sendMessage(int whichMessage, bool alert_part = true)
{
    
    client.connect(host,port);
    
    if(client.connected())
    {
    doc.clear();
    switch(whichMessage)
    {
    case 1:{
    //////////////////////////////////
    //Отправить на проверку ID карты//
    //////////////////////////////////
    
     // размер
      Serial.println();
      Serial.println("send message check_id_card ");
      int len = 41; //37 
      char lenM[2];
      itoa(len,lenM,10);
      client.println(lenM);
      delay(100);
      Serial.print("size message ");
      Serial.println(lenM);

      //Запись данных
      doc["id_esp"] = idESP;  
      doc["check_id_card"] = uidDec ;
      
      //Вывод отправленного сообщения
      serializeJson(doc, Serial);
      //Отправка сообщения
      serializeJson(doc, client);

      doc.clear();  //Очистка документа
      
      //Задержка на принятие ответа
      delay(1000);
      //Принятие ответа
      deserializeJson(doc, client);
      Serial.println();
      Serial.println("-------");
      //Вывод ответа в консоль
      serializeJson(doc, Serial);
      
      if(doc["error"])
      {//Проверка на ошибки
        checkErrorServer();
        sendMessage(1);
      }
      else
      {
      
      //Запись ответа
      answer = doc["check_result"];
      }
      break;
    }
     case 2:{
     ///////////////////////////////
     //Отправить  сигнал о тревоге//
     ///////////////////////////////
     
      // размер
      Serial.println();
      Serial.println("send message alert");
      int len = 25; 
      char lenM[2];
      itoa(len,lenM,10);
      client.print(len);
      delay(100);
      Serial.print("size message ");
      Serial.println(lenM);

      //Запись данных
      doc["id_esp"] = idESP; 
      alert_part ? doc["is_alert"] = 1 : doc["is_alert"] = 0;
      //Вывод отправленного сообщения
      serializeJson(doc, Serial);
      //Отправка сообщения
      serializeJson(doc, client);

      if(doc["error"])
      {//Проверка на ошибки
        checkErrorServer();
        sendMessage(2);
      }
      else
      {
      //Задержка
      delay(1000);
      }
      break;
     }
      case 3:{
      ///////////////////////////////////
      //Отправить запрос на регистрацию//
      ///////////////////////////////////
      // размер
      Serial.println();
      Serial.println("send message reg_esp");
      int len = 13; 
      char lenM[2];
      itoa(len,lenM,10);
      client.print(len);
      delay(100);
      Serial.print("size message ");
      Serial.println(lenM);
      
      //запись данных
      doc["reg_esp"] = idESP;
      //Вывод отправленного сообщения
      serializeJson(doc, Serial);
      //Отправка сообщения
      serializeJson(doc, client);
      
      doc.clear();  //Очистка документа
      
      //Задержка на принятие ответа
      delay(1000);
      //Принятие ответа
      deserializeJson(doc, client);

      if(doc["error"])
      {//Проверка на ошибки
        checkErrorServer(); 
      }
      else
      {
      //Вывод ответа в консоль
      Serial.println();
      Serial.println("-------");
      serializeJson(doc, Serial);
      Serial.println();

      canWork = doc["sign_esp"];  //Записываем ответ
      }
      break;
      }
    }

    //Окончание разговора
    client.stop();
    doc.clear();  //Очистка документа
    Serial.println();
    }
}

bool checkErrorServer()
{
  int error = doc["error"];
  Serial.print("error № ");
  Serial.println(error);
  switch(error)
  {
    case 1:
      //Оишбка, что датчик не зарегестрирован
      //Отправляем на повторную регистрацию
      Serial.println("Error reg on server");
      Serial.println("Send request on reg");
      sendMessage(3);
      break;
  } 
  doc.clear();
}

bool checkPIR()
{
  while(true)
  {
    pirValue = digitalRead(PIR_PIN);    // Считываем значение от датчика движения
    
    if (pirValue == 1)
    {// Если есть движение
      Serial.println("Есть движение");
      delay(50);            
      return true;
    }
    delay(100);
  }
}

void alert()
{
  while(true) //Пока не отключили сигнализацию правильной карточкой
  {
    digitalWrite(PLUS, HIGH);
    digitalWrite(MINUS, LOW); 
    
    if(checkAndSend()) //Если карта считана и проверена
    {
      if(answer) //Если карта верная
      {
        break;
      }   
    }

    delay(BLINKING_TIME);
    //Моргаем светодиодом
       
    digitalWrite(MINUS, HIGH);
    digitalWrite(PLUS, LOW); 
    delay(BLINKING_TIME);
  }
}

bool checkAndSend()
{
  delay(200);
  if ( !rfid.PICC_IsNewCardPresent()) //Есть новая карта
    return false; //Если не поднесена карта  

   // читает карту
   if ( !rfid.PICC_ReadCardSerial())
    return false;
     
   readUID();  //Переводим из byte в int (DEC)
   sendMessage(1);  //Отправляем сообщение на сервер
   return true;
}

///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////
void setup() 
{
  Serial.begin(115200);
  delay(5000);
  Serial.println();
  Serial.println("Start");
  Serial.println();

  //Датчик
  pinMode(PIR_PIN, INPUT); // Установка пин как вход
  pinMode(4, OUTPUT);     //D2 Земля датчика
  digitalWrite(4, LOW);

   //Настройка датчика
  Serial.println("Настройка началась");
  delay(30000);
  Serial.println("Настройка завершилась");

  //Светодиод
  pinMode(PLUS, OUTPUT); //D4 + 
  pinMode(MINUS, OUTPUT); //D0 -
  //Включаем светодиод
  digitalWrite(PLUS, HIGH);
  digitalWrite(MINUS, LOW);

//Выкл светодиода
//digitalWrite(MINUS, HIGH);
//digitalWrite(PLUS, LOW);

  
 // Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password); //Connect to wifi
 
  // Ждём подключения к сети  
  Serial.println("Connecting to Wifi");
  while (WiFi.status() != WL_CONNECTED) {   
    delay(500);
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  //Отправляем запрос на поделючение датчика
  sendMessage(3);
  delay(100);

  //RFID
  SPI.begin(); // инициализация SPI
  rfid.PCD_Init(); // инициализация rfid

}

void loop() 
{
  Serial.println("Step 1");
  
  //############################################
  while(WiFi.status() != WL_CONNECTED)
    delay(1000);//Восстанавливаем Wi-FI соединение 

    while(canWork == 0){
      delay(2000);//Восстанавливаем соединение с Сервером
      //Отправляем запрос на регистрацию датчика
      sendMessage(3);
      Serial.print(".");
      delay(1000);
    }
  //############################################

  Serial.println("Step 2");
  //Проверка датчика

  if(checkPIR())  //Если есть сигнал от датчика
  {
    sendMessage(2,false);
    //Запускаем таймер

    oldTime = millis();  //Начальное время
    nowTime = millis();   //Текущее время
    
    while(true)
    {
      delay(300);
      nowTime = millis();
      
      Serial.println("Step 3");
      if(nowTime - oldTime > 15000) //Если время вышло
      {
         Serial.println("ALERT");
         sendMessage(2); //Отправка на сервер сообщения о проникновении
         alert();//Включаем тревогу
         //Как только вышел значит тревога отключена
         Serial.println("Leave room");
         //Включаем светодиод
         digitalWrite(PLUS, HIGH);
         digitalWrite(MINUS, LOW);
         
         delay(TIME_TO_LEAVE); //Время чтобы покинуть комнату после включения сигнализации
         break;
      }
        answer = false;
        if(!checkAndSend()) //Считываем карту и отправляем на сервер
          continue;

      Serial.println("Step 4");
      if(answer)//Смотрим ответ от сервера
      {
         Serial.println("Step 4.1");
        /////////////////////////////////////////////////////////
        //Переходим в режим ожидания включения датчика///////////
        /////////////////////////////////////////////////////////
        
        //Выключаем светодиод
        digitalWrite(MINUS, HIGH);
        digitalWrite(PLUS, LOW);
        
        delay(10000);   //Задержка чтобы убрать карту
        answer = false;
        while(true)
        {
          
          if(checkAndSend()) //Если карта считана и проверена
          {
             if(answer) //Если карта верная
             {
                break;
             }
             //Иначе карта для включения сигнализации неверная
             //Ничего не делаем и ждём ответа
          }
          delay(100);
        }
        //Включаем светодиод
        //////////////////////////////////////////////////////////////////////
        Serial.println("Step 5");

        Serial.println("Leave room");
        //Включаем светодиод
        digitalWrite(PLUS, HIGH);
        digitalWrite(MINUS, LOW);
        
        delay(TIME_TO_LEAVE); //Время чтобы покинуть комнату после включения сигнализации
        break;  //Сигнализация снова включена
      }
      else
      {
          Serial.println("ALERT");
          alert();//Включаем тревогу
         //Как только вышел значит тревога отключена
         Serial.println("Leave room");
         //Включаем светодиод
          digitalWrite(PLUS, HIGH);
          digitalWrite(MINUS, LOW);
         
         delay(TIME_TO_LEAVE); //Время чтобы покинуть комнату после включения сигнализации
         break;
      }
      
    }
    
    delay(1000); //Даём 30 секунд чтобы уйти из помещения
    //Сбрасываем переменную о сработанном датчике
  }
}
