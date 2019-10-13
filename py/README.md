# LCONFIG PYTHON FILES

Python packages are provided to pull LCONFIG data into python for analysis.  This includes data structures for parsing the LCONFIG configuration and data files, but also structures for organizing, analyzing, and searching data and analysis results.

## Features

As of LCONFIG version 3.02, the Python modules include

- A Python implementation of the `DEVCONF` struct and all the subordinate structs.
- A `cfile` class for loading and interrogating configuration files
- A `dfile` class for loading and analyzing data files
- A `collection` class for managing groups of data files and meta data
- A `tc` module for applying thermocouple calibrations

## LCONFIG.PY

The LCONFIG python module is responsible for defining Python allegories to the C structures that constitute the device configuration, but it also supports automatic application of the LCONFIG calibration coefficients, and it provides tools for automating and managing user-defined analysis of the data.

- The [cfile](#cfile) class
- The [DEVCONF, AICONF, AOCONF, and FIOCONF](#basic) classes
- The [dfile](#dfile) class
- The [collection](#collection) class

### <a name='cfile'><a> The CFILE class

The most basic class that most users will need is the `cfile` class.  Its initialization accepts a path to a configuration file name.
```python
import lconfig
cfg = lconfig.cfile('myconfig.conf')
```

It behaves a little like a list of the devices configured in the file
```python
len(cfg)    # returns the number of devices found
cfg[0]      # returns the DEVCONF class for the first device found
```

There are two members that can be manipulated directly
```python
cfg.filename    # The absolute path to the config file
cfg.config      # A list of DEVCONF objects
```

The `__init__` member is solely responsible for parsing configuration files.

### <a name='basic'></a> The basic classes

The `cfile` class uses the `DEVCONF`, `AICONF`, `AOCONF`, and `FIOCONF` class to store configuration file directives.  These are little more than a Python allegory for the [structs defined in the LCONFIG header](../README.md#types).  Their only method is an initializer that creates members to directly parallel the C struct members.  

It should be noted that implementation parameters like `DEVCONF`'s RB member (the ring buffer), trigstate (trigger state), and trigmem (trigger memory) are absent.  They are not necessary since they are purely for implementation and have nothing to do with configuration.  In other words, their states are not defined by configuration files.

Users should never need to manually define one of these classes.  They should be defined and automatically populated by loading an configuration or data file.  Once they are loaded, users can poke around from the command prompt to see what's in there, or they can refer back to the [LCONFIG documentation](../README.md).

### <a name='dfile'></a> The DFILE class

The `DFILE` class is a child of the `CFILE` class, but in addition to loading the configuration at the head of the data file, it also loads and analyzes the data.  

Interacting with data files can be straightforward
```python
import lconfig
D = lconfig.dfile('path/to/datafile.dat')
```

There are three ways to access data.  The most basic is through the `data` member.  It is a numpy array that contains a column for each channel and a row for each sample.
```python
import matplotlib.pyplot as plt
samples,channels = D.data.shape
time = D.t()    # The t() function constructs a time array
plt.plot(time, D.data[:,0])  # plot the first channel
```

The `data` member contains the raw voltages logged in the data file.  However, the `caldata` member is an identically sized array with each column scalled according to the `calslope` and `caloffset` directives.  For channels with unspecified calibrations, the two arrays will have identical columns.
```python
plt.plot(time, D.caldata[:,1])  # This time with calibrated data
plt.ylabel(D.config[0].aich[1].calunits) # Get the unit string
```

Finally, channels can also be selected by their labels (see the `AILABEL` directive in the [LCONFIG README](../README.md)).  The `bylabel` member is a dictionary with a key for each channel with a label corresponding to a 1D numpy array with calibrated data for that channel.  Channels without a label will not appear in the `bylabel` dictionary.
```python
plt.plot(time, D.bylabel['pressure'])  # Equivalent to referencing caldata
```

There is a very important feature that permits `dfile` objects to do much more that just loading data.  The optional `afun` keyword allows users to define their own analysis function prior to loading the data.  The following example looks at a single channel named "pressure", calculates its max, min, and mean, and stores them in the `dfile.analysis` dictionary for later use.
```python
# First, define a little analysis function
# It accepts a DFILE as its only argument
def myanalysis(this_dfile):
    maxp = this_dfile.bylable['pressure'].max()
    minp = this_dfile.bylabel['pressure'].min()
    meanp = np.mean(this_dfile.bylabel['pressure'])
    this_dfile.analysis['mean'] = meanp
    this_dfile.analysis['maxp'] = maxp
    this_dfile.analysis['minp'] = minp

# Now, pass that function to the data file initializer
D = lconfig.dfile('datafile.dat', afun=myanalysis)

# The results will be available in the analysis dictionary
print('The average pressure was %f'%(D.analysis['meanp']))
```

By default, `default_afun` is used, which does nothing more than copy any meta parameters over to the analysis dictionary verbatim.

Below is a table of all 

| Member | Valid Values | Description 
|:------:|:------------:|:------------
| config | `DEVCONF` list | A list of device configurations in the data header
| filename | string | An absolute path to the data file
| start | `dict` | Dictionary containing keys `raw`, `day`, `month`, `year`, `weekday`, `hour`, `minute`, `second`.  Each (except raw) is an integer resulting from parsing the raw date-time string at the data head.
| data | `numpy.ndarray` | A numpy array of the raw data. Each column corresponds to a channel.
| caldata | `numpy.ndarray` | A numpy array of the data after applying the `calslope` and `caloffset` for each channel
| bylabel | `dict` | A dictionary of the `caldata` columns with their labels as keywords
| afun | `function` | A function that accepts this `DFILE` object as its only argument.  Calling it is the last thing that `__init__` does, and it is intended to populate the `analysis` dictionary.  By default, `default_afun` is used.
| analysis | `dict` | A dictionary containing arbitrary keys intended to store data analysis results.

### <a name='collection'></a> The COLLECTION class

For analyzing entire data sets spanning many files and directories, the `collection` class can be used to automate their analysis and to search the results.

```python
C = lconfig.collection(afun=myanalysis) # Creates an empty collection
C.add_dir('/path/to/data/directory/')   # loads all files that end in *.dat
C.add_dir('/another/path/')
  ...
```

This process defines a collection of data that will use the same analysis on all of its member data files.  As a result, a well crafted analysis function will result in a collection with analysis dictionaries that can be directly compared to one another.  The process can be micro-managed if you're in to that kind of thing.
```python
C.add_dir('/my/other/directory', pause=True, verbose=True)
```
The pause directive tells the `add_dir` method to ask for user approval before loading each file.  The verbose directive causes `add_dir` to print a summary of all files discovered.

Nicely formatted tables of the files and their analysis results can be generated automatically by the `table` function.
```python
C.table(('minp', 'meanp', 'maxp'))                  # prints a table of selected analysis results
C.table(('minp','maxp'), filename=False, header=20) # Disables the filename and re-prints the header every 20 lines
C.table({'minp':'%.4e', 'maxp':'%.4e'}, fileout='pretty.txt') # specifies the format for each column and prints to a file instead of stdout
C.table(('minp', 'meanp', 'maxp'), sort='meanp')    # Sorts the table by the value of 'meanp'
```

After reviewing the analysis results tables, it is often helpful to retrieve some specific data sets for further study.  The data can be indexed like a list.
```python
D2 = C[2]   # This is the second data file discovered
Dlast = C[-1] # This is the last data file discovered
```
The data can also be indexed like a dictionary.  If the index is a string, it is used to assemble an ordered list of the `analysis` dictionary elements with the same key.  The example below produces a scatter plot of all the maximum and mean pressures taken from all the data files in the collection.
```python
maxp = C['pmax']
meanp = C['meanp']
plt.plot(meanp, maxp, 'ko')
plt.xlabel('Mean pressure')
plt.ylabel('Maximum pressure')
```

It is also possible to assemble subsets of the original collection.  Suppose we wanted to isolate experiments where the mean pressure was precisely equal to some value; say 5.  It is possible to isolate data files where that condition is strictly true and strictly false.
```python
C5 = C(meanp=5.0)           # Select all files with meanp == 5.0
Cnot5 = C(meanp=(5.0,))     # Select all files with meanp != 5.0
```
`C5` is now a new collection that contains only `dfile` elements with a mean pressure precisely equal to 5.  By passing the argument as the sole member of a tuple, the condition is negated.  

Of course, the real world isn't usually this convenient; measurements are never precisely equal to anything unless they are integers or strings.  For that reason, it is also possible to specify ranges.
```python
C5 = C(meanp=(4.95,5.05))   # Select all files with 4.95 <= meanp <= 5.05
Cnot5 = C(meanp=(5.05,4.95))  # Select all files with meanp < 4.95 OR meanp > 5.05
```
If the lower range value is larger than the second, then the selection condition is reversed.

Using these in combination, it is possible to produce data subset plots quickly and easily.
```python
Cbig = C(pmax=(10., 10000000.))
plt.plot(Cbig['pmean'], Cbig['pmax'], 'ko')
```
This example assembles only data files where the maximum pressure climbed over 10. (whatever units) and produces a scatter plot of the maximum against the mean.  The upper limit on the range is ridiculously large to make it effectively a greater than test.

