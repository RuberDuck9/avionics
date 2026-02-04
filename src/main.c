#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>

#define bme280_addr 0x70

struct bme280{
	uint8_t temperature;
	uint8_t pressure;
	uint8_t humidity;
};

typedef struct bme280 BME280;

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

static void configure_i2c(void){ // setup I2C1 to run on port C in fmp mode

	rcc_periph_clock_enable(RCC_GPIOC);
	gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO6 | GPIO7);
	gpio_set_af(GPIOC, GPIO_AF4, GPIO6 | GPIO7);

	rcc_periph_clock_enable(RCC_I2C1);
	i2c_peripheral_disable(I2C1);
	i2c_set_speed(I2C1, i2c_speed_fmp_1m, rcc_apb1_frequency / 1e6);
	i2c_peripheral_enable(I2C1);
}

static int read_bme280(BME280*){ // check for nominal chip connection

	uint8_t bme280_chip_id_addr = 0xD0;
	uint8_t bme280_chip_id;

	i2c_transfer7(I2C1, bme280_addr, &bme280_chip_id_addr, sizeof(bme280_chip_id_addr), &bme280_chip_id, sizeof(bme280_chip_id)); 

	if (bme280_chip_id == 0x60){

		uint8_t buffer[8];
		uint8_t reg = 0xF7;
		
		i2c_transfer7(I2C1, bme280_addr, &reg, sizeof(reg), &buffer, sizeof(buffer));
	}
	else{
		return 1;
	}
}

int main(void){

	sanity_check();

	configure_i2c();

	return 0;
}
