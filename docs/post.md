[back](documentation.md)

Version 4.00<br>
September 2019<br>
Chris Martin<br>

## Post processing

The LConfig package includes a post-processing suite written in Python in the `py` directory.  This is a package with tools for loading LConfig data files, plotting, calibrating, and manipulating the data.  For full functionality, the NumPy and Matplotlib packages must be installed.  If you are using the Python package index (pip), 
```bash
$ sudo -H pip install --upgrade numpy
$ sudo -H pip install --upgrade matplotlib
```

If you are not using pip, or if you have difficulty getting pip to work correctly, there are a number of options.  The simplest is to try your system's native package manager.
```bash
$ sudo apt install python-numpy python-matplotlib
```



To get started, load the `lconfig` python package.
```bash
$ cd /path/to/lconfig/py
$ python
```

```python
>>> import lconfig as lc
>>> help(lc)
help(lc)
help(lc.LConf)

Help on class LConf in module lconfig:

class LConf
 |  Laboratory Configuration class
 |  
 |  The Python LConf class does not directly mirror the C DEVCONF struct. It
 |  contains all of the DEVCONF structs defined by a single LConfig file and
 |  all of the data sets that might have been defined by the data 
 |  acquisition operation.  Write operations are not allowed; this class is
 |  strictly defined for retrieving data and configuration parameters.
 |  
 |  LConf objects are initialized with a mandatory LConfig file
 |      LC = LConf( 'path/to/drun.conf' )
 |      
 |  Once loaded, the parameters are accessed using the "get" method: 
 |      LC.get(devnum=0, param='samplehz')
 |  
 |  The optional 'data' keyword prompts the LConf object to read in the file
 |  as a data file instead of just a configuration file.  Additionally, the
 |  'cal' keyword prompts __init__ to apply the channel configurations to
 |  the data.
 |      LC = LConf( 'path/to/drun.dat', data=True, cal=True)
 |      
 |  Once loaded, the data can be accessed individually by channel index or
 |  by channel label.  The corresponding time vector is also available.
 |      LC.get_channel(1)
 |      LC.get_channel('T3')
 |      LC.get_time()
 |  
 |  There are methods to determine some information on what was configured
 |      LC.ndev()           Number of configured devices
 |      LC.ndata()          Number of data points loaded
 |      LC.naich(devnum)    Number of analog inputs on device devnum
 |      LC.naoch(devnum)    Number of analog outputs on device devnum
 |      LC.nfioch(devnum)   Number of flexible IO channels on device devnum
 |      
 |  The labels of all configured channels can also be retrieved
 |      LC.get_labels(devnum, source='aich')
 |  
 |  There are a number of static members that contain useful information:
 |      LC.cal      Were the channel calibrations applied during load? T/F
 |      LC.data         An array of data loaded from the data file or None
 |      LC.filename     The global path to the source file
 |      LC.time         The array returned by get_time() or None
 |      LC.timestamp    The timestamp string loaded from the data file
 |      
 |  The above members are intended for public access, but the _devconf list
 |  is not intended for direct access.  Instead, use the get() function.

```
