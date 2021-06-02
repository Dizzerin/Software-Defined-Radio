#ifndef SETTINGS_H
#define SETTINGS_H

#define SETTINGS_VER    2
#define SETTINGS_MAGIC  (0x7AD10000 | SETTINGS_VER)

struct settings {
        uint32_t tx_freq;
        uint32_t rx_freq;
        float cal_factor;
        uint32_t magic;
};

#endif /* SETTINGS_H */
