#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>

#define bme280_addr 0x76 // sdo -> GND

struct bme280{
	uint32_t temperature_raw;
	uint32_t pressure_raw;
	uint16_t humidity_raw;

	uint16_t dig_T1;
	int16_t dig_T2;
	int16_t dig_T3;
	uint16_t dig_P1;
	int16_t dig_P2;
	int16_t dig_P3;
	int16_t dig_P4;
	int16_t dig_P5;
	int16_t dig_P6;
	int16_t dig_P7;
	int16_t dig_P8;
	int16_t dig_P9;
	uint8_t dig_H1;
	int16_t dig_H2;
	uint8_t dig_H3;
	int16_t dig_H4;
	int16_t dig_H5;
	int8_t dig_H6;

	int32_t t_fine;
	int32_t temperature;
	int32_t pressure;
	int32_t humidity;
};

typedef struct bme280 BME280;

void strobe(int n);
void configure_i2c(void);
int read_bme280(BME280 *bme280);
int trim_bme280(BME280 *bme280);
int compensate_bme280(BME280 *bme280);

int main(void){

	strobe(3);

	configure_i2c();

	BME280 bme280;

	uint8_t bme280_error_status = read_bme280(&bme280);
	bme280_error_status = trim_bme280(&bme280);
	bme280_error_status = compensate_bme280(&bme280);

	volatile int32_t b = bme280.temperature;

	while(1);

	return 0;
}

void strobe(int n){ // toggle pin 5 on port a

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

void configure_i2c(void){ // setup I2C1 to run on port C in fmp mode

	rcc_periph_clock_enable(RCC_GPIOB);
	gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO8 | GPIO9);
	gpio_set_af(GPIOB, GPIO_AF4, GPIO8 | GPIO9);

	rcc_periph_clock_enable(RCC_I2C1);
	i2c_peripheral_disable(I2C1);
	i2c_set_speed(I2C1, i2c_speed_fm_400k, rcc_apb1_frequency / 1e6);
	i2c_peripheral_enable(I2C1);
}

int read_bme280(BME280 *bme280){

	uint8_t bme280_id;
	uint8_t bme280_id_addr = 0xD0;
	uint8_t bme280_burst_read_addr = 0xF7;
	uint8_t buffer[8];
	int32_t temperature_raw, pressure_raw, humidity_raw;
	
	i2c_transfer7(I2C1, bme280_addr, &bme280_id_addr, sizeof(bme280_id_addr), &bme280_id, sizeof(bme280_id)); 

	if (bme280_id == 0x60){
		i2c_transfer7(I2C1, bme280_addr, &bme280_burst_read_addr, sizeof(bme280_burst_read_addr), buffer, sizeof(buffer)); 
		
		bme280->temperature_raw = (buffer[3]<<12)|(buffer[4]<<4)|(buffer[5]>>4);
		bme280->pressure_raw = (buffer[0]<<12)|(buffer[1]<<4)|(buffer[2]>>4);
		bme280->humidity_raw = (buffer[6]<<8)|(buffer[7]);

		return 0;
	}
	else{
		return 1;
	}
}

int trim_bme280(BME280 *bme280){

	uint8_t buffer[32];
	uint8_t nvm1 = 0x88;
	uint8_t nvm2 = 0xE1;

	i2c_transfer7(I2C1, bme280_addr, &nvm1, 1, buffer, 25);
	i2c_transfer7(I2C1, bme280_addr, &nvm2, 1, buffer + 25, 7);

	bme280->dig_T1 = (uint16_t)(buffer[1] << 8 | buffer[0]);
	bme280->dig_T2 = (int16_t)(buffer[3] << 8 | buffer[2]);
	bme280->dig_T3 = (int16_t)(buffer[5] << 8 | buffer[4]);

	return 0;
}

int compensate_bme280(BME280 *bme280){

	int64_t var1, var2, T, P, H;
	var1 = ((((bme280->temperature_raw>>3) - ((int32_t)bme280->dig_T1<<1))) * ((int32_t)bme280->dig_T2)) >> 11;
	var2 = (((((bme280->temperature_raw>>4) - ((int32_t)bme280->dig_T1)) * ((bme280->temperature_raw>>4) - ((int32_t)bme280->dig_T1))) >> 12) * ((int32_t)bme280->dig_T3)) >> 14;
	int32_t t_fine = var1 + var2;
	T = (t_fine * 5 + 128) >> 8;
	bme280->t_fine = t_fine;
	bme280->temperature = T;

	var1 = ((int64_t)t_fine) - 128000;
	var2 = var1 * var1 * (int64_t)bme280->dig_P6;
	var2 = var2 + ((var1 * (int64_t)bme280->dig_P5)<<17);
	var2 = var2 + (((int64_t)bme280->dig_P4)<<35);
	var1 = ((var1 * var1 * (int64_t)bme280->dig_P3)>>8) + ((var1 * (int64_t)bme280->dig_P2)<<12);
	var1 = (((((int64_t)1)<<47)+var1))*((int64_t)bme280->dig_P1)>>3;
	if (var1 == 0){
		return 1;
	}
	else{
		P = 1048576 - bme280->pressure_raw;
		P = (((P<<31)-var2)*3125)/var1;
		var1 = (((int64_t)bme280->dig_P9) * (P>>13) * (P>>13)) >> 25;
		var2 = (((int64_t)bme280->dig_P8) * P) >> 19;
		P = ((P + var1 + var2) >> 8) + (((int64_t)bme280->dig_P7)<<4);
		bme280->pressure = P;
	}

	P = (t_fine - ((int32_t)76800));
	P = (((((bme280->humidity_raw << 14) - (((int32_t)bme280->dig_H4) << 20) - (((int32_t)bme280->dig_H5) * P)) + ((int32_t)16384)) >> 15) * (((((((P * ((int32_t)bme280->dig_H6)) >> 10) * (((P * ((int32_t)bme280->dig_H3)) >> 11) + ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * ((int32_t)bme280->dig_H2) + 8192) >> 14));
	P = (P - (((((P >> 15) * (P >> 15)) >> 7) * ((int32_t)bme280->dig_H1)) >> 4));
	P = (P < 0 ? 0 : P);
	P = (P > 419430400 ? 419430400 : P);
	bme280->pressure = P;

	return 0;
}
