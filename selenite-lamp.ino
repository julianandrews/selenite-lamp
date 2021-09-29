#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
#define PIN             2
#define NUMPIXELS       7
#define MAX_DELAY       20
#define TARGET_STEPS    1000
#define TAU             6.283185

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
    double theta;
    CycleHueOptions options;
};

struct State {
    Command command;
    union Value {
        PulseHueState pulse_hue;
        CycleHuesState cycle_hues;
    } value;
};

struct CycleSteps {
    double step;
    unsigned long wait;
};

struct CycleSteps get_cycle_steps(unsigned long period) {
    double step;
    unsigned long wait;
    if (period > TARGET_STEPS * MAX_DELAY) {
        // Period is long, wait the maximum.
        step = TAU * MAX_DELAY / (double) period;
        wait = MAX_DELAY;
    } else if (period < TARGET_STEPS) {
        // Period is short, wait the minimum.
        step = TAU / (double) period;
        wait = 1;
    } else {
        step = TAU / TARGET_STEPS;
        wait = period / TARGET_STEPS;
    }
    return CycleSteps { step, wait };
}

State state = {
    CycleHues,
    .value = { .cycle_hues = { 4.79, { 3600000 } } },
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
    delay(MAX_DELAY);
}

void cycleHues() {
    CycleSteps steps = get_cycle_steps(state.value.cycle_hues.options.period);

    unsigned int hue = UINT16_MAX * (state.value.cycle_hues.theta / TAU);
    setAllPixels(hue, 255, 255);
    state.value.cycle_hues.theta += steps.step;
    while (state.value.cycle_hues.theta > TAU) {
        state.value.cycle_hues.theta -= TAU;
    }
    pixels.show();
    delay(steps.wait);
}

void pulseHue() {
    CycleSteps steps = get_cycle_steps(state.value.pulse_hue.options.period);

    unsigned int value = 255.0 * sin(state.value.pulse_hue.theta / 2);
    setAllPixels(state.value.pulse_hue.options.hue, 255, value);
    state.value.pulse_hue.theta = (state.value.pulse_hue.theta + steps.step);
    while (state.value.pulse_hue.theta > TAU) {
        state.value.pulse_hue.theta -= TAU;
    }
    pixels.show();
    delay(steps.wait);
}

void setAllPixels(unsigned int hue, unsigned int saturation, unsigned int value) {
    for (unsigned int i = 0; i < pixels.numPixels(); ++i) {
        pixels.setPixelColor(i, pixels.gamma32(pixels.ColorHSV(hue, saturation, value)));
    }
}

void turnOff() {
    for (unsigned int i = 0; i < pixels.numPixels(); ++i) {
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

    double theta = 0;
    state.value.cycle_hues = { theta, { period } };
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
