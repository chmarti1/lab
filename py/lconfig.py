#
#   Load LCONF configuration and data
#
import os, sys
import numpy as np
import json

__version__ = '3.03'




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

def _mixed(value):
    try:
        return int(value)
    except:
        try:
            return float(value)
        except:
            return str(value)


EMPTY = {
    'fioch':[],
    'aich':[],
    'aoch':[],
    'meta':{}
}


# PARAMS[param] = (ptype, scope, dictmember, makenew)
PARAMS = {
    'connection':   (str,   None,   'connection',   True),
    'serial':       (str,   None,   'serial',       False),
    'ip':           (str,   None,   'ip',           False),
    'subnet':       (str,   None,   'subnet',       False),
    'gateway':      (str,   None,   'gateway',      False),
    'samplehz':     (float, None,   'samplehz',     False),
    'settleus':     (float, None,   'settleus',     False),
    'nsample':      (int,   None,   'nsample',      False),
    'trigchannel':  (_mixed, None,  'trigchannel',  False),
    'trigedge':     (str,   None,   'trigedge',     False),
    'triglevel':    (float, None,   'triglevel',    False),
    'trigpre':      (int,   None,   'trigpre',      False),
    'fiofrequency': (float, None,   'fiofrequency', False),
    'fiochannel':   (int,   'fioch','channel',      True),
    'fiosignal':    (str,   'fioch','signal',       False),
    'fioedge':      (str,   'fioch','edge',         False),
    'fiodebounce':  (str,   'fioch','debounce',     False),
    'fiodirection': (str,   'fioch','direction',    False),
    'fiousec':      (float, 'fioch','usec',         False),
    'fiodegrees':   (float, 'fioch','degrees',      False),
    'fioduty':      (float, 'fioch','duty',         False),
    'fiocount':     (int,   'fioch','count',        False),
    'aichannel':    (int,   'aich', 'channel',      True),
    'ainegative':   (_mixed,'aich', 'negative',     False),
    'airange':      (float, 'aich', 'range',        False),
    'airesolution': (int,   'aich', 'resolution',   False),
    'aical':        (float, 'aich', 'cal',          False),
    'aizero':       (float, 'aich', 'zero',         False),
    'ailabel':      (str,   'aich', 'label',        False),
    'aiunits':      (str,   'aich', 'units',        False),
    'aochannel':    (int,   'aoch', 'channel',      True),
    'aosignal':     (str,   'aoch', 'signal',       False),
    'aofrequency':  (float, 'aoch', 'frequency',    False),
    'aoamplitude':  (float, 'aoch', 'amplitude',    False),
    'aooffset':     (float, 'aoch', 'offset',       False),
    'aoduty':       (float, 'aoch', 'duty',         False)
}    


METAS = {
    'float':    float,
    'flt':      float, 
    'string':   str,
    'str':      str, 
    'integer':  int, 
    'int':      int
}



def _read_word(ff):
    ws = ' \t\n\r'
    word = ''
    charin = ff.read(1)
    while charin:
        # If the next character is a whitespace or is empty
        if (charin in ws):
            # AND if the word is nonempty
            if word:
                return word
        elif charin == '#' and not word:
            charin = ff.readline()
            if charin.startswith('#'):
                return ''
        else:
            word += charin
        charin = ff.read(1)
    return word.lower()



def _load_config(ff):
    """load_config() is a wrapper for the inner _load_config() funciton
    This primative accepts an open file descriptor instead of a filename

>>> with open(filename, 'r') as ff:
...     config = _load_config(ff)

is equivalent to

>>> config = load_config(filename)
"""
    ws = ' \t\n\r'
    nl = '\n'
    comment = '#'
    config = []
    metastate=None
    param = _read_word(ff)
    value = _read_word(ff) if param else ''
    while param and value:
        if param in PARAMS:
            ptype, scope, member, new = PARAMS[param]
            # Use active to point to the list of scoped elements
            if scope is None:
                active = config
            else:
                active = config[-1][scope]
            # If the parameter triggers a new element, append an 
            # empty dictionary
            if new:
                if active is config:
                    active.append(EMPTY.copy())
                else:
                    active.append({})
            # If the active scope has no active dictionary
            elif not active:
                raise Exception('The parameter %s belongs to scope %s, but was found outside of an appropriate stanza.'%(param, scope))
                
            # Finally, write to the appropriate value
            try:
                active[-1][member] = ptype(value)
            except:
                raise Exception('Failed to convert value to type: %s, %s\n'%(value, repr(typ)))
        elif param == 'meta':
            if value in METAS:
                metastate = value
            elif value in ['stop', 'end', 'none']:
                metastate = None
            else:
                raise Exception('Unexpected meta type: %s\n'%(value))
        elif ':' in param:
            metastate, param = param.split(':')
            config[-1]['meta'][param] = METAS[metastate](value)
            metastate = None    
        elif metastate is not None:
            config[-1]['meta'][param] = METAS[metastate](value)
        else:
            raise Exception('Unexpected parameter: %s\n'%(param))

        param = _read_word(ff)
        value = _read_word(ff) if param else ''
            
    if param:
        raise Exception('Incomplete parameter-value pair: %s'%(param))
    return config


def load_config(filename):
    """Returns a list of dictionaries for the lconfig file
    [dict0, dict1, ...]
    
Each dictionary contains the directives found in each configuration 
stanza.  They contain 'meta', 'aich', 'aoch', and 'fioch'
"""
    with open(filename, 'r') as ff:
        return _load_config(ff)



class dfile:
    """LCONFIG data file object
    Has attributes:
filename    The full path file name of the data file
config      The configuration dictionary loaded from the file header
start       A dictionary holding the start time for the experiment
rdata       A 2D numpy array containing the raw data from the file
cdata       A 2D numpy array of calibrated data
analysis    A dictionary containing meta data reflecting data analysis

    Has methods:
apply_cal(inplace=False)        (Re)applies the calibrations
t()                             Calculates a time array

    Supports:
DFILE['label']      Returns the column corresponding to the label
DFILE[ index ]      Returns the column corresponding to the index
len(DFILE)          Returns the number of samples per channel

DFILE = dfile(filename, cal=True, calinplace=False)

filename 
is the full or partial path to the data file to be loaded.  It must 
conform to the lconfig format.

cal 
is a boolean indicating whether or not to apply the analog calibration
to the raw data.  If cal is False, then cdata will be None.

calinplace
is a boolean indicating whether the original raw data should be 
preserved in rdata, or if the calibration should be applied "in place".
When calinplace is False, the data are copied into cdata and the 
calibrations are applied there.  When calinplace is True, the columns of
rdata are overwritten
"""
    def __init__(self,filename,cal=True,calinplace=False):

        
        self.filename = os.path.abspath(filename)

        # Create some new parameters for the data file
        self.start = {'raw':'', 'day':-1, 'month':-1, 'year':-1, 'weekday':-1,
                'hour':-1, 'minute':-1, 'second':-1}
        self.analysis = {}
        self.rdata = None
        self.cdata = None
        self.config = None
        self._t = None

        with open(filename, 'r') as ff:
            # Load the configuration
            # There should only ever be one configuration, so reduce the list
            # to JUST the dictionary there.
            self.config = _load_config(ff)
            if len(self.config)!=1:
                raise Exception('DFILE: Data does not contain exactly one device configuration:\n   %s'%filename)
            self.config = self.config[0]
    
            # How many channels?
            Nch = len(self.config['aich'])
    
            # Start parsing data
            data = []
            line = ff.readline()
            while line:
                if line.startswith('#:'):
                    self.start['raw'] = line[2:].strip()
                    try:
                        temp = line.split()
                        # parse the time string
                        self.start['weekday'] = temp[1]
                        self.start['month'] = {'Jan':1, 'Feb':2, 'Mar':3, 
                                               'Apr':4, 'May':5, 'Jun':6, 
                                               'Jul':7, 'Aug':8, 'Sep':9, 
                                         'Oct':10, 'Nov':11, 'Dec':12}[temp[2]]
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
                    if len(this) == Nch:
                        data.append(this)
                    else:
                        raise Exception('DFILE: data line does not match the number of analog channels\n')
                line = ff.readline()
            self.rdata = np.array(data)

        if cal:
            self.apply_cal(calinplace)
            
        # Set up an index map based on the channel labels
        self._labelmap = {}
        index = 0
        for channel in self.config.aich:
            if 'label' in channel:
                self._labelmap[channel['label']] = index
            index += 1
            
 
    def __len__(self):
        return self.rdata.shape[0]
        
        
    def __getitem__(self, key):
        
        # If the key is a string, resolve it into an index
        if isinstance(key,str):
            if key in self._labelmap:
                key = self._labelmap[key]
            else:
                raise KeyError('DFILE: Label does not exist: %s\n'%(key))
        if self.cdata:
            return self.cdata[:,key]
        
        return self.rdata[:,key]
            
        

    def apply_cal(self, inplace=False):
        """Apply the analog input calibrations
    DFILE.apply_cal(inplace=False)
    
The "inplace" option conserves memory by overwriting the raw voltage 
values in rdata with their calibrated equivalents.  cdata is then
pointed at rdata instead of being a new array.
"""
        if inplace:
            self.cdata = self.rdata
        else:
            self.cdata = np.zeros_like(self.rdata)
            
        N,chans = self.rdata.shape
        if chans != len(self.config['aich']):
            raise Exception('DFILE: Data format does not match the input configuration.')
        for index in range(chans):
            channel = self.config['aich'][index]
            cal = 1. if 'cal' not in channel else channel['cal']
            zero = 0. if 'zero' not in channel else channel['zero']
            if cal != 1. or zero != 0.:
                self.cdata[:, index] = cal * (self.rdata[:, index] - zero)
            elif self.cdata is not self.rdata:
                self.cdata[:, index] = self.rdata[:, index]


    def get_label(self, index):
        """Get a channel label
    label_string = DFILE.get_label(index)
    
Returns an empty string if the label is not set.
"""
        return '' if 'label' not in self.config['aich'][index] else self.config['aich'][index]['label']


    def t(self):
        """Return the time array for the data samples
    t()
Returns a numpy array beginning with 0 and increasing with the sample interval.
It will be the same length as the number of samples in the data file.

Once this function is called once, its result is stored in the _t member, so 
redundant calls to t() are not inefficient.  This is only a problem if users
try to write to the result of the t() function.  If users need to modify t()'s
result, then a copy should be made
>>> time = datafile.t().copy()
"""
        if self._t is None:
            T = 1./self.config['samplehz']
            self._t = np.arange(0.,T*self.rdata.shape[0],T)
        return self._t





class collection:
    """COLLECTION of dfile objects
The collection is responsible for loading DFILE objects in batches and 
returning slices of the data for plotting.

    Has attributes:
data        A list of the DFILE objects from loaded data
afun        The analysis function passed to the dfile objects
afile       The afile directive to pass to the dfile objects 
            (should be '' or None)

    Creating and populating a COLLECTION object
>>> C = collection(afun = my_analysis_fun, asave=True)
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

By default, the member dfile objects will be directed to attempt to save their
analysis dicitonaries as json files.  This behavior can be suppressed by 
setting asave=False in the collection initializer.
"""

    def __init__(self, afun=None, asave=True):
        self.data = []
        self.afun = afun
        self.afile = None
        self._record = {}
        if asave:
            self.afile = ''
            
    
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
    The root DFILE objects will not be duplicated, but all of the containing
lists and dictionaries will be.
"""
        newbie = collection()
        newbie.data = list(self.data)
        newbie._record = {}
        newbie.afun = self.afun
        return newbie

    def __delitem__(self, item):
        if isinstance(item, int):
            del self.data[item]
            try:
                for this in self._record:
                    del self._record[this][item]
            except:
                sys.stderr.write("Warning: The _record entry seems to be corrupt - clearing.\n")
                self._record = {}
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


    def merge(self, C):
        """Merge another collection into the current one
    C1.merge(C2)
    
Modifies C1 to also include all of the entries in C2.
"""
        if self.afun is None:
            self.afun = C.afun
        elif C.afun is not self.afun:
            raise Exception('The target collection does not have a compatible analysis funciton.')
        self._record = {}
        self.data+=C.data
        


    def add_dir(self, directory = '.', pause=False, verbose=False):
        """add_dir(directory = '.')
    read in all data files in the directory to the collection
When called without an argument, it reads in the current directory.  Files with
a .dat extension will be read in.
"""
        if pause:
            verbose = True
        contents = os.listdir(directory)
        for this in contents:
            # If this is a data file
            if this.endswith('.dat'):
                fullpath = os.path.join(directory,this)
                if verbose:
                    sys.stdout.write(fullpath + '\n')
                self.data.append(dfile(fullpath, afun=self.afun, afile=self.afile))
                if pause:
                    uio = raw_input('Press enter to continue\nEnter "H" to halt:')
                    if uio == 'H':
                        raise Exception('User Halt')
                    
        # Erase the prior assembled analysis items
        self._record = {}
            
    
    def getfiles(self):
        """Construct a list of file names contained in the collection
"""
        return [this.filename for this in self.data]
        
    
    def table(self, entries, sort=None, header=-1,
              fileout=sys.stdout, filename=True):
        """Print a whitespace separated table
    C.table(('analysis_var1', 'analysis_var2'))
    
Accepts an iterable containing string parameter names to find in the data
file anlaysis results.  If entires is a dictionary, then its keywords are
treated as analysis string parameters, and the values are interpreted as
C-style format strings.  For example,

    C.table({'var1':'%.4e', 'var2':'%4d'})

If no format specifier is given, then table will simply use the repr()
function to form an entry.

sort = None
    Accepts a string to indicate an entry by which to sort the table
    For example,
        C.table(('var1','var2'), sort='var2')
        
header = -1
    Indicates how often to print a header.  If header is a negative 
    number, it will be printed once at the top of the table.  If header
    is zero, no header will be printed.  If the header is a positive
    number, it indicates the number of lines of data to print before
    
    
filename = True
    Boolean flag indicating whether to include the source file name as 
    the first column.
    
fileout = stdout
    Accepts an open file specifier or a string path to a file to 
    overwrite.  To perform a different operation (like appending), open
    the file outside of the table function and pass the file object to
    the fileout keyword.
"""
        if isinstance(fileout,str):
            ff = open(fileout,'w+')
        elif isinstance(fileout, file):
            ff = fileout
        else:
            raise Exception('Illegal value for "fileout" keyword.')
        # First, construct a table of the string entries
        table = []
        # Add the filename 
        if filename:
            table.append([
                os.path.split(this.filename)[1]
                for this in self.data])
        
        for entry in entries:
            # Select a function for building the string entry
            if isinstance(entries,dict):
                if entries[entry] is None:
                    mkstr = repr
                # Use a C-style format
                mkstr = lambda(t): entries[entry]%t
            else:
                mkstr = repr
            try:
                table.append([ mkstr(value) for value in self[entry] ])
            except:
                print sys.exc_info()
                raise Exception('TABLE: Failed while building the column for "%s"'%entry)

        if header<0:
            rown = 0
            coln = 0
            if filename:
                table[coln].insert(rown,'file_name')
                coln = 1
            for entry in entries:
                table[coln].insert(rown,entry)
                coln+=1

        elif header>0:
            inserts = int(len(table[0])/header)
            for insn in range(inserts,-1,-1):
                rown = insn * header
                coln = 0
                if filename:
                    table[coln].insert(rown,'file_name')
                    coln = 1
                for entry in entries:
                    table[coln].insert(rown,entry)
                    coln+=1
        
        rowfmt=''
        # Detect the column widths and build the row format
        for col in table:
            mwidth = 0
            for item in col:
                mwidth = max(mwidth, len(item))
            rowfmt += '%' + '%d'%(mwidth + 1) + 's'
        rowfmt += '\n'
        
        # Loop through the rows
        for rindex in range(len(table[0])):
            ff.write( rowfmt%tuple([this[rindex] for this in table]) )
        
        # If TABLE opened the file
        if not isinstance(fileout,file):
            ff.close()
            
