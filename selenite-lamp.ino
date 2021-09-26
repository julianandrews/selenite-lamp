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
    switch (read<unsigned long>()) {
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

struct PulseHueOptions {
    unsigned int hue;
    unsigned long period;
};

struct PulseHueState {
    double theta;
    PulseHueOptions options;
};

struct CycleHueOptions {
    unsigned long period;
};

struct CycleHuesState {
    unsigned int hue;
    CycleHueOptions options;
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
    .value = { .cycle_hues = { 50000, { 3600000 } } },
};

const State ERROR_STATE = {
    PulseHue,
    .value = { .pulse_hue = { 0, { 0, 1000 } } },
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
    unsigned long period = state.value.cycle_hues.options.period;
    unsigned long step;
    unsigned long wait;
    if (period < MAX_STEPS) {
        step = RAINBOW_SIZE / period;
        wait = 1;
    } else {
        step = RAINBOW_SIZE / MAX_STEPS;
        wait = period / MAX_STEPS;
    }

    setAllPixels(state.value.cycle_hues.hue, 255, 255);
    state.value.cycle_hues.hue = state.value.cycle_hues.hue + step;
    pixels.show();
    delay(wait);
}

void pulseHue() {
    unsigned long period = state.value.pulse_hue.options.period;
    double step;
    unsigned long wait;
    if (period < MAX_STEPS) {
        step = PI / (double) period;
        wait = 1;
    } else {
        step = PI / MAX_STEPS;
        wait = period / MAX_STEPS;
    }

    int value = (int) (255.0 * sin(state.value.pulse_hue.theta));
    setAllPixels(state.value.pulse_hue.options.hue, 255, value);
    state.value.pulse_hue.theta = (state.value.pulse_hue.theta + step);
    if (state.value.pulse_hue.theta > PI) {
        state.value.pulse_hue.theta -= PI;
    }
    pixels.show();
    delay(wait);
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
    Command command = read_command();
    switch (command) {
        case Query:
            write_query_response();
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
        default:
            state = ERROR_STATE;
    }
}

void write_query_response() {
    write<unsigned long>((unsigned long) state.command);
    switch (state.command) {
        case CycleHues:
            write<unsigned long>(state.value.cycle_hues.options.period);
            break;
        case PulseHue:
            write<unsigned int>(state.value.pulse_hue.options.hue);
            write<unsigned long>(state.value.pulse_hue.options.period);
            break;
    }
}

void read_cycle_hues_state() {
    unsigned long period = read<unsigned long>();
    if (period == 0) {
        state.command = Stop;
        return;
    }

    unsigned int hue = 0;
    state.value.cycle_hues = { hue, { period } };
    state.command = CycleHues;
}

void read_pulse_hue_state() {
    unsigned int hue = read<unsigned int>();
    unsigned long period = read<unsigned long>();
    if (period == 0) {
        state.command = Stop;
        return;
    }

    state.value.pulse_hue = { 0.0, { hue, period } };
    state.command = PulseHue;
}

// Write a little-endian value
template <typename T>
void write(T value) {
    union {
        byte buffer[sizeof(T)];
        T value;
    } data;
    data.value = value;
    unsigned int bytes_written = Serial.write(data.buffer, sizeof(T));
}

// Read a little-endian value
template <typename T>
T read() {
    union {
        byte buffer[sizeof(T)];
        T value;
    } data;
    unsigned int bytes_read = Serial.readBytes(data.buffer, sizeof(T));
    return data.value;
}
