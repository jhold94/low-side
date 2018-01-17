/* To compile:
*
*  gcc -fno-tree-cselim -Wall -O0 -mcpu=arm9 -o analog analog.c
*/

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <getopt.h>

volatile unsigned int *mxlradcregs;
unsigned int i, x;
unsigned long long chan[4] = {0,0,0,0};
int devmem;

devmem = open("/dev/mem", O_RDWR|O_SYNC);
	assert(devmem != -1);

// LRADC
mxlradcregs = (unsigned int *) mmap(0, getpagesize(),
  PROT_READ | PROT_WRITE, MAP_SHARED, devmem, 0x80050000);

mxlradcregs[0x148/4] = 0xfffffff; //Clear LRADC6:0 assignments
mxlradcregs[0x144/4] = 0x6543210; //Set LRDAC6:0 to channel 6:0
mxlradcregs[0x28/4] = 0xff000000; //Set 1.8v range
for(x = 0; x < 4; x++) {
  mxlradcregs[(0x50+(x * 0x10))/4] = 0x0; //Clear LRADCx reg
}

int gpio_export(int gpio)
{
	int efd;
	char buf[50];
	int ret;
	efd = open("/sys/class/gpio/export", O_WRONLY);

	if(efd != -1) {
		sprintf(buf, "%d", gpio); 
		ret = write(efd, buf, strlen(buf));
		if(ret < 0) {
			perror("Export failed");
			return -2;
		}
		close(efd);
	} else {
		// If we can't open the export file, we probably
		// dont have any gpio permissions
		return -1;
	}
	return 0;
}

int gpio_direction(int gpio, int dir)
{
	int ret = 0;
	char buf[50];
	sprintf(buf, "/sys/class/gpio/gpio%d/direction", gpio);
	int gpiofd = open(buf, O_WRONLY);
	if(gpiofd < 0) {
		perror("Couldn't open IRQ file");
		ret = -1;
	}

	if(dir == 1 && gpiofd){
		if (3 != write(gpiofd, "out", 3)) {
			perror("Couldn't set GPIO direction to out");
			ret = -2;
		}
	}
	else if(gpiofd) {
		if(2 != write(gpiofd, "in", 2)) {
			perror("Couldn't set GPIO directio to in");
			ret = -3;
		}
	}

	close(gpiofd);
	return ret;
}

int gpio_write(int gpio, int val)
{	
	char buf[50];
	int ret, gpiofd;
	sprintf(buf, "/sys/class/gpio/gpio%d/value", gpio);
	gpiofd = open(buf, O_RDWR);
	if(gpiofd > 0) {
		snprintf(buf, 2, "%d", val);
		ret = write(gpiofd, buf, 2);
		if(ret < 0) {
			perror("failed to set gpio");
			return 1;
		}

		close(gpiofd);
		if(ret == 2) return 0;
	}
	return 1;
}

void gpio_unexport(int gpio)
{
	int gpiofd;
	char buf[50];
	gpiofd = open("/sys/class/gpio/unexport", O_WRONLY);
	sprintf(buf, "%d", gpio);
	write(gpiofd, buf, strlen(buf));
	close(gpiofd);
}

void analogPinMode(int pin)
{
	if (pin == 0)
	{
		gpio_export(231);
		gpio_direction(231, 1);
		gpio_write(231, 1);
		gpio_unexport(231);
	} else if (pin == 2)
	{
		gpio_export(232);
		gpio_direction(232, 1);
		gpio_write(232, 1);
		gpio_unexport(232);
	} else
	{
		printf("Pin is not supported for current loop");
	}
}

analogPinMode(0);
analogPinMode(2);

void analog_init(void)
{
	devmem = open("/dev/mem", O_RDWR|O_SYNC);
	assert(devmem != -1);

	// LRADC
	mxlradcregs = (unsigned int *) mmap(0, getpagesize(),
	  PROT_READ | PROT_WRITE, MAP_SHARED, devmem, 0x80050000);

	mxlradcregs[0x148/4] = 0xfffffff; //Clear LRADC6:0 assignments
	mxlradcregs[0x144/4] = 0x6543210; //Set LRDAC6:0 to channel 6:0
	mxlradcregs[0x28/4] = 0xff000000; //Set 1.8v range
	for(x = 0; x < 4; x++)
	  mxlradcregs[(0x50+(x * 0x10))/4] = 0x0; //Clear LRADCx reg
}
	
	
int analogRead(int pin)
{
	for(x = 0; x < 10; x++) {
		mxlradcregs[0x18/4] = 0x7f; //Clear interrupt ready
		mxlradcregs[0x4/4] = 0x7f; //Schedule conversaion of chan 6:0
		while(!((mxlradcregs[0x10/4] & 0x7f) == 0x7f)); //Wait
		for(i = 0; i < 4; i++)
		  chan[i] += (mxlradcregs[(0x50+(i * 0x10))/4] & 0xffff);
	}
	return 0;
}

int main(int argc, char **argv)
{
	//analogPinMode(0);
	//analogPinMode(2);
	
	//analog_init();
	
	for(x = 0; x < 10; x++) {
		mxlradcregs[0x18/4] = 0x7f; //Clear interrupt ready
		mxlradcregs[0x4/4] = 0x7f; //Schedule conversaion of chan 6:0
		while(!((mxlradcregs[0x10/4] & 0x7f) == 0x7f)); //Wait
		for(i = 0; i < 4; i++)
		  chan[i] += (mxlradcregs[(0x50+(i * 0x10))/4] & 0xffff);
	}
	
        int meas_mV0=((((chan[0]/10)*45177)*6235)/100000000);
	int meas_uA0=(((meas_mV0)*1000)/240);
	int meas_mV2=((((chan[2]/10)*45177)*6235)/100000000);
	int meas_uA2=(((meas_mV2)*1000)/240);

        printf("ADC0 = %d\n", meas_mV0);
        printf("ADC2 = %d\n", meas_mV2);
        printf("ADC0 in mA = %d\n", (meas_uA0/1000));
	printf("ADC2 in mA = %d\n", (meas_uA2/1000));
	
	/* 1.1736608125[x] or (281678595/240000000)[x]*/
}

