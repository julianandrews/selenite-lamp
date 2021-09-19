use clap::{AppSettings, Clap};

use bincode::Options as _;
use std::io::Write;
use std::os::unix::prelude::AsRawFd;

fn main() {
    let opts = Options::parse();
    let serialized: Vec<u8> = match serialize_data(&opts.mode) {
        Ok(data) => data,
        Err(error) => {
            eprintln!("Failed to serialize data: {}", error);
            std::process::exit(1);
        }
    };
    let mut port = match get_serial_port(&opts.serial_port) {
        Ok(port) => port,
        Err(error) => {
            eprintln!("Failed to open serial port: {}", error);
            std::process::exit(1);
        }
    };
    if let Err(error) = port.write(&serialized) {
        eprintln!("Failed to transmit data: {}", error);
        std::process::exit(1);
    }
}

#[derive(Clap, Debug, Clone)]
#[clap(version = "0.1", author = "Julian A. <jandrews271@gmail.com>")]
#[clap(setting = AppSettings::ColoredHelp)]
struct Options {
    /// Serial port to connect to. Something like `/dev/ttyUSB0` or `COM1`.
    serial_port: String,
    #[clap(subcommand)]
    mode: Mode,
}

#[derive(Clap, Debug, Clone, serde::Serialize)]
#[repr(u8)]
enum Mode {
    /// Turn off the lights
    Stop = 0,
    /// Cycle through the rainbow
    CycleHues(CycleHuesOptions) = 1,
    /// Pulse a specific hue
    PulseHue(PulseHueOptions) = 2,
}

#[derive(Clap, Debug, Clone, serde::Serialize)]
struct PulseHueOptions {
    /// The hue to pulse. 0-65536. Circles the color wheel starting at red.
    hue: u32,
}

#[derive(Clap, Debug, Clone, serde::Serialize)]
struct CycleHuesOptions {
    /// How much of the color wheel to cycle per step.
    step: u32,
    /// How long to wait between steps.
    wait: u16,
}

fn get_serial_port(
    port_name: &str,
) -> Result<Box<dyn serialport::SerialPort>, Box<dyn std::error::Error>> {
    {
        // TODO: Figure out how to open the connection at the other end before first write.
        let file = std::fs::File::open(port_name)?;
        let mut termios = termios::Termios::from_fd(file.as_raw_fd()).unwrap();
        termios.c_cflag &= !termios::HUPCL;
    }
    let port = serialport::new(port_name, 9600).open()?;

    Ok(port)
}

fn serialize_data(mode: &Mode) -> Result<Vec<u8>, Box<bincode::ErrorKind>> {
    let serializer = bincode::DefaultOptions::new()
        .with_little_endian()
        .with_fixint_encoding();
    serializer.serialize(mode)
}
