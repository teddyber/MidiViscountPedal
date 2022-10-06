// 08/09/2022
//Midi Viscount Keyboard : reads keyboard switches and sends a midi message accordingly
// 2 keyboards connected on 16 bits multiplexer 74HC4067

//debug 1 : debug activated. use the serial output in console text mode
//debug 0 : functional mode. use the serial output in Midi Tx mode 

//Arduino resource mapping 
//DIO
// D0 - Tx
// D1 - Rx
// D2  ReadPin
// D3  74HC4067 - S3
// D4  74HC4067 - S2
// D5  74HC4067 - S1
// D6  74HC4067 - S0
// D7  ReadPin
// D8  CB1 - A0
// D9  CB2 - A1
// D10 CB3 - A2
// D11 CB4 - EnablePin1
// D12 CB5 - EnablePin2

//D2-D9 : read pins
#define R1 2
#define R2 3
#define R3 4
#define R4 5
#define R5 6
#define R6 7
#define R7 8
#define R8 9
#define A 14
#define B 15

#include "MIDIUSB.h"
#define MIDI_INTERFACE_USB 1

#define Debug 0

/*
#define S0 6
#define S1 5
#define S2 4
#define S3 3
#define ReadPin 2
//#define MuxEnablePin 6
#define A0 7
#define A1 8
#define A2 9
//#define EnableKeyb1 15
//#define EnableKeyb2 14
*/

#define noteOffset 0

uint8_t  octave = 3;  
uint8_t  velocite = 127;  // note volume

// holds incoming values from 74HC4067 multiplexers
uint16_t  muxValuesCurrent;
uint16_t  muxValuesLast;
uint8_t  midiProgram = 0;           // Midi program number (instrument)
uint8_t  keyb1CurrentValues[8];			// current state of keyboard 
//uint8_t  keyb2CurrentValues[8];
uint8_t  keyb1LastValues[8];			// last state of keyboard 
//uint8_t  keyb2LastValues[8];

void setup()
{
  if (Debug)
  {
    Serial.begin(9600); //for serial Tx on console
  }
  else
  {
   #if MIDI_INTERFACE_USB == 0
   Serial.begin(31250); //Open MIDI transmission
   #endif
  }
  
 //HW Inits
  pinMode(R1,INPUT_PULLUP);
  pinMode(R2,INPUT_PULLUP);
  pinMode(R3,INPUT_PULLUP);
  pinMode(R4,INPUT_PULLUP);
  pinMode(R5,INPUT_PULLUP);
  pinMode(R6,INPUT_PULLUP);
  pinMode(R7,INPUT_PULLUP);
  pinMode(R8,INPUT_PULLUP);
  pinMode(A, OUTPUT);
  pinMode(B, OUTPUT);
    
   muxValuesCurrent = 0;
   muxValuesLast = 0;

   MidiProgChange(0,midiProgram);  // Channel 1 : midi programme #0 : Piano
   //MidiProgChange(1,midiProgram);  // Channel 2 : midi programme #0 : Piano

}

/* ------------------------------------------------------------------ */ 


/* ----------------- Function selectMuxPin --------------------------- */
/* ** This function connects the ReadPin to the selected Mux channel * */
void SelectMuxPin(uint8_t outputPin)
// function to select pin on 74HC4067
{
    (outputPin & 0x1)?digitalWrite(A,HIGH):digitalWrite(A,LOW);
    (outputPin & 0x2)?digitalWrite(B,HIGH):digitalWrite(B,LOW);
    //(outputPin & 0x4)?digitalWrite(S2,HIGH):digitalWrite(S2,LOW);
    //(outputPin & 0x8)?digitalWrite(S3,HIGH):digitalWrite(S3,LOW);    
//    delay(0.1);
}
/* ------------------------------------------------------------------ */ 

/* ----------------- Function displayData  -------------------------- */
/* **                  debug function                                **/
void displayData(uint16_t mux)
// dumps captured data from array to serial monitor
{
//  Serial.println();
  Serial.print("Values from multiplexer:");
  Serial.print(mux);
  Serial.println(muxValuesCurrent,BIN); 
}
/* ------------------------------------------------------------------ */ 

/* ----------------- Function MidiProgChange-------------------------- */
/* ** Initializes the Midi instrument to Piano (default)          **/
void MidiProgChange(uint8_t channel, uint8_t prognumber) {
  Serial.write(0xC0+channel);
  Serial.write(prognumber);
}
/* ------------------------------------------------------------------ */ 

/* ----------------- Function MidiNoteOn  --------------------------- -----*/
/* ** write on the Midi bus the note                                  **/
//void MidiNoteOn(int cmd, int pitch, int velocity) {
void MidiNoteOn(midiEventPacket_t noteMidi) 
{
   if (Debug)
   {
    Serial.print("Canal:");
    Serial.print(noteMidi.byte1,HEX);
    Serial.print(" pitch=");
    Serial.print(noteMidi.byte2);
    Serial.print(" velocity=");
    Serial.println(noteMidi.byte3);
   }
   else
   {
    #if MIDI_INTERFACE_USB == 0
/*    Serial.write(cmd);
    Serial.write(pitch);
    Serial.write(velocity);*/
    Serial.write(noteMidi.byte1); // cmd
    Serial.write(noteMidi.byte2); // pitch
    Serial.write(noteMidi.byte3); // velocity
    #else
    MidiUSB.sendMIDI(noteMidi);
    MidiUSB.flush();
    #endif
   }
}
/* ------------------------------------------------------------------ */ 

/* ----------------- Function ReadMux --------------------------------*/
/* ** returns on a uint8 the status of Pedal 8 channels              **/
uint8_t ReadMux(uint8_t nbMuxInputs)
{
  uint8_t tmpReadMux=0;
  bool value=0;
  // read first multiplexer 16 pins
  for (uint8_t i = 0; i < nbMuxInputs; i++)
  {
    //SelectMuxPin(i); // choose an input pin on the plug
    value = digitalRead(R1+i); //read
    tmpReadMux ^= (-value ^ tmpReadMux ) & (1 << i);

  }
 return tmpReadMux;	
}
/* ------------------------------------------------------------------ */ 

/* ----------------- Function writeDeMux -----------------------------*/
/* ** sets the A0, A1, A2 bits to select the right column on keyboard */
void writeDeMux(uint8_t col)
{
    (col & 0x1)?digitalWrite(A,HIGH):digitalWrite(B,LOW);
    (col & 0x2)?digitalWrite(B,HIGH):digitalWrite(B,LOW);
    //(col & 0x4)?digitalWrite(A2,HIGH):digitalWrite(A2,LOW);
}
/* ------------------------------------------------------------------ */ 

/* ----------------- loop -----------------------------*/
/*****************  Main Loop **************************/
void loop()
{

//  midiEventPacket_t noteOn;
  uint8_t pitch;
  // Loop on 4 columns
  for(uint8_t keybCol = 0; keybCol <4; keybCol ++)
  {
		// Select column on keyboard demultiplexer
	  writeDeMux(keybCol);
    //select Keyboard 1
    //digitalWrite(EnableKeyb1,HIGH);
    //digitalWrite(EnableKeyb2,LOW);    
 	  //read 8 keyboard inputs
    muxValuesCurrent = ReadMux(8);
 	  
	  //keeping only 8 useful bits
	  keyb1CurrentValues[keybCol] = muxValuesCurrent & 255;
    //select Keyboard 2
    //digitalWrite(EnableKeyb1,LOW);
    //digitalWrite(EnableKeyb2,HIGH);
    //read 8 keyboard inputs
    //muxValuesCurrent = ReadMux(8);
    //keyb2CurrentValues[keybCol] = muxValuesCurrent & 255;	  

	  //is there any change in the acquired values ?
	  if( keyb1CurrentValues[keybCol] !=  keyb1LastValues[keybCol])
	  {
		   for(uint8_t j=0; j<8;j++)
			{
			  if ((keyb1CurrentValues[keybCol] >> j & 1) != (keyb1LastValues[keybCol] >> j & 1)) 
			  {
  				if (keyb1CurrentValues[keybCol] >> j & 1)
  				{
  				  //MidiNoteOn(0x90, noteOffset + j + (keybCol *8) + 12*octave, 0x00); // velocity zero = note off
            pitch = noteOffset + (7-j) + (keybCol *8) + (12*octave);
            MidiNoteOn({0x09, 0x90 , pitch , 0x00});
  				}
  				else
  				{
            pitch = noteOffset + (7-j) + (keybCol *8) + 12*octave;
  				  MidiNoteOn({0x09, 0x90, pitch, velocite});
  				}
			  }
			}

	  }
	  /*if( keyb2CurrentValues[keybCol] !=  keyb2LastValues[keybCol])
	  {
		   for(uint8_t k=0; k<8;k++)
			{
			  if ((keyb2CurrentValues[keybCol] >> k & 1) != (keyb2LastValues[keybCol] >> k & 1)) 
			  {
  				if (keyb2CurrentValues[keybCol] >> k & 1){
            pitch = noteOffset + (7-k) + (keybCol *8) + 12*octave;
  				  MidiNoteOn({0x09, 0x91, pitch, 0x00}); // velocity zero = note off
  				}
  				else
  				{
            pitch = noteOffset + (7-k) + (keybCol *8) + 12*octave;
  				  MidiNoteOn({0x09, 0x91, pitch, velocite});
  				}
			  }
			}		  
	  }*/

  	keyb1LastValues[keybCol] =  keyb1CurrentValues[keybCol];
  	//keyb2LastValues[keybCol] =  keyb2CurrentValues[keybCol];
  }
}
