#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/usart.h>
#include <stdio.h>
#include <math.h>
#include "bme280.h"

void strobe(int n);
void configure_i2c(void);
void configure_usart(void);
void usart_print(char *buffer);
void usart_println(char *buffer);

int main(void){

	strobe(3);

	BME280 bme280;

	configure_i2c();
	configure_usart();

	char buffer[250];
	
	if (bme280_configure(&bme280, 0b101, 0101, 0b101, 0b11, 0b000, 0b111) != 0){
		// Should probably do something here
	}

	float altitude;

	while(1){
		for (int i = 0; i < 29999; i++) __asm__("nop");
		bme280_measure(&bme280);

		altitude = 44330 * (1 - pow((bme280.pressure / 101325), 0.1903));

		snprintf(buffer, sizeof(buffer), "Temperature: %fC Pressure: %fPa Humidity: %f%%, Altitude: %fm", bme280.temperature, bme280.pressure, bme280.humidity, altitude);
		usart_println(buffer);
	}

	return 0;
}

void strobe(int n){

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

void configure_i2c(void){ 

	rcc_periph_clock_enable(RCC_GPIOB);
	gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO8 | GPIO9);
	gpio_set_af(GPIOB, GPIO_AF4, GPIO8 | GPIO9);

	rcc_periph_clock_enable(RCC_I2C1);
	i2c_peripheral_disable(I2C1);
	i2c_set_speed(I2C1, i2c_speed_fm_400k, rcc_apb1_frequency / 1e6);
	i2c_peripheral_enable(I2C1);
}

void configure_usart(void){

	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_USART2);

	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO2); 
	gpio_set_af(GPIOA, GPIO_AF7, GPIO2);

	usart_set_baudrate(USART2, 115200);
	usart_set_databits(USART2, 8);
	usart_set_stopbits(USART2, USART_STOPBITS_1);
	usart_set_parity(USART2, USART_PARITY_NONE);
	usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);
	usart_set_mode(USART2, USART_MODE_TX);

	usart_enable(USART2);
}

void usart_print(char *buffer){

	while (*buffer){
		usart_send_blocking(USART2, *buffer);
		buffer++;
	}
}

void usart_println(char *buffer){

	usart_print(buffer);
	usart_print("\r\n");
}
