#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

#define PIN             2
#define NUMPIXELS       7
#define MAX_DELAY       20
#define TARGET_STEPS    1000
#define TAU             6.283185
#define GLEAM_GROUPS    4

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

enum class Command {
    Error = 0,
    Query = 1,
    Stop = 2,
    CycleHues = 3,
    PulseHue = 4,
    ShowRGB = 5,
    Gleam = 6,
};

enum class Error {
    ReadError = 1,
    WriteError = 2,
    ZeroPeriodError = 4,
};

unsigned int operator|=(unsigned int &a, Error b) {
    return a = (a | static_cast<unsigned int>(b));
}

Command readCommand(unsigned int *error) {
    auto command = read<unsigned long>(error);
    if (*error) {
        command = 0;
    }
    switch (command) {
        case 0:
            return Command::Error;
        case 1:
            return Command::Query;
        case 2:
            return Command::Stop;
        case 3:
            return Command::CycleHues;
        case 4:
            return Command::PulseHue;
        case 5:
            return Command::ShowRGB;
        case 6:
            return Command::Gleam;
    }
}

struct ShowRGBOptions {
    byte red;
    byte green;
    byte blue;
};

struct ShowRGBState {
    ShowRGBOptions options;
};

struct PulseHueOptions {
    unsigned int hue;
    unsigned long period;
    unsigned long wait;
};

struct PulseHueState {
    double theta;
    unsigned long wait_left;
    PulseHueOptions options;
};

struct CycleHueOptions {
    unsigned long period;
};

struct CycleHuesState {
    double theta;
    CycleHueOptions options;
};

struct GleamOptions {
    unsigned long period;
    float frequency;
};

struct GleamState {
    double theta[GLEAM_GROUPS];
    unsigned int hue[GLEAM_GROUPS];
    GleamOptions options;
};

struct ErrorOptions {
    unsigned int error;
};

struct ErrorState {
    double theta;
    ErrorOptions options;
};

struct State {
    Command command;
    union Value {
        ShowRGBState show_rgb;
        PulseHueState pulse_hue;
        CycleHuesState cycle_hues;
        GleamState gleam;
        ErrorState error;
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
    Command::CycleHues,
    .value = { .cycle_hues = { 4.79, { 3600000 } } },
};

void setup() {
    Serial.begin(9600, SERIAL_8N1);
    pixels.begin();
}

void loop() {
    if (Serial.available()) {
        processCommand();
    }
    switch (state.command) {
        case Command::Stop:
            stop();
            break;
        case Command::CycleHues:
            cycleHues();
            break;
        case Command::PulseHue:
            pulseHue();
            break;
        case Command::ShowRGB:
            showRGB();
            break;
        case Command::Gleam:
            gleam();
            break;
        case Command::Error:
            error();
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

    if (state.value.pulse_hue.wait_left > 0) {
        delay(min(MAX_DELAY, state.value.pulse_hue.wait_left));
        state.value.pulse_hue.wait_left -= MAX_DELAY;
    } else {
        unsigned int value = 255.0 * sin(state.value.pulse_hue.theta / 2);
        setAllPixels(state.value.pulse_hue.options.hue, 255, value);
        state.value.pulse_hue.theta += steps.step;
        if (state.value.pulse_hue.theta > TAU) {
            state.value.pulse_hue.theta = 0;
            state.value.pulse_hue.wait_left = state.value.pulse_hue.options.wait;
        }
        pixels.show();
        delay(steps.wait);
    }
}

void showRGB() {
    for (unsigned int i = 0; i < pixels.numPixels(); ++i) {
        unsigned long color = pixels.Color(
            state.value.show_rgb.options.red,
            state.value.show_rgb.options.green,
            state.value.show_rgb.options.blue
        );
        pixels.setPixelColor(i, color);
    }
    pixels.show();
}

void gleam() {
    CycleSteps steps = get_cycle_steps(state.value.gleam.options.period);
    for (int i = 0; i < NUMPIXELS; ++i) {
        unsigned int hue = state.value.gleam.hue[i % GLEAM_GROUPS];
        unsigned int value = 255.0 * sin(state.value.gleam.theta[i % GLEAM_GROUPS] / 2);
        pixels.setPixelColor(i, pixels.ColorHSV(hue, 255, value));
        if (state.value.gleam.theta[i % GLEAM_GROUPS] || randomEvent(state.value.gleam.options.frequency)) {
            state.value.gleam.theta[i % GLEAM_GROUPS] += steps.step;
        }
        if (state.value.gleam.theta[i % GLEAM_GROUPS] > TAU) {
            state.value.gleam.hue[i % GLEAM_GROUPS] = random(UINT16_MAX);
            state.value.gleam.theta[i % GLEAM_GROUPS] = 0;
        }
    }
    pixels.show();
    delay(steps.wait);
}

bool randomEvent(float frequency) {
    return (float)random(INT32_MAX) / (float)INT32_MAX < frequency;
}

void error() {
    CycleSteps steps = get_cycle_steps(1000);

    unsigned int value = 255.0 * sin(state.value.error.theta / 2);
    setAllPixels(0, 255, value);
    state.value.error.theta += steps.step;
    while (state.value.error.theta > TAU) {
        state.value.error.theta -= TAU;
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

void processCommand() {
    unsigned int error = 0;
    Command command = readCommand(&error);
    if (!error) {
        switch (command) {
            case Command::Query:
                writeQueryResponse(&error);
                break;
            case Command::Stop:
                state.command = Command::Stop;
                break;
            case Command::CycleHues:
                readCycleHuesState(&error);
                break;
            case Command::PulseHue:
                readPulseHueState(&error);
                break;
            case Command::ShowRGB:
                readShowRGBState(&error);
                break;
            case Command::Gleam:
                readGleamState(&error);
                break;
        }
    }
    if (error) {
        state.value.error = { 0, { error } };
        state.command = Command::Error;
    }
}

void writeQueryResponse(unsigned int *error) {
    write<unsigned long>((unsigned long) state.command, error);
    switch (state.command) {
        case Command::CycleHues:
            write<unsigned long>(state.value.cycle_hues.options.period, error);
            break;
        case Command::PulseHue:
            write<unsigned int>(state.value.pulse_hue.options.hue, error);
            write<unsigned long>(state.value.pulse_hue.options.period, error);
            write<unsigned long>(state.value.pulse_hue.options.wait, error);
            break;
        case Command::ShowRGB:
            write<byte>(state.value.show_rgb.options.red, error);
            write<byte>(state.value.show_rgb.options.green, error);
            write<byte>(state.value.show_rgb.options.blue, error);
            break;
        case Command::Gleam:
            write<unsigned long>(state.value.gleam.options.period, error);
            write<float>(state.value.gleam.options.frequency, error);
            break;
        case Command::Error:
            write<unsigned int>(state.value.error.options.error, error);
            break;

    }
}

void readCycleHuesState(unsigned int *error) {
    unsigned long period = read<unsigned long>(error);
    if (period == 0) {
        *error |= Error::ZeroPeriodError;
    }
    if (*error) {
        return;
    }

    double theta = 0;
    state.value.cycle_hues = { theta, { period } };
    state.command = Command::CycleHues;
}

void readPulseHueState(unsigned int *error) {
    unsigned int hue = read<unsigned int>(error);
    unsigned long period = read<unsigned long>(error);
    unsigned long wait = read<unsigned long>(error);
    if (period == 0) {
        *error |= Error::ZeroPeriodError;
    }
    if (*error) {
        return;
    }

    state.value.pulse_hue = { 0.0, 0, { hue, period, wait } };
    state.command = Command::PulseHue;
}

void readShowRGBState(unsigned int *error) {
    byte red = read<byte>(error);
    byte green = read<byte>(error);
    byte blue = read<byte>(error);
    if (*error) {
        return;
    }

    state.value.show_rgb.options.red = red;
    state.value.show_rgb.options.green = green;
    state.value.show_rgb.options.blue = blue;
    state.command = Command::ShowRGB;
}

void readGleamState(unsigned int *error) {
    unsigned long period = read<unsigned long>(error);
    float frequency = read<float>(error);
    if (*error) {
        return;
    }
    for (int i = 0; i < GLEAM_GROUPS; ++i) {
        state.value.gleam.theta[i] = 0;
        state.value.gleam.hue[i] = random(UINT16_MAX);
    }
    state.value.gleam.options = { period, frequency };
    state.command = Command::Gleam;
}

// Write a little-endian value
template <typename T>
void write(T value, unsigned int *error) {
    union {
        byte buffer[sizeof(T)];
        T value;
    } data;
    data.value = value;
    unsigned int bytes_written = Serial.write(data.buffer, sizeof(T));
    if (bytes_written != sizeof(T)) {
        *error |= Error::WriteError;
    }
}

// Read a little-endian value
template <typename T>
T read(unsigned int *error) {
    union {
        byte buffer[sizeof(T)];
        T value;
    } data;
    unsigned int bytes_read = Serial.readBytes(data.buffer, sizeof(T));
    if (bytes_read != sizeof(T)) {
        *error |= Error::ReadError;
    }
    return data.value;
}
