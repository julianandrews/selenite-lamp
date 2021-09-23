use bincode::Options;
use clap::{AppSettings, Clap};

#[derive(Clap, Debug, Clone, serde::Serialize)]
#[clap(setting = AppSettings::ColoredHelp)]
#[repr(u8)]
pub enum Command {
    /// Query the current command state
    Query = 0,
    /// Turn off the lights
    Stop = 1,
    /// Cycle through the rainbow
    CycleHues(CycleHuesOptions) = 2,
    /// Pulse a specific hue
    PulseHue(PulseHueOptions) = 3,
}

impl Command {
    pub fn serialize(&self) -> Result<Vec<u8>, Box<bincode::ErrorKind>> {
        let serializer = bincode::DefaultOptions::new()
            .with_little_endian()
            .with_fixint_encoding();
        serializer.serialize(self)
    }
}

#[derive(Clap, Debug, Clone, serde::Serialize)]
pub struct PulseHueOptions {
    /// The hue to pulse. 0-65536. Circles the color wheel starting at red.
    hue: u32,
}

#[derive(Clap, Debug, Clone, serde::Serialize)]
pub struct CycleHuesOptions {
    /// How much of the color wheel to cycle per step.
    step: u32,
    /// How long to wait between steps.
    wait: u16,
}
