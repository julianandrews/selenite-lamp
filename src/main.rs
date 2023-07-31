mod args;
mod serial;

use std::io::Write;
use std::sync::mpsc;

use anyhow::{Context, Result};
use clap::{IntoApp, Parser};
use notify::Watcher;

use args::{Args, CliCommand};
use selenite_lamp_commands::Command;

fn main() -> Result<()> {
    let args = Args::parse();
    match &args.command {
        CliCommand::BashCompletion => {
            print_bash_completion(&mut Args::into_app());
            Ok(())
        }
        CliCommand::WatchFile(opts) => watch_file(&opts.file, &args.serial_port, args.verbose),
        CliCommand::Command(command) => send_command(command, &args.serial_port, args.verbose),
    }
}

fn print_bash_completion(cmd: &mut clap::App) {
    clap_complete::generate(
        clap_complete::Shell::Bash,
        cmd,
        cmd.get_name().to_string(),
        &mut std::io::stdout(),
    );
}

fn send_command(command: &Command, serial_port: &str, verbose: bool) -> Result<()> {
    let serialized: Vec<u8> = command
        .to_bincode()
        .context("Failed to serialize command")?;
    if verbose {
        let numbers: Vec<_> = serialized.iter().map(|b| b.to_string()).collect();
        println!("Sending command: {}", numbers.join(","));
    }
    let mut port = serial::get_serial_port(serial_port).context("Failed to open serial port")?;
    port.write(&serialized).context("Faield to transmit data")?;
    if let Command::Query = command {
        let json = Command::from_bincode(port).context("Query failed")?;
        println!("{}", serde_json::to_string(&json).unwrap());
    }

    Ok(())
}

fn watch_file(path: &std::path::Path, serial_port: &str, verbose: bool) -> Result<()> {
    use notify::event::{CreateKind, EventKind, ModifyKind, RenameMode};

    let (tx, rx) = mpsc::channel();

    let mut watcher = notify::RecommendedWatcher::new(tx, notify::Config::default())?;
    let to_watch = path
        .parent()
        .ok_or(anyhow::anyhow!("Failed to find watch directory"))?;
    watcher.watch(to_watch, notify::RecursiveMode::NonRecursive)?;

    update_from_file(path, serial_port, verbose);
    loop {
        match rx.recv()? {
            Ok(event) => {
                if matches!(
                    event.kind,
                    EventKind::Create(CreateKind::File)
                        | EventKind::Modify(
                            ModifyKind::Data(_)
                                | ModifyKind::Metadata(_)
                                | ModifyKind::Name(RenameMode::To)
                        )
                ) && event.paths.iter().any(|p| p.as_path() == path)
                {
                    // Debounce. For some reason the notify-debouncer-mini crate tanks performance,
                    // but it's simple enough to wait a few milliseconds and then clear the channel.
                    std::thread::sleep(std::time::Duration::from_millis(20));
                    loop {
                        match rx.try_recv() {
                            Ok(_) => {}
                            Err(mpsc::TryRecvError::Empty) => break,
                            Err(e) => Err(e)?,
                        }
                    }
                    update_from_file(path, serial_port, verbose);
                }
            }
            Err(error) => eprintln!("Error watching file: {:?}", error),
        }
    }
}

fn update_from_file(path: &std::path::Path, serial_port: &str, verbose: bool) {
    let json = match std::fs::read_to_string(path) {
        Ok(json) => json,
        Err(error) => {
            eprintln!("Failed to read file {:?}: {:?}", path, error);
            return;
        }
    };
    let command: Command = match serde_json::de::from_str(&json) {
        Ok(command) => command,
        Err(error) => {
            eprintln!("Failed to parse file {:?}: {:?}", path, error);
            return;
        }
    };
    println!("Sending command {}", json.trim());
    if let Err(error) = send_command(&command, serial_port, verbose) {
        eprintln!("{:?}", error);
    }
}
