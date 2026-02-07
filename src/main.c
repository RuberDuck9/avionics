#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include "bme280.h"

void strobe(int n);
void configure_i2c(void);

int main(void){

	BME280 bme280;

	configure_i2c();

	
	if (bme280_configure(&bme280, 0b011, 0b011, 0b011, 0b11, 0b000, 0b111) != 0){
		// Should probably do something here
	}

	while(1){
		for (int i = 0; i < 59999; i++) __asm__("nop");
		bme280_measure(&bme280);
		strobe(1);
	}

	return 0;
}

void strobe(int n){

	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);
	gpio_set(GPIOA, GPIO5);

	int j = 2 * n;

	while (j > 0){
		for (int i = 0; i < 99999; i++){
			__asm__("nop");
		}
		gpio_toggle(GPIOA, GPIO5);
		j--;
	}

	gpio_clear(GPIOA, GPIO5);
}

void configure_i2c(void){ 

	rcc_periph_clock_enable(RCC_GPIOB);
	gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO8 | GPIO9);
	gpio_set_af(GPIOB, GPIO_AF4, GPIO8 | GPIO9);

	rcc_periph_clock_enable(RCC_I2C1);
	i2c_peripheral_disable(I2C1);
	i2c_set_speed(I2C1, i2c_speed_fm_400k, rcc_apb1_frequency / 1e6);
	i2c_peripheral_enable(I2C1);
}
