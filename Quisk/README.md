OpenRadio Quisk Interface
=========================

This code is used provide an easy-to-use way of controlling the OpenRadio SDR from within Quisk.
It comes in two parts:

1. Firmware to program onto the OpenRadio's Arduino. This presents a CSV-style command interface which is easier to talk to from code than the original 'terminal'-style interface. Any saved settings (i.e. calibration) which were saved with the original 'interactive' code will be used. 

2. A Quisk config file which also provides an interface allowing communication with the OpenRadio board. This file (quisk_conf_openradio.py) needs to be copied to your home directory (~/.quisk_conf.py) and edited appropriately. The following settings may need to be edited:
 * Sound card settings
 * Serial port settings

So, to make this work:

1. Compile and program the arduino firmware (openradio_quisk.ino) to your OpenRadio board.

2. Copy the quisk_conf_openradio.py file to ~/quisk_conf.py and edit appropriately.

3. Run Quisk! The actual tuned frequency will now be shown, and you can use Quisk to tune around the HF bands. 

TODO
====
* Transmit functionality from within Quisk?
* PSK transmit routines, for other applications

