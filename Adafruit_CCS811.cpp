#include "Adafruit_CCS811.h"

bool Adafruit_CCS811::begin(uint8_t addr)
{
	_i2caddr = addr;
	
	_i2c_init();

	SWReset();
	delay(100);
	
	//check that the HW id is correct
	if(this->read8(CCS811_HW_ID) != CCS811_HW_ID_CODE)
		return false;
	
	//try to start the app
	this->write(CCS811_BOOTLOADER_APP_START, NULL, 0);
	delay(100);
	
	//make sure there are no errors and we have entered application mode
	if(checkError()) return false;
	if(!_status.FW_MODE) return false;
	
	disableInterrupt();
	
	//default to read every second
	setDriveMode(CCS811_DRIVE_MODE_1SEC);
	
	return true;
}

void Adafruit_CCS811::setDriveMode(uint8_t mode)
{
	_meas_mode.DRIVE_MODE = mode;
	this->write8(CCS811_MEAS_MODE, _meas_mode.get());
}

void Adafruit_CCS811::enableInterrupt()
{
	_meas_mode.INT_DATARDY = 1;
	this->write8(CCS811_MEAS_MODE, _meas_mode.get());
}

void Adafruit_CCS811::disableInterrupt()
{
	_meas_mode.INT_DATARDY = 0;
	this->write8(CCS811_MEAS_MODE, _meas_mode.get());
}

bool Adafruit_CCS811::available()
{
	_status.set(read8(CCS811_STATUS));
	if(!_status.DATA_READY)
		return false;
	else return true;
}

uint8_t Adafruit_CCS811::readData()
{
	if(!available())
		return false;
	else{
		uint8_t buf[8];
		this->read(CCS811_ALG_RESULT_DATA, buf, 8);

		_eCO2 = ((uint16_t)buf[0] << 8) | ((uint16_t)buf[1]);
		_TVOC = ((uint16_t)buf[2] << 8) | ((uint16_t)buf[3]);
		
		if(_status.ERROR)
			return buf[5];
			
		else return 0;
	}
}

void Adafruit_CCS811::setEnvironmentalData(uint8_t humidity, double temperature)
{
	/* Humidity is stored as an unsigned 16 bits in 1/512%RH. The
	default value is 50% = 0x64, 0x00. As an example 48.5%
	humidity would be 0x61, 0x00.*/
	
	/* Temperature is stored as an unsigned 16 bits integer in 1/512
	degrees; there is an offset: 0 maps to -25°C. The default value is
	25°C = 0x64, 0x00. As an example 23.5% temperature would be
	0x61, 0x00.
	The internal algorithm uses these values (or default values if
	not set by the application) to compensate for changes in
	relative humidity and ambient temperature.*/
	
	uint8_t hum_perc = humidity << 1;
	
	float fractional = modf(temperature, &temperature);
	uint16_t temp_high = (((uint16_t)temperature + 25) << 9);
	uint16_t temp_low = ((uint16_t)(fractional / 0.001953125) & 0x1FF);
	
	uint16_t temp_conv = (temp_high | temp_low);

	uint8_t buf[] = {hum_perc, 0x00,
		(uint8_t)((temp_conv >> 8) & 0xFF), (uint8_t)(temp_conv & 0xFF)};
	
	this->write(CCS811_ENV_DATA, buf, 4);

}

//calculate temperature based on the NTC register
double Adafruit_CCS811::calculateTemperature()
{
	uint8_t buf[4];
	this->read(CCS811_NTC, buf, 4);

	uint32_t vref = ((uint32_t)buf[0] << 8) | buf[1];
	uint32_t vntc = ((uint32_t)buf[2] << 8) | buf[3];
	
	//from ams ccs811 app note
	uint32_t rntc = vntc * CCS811_REF_RESISTOR / vref;
	
	double ntc_temp;
	ntc_temp = log((double)rntc / CCS811_REF_RESISTOR); // 1
	ntc_temp /= 3380; // 2
	ntc_temp += 1.0 / (25 + 273.15); // 3
	ntc_temp = 1.0 / ntc_temp; // 4
	ntc_temp -= 273.15; // 5
	return ntc_temp - _tempOffset;

}

void Adafruit_CCS811::setThresholds(uint16_t low_med, uint16_t med_high, uint8_t hysteresis)
{
	uint8_t buf[] = {(uint8_t)((low_med >> 8) & 0xF), (uint8_t)(low_med & 0xF),
	(uint8_t)((med_high >> 8) & 0xF), (uint8_t)(med_high & 0xF), hysteresis};
	
	this->write(CCS811_THRESHOLDS, buf, 5);
}

void Adafruit_CCS811::SWReset()
{
	//reset sequence from the datasheet
	uint8_t seq[] = {0x11, 0xE5, 0x72, 0x8A};
	this->write(CCS811_SW_RESET, seq, 4);
}

bool Adafruit_CCS811::checkError()
{
	_status.set(read8(CCS811_STATUS));
	return _status.ERROR;
}

void Adafruit_CCS811::write8(byte reg, byte value)
{
	this->write(reg, &value, 1);
}

uint8_t Adafruit_CCS811::read8(byte reg)
{
	uint8_t ret;
	this->read(reg, &ret, 1);
	
	return ret;
}

void Adafruit_CCS811::_i2c_init()
{
	Wire.begin();
}

void Adafruit_CCS811::read(uint8_t reg, uint8_t *buf, uint8_t num)
{
	uint8_t value;
	uint8_t pos = 0;
	
	//on arduino we need to read in 32 byte chunks
	while(pos < num){
		
		uint8_t read_now = min(32, num - pos);
		Wire.beginTransmission((uint8_t)_i2caddr);
		Wire.write((uint8_t)reg + pos);
		Wire.endTransmission();
		Wire.requestFrom((uint8_t)_i2caddr, read_now);
		
		for(int i=0; i<read_now; i++){
			buf[pos] = Wire.read();
			pos++;
		}
	}
}

void Adafruit_CCS811::write(uint8_t reg, uint8_t *buf, uint8_t num)
{
	Wire.beginTransmission((uint8_t)_i2caddr);
	Wire.write((uint8_t)reg);
	Wire.write((uint8_t *)buf, num);
	Wire.endTransmission();
}