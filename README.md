Userspace driver for the maxtor OneTouch III drive's button.

## What is it ##

An external USB/Firewire hard drive, with a button on it that can launch
a program in windows -- e.g. a backup or sync.

## Button support ##

The button does not currently work anywhere but Windows, where it
is supported by a program that periodically polls the drive for
status.  The polling happens by sending a SCSI INQUIRY command for
vendor-specific "Vital Page Data" (VPD) 0xC3, which returns a packet
of the form 00 C3 00 06 01 00 00 00 XX YY, with XX being the number of
button-down events since the last poll, and YY being the current state
of the button.  The second four bytes may actually have some meaning, but
if so, I have no idea what.

The original OneTouch had a much nicer interface from the point of view of
the kernel -- it provided an additional USB endpoint that could tell the
kernel "hey, I got pressed" without polling.  My guess is that the addition
of the Firewire port made this somehow less workable.

We can, however, easily replicate the windows behavior in user space.
All we need to do is the same polling.

Thus, I've written a short program to do just that.  It polls once
per second, and injects inputs via Linux's "uinput" subsystem.

## USB IDs supported ##

* 0x0d49 : 0x7201 

## Firewire support ##

It appears to work with no code changes, but I have not yet constructed
a sample rule for udev to automatically launch it.  In addition, it
reports the USB IDs to the uinput subsytem, instead of trying to
shoe-horn the Firewire ID in.

The Firewire IDs are:
* Vendor ID: 0x000010B9
* Unit Spec ID: 0x0000609E
* Model ID: 0x00007200

## Contact ##

https://github.com/wnoise/onetouch-iii-button
