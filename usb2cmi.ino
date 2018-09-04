/* =======================================================================================
    USB Keyboard & Mouse Interface for Fairlight CMI Series III
    (Keyboard interface may also be used with Series I / II / IIX)

    Teensy 3.6 used as core of interface.
    Note that you *must* use a Teensy 3.6!

    Copyright 2018, Joe Britt (britt@shapetable.com)
    
    Permission is hereby granted, free of charge, to any person obtaining a copy of this software and 
    associated documentation files (the "Software"), to deal in the Software without restriction, 
    including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
    and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, 
    subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included in all copies or substantial 
    portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT 
    NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

    USB code based on public domain example "Mouse" from USBHost_t36 by Paul Stoffregren / PJRC.
 */


/* ---------------------------------------------------------------------------------------
    Hardware Specifics
 */
 
#define CMI_SERIAL      Serial1           // to/from CMI
#define KEYBD_SERIAL    Serial2           // to/from legacy keyboard

#define EXP_SERIAL      Serial3

#define MIDI_SERIAL     Serial4

#define MODE_SEL        22                // input selects Series I/II/IIX or III mode

#define R_LED           16                // activity LED
#define G_LED           17

/* ---------------------------------------------------------------------------------------
    DL1416-style display
    Connected via 23017 i2c port expander
 */

#include <i2c_t3.h>

#define IO_EXP_RST      12    // pull low to reset, wait, then let high

#define I2C_SDA_PIN     38    // SDA1 / SCL1
#define I2C_SCL_PIN     37

#define IO_EXP_ADDR     0x20  // all 3 external addr lines tied low

void init_led_display_exp();

void blank_led_display();
void putc_led_display( char c );
void led_display_string( char *s );


/* ---------------------------------------------------------------------------------------
    M I D I
    Stuff
 */

#include <MIDI.h>

MIDI_CREATE_INSTANCE(HardwareSerial, MIDI_SERIAL, MIDI);
const int channel = 1;

//#define MIDI_CHANNEL    MIDI_CHANNEL_OMNI
#define MIDI_CHANNEL    1

/* ---------------------------------------------------------------------------------------
    U S B
    Stuff
 */
 
#include "USBHost_t36.h"

USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);
KeyboardController keyboard1(myusb);
KeyboardController keyboard2(myusb);
USBHIDParser hid1(myusb);
MouseController mouse1(myusb);
uint32_t buttons_prev = 0;

USBDriver *drivers[] = { &hub1, &hub2, &keyboard1, &keyboard2, &hid1 };

#define CNT_DEVICES (sizeof(drivers)/sizeof(drivers[0]))

const char * driver_names[CNT_DEVICES] = { "Hub1","Hub2", "KB1", "KB2", "HID1" };
bool driver_active[CNT_DEVICES] = { false, false, false, false };

USBHIDInput *hiddrivers[] = { &mouse1 };

#define CNT_HIDDEVICES (sizeof(hiddrivers)/sizeof(hiddrivers[0]))

const char * hid_driver_names[CNT_DEVICES] = { "Mouse1" };
bool hid_driver_active[CNT_DEVICES] = { false, false };
bool show_changed_only = false; 

// Turn these on to see internal state as keys are pressed
#if 0
  #define DEBUG(_x)         {Serial.print(_x);}
  #define DEBUG_HEX(_x)     {Serial.print(_x,HEX);}
#else
  #define DEBUG(_x)
  #define DEBUG_HEX(_x)
#endif

char OEMKey() {
  char oem;
  
  if (keyboard1) {
    oem = keyboard1.getOemKey();
  } else {
    oem = keyboard2.getOemKey();
  }

  return oem;
}

void OnRelease( int key ) {
  clear_cmi_key();
}

void OnPress( int key )
{
  char cmi_val = key;                                                 // assume it is an alpha/num/symbol
  
  DEBUG("key '");
  switch (key) {
    case KEYD_UP:         cmi_val = 0x1c;   DEBUG("UP");    break;    // CMI up arrow
    case KEYD_DOWN:       cmi_val = 0x1d;   DEBUG("DN");    break;    // CMI down arrow
    case KEYD_LEFT:       cmi_val = 0x1f;   DEBUG("LEFT");  break;    // CMI left arrow
    case KEYD_RIGHT:      cmi_val = 0x1e;   DEBUG("RIGHT"); break;    // CMI right arrow
    
    case KEYD_INSERT:                       DEBUG("Ins");   break;    //
    case KEYD_DELETE:     cmi_val = 0x0b;   DEBUG("Del");   break;    // CMI sub
    
    case KEYD_PAGE_UP:    cmi_val = 0x0e;   DEBUG("PUP");   break;    // CMI add
    case KEYD_PAGE_DOWN:  cmi_val = 0x19;   DEBUG("PDN");   break;    // CMI set
    
    case KEYD_HOME:       cmi_val = 0x18;   DEBUG("HOME");  break;    // CMI home
    case KEYD_END:        cmi_val = 0x0c;   DEBUG("END");   break;    // CMI clear

    case 0x0a:            cmi_val = 0x0d;   DEBUG("ENTER"); break;    // CMI return
    
    case KEYD_F1:         cmi_val = 0x81;   DEBUG("F1");    break;    // CMI F1
    case KEYD_F2:         cmi_val = 0x82;   DEBUG("F2");    break;    // CMI F2
    case KEYD_F3:         cmi_val = 0x83;   DEBUG("F3");    break;    // CMI F3
    case KEYD_F4:         cmi_val = 0x84;   DEBUG("F4");    break;    // CMI F4
    case KEYD_F5:         cmi_val = 0x85 ;  DEBUG("F5");    break;    // CMI F5
    case KEYD_F6:         cmi_val = 0x86;   DEBUG("F6");    break;    // CMI F6
    case KEYD_F7:         cmi_val = 0x87;   DEBUG("F7");    break;    // CMI F7
    case KEYD_F8:         cmi_val = 0x88;   DEBUG("F8");    break;    // CMI F8
    case KEYD_F9:         cmi_val = 0x89;   DEBUG("F9");    break;    // CMI F9
    case KEYD_F10:        cmi_val = 0x8a;   DEBUG("F10");   break;    // CMI F10
    case KEYD_F11:        cmi_val = 0x8b;   DEBUG("F11");   break;    // CMI F11
    case KEYD_F12:        cmi_val = 0x8c;   DEBUG("F12");   break;    // CMI F12

    case 0x00:            switch( OEMKey() ) {
                            case 0x46:      cmi_val = 0x8d;   DEBUG("PrtScr");    break;    // CMI F13
                            case 0x47:      cmi_val = 0x8e;   DEBUG("ScrLk");     break;    // CMI F14
                            case 0x48:      cmi_val = 0x8f;   DEBUG("Pause");     break;    // CMI F15
                            case 0x2a:      cmi_val = 0x7f;   DEBUG("Bksp");      break;    // CMI Rub Out
                          }
                          break;
                          
    default:              DEBUG((char)key); break;
  }
  DEBUG("'  ");
  DEBUG_HEX(key);
  DEBUG(" MOD: ");
  
  if (keyboard1) {
    DEBUG_HEX(keyboard1.getModifiers());
    DEBUG(" OEM: ");
    DEBUG_HEX(keyboard1.getOemKey());
    DEBUG(" LEDS: ");
    DEBUG_HEX(keyboard1.LEDS());
  } else {
    DEBUG_HEX(keyboard2.getModifiers());
    DEBUG(" OEM: ");
    DEBUG_HEX(keyboard2.getOemKey());
    DEBUG(" LEDS: ");
    DEBUG_HEX(keyboard2.LEDS());
  }

  DEBUG("\n");

  if( cmi_val ) {
    send_cmi_key( cmi_val );
  }

  //Serial.print("key ");
  //Serial.print((char)keyboard1.getKey());
  //Serial.print("  ");
  //Serial.print((char)keyboard2.getKey());
  //Serial.println();
}


/* ---------------------------------------------------------------------------------------
    C M I
    Stuff

    The original (I/II/IIX) and III keyboards both send ASCII for keypresses.
    No key-up messages, just simple ASCII sent on key-down.

    The keyboard does to delay-until-repeat and auto-repeat (Typematic), we emulate that behavior.

    The Series III keyboard G-Pad sends 6-byte movement packets, with the first byte always = 0x80.

    The Series III keyboard F1 - F15 keys send key values of 0x81 - 0x8f, respectively.

    The Series I / II / IIX keyboard only sends uppercase ASCII.

    A GPIO input is used to select Series I/II/IIX or Series III mode.
    An LED is blinked when data (keyboard or mouse) is sent to the CMI.
 */

#define ACT_LED         R_LED

int loopcnt;                              // # of passes thru loop() before turning the LED back on
char last_cmi_key;                        // set to the last key char we sent, cleared on key up, used for typematic
bool key_repeating = false;               // if true, we are repeating

#define DELAY_UNTIL_REPEAT    350         // typematic (auto repeat) millisecond delay until repeat
#define REPEAT_TIME           100         // milliseconds between repeat chars

#include <elapsedMillis.h>
elapsedMillis keyTimeElapsed;             // measure elapsed time for key autorepeat (typematic)

bool is_series_3() {
  return ( digitalRead( MODE_SEL ) ? true : false );
}

void send_cmi_char( char c ) {            // send a char that we want to flicker the LED for
    digitalWrite( ACT_LED, false );         // make LED flicker, will be turned back on in main loop
    loopcnt = 10000;  

    if( !is_series_3() )                    // CMI just expects ASCII, no key-ups. Only send lower case to SIII
      c = toupper( c );
  
    DEBUG_HEX( c );
    CMI_SERIAL.write( c );    
}


bool repeating_cmi_key( char c ) {
  if( isprint( c ) || (c == 0x7f) )       // printable char OR rub out / backspace
    return true;
  else
    return false;
}

void clear_cmi_key() {
  last_cmi_key = 0;                       // reset typematic state
  key_repeating = false;
}

void send_cmi_key( char c ) {
  if( (c & 0x80) && !is_series_3() ) {      // if it's an SIII-only key and we are in I/II/IIX mode, don't send it
    return;
  } else {
    send_cmi_char( c );                     // handles flickering status LED and uppercasing if not in SIII mode  
    last_cmi_key = c;
    keyTimeElapsed = 0;                     // start timer for delay-until-repeat
  }
}


#define MAX_GPAD_X    0x3ff
#define MAX_GPAD_Y    0x3ff

short     gpad_x;                         // G-Pad (Series III) cursor position
short     gpad_y;

short     last_gpad_x;                    // previous G-Pad (Series III) cursor position
short     last_gpad_y;                    //  (used to prevent re-sending same position over & over)

bool      show_cursor_gpad = true;
  
bool      mouse_left_button;

/*
  Each mouse movement results in 6 bytes being sent.
  At 9,600 bps, that takes ~1ms/byte
*/

void Send_GPad_Packet() {
  unsigned char b;

  if( is_series_3() ) {                     // only send gpad messages to Series III
    digitalWrite( ACT_LED, false );         // make LED flicker, will be turned back on in main loop
    loopcnt = 3500;  
    
    CMI_SERIAL.write( 0x80 );               // control byte
  
    b = 0xe0;                               // 111p tsc0: p = pen on pad
                                            //            t = touch
                                            //            s = shift key down (keyboard i/f adds)
                                            //        c = ctl key down (keyboard i/f adds)
  
    if ( mouse_left_button )
      b |= 0x08;                            // touch
  
    if ( show_cursor_gpad )
      b |= 0x10;                            // pen on pad
  
    CMI_SERIAL.write( b );
  
    CMI_SERIAL.write( 0xe0 | (gpad_x & 0x001f) );           // low 5 bits
    CMI_SERIAL.write( 0xe0 | ((gpad_x >> 5) & 0x001f ));    // hi 5 bits
  
    CMI_SERIAL.write( 0xe0 | (gpad_y & 0x001f) );           // low 5 bits
    CMI_SERIAL.write( 0xe0 | ((gpad_y >> 5) & 0x001f ));    // hi 5 bits
  }
}


void init_gpad() {
  last_gpad_x = gpad_x = (MAX_GPAD_X / 2);
  last_gpad_y = gpad_y = (MAX_GPAD_Y / 2);

  Send_GPad_Packet();     // get cursor on-screen and centered
}


void cmi_gpad_move( int8_t dx, int8_t dy ) {

  gpad_x += dx;

  if (gpad_x < 0)
    gpad_x = 0;
  else if (gpad_x > MAX_GPAD_X)
    gpad_x = MAX_GPAD_X;
  
  gpad_y -= dy;                       // G-Pad Y axis is reversed

  if (gpad_y < 0)
    gpad_y = 0;
  else if (gpad_y > MAX_GPAD_Y)
    gpad_y = MAX_GPAD_Y;


  Send_GPad_Packet();                 
}


/* ---------------------------------------------------------------------------------------
    midi

    CMI Series I/II/IIX use a proprietary protocol between the music keyboard and the CMI.
    Each event message is 3 bytes long:

    Control 1:    0xD5  0x80  val (80-FF, 80 is lowest)
    Control 2:    0xD5  0x81  val
    Control 3:    0xD5  0x82  val

    Switch 1:     0xD3  0x80  val (80 = UP, FF = DOWN)
    Switch 2:     0xD3  0x81  val

    Music Keys
    ----------

    Lowest note on master KB is F0.
    16 velocity values, from 0xF0 (fastest) to 0xFF (fastest)

    F0            Note on: 0xC1 0x80  vel     /       Note off: 0xC0 0x80 0x80
    F#0                    0xC1 0x81  vel     /                 0xC0 0x81 0x80
    G0                     0xC1 0x82  vel     /                 0xC0 0x82 0x80
    ...

    PITCHBEND controller is mappted to CMI Control 1

    
    XXX TODO:

    - change midi channel from keypad
    - other continuous controllers
    - channelizer for SIII
 */

#define CMI_F0_NOTE_VAL         0x80                // F0 on master keyboard
#define MIDI_F0_NOTE_VAL        0x11
#define CMI_NOTE_DIFF           ( CMI_F0_NOTE_VAL - MIDI_F0_NOTE_VAL )

#define CMI_MIDI_CHANNEL        1

void send_cmi_music_kb( uint8_t c ) {
  CMI_SERIAL.write( c );

  //Serial.print("M: ");
  //Serial.println( c, HEX );
}


void handle_midi() {
  if( is_series_3() ) {                             // SIII just takes MIDI from music kb
    if( MIDI_SERIAL.available() )
      MIDI_SERIAL.write( MIDI_SERIAL.read() );
  }
  else {
    int note, velocity, channel;
    byte type;
    
    if (MIDI.read()) {                                                    // Is there a MIDI message incoming ?
      type = MIDI.getType();
      channel = MIDI.getChannel();

      if( channel == CMI_MIDI_CHANNEL ) {
        switch (type) {
          
          case midi::NoteOn:
            note = MIDI.getData1();
            velocity = MIDI.getData2();
          
            if (velocity > 0) {
              //Serial.println(String("Note On:  ch=") + channel + ", note=" + note + ", velocity=" + velocity);
  
              send_cmi_music_kb( 0xC1 );                                     // note on
              send_cmi_music_kb( note + CMI_NOTE_DIFF );                     // convert MIDI note to CMI note
              send_cmi_music_kb( map( velocity, 0x00, 0x7f, 0xFF, 0xF0 ) );  // CMI has 16 velocity values, reversed sense
              
            } else {
              //Serial.println(String("Note Off: ch=") + channel + ", note=" + note);
  
              send_cmi_music_kb( 0xC0 );                                     // note off
              send_cmi_music_kb( note + CMI_NOTE_DIFF );                     // convert MIDI note to CMI note
              send_cmi_music_kb( 0x80 );                                     // always sent on note off
            
            }
            break;
        
          case midi::NoteOff:
            note = MIDI.getData1();
            velocity = MIDI.getData2();
      
            //Serial.println(String("Note Off: ch=") + channel + ", note=" + note + ", velocity=" + velocity);
  
            send_cmi_music_kb( 0xC0 );                                       // note off
            send_cmi_music_kb( note + CMI_NOTE_DIFF );                       // convert MIDI note to CMI note
            send_cmi_music_kb( 0x80 );                                       // always sent on note off

            break;
  
          case midi::PitchBend:  

            send_cmi_music_kb( 0xD5 );
            send_cmi_music_kb( 0x80 );
            send_cmi_music_kb( map( MIDI.getData2(), 0x00, 0x7F, 0x80, 0xFF ) );
            
            //Serial.print("CC: ");
            //Serial.print( MIDI.getData1(), HEX );
            //Serial.print(" ");
            //Serial.println( MIDI.getData2(), HEX );
            
            break;

   
          default:  

            Serial.print("DEF: ");
            Serial.print( type, HEX );           
            Serial.print(" ");
            Serial.print( MIDI.getData1(), HEX );
            Serial.print(" ");
            Serial.println( MIDI.getData2(), HEX );
            
            break;
        }
      }
    }
  }
}


/* ---------------------------------------------------------------------------------------
    setup & loop
 */

void setup()
{
  // ===============================
  // Set up hardware
  
  pinMode( MODE_SEL, INPUT_PULLUP );
  
  pinMode( R_LED, OUTPUT );
  digitalWrite( R_LED, true );

  pinMode( G_LED, OUTPUT );                 // power on
  digitalWrite( G_LED, false );
  
  // ===============================
  // Set up console (debug) serial
    
  Serial.begin( 115200 );
  delay( 1500 );                            // wait for serial, but don't block if it's not connected
  Serial.println("Welcome to USB Keyboard & Mouse adapter for CMI");

  Serial.print("Series ");
  Serial.print( is_series_3() ? "III" : "I / II / IIX" );
  Serial.println(" mode selected.");
    
  Serial.println(sizeof(USBHub), DEC);

  // ===============================
  // Set up alphanumeric LED display
  
  init_led_display_exp();
  
  led_display_string((char*)"VERSION 1.0 ");
  delay( 1000 );
  led_display_string((char*)"  POWER ON  ");

  // ===============================
  // Set up CMI comms
  
  CMI_SERIAL.begin( 9600 );       // init output port   
  KEYBD_SERIAL.begin( 9600 );     // init legacy keyboard port   

  init_gpad();

  // ===============================
  // Set up USB
  
  myusb.begin();
  
  keyboard1.attachPress(OnPress);
  keyboard1.attachRelease(OnRelease);

  keyboard2.attachPress(OnPress);
  keyboard2.attachRelease(OnRelease);

  // ===============================
  // Set up MIDI
  
  MIDI.begin( MIDI_CHANNEL );
  MIDI_SERIAL.begin( 31250, SERIAL_8N1_TXINV );         // our hardware is inverted on the transmit side only!

  // ===============================
  // Set up keypad I/Os

  init_keypad();
}  


void loop()
{
  // ===============================
  // Update Activity LED
  
  if( loopcnt )
    loopcnt--;
  else
    digitalWrite( ACT_LED, true );

  // ===============================
  // Check for characters received from the CMI

  if( CMI_SERIAL.available() ) {
    char c = CMI_SERIAL.read();
    Serial.print( "LED: ");
    Serial.print( c );
    Serial.print( " " );
    Serial.println( c, HEX );

    KEYBD_SERIAL.write( c );
    delay(1);                             // XXX hack -- without this, legacy music kb led display sometimes shows same char in pos 1 & 2
    putc_led_display( c );
  }

  // ===============================
  // Check for characters received from the legacy keyboard

  if( KEYBD_SERIAL.available() ) {
    char c = KEYBD_SERIAL.read();
    CMI_SERIAL.write( c );

    Serial.print("K: ");                                      // for figuring out what the music KB sends
    Serial.println(c, HEX);
  }
  
  // ===============================
  // Handle Typematic action (key repeat)
  
  if( key_repeating ) {
    if( keyTimeElapsed >= REPEAT_TIME ) {
      send_cmi_key( last_cmi_key );
      keyTimeElapsed = 0;
    }
  } else
    if( repeating_cmi_key(last_cmi_key) ) {                     // is a printing character key held down?
      if( keyTimeElapsed >= DELAY_UNTIL_REPEAT ) {
        key_repeating = true;
        keyTimeElapsed = 0;
      }
    }

  // ===============================
  // Give USB time
  
  myusb.Task();

  for (uint8_t i = 0; i < CNT_DEVICES; i++) {
    if (*drivers[i] != driver_active[i]) {
      if (driver_active[i]) {
        Serial.printf("*** Device %s - disconnected ***\n", driver_names[i]);
        driver_active[i] = false;
      } else {
        Serial.printf("*** Device %s %x:%x - connected ***\n", driver_names[i], drivers[i]->idVendor(), drivers[i]->idProduct());
        driver_active[i] = true;

        const uint8_t *psz = drivers[i]->manufacturer();
        if (psz && *psz) Serial.printf("  manufacturer: %s\n", psz);
        psz = drivers[i]->product();
        if (psz && *psz) Serial.printf("  product: %s\n", psz);
        psz = drivers[i]->serialNumber();
        if (psz && *psz) Serial.printf("  Serial: %s\n", psz);
      }
    }
  }

  for (uint8_t i = 0; i < CNT_HIDDEVICES; i++) {
    if (*hiddrivers[i] != hid_driver_active[i]) {
      if (hid_driver_active[i]) {
        Serial.printf("*** HID Device %s - disconnected ***\n", hid_driver_names[i]);
        hid_driver_active[i] = false;
      } else {
        Serial.printf("*** HID Device %s %x:%x - connected ***\n", hid_driver_names[i], hiddrivers[i]->idVendor(), hiddrivers[i]->idProduct());
        hid_driver_active[i] = true;

        const uint8_t *psz = hiddrivers[i]->manufacturer();
        if (psz && *psz) Serial.printf("  manufacturer: %s\n", psz);
        psz = hiddrivers[i]->product();
        if (psz && *psz) Serial.printf("  product: %s\n", psz);
        psz = hiddrivers[i]->serialNumber();
        if (psz && *psz) Serial.printf("  Serial: %s\n", psz);
      }
    }
  }


  if(mouse1.available()) {
    /*
    Serial.print("Mouse: buttons = ");
    Serial.print(mouse1.getButtons());    
    Serial.print(",  mouseX = ");
    Serial.print(mouse1.getMouseX());
    Serial.print(",  mouseY = ");
    Serial.print(mouse1.getMouseY());
    Serial.print(",  wheel = ");
    Serial.print(mouse1.getWheel());
    Serial.print(",  wheelH = ");
    Serial.print(mouse1.getWheelH());
    Serial.println();
    */

    mouse_left_button = (mouse1.getButtons() & 0x01) ? true : false;

    if( mouse1.getButtons() & 0x02 )
      show_cursor_gpad = !show_cursor_gpad;

    cmi_gpad_move( mouse1.getMouseX(), mouse1.getMouseY() );

    mouse1.mouseDataClear();
  }

  // ===============================
  // Give MIDI time

  handle_midi();

  // ===============================
  // Scan the keypad

  scan_keypad();
}


/* ---------------------------------------------------------------------------------------
    Alphanumeric LED Displays
 */

/*
  23017 i2c port expander -> DL1416 LED display pin mapping

  GPA0      LED_A0
  GPA1      LED_D6
  GPA2      LED_D3
  GPA3      LED_D2
  GPA4      LED_D1
  GPA5      LED_D0
  GPA6      LED_D4
  GPA7      LED_D5

  GPB0      n/a
  GPB1      n/a
  GPB2      /LED_3_CS
  GPB3      /LED_WR
  GPB4      /LED_CU
  GPB5      /LED_2_CS
  GPB6      /LED_1_CS
  GPB7      LED_A1
*/


/* ---------------------------------------------
   MCP23017 i2c port expander registers
   
   IOCON.BANK = 0 (default)
*/

typedef unsigned char uchar;

void exp_wr( uchar addr, uchar val );
uchar exp_rd( uchar addr );

#define IODIRA          0x00      // IO Direction (1 = input, 0xff on reset)
#define IODIRB          0x01
#define IOPOLA          0x02      // IO Polarity (1 = invert, 0x00 on reset)
#define IOPOLB          0x03
#define GPINTENA        0x04      // 1 = enable interrupt on change
#define GPINTENB        0x05
#define DEFVALA         0x06      // default compare values for interrupt on change, 0xoo on reset
#define DEFVALB         0x07
#define INTCONA         0x08      // 1 = pin compared to DEFVAL, 0 = pin compare to previous pin value
#define INTCONB         0x09
#define IOCON           0x0a      // see bit definitions below
//#define IOCON           0x0b    // duplicate
#define GPPUA           0x0c      // 1 = pin pullup enabled, 0x00 on reset
#define GPPUB           0x0d
#define INTFA           0x0e      // interrupt flags, 1 = associated pin caused interrupt
#define INTFB           0x0f
#define INTCAPA         0x10      // interrupt capture, holds GPIO port values when interrupt occurred
#define INTCAPB         0x11
#define GPIOA           0x12      // read: value on port, write: modify OLAT (output latch) register
#define GPIOB           0x13
#define OLATA           0x14      // read: value in OLAT, write: modify OLAT
#define OLATB           0x15

// IOCON bit values

#define IOCON_BANK      0x80      // 1: regs in different banks, 0: regs in same bank (addrs are seq, default)
#define IOCON_MIRROR    0x40      // 1: 2 INT pins are OR'ed, 0: 2 INT pins are independent (default)
#define IOCON_SEQOP     0x20      // 1: sequential ops disabled, addr ptr does NOT increment, 0: addr ptr increments (default)
#define IOCON_ODR       0x04      // 1: INT pin is Open Drain, 0: INT pin is active drive (default)
#define IOCON_INTPOL    0x02      // 1: INT pin is active hi, 0: INT pin is active lo (default)
#define IOCON_INTCC     0x01      // 1: reading INTCAP clears interrupt, 0: reading GPIO reg clears interrupt (default)


void exp_wr( uchar addr, uchar val ) {
  Wire1.beginTransmission( IO_EXP_ADDR );
  Wire1.write( addr );
  Wire1.write( val );
  Wire1.endTransmission();
}

uchar exp_rd( uchar addr ) {
  uchar r;
  
  Wire1.beginTransmission( IO_EXP_ADDR );
  Wire1.write( addr );
  Wire1.endTransmission();  

  Wire1.requestFrom( IO_EXP_ADDR, 1 );
  r = Wire1.read();
  
  return r;
}

void init_led_display_exp() {  
  pinMode( IO_EXP_RST, OUTPUT );      // hard reset the part
  digitalWrite( IO_EXP_RST, 0 );  
  delay( 2 );                         // let it out of reset
  digitalWrite( IO_EXP_RST, 1 );
  
  // crank up i2c
  Wire1.begin();
  Wire1.setOpMode(I2C_OP_MODE_IMM);

  Serial.print("initializing port expander...");  
  
  exp_wr( OLATA,    0xff );           // LED pins hi
  exp_wr( IODIRA,   0x00 );           // all port A pins OUTPUTS

  exp_wr( OLATB,    0xff );
  exp_wr( IODIRB,   0x03 );           // port B[7:3] are OUTPUTS, B[1:0] are INPUTS

  Serial.println("Done!");
}

/*  Take in a byte in normal bit order, return a byte with data bits swizzled as hardware is connected.

       0  0
       D6 1
       D3 2
       D2 3
       D1 4
       D0 5
       D4 6
       D5 7
*/

char led_data_swizzle( char c ) {
  char r = 0;

  r |=  (c & 0x01) << 5;            // 0
  r |=  ((c & 0x02) >> 1) << 4;     // 1
  r |=  ((c & 0x04) >> 2) << 3;     // 2
  r |=  ((c & 0x08) >> 3) << 2;     // 3
  r |=  ((c & 0x10) >> 4) << 6;     // 4
  r |=  ((c & 0x20) >> 5) << 7;     // 5
  r |=  ((c & 0x40) >> 6) << 1;     // 6

  return r;
}


/*  ---------------------------
    Keypad

    Keypad rows are driven with LED Data lines:

    D4 (GPA6) -> Row 0
    D0 (GPA5) -> Row 1
    D1 (GPA4) -> Row 2
    D2 (GPA3) -> Row 3

    2 column bits are read back on the i2c port expander.
    The other 2 column bits are read back on pin 8 () and pin 5 () of the expansion connector.

    Col 0 -> i2c GPB1
    Col 1 -> i2c GPB0
    Col 2 -> exp pin 5 (digital pin 7)
    Col 3 -> exp pin 8 (digital pin 11)

    To scan:

    >>> Put this after potential LED update in loop()

    0. initialize drive_count_loops = 0
    1. Drive current row low, others high (LED data)
    2. drive_count_loops++;
    3. drive_count_loops >= 3?
       YES -> sample columns, XOR to see what changed, send char on key downs

*/

#define ROW_0_LOW     0b10111111      // GPA6
#define ROW_1_LOW     0b11011111      // GPA5
#define ROW_2_LOW     0b11101111      // GPA4
#define ROW_3_LOW     0b11110111      // GPA3

#define ROW_LOOPS     5000            // still very responsive, but enough time for weak pullups in the 23017 & to debounce

int drive_count_loops;
int cur_row;                          // row we are scanning now. goes from 0->3->0

char matrix_cur[4];                   // current state, only low 4 bits of each byte are used
char matrix_last[4];                  // last state, use xor to figure out if any changed

// key map tables, [row][col]

char keymap[4][4] = { { '1', '4', '7', '*' },
                      { '2', '5', '8', '0' },
                      { '3', '6', '9', '#' },
                      { 'A', 'B', 'C', 'D' } };


// drive one row low. -1 = all high.

void drive_row( int r ) {
  switch( r ) {
    case 0:   exp_wr( GPIOA, ROW_0_LOW );     break;
    case 1:   exp_wr( GPIOA, ROW_1_LOW );     break;
    case 2:   exp_wr( GPIOA, ROW_2_LOW );     break;
    case 3:   exp_wr( GPIOA, ROW_3_LOW );     break;
    default:  exp_wr( GPIOA, 0xff );          break;
  }
}

// return true (read a 0) or false (read a 1)

bool read_col( int c ) {
  bool r = false;
  
  switch( c ) {
    case 0:     r = (exp_rd( GPIOB ) & 0b00000010);     break;
    case 1:     r = (exp_rd( GPIOB ) & 0b00000001);     break;
    case 2:     r = digitalRead( 7 );                   break;
    case 3:     r = digitalRead( 11 );                  break;
  }

  return r;
}


void keypad_col_pullups_enable( bool en ) {
  if( en ) {
    exp_wr( GPPUB,   0x03 );           // port B[1:0] INPUTS have pullups ENABLED
    pinMode( 7,   INPUT_PULLUP );
    pinMode( 11,  INPUT_PULLUP );
  } else {
    exp_wr( GPPUB,   0x00 );           // port B[1:0] INPUTS have pullups ENABLED
    pinMode( 7,   INPUT );
    pinMode( 11,  INPUT );
  }
}


void init_keypad() {
  Serial.print("initializing keypad...");
  
  keypad_col_pullups_enable( false );

  drive_count_loops = 0;
  cur_row = 0;

  matrix_cur[0] = 0;
  matrix_cur[1] = 0;
  matrix_cur[2] = 0;
  matrix_cur[3] = 0;

  matrix_last[0] = 0;
  matrix_last[1] = 0;
  matrix_last[2] = 0;
  matrix_last[3] = 0;
  
  Serial.println("done!");
}


/*  LED data lines are used as the row drivers, so some caution must be used when scanning the keypad.

    We need the column inputs to normally be just inputs (no pullups) so they don't interfere with the data lines for
     real LED display writes.
     
    But to do a scan, we need to drive a row (data line) low, the others high, wait, and sample the column inputs.
    But we don't want to just spinwait after driving the row. That would introduce latency for handling incoming real-time
     (MIDI, keyboard) events.

    So, the LED and keypad code have to coordinate. Most of the time we are not writing to the LED displays. So, the common
     path through loop() is:

    1. drive a row
    2. wait some number of loop()s
    3. sample the columns for that row
    4. update keypad state change bitmaps
    5. send chars if we just saw key downs (no key up events)
    6. next row, goto 1

    This is driven with a state machine. If an LED write happens, the state machine is reset.

    We implement this interlock/reset at the low level (here), so that otehr implementations without this hardware
     restriction don't have to worry about it.
*/

#define KEY_STATE_IDLE                0           // keypad scan state machine states
#define KEY_STATE_DRIVE_ROW           1
#define KEY_STATE_WAIT_LOOP           2
#define KEY_STATE_SAMPLE_COLS         3

int keyscan_state = KEY_STATE_IDLE;

int scan_row;                                     // row currently driving low


void keypad_send( char c ) {                      // send keypad keypresses to CMI
  Serial.print("keypad: ");
  Serial.println( c );
  send_cmi_char( c );                             // no autorepeat, handle flickering status LED
}


bool went_down( int row, char mask ) {
  return( ((matrix_cur[row] ^ matrix_last[row]) & mask) && (matrix_cur[row] & mask) );          // changed and is now down?
}


void scan_keypad() {
  
  switch( keyscan_state ) {
    case KEY_STATE_IDLE:          keypad_col_pullups_enable( true );                            // turn on pullups for the scan. LED code may turn off.
                                  drive_count_loops = 0;
                                  cur_row++;
                                  if( cur_row == 4 )
                                    cur_row = 0;
                                  keyscan_state = KEY_STATE_DRIVE_ROW;
                                  break;
                                  
    case KEY_STATE_DRIVE_ROW:     drive_row( cur_row ); 
                                  keyscan_state = KEY_STATE_WAIT_LOOP;
                                  break;                            
                                  
    case KEY_STATE_WAIT_LOOP:     if( drive_count_loops++ > ROW_LOOPS )
                                    keyscan_state = KEY_STATE_SAMPLE_COLS;
                                  break;
                                  
    case KEY_STATE_SAMPLE_COLS:   matrix_last[cur_row] = matrix_cur[cur_row];                   // remember what they were
    
                                  matrix_cur[cur_row] =  ( read_col( 0 ) ? (0<<0) : (1<<0) );   // see what they are now
                                  matrix_cur[cur_row] |= ( read_col( 1 ) ? (0<<1) : (1<<1) );
                                  matrix_cur[cur_row] |= ( read_col( 2 ) ? (0<<2) : (1<<2) );
                                  matrix_cur[cur_row] |= ( read_col( 3 ) ? (0<<3) : (1<<3) );
                         
                                  // send ASCII for any keys that are down. no autorepeat.
                                  
                                  if( went_down( cur_row, 0x01 ) ) keypad_send( keymap[cur_row][0] );
                                  if( went_down( cur_row, 0x02 ) ) keypad_send( keymap[cur_row][1] );
                                  if( went_down( cur_row, 0x04 ) ) keypad_send( keymap[cur_row][2] );
                                  if( went_down( cur_row, 0x08 ) ) keypad_send( keymap[cur_row][3] );
                                  
                                  keyscan_state = KEY_STATE_IDLE;
                                  break;  
  }
}



void reset_keypad_scan() {
  keypad_col_pullups_enable( false );       // pullups off so they don't interfere with LED data
  keyscan_state = KEY_STATE_IDLE;           // reset the scanner, will continue after LED write is done
}



void put_led_char( int pos, char c ) {
  uchar portA;
  uchar portB;
  uchar portB_CS_sel;

  reset_keypad_scan();                      // make sure the keypad scanner doesn't get in our way
  
  pos ^= 0x03;
  
  // ---------------------------
  // Set up the data bus

  portA = led_data_swizzle( c );
  portA |= (pos & 0x01);                    // get LED_A0 into bit 0 of GPIOA
  exp_wr( GPIOA,    portA);                 // set up swizzled data and LED_A0

  // ---------------------------
  // figure out which chip select                                
  
  if( pos < 4 )
    portB_CS_sel = ~0x04;
  else
    if( pos < 8 )
      portB_CS_sel = ~0x20;
    else
      portB_CS_sel = ~0x40;

  // ---------------------------
  // run a bus cycle to write the swizzled char data
  
  portB = 0x7c;                             // A1 = 0, CU = 1, WR = 1, CS = 1
  portB |= ((pos & 0x02) << 6);             // get LED_A1 into bit 7 of GPIOB
  
  exp_wr( GPIOB,    portB );                // A1 = LED_A1, CU =1, WR = 1, CS = 1

  portB &= portB_CS_sel;                    // drop the right /CS
  exp_wr( GPIOB,    portB );                // A1 = LED_A1, CU = 1, WR = 1, CS = 0

  portB &= 0xf4;                            // drop /WR
  exp_wr( GPIOB,    portB );                // A1 = LED_A1, CU = 1, WR = 0, CS = 0
  
  exp_wr( GPIOB,    0xff );                 // A1 = 1, CU = 1, WR = 1, CS = 1 
}


int curpos = 0;

void putc_led_display( char c ) {
  if( isprint( c ) ) {
    if( curpos == 12 )
      curpos = 0;

    put_led_char( curpos, c );
    curpos++;
  } else
    if( (c == 0x0a) || (c == 0x0d) ) {
      curpos = 0;
    }

  //Serial.println(curpos);
}

void blank_led_display() {
  putc_led_display( 0x0d );
  for( int xxx = 0; xxx != 12; xxx++ )
    putc_led_display( ' ' );  
}

void led_display_string( char *s ) {
  blank_led_display();
  for( int xxx = 0; xxx != 12; xxx++ ) {
    if( s[xxx] )
      putc_led_display( s[xxx] );
    else
      break;
  }
}
