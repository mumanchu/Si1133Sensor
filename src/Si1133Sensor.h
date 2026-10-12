#pragma once

///////////////////////////////////////////////////////////////////////////////
// Si1133 UV, IR and ALS Sensor (obsolete)
// Copyright (C) https://muman.ch and https://github.com/mumanchu, 2017, 2026
// If you re-use this code, please include the above copyright notice
/*
Full details of this Arduino-style library can be found here,
https://github.com/mumanchu/Si1133Sensor

And there's more in the original blog about UV Sensors
https://muman.ch/index.htm?muman-uv-sensors.htm

DATA SHEET, Rev. 0.91
Page numbers refer to this version of the data sheet,
https://muman.ch/pub/Si1133.pdf

AN968 Si1133 OPTICAL DESIGN GUIDE, etc.
https://www.silabs.com/documents/public/application-notes/AN968-Si1133-UV-Index-Sensor-Electrical-and-Optical-Design-Guide.pdf
*/

#include <Wire.h>


class Si1133Sensor
{
protected:
	TwoWire* wire;
	byte i2cAdds = 0x55;		// fixed I2C address (AD=VCC), alternate address 0x52 (AD=0V)

public:

	// Register Numbers
	typedef enum : byte
	{
		PART_ID = 0x00,			// 0x33 for Si1133
		HW_ID = 0x01,			// ssssiiii, ssss=silicon HW rev, iiii=implementation code
		REV_ID = 0x02,			// mmmmnnnn, mmmm=major, nnnn=minor
		HOSTIN0 = 0x0A,			// Parameter values for PARAM_SET command 
		COMMAND = 0x0B,			// see Si113_CMD
		IRQ_ENABLE = 0x0F,		// 00543210, IRQ enable bits for channels 5 4 3 2 1 0 when ready
		RESPONSE1 = 0x10,		// Parameter value for PARAM_QUERY, or parameter that was written for PARAM_SET
		RESPONSE0 = 0x11,		// rspecccc, r=running, s=suspend, p=sleep, e=CMD_ERR, cccc=CMD_CTR, see Note (1) below
		IRQ_STATUS = 0x12,		// 00543210, IRQx active bit for channels 5 4 3 2 1 0 when ready, cleared when read
		HOSTOUT0 = 0x13,		// Sensor readings according to CHAN_LIST, 16 or 24-bit values, MS byte first (bigstartian)
		//HOSTOUT1..HOSTOUT24	// 26 bytes in all, see ADCPOSTx..24BIT_OUT
		HOSTOUT25 = 0x2C

	} Si1133_REG;

	/* Note (1)
	If CMD_ERR = 0
	CMD_CTR :    0000 = Set to 0 by RESET_CMD_CTR command
	             0xxx = Incremented on each successful command, rolls over?
	             1111 = After power up or reset
	If CMD_ERR = 1
	CMD_CTR :    1000 = Invalid command
	             1001 = Invalid parameter number
	             1010 = ADC saturation or overflow
	             1011 = Output buffer overflow (burst mode)
	*/

	// Parameter Numbers
	typedef enum : byte
	{
		I2C_ADDR = 0x00,    // Can be programmed with a new I2C address, set to 0 on reset
		CHAN_LIST = 0x01,   // xx543210, 1=channel enabled, 0=disabled
		
		// The bits in these parameters are not defined as enums. You only use them once, 
		// read the relevant page numbers in the data sheet to get the details

		// Channel 0       Bit 76543210 (see data sheet p31..34)
		ADCCONFIG0 = 0x02,  // 0rrmmmmm, rr=DECIM_RATE, mmmmm=ADCMUX, p31
		ADCSENS0 = 0x03,    // gssshhhh, g=HSIG, sss=SW_GAIN, hhhh=HW_GAIN, p32
		ADCPOST0 = 0x04,    // 0nppp0tt, n=24BIT_OUT, ppp=POSTSHIFT, tt=THRESH_EN, p33
		MEASCONFIG0 = 0x05, // cc000000, cc=COUNTER_INDEX (00=FORCED or BURST mode), p34
		// Channel 1
		ADCCONFIG1 = 0x06,
		ADCSENS1 = 0x07,
		ADCPOST1 = 0x08,
		MEASCONFIG1 = 0x09,
		// Channel 2
		ADCCONFIG2 = 0x0A,
		ADCSENS2 = 0x0B,
		ADCPOST2 = 0x0C,
		MEASCONFIG2 = 0x0D,
		// Channel 3
		ADCCONFIG3 = 0x0E,
		ADCSENS3 = 0x0F,
		ADCPOST3 = 0x10,
		MEASCONFIG3 = 0x11,
		// Channel 4
		ADCCONFIG4 = 0x12,
		ADCSENS4 = 0x13,
		ADCPOST4 = 0x14,
		MEASCONFIG4 = 0x15,
		// Channel 5
		ADCCONFIG5 = 0x16,
		ADCSENS5 = 0x17,
		ADCPOST5 = 0x18,
		MEASCONFIG5 = 0x19,
		// Measurement rate counter, 16-bit value x 800uS
		MEASRATE_H = 0x1A,
		MEASRATE_L = 0x1B,
		// Measurement rate extension counters, selected by cc bits in MEASCONFIGx
		MEASCOUNT0 = 0x1C,
		MEASCOUNT1 = 0x1D,
		MEASCOUNT2 = 0x1E,
		// 16-bit threshold levels, selected by tt bits in ADCPOSTx
		THRESHOLD0_H = 0x25,
		THRESHOLD0_L = 0x26,
		THRESHOLD1_H = 0x27,
		THRESHOLD1_L = 0x28,
		THRESHOLD2_H = 0x29,
		THRESHOLD2_L = 0x2A,
		// Burst enable count, bits 6..0, 00xxxxxx
		BURST = 0x2B

	} Si1133_PARAM;

	// Values for ADCCONFIx::ADCMUX, p31
	typedef enum : byte
	{
		SMALL_IR = 0x00,
		MEDIUM_IR = 0x01,
		LARGE_IR = 0x02,
		WHITE = 0x0B,
		LARGE_WHITE = 0x0D,
		UV = 0x18,
		UV_DEEP = 0x19

	} Si1133_ADC_MUX;

	// Commands, written to COMMAND register
	typedef enum : byte
	{
		RESET_CMD_CTR = 0x00,	// Reset RESPONSE0 CMD_CTR field to 0 and does nothing else
		RESET_SW = 0x01,		// Force a power-up reset, RESPONSE0 CMD_CTR set to xxx01111
		FORCE = 0x11,			// Initiate CHAN_LIST measurements that don't have MEASCONFIGx
		PAUSE = 0x12,			// Pause autonomous measurements specified in CHAN_LIST
		START = 0x13,			// Start autonomous measurements specified in CHAN_LIST
		PARAM_QUERY = 0x40,		// 01xxxxxx, read param xxxxxx to RESPONSE1
		PARAM_SET = 0x80		// 10xxxxxx, writeINPUT0 to param xxxxxx

	} Si1133_CMD;

	// RESPONSE0 register flags, p17
	typedef enum : byte
	{
		RUNNING = 0b10000000,
		SUSPEND = 0b01000000,
		SLEEP   = 0b00100000,
		CMD_ERR = 0b00010000,	// Cleared by RESET or RESET_CMD_CTR
		CMD_CTR = 0b00001111	// Bits 3..0, command counter, or error code if CMD_ERR = 1

	} Si1133_RESPONSE0_FLAG;

	// Data for writeConfiguration(), parameter number/parameter value pairs
	#pragma pack(push, 1)
	typedef struct {
		Si1133_PARAM paramNumber;
		byte paramValue;
	} Si1133_CONFIG;
	#pragma pack(pop)

	// Constants for UV Index calculation, calculateUVI()
	// - modify K according to the diffuser characteristics
	// - calculate M according to the settings (TODO)
	// These values are for a typical diffuser, ADC gain = 9, decimation filter = 3
	// see Data Sheet p5 and the AN968 Design Guide
	float uviK = 0.0187f;
	float uviM = 0.00391f;

	// cmdError error code, set non-zero if a command fails, p17
	// check this value if a method returns false
	// 0x00 = command executed successfully
	// 0x10 = invalid command
	// 0x11 = invalid parameter number
	// 0x12 = ADC saturation or overflow
	// 0x13 = output buffer overflow, burst mode data > 26 bytes
	// 0x30 = I2C failed } additional values
	// 0x31 = timeout    }
	uint cmdErr = 0;
	uint cmdErrCounter = 0;		// counts the number of CMD_ERR non-zero events since power-up

	// Conversion time in milliseconds for all channels
	// VALID FOR FORCED MODE ONLY!
	// this is updated by automaticGainControl() for polling purposes
	// or initialize it yourself with calculateConversionTime()
	ulong conversionTimeMs = 0;

	// Override this in a derived class to code your own configuration,
	// see the example below
	virtual bool configureChip();

	bool begin(TwoWire* twoWire);
	bool softwareReset();
	bool writeConfiguration(const Si1133_CONFIG* config, uint configLength);
	bool writeIRQEnable(byte irqEnable);
	bool readIRQStatus(byte* irqStatus);
	bool forceSingleMeasurement();
	bool startAutonomousMeasurements();
	bool pauseAutonomousMeasurements();
	uint getNumberOfChannels();
	bool readMeasurements(ulong* values, uint* numberOfValues);
	bool automaticGainControl(ulong* values, uint numberOfValues);
	bool normaliseMeasurements(const ulong* values, uint numberOfValues, ulong* normalisedValues);
	bool calculateConversionTime(ulong* conversionTimeMs);
	bool calculateLUX(ulong rawVis, ulong rawIr, float* lux);
	bool calculateUVI(ulong rawUv, float* uvi);

protected:
	// Shadow copy of the Si1133's parameter values
	// these are needed for measurements and calculations
	byte parameterShadow[BURST + 1];
	bool readShadowParameters();

	bool awaitSleep();
	bool resetCmdCtr();
	bool awaitCmdCtr();
	bool writeParam(Si1133_PARAM paramNumber, byte paramValue);
	bool readParam(Si1133_PARAM paramNumber, byte* paramValue);
	bool writeCommand(Si1133_CMD cmd);
	bool writeReg(Si1133_REG reg, byte data);
	bool writeReg(Si1133_REG reg, const byte* data, uint length = 1);
	bool readReg(Si1133_REG reg, byte* data, uint length = 1);
	bool pollReg(Si1133_REG reg, byte* data, bool first);

};


// Startup
// - check an Si1133 is connected
// - do a software reset so all parameters are set to defaults
// - take a copy of the shadow parameters
// Remains in SLEEP mode after the reset.
bool Si1133Sensor::begin(TwoWire* twoWire)
{
	wire = twoWire;
	memset(parameterShadow, 0, sizeof(parameterShadow));

	cmdErr = 0;
	cmdErrCounter = 0;
	conversionTimeMs = 0;

	// no access until 30mS after power up
	//while (millis() < 30)
	//	yield();

	// check the Si1133 is connected, part id should be 0x33
	byte partId;
	if (!readReg(PART_ID, &partId) || partId != 0x33)
		return false;

	// reset parameters and registers to default values
	if (!softwareReset())
		return false;

	// read parameter shadow data, the default configuration after RESET_SW
	if (!readShadowParameters())
		return false;

	return true;
}

// Reset parameters and registers to default values
// the RESPONSE0 register will show RUNNING immediately after reset 
// and then SLEEP after initialization is complete
bool Si1133Sensor::softwareReset()
{
	// RESPONSE0 CMD_CTR field is set to 0bxxx01111, checked below
	if (!writeReg(COMMAND, RESET_SW))
		return false;
	delay(30);			// startup sequence delay, min. 25mS

	// verify CMD_CTR was set to 0x0F
	// rspecccc, r=running, s=suspended, p=sleeping, e=CMD_ERR, cccc=CMD_CTR
	byte response0;
	if (!readReg(RESPONSE0, &response0))
		return false;
	if ((response0 & CMD_CTR) != 0x0F)
		return false;
	return true;
}

// Override this in a derived class, see the example below
bool Si1133Sensor::configureChip()
{
	LOGERROR("override configureChip() in a derived class");
	return false;
}


// Inheritance Example
// copy/paste this into your sketch and edit the configuration
#if 0

// Use inheritance to override configureChip() for your own application
class MySi1133Sensor : public Si1133Sensor
{
public:
	bool configureChip();
};

bool MySi1133Sensor::configureChip()
{
	// for autonomouse mode, measurement rate x 800uS
	//const ushort measRate = 1250;	// 1s / 800us

	// Your configuration
	static const Si1133_CONFIG defaultConfig[] =
	{
		//     Bit   76543210          76543210
		CHAN_LIST,	0b00001111,     // 00543210, 1 bit per channel, 0x00=all channels disabled
		// Channel 0, UV sensor
		ADCCONFIG0,	0b00011000,     // 0rrmmmmm, rr=DECIM_RATE, mmmmm=ADCMUX, p31
		ADCSENS0,	0b10000001,     // gssshhhh, g=HSIG, sss=SW_GAIN, hhhh=HW_GAIN, p32
		ADCPOST0,	0b01000000,     // 0nppp0tt, n=24BIT_OUT, ppp=POSTSHIFT, tt=THRESH_EN, p33
		MEASCONFIG0,0b00000000,     // cc000000, cc=MEASCOUNTx index (00=FORCED or BURST mode), p34
		// Channel 1, LARGE_WHITE sensor
		ADCCONFIG1,	0b00001101,
		ADCSENS1,	0b10000000,
		ADCPOST1,	0b01000000,
		MEASCONFIG1,0b00000000,
		// Channel 2, MEDIUM_IR sensor
		ADCCONFIG2,	0b00000001,
		ADCSENS2,	0b10000000,
		ADCPOST2,	0b01000000,
		MEASCONFIG2,0b00000000,
		// Channel 3, WHITE sensor
		ADCCONFIG3,	0b00001011,
		ADCSENS3,	0b10000000,
		ADCPOST3,	0b01000000,
		MEASCONFIG3,0b00000000,

		// Forced mode
		MEASRATE_H, 0,				// 0=forced mode
		MEASRATE_L, 0,
		MEASCOUNT0, 0,				// 0=forced mode
		MEASCOUNT1, 0,
		MEASCOUNT2, 0,
		// (for autonomous mode, 800uS * MEASRATE * MEASCOUNT, p34)
		//MEASRATE_H, (byte)(measRate >> 8),
		//MEASRATE_L, (byte)measRate,

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
#endif


// Write a configuration array to the Parameter Table, [param, data]
bool Si1133Sensor::writeConfiguration(const Si1133_CONFIG* config, uint configLength)
{
	for (int i = 0; i < configLength; ++i) {
		if (!writeParam(config[i].paramNumber, config[i].paramValue))
			return false;
	}
	return true;
}

// FORCED mode, start a new measurement
// all IRQ_STATUS bits are set simultaneously when all measurements are complete
inline bool Si1133Sensor::forceSingleMeasurement()
{
	return writeCommand(FORCE);
}

// AUTONOMOUS or BURST mode, begin continuous timed measurements
// each IRQ_STATUS bit is set individually as each measurement becomes available
inline bool Si1133Sensor::startAutonomousMeasurements()
{
	return writeCommand(START);
}

// AUTONOMOUS or BURST mode, pause continuous timed measurements
inline bool Si1133Sensor::pauseAutonomousMeasurements()
{
	return writeCommand(PAUSE);
}

// Enable interrupts on the irqEnable channels
// irqEnable should match CHAN_LIST bits
inline bool Si1133Sensor::writeIRQEnable(byte irqEnable)
{
	return writeReg(IRQ_ENABLE, irqEnable);
}

// Read the channels with IRQ pending
// irqStatus should match CHAN_LIST bits
inline bool Si1133Sensor::readIRQStatus(byte* irqStatus)
{
	return readReg(IRQ_STATUS, irqStatus);
}

// Read all parameter values and save them in the parameterShadow[] array
bool Si1133Sensor::readShadowParameters()
{
	for (int i = 0; i <= BURST; ++i) {
		if (!readParam((Si1133_PARAM)i, parameterShadow + i))
			return false;
	}
	return true;
}

// Return the number of channels that have been configured in CHAN_LIST 
// from the shadow parameter values
// Use this to get the length of the readMeasurements() 'values' array
// or to verify the configuration is correct
uint Si1133Sensor::getNumberOfChannels()
{
	uint channelMask = 1;
	uint numberOfChannels = 0;
	uint chanList = parameterShadow[CHAN_LIST];
	for (uint channel = 0; channel < 6; ++channel) {
		// 1 = channel is configured
		if (chanList & channelMask)
			++numberOfChannels;
		channelMask <<= 1;
	}
	return numberOfChannels;
}


// IMPORTANT!
// The chip should be in SLEEP mode (in between readings) when measurements
// are read, so they are not read at the moment they are changing. This is 
// NOT checked for in the code below. But if IRQ_STATUS has just been set 
// then it is safe.

// Read all measurements with one I2C burst message, always reads 18 bytes.
// Do not call this until you know the measurements are ready, check
// readIRQStatus() first and use the 'conversionTimeMs' value to limit the 
// polling rate.
// The number of values returned depends on the channel configuration, it is 
// not hard-wired.
// ulong* must point to a ulong array the size of the number of channels.
// 16 and 24-bit values are both returned in ulong, but 16-bit values only 
// use the lower 16 bits.
// Saturation/overflow is returned as 0xFFFFFFFF (-1) for both 16 and 24-bit 
// values (not 0xFFFF for 16-bit and 0x7FFFFF for 24-bit).
bool Si1133Sensor::readMeasurements(ulong* values, uint* numberOfValues)
{
	*numberOfValues = 0;

	// BURST mode is not supported, this code only works for one set 
	// of channel readings
	if (parameterShadow[BURST] & 0x80)
		return false;

	// read all possible measurements, 6 channels * 24 bits = 18 bytes
	// 18 bytes in auto-increment-address-mode, the overhead is not significant
	// (it's usually around 12 bytes, depending on the configuration)
	byte data[18];
	if (!readReg(HOSTOUT0, data, 18))
		return false;

	// create an array of ulong values from the 16 or 24-bit data for each
	// channel, according to the configuration
	byte* dataPtr = data;
	uint channelMask = 1;
	uint numberOfChannels = 0;
	uint chanList = parameterShadow[CHAN_LIST];

	for (uint channel = 0; channel < 6; ++channel) {
		// if channel is configured, return the measurement, 16 or 24 bits
		if (chanList & channelMask) {
			ulong value;
			byte adcPost = parameterShadow[ADCPOST0 + (channel * 4)];
			if (adcPost & 0x40) {
				// 24-bit signed value
				value = (dataPtr[0] << 16) + (dataPtr[1] << 8) + dataPtr[2];
				if (value & 0x800000)	// -ve value? set it to zero
					value = 0;
				if (value == 0x7FFFFF)	// 24-bit saturation/overflow
					value = 0xFFFFFFFF;
				dataPtr += 3;
			}
			else {
				// 16-bit unsigned value
				value = (dataPtr[0] << 8) + dataPtr[1];
				if (value == 0xFFFF)	// 16-bit saturation/overflow
					value = 0xFFFFFFFF;
				dataPtr += 2;
			}
			values[numberOfChannels++] = value;
		}
		channelMask <<= 1;
	}
	*numberOfValues = numberOfChannels;
	return true;
}

// Adjust the gain of each channel independently to ensure the optimal range is 
// selected. Because the channel gains will be changing, it's important to use 
// normaliseMeasurements() to compensate for the different gains of each channel.
// This feature only works in FORCED mode.
// As light/UV/IR levels fall, the gains increase and the conversion time also
// increases. Use the 'conversionTimeMs' value to regulate the IRQ_STATUS polling.
// 'values' & 'numberOfValues' are the raw values returned by readMeasurements().
// The DECIM_RATE remains fixed.
// TODO also update the M factor used by calculateUVI().
bool Si1133Sensor::automaticGainControl(ulong* values, uint numberOfValues)
{
	uint channelMask = 1;
	uint chanList = parameterShadow[CHAN_LIST];
	uint ixValue = 0;
	bool gainChanged = false;

	// total conversion time in microseconds, for IRQ_STATUS polling
	ulong conversionTimeUs = 0;

	// check each channel
	for (uint channel = 0; channel < 6; ++channel) {

		// if channel is configured
		if (chanList & channelMask) {
			if (ixValue >= numberOfValues) {
				LOGERROR("not enough values");
				return false;
			}
			ulong value = values[ixValue++];

			// Channel x  Bit 76543210 (see data sheet p31..34)
			// ADCCONFIGx     0rrmmmmm, rr=DECIM_RATE, mmmmm=ADCMUX, p31
			// ADCSENSx       gssshhhh, g=HSIG, sss=SW_GAIN, hhhh=HW_GAIN, p32
			// ADCPOSTx       0nppp0tt, n=24BIT_OUT, ppp=POSTSHIFT, tt=THRESH_SEL, p33
			// MEASCONFIG     cc000000, cc=COUNTER_INDEX (00=FORCED or BURST mode), p34

			// register numbers for this channel
			uint adcConfigParam = ADCCONFIG0 + (channel * 4);
			uint adcSensParam = adcConfigParam + 1;
			uint adcPostParam = adcConfigParam + 2;
			uint measConfigParam = adcConfigParam + 3;

			// not supported for autonomous mode
			if ((parameterShadow[measConfigParam] & 0b11000000) != 0) {
				LOGERROR("FORCED mode only");
				return false;
			}
			// swGain : number of accumulated samples in 24 bits, with optional shift
			byte adcSens = parameterShadow[adcSensParam];
			uint swGain = (adcSens >> 4) & 0x07;
			if (swGain > 0) {		// AGC is not supported for channel's using SW_GAIN
				LOGERROR("automaticGainControl not supported for SW_GAIN");
				return false;
			}
			// hwGain : 0..11, decimRate x 2^hwGain
			uint hwGain = adcSens & 0x0F;

			// The UV channel behaves differently...
			// At low UV values with high ALS levels (e.g. full sunlight through a 
			// glass window which filters the UV), the UV reading is "all over the 
			// place", jumping between 0 and a high value with consecutive readings. 
			// This can change the gains and cause oscillation, so some kind of 
			// damping is required.
			uint adcMux = parameterShadow[adcConfigParam] & 0x1F;
			bool uvChannel = adcMux == Si1133_ADC_MUX::UV || adcMux == Si1133_ADC_MUX::UV_DEEP;
			if (uvChannel) {
				//TODO UV damping to prevent oscillation
				// [in progress]

			}

			// should we change the gain for this channel?
			bool changeGain = false;
			bool increaseGain = false;
			bool decreaseGain = false;
			// switching points depend on 16 or 24 bits
			if (parameterShadow[adcPostParam] & 0b01000000) {
				// 24 bit signed value (already set to 0 if -ve, see readMeasurements())
				increaseGain = value < 0xFFF;
				decreaseGain = value > 0xFFFFF;
			}
			else {
				// 16 bit unsigned value
				increaseGain = value < 0xFF;
				decreaseGain = value > 0xFFF;
			}
			if (increaseGain) {
				if (hwGain < 11) {
					++hwGain;
					changeGain = true;
				}
				else if (adcSens & 0x80) {
					// max gain, clear HSIG
					adcSens &= ~0x80;
					changeGain = true;
				}
			}
			else if (decreaseGain) {
				if (hwGain > 0) {
					--hwGain;
					changeGain = true;
				}
				else if (!(adcSens & 0x80)) {
					// min gain, set HSIG
					adcSens |= 0x80;
					changeGain = true;
				}
			}

			// if changed, update the parameter
			if (changeGain) {
				gainChanged = true;

				#ifdef DEBUG
				char buf[128];
				sprintf(buf, "gain %s : channel=%u gain=%u", increaseGain ? "incresed" : "decreased",
					channel, hwGain);
				LOGERROR(buf);
				#endif

				adcSens = (adcSens & 0xF0) + hwGain;
				if (!writeParam((Si1133_PARAM)(adcSensParam), adcSens))
					return false;
			}

			// update the total conversion time in microseconds
			// measurement times in microseconds, p31
			static const byte measTime[4] = { 50, 98, 195, 25 };
			uint decimRate = (parameterShadow[adcConfigParam] >> 5) & 0x03;
			// conversion time in microseconds, 
			// two readings are taken for each measurement
			conversionTimeUs += measTime[decimRate] * (1 << hwGain) * 2;
			// add processing time per measurement 155uS, p35
			conversionTimeUs += (155 * 2);
		}
		channelMask <<= 1;
	}
	if (ixValue != numberOfValues)
		LOGERROR("bug, too many values");

	// conversion time in milliseconds for the next set of readings
	conversionTimeMs = (conversionTimeUs + 1000) / 1000;
	
	return gainChanged;
}

// Adjust the measurements according to the gain etc.
// All are normalised to the highest gain x128, for other gains just shift 
// the 32-bit results right.
// 'values' must point to the raw values returned by readMeasurements()
// 'numberOfValues' must match the number of channels
bool Si1133Sensor::normaliseMeasurements(const ulong* values, uint numberOfValues, 
	ulong* normalisedValues)
{
	if (numberOfValues == 0 || numberOfValues > 6)
		return false;

	// set all to normalised values to overflow in case normalisation fails
	memset(normalisedValues, 0xFF, numberOfValues * sizeof(ulong));

	uint channelMask = 1;
	uint chanList = parameterShadow[CHAN_LIST];
	uint ixValue = 0;

	// check each channel
	for (uint channel = 0; channel < 6; ++channel) {

		// if channel is configured
		if (chanList & channelMask) {
			if (ixValue >= numberOfValues) {
				LOGERROR("bug, not enough values");
				return false;
			}
			ulong value = values[ixValue];

			// overflow, leave as 0xFFFFFFFF
			if (value == 0xFFFFFFFF) {
				normalisedValues[ixValue++] = value;
				channelMask <<= 1;
				continue;
			}

			// Channel x  Bit 76543210 (see data sheet p31..34)
			// ADCCONFIGx     0rrmmmmm, rr=DECIM_RATE, mmmmm=ADCMUX, p31
			// ADCSENSx       gssshhhh, g=HSIG, sss=SW_GAIN, hhhh=HW_GAIN, p32
			// ADCPOSTx       0nppp0tt, n=24BIT_OUT, ppp=POSTSHIFT, tt=THRESH_SEL, p33
			// MEASCONFIGx    cc000000, cc=COUNTER_INDEX (00=FORCED or BURST mode), p34

			// register numbers for this channel
			uint adcConfigParam = ADCCONFIG0 + (channel * 4);
			uint adcSensParam = adcConfigParam + 1;
			uint adcPostParam = adcConfigParam + 2;
			uint measConfigParam = adcConfigParam + 3;

			// normalisation is not supported for autonomous mode
			if ((parameterShadow[measConfigParam] & 0b11000000) != 0) {
				LOGERROR("FORCED mode only");
				return false;
			}

			// swGain : number of accumulated samples in 24 bits, with optional shift
			byte adcSens = parameterShadow[adcSensParam];
			uint swGain = (adcSens >> 4) & 0x07;
			uint postShift = (parameterShadow[adcPostParam] >> 3) & 0x07;
			// hwGain : 0..11, decimRate x 2^hwGain
			uint hwGain = adcSens & 0x0F;
			// 16 or 24 bits
			bool bits24 = (parameterShadow[adcPostParam] & 0b01000000) != 0;

			// HSIG active, reading is 14.5x smaller than actual, so multiply by 14.5
			if (adcSens & 0x80)
				value = (value * 14500L) / 1000;

			// SW_GAIN : number of accumulated samples in 24 bits, with optional shift right
			// (in FORCED mode, IRQ is not asserted until all samples taken and postShift was done)
			if (swGain) {
				// if postShift divides by the number of samples, do nothing (shift==0)
				int shift = swGain - postShift;
				//TODO are these really configuration errors?
				// shift right is greater than the accumulated samples, so shift left
				if (shift < 0)
					value <<= -shift;
				// shift right is less than the accumulated samples, so shift right
				else if (shift > 0)
					value >>= shift;
			}

			// multiplied by hardware gain (power of 2)
			if (hwGain < 11)
				value <<= (11 - hwGain);

			// normalised value!
			normalisedValues[ixValue++] = value;
		}
		channelMask <<= 1;
	}
	if (ixValue != numberOfValues)
		LOGERROR("bug, too many values");
	return true;
}

// Calculate the total conversion time in milliseconds for all channels
// >>> FOR FORCED MODE ONLY! <<<
// It takes into account the defined channels, HW_GAIN, SW_GAIN and DECIM_RATE.
// Note that the conversion time is also calculated when automaticGainControl() 
// is called, so you can adjust the IRQ_STATUS polling rate according to the
// conversion time.
// TODO This always comes out about 2..4% too low when compared to the actual 
// conversion time.
bool Si1133Sensor::calculateConversionTime(ulong* conversionTimeMs)
{
	uint channelMask = 1;
	uint chanList = parameterShadow[CHAN_LIST];

	// total conversion time in microseconds, for IRQ_STATUS polling
	ulong conversionTimeUs = 0;

	// check each channel
	for (uint channel = 0; channel < 6; ++channel) {

		// if channel is configured
		if (chanList & channelMask) {

			// Channel x  Bit 76543210 (see data sheet p31..34)
			// ADCCONFIGx     0rrmmmmm, rr=DECIM_RATE, mmmmm=ADCMUX, p31
			// ADCSENSx       gssshhhh, g=HSIG, sss=SW_GAIN, hhhh=HW_GAIN, p32
			// ADCPOSTx       0nppp0tt, n=24BIT_OUT, ppp=POSTSHIFT, tt=THRESH_SEL, p33
			// MEASCONFIG     cc000000, cc=COUNTER_INDEX (00=FORCED or BURST mode), p34

			// register numbers for this channel
			uint adcConfigParam = ADCCONFIG0 + (channel * 4);
			uint adcSensParam = adcConfigParam + 1;
			uint measConfigParam = adcConfigParam + 3;

			// not supported for autonomous mode
			if ((parameterShadow[measConfigParam] & 0b11000000) != 0) {
				LOGERROR("FORCED mode only");
				return false;
			}
			// swGain : number of accumulated samples in 24 bits, with optional shift
			byte adcSens = parameterShadow[adcSensParam];
			uint swGain = (adcSens >> 4) & 0x07;
			// hwGain : 0..11, decimRate x 2^hwGain
			uint hwGain = adcSens & 0x0F;

			// measurement times in microseconds, rounded up, p31
			static const byte measTime[4] = { 50, 98, 195, 25 };
			uint decimRate = (parameterShadow[adcConfigParam] >> 5) & 0x03;
			// conversion time in microseconds,
			// two readings are taken for each measurement, first is dark channel?
			ulong ctus = measTime[decimRate] * (1 << hwGain) * 2;
			// add processing time per measurement 155uS, p35
			ctus += (155 * 2);
			// SW_GAIN, multiply by the number of samples
			if (swGain)
				ctus *= 1 << swGain;
			conversionTimeUs += ctus;
		}
		channelMask <<= 1;
	}

	// conversion time in milliseconds for the next set of readings
	*conversionTimeMs = (conversionTimeUs + 1000) / 1000;

	return true;		// amazing, it worked!
}

//TODO This is your homework :-)
bool Si1133Sensor::calculateLUX(ulong rawVis, ulong rawIr, float* lux)
{
	*lux = NAN;
	return true;
}

// The UV Index equation depends on the sensor's cover/diffuser, the ADC gain 
// and the decimation filter settings.
// For a typical diffuser, ADC gain = 9, decimation filter = 3, then this is 
// the algorithm (data sheet p5),
//    UV index = 0.0187 * ((0.00391 * Input^2) + Input))
// See also AN968: Si1133 UV Index Sensor Electrical and Optical Design Guide, p9
bool Si1133Sensor::calculateUVI(ulong rawUv, float* uvi)
{
	if (rawUv == -1) {
		*uvi = NAN;
		return false;
	}

	//TODO modify K according to the diffuser characteristics
	//TODO adjust rawUv and calculate M according to the settings
	
	*uvi = uviK * ((uviM * rawUv * rawUv) + rawUv);
	return true;
}


////////////////////////////////////////////////////////////
// Internal Methods
// "There is madness in his methods" - Liamwil Peareshakes

// Reset the command counter CMD_CTR in RESPOSE0 to zero
// the next valid command increments it to 1
// poll the CMD_CTR with awaitCmdCtr()
bool Si1133Sensor::resetCmdCtr()
{
	cmdErr = 0x30;			// 0x30 = I2C failed
	return writeReg(COMMAND, RESET_CMD_CTR);
}

// After every command, poll RESPONSE0 and check the CMD_CTR field
// CMD_CTR was incremented when the command completes successfully,
// or the CMD_ERR bit is set and it contains an error code, p17
// errorCode : 0x00 = command executed successfully
//             0x10 = invalid command
//             0x11 = invalid parameter number
//             0x12 = ADC saturation or overflow
//             0x13 = output buffer overflow, burst mode data > 26 bytes
//             0x30 = I2C failed } additional values
//             0x31 = timeout    }
bool Si1133Sensor::awaitCmdCtr()
{
	cmdErr = 0x30;			// 0x30 = I2C failed
	ulong t = millis();
	bool first = true;
	while (1) {
		yield();
		byte response0;
		if (!pollReg(RESPONSE0, &response0, first))
			break;
		first = false;
		if (response0 & CMD_ERR) {	// error
			cmdErr = response0 & 0x1f;
			break;
		}
		if ((response0 & CMD_CTR) == 1) {
			cmdErr = 0x00;			// incremented to 1, success
			return true;
		}
		if (millis() - t > 1000) {
			cmdErr = 0xfe;			// 0xfe = timed out
			break;
		}
	}
	// failed, RESET_CMD_CTR is sent at start of next command
	++cmdErrCounter;				// count the number of errors
	return false;
}

// Wait until the Si1133 is in SLEEP mode, with timeout
// This is a naughty blocking function, best not to use it
bool Si1133Sensor::awaitSleep()
{
	ulong t = millis();
	bool first = true;
	while (1) {
		byte response0;
		if (!pollReg(RESPONSE0, &response0, first))
			break;
		first = false;
		if (response0 & SLEEP)		// in SLEEP mode
			return true;
		if (millis() - t >= 1000)	// timed out
			break;
		yield();
	}
	return false;
}

// Write to COMMAND register and poll RESPONSE0 until CMD_CTRL field is incremented
bool Si1133Sensor::writeCommand(Si1133_CMD cmd)
{
	// set command counter to 0
	if (!resetCmdCtr())
		return false;
	// write COMMAND register
	if (!writeReg(COMMAND, cmd))
		return false;
	// poll RESPONSE0 and check the CMD_CTR field was incremented
	// set errorCode to the result
	return awaitCmdCtr();
}

// Write one parameter
bool Si1133Sensor::writeParam(Si1133_PARAM paramNumber, byte paramValue)
{
	// set command counter to 0
	if (!resetCmdCtr())
		return false;
	// write data & PARAM_SET bit | parameter number to HOSTIN0 & COMMAND registers
	byte b[2] = { paramValue, PARAM_SET | paramNumber };
	if (!writeReg(HOSTIN0, b, 2))
		return false;
	// poll RESPONSE0 and check the CMD_CTR field was incremented
	// set errorCode to the result
	if (!awaitCmdCtr())
		return false;

	// update parameter shadow data
	parameterShadow[paramNumber] = paramValue;

	#ifdef DEBUG
	// debug test only, check RESPONSE1 contains 'paramValue'
	byte response1;
	if (!readReg(RESPONSE1, &response1))
		return false;
	if (response1 != paramValue)
		return false;
	#endif

	return true;
}

// Read one parameter
bool Si1133Sensor::readParam(Si1133_PARAM paramNumber, byte* paramValue)
{
	*paramValue = 0;
	if (!writeCommand((Si1133_CMD)(PARAM_QUERY | paramNumber)))
		return false;
	// return the data byte
	return readReg(RESPONSE1, paramValue);
}

// Write one register
inline bool Si1133Sensor::writeReg(Si1133_REG reg, byte data)
{
	return writeReg(reg, &data);
}

// Write one or more registers
bool Si1133Sensor::writeReg(Si1133_REG reg, const byte* data, uint length /*=1*/)
{
	wire->beginTransmission(i2cAdds);
	wire->write((byte)reg);
	if (wire->write(data, length) != length) {
		LOGERROR("write() failed");
		return false;
	}
	if (wire->endTransmission() != 0) {
		LOGERROR("endTransmission() failed");
		return false;
	}
	return true;
}

// Read one or more registers
bool Si1133Sensor::readReg(Si1133_REG reg, byte* data, uint length /*=1*/)
{
	memset(data, 0, length);		// return 0s if it fails

	wire->beginTransmission(i2cAdds);
	wire->write((byte)reg);			// default is auto-increment address
	if (wire->endTransmission(false) != 0) {
		LOGERROR("endTransmission() failed");
		return false;
	}
	wire->requestFrom((int)i2cAdds, length);
	if (wire->readBytes((byte*)data, length) != length) {
		LOGERROR("readBytes() failed");
		return false;
	}
	return true;
}

// Poll the same register, sending the address only once when first=true, 
// uses "auto increment disable"
bool Si1133Sensor::pollReg(Si1133_REG reg, byte* data, bool first)
{
	*data = 0;

	// send address on first call, with auto increment disabled
	if (first) {
		wire->beginTransmission(i2cAdds);
		// 0x40 = auto increment disable
		wire->write((byte)reg | 0x40);
		if (wire->endTransmission(false) != 0) {
			LOGERROR("endTransmission() failed");
			return false;
		}
	}
	wire->requestFrom((int)i2cAdds, 1);
	int b = wire->read();
	if (b < 0) {
		LOGERROR("read() failed");
		return false;
	}
	*data = (byte)b;
	return true;
}

