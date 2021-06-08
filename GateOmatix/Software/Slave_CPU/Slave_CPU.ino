/*
  =====================================================
  Gate sequencer by Jean_k<3

  Update 08 JAN 2021

  64 step sequencer, 8 part compatible eurorack
  =====================================================

  ________________________________________________________________________________________________
  ________________________________________________________________________________________________
  ________________________________________________________________________________________________

  TODO :

  SOFTWARE

  Clock multiplier/diviser per part
  Detection Clock interne/externe

  Reset -> interupt
  Clock ext -> interupt

  Choix de la paterne (shift + page nav)

  Fonction save/Load (SD card)

  HARDWARE

  Checker si un jack est connecter ou non
  Carte SD
  Alim 12v
  transistorisation des entrées/sorties


  DONE :

  Encodeur Tempo
  Afficheur 7 segments
  Gestion des LEDs
  Matrice de boutons
  Fonction de choix de la part

  Ajout du selecteur
  Fonction de choix de la part
  Fonction de gestion du nombre de pas
  Fonction Pages navigation
  Step ON-OFF
  Fonction sens de lecture
  Fonction Fill/Clear
  Fonction Copy/Past
  Lecture des bouttons de fonctions
  Fonction pause-lecture/stop
  Fonction de gestion de la taille de la gate
  Fonction Step jump
  Reset

  ________________________________________________________________________________________________
  ________________________________________________________________________________________________
  ________________________________________________________________________________________________


  FUNCTION :

  Bouttons
    Shift + step button = step jump
    Fill + shift = Clear
    Copy/Paste + shift = Save
    Pause/Play + shift = Stop

   Encoder
    Tempo + push = clock divider
    Gate length + push = Number of step
    Page navigation + push = Forward/Backward/Random

   Selecter
    Part choice + shift = patern choice

  --------------------------------------------------------------------------

  ANALOGUE PIN
  1 Selecteur pour les parts
  3 encoder pour le tempo/clock divider, gate/step, page/nombre de page

  DIGITAL PIN

  jack : 12
    output
      Clock out
      Reset out
      8 Gate out

     input
        Clock in
        Reset in

  Boutons : 24
    16 bouttons de pas
    5 boutton pour Shift, Copy-Paste, Save/Load, Fill/Clear, Play-Pause/Stop
    3 boutton d'encoder
  (Matrice de boutons 5x5 = 25 boutons disponible)

  Leds : 43
    16 leds de pas
    16 leds timeline
    3 leds de fonctions (une par encoder pour savoir sa fonction actuelle)
    1 led pour le BPM
    Afficheur 3x7 segments 11 pin (7 chathodes+ / 3 anodes-)
  (Matrice de LED en 6*6 = 36 LEDs)
  ( + 7 pour l'afficheur)


*/

#define ALL_STEP 64
#define ALL_PART 8

//---------------------Encoder 1 General/Patern--------------------------

int count;

#define general_SW_pin 2
#define general_inputCLK 3
#define general_inputDT 4
#define general_led_pin A7

bool general_currentStateCLK;
bool general_previousStateCLK;
bool general_SW_state;
bool last_general_SW_state;
bool general = HIGH;

byte which_parameter;
char*  general_param[4] {"PAG", "BPn", "DEL", "SnG"};

byte which_mode;
char* mode[2] {"pat","nut"};

//---------------------Encoder 2 Per Part/Per Step--------------------------
#define per_part_SW_pin A6
#define per_part_inputCLK A4
#define per_part_inputDT A5
#define per_part_led_pin A3

bool per_part_currentStateCLK;
bool per_part_previousStateCLK;
bool per_part_SW_state;
bool last_per_part_SW_state;
bool per_part;
bool per_part_led_state;

byte which_part_param;
char*  part_param[5] {"nbs", "uAY", "ist", "CLO", "gsi"};

byte which_step_param;
char*  step_param[3] {"gsi", "rat", "pby"};

//---------------------Encoder 3 DATA--------------------------
#define data_SW_pin 36
#define data_inputCLK 34
#define data_inputDT 35
#define data_led_pin 37

bool data_currentStateCLK;
bool data_previousStateCLK;
bool data_SW_state;
bool last_data_SW_state;
bool data_led_state;

bool validate;

byte page_pos;
byte BPM;
byte delays;
byte swing;

byte gate_rate[ALL_PART];
byte number_of_step[ALL_PART];
byte which_way;
byte clock_modulator[ALL_PART];
bool inverted_state[ALL_PART];

byte probability[ALL_STEP];
byte racheting[ALL_STEP][4]; // 4 mode de racheting
byte step_gate_rate[ALL_STEP];

bool ping[ALL_PART] = {1};

//---------------------I2C------------------------------
#include <Wire.h>
#include <I2C_Anything.h>
bool changed = false;

//---------------------DISPLAY--------------------------
#include "SevSeg.h"
SevSeg sevseg;
byte to_print[6];
byte which_encoder;
char*  my_string[5] {"frd", "brd", "png", "rnd", "bro"};

//---------------------Selector--------------------------
#define selectorPin A0
const byte gate_out_pin[ALL_PART] = {A8, A9, A10, A11, A12, A13, A14, A15};

const float divider = 146.00;
byte selected_part;
unsigned long previous_time;

//---------------------Le temps--------------------------
bool clock;
bool play;

#define reset_pin 8
bool reset_state;
bool last_reset_state;
unsigned long lastTime;

bool step_to_jump [ALL_STEP];

unsigned long step_start;
float step_length;

int gate_length[ALL_PART];

unsigned long lastDebounceTime = 0;   // For the encoders buttons


//---------------------Buttons Matrix--------------------------
#include <Keypad.h>
#define shift_pin A2
const byte ROWS = 4; //four rows
const byte COLS = 5; //five columns
const byte rowPins[ROWS] = {42, 43, 44, 45}; //connect to the row pinouts of the keypad
const byte colPins[COLS] = {49, 50, 51, 52, 53}; //connect to the column pinouts of the keypad

bool step_state[ALL_PART][ALL_STEP] = {0};
byte which_step[7] = {0}; // To save the last changed step and send it via I2C

bool copied_state[ALL_STEP] = {0};
byte actual_step[ALL_PART];

char keys[ROWS][COLS] = {
  {'a', 'b', 'c', 'd', '1'},
  {'e', 'f', 'g', 'h', '2'},
  {'i', 'j', 'k', 'l', '3'},
  {'m', 'n', 'o', 'p', '4'}
};
Keypad kpd = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
byte my_key;

bool shift = false;
bool pause = false;
bool stop = false;
bool copy = false;
bool fill = false;
bool save = false;

//---------------------Step Led--------------------------
const int verrou = 11;    //LATCH PIN
const int horloge = 12;   //CLOCK PIN
const int data = 10;      //DATA  PIN

//---------------------Tempo Led--------------------------
const int verrou2 = 6;    //LATCH PIN
const int horloge2 = 5;   //CLOCK PIN
const int data2 = 7;      //DATA  PIN
unsigned long previous;   // Blink tempo LED if step_jump

//
//============================================================================== HANDLE BUTTONS
//
void Handle_buttons () {

  for (byte i = 0; i < 7; ++i) {
  }

  if (kpd.getKeys()) // Si un boutton bouge
  {
    for (int i = 0; i < LIST_MAX; i++) // on stock son numéro dans une liste
    {
      if (kpd.key[i].stateChanged) { // selon quelle boutton bouge
        my_key = kpd.key[i].kchar;

        switch (kpd.key[i].kstate) {  // Report active key state : IDLE, PRESSED, HOLD, or RELEASED
          case PRESSED:

            if (my_key > 96 && my_key < 113) {

              if (shift == false) {

                changed = true;
                step_state[selected_part][my_key - 97 + page_pos * 16] = !step_state[selected_part][my_key - 97 + page_pos * 16];

                if (i < 6) {
                  which_step[i] = (my_key - 97) + page_pos * 16; // the 6 last step who has changed
                }


              } else if (shift == true) { // jump the step
                step_to_jump[my_key - 97 + page_pos * 16] = !step_to_jump[my_key - 97 + page_pos * 16];
              }
            }
            switch (my_key) {
              case 49 :
                pause = !pause;
                stop = true;
                break;
              case 50 :
                copy = true;
                break;
              case 51 :
                fill = true;
                break;
              case 52 :
                save = true;
                break;
            }
            break;

          case RELEASED :
            switch (my_key) {
              case 49 :
                stop = false;
                break;
              case 50 :
                copy = false;
                break;
              case 51 :
                fill = false;
                break;
              case 52 :
                save = false;
                break;
            }
            break;
        }
      }
    }
  }


  shift = !digitalRead(shift_pin);

  if (!shift) { //_________________ sans le SHIFT

    if (!pause) {
      //Serial.println("play");
      play = true;
    } else if (pause) {
      //Serial.println("paused");
      play = false;
    }

    if (copy)
    {
      //Serial.println("Copied");
      for (int i = 0; i < ALL_STEP; ++i) {
        copied_state[i] = step_state[selected_part][i];              // On copie les états de la voie actuelle
      }
    }

    if (fill) {
      //Serial.println("Filled");
      step_state[selected_part][actual_step[selected_part]] = 1;            // On copie les états de la voie actuelle
    }

    if (save) {
      //Serial.println("Saved");
    }

  } else if (shift) { //_________________ Avec le SHIFT

    if (stop) {
      //Serial.println("stoped");
      play = false;
      pause = true;
      for (int i = 0; i < ALL_PART; ++i) {
        actual_step[i] = 0;
      }
    }

    if (copy)
    {
      //Serial.println("pasted");
      for (int i = 0; i < ALL_STEP; ++i)
      {
        step_state[selected_part][i] = copied_state[i];              // On colle les états de la voie actuelle
      }
    }

    if (fill) {
      //Serial.println("cleared");
      for (int i = 0; i < ALL_STEP; ++ i) {
        step_state[selected_part][i] = 0;
      }
    }

    if (save) {
      //Serial.println("loaded");
    }
  }
}
//
//============================================================================== UPDATE LEDS
//
void update_LEDs() {

  bool tempo_led[ALL_STEP];

  unsigned long current = millis();

  for (int i = 0; i < 16; ++i) {
    if (i + page_pos * 16 == actual_step[selected_part]) {
      tempo_led[i] = true;
    } else {
      tempo_led[i] = false;
    }
  }

  if (current - previous >= 100) {
    previous = current;

    for (int i = 0; i < 16; ++i) {
      if (step_to_jump[i + page_pos * 16]) {
        tempo_led[i] = !tempo_led[i];
      }
    }
  }

  digitalWrite(verrou, LOW);
  digitalWrite(verrou2, LOW);

  for (int i = 16; i >= 0; --i) {
    digitalWrite(horloge, LOW);
    digitalWrite(horloge2, LOW);
    digitalWrite(data, tempo_led[i]);
    digitalWrite(data2, step_state[selected_part][i + page_pos * 16]);
    digitalWrite(horloge, HIGH);
    digitalWrite(horloge2, HIGH);
  }
  digitalWrite(verrou, HIGH);
  digitalWrite(verrou2, HIGH);
}
//
//============================================================================== UPDATE SCREEN
//
void update_screen() {
  
  if (which_encoder == 0) {       // General parameter
    sevseg.setChars(general_param[which_parameter/2]);
  } else if (which_encoder == 1){ // Patern
    sevseg.setChars(mode[which_mode]);
  }else if (which_encoder == 2){  // Per part
    sevseg.setChars(part_param[which_part_param/2]);
  }else if (which_encoder == 3){  // Per step
        sevseg.setChars(step_param[which_step_param/2]);
  }else if (which_encoder == 4){  // Data
    
  }
  sevseg.refreshDisplay();

  //Serial.println(which_encoder);
  // 0 = BPM
  // 1 = Clock Divider
  // 2 = Gate length
  // 3 = number of Step
  // 4 = page navigation
  // 5 = Sens de lecture

  //Serial.println(which_way);
  // 0 = Frd forward
  // 1 = Brd Backward
  // 2 = Png Ping Pong
  // 3 = rnd Random
  // 4 = Bro
}
//
//============================================================================== RESET
//
void reset() {

  int reading = digitalRead(reset_pin);
  if (reading != last_reset_state) {
    lastTime = millis();
  }

  if ((millis() - lastTime) > 5) {

    if (reading != reset_state) {
      reset_state = reading;


      if (reset_state == HIGH) {
        for (int i = 0; i < ALL_PART; ++i) {
          actual_step[i] = 0;
        }
      }
    }
  }
  last_reset_state = reading;
}
//
//============================================================================== NEXT STEP
//
void next_step() {

  switch (which_way) {

    case 0 : // forward

      for (int i = 0; i < ALL_PART; ++i) {

        ++actual_step[i];

        while (step_to_jump[actual_step[i]]) {
          ++actual_step[i];
        }


        if (actual_step[i] > number_of_step[i] - 1) {
          actual_step[i] = 0;
          while (step_to_jump[actual_step[i]]) {
            ++actual_step[i];
          }
        }
      }
      break;

    case 1 : // backward
      for (int i = 0; i < ALL_PART; ++i) {
        --actual_step[i];

        while (step_to_jump[actual_step[i]]) {
          --actual_step[i];
        }


        if (actual_step[i] > 65) {
          actual_step[i] = number_of_step[i] - 1;
          while (step_to_jump[actual_step[i]]) {
            --actual_step[i];
          }
        }
      }
      break;

    case 2 : // ping pong

      byte last_valid_step[ALL_PART];

      for (int i = 0; i < ALL_PART; ++i) {

        if (ping[i]) {
          ++actual_step[i];
          last_valid_step[i] = actual_step[i];

          while (step_to_jump[actual_step[i]]) {
            ++actual_step[i];
            if (actual_step[i] > number_of_step[i] - 1) {
              actual_step[i] = last_valid_step[i] - 2;
              ping[i] = false;
            }
          }

          if (actual_step[i] >= number_of_step[i] - 1) {
            ping[i] = false;
          }

        } else {
          --actual_step[i];
          last_valid_step[i] = actual_step[i];

          while (step_to_jump[actual_step[i]]) {
            --actual_step[i];
            if (actual_step[i] > 65) {
              actual_step[i] = last_valid_step[i] + 2;
              ping[i] = true;
            }
          }

          if (actual_step[i] > 65) {
            ping[i] = true;
          }
        }
      }
      break;

    case 3 :  // random

      for (int i = 0; i < ALL_PART; ++i) {

        byte my_try = random(number_of_step[i]);

        while (step_to_jump[my_try]) {
          my_try = random(number_of_step[i]);
        }
        actual_step[i] = my_try;
      }
      break;

    case 4 : // brownien

      for (int i = 0; i < ALL_PART; ++i) {

        byte proba = random(100);
        byte my_try = random(100);

        if (proba > my_try) {
          ++actual_step[i];

          while (step_to_jump[actual_step[i]]) {
            ++actual_step[i];
          }


          if (actual_step[i] > number_of_step[i] - 1) {
            actual_step[i] = 0;
            while (step_to_jump[actual_step[i]]) {
              ++actual_step[i];
            }
          }
        }
      }
      break;
  }
  step_start = millis();
}
//
//============================================================================== SETUP
//

void clock_ISR() {
  clock = HIGH;
}

void setup() {

  Serial.begin(115200);

  Wire.begin(1);                // join i2c bus with address #8
  Wire.onRequest(requestEvent); // register event

  attachInterrupt(digitalPinToInterrupt(19), clock_ISR, RISING);

  byte numDigits = 3;
  byte digitPins[] = {22, 23, 24}; // 1 2 3
  byte segmentPins[] = {25, 26, 27, 28, 29, 30, 31}; // A B C D E F G
  byte hardwareConfig = COMMON_CATHODE; // See README.md for options
  bool disableDecPoint = true; // Use 'true' if your decimal point doesn't exist or isn't connected

  sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins, disableDecPoint);
  sevseg.setBrightness(100);

  pinMode(verrou, OUTPUT);
  pinMode(horloge, OUTPUT);
  pinMode(data, OUTPUT);

  pinMode(verrou2, OUTPUT);
  pinMode(horloge2, OUTPUT);
  pinMode(data2, OUTPUT);

  pinMode(general_SW_pin, INPUT_PULLUP);
  pinMode(general_led_pin, OUTPUT);
  pinMode (general_inputCLK, INPUT);
  pinMode (general_inputDT, INPUT);

  pinMode(per_part_SW_pin, INPUT_PULLUP);
  pinMode(per_part_led_pin, OUTPUT);
  pinMode (per_part_inputCLK, INPUT);
  pinMode (per_part_inputDT, INPUT);

  pinMode(data_SW_pin, INPUT_PULLUP);
  pinMode(data_led_pin, OUTPUT);
  pinMode (data_inputCLK, INPUT);
  pinMode (data_inputDT, INPUT);

  general_previousStateCLK = digitalRead(general_inputCLK);
  per_part_previousStateCLK = digitalRead(per_part_inputCLK);
  data_previousStateCLK = digitalRead(data_inputCLK);

  for (int i = 0; i < ALL_PART; ++i) {
    pinMode(gate_out_pin[i], OUTPUT);
  }

  pinMode(shift_pin, INPUT_PULLUP);
  pinMode(reset_pin, INPUT);

  pause = true;                     // Sur pause par défaut
}
//
//==============================================================================
//
void loop() {

  unsigned long current_time = millis();
  if (current_time - previous_time >= 100UL) { // check for the selector every 100ms
    previous_time = current_time;
    selected_part = round(analogRead(selectorPin) / divider);
  }

  Handle_buttons();
  Handle_encoder();
  update_screen();
  update_LEDs();

  reset();

  if (play) {
    if (clock == HIGH) {
      next_step();
      clock = LOW;
    }
  }
}

void requestEvent() {

  Wire.write(gate_rate, 8);         // 8 bytes
  Wire.write(actual_step, 8);       // 8 bytes
  Wire.write(which_step, 6);        // 6 bytes (the last modified step)
  Wire.write(selected_part);        // 1 byte
  Wire.write(changed);              // 1 byte
  Wire.write(BPM);                  // 1 byte
  //  Wire.write(clockDivider);         // 1 byte

  //                           Total : 26 bytes

  changed = false;
  for (byte i = 0; i < 6; ++i) {
    which_step[i] = 88;
  }

}
