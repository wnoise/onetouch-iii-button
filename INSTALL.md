## Prerequisites: ##

* Needs the "cook" package to compile.  But doing it by hand is easy.
* Needs the "sg3-utils" library to talk to the SCSI subsytem.
  Debian provides this in the "sgutils2-dev" package.

## Compilation ##

Typing "cook" should produce a binary named "onetouch-iii-button"

## Testing: ##

It takes one command-line argument: the device-file for it to monitor.
Run it as a user with permission to open both /dev/uinput and the device.
If it is root, it will drop its privileges after opening these files.

(e.g. "sudo onetouch-iii-button /dev/sdc".)

## Install: ##

You can, of course, run this however you want.  We have included a
sample udev rule to start it automatically when this drive is plugged
into the USB port.  The poller should automatically be started by udevd,
and device removal will cause the program to exit.

### For USB: ###

* Copy onetouch-iii-button to /lib/udev
* Copy onetouch-autostart-daemon.rules to /etc/udev/rules.d

### For Firewire: ###

* Copy onetouch-iii-button to /lib/udev
* Figure out what changes to make to onetouch-autostart-daemon.rules in
  order to recognize the drive, and copy to /etc/udev/rules.d .  Good luck,
  and let me know.
