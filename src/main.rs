mod commands;
mod serial;

use std::io::Write;

use clap::{crate_version, AppSettings, Clap};

use commands::Command;

fn main() {
    let opts = Options::parse();
    let serialized: Vec<u8> = match opts.command.serialize() {
        Ok(data) => data,
        Err(error) => {
            eprintln!("Failed to serialize command: {}", error);
            std::process::exit(1);
        }
    };
    let mut port = match serial::get_serial_port(&opts.serial_port) {
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
    if let Command::Query = opts.command {
        match serial::read_byte(port) {
            Ok(b) => println!("{:?}", b),
            Err(error) => {
                eprintln!("Query failed: {}", error);
                std::process::exit(1);
            }
        }
    }
}

#[derive(Clap, Debug, Clone)]
#[clap(version = crate_version!(), author = "Julian Andrews <jandrews271@gmail.com>")]
#[clap(setting = AppSettings::ColoredHelp)]
pub struct Options {
    /// Serial port to connect to. Something like `/dev/ttyUSB0` or `COM1`.
    pub serial_port: String,
    #[clap(subcommand)]
    pub command: Command,
}
