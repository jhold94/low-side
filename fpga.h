#ifndef __FPGA_H_
#define __FPGA_H_

struct cbarpin
{
        int addr;
        char *name;
};

int fpga_init(char *path, char adr);
uint8_t fpeek8(int twifd, uint16_t addr);
int bitRead(int value, char bit);
int specialDigitalRead(int pin);

#endif
