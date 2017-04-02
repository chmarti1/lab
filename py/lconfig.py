#
#   Load LCONF configuration and data
#
import os, sys
import numpy as np

__version__ = '2.0.1'

#define LCONF_MAX_STR 32
#define LCONF_MAX_READ "%32s"
#define LCONF_MAX_META 16
#define LCONF_MAX_STCH 15
#define LCONF_MAX_AOCH 1
#define LCONF_MAX_NAOCH 2
#define LCONF_MAX_AICH 13
#define LCONF_MAX_NAICH 14
#define LCONF_MAX_AIRES 8
#define LCONF_MAX_NDEV 32
#define LCONF_MAX_AOBUFFER  512
#define LCONF_BACKLOG_THRESHOLD 1024

LCONF_DEFAULTS = {
#define LCONF_DEF_AI_NCH 199
'LCONF_DEF_AI_NCH': 199,
#define LCONF_DEF_AI_RANGE 10.
'LCONF_DEF_AI_RANGE': 10.,
#define LCONF_DEF_AI_RES 0
'LCONF_DEF_AI_RES': 0,
#define LCONF_DEF_AO_AMP 1.
'LCONF_DEF_AO_AMP': 1.,
#define LCONF_DEF_AO_OFF 2.5
'LCONF_DEF_AO_OFF': 2.5,
#define LCONF_DEF_AO_DUTY 0.5
'LCONF_DEF_AO_DUTY': 0.5
}

# map connection indices to the connection string
LCONF_CONNECTIONS = ['any','usb',None,'eth']



class AICONF:
    def __init__(self):
        self.channel = -1
        self.nchannel = LCONF_DEFAULTS['LCONF_DEF_AI_NCH']
        self.range = LCONF_DEFAULTS['LCONF_DEF_AI_RANGE']
        self.resolution = LCONF_DEFAULTS['LCONF_DEF_AI_RES']

class AOCONF:
    def __init__(self):
        self.channel = -1
        self.signal = 1
        self.amplitude = LCONF_DEFAULTS['LCONF_DEF_AO_AMP']
        self.frequency = -1
        self.offset = LCONF_DEFAULTS['LCONF_DEF_AO_OFF']
        self.duty = LCONF_DEFAULTS['LCONF_DEF_AO_DUTY']

class DEVCONF:
    def __init__(self):
        self.connection = -1
        self.serial = ''
        self.ip = ''
        self.gateway = ''
        self.subnet = ''
        self.naich = 0
        self.naoch = 0
        self.samplehz = -1
        self.nsample = 64;
        self.settleus = 1.;
        self.aich = []
        self.aoch = []
        self.meta = {}


def read_pair(ff):
    """Read in a single parameter word
    param, value = read_pair(ff)
"""
    ws = ' \t\n\r'
    nl = '\n'
    comment = '#'
    charin = '!'
    param = ''
    # so long as the file is not empty, keep reading
    while charin:
        word = ''
        
        # Kill off leading whitespace
        charin = ff.read(1)
        while charin and charin in ws:
            charin = ff.read(1)
            
        # Read in a word
        while charin and charin not in ws:
            word += charin
            charin = ff.read(1)
        
        # If the word is non-empty
        if word:
            # If the word starts with the termination characters
            if word.startswith('##'):
                ff.readline()
                return '', ''
            # If we've found a comment
            elif word.startswith('#'):
                # Kill off the line
                ff.readline()
            elif param:
                return param,word
            else:
                param = word
    return param,word
                




def load_config(filename):
    """Load a configuraiton file
    conf = load_config(filename)
        OR
    conf = load_config(ff)

load_config accepts a file name or an open file.  filename should be a 
string path to a valid configuration file.  ff should be a file object
referencing an open file stream to a valid configuration file.

conf will be a list of DEVCONF objects corresponding to the devices 
configured in the file.
"""

    opened = True
    if not isinstance(filename,file):
        ff = open(filename,'r')
        opened = True
    else:
        ff = filename
        opened = False

    # These dicitonaries are maps from the config file parameter name and
    #   the class member name.  They are grouped by data type.
    # Global integer map, global float map, and global string map
    GIM = {'nsample':'nsample'}
    GFM = {'samplehz':'samplehz', 'settleus':'settleus'}
    GSM = {'ip':'ip', 'serial':'serial', 'gateway':'gateway', 'subnet':'subnet'}
    # Analog input integer map, analog input float map, analog input string map
    AIIM = {'airesolution':'resolution'}
    AIFM = {'airange':'range'}
    AISM = {}
    # Analog output integer, float, and string map
    AOIM = {}
    AOFM = {'aoamplitude':'amplitude', 'aofrequency':'frequency', 
            'aooffset':'offset', 'aoduty':'duty'}
    AOSM = {'aosignal':'signal'}
    
    done = []
    this = None
    thisai = None
    thisao = None
    
    # Read in the new
    param,value = read_pair(ff)
    param = param.lower()
    value = value.lower()

    # Initialize the meta type
    metatype = 'n'

    while param and value:
        
        # Terminatino characters
        if param == 'connection':
            this = DEVCONF()
            done.append(this)
            try:
                index = int(value)
                this.connection = LCONF_CONNECTIONS[index]
            except:
                this.connection = value
                # if this is not an integer
                if value not in LCONF_CONNECTIONS:
                    raise Exception('Unrecognized connection specifier: {:s}'.format(value))
            thisai = None
            thisao = None
        elif not this:
            raise Exception('Missing CONNECTION parameter.')
        # Global parameters
        elif param in GSM:
            this.__dict__[GSM[param]] = value
        elif param in GIM:
            this.__dict__[GIM[param]] = int(value)
        elif param in GFM:
            this.__dict__[GFM[param]] = float(value)
        # New analog input channel
        elif param == 'aichannel':
            thisai = AICONF()
            this.aich.append(thisai)
            thisai.channel = int(value)
            this.naich += 1
        elif param == 'ainegative':
            if value=='ground':
                index = 199
            else:
                index = int(value)
            this.nchannel = index
        # Analog input parameters
        elif param in AIIM:
            thisai.__dict__[AIIM[param]] = int(value)
        elif param in AIFM:
            thisai.__dict__[AIFM[param]] = float(value)
        elif param in AISM:
            thisai.__dict__[AISM[param]] = value
        # New analog output channel
        elif param == 'aochannel':
            thisao = AOCONF()
            this.aoch.append(thisao)
            thisao.channel = int(value)
            this.naoch += 1
        elif param in AOIM:
            thisao.__dict__[AOIM[param]] = int(value)
        elif param in AOFM:
            thisao.__dict__[AOFM[param]] = float(value)
        elif param in AOSM:
            thisao.__dict__[AOSM[param]] = value
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
            this.meta[param[4:]] = int(value)
        elif param.startswith('flt:'):
            this.meta[param[4:]] = float(value)
        elif param.startswith('str:'):
            this.meta[param[4:]] = value
        elif metatype == 'i':
            this.meta[param] = int(value)
        elif metatype == 'f':
            this.meta[param] = float(value)
        elif metatype == 's':
            this.meta[param] = value
        else:
            raise Exception('Unrecognized parameter: {:s}.'.format(param))
        
        param,value = read_pair(ff)
        param = param.lower()
        value = value.lower()

    if opened:
        ff.close()
        
    return done



class dfile:
    """LCONFIG data file object
    Has attributes:
filename    The full path file name of the data file
config      A DEVCONF object containing the read header device configuration
start       A dictionary holding the start time for the experiment
data        A 2D numpy array containing the raw data from the file
analysis    A dictionary containing meta data reflecting data analysis
afun        The function used to generate the analysis dictionary

DFILE = dfile(filename, afun=None)

filename is the full or partial path to the data file to be loaded.  It must
conform to the lconfig format.

afun is an optional user-defined analysis function that can be called to 
produce meta-analysis parameters on the data.  It must operate with the call
signature
>>> DFILE.analysis = afun(DFILE)
"""
    def __init__(self,filename,afun=None):
        self.filename = os.path.abspath(filename)
        self.config = None
        self.start = {'raw':'', 'day':-1, 'month':-1, 'year':-1, 'weekday':-1,
                'hour':-1, 'minute':-1, 'second':-1}
        self.data = None
        self.analysis = None
        self.afun = afun
    
        try:
            ff = open(filename,'r')
        except:
            raise Exception('Failed to open the data file: {:s}'.format(filename))
        # load the configuration data
        self.config = load_config(ff)

        data = []
        line = ff.readline()
        while line:
            if line.startswith('#:'):
                self.start['raw'] = line[2:].strip()
                try:
                    temp = line.split()
                    # parse the time string
                    self.start['weekday'] = temp[1]
                    self.start['month'] = {'Jan':1, 'Feb':2, 'Mar':3, 'Apr':4, 'May':5, 
                            'Jun':6, 'Jul':7, 'Aug':8, 'Sep':9, 'Oct':10, 'Nov':11, 
                            'Dec':12}[ temp[2] ]
                    self.start['day'] = int(temp[3])
                    time = temp[4].split(':')
                    self.start['hour'] = int(time[0])
                    self.start['minute'] = int(time[1])
                    self.start['second'] = int(time[2])
                    self.start['year'] = int(temp[5])
                except:
                    pass
                    
            elif line.startswith('#'):
                pass
            else:
                this = [float(dd) for dd in line.strip().split()]
                if this:
                    data.append(this)
            line = ff.readline()
        self.data = np.array(data)
        ff.close()
    
        if self.afun!=None:
            try:
                self.analysis = self.afun(self)
            except:
                print "Exception encounterd while executing the analysis function"
                print sys.exc_info()[1]

class collection:
    """COLLECTION of dfile objects
The collection is responsible for loading DFILE objects in batches and 
returning slices of the data for plotting.

    Has attributes:
data        A list of the DFILE objects from loaded data
afun        The analysis function passed to the dfile objects

    Creating and populating a COLLECTION object
>>> C = collection(afun = my_analysis_fun)
>>> C.add_dir('my/test/dir')

    Retrieving individual DFILE objects from the collection
This example returns the fourth (item 3) DFILE object in the collection list.
>>> d3 = C[3]

    Retrieving specific analysis results for all items in the collection
This example assembles and returns an ordered list of all values for the 
'voltage' keyword in the analysis dictionary of each DFILE object.  This 
assumes that the AFUN analysis function returns a dictionary containing a 
'voltage' keyword.
>>> voltage = C['voltage']

    Assembling collection subsets
This operation assembles a new collection that is a subset of the original
collection based on a series of criteria.  This example makes a new collection
C25, which contains only tests where the 'voltage' analysis keyword is exactly
2.5
>>> C25 = C(voltage=2.5)
To specify the opposite (that the voltage keyword be NOT 2.5) the value should
appear as the only element of a tuple,
>>> C25 = C(voltage=(2.5,))
Alternately, test subsets can be created by ranges.  In this example, C25 will
contain all tests where the 'voltage' analysis keyword lies between 2.4 and 
2.6.
>>> C25 = C(voltage=(2.4,2.6))
To specify the opposite (that the voltage keyword NOT be between 2.4 and 2.6),
simply reverse the values so that the larger is listed first.
>>> C25 = C(voltage=(2.6,2.4))
"""

    def __init__(self, afun=None):
        self.data = []
        self.afun = afun
        self._record = {}
    
    def __iter__(self):
        return self.data.__iter__()
        
    def __len__(self):
        return len(self.data)

    def __getitem__(self, item):

        if isinstance(item, int):
            return self.data[item]
            
        if item not in self._record:
            self._record[item] = []
            for this in self.data:
                if item in this.analysis:
                    self._record[item].append(this.analysis[item])
                else:
                    self._record[item].append(None)

        return self._record[item]

    def _copy(self):
        """Return a copy of the COLLECTION
    The root DFILE objects will not be dublicated, but all of the containing
lists and dictionaries will be.
"""
        newbie = collection()
        newbie.data = list(self.data)
        newbie._record = dict(self._record)
        newbie.afun = self.afun
        return newbie

    def __delitem__(self, item):
        if isinstance(item, int):
            del self.data[item]
            for this in self._record:
                del self._record[this][item]
        elif item in self._record:
            del self._record[item]
        else:
            raise Exception('Found no item to delete: %s'%repr(item))


    def __call__(self,**kwarg):
        newbie = self._copy()
        for item in kwarg:
            value = kwarg[item]
            dindex = 0
            while dindex<len(newbie):
                dset = newbie.data[dindex]  # On which data set are we operating
                remove = False   # Should this data set be removed from the subset?
                if isinstance(value,tuple):
                    # A 1-element tuple is "not-equal-to"
                    if len(value)==1:
                        remove = (dset.analysis[item] == value)
                    # A 2-element ascending tuple is "between"
                    elif len(value)==2:
                        if value[0]<value[1]:
                            remove = (dset.analysis[item] < value[0] or 
                                      dset.analysis[item] > value[1])
                        else:
                            remove = (dset.analysis[item] < value[0] and
                                      dset.analysis[item] > value[1])
                    else:
                        raise Exception('Illegal keyword value; %s=%s'%(item,repr(value)))
                else:
                    remove = dset.analysis[item]
                    
                if remove:
                    del newbie[dindex]
                else:
                    dindex+=1
        return newbie


    def add_dir(self, directory = '.'):
        """add_dir(directory = '.')
    read in all data files in the directory to the collection
When called without an argument, it reads in the current directory.  Files with
a .dat extension will be read in.
"""
        contents = os.listdir(directory)
        for this in contents:
            # If this is a data file
            if this.endswith('.dat'):
                fullpath = os.path.join(directory,this)
                self.data.append(dfile(fullpath, afun=self.afun))
        # Erase the prior assembled analysis items
        self._record = {}
            
    
