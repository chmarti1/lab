#
#   Load LCONF configuration and data
#
import os, sys
import numpy as np
import json
import matplotlib.pyplot as plt

__version__ = '4.00'





# Helper funcitons
def _read_param(ff):
    """Read in a single word
word = read_pair(ff)
"""
    # This algorithm very closely mirrors the one used by LCONFIG
    ws = ' \t\n\r'
    
    LCONF_MAX_STR = 80
    
    param = ''
    quote = False
    
    # so long as the file is not empty, keep reading
    # Read in text one character at a time
    charin = ff.read(1)
    while charin and len(param) < LCONF_MAX_STR-1:
        # Toggle the quote state
        if charin == '"':
            if quote:
                quote = False
                break
            else:
                quote = True
        # If this is a comment
        elif not quote and charin == '#':
            # Kill off the rest of the line
            charin = ff.read(1)
            # If the next character is another #, then we're done 
            # with the configuration part of the file
            if charin == '#':
                # Rewind by two characters so the read param algorithm
                # will stick on the ## character combination
                ff.seek(-2,1)
                return param
            # Kill off the remainder of the line
            while charin and charin!='\n':
                charin = ff.read(1)
            # If the parameter is not empty, the # was in the middle
            # of a word.  Go ahead and return that word.
            if(param):
                return param
        elif charin in ws:
            if quote:
                param = param + charin
            elif param:
                return param
        elif quote:
            param = param + charin
        else:
            param = param + charin.lower()
        charin = ff.read(1)
    return param


def _filter_value(value, default):
    """return a configuration entry value based on the default type"""
    if isinstance(default, LEnum):
        done = LEnum(default)
        # Is value an integer?
        try:
            value = int(value)
        except:
            pass
        done.set(value)
        return done
    elif isinstance(default, int):
        return int(value)
    elif isinstance(default, float):
        return float(value)
    return value


class LEnum:
    """Enumerated value class
    
LE = LEnum(strings, values=None, state=0)

The enumerated class defines a discrete set of legal states defined by
a list of strings.  Each string is intended to "name" the state.  The
LEnum class remembers the current state by the index in the string list.
Optionally, a set of values can be specified to indicate integer values
corresponding to each of the states.  If values are not specified, then
the values will be treated as equal to the index in the strings list.

This class is intended to mimick the behavior of a C-style enum type.

LE.set(value)
LE.set('state')
    The set method sets the enumerated state by name or by value 
    depending on whether an integer or string is found.
    
LE.setstate(state)
    The setstate method sets the enumerated state directly rather than
    using the integer value or string name.  This is the index that 
    should be an index in the strings and values lists.
    
LE.get()
    Return the string value for the current state

LE.getvalue()
    Return the integer value for the current state
    
LE.getstate()
    Return the current state
    
LE2 = LEnum(LE)
    Builds a new enumerated instance using LE as a prototype.  The values
    and strings will NOT be copied, but are instead accessed by reference.
"""
    def __init__(self, strings, values=None, state=0):
        # If this is a copy operation
        if isinstance(strings, LEnum):
            self._values = strings._values
            self._state = strings._state
            self._strings = strings._strings
            return
        
        self._strings = list(strings)
        self._values = None
        
        if not self._strings:
            raise Exception('LEnum __INIT__: The strings list cannot be empty.')
        for this in self._strings:
            if not isinstance(this, str):
                raise Exception('LEnum __INIT__: Found a state name that was not a string: %s'%repr(this))
                
        if values:
            self._values = list(values)
            for this in self._values:
                if not isinstance(this, int):
                    raise Exception('LEnum __INIT__: Found a state value that was not an integer: %s'%repr(this))
            if len(self._values) != len(self._strings):
                raise Exception('LEnum __INIT__: The values and strings lists MUST be the same length.')

        self._state = state
        
    def __str__(self):
        return self.get()
        
    def __repr__(self):
        out = '{'
        for ind in range(len(self._strings)):
            if ind == self._state:
                out += '('
            if self._values:
                out += '%s:%d'%(
                        self._strings[ind], 
                        self._values[ind])
            else:
                out += self._strings[ind]
            if ind == self._state:
                out += ')'
            if ind < len(self._strings)-1:
                out += ', '
        out += '}'
        return out

    def get(self):
        """return the string name of the current state"""
        return self._strings[self._state]
        
    def getvalue(self):
        if self._values:
            return self._values[self._state]
        return self._state
        
    def getstate(self):
        return self._state
        
    def set(self,ind):
        if isinstance(ind,str):
            if ind in self._strings:
                self._state = self._strings.index(ind)
                return
            raise Exception('LEnum set: State not recognized: %s'%ind)
        # If this is an integer value
        if not isinstance(ind, int):
            raise Exception('LEnum set: Value was not an integer: %s'%repr(ind))
        
        if self._values:
            if ind in self._values:
                self._state = self._values.index(ind)
                return
        elif ind < len(self._strings) and ind>=0:
            self._state = ind
            return
            
        raise Exception('LEnum set: Value not recognized: %d'%ind)
        
        
    def setstate(self,ind):
        if ind < len(self._strings) and ind>=0:
            self._state = ind
            return
        raise Exception('LE setstate: State is out-of-range: %d'%ind)

###
# Default dictionaries
###

DEF_DEV = {
    'connection':LEnum(['any', 'usb', 'eth'], values=[0,1,3]),
    'serial':'',
    'ip':'',
    'gateway':'',
    'subnet':'',
    'samplehz':-1.,
    'settleus':1.,
    'nsample':64,
    'trigchannel':-1,
    'triglevel':0.,
    'trigpre':0,
    'trigedge':LEnum(['rising', 'falling', 'all'], state=0),
    'effrequency':0,
}

DEF_AICH = {
    'aichannel':-1,
    'ainegative':LEnum(
            ['differential', 'differential', 'differential', 'differential', 'differential', 'differential', 'differential', 'ground'], 
            values=[1, 3, 5, 7, 9, 11, 13, 199], state=7),
    'airange':10.,
    'airesolution':0,
    'aicalslope':1.,
    'aicalzero':0.,
    'ailabel':'',
    'aicalunits':''
}

DEF_AOCH = {
    'aochannel':-1,
    'aosignal':LEnum(['constant', 'sine', 'square', 'triangle', 'noise']),
    'aofrequency':-1.,
    'aoamplitude':1.,
    'aooffset':2.5,
    'aoduty':0.5,
    'aolabel':''
}

DEF_EFCH = {
    'efchannel':-1,
    'efsignal':LEnum(['pwm', 'count', 'frequency', 'phase', 'quadrature']),
    'efedge':LEnum(['rising', 'falling', 'all']),
    'efdebounce':LEnum(['none', 'fixed', 'reset', 'minimum']),
    'efdirection':LEnum(['input', 'output']),
    'efusec':0.,
    'efdegrees':0.,
    'efduty':0.5,
    'efcount':0,
    'eflabel':''
}

DEF_COMCH = {
    'comchannel':LEnum(['uart', 'spi', 'i2c', '1wire', 'sbus']),
    'comrate':-1,
    'comin':-1,
    'comout':-1,
    'comclock':-1,
    'comoptions':'',
    'comlabel':''
}


class LConf:
    """Laboratory Configuration class

The Python LConf class does not directly mirror the C DEVCONF struct. It
contains all of the DEVCONF structs defined by a single LConfig file and
all of the data sets that might have been defined by the data 
acquisition operation.  Write operations are not allowed; this class is
strictly defined for retrieving data and configuration parameters.

LConf objects are initialized with a mandatory LConfig file
    LC = LConf( 'path/to/drun.conf' )
    
Once loaded, the parameters are accessed using the "get" method: 
    LC.get(devnum=0, param='samplehz')

The optional 'data' keyword prompts the LConf object to read in the file
as a data file instead of just a configuration file.  Additionally, the
'cal' keyword prompts __init__ to apply the channel configurations to
the data.
    LC = LConf( 'path/to/drun.dat', data=True, cal=True)
    
Once loaded, the data can be accessed individually by channel index or
by channel label.  The corresponding time vector is also available.
    LC.get_channel(1)
    LC.get_channel('T3')
    LC.get_time()

There are methods to determine some information on what was configured
    LC.ndev()           Number of configured devices
    LC.ndata()          Number of data points loaded
    LC.naich(devnum)    Number of analog inputs on device devnum
    LC.naoch(devnum)    Number of analog outputs on device devnum
    LC.nefch(devnum)   Number of flexible IO channels on device devnum
    
The labels of all configured channels can also be retrieved
    LC.get_labels(devnum, source='aich')

There are a number of static members that contain useful information:
    LC.cal      Were the channel calibrations applied during load? T/F
    LC.data         An array of data loaded from the data file or None
    LC.filename     The global path to the source file
    LC.time         The array returned by get_time() or None
    LC.timestamp    The timestamp string loaded from the data file
    
The above members are intended for public access, but the _devconf list
is not intended for direct access.  Instead, use the get() function.
"""
    def __init__(self, filename, data=False, cal=True):
        self._devconf = []
        self.time = None
        # Externals
        self.timestamp = ''
        self.data = None
        self.cal = cal
        self.filename = os.path.abspath(filename)

        with open(filename,'r') as ff:
            
            # start the parse
            # Read in the new
            param = _read_param(ff)
            value = _read_param(ff)
            # Initialize the meta type
            metatype = 'n'

            while param and value:
                
                #####
                # First, if the parameter indicates the need for a new
                # device connection or channel, create the new element.
                #####
                # Detect a new connection configuration
                if param == 'connection':
                    # Appending a minimal dictionary
                    # The nested configurations are the only ones that
                    # need to be defined explicitly.  All other 
                    # parameters are defined by their defaults in 
                    # DEF_DEV
                    self._devconf.append({
                            'aich':[], 'aoch':[], 'efch':[], 'meta':{}})
                # Detect a new analog input channel        
                elif param == 'aichannel':
                    # Append a minimal dictionary
                    self._devconf[-1]['aich'].append({})
                # Detect a new analog output channel
                elif param == 'aochannel':
                    # Append a minimal dictionary
                    self._devconf[-1]['aoch'].append({})
                # Detect a new analog output channel
                elif param == 'efchannel':
                    # Append a minimal dictionary
                    self._devconf[-1]['efch'].append({})
                    
                #####
                # Deal with the parameter
                #####
                # IF this is a global parameter
                if param in DEF_DEV:
                    self._devconf[-1][param] = \
                            _filter_value(value, DEF_DEV[param])
                elif param in DEF_AICH:
                    self._devconf[-1]['aich'][-1][param] = \
                            _filter_value(value, DEF_AICH[param])
                elif param in DEF_AOCH:
                    self._devconf[-1]['aoch'][-1][param] = \
                            _filter_value(value, DEF_AOCH[param])
                elif param in DEF_EFCH:
                    self._devconf[-1]['efch'][-1][param] = \
                            _filter_value(value, DEF_EFCH[param])
                # Check for meta parameters
                elif param == 'meta':
                    if value == 'str' or value == 'string':
                        metatype = 's'
                    elif value == 'int' or value == 'integer':
                        metatype = 'i'
                    elif value == 'flt' or value == 'float':
                        metatype = 'f'
                    elif value == 'none' or value == 'end' or value == 'stop':
                        metatype = 'n'
                    else:
                        raise Exception('Unrecognized meta flag {:s}.'.format(param))
                elif param.startswith('int:'):
                    self._devconf[-1]['meta'][param[4:]] = int(value)
                elif param.startswith('flt:'):
                    self._devconf[-1]['meta'][param[4:]] = float(value)
                elif param.startswith('str:'):
                    self._devconf[-1]['meta'][param[4:]] = value
                elif metatype == 'i':
                    self._devconf[-1]['meta'][param] = int(value)
                elif metatype == 'f':
                    self._devconf[-1]['meta'][param] = float(value)
                elif metatype == 's':
                    self._devconf[-1]['meta'][param] = value
                else:
                    raise Exception('Unrecognized parameter: {:s}.'.format(param))
                
                param = _read_param(ff)
                value = _read_param(ff)
            
            if not data:
                return
            
            self.data = []
            
            # Read in the ##
            if not ff.readline().startswith('##'):
                sys.stderr.write('LCONF expected ## before data\n')
                return
                
            # Read in the date/timestamp
            self.timestamp = ff.readline()
            
            # Read in the data
            thisline = ff.readline()
            while thisline:
                self.data.append([float(this) for this in thisline.split()])
                thisline = ff.readline()
            self.data = np.array(self.data)
            
            # Apply the calibrations?
            if cal:
                # Calculate the calibrated data
                for aich in range(len(self._devconf[0]['aich'])):
                    temp = self.get(0, 'aicalzero', aich=aich)
                    if temp != 0.:
                        self.data[:,aich] -= temp
                    
                    temp = self.get(0,'aicalslope', aich=aich)
                    if temp != 1.:
                        self.data[:,aich] *= temp
                
            T = 1./self.get(0, 'samplehz')
            N = self.data.shape[0]
            self.time = np.arange(0., (N-0.5)*T, T) 


    def __str__(self, width=80):
        out = ''
        for devnum in range(len(self._devconf)):
            out += '*** Device ' + str(devnum) + ' ***\n'
            device = self._devconf[devnum]
            for this in DEF_DEV:
                out += '  %14s : %-14s\n'%(this, repr(self.get(devnum, this)))
            out += '  * Analog Inputs\n'
            for aich in range(len(self._devconf[devnum]['aich'])):
                out += '    * AICH %d\n'%aich
                for this in DEF_AICH:
                    out += '    %14s : %-14s\n'%(this, repr(self.get(devnum, this, aich=aich)))
            out += '  * Analog Outputs\n'
            for aoch in range(len(self._devconf[devnum]['aoch'])):
                out += '    * AOCH %d\n'%aoch
                for this in DEF_AOCH:
                    out += '    %14s : %-14s\n'%(this, repr(self.get(devnum, this, aoch=aoch)))
            out += '  * Flexible Digital IO\n'
            for efch in range(len(self._devconf[devnum]['efch'])):
                out += '    * EFCH %d *'%efch
                for this in DEF_EFCH:
                    out += '    %14s : %-14s\n'%(this, repr(self.get(devnum, this, efch=efch)))
            out += '  * Meta Parameters\n'
            for param,value in self._devconf[devnum]['meta'].items():
                out += '    %14s : %-14s\n'%(param, value)
        return out

    def _get_label(self, devnum, source, label):
        """Return the index of the aich, aoch, or efch member with the label matching label.
    """
        lkey = {'aich':'ailabel', 'aoch':'aolabel', 'efch':'eflabel'}[source]
        for index in range(len(self._devconf[devnum][source])):
            this = self._devconf[devnum][source][index]
            if lkey in this and this[lkey] == label:
                return index
        raise Exception('Failed to find key %s with value %s'%(lkey, repr(label)))
        
    def _get_index(self, time):
        """Get the index closest to the time specified"""
        index = int(np.round(time*self.get(0,'samplehz')))
        # Clamp the values based on the data size
        return min(max(index, 0), self.ndata()-1)

    def ndev(self):
        """Return the number of device configurations loaded"""
        return len(self._devconf)
        
    def naich(self, devnum):
        """Return the number of analog input channels in device devnum"""
        return len(self._devconf[devnum]['aich'])
        
    def naoch(self, devnum):
        """Return the number of analog output channels in device devnum"""
        return len(self._devconf[devnum]['aoch'])
        
    def nefch(self, devnum):
        """Return the number of flexible IO channels in device devnum"""
        return len(self._devconf[devnum]['efch'])
        
    def ndata(self):
        """Return the number of data samples in the data set.  If no 
data are available, ndata() raises an exception"""
        if self.data is not None:
            return self.data.shape[0]
        raise Exception('NDATA: The LConf object has no data loaded')

    def get_labels(self, devnum, source='aich'):
        """Return an ordered list of channel labels
    [...] = get_labels(devnum, source='aich')

The default source is 'aich', but the labels for 'aoch' and 'efch' can
also be retrieved.
"""
        lkey = {'aich':'ailabel', 'aoch':'aolabel', 'efch':'eflabel'}[source]
        out = []
        for this in self._devconf[devnum][source]:
            if lkey in this:
                out.append( this[lkey] )
            else:
                out.append( '' )
        return out
        

    def get(self, devnum, param, aich=None, efch=None, aoch=None):
        """Retrieve a parameter value
    get(devnum, param, aich=None, efch=None, aoch=None)

** Global Parameters **
To return a global parameter from device number devnum.  For example,
to retrieve the sample rate from device 0,
    D.get(0, 'samplehz')
    
To access multiple parameters at a time from the same device, specify 
the parameter as an iterable like a tuple or a list.
    D.get(0, ('samplehz', 'ip'))

To return a parameter belonging to one of the nested configuration 
systems (analog inputs, analog outputs, or ef channels) use the 
optional keywords to identify the channel index.

** Analog Inputs **
To return the entire analog input list, simply request 'aich' directly.
    D.get(0, 'aich')
    
To access the individual channel parameters, use the aich keyword
    D.get(0, 'airange', aich=0)

The aich can also be used to call out a channel by its label.  Channels
without a label can never be matched, even if the string is empty.
    D.get(0, 'airange', aich='Ambient Temperature')
    
** EF and AO configuration **
The same rules apply for the analog output and ef channels.
    D.get(0, 'aosignal', aoch=0)
    
"""
        # Define the local and default dictionaries
        source = self._devconf[devnum]
        default = DEF_DEV
        
        # Override the source and default if aich, aoch, or efch are 
        # specified.
        if aich is not None:
            # If the reference is by label, search for the correct label
            flag = False
            if isinstance(aich,str):
                aich = self._get_label(devnum, 'aich', aich)
            source = source['aich'][aich]
            default = DEF_AICH
        elif aoch is not None:
            # If the reference is by label, search for the correct label
            flag = False
            if isinstance(aoch,str):
                aoch = self._get_label(devnum, 'aoch', aoch)
            source = source['aoch'][aoch]
            default = DEF_AOCH
        elif efch is not None:
            # If the reference is by label, search for the correct label
            flag = False
            if isinstance(efch,str):
                efch = self._get_label(devnum, 'efch', efch)
            source = source['efch'][efch]
            default = DEF_EFCH
            
        # If the recall is multiple    
        if hasattr(param, '__iter__'):
            out = []
            for pp in param:
                if pp in source:
                    out.append(source[pp])
                elif pp in default:
                    out.append(default[pp])
                else:
                    raise Exception('Unrecognized parameter: %s'%pp)
            return tuple(out)
            
        # If the recall is single
        if param in source:
            return source[param]
        elif param in default:
            return default[param]
        else:
            raise Exception('Unrecognized parameter: %s'%param)


    def get_channel(self, aich, downsample=None, start=None, stop=None):
        """Retrieve data from channel aich
    x = get_channel(aich)

AICH can be the integer index for the channel in the first device's 
analog input channels, or it can be the string channel label.  The first
channel with a matching label will be returned.

X is the numpy array containing data for the requested channel.

Optional keyword parameters are

DOWNSAMPLE
The downsample key indicates an integer number of samples to reject per
sample returned.  The first example below returns every other sample.  
The second example returns every third sample.
    x = get_channel(aich, downsample=1)
    x = get_channel(aich, downsample=2)
    
START, STOP
If they are not left as None, these specify alternate time values at 
which to start and stop the data.  get_channel() can be executed with 
neither, one, or both of these parameters.
    x = get_channel(aich, start=1.5)    # From 1.5 seconds to end-of-test
    x = get_channel(aich, stop=2)       # From 0 to 2 seconds
    x = get_channel(aich, start=1.5, stop=2) # Between 1.5 and 2 seconds
"""
        if self.data is None:
            raise Exception('GET_CHANNEL: This LConf object does not have channel data.')
            
        if isinstance(aich,str):
            aich = self._get_label(0, 'aich', aich)
        
        if downsample or start or stop:
            fs = self.get(0,'samplehz')
            # Initialize slice indices
            I0 = 0
            I1 = -1
            I2 = 1
            if start is not None:
                I0 = self._get_index(start)
            if stop is not None:
                I1 = self._get_index(stop)
            if downsample is not None:
                I2 = int(downsample+1)
            return self.data[I0:I1:I2, aich]
            
        return self.data[:,aich]

    def get_time(self, downsample=None, start=None, stop=None):
        """Retrieve a time vector corresponding to the channel data
    t = get_time()
    
This funciton merely returns a to the "time" member array if data were
loaded when the LConf object was defined.  Otherwise, get_time() raises
an exception
"""
        if self.time is None:
            raise Exception('GET_TIME: This LConf object does not have channel data.')
            
        if downsample or start or stop:
            fs = self.get(0,'samplehz')
            # Initialize slice indices
            I0 = 0
            I1 = -1
            I2 = 1
            if start is not None:
                I0 = self._get_index(start)
            if stop is not None:
                I1 = self._get_index(stop)
            if downsample is not None:
                I2 = int(downsample+1)
            return self.time[I0:I1:I2]
        return self.time


    def show_channel(self, aich, ax=None, fig=None, downsample=None, 
            show=True, ylabel=None, xlabel=None, fs=16,
            start=None, stop=None,
            plot_param={}):
        """Plot the data from a channel
    mpll = show_channel(aich)
    
Returns the handle to the matplotlib line object created by the plot
command.  The aich is the same index or string used by the get_channel
command.  Optional parameters are:

AX
An optional matplotlib axes object pointing to an existing axes to which
the line should be added.  This method can be used to show multiple data
sets on a single plot.

FIG
The figure can be specified either with a matplotlib figure object or an
integer figure number.  If it exists, the figure will be cleared and a
new axes will be created for the plot.  If it does not exist, a new one
will be created.

DOWNSAMPLE
This parameter is passed to get_time() and get_channel() to reduce the 
size of the dataset shown.

SHOW
If True, then a non-blocking show() command will be called after 
plotting to prompt matplotlib to display the plot.  In some interfaces,
this step is not necessary.

XLABEL, YLABEL
If either is supplied, it will be passed to the set_xlabel and 
set_ylabel functions instead of the automatic values generated from the
channel labels and units

FS
Short for "fontsize" indicates the label font size in points.

PLOT_PARAM
A dictionar of keyword, value pairs that will be passed to the plot 
command to configure the line object.
"""

        # Initialize the figure and the axes
        if ax is not None:
            fig = ax.get_figure()
        elif fig is not None:
            if isinstance(fig, int):
                fig = plt.figure(fig)
            fig.clf()
            ax = fig.add_subplot(111)
        else:
            fig = plt.figure()
            ax = fig.add_subplot(111)
        
        if isinstance(aich,str):
            aich = self._get_label(0,'aich',aich)
            
        # Get the y-axis label
        if 'ailabel' in self._devconf[0]['aich'][aich]:
            ailabel = self._devconf[0]['aich'][aich]['ailabel']
        else:
            ailabel = 'AI%d'%self.get(0, 'aichannel', aich=aich)
            
        # Get the y-axis units
        if 'aicalunits' in self._devconf[0]['aich'][aich]:
            aicalunits = self._devconf[0]['aich'][aich]['aicalunits']
        else:
            aicalunits = 'V'
            
        # Get data and time
        t = self.get_time(downsample=downsample, start=start, stop=stop)
        y = self.get_channel(aich, downsample=downsample, start=start, stop=stop)
        
        ll = ax.plot(t, y, label=ailabel, **plot_param)
        
        if xlabel:
            ax.set_xlabel(xlabel, fontsize=fs)
        else:
            ax.set_xlabel('Time (s)', fontsize=fs)
        if ylabel:
            ax.set_ylabel(ylabel, fontsize=fs)
        else:
            ax.set_ylabel('%s (%s)'%(ailabel, aicalunits), fontsize=fs)
        ax.grid('on')
        if show:
            plt.show(block=False)

        return ll

    def get_events(self, aich, level=0., edge='any', start=None, 
            stop=None, count=None, debounce=1, diff=0):
        """Detect edge crossings returns a list of indexes corresponding to data 
where the crossings occur.

AICH
The channel to search for edge crossings

LEVEL
The level of the crossing

EDGE
can be rising, falling, or any.  Defaults to any

START
The time (in seconds) to start looking for events.  Starts at t=0 if 
unspecified.

STOP
The time to stop looking for events.  Defaults to the end of data.

COUNT
The integer maximum number of events to return.  If unspecified, there 
is no limit to the number of events.

DEBOUNCE
The debounce filter requires that DEBOUNCE (integer) samples before and
after a transition remain high/low.  Redundant transitions within that
range are ignored.  For example, if debounce=3, let "l" indicate a 
sample less than the level, and "g" indicate greater than: 
The following would indicate a single rising edge
    lllggg
    lllglggg
    lllggllggllggg
The following would not be identified as any kind of edge
    lllgglll
    lllgllgllglll
    
In this way, a rapid series of transitions are all grouped as a single 
edge event.  The window in which these transitions are conflated is 
determined by the debounce integer.  If none is specified, then debounce
is 1 (no filter).

DIFF
Specifies the number of derivatives to take prior to scanning for events
This is done by y.
"""

        edge = edge.lower()
        edge_mode = 0
        if edge == 'rising':
            edge_mode = 1
        elif edge == 'falling':
            edge_mode = -1
        
        i0 = 0
        i1 = self.ndata()-1
        if start:
            i0 = self._get_index(start)
        if stop:
            i1 = self._get_index(stop)
            
        indices = []
        
        # Get the channel data
        y = self.get_channel(aich)
        if diff:
            y = np.diff(y, diff)
            y *= self.get(0, 'samplehz')**diff
        
        # State machine variables
        rising_index = None
        falling_index = None
        series_count = 1
        test_last = (y[i0] > level)
        
        for index in range(i0+1, i1):
            test = (y[index] > level)
            
            if test == test_last:
                series_count += 1
            # If there has been a value change
            else:
                series_count = 1
            
            # Check the sample count
            if series_count >= debounce:
                # If the sample is greater than
                if test:
                    falling_index = index
                    if rising_index and edge_mode >= 0:
                        indices.append(rising_index+diff)
                        rising_index = None
                # If the sample is less than
                else:
                    rising_index = index
                    if falling_index and edge_mode <= 0:
                        indices.append(falling_index+diff)
                        falling_index = None
                
            if count and len(indices) >= count:
                break
                
            test_last = test
        return indices
        
