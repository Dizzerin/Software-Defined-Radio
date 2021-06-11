#ifndef PINS_H
#define PINS_H

// Pin Definitions
#define TX_RX_SWITCH    2
#define MODULATION      3
#define LED             13
#define S0 		4
#define S1		5

// Which Si5351 clock is used for what...
#define RX_CLOCK    SI5351_CLK0
#define TX_CLOCK    SI5351_CLK2
#define SPARE_CLOCK    SI5351_CLK1


// Fast pin toggling.
#define digitalToggleFast(P) \
    *portInputRegister(digitalPinToPort(P)) = digitalPinToBitMask(P)

#endif /* PINS_H */
