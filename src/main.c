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

static void strobe(int n){ // toggle pin 5 on port a

	rcc_periph_clock_enable(RCC_GPIOA);
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO5);
	gpio_set(GPIOA, GPIO5);

	int j = 2 * n;

	while (j > 0){
		for (int i = 0; i < 999999; i++){
			__asm__("nop");
		}
		gpio_toggle(GPIOA, GPIO5);
		j--;
	}

	gpio_clear(GPIOA, GPIO5);
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

static int read_bme280(BME280*){ 

	uint8_t bme280_chip_id_addr = 0xD0;
	uint8_t bme280_chip_id;

	i2c_transfer7(I2C1, bme280_addr, &bme280_chip_id_addr, sizeof(bme280_chip_id_addr), &bme280_chip_id, sizeof(bme280_chip_id)); 

	if (bme280_chip_id == 0x60){

		uint8_t buffer[8];
		uint8_t reg = 0xF7;
		
		i2c_transfer7(I2C1, bme280_addr, &reg, sizeof(reg), buffer, sizeof(buffer));
	}
	else{
		return 1;
	}
}

int main(void){

	strobe(3);

	return 0;
}
