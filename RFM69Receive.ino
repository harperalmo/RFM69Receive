
//Version 0.1
//This is a test change to see what's up with github
//Remote temperature/humidity reporter system.  This is the receive side initially 
//using a Moteino(with RFM69W)to display the temp/humidity data sent by 
//another Moteino that is connected to a Sensirion SHT15.  The SHT15 is on 
//a breakout created by the Wonderful Wizards of AdaFruit.  If you are using
//an SHT15, be sure to visit www.adafruit.com and read about humidity sensor
//reconditioning. 

//This version uses a Sparkfun 3.3V serial backpack 16x2 LCD for display.
//
//This code used the RFM69 arduino library's struct_receive example
//as a beginning. The library is available from LowPowerLabs, the home of Moteino
//and other cool things, including a lot of useful information.
//www.lowpowerlabs.com
//  Low Power Labs, Sparkfun, and Adafruit deserve your business. They support
//  open software, open hardware, and do lots of education.  


//SoftwareSerial is used to write to the LCD
#include <SoftwareSerial.h>

#include <RFM69.h>  //Radio lib
#include <SPI.h>    //Radio uses SPI to talk to moteino with

SoftwareSerial serialLCD(8,7); // pin 7 = TX, pin 8 = RX (unused)

//Constants for meandering around the 16x2 display
#define lcdLineLength     16     //Number of positions per line
#define lcdLineCount      2     //number of lines per LCD display

#define lcdLine2Inc       0x40  //Increment between line one, position 1 (128) and line 2, pos 2( 192).
#define lcdFirstPos       0x80  //The numeric code for first line, first character position

#define lcdCursorCmd      0xFE  // HD44780 type LCD's use this to start a command to position the cursor
#define lcdClearDisp      0x01  //Clears the entire display

#define backLightCmd      0x7c  //command control code for backlight settings (and other things)
#define backLightOff      0x80  //Turn off display
#define backLight40       0x8C  //40% on
#define backLight73       0x96  //73%on
#define backLight100      0x9D  //100% on

#define lcdDegreeChar    '\xDF' //THis is the closest to the degree symbol on the selected ROM code
#define lcdPercentChar   '\x25'
char    tempUnit[]     = { ' ', 'lcdDegreeChar', 'F', '\0'};
char    humidityUnit[] = { ' ', 'lcdPercentChar', '\0'}; 

#define LCD_BAUD           9600

#define SERIAL_MONITOR_BAUD 115200   //for debugging using the arduino IDE serial monitor


#define NODEID      1
#define NETWORKID   100
#define FREQUENCY   RF69_433MHZ        //Match this with the version of your Moteino! (others: RF69_433MHZ, RF69_868MHZ)
#define KEY         "thisIsEncryptKey" //has to be same 16 characters/bytes on all nodes, not more not less!
#define ACK_TIME    30                 // # of ms to wait for an ack

typedef struct {		
  int           nodeId;    //store this nodeId
  float         temp;      //temperature
  float         humidity;  //relative humidity
  } Payload;
Payload theData;

RFM69    radio;
boolean  promiscuousMode = false; //set to 'true' to sniff all packets on the same network
byte     ackCount=0;
boolean  dataIsGood;



void setup() {
  serialLCD.begin( LCD_BAUD );
  delay(1000);
  clearLCD();
  
  Serial.begin(SERIAL_MONITOR_BAUD);
  delay(10);

  radio.initialize(FREQUENCY,NODEID,NETWORKID);
  //radio.setHighPower(); //uncomment only for RFM69HW!
  radio.encrypt(KEY);
  radio.promiscuous(promiscuousMode);
  }



void loop() {
   dataIsGood = true;;
   if (radio.receiveDone()){
     Serial.print('[');Serial.print(radio.SENDERID, DEC);Serial.print("] ");
     Serial.print(" [RX_RSSI:");Serial.print(radio.readRSSI());Serial.print("]");
     if (promiscuousMode){
       Serial.print("to [");Serial.print(radio.TARGETID, DEC);Serial.print("] ");
       }
	
     if (radio.DATALEN != sizeof(Payload)){
       dataIsGood = false;
       lcdErrorMsg("Bad Data");
       }
    
     if (radio.ACK_REQUESTED){
       byte theNodeID = radio.SENDERID;
       radio.sendACK();

       // When a node requests an ACK, respond to the ACK
       // and also send a packet requesting an ACK (every 3rd one only)
       // This way both TX/RX NODE functions are tested on 1 end at the GATEWAY
       if (ackCount++%3==0){
         Serial.print(" Pinging node ");
         Serial.print(theNodeID);
         Serial.print(" - ACK...");
         delay(3); //need this when sending right after reception .. ?
         
         if (radio.sendWithRetry(theNodeID, "ACK TEST", 8, 0))  // 0 = only 1 attempt, no retries
           Serial.print("ok!");
         else Serial.print("nothing");
         }
       }
    Serial.println();
   } //end if radio.receiveDone
  
   if( dataIsGood ){
     writeLcdLine( 0, "temp     ", theData.temp, tempUnit );
     writeLcdLine( 1, "humidity ", theData.humidity, humidityUnit);
     }
  } //end of loop()


void writeLcdLine(  byte line, char* caption, float val, char* units  ){
  positionCursor( line, 0);
  serialLCD.print( caption);
  serialLCD.print( val, 1);
  serialLCD.print( units );
  }

void clearLCD(){
  serialLCD.write( lcdCursorCmd); // move cursor to beginning of first line
  serialLCD.write( lcdClearDisp);
  }
 
 void positionCursor( byte line, byte column){
   byte positionCode;
   //This is 0 based, e.g. line= 0, column = 15 positions
   //the cursor on the first line in the last position if the 
   //LCD is 16 characters wide
   serialLCD.write(lcdCursorCmd);
   //The following code does not generalize to other MxN
   //display cofigurations.  Only works for 16x2
   //Need to read the driver spec to see if there
   //is a generalization
   positionCode = (line*lcdLine2Inc + column) | lcdFirstPos;
   serialLCD.write( positionCode );
   }

 void lcdErrorMsg( char *msg){
   clearLCD();
   serialLCD.print( msg );
   }

