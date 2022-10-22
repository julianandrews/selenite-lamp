use clap::{AppSettings, Parser, Subcommand};

#[derive(Parser, Debug, Clone)]
#[clap(version, setting = AppSettings::DeriveDisplayOrder)]
pub struct Args {
    /// Serial port to connect to. Something like `/dev/ttyUSB0` or `COM1`.
    #[clap(env = "SELENITE_PORT")]
    pub serial_port: String,
    /// Print verbose output.
    #[clap(short, long)]
    pub verbose: bool,
    #[clap(subcommand)]
    pub command: CliCommand,
}

#[derive(Subcommand, Debug, Clone)]
#[clap(setting = AppSettings::ColoredHelp)]
pub enum CliCommand {
    #[clap(flatten)]
    Command(selenite_lamp_commands::Command),
    /// Watch the provided file for json, and update when it changes
    WatchFile(WatchFileOptions),
    #[clap(setting = AppSettings::Hidden)]
    BashCompletion,
}

#[derive(Parser, Debug, Clone)]
pub struct WatchFileOptions {
    pub file: std::path::PathBuf,
}
