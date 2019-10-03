[back](documentation.md)

Version 3.06<br>
June 2019<br>
Chris Martin<br>

## <a name="config"></a> Writing configuration files

- [Conneciton](#connection)
- [Device](#device)
- [Comments, stops, caps, and quotes](#cscq)
- [Scope](#scope)
- [Global parameters](#global)
- [Analog input](#ai)
- [Digital input streaming](#di)
- [Analog output](#ao)
- [Triggering](#trigger)
- [Extended features](#ef)
- [Digital communication](#com)
- [Meta parameters](#meta)


Configuration files are just plain text; no special editors or software required.  Let's look at a really simple example:
```bash
connection any
samplehz 1000
aichannel 0
```
Here, we have three parameters and three values separated by spaces, tabs, or new lines (a.k.a. white space).  All LConfig entries take the form of parameter-value pairs like this.  

Each *parameter* is the name of some setting that LConfig recognizes and knows how to handle.  If LConfig is looking for a parameter and sees a bit of text it doesn't recognize, it will throw an error and halt.  Similarly, LConfig expects certain types of *values* for each parameter, and if it doesn't know how to handle an entry, it will raise an error and halt.  

Here, we'll explore the most important concepts for writing your own configuration files, but a complete list of all parameters, their meaning, and the values they expect is in the [Reference](reference.md) page and in the `lconfig.h` comments.

### <a name="connection"></a> Connection
The connection parameter is the single most essential element of any configuration file.  It tells LConfig how to search for a LabJack device, and it is the signal to LConfig that a new device connection is being configured.  Because LabJack's LJM interface is inherently designed for multiple device connections, so is the LConfig system.

```bash
connection eth
... entries for first device ...

connection eth
... entries for second device ...
```

This example configures two devices to be connected over ethernet.  All of the entries that follow a connection entry apply to that device connection.  Entries that come before a connection entry do not belong to a device and will cause an error, so configuration files *need* at least one configuration entry.

What the application decides to do with the entries is up to the application.  For example, `dburst` ignores all but the first entry, but `drun` can accept up to three device connections.  Each is treated as a separate data acquisition job to be done in order.

The `connection` parameter accepts three values: `any`, `usb`, or `eth`.  They have precisely the result one would expect; a connection will be enforced over either usb, ethernet, or either.

[top](#config)

### <a name="device"></a> Specifying a device
There are five parameters that tell LConfig how to find a valid LabJack device.  We have already seen the `connection` parameter, which is always required, and specifies whether USB or ethernet will be used.  In addition, LConfig can be given the `device`, `serial`, `ip`, and `name` parameters.

`device` specifies the *type* of device, and expects `any`, `t4`, or `t7`.  If `device` is unspecified, it defaults to `any`.

`serial` specifies the serial number of the device.  If it is unspecified, then it is ignored, but if specified, the device *must* have a matching serial number.

`ip` specifies the IP address where the target device may be found when operating over an ethernet conneciton.  When the connection is over USB, `ip` is no longer used to identify a device, but will be written to the device like any other parameter.  When the connection type is `any`, the behavior of the `ip` parameter is ambiguous, so specifying it will cause an error.

`name` specifies the string name (alias) of the target device.  If it is not specified, then the parameter is ignored, but if it is specified, then the device *must* have a name that matches the `name` value.

[top](#config)

### <a name="cscq"></a> Comments, stops, caps, and quotes

When a word begins with a single `#` character, the rest of the line is ignored as a comment.  This allows users to write notes in their configurations that will not be interpreted by LConfig.

When a word begins with `##`, LConfig stops processing.  When configuration settings are written to the header of a data file (for example), this allows the parser to re-load the configuration while ignoring the data that follow.

Prior to processing all parameters or values, the text is forced to lower-case.  This means that all entries are case-insensitive, but there can be trouble when users try to specify a device name with upper-case letters or spaces.

```bash
# LConfig will interpret the name parameter to be "this", and will
# throw an error when it fails to recognize the "is" parameter.
connection usb
name This Is a Name

# Inside quotes, all cases are respected, and white space is not 
# interpreted as a parameter-value separator.
connection usb
name "This is a Name"
```

[top](#config)

### <a name="scope"></a> Parameter scope

So far, all of the parameters we have been discussing are applied to the device being configured, but some parameters are applied more narrowly.  For example, parameters that describe an analog input will not apply to the entire device, but only to a single analog input channel.  This limited range of influence is called the parameter's *scope*.  Global parameters are applied to the entire device.  Other scopes include analog input, analog output, and flexible IO.  Parameters with these scopes only apply to a single channel of each of those types.

[top](#config)

### <a name="global"></a> Global parameters

In addition to the parameters used to identify the device, there are several important parameters that define the behavior of the device.  Perhaps the most important is `samplehz`, which indicates the number of times per second each channel will be sampled in a stream.  

`nsample` is an integer sample count whose behavior depends on the application.  Nominally, `nsample` tells the  `iscomplete_data_stream()` function when to declare that the data acquisition process is complete, but it's entirely up to the application whether or not to keep going.  For example, `drun` utterly ignores `nsample` and just waits for a user keystroke.  However, `dburst` uses `nsample` to determine the duration of the entire measurement.

Especially for high-output-impedance sensors like thermocouples, the `settleus` parameter can be extremely useful in achieving clean measurements.  This specifies the time (in microseconds) for each signal to "settle" before a measurement occures.  Each time a device switches channels in a stream operation, there is some time required for the internal circuitry to settle in to the new value.  Read about [multiplexers](https://en.wikipedia.org/wiki/Multiplexer) or [ghosting](https://knowledge.ni.com/KnowledgeArticleDetails?id=kA00Z0000019KzzSAE) for more information.

Trigger configuration is also a global process, but there is a separate section devoted to configuring triggers [below](#trigger).

### <a name="ai"></a> Analog inputs

A new analog input channel is created for each instance of the `aichannel` parameter.  All parameters with the analog input scope that follow will be applied to that channel.  Any analog input parameters that come before an `aichannel` parameter have no target and will cause an error.

In this example, a T7 device at IP address 192.168.1.11 will have three analog input channels configured.  Note that they do not need to be in physical order.  The channels will be streamed and reported in the order they are specified in the configuration file. The `aichannel` parameter is the only mandatory parameter for creating an analog input channel; the others default to relatively safe values.

```bash
connection eth
device T7
ip 192.168.1.11
samplehz 2000

# This is an example of a +/- 10V differential input channel
aichannel 0
ainegative 1
ailabel "Default differential"

# This is an example of a +/- 1V differential input channel
aichannel 4
ainegative differential
airange 1
ailabel "I'm a differential channel"

# This is an example of a +/- 0.1V single-ended input channel
aichannel 2
ainegative ground
airange 0.1
```

On the T7, channels 0-13 are normal analog inputs, channel 14 is the on-board temperature sensor, and 199 is the device ground.  On the T4, channels 0-3 are +/- 10V inputs, and 4-7 are 0-2.5V inputs that are coincident with the lower four bits of the FIO register.  Additional channels are available with the MUX80 addition, but LConfig does not yet support LabJack add-ons.

The `ainegative` parameter indicates whether the measurement should be referenced relative to the device's ground (single-ended) or relative to a second analog input channel (differential).  The latter is far better for noise rejection, but it uses twice as many analog input ports.  Single-ended is the default.  A single-ended measurement can be specified by the word `ground` or with the value `199` (specified by LJM driver).  Differential measurements are only available on even-numbered channels, and the negative input *must* be on the neighboring negative channel.  A differential measurement may be specified either by specifying the negative channel number or automatically with the word `differentail`.

The `airange` parameter determines how the input voltage should be scaled, and is only used by the T7.  Legal values are 0.01, 0.1, 1.0, or 10.  These represent the bipolar input voltage ranges that the channels can accept.  The choice represents a tradeoff between flexibility of input values and precision of measurement.

See the [Reference](reference.md) page for more parameters.

[top](#config)

### <a name="di"></a> Digital input stream

If a nonzero integer value is written to the `distream` parameter, then an extra input stream of the lowest 16-bits of the DIO register (FIO and EIO) will be added like an extra analog input channel.  This stream is always last in the stream order regardless of when `distream` is specified.

The value stored in `distream` is interpreted as a 16-bit mask indicating which of the digital inputs should be set to inputs.  A 1 indicates and input, and a 0 indicates an output.  For example, to stream in DIO8 and DIO10, the value written to `distream` should be 2^8^ + 2^10^ = 256 + 1024 = 1280.

```bash
# Stream inputs on DIO8 and DIO10 (EIO0 and EIO2)
diostream 1280
```

[top](#config)

### <a name="ao"></a> Analog outputs

LConfig uses the device analog output to mimic the behavior of a function generator.  Configuring an analog output begins in the same way as configuring an analog input; with the `aochannel` parameter.  `aochannel` expects an integer indicating the physical channel to be used (0 or 1).  The outputs must remain in the range 0-5V.

The behavior of each output is controlled with `aosignal`, `aoamplitude`, `aooffset`, `aofrequency`, and `aoduty`.  Because output streaming occurs sychronously with input streaming, the output will be updated at the same sample rate as the analog inputs.  That determines the limits on the fidelity of the output with increasing signal frequency.  How each parameter is interpreted depends on the signal specified.

`aosignal` expects `constant`, `sine`, `square`, `triangle`, or `noise`.

A `constant` signal simply outputs the voltage specified by the `aooffset` parameter.  All other parameters are ignored.

```bash
# This configuration stanza makes a 2.5V constant output
aochannel 0
aosignal constant
aooffset 2.5
aolabel "2.5V Constant"
```

A `sine` signal produces a sine wave with frequency `aofrequency` in Hz, amplitude `aoamplitude` in volts, and mean value `aooffset` in volts.  `aoduty` is ignored.

```bash
# This configuration stanza makes a 10Hz sine wave 
# with maximum 3.5V, minimum 1.5V
aochannel 1
aosignal sine
aooffset 2.5
aoamplitude 1.
aofrequency 10.
aolabel "Sine wave"
```

A `square` signal produces a square wave that repeats with a frequency `aofrequency` in Hz, has a maximum `aooffset + aoamplitude`, a minimum `aooffset - aoamplitude`, and a duty cycle `aoduty`.  The duty cycle is a number between 0 and 1 specifiying the fraction of the wave's time spent at its maximum.  The remainder is spent at its minimum.

```bash
# This configuration stanza makes a 100Hz square wave
# with maximum 2.5, minimum 1.5, and a 40% duty cycle
aochannel 0
aosignal square
aooffset 2
aoamplitude 0.5
aofrequency 100
aoduty 0.4
```

A `triangle` signal produces a triangle wave that repeats with a frequency `aofrequency` in Hz, has a maximum `aooffset + aoamplitude`, a minimum `aooffset - aoamplitude`, and a duty cycle `aoduty`.  For a triangle wave, the duty cycle is a number between 0 and 1 specifying the fraction of the wave's time spent rising.  The remainder is spent falling.

```bash
# This configuration stanza makes a 100Hz triangle wave
# with maximum 4.0V, minimum 3.0V
# Duty cycle is not specified, so it defaults to 50%
aochannel 0
aosignal triangle
aooffset 3.5
aoamplitude 0.5
aofrequency 100
```

A `noise` signal produces a sequence of random samples that repeats with frequency `aofrequency`, with an expected value `aooffset`, a maximum `aooffset + aoamplitude`, and a minimum `aooffset - aoamplitude`.  `aoduty` is ignored.  So-called "white noise" signals are a common tool to produce a signal with rich frequency content.  This signal will exhibit a sharp decline in signal content below the repetition frequency.

```bash
# This configuration stanza makes a noise signal that repeats
# twice per second, and varies between 1V and 3V.
aochannel 0
aosignal square
aooffset 2
aoamplitude 1
aofrequency 2
aoduty 0.4
```

[top](#config)

### <a name="trigger"></a> Triggering

A *trigger* event is something that tells the device to start collecting data.  When trigger parameters are not configured, the data acquisition device simply starts taking data whenever the configuration is complete.  For experiments where data collection needs to be synchronized with a physical event, this is rarely good enough.

The `trigchannel` parameters names an analog input channel that should be monitored for a trigger event.  It should be emphasized that the `triggerchannel` is NOT the physical analog input channel, but integer indicating which (by the order they were configured) input channel should be monitored.  

Trigger events on an analog input is determined by when the voltage crosses a *threshold* voltage.  The value of the threshold is set by the `triglevel` parameter in volts.  If the signal MUST be increasing from below for the event to qualify, then the `trigedge` parameter should be set to `rising`.  Alternately, `trigedge` may be set to `falling` or `all`.

If `trigchannel` is set to `hsc`, then the high-speed-counter 2 will be used instead.  This allows a fast digital pulse to be used for synchronizing without needing to add an unnecessary analog input channel.

```bash
# Configure a generic single-ended analog input on physical
# channel AI2
aichannel 2
airange 10

# Trigger on a rising edge crossing 2.5V on the first aichannel
# configured.  In this case, that happens to be AI2.
trigchannel 0
triglevel 2.5
trigedge rising
```

### <a name="ef"></a> Extended features

Digital input-output extended features can be configured just like an analog input or output.  The biggest difference is that they do not participate in streaming, so applications need to interact with them via the `lc_update_ef` function.

An extended feature channel is created by the `EFCHANNEL` parameter, which accepts a single integer digital pin number on which the channel is to be configured.  Like the analog channels, extended features can be given a string label using the `EFLABEL` parameter.

The function of the channel is determined by the `EFSIGNAL` and `EFDIRECTION` parameters.  The direction indicates whether the channel is an input or an output.  The signal indicates what kind of extended feature is to be used.

Because of the way LabJacks manage the internal timing for the extended features, all extended feature output signals share a single global `EFFREQUENCY` parameter that defines their repetition rate.  For example, the following example configures two pulse-width-modulated outputs with one signal phase-shifted relative to the first.  Both signals will share the same 1000Hz frequency.

```bash
effrequency 1000

efchannel 0
eflabel "Primary PWM Output"
efsignal pwm
efdirection output
efduty 0.75

efchannel 1
eflabel "Secondary PWM Output"
efsignal pwm
efdirection output
efduty 0.25
efdegrees 270
```

The LCONFIG system supports most of the LabJack EF features, but the functional interface is not very thoroughly developed as of version 4.00.  Enabling features require calling the `lc_update_ef()` function, and applications must acquire measurement results directly from the `lc_devconf_t` struct.  This is intended to change in the near future.

For now, the behavior of the extended features is sufficiently complicated and changing quickly enough, that detailed documentation here is not warranted.  For detailed documentation of the extended features support, see the comments of the `lc_load_config()` function in the `lconfig.h` header file.

### <a name="com"></a> Digital communication

LabJack's T-series includes support for Universal Asynchronous Receive/Transmit (UART), Serial Peripheral Interface (SPI), Inter-Integrated Circuit (I2C), One-Wire, and SBUS digital interfaces.  As of version 4.00, LConfig only supports UART, but there are plans to expand support in the future.

Unlike the other interfaces, COM channels are not set up starting with their physical pin number because most COM channels include a number of physical pins.  Instead, COM channels are created by the `COMCHANNEL` parameter, which is expected to be followed by `UART`, `SPI`, `I2C`, `1WIRE`, or `SBUS`.  The parameters that follow will depend on which interface is selected.

The example below shows a typical configuration for a UART channel.  Here, we see the Rx and Tx pin numbers are selected by the `COMIN` and `COMOUT` parameters.  The baud rate is selected by the `COMRATE` parameter.  The `COMOPTIONS` parameter expects a formatted string to specify the number of bits per transmission, the "parity", and the number of stop bits.  Parity can be (N)one, (E)ven, or (O)dd.  

```bash
comchannel uart
comin 8
comout 9
comrate 9600
comoptions 8N1
```

### <a name="meta"></a> Meta parameters

Meta parameters allow experimentalists to embed additional entries in the configuration (and therefore the data).  Since these parameters are not acted on by LConfig, they can be anything the user wants.  There are three data types that are permitted: integers `int`, floating point `flt`, and strings `str`.  Any meta parameters specified in the configuration file will be faithfully written to the header of the data file.

This allows users to embed things like test conditions or experimentalist's notes that will help with the data analysis.

A single meta parameter can be specified by its type, a colon, and its name.  For example,
```bash
int:doses 3
flt:temperature 27.4
str:note0 "Initial ignition failed. Second succeeded."
str:note1 "Low hum from power supply."
```
In these examples, new parameters that are completely meaningless to LConfig are read and associated with the data.

Of course, the `type:name value` format is not very elegant; especially if there are many meta values.  To address that problem, meta parameters may be configured in stanzas.  LConfig recognizes a parameter `meta` followed by a value that determines the variable type to be defined.  Once one of these entries begins a meta stanza, all unrecognized parameters will be used to create a new meta entry rather than raising an error.  The following example is equivalent to the one above.
```bash
meta str
	note0 "Initial ignition failed. Second succeeded."
	note1 "Low hum from power supply."
meta flt
	temperature 27.4
meta int
	doses 3
meta end
```

Neither the indentation, nor the `meta end` entry at the bottom are required, but they are good practice.  Although it is not forbidden, users should avoid specifying standard LConfig entries inside a meta stanza.  For example,
```bash
meta str
	note0 "My first note"
nane "MyLabJack_name"
```
Note that the configuration file attempted to specify the device's *name*, but thanks to a keystroke error and a failure to end the meta stanza, this configuration file will create the utterly useless *nane* meta parameter instead of raising an error that would alert the user of the mistake.

[top](#config)
