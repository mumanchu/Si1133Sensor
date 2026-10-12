# Arduino Library for the Silicon Labs Si1133 UV, IR and ALS Sensor (obsolete)

> [!NOTE]
> The Silicon Labs Si1133 chip is now discontinued and is no longer manufactured. (They are not recommended for new designs :-)


## Installing the Library with the Arduino IDE

First, download the latest library Zip file from with the green button on the library's Github screen, "<> Code / Download Zip". This creates a file called "Si1133Sensor-main.zip" in your Downloads directory.

Now open the Arduino IDE. Select menu item "Sketch / Include Library > Add ZIP Library...". This opens the "Select ZIP File" window. Open the Downloads directory and choose the "Si1133Sensor-main.zip" file you just downloaded. You should see a "Successfully installed library" popup on the bottom right.

To open the example sketch, select menu item "File > Examples" and scroll down beyond "Examples from Custom Libraries" and choose the "Si1133Sensor > Si1133Sensor" entry. This opens the "Si1133Sensor.ino" example sketch in a new Arduino IDE window.

Here are some technical details...




# Class Reference

To find out what each method does, open the `src/Si1133Sensor.h` file and read the comments for each method.

```cpp
class Si1133Sensor
{
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
};
```

## References

**Data Sheet, Rev. 0.91** \
All page numbers refer to this version of the data sheet \
https://muman.ch/pub/Si1133.pdf

**AN968 Si1133 Optical Design Guide** \
UV Index calculation details, diffuser, etc. \
https://www.silabs.com/documents/public/application-notes/AN968-Si1133-UV-Index-Sensor-Electrical-and-Optical-Design-Guide.pdf


## Revision History

| Date  | Revision | Description |
|:---------- |:---------|:----------- |
| 2026.10.12 | 0.0.0	| Preliminary |

<br/>

## Joke of the Week




