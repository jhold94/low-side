
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdint.h>

#include "i2c-dev.h"

int bitRead(int value, char bit)
{
        return((value >> bit) & 0x01);
}

int fpga_init(char *path, char adr)
{
	static int fd =-1;
	
	if(fd != -1) return fd;
	
	if(path == NULL) {
		fd = open("/dev/i2c-0", O_RDWR);
	}
	
	if(!adr) adr = 0x28;
	
	if(fd != -1) {
		if (ioctl(fd, I2C_SLAVE_FORCE, 0x28) < 0) {
			perror("FPGA did not ACK 0x28\n");
			return -1;
		}
	}
	
	return fd;
}

uint8_t fpeek8(int twifd, uint16_t addr)
{
	uint8_t data[2];
	data[0] = ((addr >> 8) & 0xff);
	data[1] = (addr & 0xff);
	if (write(twifd, data, 2) != 2) {
		perror("I2C Addresss set Failed");
	}
	read(twifd, data, 1);
		    
	return data[0];
}

int specialDigitalRead(int pin)
{
	int twifd = fpga_init(NULL, 0);
        int devreg = fpeek8(twifd, 0xE);
        int state;
        switch(pin)
        {
                case 207:
                        state = bitRead(devreg, 0);
                        return state;
			break;
                case 208:
                        state = bitRead(devreg, 1);
			return state;
                        break;
                case 209:
                        state = bitRead(devreg, 2);
			return state;
                        break;
		case 206:
                        state = bitRead(devreg, 3);
			return state;
                        break;
                default:
                        break;
        }
        return state;
}

int main(int argc, char ** argv)
{
	int x;
        for(x = 206; x < 210; x++) {
                printf("Pin %d value = %d\n", x, specialDigitalRead(x));
        }
        return 0;
}
