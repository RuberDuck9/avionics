#include <math.h>
#include "bme280.h"
#include "io.h"
#include "string.h"

int main(void){

	uint8_t bme280_status;
	BME280 bme280;
	float altitude;
	char char_temperature[10];
	char char_pressure[10];
	char char_humidity[10];
	char char_altitude[10];

	strobe(3);

	configure_i2c();
	configure_usart();

	bme280_status = bme280_configure(&bme280, 0b101, 0b101, 0b101, 0b11, 0b000, 0b111);
	for (int i = 0; i < 999999; i++) __asm__("nop");

	while(1){
		if (bme280_status == 0){
			bme280_status = bme280_measure(&bme280);
			altitude = 44330 * (1 - pow((bme280.pressure / 101325), 0.1903));
			float_to_string(bme280.temperature, char_temperature, 2);
			float_to_string(bme280.pressure, char_pressure, 2);
			float_to_string(bme280.humidity, char_humidity, 2);
			float_to_string(fabs(altitude), char_altitude, 2);
			usart_print("{temperature: "); usart_print(char_temperature); usart_print("C; "); usart_print("pressure: "); usart_print(char_pressure); usart_print("Pa; "); usart_print("humidity: "); usart_print(char_humidity); usart_print("%; "); usart_print("altitude: "); if (altitude < 0) usart_print("-"); usart_print(char_altitude); usart_println("m}");
		}
	}

	return 0;
}
