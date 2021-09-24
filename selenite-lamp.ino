#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
#define PIN             2
#define NUMPIXELS       7
#define MAX_STEPS       1000
#define RAINBOW_SIZE    65535
#define PI              3.141592653
#define STOP_DELAY      100

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

enum Command {
    Query = 0,
    Stop = 1,
    CycleHues = 2,
    PulseHue = 3,
    Unknown = 255,
};

Command read_command() {
    unsigned long m = read_ulong();
    switch (m) {
        case 0:
            return Query;
        case 1:
            return Stop;
        case 2:
            return CycleHues;
        case 3:
            return PulseHue;
        default:
            return Unknown;
    }
}

struct PulseHueState {
    unsigned int hue;
    double theta;
    double theta_step;
    unsigned long wait;
};

struct CycleHuesState {
    unsigned int hue;
    unsigned long step;
    unsigned long wait;
};

struct State {
    Command command;
    union Value {
        PulseHueState pulse_hue;
        CycleHuesState cycle_hues;
    } value;
};

State state = {
    CycleHues,
    .value = { .cycle_hues = { 50000, 1, 15 }}
};

const State ERROR_STATE = {
    PulseHue,
    .value = { .pulse_hue = { 0, 0.0, 0.2, 32 } }
};

void setup() {
    Serial.begin(9600, SERIAL_8N1);
    pixels.begin();
}

void loop() {
    if (Serial.available()) {
        process_command();
    }
    switch (state.command) {
        case Stop:
            stop();
            break;
        case CycleHues:
            cycleHues();
            break;
        case PulseHue:
            pulseHue();
            break;
    }
}

void stop() {
    turnOff();
    pixels.show();
    delay(STOP_DELAY);
}

void cycleHues() {
    setAllPixels(state.value.cycle_hues.hue, 255, 255);
    state.value.cycle_hues.hue = state.value.cycle_hues.hue + state.value.cycle_hues.step;
    pixels.show();
    delay(state.value.cycle_hues.wait);
}

void pulseHue() {
    int value = (int) (255.0 * sin(state.value.pulse_hue.theta));
    setAllPixels(state.value.pulse_hue.hue, 255, value);
    state.value.pulse_hue.theta = (state.value.pulse_hue.theta + state.value.pulse_hue.theta_step);
    if (state.value.pulse_hue.theta > PI) {
        state.value.pulse_hue.theta -= PI;
    }
    pixels.show();
    delay(state.value.pulse_hue.wait);
}

void setAllPixels(int hue, int saturation, int value) {
    for (int i = 0; i < pixels.numPixels(); ++i) {
        pixels.setPixelColor(i, pixels.gamma32(pixels.ColorHSV(hue, saturation, value)));
    }
}

void turnOff() {
    for (int i = 0; i < pixels.numPixels(); ++i) {
        pixels.setPixelColor(i, pixels.Color(0,0,0));
    }
}

void process_command() {
    switch (read_command()) {
        case Query:
            Serial.write(state.command);
            break;
        case Stop:
            state.command = Stop;
            break;
        case CycleHues:
            read_cycle_hues_state();
            break;
        case PulseHue:
            read_pulse_hue_state();
            break;
        case Unknown:
            state = ERROR_STATE;
    }
}

void read_cycle_hues_state() {
    unsigned long period = read_ulong();
    if (period == 0) {
        state.command = Stop;
        return;
    }

    unsigned int hue = 0;
    unsigned long step;
    unsigned long wait;
    if (period < MAX_STEPS) {
        step = RAINBOW_SIZE / period;
        wait = 1;
    } else {
        step = RAINBOW_SIZE / MAX_STEPS;
        wait = period / MAX_STEPS;
    }

    state.value.cycle_hues = { hue, step, wait };
    state.command = CycleHues;
}

void read_pulse_hue_state() {
    unsigned int hue = read_uint();
    unsigned long period = read_ulong();
    if (period == 0) {
        state.command = Stop;
        return;
    }

    double theta = 0.0;
    double step;
    unsigned long wait;
    if (period < MAX_STEPS) {
        step = PI / (double) period;
        wait = 1;
    } else {
        step = PI / MAX_STEPS;
        wait = period / MAX_STEPS;
    }
    state.value.pulse_hue = { hue, theta, step, wait };
    state.command = PulseHue;
}

unsigned long read_ulong() {
    byte buffer[4] = {0, 0, 0, 0};
    unsigned int bytes_read = Serial.readBytes(buffer, 4);
    return (unsigned long) buffer[3] << 24
        | (unsigned long) buffer[2] << 16
        | (unsigned long) buffer[1] << 8
        | (unsigned long) buffer[0] << 0;
}

unsigned int read_uint() {
    byte buffer[2] = {0, 0};
    unsigned int bytes_read = Serial.readBytes(buffer, 2);
    return (unsigned int) buffer[1] << 8
        | (unsigned int) buffer[0] << 0;
}
