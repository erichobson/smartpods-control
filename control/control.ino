/* NOTE: UPLOADING THIS TO THE DESK WILL OVERWRITE EVERYTHING */
/* NOTE: THE MINI COMPUTER THAT CONTROLS THE DESK WILL NO LONGER WORK */
/* NOTE: YOU WILL NOT BE ABLE TO USE THE MINI COMPUTER AGAIN */
/* NOTE: THIS IS A ONE-WAY STREET */
/* NOTE: YOU HAVE BEEN WARNED */

#include <limits.h>
#include <EEPROM.h>

#define UP      9
#define DOWN    10
#define IN      11
#define OUT     12

#define HALL_DOWN    2
#define HALL_IN      3
#define BUTTON       18

#define MAX_PRESETS    3
#define PRESET_SIZE    8

enum {
    STOP,
    MOVE_UP,
    MOVE_DOWN,
    MOVE_IN,
    MOVE_OUT,
};

static volatile unsigned long inOutPulseCount = 0;
static volatile unsigned long upDownPulseCount = 0;
static bool movingIn = false;
static bool movingDown = false;
static int state = STOP;

/* Function prototypes */
static void moveToPosition(const unsigned long targetInOutPulses, const unsigned long targetUpDownPulses);
static void updateMovement(void);
static void savePreset(const int presetNumber);
static void loadPreset(const int presetNumber);
static void processSerialCommand(void);
static void calibrate(void);
static void countPulses(void);
static void countUpDownPulses(void);

void setup() {
    pinMode(UP, OUTPUT);
    pinMode(DOWN, OUTPUT);
    pinMode(IN, OUTPUT);
    pinMode(OUT, OUTPUT);
    pinMode(BUTTON, INPUT_PULLUP);
    Serial.begin(9600);

    attachInterrupt(digitalPinToInterrupt(HALL_IN), countPulses, RISING);
    attachInterrupt(digitalPinToInterrupt(HALL_DOWN), countUpDownPulses, FALLING);

    Serial.println("Standing Desk Control Ready");
}

void loop() {
    processSerialCommand();  
    updateMovement();
}

static void updateMovement(void) {
    switch (state) {
        case MOVE_UP:
            movingDown = false;
            digitalWrite(UP, HIGH);
            digitalWrite(DOWN, LOW);
            digitalWrite(IN, LOW);
            digitalWrite(OUT, LOW);
            break;

        case MOVE_DOWN:
            movingDown = true;
            digitalWrite(UP, LOW);
            digitalWrite(DOWN, HIGH);
            digitalWrite(IN, LOW);
            digitalWrite(OUT, LOW);
            break;

        case MOVE_IN:
            movingIn = true;
            digitalWrite(UP, LOW);
            digitalWrite(DOWN, LOW);
            digitalWrite(IN, HIGH);
            digitalWrite(OUT, LOW);
            break;

        case MOVE_OUT:
            movingIn = false;
            digitalWrite(UP, LOW);
            digitalWrite(DOWN, LOW);
            digitalWrite(IN, LOW);
            digitalWrite(OUT, HIGH);
            break;

        case STOP:
            digitalWrite(UP, LOW);
            digitalWrite(DOWN, LOW);
            digitalWrite(IN, LOW);
            digitalWrite(OUT, LOW);
            break;
    }
}

static void moveToPosition(const unsigned long targetInOutPulses, const unsigned long targetUpDownPulses) {
    while (inOutPulseCount != targetInOutPulses) {
        if (inOutPulseCount < targetInOutPulses) {
            state = MOVE_IN;
        } else {
            state = MOVE_OUT;
        }
        updateMovement();
        delay(10);
    }
    
    while (upDownPulseCount != targetUpDownPulses) {
        if (upDownPulseCount < targetUpDownPulses) {
            state = MOVE_DOWN;
        } else {
            state = MOVE_UP;
        }
        updateMovement();
        delay(10);
    }
    
    state = STOP;
    updateMovement();
}

static void savePreset(const int presetNumber) {
    if (presetNumber >= 1 && presetNumber <= MAX_PRESETS) {
        int address = (presetNumber - 1) * PRESET_SIZE;
        EEPROM.put(address, inOutPulseCount);
        EEPROM.put(address + 4, upDownPulseCount);
        Serial.print("Preset ");
        Serial.print(presetNumber);
        Serial.println(" saved.");
    }
}

static void loadPreset(const int presetNumber) {
    if (presetNumber >= 1 && presetNumber <= MAX_PRESETS) {
        int address = (presetNumber - 1) * PRESET_SIZE;
        unsigned long savedInOutPulses, savedUpDownPulses;
        EEPROM.get(address, savedInOutPulses);
        EEPROM.get(address + 4, savedUpDownPulses);
        moveToPosition(savedInOutPulses, savedUpDownPulses);
    }
}

static void processSerialCommand(void) {
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        if (command.startsWith("SAVE")) {
            int presetNumber = command.substring(4).toInt();
            savePreset(presetNumber);
        } else if (command.startsWith("LOAD")) {
            int presetNumber = command.substring(4).toInt();
            loadPreset(presetNumber);
        } else if (command.startsWith("MOVE")) {
            int inOut = command.substring(5, command.indexOf(',')).toInt();
            int upDown = command.substring(command.indexOf(',') + 1).toInt();
            moveToPosition(inOut, upDown);
        } else if (command == "CALIBRATE") {
            calibrate();
        } else {
            switch (command.charAt(0)) {
                case 'U': state = MOVE_UP; break;
                case 'D': state = MOVE_DOWN; break;
                case 'I': state = MOVE_IN; break;
                case 'O': state = MOVE_OUT; break;
                case 'S': state = STOP; break;
            }
        }
    }
}


static void calibrate(void) {
    Serial.println("Starting calibration...");
    
    /* Move to lowest position */
    state = MOVE_DOWN;
    updateMovement();
    delay(35000);

    /* Move to inner position */
    state = MOVE_IN;
    updateMovement();
    delay(20000);
    
    /* Reset counters */
    inOutPulseCount = 0;
    upDownPulseCount = 0;
    
    /* Move to highest position */
    state = MOVE_UP;
    updateMovement();
    delay(35000);

    /* Move to outer position */
    state = MOVE_OUT;
    updateMovement();
    delay(20000);

    Serial.print("Max height: ");
    Serial.print(upDownPulseCount);
    Serial.println(" pulses");

    Serial.print("Max width: ");
    Serial.print(inOutPulseCount);
    Serial.println(" pulses"); 
       
    state = STOP;
    updateMovement();
    Serial.println("Calibration complete.");
}

static void countPulses(void) {
    if (movingIn) {
        inOutPulseCount++;
    } else {
        inOutPulseCount--;
    }
}

static void countUpDownPulses(void) {
    if (movingDown) {
        upDownPulseCount++;
    } else {
        upDownPulseCount--;
    }
}
