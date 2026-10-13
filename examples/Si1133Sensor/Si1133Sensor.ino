///////////////////////////////////////////////////////////////////////////////
// Example Sketch for Si1133Sensor Library
// Copyright (C) 2026.10.12, https://muman.ch + https://github.com/mumanchu
// If you re-use this software, please include the above copyright notice

#include <Wire.h>

// Comment this out for release mode, to remove the log output
#define DEBUG

#ifdef DEBUG
#define LOGERROR(s) { Serial.println(s); Serial.flush(); }
#else
#define LOGERROR(s)
#endif

typedef unsigned long ulong;    // 32 bits
typedef unsigned int uint;      // 16 or 32 bits
typedef unsigned short ushort;  // 16 bits


#include "Si1133Sensor.h"

// Use inheritance to override configureChip() for your own application
class MySi1133Sensor : public Si1133Sensor
{
public:
	bool configureChip();
};

bool MySi1133Sensor::configureChip()
{
	// Your configuration
	static const Si1133_CONFIG defaultConfig[] =
	{
		//      Bit   76543210         76543210
		CHAN_LIST,  0b00001111,     // 00543210, 1 bit per channel, 0x00=all channels disabled
		// Channel 0, UV sensor
		ADCCONFIG0, 0b00011000,     // 0rrmmmmm, rr=DECIM_RATE, mmmmm=ADCMUX, p31
		ADCSENS0,   0b10000000,     // gssshhhh, g=HSIG, sss=SW_GAIN, hhhh=HW_GAIN, p32
		ADCPOST0,   0b01000000,     // 0nppp0tt, n=24BIT_OUT, ppp=POSTSHIFT, tt=THRESH_SEL, p33
		MEASCONFIG0,0b00000000,     // cc000000, cc=MEASCOUNTx index (00=FORCED or BURST mode), p34
		// Channel 1, LARGE_WHITE sensor
		ADCCONFIG1,	0b01001101,
		ADCSENS1,	0b10000000,
		ADCPOST1,	0b01000000,
		MEASCONFIG1,0b00000000,
		// Channel 2, MEDIUM_IR sensor
		ADCCONFIG2,	0b01000001,
		ADCSENS2,	0b10000000,
		ADCPOST2,	0b01000000,
		MEASCONFIG2,0b00000000,
		// Channel 3, WHITE sensor
		ADCCONFIG3,	0b01001011,
		ADCSENS3,	0b10000000,
		ADCPOST3,	0b01000000,
		MEASCONFIG3,0b00000000,

		// Forced mode
		MEASRATE_H, 0,				// 0=forced mode
		MEASRATE_L, 0,
		MEASCOUNT0, 0,				// 0=forced mode
		MEASCOUNT1, 0,
		MEASCOUNT2, 0,

		BURST,		0b00000000,		// 0x00 = burst disabled, 0x8ccc = burst enabled, ccc=burst count
	};

	if (!writeConfiguration(defaultConfig, sizeof(defaultConfig) / sizeof(Si1133_CONFIG)))
		return false;

	// calculate conversion time, for FORCED mode polling
	if (!calculateConversionTime(&conversionTimeMs))
		return false;

	// enable channel IRQs
	if (!writeReg(IRQ_ENABLE, 0b00001111))
		return false;

	return true;
}

MySi1133Sensor si1133;


void setup()
{
	// I use different pins for RX/TX logging
	// this is only for the STM32 Nucleo-64 boards
	#ifdef ARDUINO_NUCLEO_64
	Serial.setTx(PC_10);
	Serial.setRx(PC_11);
	#endif

	Serial.begin(115200);
	delay(3000);

	// PuTTY clear screen and scrollback
	Serial.print("\033[2J\033[H\033[3J");

	Serial.println("\n\rStarted\n\r");
	Serial.flush();

	// Some boards have no user LED
	#ifdef LED_BUILTIN
	pinMode(LED_BUILTIN, OUTPUT);
	#endif

	Wire.begin();
	Wire.setClock(400000);
	Wire.setTimeout(100);

	if (!si1133.begin(&Wire)) {
		Serial.println("si1133.begin() failed");
		Serial.flush();
		while (1) yield();
	}

	// custom configuration
	if (!si1133.configureChip()) {
		Serial.println("si1133.configureChip() failed");
		Serial.flush();
		while (1) yield();
	}
}


void pollSi1133()
{
	static bool measurementInProgress = false;
	static ulong pollStart = 0;
	char buf[128];

	// start next measurement
	if (!measurementInProgress) {
		pollStart = millis();
		si1133.forceSingleMeasurement();
		measurementInProgress = true;
		return;
	}

	// measurement in progress, is it time to poll the IRQ status?
	ulong pt = millis();
	if (pt - pollStart < si1133.conversionTimeMs)
		return;

	// wait 10ms until next poll
	pollStart += 10;
	//Serial.println(pollStart);

	// check IRQ, are the readings ready yet?
	byte irqStatus;
	if (!si1133.readIRQStatus(&irqStatus))
		return;
	if (irqStatus == 0)
		return;
	measurementInProgress = false;

	// get actual time between readings
	static ulong tStart = 0;
	ulong t = micros();
	ulong dt = tStart == 0 ? 0 : t - tStart;
	tStart = t;

	// new measurements available, toggle the LED
	#ifdef LED_BUILTIN
	digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
	#endif

	// read all measurements
	// in FORCED mode all readings are ready at the same time
	ulong values[4] = { 0 };
	uint numberOfValues;
	if (!si1133.readMeasurements(values, &numberOfValues, 4))
		return;

	ulong normalisedValues[4] = { 0 };
	if (!si1133.normaliseMeasurements(values, numberOfValues, normalisedValues))
		return;
		
	#if 0
	// DECIMAL

	// display raw values
	sprintf(buf, "%-08luus   UV=%-10lu  LW=%-10lu  IR=%-10lu  W=%-10lu",
		dt, values[0], values[1], values[2], values[3]);
	Serial.println(buf);
	Serial.flush();

	// display normalised values
	sprintf(buf, "             UV=%-10lu  LW=%-10lu  IR=%-10lu  W=%-10lu",
		normalisedValues[0], normalisedValues[1], normalisedValues[2], normalisedValues[3]);
	Serial.println(buf);
	
	#else 
	// HEX

	// display raw values
	sprintf(buf, "%-08luus   UV=%-8lX  LW=%-8lX  IR=%-8lX  W=%-8lX",
		dt, values[0], values[1], values[2], values[3]);
	Serial.println(buf);
	Serial.flush();

	// display normalised values
	sprintf(buf, "             UV=%-8lX  LW=%-8lX  IR=%-8lX  W=%-8lX",
		normalisedValues[0], normalisedValues[1], normalisedValues[2], normalisedValues[3]);
	Serial.println(buf);
	#endif

	// adjust gain according to the last measurements
	if (si1133.automaticGainControl(values, numberOfValues)) {
		sprintf(buf, "conversionTime=%lumS", si1133.conversionTimeMs);
		Serial.println(buf);
	}

	Serial.println();
	Serial.flush();
}


void loop()
{
	pollSi1133();


	#ifdef STM32_CORE_VERSION
	// Experimental, for STM32 HAL only
	// if I2C fails, it often hangs and the bus must be reset
	// do this here so it works for all I2C devices
	I2C_HandleTypeDef* i2cHandle = Wire.getHandle();
	if (i2cHandle && i2cHandle->ErrorCode != 0) {
		ulong clockSpeed = i2cHandle->Init.ClockSpeed;
		Wire.end();
		delay(100);
		Wire.begin();
		Wire.setClock(clockSpeed);
		Wire.setTimeout(100);
	}
	#endif

}

