#include <array>
#include <stdint.h>

struct CanMessage {
    uint32_t id;
    unsigned long channel;
    unsigned long direction;
    int64_t timestamp_ns;
    unsigned long type;
    unsigned long dlc;
    unsigned long rtr;
    unsigned long fdf;
    unsigned long brs;
    unsigned long esi;
    unsigned char data[64];  // matches payload[]
};
