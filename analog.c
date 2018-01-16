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

volatile unsigned int *mxlradcregs;
unsigned int i, x;
unsigned long long chan[4] = {0,0,0,0};
int devmem;


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
	analogPinMode(0);
	analogPinMode(2);
	
	analog_init();
	
        int meas_mV0=((((chan[0]/10)*45177)*6235)/100000000);
	int meas_uA0=(((meas_mV0)*1000)/240);
	int meas_mV2=((((chan[2]/10)*45177)*6235)/100000000);
	int meas_uA2=(((meas_mV2)*1000)/240);

	//return meas_uA;
        printf("ADC0 = %d", analogRead(0));
        printf("ADC2 = %d", analogRead(2));
        printf("ADC0 in mA = %d", (meas_uA0/1000));
	printf("ADC2 in mA = %d", (meas_uA2/1000));
	
	/* 1.1736608125[x] or (281678595/240000000)[x]*/
}
