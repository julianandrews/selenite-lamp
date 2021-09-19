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

struct PulseHueState {
    uint32_t hue;
    float theta;
};

struct CycleHuesState {
    uint32_t hue;
    uint32_t step;
    int wait;
};

enum Mode {
    Stop = 0,
    CycleHues = 1,
    PulseHue = 2,
    Unknown = 255,
};

struct State {
    Mode mode;
    union Value {
        PulseHueState pulse_hue;
        CycleHuesState cycle_hues;
    } value;
};

State state;

void setup() {
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
    clock_prescale_set(clock_div_1);
#endif

    state.mode = PulseHue;
    state.value.pulse_hue = { 40000 };
    Serial.begin(9600, SERIAL_8N1); // opens serial port, sets data rate to 9600 bps
    pixels.begin();
    while (!Serial) { ; }
}

void loop() {
    if (Serial.available()) {
        read_state();
    }
    switch (state.mode) {
        case Stop:
            turnOff();
            pixels.show();
            delay(100);
            break;
        case CycleHues:
            cycleHues();
            break;
        case PulseHue:
            pulseHue(0.02, 32);
            break;
    }
}

void cycleHues() {
    setAllPixels(state.value.cycle_hues.hue, 255, 255);
    state.value.cycle_hues.hue = (state.value.cycle_hues.hue + state.value.cycle_hues.step) % RAINBOW_SIZE;
    pixels.show();
    delay(state.value.cycle_hues.wait);
}

void pulseHue(float theta_step, int wait) {
    int value = (int) (255.0 * sin(state.value.pulse_hue.theta));
    setAllPixels(state.value.pulse_hue.hue, 255, value);
    state.value.pulse_hue.theta = (state.value.pulse_hue.theta + theta_step);
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

void read_state() {
    Mode mode = read_mode();
    switch (mode) {
        case Stop:
            state.mode = Stop;
            break;
        case CycleHues:
            state.value.cycle_hues = { 0, read_uint32_t(), read_int() };
            state.mode = CycleHues;
            break;
        case PulseHue:
            state.value.pulse_hue =  { read_uint32_t(), 0.0 };
            state.mode = PulseHue;
            break;
        case Unknown:
            state.value.pulse_hue = { 0, 0.0 };
            state.mode = PulseHue;
    }
}

Mode read_mode() {
    uint32_t m = read_uint32_t();
    switch (m) {
        case 0:
            return Stop;
        case 1:
            return CycleHues;
        case 2:
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
