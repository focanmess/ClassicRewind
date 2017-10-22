#include <Wire.h>

#define CONTROLLER_I2C_ADDRESS 0x52

/* Button bits for bytes 6 and 7 */
#define BUTTON_NONE   0x0000
#define BUTTON_R      0x0002
#define BUTTON_START  0x0004
#define BUTTON_HOME   0x0008
#define BUTTON_SELECT 0x0010
#define BUTTON_L      0x0020
#define BUTTON_DOWN   0x0040
#define BUTTON_RIGHT  0x0080
#define BUTTON_UP     0x0100
#define BUTTON_LEFT   0x0200
#define BUTTON_ZR     0x0400
#define BUTTON_X      0x0800
#define BUTTON_A      0x1000
#define BUTTON_Y      0x2000
#define BUTTON_B      0x4000
#define BUTTON_ZL     0x8000

/* Pin mapping */
#define PIN_HOME              2
#define PIN_REWIND            3
#define PIN_SWITCH_SDA        4
#define PIN_DETECT            5
#define PIN_DETECT_CONTROLLER 8

#define CMD_NOP  0
#define CMD_INFO 1
#define CMD_POLL 2

/* These buttons are awful */
#define DEBOUNCE 750000UL

volatile uint8_t CON_INFO[6] = { 0x01, 0x02, 0x03, 0x04, 0x03, 0x01 };
volatile uint8_t CON_POLL[21] = { 0x80, 0x80, 0x7F, 0x7F, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
volatile unsigned long nextMicros = 0UL;
volatile int cmd = CMD_NOP;
volatile int pollOperation = 0;
volatile int pollCounter = 0;
volatile int pollState = 0;
volatile int pulsePin = 0;

inline void controller_press(uint16_t banks) __attribute__((always_inline));

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_HOME, INPUT_PULLUP);
  pinMode(PIN_REWIND, INPUT_PULLUP);
  pinMode(PIN_SWITCH_SDA, OUTPUT);
  pinMode(PIN_DETECT, OUTPUT);
  pinMode(PIN_DETECT_CONTROLLER, INPUT);
  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(PIN_SWITCH_SDA, LOW);
  digitalWrite(PIN_DETECT, HIGH);
  Wire.begin(CONTROLLER_I2C_ADDRESS);
  digitalWrite(SDA, LOW);
  digitalWrite(SCL, LOW);
  Wire.setClock(400000L);
  Wire.onReceive(receiveData);
  Wire.onRequest(requestData);
  delay(1000);
  attachInterrupt(digitalPinToInterrupt(PIN_HOME), isr_home, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_REWIND), isr_rewind, FALLING);
  *digitalPinToPCMSK(PIN_DETECT_CONTROLLER) |= bit(digitalPinToPCMSKbit(PIN_DETECT_CONTROLLER));
  PCIFR |= bit(digitalPinToPCICRbit(PIN_DETECT_CONTROLLER));
  PCICR |= bit(digitalPinToPCICRbit(PIN_DETECT_CONTROLLER));
}

/*
 * Keep the detect line connected to the system low (controller disconnected)
 * for some period of time to reset the I2C clock, forcing the first-party
 * controllers to re-initialize when the detect line goes high (controller
 * reconnected).
 */
void loop() {
  if (pulsePin) {
    delay(200);
    digitalWrite(PIN_DETECT, HIGH);
    digitalWrite(LED_BUILTIN, LOW);
    pulsePin = 0;
  }
}

void isr_home() {
  if ((long)(micros() - nextMicros) >= 0L) {
    nextMicros = micros() + DEBOUNCE;
    pollOperation = 1;
    pollState = 0;
    pollCounter = 0;
    digitalWrite(PIN_SWITCH_SDA, HIGH);
    digitalWrite(LED_BUILTIN, HIGH);
  }
}

void isr_rewind() {
  if ((long)(micros() - nextMicros) >= 0L) {
    nextMicros = micros() + DEBOUNCE;
    pollOperation = 2;
    pollState = 100;
    pollCounter = 0;
    digitalWrite(PIN_SWITCH_SDA, HIGH);
    digitalWrite(LED_BUILTIN, HIGH);
  }
}

ISR(PCINT0_vect) {
  if ((!pulsePin) && (digitalRead(PIN_DETECT_CONTROLLER))) {
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(PIN_DETECT, LOW);
    pulsePin = 1;
  }
}

inline void controller_press(uint16_t bank) {
  CON_POLL[0] = 0x80;
  CON_POLL[1] = 0x80;
  CON_POLL[2] = 0x7F;
  CON_POLL[3] = 0x7F;
  CON_POLL[4] = 0x00;
  CON_POLL[5] = 0x00;
  CON_POLL[6] = ~(bank & 0xFF);        /* BUTTON_R | BUTTON_START | BUTTON_HOME | BUTTON_SELECT | BUTTON_L | BUTTON_DOWN | BUTTON_RIGHT */
  CON_POLL[7] = ~((bank >> 8) & 0xFF); /* BUTTON_UP | BUTTON_LEFT | BUTTON_ZR | BUTTON_X | BUTTON_A | BUTTON_Y | BUTTON_B | BUTTON_ZL */
  /* Remaining bytes should be set to 0 or driver will discard the inputs */
}

void requestData() {
  if (cmd == CMD_INFO) {
    Wire.write((uint8_t *)CON_INFO, 6);
  }
  else if (cmd == CMD_POLL) {
      /* Press Home */
      if (pollOperation == 1) {
        pollCounter++;
        if (pollState == 0) {
          controller_press(BUTTON_NONE);
          if (pollCounter > 10) {
            pollCounter = 0;
            pollState = 1;
          }
        }
        else if (pollState == 1) {
          controller_press(BUTTON_HOME);
          if (pollCounter > 20) {
            pollCounter = 0;
            pollState = 2;
          }
        }
        else if (pollState == 2) {
          controller_press(BUTTON_NONE);
          if (pollCounter > 50) {
            digitalWrite(PIN_SWITCH_SDA, LOW);
            pollCounter = 0;
            pollOperation = 0;
            digitalWrite(PIN_DETECT, LOW);
            pulsePin = 1;
          }
        }
        else {
          digitalWrite(PIN_SWITCH_SDA, LOW);
          pollCounter = 0;
          pollOperation = 0;
          digitalWrite(PIN_DETECT, LOW);
          pulsePin = 1;
        }
      }
      /* Rewind - Press Home, B, B, Down, X */
      else if (pollOperation == 2) {
        pollCounter++;
        if (pollState == 100) {
          controller_press(BUTTON_NONE);
          if (pollCounter > 10) {
            pollCounter = 0;
            pollState = 101;
          }
        }
        else if (pollState == 101) {
          controller_press(BUTTON_HOME);
          if (pollCounter > 20) {
            pollCounter = 0;
            pollState = 102;
          }
        }
        else if (pollState == 102) {
          controller_press(BUTTON_NONE);
          if (pollCounter > 250) {
            pollCounter = 0;
            pollState = 103;
          }
        }
        else if (pollState == 103) {
          controller_press(BUTTON_B);
          if (pollCounter > 10) {
            pollCounter = 0;
            pollState = 104;
          }
        }
        else if (pollState == 104) {
          controller_press(BUTTON_NONE);
          if (pollCounter > 50) {
            pollCounter = 0;
            pollState = 105;
          }
        }
        else if (pollState == 105) {
          controller_press(BUTTON_B);
          if (pollCounter > 10) {
            pollCounter = 0;
            pollState = 106;
          }
        }
        else if (pollState == 106) {
          controller_press(BUTTON_NONE);
          if (pollCounter > 50) {
            pollCounter = 0;
            pollState = 107;
          }
        }
        else if (pollState == 107) {
          controller_press(BUTTON_DOWN);
          if (pollCounter > 10) {
            pollCounter = 0;
            pollState = 108;
          }
        }
        else if (pollState == 108) {
          controller_press(BUTTON_NONE);
          if (pollCounter > 50) {
            pollCounter = 0;
            pollState = 109;
          }
        }

        else if (pollState == 109) {
          controller_press(BUTTON_X);
          if (pollCounter > 10) {
            pollCounter = 0;
            pollState = 110;
          }
        }
        else if (pollState == 110) {
          controller_press(BUTTON_NONE);
          if (pollCounter > 50) {
            digitalWrite(PIN_SWITCH_SDA, LOW);
            pollCounter = 0;
            pollOperation = 0;
            digitalWrite(PIN_DETECT, LOW);
            pulsePin = 1;
          }
        }
        else {
          digitalWrite(PIN_SWITCH_SDA, LOW);
          pollCounter = 0;
          pollOperation = 0;
          digitalWrite(PIN_DETECT, LOW);
          pulsePin = 1;
        }
      }
      else {
        controller_press(BUTTON_NONE);
      }
      Wire.write((uint8_t *)CON_POLL, 21);
  }
  cmd = CMD_NOP;
}

void receiveData(int numBytes) {
  static int request, i;
  if (numBytes == 1) {
    request = Wire.read();
    if (request == 0xFA) {
      cmd = CMD_INFO;
    }
    else if (request == 0x00) {
      cmd = CMD_POLL;
    }
  }
  else {
    while (numBytes > 0) {
      numBytes--;
      request = Wire.read();
    }
  }
}

/* EOF */
