[back](documentation.md)

Version 4.00<br>
September 2019<br>
Chris Martin<br>

## Binaries

There are two binaries included with LConfig: `lcrun` and `lcburst`.  These are inteded to be sufficiently generic to do most data acquisition tasks.  `lcrun` is intended for long slow sample-rate tests where the test duration may not be known in advance.  `lcburst` is intended for short high-speed tests.

On a Linux system, following the steps in [Compiling](compiling.bin), the binaries will be installed in `/usr/local/bin`.  For help text, use

```bash
$ lcrun -h
... prints help text ...
$ lcburst -h
... prints help text ...
```
If you're using a terminal window and want to be able to scoll comfortably, use
```bash
$ lcrun -h | less
$ lcburst -h | less
```

### lcrun

The **D**ata **RUN** utility, `lcrun`, streams data directly to a [data file](data.md).  This means it is well suited to long tests that involve big data sets, but the maximum data rate will be limited by the write speed to the hard drive.  For high-speed applications, look at `lcburst`.

`lcrun` ignores the `nsample` parameter.

```bash
$ lcrun -h
lcrun [-h] [-r PREFILE] [-p POSTFILE] [-d DATAFILE] [-c CONFIGFILE] 
     [-f|i|s param=value]
  Runs a data acquisition job until the user exists with a keystroke.

PRE and POST data collection
  When LCRUN first starts, it loads the selected configuration file
  (see -c option).  If only one device is found, it will be used for
  the continuous DAQ job.  However, if multiple device configurations
  are found, they will be used to take bursts of data before (pre-)
  after (post-) the continuous DAQ job.

  If two devices are found, the first device will be used for a single
  burst of data acquisition prior to starting the continuous data 
  collection with the second device.  The NSAMPLE parameter is useful
  for setting the size of these bursts.

  If three devices are found, the first and second devices will still
  be used for the pre-continous and continuous data sets, and the third
  will be used for a post-continuous data set.

  In all cases, each data set will be written to its own file so that
  LCRUN creates one to three files each time it is run.

-c CONFIGFILE
  By default, LCRUN will look for "lcrun.conf" in the working
  directory.  This should be an LCONFIG configuration file for the
  LabJackT7 containing no more than three device configurations.
  The -c option overrides that default.
     $ lcrun -c myconfig.conf

-d DATAFILE
  This option overrides the default continuous data file name
  "YYYYMMDDHHmmSS_lcrun.dat"
     $ lcrun -d mydatafile.dat

-r PREFILE
  This option overrides the default pre-continuous data file name
  "YYYYMMDDHHmmSS_lcrun_pre.dat"
     $ lcrun -r myprefile.dat

-p POSTFILE
  This option overrides the default post-continuous data file name
  "YYYYMMDDHHmmSS_lcrun_post.dat"
     $ lcrun -p mypostfile.dat

-n MAXREAD
  This option accepts an integer number of read operations after which
  the data collection will be halted.  The number of samples collected
  in each read operation is determined by the NSAMPLE parameter in the
  configuration file.  The maximum number of samples allowed per channel
  will be MAXREAD*NSAMPLE.  By default, the MAXREAD option is disabled.

-f param=value
-i param=value
-s param=value
  These flags signal the creation of a meta parameter at the command
  line.  f,i, and s signal the creation of a float, integer, or string
  meta parameter that will be written to the data file header.
     $ lcrun -f height=5.25 -i temperature=22 -s day=Monday

GPLv3
(c)2017-2019 C.Martin
```

### lcburst

The **D**ata **BURST** utility is intended for high speed streaming for a known duration.  The command-line options can be used to override the configured `nsample`, and otherwise, `nsample` determines the number of samples per channel that should be collected.

```bash
$ lcburst -h
lcburst [-h] [-c CONFIGFILE] [-n SAMPLES] [-t DURATION] [-d DATAFILE]
     [-f|i|s param=value]
  Runs a single high-speed burst data colleciton operation. Data are
  streamed directly into ram and then saved to a file after collection
  is complete.  This allows higher data rates than streaming to the hard
  drive.

-c CONFIGFILE
  Specifies the LCONFIG configuration file to be used to configure the
  LabJack.  By default, LCBURST will look for lcburst.conf

-d DATAFILE
  Specifies the data file to output.  This overrides the default, which is
  constructed from the current date and time: "YYYYMMDDHHmmSS_lcburst.dat"

-f param=value
-i param=value
-s param=value
  These flags signal the creation of a meta parameter at the command
  line.  f,i, and s signal the creation of a float, integer, or string
  meta parameter that will be written to the data file header.
     $ LCBURST -f height=5.25 -i temperature=22 -s day=Monday

-n SAMPLES
  Specifies the integer number of samples per channel desired.  This is
  treated as a minimum, since LCBURST will collect samples in packets
  of LCONF_SAMPLES_PER_READ (64) per channel.  LCONFIG will collect the
  number of packets required to collect at least this many samples.

  For example, the following is true
    $ lcburst -n 32   # collects 64 samples per channel
    $ lcburst -n 64   # collects 64 samples per channel
    $ lcburst -n 65   # collects 128 samples per channel
    $ lcburst -n 190  # collects 192 samples per channel

  Suffixes M (for mega or million) and K or k (for kilo or thousand)
  are recognized.
    $ lcburst -n 12k   # requests 12000 samples per channel

  If both the test duration and the number of samples are specified,
  which ever results in the longest test will be used.  If neither is
  specified, then LCBURST will collect one packet worth of data.

-t DURATION
  Specifies the test duration with an integer.  By default, DURATION
  should be in seconds.
    $ lcburst -t 10   # configures a 10 second test

  Short or long test durations can be specified by a unit suffix: m for
  milliseconds, M for minutes, and H for hours.  s for seconds is also
  recognized.
    $ lcburst -t 500m  # configures a 0.5 second test
    $ lcburst -t 1M    # configures a 60 second test
    $ lcburst -t 1H    # configures a 3600 second test

  If both the test duration and the number of samples are specified,
  which ever results in the longest test will be used.  If neither is
  specified, then LCBURST will collect one packet worth of data.

GPLv3
(c)2017-2019 C.Martin
```

