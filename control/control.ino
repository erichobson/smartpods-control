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
#define SERIAL_PORT            9600

#define HEIGHT_ADDRESS   0x00
#define DEPTH_ADDRESS    0x04
#define HEIGHT_MAX_ADDRESS     0x08
#define DEPTH_MAX_ADDRESS      0x0C

#define PRESET_START_ADDRESS   0x10
#define MAX_PRESETS            3
#define PRESET_SIZE            8
#define PRESET_ADDRESS(preset) (PRESET_START_ADDRESS + (preset * PRESET_SIZE))

typedef struct {
    bool up, down, in, out;
} Movement;

typedef struct {
    volatile unsigned long height, depth;
    unsigned long height_max, depth_max;
    unsigned long height_min, depth_min;
} Pulse;

/* Declarations */
static void update_movement(void);
static void processSerialCommand(void);
static void stop(void);
static void calibrate(void);
static unsigned long get_pulse(const int addr);
static void set_pulse(const int addr, const unsigned long pulse);
static void update_height(void);  
static void update_depth(void);
static void log(void);

static Movement *move;
static Pulse *pulse;

/**
 * @brief 
 *
 *
 */
static void setup(void) {
    /* Initialize pins */
    pinMode(UP, OUTPUT);
    pinMode(DOWN, OUTPUT);
    pinMode(IN, OUTPUT);
    pinMode(OUT, OUTPUT);
    pinMode(BUTTON, INPUT_PULLUP);

    /* Initialize serial */
    Serial.begin(SERIAL_PORT);

    /* Attach interrupts */
    attachInterrupt(digitalPinToInterrupt(HALL_IN), update_depth, RISING);
    attachInterrupt(digitalPinToInterrupt(HALL_DOWN), update_height, FALLING);
    attachInterrupt(digitalPinToInterrupt(BUTTON), stop, FALLING);

    move = (Movement *)malloc(sizeof(Movement));
    pulse = (Pulse *)malloc(sizeof(Pulse));

    /* Initialize pointers */
    initialize_movement(move);
    initialize_pulse(pulse);

    Serial.println("SmartPods Control Ready");
}

/**
 * @brief 
 *
 *
 */
static void loop(void) {
    processSerialCommand();
    update_movement();

    set_pulse(HEIGHT_ADDRESS, pulse->height);
    set_pulse(DEPTH_ADDRESS, pulse->depth);
}

/**
 * @brief 
 *
 *
 */
static void initialize_pulse(Pulse *pulse) {
    pulse->height = get_pulse(HEIGHT_ADDRESS);
    pulse->depth = get_pulse(DEPTH_ADDRESS);
    pulse->height_max = get_pulse(HEIGHT_MAX_ADDRESS);  
    pulse->depth_max = get_pulse(DEPTH_MAX_ADDRESS);
    pulse->height_min = 0;
    pulse->depth_min = 0;
    return;
}

/**
 * @brief 
 *
 *
 */
static void initialize_movement(Movement *move) {
    move->up = false;
    move->down = false;
    move->in = false;
    move->out = false;
    return;
}

/**
 * @brief 
 *
 *
 */
static void update_movement(void) {
    digitalWrite(UP, move->up ? HIGH : LOW);
    digitalWrite(DOWN, move->down ? HIGH : LOW);
    digitalWrite(OUT, move->out ? HIGH : LOW);
    digitalWrite(IN, move->in ? HIGH : LOW);
}

/**
 * @brief 
 *
 *
 */
static void processSerialCommand(void) {
    if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');
        command.trim();

        if (command == "STOP") {
            stop();
        } else {
            switch (command.charAt(0)) {
                case 'C':
                case 'c':
                    calibrate();
                    break;

                case 'U':
                case 'u':
                    move->up = true; 
                    break;

                case 'D':
                case 'd':
                    move->down = true;
                    break;

                case 'I':
                case 'i':
                    move->in = true; 
                    break;

                case 'O':
                case 'o':
                    move->out = true; 
                    break;

                case 'S':
                case 's':
                    stop();
                    break;

                default:
                    Serial.println("Unknown command");
                    break;
            }
        }
    }
}

/**
 * @brief 
 *
 *
 */
static void stop(void) {
    move->up = false; 
    move->down = false; 
    move->in = false;
    move->out = false; 
}

/**
 * @brief 
 *
 *
 */
static void calibrate(void) {
    Serial.println("Starting calibration...");

    /* Move to lowest position */
    move->down = true;
    move->in = true;
    update_movement();
    delay(30000);

    stop();

    /* Reset counters */
    pulse->height = 0;
    pulse->depth = 0;

    move->up = true;
    move->out = true;
    update_movement();
    delay(35000);

    stop();

    Serial.print("Max height: ");
    Serial.print(pulse->height);
    Serial.println(" pulses");

    Serial.print("Max depth: ");
    Serial.print(pulse->depth);
    Serial.println(" pulses"); 

    pulse->height_max = pulse->height;
    pulse->depth_max = pulse->depth;

    Serial.println("Calibration complete.");
}

/**
 * @brief 
 *
 *
 */
static unsigned long get_pulse(const int addr) {
    unsigned long pulse;
    EEPROM.get(addr, pulse);
    return pulse;
}

/**
 * @brief 
 *
 *
 */
static void set_pulse(const int addr, const unsigned long pulse) {
    if (pulse == get_pulse(addr)) {
        return;
    }

    Serial.print("Updating ");
    Serial.print((addr == HEIGHT_ADDRESS) ? "height " : "depth ");
    Serial.print("address: ");
    Serial.println(pulse);

    EEPROM.put(addr, pulse);
    return;
}


/**
 * @brief 
 *
 *
 */
static void update_height(void) {
    if (move->up == true) {
        pulse->height++;
    }

    if (move->down == true) {
        pulse->height--;
    }

    return;
}

static void update_depth(void) {
    if (move->out == true) {
        pulse->depth++;
    }

    if (move->in == true) {
        pulse->depth--;
    }
  
    return;
}


