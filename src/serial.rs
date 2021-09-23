use std::os::unix::prelude::AsRawFd;

pub fn get_serial_port(
    port_name: &str,
) -> Result<Box<dyn serialport::SerialPort>, Box<dyn std::error::Error>> {
    fix_hupcl(port_name)?;
    let port = serialport::new(port_name, 9600).open()?;
    Ok(port)
}

pub fn read_byte(
    mut port: Box<dyn serialport::SerialPort>,
) -> Result<u8, Box<dyn std::error::Error>> {
    while port.bytes_to_read()? == 0 {}
    let mut buf = [0; 1];
    port.read_exact(&mut buf)?;
    Ok(buf[0])
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
        std::thread::sleep(std::time::Duration::from_millis(3000));
    }
    Ok(())
}
