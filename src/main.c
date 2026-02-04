#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

static void sanity_check(void){ // toggle port c pin 13

	int i,j = 0;
	
	rcc_periph_clock_enable(RCC_GPIOC);

	gpio_set(GPIOC, GPIO13);

	while (j < 10){
		for (i = 0; i < 10000000; i++){
			__asm__("nop");

			gpio_toggle(GPIOC, GPIO13);

			j++;
		}
	}
}

int main(void){

	sanity_check();

	return 0;
}
