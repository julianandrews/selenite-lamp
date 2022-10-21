use std::time::Duration;

use serialport::SerialPort;

pub fn get_serial_port(port_name: &str) -> Result<Box<dyn SerialPort>, serialport::Error> {
    let port = serialport::new(port_name, 9600)
        .timeout(Duration::from_millis(100))
        .open()?;
    Ok(port)
}
