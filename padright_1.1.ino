#include "padrack.h"
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN 9 // Configurable, see typical pin layout above
#define SS_PIN 10 // Configurable, see typical pin layout above

MFRC522 mfrc522(SS_PIN, RST_PIN); // Create MFRC522 instance.

MFRC522::MIFARE_Key key;
MFRC522::StatusCode rfidstatus;
// regarding rfid card
int blockAddr = 2;
byte readByte[18];
byte writeByte[16];
int authAddr = 3;
byte byteSize = sizeof(readByte);

//-------------- batch Write -----------------
// Variable  for pad amount
byte padAmount = 1;

// --------- eeprom address for menu variables ---------
static byte rack1Address = 1;
static byte rack2Address = 2;

static byte motorTimeAddress = 10;
//----------- class objects ---------------------

LiquidCrystal_I2C lcd(0x27, 16, 2);
padrack rack1, rack2, rack3, rack4, rack5;
//-------------------- input and output varaibles --------------
byte motor1 = 5;
byte motor2 = 14;

byte menuButton = 6;
byte selectButton = 7;
byte okButton = 8;
byte buzzer = 3;
////----------- logic variables -------------
bool change = false;
bool isUpdate = false;
bool changeDone = true;
byte maxRackCapacity = 40;
int motorTimeVariable = 100;
uint32_t previousTime = millis();
int topMenuPosition = 0;
int state = 0;
bool makeChange = false;
char status = 'n';
byte arrow[8] = {
    0b00000,
    0b11100,
    0b10010,
    0b01001,
    0b01001,
    0b10010,
    0b11100,
    0b00000};

void setup()
{
  Serial.begin(9600);
  lcd.init(); // initialize the lcd
  lcd.backlight();
  lcd.createChar(0, arrow);
  SPI.begin();        // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522 card
  pinMode(menuButton, INPUT_PULLUP);
  pinMode(selectButton, INPUT_PULLUP);
  pinMode(okButton, INPUT_PULLUP);
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);
  pinMode(motor1, OUTPUT);
  pinMode(motor2, OUTPUT);
  digitalWrite(motor1, LOW);
  digitalWrite(motor2, LOW);
  readFromEEPROM();

  // key for auth rfid
  for (byte i = 0; i < 6; i++)
  {
    key.keyByte[i] = 0xFF;
  }
  startMessage();
}
void startMessage()
{
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("Powered By");
  lcd.setCursor(2, 1);
  lcd.print("Kaicho Group");
  delay(3000);
  lcd.clear();
  lcd.setCursor(6, 0);
  lcd.print("RED");
  lcd.setCursor(2, 1);
  lcd.print("INTERNATIONAL");
  delay(5000);
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Kawach");
  lcd.setCursor(2, 1);
  lcd.print("Pad Vending");
  delay(2000);
  homePage2();
}
void loop()
{
  menuManagement();
  manageRFID();
}
void manageRFID()
{
  if (mfrc522.PICC_IsNewCardPresent())
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Card Detected!");
    lcd.setCursor(0, 1);
    lcd.print("Please Wait.....");
    if (mfrc522.PICC_ReadCardSerial())
    {
      if (readCard())
      {
        success(500);
        if (readByte[15] != 0 && readByte[0] == 107) // verify the card and available quantity on card
        {
          dumpToWriteVar(readByte, 16);
          if (!writeCard(writeByte))
          {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Failed!");
            lcd.setCursor(0, 1);
            lcd.print("Try Again");
            warning();
          }
          else
          {
            byte lastData = readByte[15];
            readCard();
            if (getStock() != 0) // check the stock before proceeding
            {
              if (lastData != readByte[15]) // check the stock before proceeding
              {
                delay(500);
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Left for Card:");
                lcd.setCursor(0, 1);
                lcd.print(writeByte[15]);
                delay(3000);
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Receive Pad");
                lcd.setCursor(0, 1);
                lcd.print("Thank you!");
                success(800);
                delay(1000);
                runMotor();
              }
              else // print if card scan time is fast
              {
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Sorry");
                lcd.setCursor(0, 1);
                lcd.print("Hold card for 1Sec");
                warning();
              }
            }

            else // print if no stock remaining
            {
              lcd.clear();
              lcd.setCursor(0, 0);
              lcd.print("Sorry");
              lcd.setCursor(0, 1);
              lcd.print("No Stocks");
              warning();
            }
          }
        }
        else
        {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Opps!");
          lcd.setCursor(0, 1);
          lcd.print("Zero Balance");
          warning();
        }
      }
      else
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Failed!");
        lcd.setCursor(0, 1);
        lcd.print("Try Again");
        warning();
      }
    }
    halt();
    status = 'n';
    changeDone = true;
  }
}
void menuManagement()
{
  if (status == 'n' || status == 'a')
  {
    if (!digitalRead(menuButton))
    {
      delay(300);
      state++;
      if (state == 2)
        state = 0;
      makeChange = true;
      changeDone = true;
    }
    else if (!digitalRead(selectButton))
    {
      delay(300);
      topMenuPosition++;
      if (topMenuPosition == 4)
        topMenuPosition = 0;
      makeChange = true;
      changeDone = true;
    }
    if (!digitalRead(okButton) && state == 1)
    {
      delay(300);
      if (state == 1 && topMenuPosition == 0)
        status = 'r';
      else if (state == 1 && topMenuPosition == 1)
        status = 'f';
      else if (state == 1 && topMenuPosition == 2)
        status = 'm';
      else if (state == 1 && topMenuPosition == 3)
        status = 'b';
      // Serial.println(status);
      switch (status)
      {
      case 'f':
        fillMenu();
        while (digitalRead(okButton))
        {
          // delay(300);
          if (!digitalRead(selectButton))
          {
            delay(300);
            maxRackCapacity = maxRackCapacity + 1;
            if (maxRackCapacity > 60)
              maxRackCapacity = 1;
            fillMenu();
          }
        }
        fillAll(maxRackCapacity);
        break;
      case 'm':
        motorTime();
        while (digitalRead(okButton))
        {
          // delay(300);
          if (!digitalRead(selectButton))
          {
            delay(300);
            motorTimeVariable = motorTimeVariable + 100;
            if (motorTimeVariable > 5000)
              motorTimeVariable = 100;
            motorTime();
          }
        }
        break;
      case 'r':
        manageRack1();
        while (digitalRead(okButton))
        {
          // delay(300);
          if (!digitalRead(selectButton))
          {
            delay(300);
            rack1.incQuantity();
            manageRack1();
          }
        }
        while (!digitalRead(okButton))
          ;
        manageRack2();
        while (digitalRead(okButton))
        {
          if (!digitalRead(selectButton))
          {
            delay(300);
            rack2.incQuantity();
            manageRack2();
          }
        }
        while (!digitalRead(okButton))
          ;
        break;
      case 'b':
        batchWrite();
        while (digitalRead(okButton))
        {
          if (!digitalRead(selectButton))
          {
            delay(300);
            padAmountInc();
            batchWrite();
          }
        }
        while (!digitalRead(okButton))
          ;
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Scan Card!");
        while (digitalRead(okButton))
        {
          if (mfrc522.PICC_IsNewCardPresent())
          {
            if (mfrc522.PICC_ReadCardSerial())
            {
              // writeByte[0] = 107;
              writeByte[15] = padAmount;
              if (writeCard(writeByte))
              {
                lcd.setCursor(0, 1);
                lcd.print("Done");
                success(500);
              }
            }
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Scan Card!");
            halt();
          }
        }
        // halt();
        while (!digitalRead(okButton))
          ;
        /////   batch code
        break;
      }
      writeToEPPROM(status);
      success(800);
      save();
      changeDone = true;
    }
  }

  menu();
  makeChange = false;
}

void homePage2()
{
  if (changeDone == true)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("SCAN CARD HERE");
    lcd.setCursor(0, 1);
    lcd.print("N0 OF STOCKS:");
    lcd.print(getStock());
    changeDone = false;
  }
}
void topMenu()
{
  switch (topMenuPosition)
  {
  case 0:
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.write((byte)0);
    lcd.setCursor(3, 0);
    lcd.print("Rack Quantity");
    lcd.setCursor(3, 1);
    lcd.print("Fill All Rack");
    break;
  case 1:
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("Rack Quantity");
    lcd.setCursor(0, 1);
    lcd.write((byte)0);
    lcd.setCursor(3, 1);
    lcd.print("Fill All Rack");
    break;
  case 2:
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("Fill All Rack");
    lcd.setCursor(0, 1);
    lcd.write((byte)0);
    lcd.setCursor(3, 1);
    lcd.print("Motor Time");
    break;
  case 3:
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("Motor Time");
    lcd.setCursor(0, 1);
    lcd.write((byte)0);
    lcd.setCursor(3, 1);
    lcd.print("Batch Write");
    break;
  }
}
void menu()
{
  switch (state)
  {
  case 0:
    homePage2();
    topMenuPosition = 0;
    status = 'a';
    break;
  case 1:
    if (makeChange)
      topMenu();
    isUpdate = false;
    break;
  }
}
void batchWrite()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pad Amount");
  lcd.setCursor(0, 1);
  // enter number
  lcd.print(padAmount);
}
void manageRack1()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Rack1  Quantity");
  lcd.setCursor(0, 1);
  // enter number
  lcd.print(rack1.getQuantity());
}
void manageRack2()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Rack2 Quantity");
  lcd.setCursor(0, 1);
  // enter number
  lcd.print(rack2.getQuantity());
}
void motorTime()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Motor Time (ms)");
  lcd.setCursor(0, 1);
  // enter number
  lcd.print(motorTimeVariable);
}
void save()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Saving...");
  digitalWrite(buzzer, HIGH);
  delay(2000);
  digitalWrite(buzzer, LOW);
  status = 'n';
  state = 0;
}
void writeToEPPROM(char status)
{

  if (status == 'r' || status == 'f')
  {
    EEPROM.write(rack1Address, rack1.getQuantity());
    EEPROM.write(rack2Address, rack2.getQuantity());
  }
  else if (status == 'm')
  {
    writeIntIntoEEPROM(motorTimeAddress, motorTimeVariable);
  }
}
void readFromEEPROM()
{
  motorTimeVariable = readIntFromEEPROM(motorTimeAddress);
  if (motorTimeVariable == -1)
    motorTimeVariable = 0;
  rack1.setQuantity(EEPROM.read(rack1Address));
  rack2.setQuantity(EEPROM.read(rack2Address));
}

void writeIntIntoEEPROM(int address, int number)
{
  EEPROM.write(address, number >> 8);
  EEPROM.write(address + 1, number & 0xFF);
}
int readIntFromEEPROM(int address)
{
  return (EEPROM.read(address) << 8) + EEPROM.read(address + 1);
}

bool readCard()
{
  byte buffersize = 18;
  if (auth_A())
  {
    rfidstatus = (MFRC522::StatusCode)mfrc522.MIFARE_Read(blockAddr, readByte, &buffersize);
    if (rfidstatus != MFRC522::STATUS_OK)
    {
      return false;
    }
  }
  return true;
}
bool writeCard(byte writeByte[])
{
  if (auth_B())
  {
    rfidstatus = (MFRC522::StatusCode)mfrc522.MIFARE_Write(blockAddr, writeByte, 16);
    if (rfidstatus != MFRC522::STATUS_OK)
    {
      return false;
    }
  }
  return true;
}

void dumpToWriteVar(byte *buffer, byte bufferSize)
{
  for (byte i = 0; i < bufferSize; i++)
  {
    Serial.print(buffer[i]);
    writeByte[i] = buffer[i];
  }
  writeByte[15]--;
}
void halt()
{
  mfrc522.PICC_HaltA();      // Halt PICC
  mfrc522.PCD_StopCrypto1(); // Stop encryption on PCD
}
bool auth_A()
{
  rfidstatus = (MFRC522::StatusCode)mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, authAddr, &key, &(mfrc522.uid));
  if (rfidstatus != MFRC522::STATUS_OK)
  {
    return false;
  }
  return true;
}
bool auth_B()
{
  rfidstatus = (MFRC522::StatusCode)mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_B, authAddr, &key, &(mfrc522.uid));
  if (rfidstatus != MFRC522::STATUS_OK)
  {
    return false;
  }
  return true;
}
void runMotor()
{
  Serial.println("motor");
  if (getStock() > 0)
  {
    if (rack1.getQuantity() != 0)
    {
      Serial.println("motor1");
      digitalWrite(motor1, HIGH);
      delay(motorTimeVariable);
      digitalWrite(motor1, LOW);
      rack1.decQuantity();
    }
    else if (rack2.getQuantity() != 0)
    {
      Serial.println("motor2");
      digitalWrite(motor2, HIGH);
      delay(motorTimeVariable);
      digitalWrite(motor2, LOW);
      rack2.decQuantity();
    }
    writeToEPPROM('r');
  }
}
void fillAll(int num)
{
  rack1.setQuantity(num);
  rack2.setQuantity(num);
}
void fillMenu()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enter capacity");
  lcd.setCursor(0, 1);
  lcd.print("of one Rack:");
  lcd.print(maxRackCapacity);
}
int getStock()
{
  return rack1.getQuantity() + rack2.getQuantity();
}
void success(int _time)
{
  digitalWrite(buzzer, HIGH);
  delay(_time);
  digitalWrite(buzzer, LOW);
}
void warning()
{
  digitalWrite(buzzer, HIGH);
  delay(400);
  digitalWrite(buzzer, LOW);
  delay(400);
  digitalWrite(buzzer, HIGH);
  delay(400);
  digitalWrite(buzzer, LOW);
  delay(400);
  digitalWrite(buzzer, HIGH);
  delay(400);
  digitalWrite(buzzer, LOW);
}
void padAmountInc()
{
  padAmount++;
  if (padAmount > 20)
  {
    padAmount = 1;
  }
}