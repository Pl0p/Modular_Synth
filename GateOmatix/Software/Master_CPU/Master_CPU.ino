#include <Wire.h>
#include <I2C_Anything.h>

#define ALL_PART 8
#define ALL_STEP 64
#define DEBUG false

const byte gate_out_pin[ALL_PART] = {4, 5, 6, 7, 8, 9, 10, 11};

int gate_length[ALL_PART];
bool in_time[ALL_PART];

unsigned long step_start = 0;

bool step_state[ALL_PART][ALL_STEP] = {0};
bool copied_state[ALL_STEP] = {0};

// from the slave
byte gate_rate[ALL_PART] = {0};                 // 8 bytes
byte actual_step[ALL_PART] = {0} ;              // 8 bytes
byte which_step[7] = {0};                       // 6 byte
byte selected_part = 0;                         // 1 byte
bool changed = false;                           // 1 byte
int BPM = 120;                                  // 4 byte
byte which_clock = 17;                          // 1 byte

bool shift;                                     // 1 byte
bool copy;                                      // 1 byte
bool fill;                                      // 1 byte
bool save;                                      // 1 byte
//                                        Total = 32 bytes
float step_length = 0;
float clock_modulator[35] = {0.0625, 0.0666, 0.0714, 0.0769, 0.0833, 0.0909, 0.1, 0.1111, 0.125, 0.1428, 0.1666, 0.2, 0.25, 0.3333, 0.4, 0.5, 0.6666, 1, 1.5, 2, 2.5, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
//
//============================================================================== FUNCTIONS
//
void handle_function() {
  if (!shift) { //_________________ sans le SHIFT

    if (copy)
    {
      //Serial.println("Copied");
      for (int i = 0; i < ALL_STEP; ++i) {
        copied_state[i] = step_state[selected_part][i];              // On copie les états de la voie actuelle
      }
    }

    if (fill) {
      //Serial.println("Filled");
      step_state[selected_part][actual_step[selected_part]] = 1;     // On remplie tous les pas de la voie
    }

    if (save) {
      //Serial.println("Saved");
    }

  } else if (shift) { //_________________ Avec le SHIFT

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
//============================================================================== CLOCK
//
void the_clock() {

  const float X = 60000.0;
  float Y = BPM; // convert BPM in a float

  step_length = (X / Y) * clock_modulator[which_clock];

  if (millis() - step_start >= step_length)  // Si on dépasse la taille du STEP
  {
    step_start = millis();
    digitalWrite(3, HIGH);
    for (byte i = 0; i < ALL_PART; ++i) {
      in_time[i] = true;
    }
  }
  digitalWrite(3, LOW);
}
//
//============================================================================== SETUP
//
void setup() {

  Wire.begin();        // join i2c bus (address optional for master)
  Serial.begin(115200);  // start serial for output

  pinMode(3, OUTPUT);

  for (byte i = 0; i < ALL_PART; ++i) {
    pinMode(gate_out_pin[i], OUTPUT);
  }
}
//
//============================================================================== LOOP
//
void loop() {

  request_data();                       // Get the data from the slave

  if (changed) {                        // If a step_state has changed, update it (slave send which_step) or send 88 if any
    for (byte i = 0; i < 6; ++i) {      // Slave send 6 or less address to update

      if (which_step[i] <= 64) {        // If it's 88 = nothing changed
        step_state[selected_part][which_step[i]] = !step_state[selected_part][which_step[i]];
      }
    }
  }


  for (int i = 0; i < ALL_PART; ++i) {

    gate_length[i] = gate_rate[i] * (step_length / 100);

    if (step_state[i][actual_step[i]]) {

      if (millis() - step_start >= gate_length[i]) {
        in_time[i] = false;
      }
      if (in_time[i]) {
        digitalWrite(gate_out_pin[i], HIGH);  // INVERTED because of NPN transistor
      }
    }

    if (!step_state[i][actual_step[i]] || !in_time[i]) {
      digitalWrite(gate_out_pin[i], LOW);  // INVERTED because of NPN transistor
    }
  }

  the_clock();

  if (DEBUG) {
    print_debug();
  }

}
//
//============================================================================== REQUEST_DATA
//
void request_data() {

  Wire.requestFrom(1, 30);            // request 32 bytes from slave device #1

  while (Wire.available()) {

    for (byte i = 0; i < 8; ++i) {    // 8 bytes
      gate_rate[i] = Wire.read();
    }
    for (byte i = 0; i < 8; ++i) {    // 8 bytes
      actual_step[i] = Wire.read();
    }

    for (byte i = 0; i < 6; ++i) {    // 6 bytes
      which_step[i] = Wire.read();
    }

    selected_part = Wire.read();      // 1 byte
    changed = Wire.read();            // 1 byte
    I2C_readAnything (BPM);           // 2 byte
    which_clock = Wire.read();        // 1 byte
    shift = Wire.read();              // 1 byte
    copy = Wire.read();               // 1 byte
    fill = Wire.read();               // 1 byte
  }
}
//
//============================================================================== PRINT_DEBUG
//
void print_debug() {

  Serial.println("=========================");
  Serial.print(" BPM : ");
  Serial.println(BPM);
  Serial.print("step start : ");
  Serial.println(step_start);
  Serial.print("step length : ");
  Serial.println(step_length);
  Serial.print("clock modulator : ");
  Serial.println(clock_modulator[which_clock]);
  Serial.print("which clock : ");
  Serial.println(which_clock);

  Serial.println("___________________________________________________________________________________________________");

  Serial.println(" n°| actual step | gate rate | Page 1          | Page 2         | Page 3         | Page 4         |");
  for (byte i = 0; i < ALL_PART; ++i) {
    Serial.print(" ");
    Serial.print(i + 1);
    Serial.print(" | ");
    Serial.print(actual_step[i]);
    if (actual_step[i] < 10) {
      Serial.print("           | ");
    } else {
      Serial.print("          | ");
    }
    Serial.print(gate_rate[i]);
    Serial.print("       | ");
    for (byte j = 0; j < ALL_STEP; ++j) {
      Serial.print(step_state[i][j]);
      if (j == 15 | j == 31 | j == 47 | j == 63) {
        Serial.print ("|");
      }
    }
    Serial.println();
  }
  Serial.println("__________________________________________________________________________________________________|");

  Serial.print("Selected part : ");
  Serial.print(selected_part);

  for (byte i = 0; i < 10; ++i) {
    Serial.println();
  }
}
