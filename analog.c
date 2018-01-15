/* turn the first half into a single init like fpga_init then only call 
 "chan[i] += (mxlradcregs[(0x50+(i * 0x10))/4] & 0xffff);"
 when needed to actually read analog */



/* To compile:
*
*  gcc -fno-tree-cselim -Wall -O0 -mcpu=arm9 -o mx28adcctl mx28adcctl.c
*/

#include <iostream>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "sources/gpiolib.h"

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
		std::cout << "Pin is not supported for current loop" << std::endl;
	}
}

//int main(int argc, char **argv)
int analogRead(int pin) {
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
	for(x = 0; x < 4; x++)
	  mxlradcregs[(0x50+(x * 0x10))/4] = 0x0; //Clear LRADCx reg

	for(x = 0; x < 10; x++) {
		mxlradcregs[0x18/4] = 0x7f; //Clear interrupt ready
		mxlradcregs[0x4/4] = 0x7f; //Schedule conversaion of chan 6:0
		while(!((mxlradcregs[0x10/4] & 0x7f) == 0x7f)); //Wait
		for(i = 0; i < 4; i++)
		  chan[i] += (mxlradcregs[(0x50+(i * 0x10))/4] & 0xffff);
	}
	/* This is where value to voltage conversions would take
	 * place.  Values below are generic and can be used as a 
	 * guideline.  They were derived to be within 1% error,
	 * however differences in environments, tolerance in components,
	 * and other factors may bring that error up.  Further calibration 
	 * can be done to reduce this error on a per-unit basis.
	 *
	 * The math is done to multiply everything up and divide
	 * it down to the resistor network ratio.  It is done 
	 * completely using ints and avoids any floating point math
	 * which has a slower calculation speed and can add in further
	 * error. The intended output units are listed with each equation.
	 *
	 * Additionally, since very large numbers are used in the
	 * example math below, it may not be possible to implement the math
	 * as-is in some real world applications. 
	 *
	 * All chan[x] values include 10 samples, this needs to be 
	 * divided out to get an average.
	 *
	 * TS-7680
	 *   LRADC channels 3:0
	 *     PCB [Rev C] or [Rev B with R134-R137 removed]:
	 *       0-10 V in. chan[x] mV=((((chan[x]/10)*45177)*6235)/100000000);
	 *       4-20 mA in. chan[x]:
	 *         meas_mV=((((chan[x]/10)*45177)*6235)/100000000);
	 *         uA=(((meas_mV)*1000)/240)
	 */


	
}

int main(int argc, char **argv)
{
        meas_mV=((((chan[pin]/10)*45177)*6235)/100000000);
	meas_uA=(((meas_mV)*1000)/240);

	return meas_uA;
        printf("ADC0 = %d", analogRead(0));
        printf("ADC2 = %d", analogRead(2));
        printf("ADC0 in mA = %d", meas_uA * 1000)
