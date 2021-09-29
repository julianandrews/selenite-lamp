use std::io::Read;

use bincode::Options;
use clap::{AppSettings, Clap};

#[derive(Clap, Debug, Clone, serde::Deserialize, serde::Serialize)]
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

#[derive(Clap, Debug, Clone, serde::Deserialize, serde::Serialize)]
pub struct StopOptions {}

#[derive(Clap, Debug, Clone, serde::Deserialize, serde::Serialize)]
pub struct PulseHueOptions {
    /// The hue to pulse. 0-65535. Circles the color wheel starting at red.
    hue: u16,
    /// The pulse period in milliseconds.
    #[clap(default_value = "5000")]
    period: u32,
}

#[derive(Clap, Debug, Clone, serde::Deserialize, serde::Serialize)]
pub struct CycleHuesOptions {
    /// The cycle period in milliseconds.
    #[clap(default_value = "5000")]
    period: u32,
}

#[derive(Clap, Debug, Clone, serde::Deserialize, serde::Serialize)]
pub struct ErrorOptions {
    error_code: u16,
}
