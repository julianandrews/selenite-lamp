# Selenite Lamp

## udev symlink

Plug in the lamp and run:

    sudo dmesg | grep usb

to get the vendor id and product id of the device. Then write a file like:

```
/etc/udev/rules.d

SUBSYSTEM=="tty", ATTRS{idVendor}=="1a86", ATTRS{idProduct}=="7523", SYMLINK+="selenite-lamp", RUN+="/bin/stty -F /dev/selenite-lamp -hupcl"
```

Finally run:

    sudo udevadm control --reload-rules

To reload the rules. /dev/selenite-lamp should now appear whenever the lamp is plugged in.
