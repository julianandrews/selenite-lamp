#![feature(arbitrary_enum_discriminant)]

mod commands;
mod serial;

use std::io::Write;

use anyhow::{Context, Result};
use notify_debouncer_mini::new_debouncer;

use clap::{AppSettings, IntoApp, Parser};
use commands::Command;

fn main() -> Result<()> {
    let opts = Options::parse();
    match &opts.command {
        Command::BashCompletion => {
            let cmd = &mut Options::into_app();
            clap_complete::generate(
                clap_complete::Shell::Bash,
                cmd,
                cmd.get_name().to_string(),
                &mut std::io::stdout(),
            );
            Ok(())
        }
        Command::WatchFile(watch_file_options) => {
            watch_file(&watch_file_options.file, &opts.serial_port, opts.verbose)
        }
        _ => send_command(&opts.command, &opts.serial_port, opts.verbose),
    }
}

fn send_command(command: &Command, serial_port: &str, verbose: bool) -> Result<()> {
    let serialized: Vec<u8> = command.serialize().context("Failed to serialize command")?;
    if verbose {
        let numbers: Vec<_> = serialized.iter().map(|b| b.to_string()).collect();
        println!("Sending command: {}", numbers.join(","));
    }
    let mut port = serial::get_serial_port(serial_port).context("Failed to open serial port")?;
    port.write(&serialized).context("Faield to transmit data")?;
    if let Command::Query = command {
        let json = Command::read(port).context("Query failed")?;
        println!("{}", serde_json::to_string(&json).unwrap());
    }

    Ok(())
}

fn watch_file(path: &std::path::Path, serial_port: &str, verbose: bool) -> Result<()> {
    let (tx, rx) = std::sync::mpsc::channel();

    let mut debouncer = new_debouncer(std::time::Duration::from_millis(50), None, tx)?;
    debouncer
        .watcher()
        .watch(path, notify::RecursiveMode::NonRecursive)?;

    update_from_file(path, serial_port, verbose);
    for _events in rx {
        update_from_file(path, serial_port, verbose);
    }
    unreachable!();
}

fn update_from_file(path: &std::path::Path, serial_port: &str, verbose: bool) -> () {
    let json = match std::fs::read_to_string(path) {
        Ok(json) => json,
        Err(error) => {
            eprintln!("Failed to read file: {:?}", error);
            return;
        }
    };
    let command: Command = match serde_json::de::from_str(&json) {
        Ok(command) => command,
        Err(error) => {
            eprintln!("Failed to parse file: {:?}", error);
            return;
        }
    };
    println!("Sending command {}", json.trim());
    if let Err(error) = send_command(&command, serial_port, verbose) {
        eprintln!("{:?}", error);
    }
}

#[derive(Parser, Debug, Clone)]
#[clap(version, setting = AppSettings::DeriveDisplayOrder)]
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
