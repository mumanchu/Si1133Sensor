# Arduino Library for the Silicon Labs Si1133 UV, IR and ALS Sensor (Obsolete)

> [!NOTE]
> The Silicon Labs Si1133 chip is now discontinued and is no longer manufactured. (They are not recommended for new designs :-)

<img width="500" height="311" alt="image" src="https://github.com/user-attachments/assets/4feb4436-5b70-4d10-9c36-a11f0728a7e4" />

This really tiny chip (2x2mm) was quite nice. It contained three sensor types, UV, IR and visible light. These were available through seven selectable channels: Small IR, Medium IR, Large IR, White, Large White, UV and UV Deep. The UV index sensor had a sensitivity that _matches the erythemal curve_.

I found only one breakout board that's still available, the strange-looking "UV 4 click" board (above right) from MIKROE (CHF14.50 from Digikey). You can see that the chip at the top of the MIKROE board is the same size as an SMD resistor, but with 10 solder pads instead of two. You can still buy the discrete chips while stocks last (CHF 2.53, or a rip-off CHF 14.-), but good luck soldering it by hand!

## Installing the Library with the Arduino IDE

First, download the latest library Zip file from with the green button on the library's Github screen, "<> Code / Download Zip". This creates a file called "Si1133Sensor-main.zip" in your Downloads directory.

Now open the Arduino IDE. Select menu item "Sketch / Include Library > Add ZIP Library...". This opens the "Select ZIP File" window. Open the Downloads directory and choose the "Si1133Sensor-main.zip" file you just downloaded. You should see a "Successfully installed library" popup on the bottom right.

To open the example sketch, select menu item "File > Examples" and scroll down beyond "Examples from Custom Libraries" and choose the "Si1133Sensor > Si1133Sensor" entry. This opens the "Si1133Sensor.ino" example sketch in a new Arduino IDE window.


## Class Reference

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

## Example Sketch

The example shows how to use inheritance to derive a new `Si1133Sensor` class and override the `configureChip()` method for your own custom configuration.

It then polls the chip's IRQ_STATUS according to the `conversionTimeMs` value and displays the measurements in decimal or hex.

`automaticGainControl()` is used to adjust the gain for the optimum resolution.

<br/>

Here are a few points to note...

## FORCED Mode and AUTONOMOUS Mode

Sampling can be done on request (FORCED mode) or automatically at a configured rate (AUTONOMOUS mode). 

In forced mode, all channels are read sequentially and the `IRQ_STATUS` bits are set simultaneously for ALL channels when all channels have been read. That means there can be quite a long time (up to several seconds) between readings, depending on the gain and decimation settings. Forced mode is required if you are using the `automaticGainControl()` feature.

In autonomous mode, each channel bit in `IRQ_STATUS` is set as each channel's reading is taken, so each reading can be read individually. But it still takes the same amount of time to obtain all the readings.

In both cases you must poll IRQ_STATUS or the INT output to determine when a reading is ready, to avoid reading the values at the moment they may be changing.


## SW_GAIN (0..7 = 1..128), p32

This is not really a gain, it is an accumulation of readings for averaging. After the number of readings has been taken the result can be shifted right to divide by the number of readings taken by setting the `ADCPOSTx.POSTSHIFT` value (0..7) to match the number of accumulated readings (0..7 = 1..128). Or you can do that yourself after reading the 24-bit total.

Number of readings to sum = `(1 << SW_GAIN)`

## HW_GAIN (0..11), p32

This is a true gain setting. It defines the time taken for each measurement. 

The values shown in the data sheet are 'nominal measurement time for 512 clocks', `24.4us * (1 << HW_GAIN)`, which is for a `DECIM_RATE` of 3:\
0=24.4us, 1=48.8us, 2=97.5us, ... 10 (0x0A)=25mS, 11 (0x0B)=50mS

Other values of DECIM_RATE will increase the measurement time, see the code for `calculateConversionTime()`.


## DECIM_RATE (0..3) Decimation Filter, p31

The number of 21MHz clocks per measurement. Note that two measurements are always taken, the first is for ADC offset calibration (dark channel?), so multiply the period value by two to get the actual period.

| Value | Clocks | Period                 |
|:----  |:------ |:------                 |
| 0	    | 1024   | 48.8us * (1 << HW_GAIN) |
| 1	    | 2048   | 97.6us * (1 << HW_GAIN) |
| 2	    | 4096   | 195us  * (1 << HW_GAIN) |
| 3	    | 512    | 24.4us * (1 << HW_GAIN) |


## Threshold Settings, p33

There are three programmable threshold values. Each channel can use one of these three threshold values, selected according to the `THRESH_SEL` bits for each channel. The `INT` output is is activated if the channel's reading exceeds the threshold value.

The problem is, the threshold value is dependent on `HSIG`, `SW_GAIN` and `HW_GAIN`. If the `automaticGainControl()` feature is used then the threshold value will be invalidated whenever the gain is changed. 

So you cannot use `automaticGainControl()` if you want to use the threshold values. Instead, you can check the normalised values yourself.

## References

**Si1133 Data Sheet, Rev. 0.91** \
All page numbers refer to this version of the data sheet \
https://muman.ch/pub/Si1133.pdf

**AN968 Si1133 Optical Design Guide** \
UV Index calculation details, diffuser, etc. \
https://www.silabs.com/documents/public/application-notes/AN968-Si1133-UV-Index-Sensor-Electrical-and-Optical-Design-Guide.pdf


## Revision History

| Date  | Revision | Description |
|:---------- |:---------|:----------- |
| 2026.10.12 | 0.0.0	| Preliminary |


## Joke of the Week

"_It's enough to make a Chef turn in his gravy._"


