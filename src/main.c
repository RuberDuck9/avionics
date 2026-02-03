#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

int main(void){

	rcc_periph_clock_enable(RCC_GPIOC);

	gpio_set(GPIOC, GPIO13);

	while (1) {
		for (int i = 0; i < 10000000; i++) {
			__asm__("nop");
		}

		gpio_toggle(GPIOC, GPIO13);
	}

	return 0;
}
