#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif
#define PIN             2
#define NUMPIXELS       7
#define BRIGHTNESS      255
#define RAINBOW_SIZE    65536
#define PI              3.141592653

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

enum Command {
    Query = 0,
    Stop = 1,
    CycleHues = 2,
    PulseHue = 3,
    Unknown = 255,
};

struct PulseHueState {
    uint32_t hue;
    double theta;
    double theta_step;
    int wait;
};

struct CycleHuesState {
    uint32_t hue;
    uint32_t step;
    int wait;
};

struct State {
    Command command;
    union Value {
        PulseHueState pulse_hue;
        CycleHuesState cycle_hues;
    } value;
};

State state;

void setup() {
    state.command = CycleHues;
    state.value.cycle_hues = { 50000, 1, 15 };
    Serial.begin(9600, SERIAL_8N1);
    pixels.begin();
    while (!Serial) { ; }
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
    delay(100);
}

void cycleHues() {
    setAllPixels(state.value.cycle_hues.hue, 255, 255);
    state.value.cycle_hues.hue = (state.value.cycle_hues.hue + state.value.cycle_hues.step) % RAINBOW_SIZE;
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
            state.value.pulse_hue = { 0, 0.0, 0.2, 32 };
            state.command = PulseHue;
    }
}

void read_cycle_hues_state() {
    uint32_t hue = 0;
    int wait = read_int();
    uint32_t step = read_uint32_t();

    state.value.cycle_hues = { hue, step, wait };
    state.command = CycleHues;
}

void read_pulse_hue_state() {
    uint32_t hue = read_uint32_t();
    int period = read_int();
    double theta = 0.0;
    double step = 2.0 * PI / (double) period;
    int wait = 2;
    state.value.pulse_hue = { hue, theta, step, wait };
    state.command = PulseHue;
}

Command read_command() {
    uint32_t m = read_uint32_t();
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

uint32_t read_uint32_t() {
    byte buffer[4] = {0, 0, 0, 0};
    Serial.readBytes(buffer, 4);
    return buffer[3] << 24 | buffer[2] << 16 | buffer[1] << 8 | buffer[0] << 0;
}

int read_int() {
    byte buffer[2] = {0, 0};
    Serial.readBytes(buffer, 2);
    return buffer[1] << 8 | buffer[0] << 0;
}
