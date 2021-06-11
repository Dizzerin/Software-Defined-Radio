# Software-Defined-Radio
This project is a quadrature sampling down converter software defined radio reciver for Engineering Electronics II at Walla Walla University.
My partners for this project were [Darius Felder](https://github.com/darius3241) and [Eric Walsh](https://github.com/Eric-Walsh).
For a quick look at the final results of our project jump to [results](#results).
For a quick setup guide jump to [Quisk setup](#quisk-setup).

## Design Objectives
Our professor, [Dr. Rob Frohne](https://github.com/frohro), gave us the following design objectives:
  1. Minimum discernible signal less than 1μV
  2. Good image rejection
  3. Low noise figure
  4. Inexpensive to build
  
  1. **Simple Design**: We wanted our design to be easy to understand and construct
  2. **Ease of Construction**: We wanted to build a simple radio that works
  3. **Low Cost**: Our budget for this project was $30
  
## Overview
The Software Defined Radio (SDR) receiver connects to an antenna and covers the filtering, mixing, and amplification of the desired signal. It then connects a sound card within a computer and software handles the demodulation of the signal into playable audio. The mixing is handled using an Arduino Nano running code provided to us by our Professor, [Dr. Rob Frohne](https://github.com/frohro), which interfaces with [Quisk](https://james.ahlstrom.name/quisk/), an SDR that controls the receiver through the Arduino. Using Quisk, the radio can be tuned to a certain frequency by adjusting the speed of the local oscillator used in the mixing process. Our budget on this project to keep with the low-cost aspect was about $30 including fabrication of the PCB and the required components. 

Note: Overview section by [Konrad McClure](https://github.com/KonradMcClure)

## Theory
The basics of the SDR Receiver are shown below in the block diagram, with the blue components being defined by software. An antenna picks up the radio signal and passes it through a bandpass filter. This filter will attenuate any signals outside our desired range of 6–16 MHz. This is then passed through a Tayloe Mixer, also known as the Tayloe Quadrature Product Detector. The Tayloe Mixer is a simple and efficient mixer that uses a 1:4 demultiplexer and operational amplifiers to generate the I and Q quadrature signals for demodulation. After the signal has been split into four bandbase signals, they are amplified and combined into the I and Q quadrature signals. Those signals go through a final Low-Pass filter that will attenuate signals above 100KHz in frequency, effectively smoothing the output signal. This signal is then sent via 3.5mm audio cable to the soundcard which demodulates the signal. Quisk can then read the input from the sound card and play it. Quisk also interfaces to the Arduino Nano to set the speed of the local oscillator for tuning into the desired frequency.
(Note: This Theory Section is a slightly modified version of work originally written by [Konrad McClure](https://github.com/KonradMcClure)

## Design 
Here is a basic block diagram of our design:
<img width="468" alt="image" src="https://user-images.githubusercontent.com/82369669/121578643-be264080-c9df-11eb-80bb-a3609d6049d5.png">

## Schematic 
![IMG_1417](https://user-images.githubusercontent.com/82369669/121584644-8b337b00-c9e6-11eb-850f-7782e2abbaeb.png)

### List of components
<img width="553" alt="IMG_2660" src="https://user-images.githubusercontent.com/82369669/121580264-65f03e00-c9e1-11eb-8dd8-08c8ac006f2f.png">

### Voltage smoother 
This was added to smooth out 5V power supply from USB. This will help elimate supply noise. 
![IMG_0552](https://user-images.githubusercontent.com/82369669/121580777-fcbcfa80-c9e1-11eb-95ea-0c9cf91d4400.png)

### Voltage converter
This device converts the 5V logic of the arduino nano to the 3.3V logic of the Si5351a for both directions. 
![IMG_2141](https://user-images.githubusercontent.com/82369669/121580883-19593280-c9e2-11eb-8b31-d71b53603982.png)

### Local Oscillator (Si5351a)
The local oscillator we used, SI5351-AB-GT clock generator was selected largely due to the fact that Dr. Frohne has used this chip with success. 
![IMG_3429](https://user-images.githubusercontent.com/82369669/121580915-21b16d80-c9e2-11eb-95d5-8d76481a7042.png)

### Mixer (tayloe)
The Tayloe Mixer is a mixer design as seen in [this paper](http://www.norcalqrp.org/files/Tayloe_mixer_x3a.pdf) by Dan Tayloe. We chose it due to its relatively simple and elegant design. It utilizes a [two-channel SN74CBT multiplexer](http://www.ti.com/lit/ds/symlink/sn74cbt3253.pdf?ts=1591655665924) and a pair of [INA821ID single gain resistor instrumentation amplifiers](http://www.ti.com/lit/ds/symlink/ina821.pdf?HQS=TI-null-null-mousermode-df-pf-null-wwe&DCM=yes&ref_url=https%3A%2F%2Fwww.mouser.com%2F&distId=26). To be more specific, Tayloe describes this as a "switching integrator," rather than a mixer, since a mixer normally produces both a sum and difference frequency of the RF and LO signals. 
![IMG_1014](https://user-images.githubusercontent.com/82369669/121586223-44df1b80-c9e8-11eb-97ff-ce0ca510c2ee.png)

### Amplifiers
Multiple feedback low pass filter and amplifier was used 
![IMG_8803](https://user-images.githubusercontent.com/82369669/121581219-73f28e80-c9e2-11eb-8338-f5c0e73de7f5.png)

### Arduino Nano
![IMG_3501](https://user-images.githubusercontent.com/82369669/121581344-92588a00-c9e2-11eb-8d36-6ace54345ab7.png)

## Simulation
Complete LTSpice model 
<img width="436" alt="image" src="https://user-images.githubusercontent.com/82369669/121587049-37766100-c9e9-11eb-94d1-2ee6531a2701.png">

### Results of simulation 
<img width="992" alt="image" src="https://user-images.githubusercontent.com/82369669/121595561-4235f380-c9f3-11eb-8e2d-aac017399d5d.png">

## Quisk setup 
According to the [Quisk pip project description](https://pypi.org/project/quisk/), the [Quisk software](https://james.ahlstrom.name/quisk/) is "Software Defined Radio (SDR) transceiver software. You supply radio hardware that converts signals at the antenna to complex (I/Q) data at an intermediate frequency (IF). Data can come from a sound card, Ethernet or USB. Quisk then filters and demodulates the data and sends the audio to your speakers or headphones."  We used Quisk in our project for just this purpose.

To get Quisk up and running with our radio, simply:
1. Flash the [openradio_quisk.ino](openradio_quisk.ino) file to the arduino nano microcontroller.
2. Start Quisk and configure a new radio.  Point Quisk to the [quisk_conf_openradio.py](quisk_conf_openradio.py) file.  Alternatively you can simply copy the file to your home directory (~/.quisk_conf.py).
Note: You may need to configure the following settings in this python file:
 * Sound card settings
 * Serial port settings
3. Restart Quisk tune to your desired frequency and listen away!

## Board build up plan 
See [board build up plan](https://github.com/darius3241/Software-Defined-Radio/wiki/Board-build-up-plan).

## Revisions and debugging 
See [revisions and debugging](https://github.com/darius3241/Software-Defined-Radio/wiki/Debugging-and-Revisions).

## Complete Design 
Photo of our complete board:
![20210519_111718](https://user-images.githubusercontent.com/82369669/121589406-def49300-c9eb-11eb-818c-ed892d1545dc.jpg)

## Results
In the end, we had about 1μV of sensitivity and between 10 and 30 dB of image rejection. (Darius’
radio had about 10dB of rejection while mine had about 30dB of image rejection). Though our radios
are working, they have less sensitivity than desired and when we examine the waterfall spectral display
on Quisk there are some distinct lines. We believe these are from signals on the USB input, so it may be
worthwhile to investigate isolating this further in future revisions.

Credit to: Dr. Frohne for his extensive help with the debugging process.

## Future Revision Recomendations
Other recommendations for future revisions include using continuous rows of pin headers for test
points instead of single pin headers to ease soldering, include better ground test points (perhaps two
pads to solder a wire between which ground clips can be clipped onto), moving plastic components
further from surface mount chips which may need to be de-soldered, and including jumpers to allow the
op amps to be powered from an alternate power source separate from the 5V circuit so additional
power can be provided if desired without damaging other components.

## External tools used:
[Multiple Feedback Low-Pass Filter Design Tool](http://sim.okawa-denshi.jp/en/OPtazyuLowkeisan.htm)
[Third Order Butterworth Bandpass Filter Design Tool](https://rf-tools.com/lc-filter/)

## Credits
[Darius Felder](https://github.com/darius3241) - project partner primarily responsible for testing, simulations, and documentation
[Eric Walsh](https://github.com/Eric-Walsh) - project partner primarily responsible for testing and LTSpice simulations
[Dr. Frohne](https://github.com/frohro) - class professor
[Konrad McClure](https://github.com/KonradMcClure) - previous project pioneer
[Caleb Froelich](https://github.com/froeca) - previous project pioneer
[Jordyn Watkins](https://github.com/greenjacketgirl) - previous project pioneer
