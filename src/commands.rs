use std::io::Read;

use bincode::Options;
use clap::{AppSettings, Parser, Subcommand};

#[derive(Debug, Clone, Subcommand, serde::Deserialize, serde::Serialize)]
#[clap(setting = AppSettings::ColoredHelp)]
#[serde(rename_all = "kebab-case")]
#[repr(u8)]
pub enum Command {
    #[clap(setting = AppSettings::Hidden)]
    Error(ErrorOptions) = 0,
    /// Query the current command state
    Query = 1,
    /// Turn off the lights
    Stop(StopOptions) = 2,
    /// Cycle through the rainbow
    CycleHues(CycleHuesOptions) = 3,
    /// Pulse a specific hue
    PulseHue(PulseHueOptions) = 4,
    /// Show a specific RGB color
    ShowRGB(ShowRGBOptions) = 5,
    /// Randomly pulse groups of leds to create a gleaming effect
    Gleam(GleamOptions) = 6,
    #[clap(setting = AppSettings::Hidden)]
    BashCompletion,
}

impl Command {
    pub fn read(reader: impl Read) -> Result<Self, Box<bincode::ErrorKind>> {
        Self::serializer().deserialize_from(reader)
    }

    pub fn serialize(&self) -> Result<Vec<u8>, Box<bincode::ErrorKind>> {
        Self::serializer().serialize(self)
    }

    fn serializer() -> impl Options {
        bincode::DefaultOptions::new()
            .with_little_endian()
            .with_fixint_encoding()
    }
}

#[derive(Parser, Debug, Clone, serde::Deserialize, serde::Serialize)]
pub struct StopOptions {}

#[derive(Parser, Debug, Clone, serde::Deserialize, serde::Serialize)]
pub struct PulseHueOptions {
    /// The hue to pulse. 0-65535. Circles the color wheel starting at red.
    hue: u16,
    /// The pulse period in milliseconds.
    #[clap(default_value = "5000")]
    period: u32,
    /// The wait time in milliseconds between pulses.
    #[clap(default_value = "0")]
    wait: u32,
}

#[derive(Parser, Debug, Clone, serde::Deserialize, serde::Serialize)]
pub struct CycleHuesOptions {
    /// The cycle period in milliseconds.
    #[clap(default_value = "5000")]
    period: u32,
}

#[derive(Parser, Debug, Clone, serde::Deserialize, serde::Serialize)]
pub struct ShowRGBOptions {
    red: u8,
    green: u8,
    blue: u8,
}

#[derive(Parser, Debug, Clone, serde::Deserialize, serde::Serialize)]
pub struct GleamOptions {
    /// The cycle period in milliseconds.
    #[clap(default_value = "10000")]
    period: u32,
    /// The probability of a pulse starting.
    #[clap(default_value = "0.002")]
    frequency: f32,
    /// The number of independent LED groups to use.
    #[clap(default_value = "4")]
    num_groups: u16,
}

#[derive(Parser, Debug, Clone, serde::Deserialize, serde::Serialize)]
pub struct Color {}

#[derive(Parser, Debug, Clone, serde::Deserialize, serde::Serialize)]
pub struct ErrorOptions {
    error_code: u16,
}
