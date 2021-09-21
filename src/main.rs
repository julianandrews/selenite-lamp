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
    fix_hupcl(port_name)?;
    let port = serialport::new(port_name, 9600).open()?;
    Ok(port)
}

fn fix_hupcl(port_name: &str) -> Result<(), Box<dyn std::error::Error>> {
    // If HUPCL is set, opening the serial port triggers a reset of the arduino. We can fix this by
    // opening the file and unsetting HUPCL. The first time we do this the arduino will reset
    // though so we have to wait long enough for the arduino to reboot if HUPCL was set.
    let file = std::fs::File::open(port_name)?;
    let fd = file.as_raw_fd();
    let mut termios = termios::Termios::from_fd(fd)?;
    let hupcl_was_set = termios.c_cflag & termios::HUPCL != 0;
    termios.c_cflag &= !termios::HUPCL;
    if hupcl_was_set {
        termios::tcsetattr(fd, termios::TCSANOW, &termios)?;
        std::thread::sleep(std::time::Duration::from_millis(2000));
    }
    Ok(())
}

fn serialize_data(mode: &Mode) -> Result<Vec<u8>, Box<bincode::ErrorKind>> {
    let serializer = bincode::DefaultOptions::new()
        .with_little_endian()
        .with_fixint_encoding();
    serializer.serialize(mode)
}
