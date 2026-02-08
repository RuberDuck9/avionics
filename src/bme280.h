#include <libopencm3/stm32/i2c.h>

static uint8_t bme280_addr = 0x76; // sdo -> GND
static uint8_t bme280_id_addr = 0xD0;
static uint8_t bme280_burst_read_addr = 0xF7;
static uint8_t nvm1_addr = 0x88;
static uint8_t nvm2_addr = 0xE1;
static uint8_t bme280_rst_reg_addr = 0xE0;
static uint8_t bme280_ctrl_hum_reg_addr = 0xF2;
static uint8_t bme280_config_reg_addr = 0xF5;
static uint8_t bme280_ctrl_meas_reg_addr = 0xF4;

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
	int32_t temperature_compensated;
	int32_t pressure_compensated;
	int32_t humidity_compensated;
	float temperature;
	float pressure;
	float humidity;
};

typedef struct bme280 BME280;

int bme280_measure(BME280 *bme280);
int bme280_configure(BME280 *bme280, uint8_t osrs_t, uint8_t osrs_p, uint8_t osrs_h, uint8_t mode, uint8_t t_sb, uint8_t filter);
static int bme280_read_trim(BME280 *bme280);
static int bme280_read_raw(BME280 *bme280);
static int bme280_compensate_temperature(BME280 *bme280);
static int bme280_compensate_pressure(BME280 *bme280);
static int bme280_compensate_humidity(BME280 *bme280);

int bme280_measure(BME280 *bme280){

	if (bme280_read_raw(bme280) == 0){
		if (bme280->temperature_raw == 0x8000) bme280->temperature = 0;
		else{
			if (bme280_compensate_temperature(bme280) != 0){
				return 2;
			}
			bme280->temperature = bme280->temperature_compensated / 100.0f;
		}

		if (bme280->pressure_raw == 0x8000) bme280->pressure = 0;
		else{
			if (bme280_compensate_pressure(bme280) != 0){
				return 3;
			}
			bme280->pressure = bme280->pressure_compensated / 256.0f;
		}

		if (bme280->humidity_raw == 0x800) bme280->humidity = 0;
		else{
			if (bme280_compensate_humidity(bme280) != 0){
				return 4;
			}
			bme280->humidity = bme280->humidity_compensated / 1024.0f;
		}
	}
	else{
		return 1;
	}

	return 0;
}

int bme280_configure(BME280 *bme280, uint8_t osrs_t, uint8_t osrs_p, uint8_t osrs_h, uint8_t mode, uint8_t t_sb, uint8_t filter){

	if (bme280_read_trim(bme280) != 0){
		return 1;
	}

	uint8_t rst_reg_data[2] = {bme280_rst_reg_addr, 0xB6};
	i2c_transfer7(I2C1, bme280_addr, rst_reg_data, 2, NULL, 0);

	uint8_t hum_reg_data[2] = {bme280_ctrl_hum_reg_addr, osrs_h};
	i2c_transfer7(I2C1, bme280_addr, hum_reg_data, 2, NULL, 0);
	uint8_t hum_reg_readout;
	i2c_transfer7(I2C1, bme280_addr, &hum_reg_data[0], 1, &hum_reg_readout, 1);
	if (hum_reg_data[1] != hum_reg_readout){
		return 2;
	}

	uint8_t sb_reg_data[2] = {bme280_config_reg_addr, (t_sb << 5) | (filter << 2)};
	i2c_transfer7(I2C1, bme280_addr, sb_reg_data, 2, NULL, 0);
	uint8_t sb_reg_readout;
	i2c_transfer7(I2C1, bme280_addr, &sb_reg_data[0], 1, &sb_reg_readout, 1);
	if (sb_reg_data[1] != sb_reg_readout){
		return 3;
	}

	uint8_t ctrl_meas_reg_data[2] = {bme280_ctrl_meas_reg_addr, (osrs_t << 5) | (osrs_p << 2) | mode};
	i2c_transfer7(I2C1, bme280_addr, ctrl_meas_reg_data, 2, NULL, 0);
	uint8_t ctrl_meas_reg_readout;
	i2c_transfer7(I2C1, bme280_addr, &ctrl_meas_reg_data[0], 1, &ctrl_meas_reg_readout, 1);
	if (ctrl_meas_reg_data[1] != ctrl_meas_reg_readout){
		return 4;
	}

	return 0;
}

static int bme280_read_trim(BME280 *bme280){

	uint8_t buffer[32];

	i2c_transfer7(I2C1, bme280_addr, &nvm1_addr, 1, buffer, 25);
	i2c_transfer7(I2C1, bme280_addr, &nvm2_addr, 1, buffer + 25, 7);

	bme280->dig_T1 = (uint16_t)(buffer[1] << 8 | buffer[0]);
	bme280->dig_T2 = (int16_t)(buffer[3] << 8 | buffer[2]);
	bme280->dig_T3 = (int16_t)(buffer[5] << 8 | buffer[4]);

	bme280->dig_P1 = (uint16_t)(buffer[7] << 8 | buffer[6]);
	bme280->dig_P2 = (int16_t)(buffer[9] << 8 | buffer[8]);
	bme280->dig_P3 = (int16_t)(buffer[11] << 8 | buffer[10]);
	bme280->dig_P4 = (int16_t)(buffer[13] << 8 | buffer[12]);
	bme280->dig_P5 = (int16_t)(buffer[15] << 8 | buffer[14]);
	bme280->dig_P6 = (int16_t)(buffer[17] << 8 | buffer[16]);
	bme280->dig_P7 = (int16_t)(buffer[19] << 8 | buffer[18]);
	bme280->dig_P8 = (int16_t)(buffer[21] << 8 | buffer[20]);
	bme280->dig_P9 = (int16_t)(buffer[23] << 8 | buffer[22]);

	bme280->dig_H1 = buffer[24];
	bme280->dig_H2 = (int16_t)(buffer[26] << 8 | buffer[25]);
	bme280->dig_H3 = buffer[27];
	bme280->dig_H4 = (int16_t)((buffer[28] << 4) | (buffer[29] & 0x0F));
	bme280->dig_H5 = (int16_t)((buffer[30] << 4) | (buffer[29] >> 4));
	bme280->dig_H6 = (int8_t)buffer[31];

	return 0;
}

static int bme280_read_raw(BME280 *bme280){

	uint8_t bme280_id;
	uint8_t buffer[8];
	
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

static int bme280_compensate_temperature(BME280 *bme280){

	int32_t var1, var2, T;
	var1 = ((((bme280->temperature_raw >> 3) - ((int32_t)bme280->dig_T1 << 1))) * ((int32_t)bme280->dig_T2)) >> 11;
	var2 = (((((bme280->temperature_raw >> 4) - ((int32_t)bme280->dig_T1)) * ((bme280->temperature_raw >> 4) - ((int32_t)bme280->dig_T1))) >> 12) * ((int32_t)bme280->dig_T3)) >> 14;
	int32_t t_fine = var1 + var2;
	T = (t_fine * 5 + 128) >> 8;
	bme280->t_fine = t_fine;
	bme280->temperature_compensated = T;

	return 0;
}

static int bme280_compensate_pressure(BME280 *bme280){

	int64_t var1, var2, p;
	var1 = ((int64_t)bme280->t_fine) - 128000;
	var2 = var1 * var1 * (int64_t)bme280->dig_P6;
	var2 = var2 + ((var1 * (int64_t)bme280->dig_P5) << 17);
	var2 = var2 + (((int64_t)bme280->dig_P4) << 35);
	var1 = ((var1 * var1 * (int64_t)bme280->dig_P3) >> 8) + ((var1 * (int64_t)bme280->dig_P2) << 12);
	var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)bme280->dig_P1) >> 33;
	if (var1 == 0){
		return 1;
	}
	else{
		p = 1048576 - bme280->pressure_raw;
		p = (((p << 31) - var2) * 3125) / var1;
		var1 = (((int64_t)bme280->dig_P9) * (p >> 13) * (p >> 13)) >> 25;
		var2 = (((int64_t)bme280->dig_P8) * p) >> 19;
		p = ((p + var1 + var2) >> 8) + (((int64_t)bme280->dig_P7) << 4);
		bme280->pressure_compensated = (uint32_t)p;
	}

	return 0;
}

static int bme280_compensate_humidity(BME280 *bme280){

	int32_t h;
	h = (bme280->t_fine - ((int32_t)76800));
	h = (((((bme280->humidity_raw << 14) - (((int32_t)bme280->dig_H4) << 20) - (((int32_t)bme280->dig_H5) * h)) + ((int32_t)16384)) >> 15) * (((((((h * ((int32_t)bme280->dig_H6)) >> 10) * (((h * ((int32_t)bme280->dig_H3)) >> 11) + ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * ((int32_t)bme280->dig_H2) + 8192) >> 14));
	h = (h - (((((h >> 15) * (h >> 15)) >> 7) * ((int32_t)bme280->dig_H1)) >> 4));
	h = (h < 0 ? 0 : h);
	h = (h > 419430400 ? 419430400 : h);
	bme280->humidity_compensated = (uint32_t)h >> 12;

	return 0;
}
