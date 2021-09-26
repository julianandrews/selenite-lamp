mod commands;
mod serial;

use std::io::Write;

use clap::{crate_authors, crate_version, AppSettings, Clap};

use commands::Command;

fn main() {
    let opts = Options::parse();
    if let Command::BashCompletion = opts.command {
        use clap::IntoApp;
        use clap_generate::generators::Bash;
        clap_generate::generate::<Bash, _>(
            &mut Options::into_app(),
            "selenite-lamp",
            &mut std::io::stdout(),
        );
        return;
    }
    let serialized: Vec<u8> = match opts.command.serialize() {
        Ok(data) => data,
        Err(error) => {
            eprintln!("Failed to serialize command: {}", error);
            std::process::exit(1);
        }
    };
    if opts.verbose {
        let numbers: Vec<_> = serialized.iter().map(|b| b.to_string()).collect();
        println!("Sending command: {}", numbers.join(","));
    }
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
        match Command::read(port) {
            Ok(command) => println!("{}", serde_json::to_string(&command).unwrap()),
            Err(error) => {
                eprintln!("Query failed: {}", error);
                std::process::exit(1);
            }
        }
    }
}

#[derive(Clap, Debug, Clone)]
#[clap(version = crate_version!(), author = crate_authors!())]
#[clap(setting = AppSettings::ColoredHelp)]
pub struct Options {
    /// Serial port to connect to. Something like `/dev/ttyUSB0` or `COM1`.
    #[clap(env = "SELENITE_PORT")]
    pub serial_port: String,
    /// Print verbose output.
    #[clap(short, long)]
    pub verbose: bool,
    #[clap(subcommand)]
    pub command: Command,
}
