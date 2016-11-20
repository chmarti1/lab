# Read in data files and summarize the results
import os
import sys
import lconfig
from matplotlib import pyplot as plt
import numpy as np
import sys

def loadall(WORKING_DIR = '/home/chris/experiment/data'):
    """Load all *.dat files containing lconfig data
    records, DATA_RECORD = loadall()
    
records = number of files loaded
DATA_RECORD = dictionary containing the results
"""
    DATA_RECORD = { 'data':[],
                    'path':[],
                    'filename':[],
                    'fileprefix':[],     
                    'date':[],
                    'points':[],
                    'excite':[],
                    'V':[],
                    'I':[],
                    'analysis':[]}
                    
    meta_params = [ 'standoff_in', 
                    'plate_tpeak_c', 
                    'fuel_scfh',
                    'oxygen_scfh',
                    'flow_scfh',
                    'ratio_fto']
                    
    # Add the meta params to the initial data record
    for thismeta in meta_params:
        DATA_RECORD[thismeta] = []
    
    contents = os.listdir(WORKING_DIR)
    contents.sort()

    records = 0    
    
    for filename in contents:
        thisfile = os.path.join(WORKING_DIR,filename)
        thisfile = os.path.abspath(thisfile)
        # Only analyze *.dat files    
        fileprefix, _, temp = filename.partition('.')
        if temp == 'dat':
            records += 1
            DATA_RECORD['analysis'].append({})
            DATA_RECORD['filename'].append(filename)
            DATA_RECORD['path'].append(thisfile)
            # load
            data = lconfig.dfile(thisfile)
            DATA_RECORD['data'].append(data)
    
            # Strip the date from the file prefix
            # Isolate only characters that are alpha
            temp = ''
            for thischar in fileprefix:
                if thischar.isalpha():
                    temp += thischar
                else:
                    break
            DATA_RECORD['fileprefix'].append(temp)
            # Test starting time
            DATA_RECORD['date'].append(data.start)
            # Pull the aosignal parameter
            DATA_RECORD['excite'].append(data.config[0].aoch[0].signal)
            # Get the calibrated data
            DATA_RECORD['V'].append(data.data[:,0])
            DATA_RECORD['I'].append(data.data[:,1]*25.242500 - .15605)
            
            # Now read the meta parameters
            for thismeta in meta_params:
                if thismeta in data.config[0].meta:
                    DATA_RECORD[thismeta].append(data.config[0].meta[thismeta])
                else:
                    DATA_RECORD[thismeta].append(None)
            
            # How many data points were collected?
            DATA_RECORD['points'].append(data.data.shape)
    return DATA_RECORD
    
    
    
    
    
def summary(DATA_RECORD):
    """Print a summary of the data to stdout"""
    HEADER = '{:2s} {:5s} {:5s} {:5s} {:5s} {:5s} {:5s} {:8s} {:5s} {:5s} {:5s} {:5s}'
    LINE = '{:2d} {:5.3f} {:5.0f} {:5.2f} {:5.2f} {:5.2f} {:5.3f} {:8s} {:5.1f} {:5.1f} {:5.1f} {:5.1f}'
    records = len(DATA_RECORD['filename'])
    
    print HEADER.format('#', 'S.O.', 'Tpeak', 'O2', 'FG', 'Flow', 'F/O', 'excite', 'Vmax', 'Vmin', 'Imax', 'Imin')
    for index in range(records):
        print LINE.format(  index, 
                DATA_RECORD['standoff_in'][index],
                DATA_RECORD['plate_tpeak_c'][index],
                DATA_RECORD['oxygen_scfh'][index],
                DATA_RECORD['fuel_scfh'][index],
                DATA_RECORD['flow_scfh'][index],
                DATA_RECORD['ratio_fto'][index],
                DATA_RECORD['excite'][index],
                max(DATA_RECORD['V'][index]), min(DATA_RECORD['V'][index]),
                max(DATA_RECORD['I'][index]), min(DATA_RECORD['I'][index]))
    
    


def proto(c, v):
    """Prototype I-V characteristic.
    i = proto(c,v)
    
c is a 6-element coefficient array indicating the I-V shape.
v is an array of voltages at which to calculate the current.

proto() asserts a piece-wise linear model for I-V characteristics.
"""
    a1 = c[0]   # negative saturation slope
    a2 = c[1]   # ohmic slope
    a3 = c[2]   # positive saturation slope
    v0 = c[3]   # floating potential
    b1 = c[4]   # negative saturation offset
    b3 = c[5]   # positive saturation offset
    # calculate the ohmic regime offset
    b2 = -v0*a2     # The ohmic offset is just related to the slope
    # calculate the saturation voltages
    # a1 v1 + b1 == a2 v1 + b2
    v1 = (b1-b2)/(a2-a1)
    # a3 v2 + b3 == a2 v3 + b2
    v2 = (b3-b2)/(a2-a3)

    i = np.zeros(v.shape)        
    index = v<v1
    i[index] = a1*v[index] + b1
    
    index = (~index) * (v<v2)
    i[index] = a2*v[index] + b2
    
    index = v>=v2
    i[index] = a3*v[index] + b3
    
    return i


def proto2(c,v):
    a1 = c[0]   # negative saturation slope
    a2 = c[1]   # ohmic slope
    a3 = c[2]   # positive saturation coefficient
    v0 = c[3]   # floating potential
    b1 = c[4]   # negative saturation offset
    i2 = c[5]   # positive saturation current

    # calculate the ohmic regime offset
    b2 = -v0*a2     # The ohmic offset is just related to the slope
    # calculate the saturation voltages
    # a1 v1 + b1 == a2 v1 + b2
    v1 = (b1-b2)/(a2-a1)
    # a2 v2 + b2 == i2
    v2 = (i2-b2)/a2
    
    i = np.zeros(v.shape)        
    index = v<v1
    i[index] = a1*v[index] + b1
    
    index = (~index) * (v<v2)
    i[index] = a2*v[index] + b2
    
    index = v>=v2
    i[index] = a3*v[index] + b3
    
    
def showindex(DATA_RECORD, index, style='b.'):
    """Plot the data from data index
    showindex(DATA_RECORD, index, style='b.')
"""
    if hasattr(index,'__iter__'):
        color = 'bgrcmyk'
        marker = '.s^do'
        style = []
        for m in marker:
            for c in color:
                style.append(c+m)
        ii = 0
        for thisindex in index:
            showindex(DATA_RECORD, thisindex, style[ii%len(style)])
            ii+=1
        return
    
    V = DATA_RECORD['V'][index]
    I = DATA_RECORD['I'][index]
    N = len(V)
    T = 1./DATA_RECORD['data'][index].config[0].samplehz
    time = np.arange(0., N*T, T)

    f1 = plt.figure(1)
    f1.clf()
    ax = f1.add_subplot(111)
    ax.plot(time, I, 'g.-')
    ax.set_xlabel('Time (s)')
    ax.set_ylabel('Current (uA)')    
    
    f2 = plt.figure(2)
    f2.hold('on')
    ax = f2.gca()
    ax.plot(V,I,style)
    ax.set_xlabel('Voltage (V)')
    ax.set_ylabel('Current (uA)')
    # if the analysis contains a fit
    if 'c' in DATA_RECORD['analysis'][index]:
        c = DATA_RECORD['analysis'][index]['c']
        vv = np.arange(min(V),max(V),.05)
        ii = proto(c,vv)
        ax.plot(vv,ii,style[0]+'-')
    f2.canvas.draw()
    f2.canvas.flush_events()


   
def fitindex(RECORD, index,verbose=True):
    """Perform a piecewise fit of the data
    fitindex(RECORD, index)

Stores the result in the 'analysis' dictionary:
RECORD['analysis'][index]
"""
    if hasattr(index,'__iter__'):
        for thisindex in index:
            if verbose:
                print "Fitting index: ", thisindex
            go_f = True
            while go_f:
                try:
                    fitindex(RECORD,thisindex,verbose=False)
                except:
                    print "  INDEX FAILED", thisindex
                go_f = False
        return

    # Calculate the error2 over the fit
    def e2(c, v, i):
        delta = i - proto(c,v)
        return (delta*delta).sum()
        
    # Estimate the hessian
    def hessian(c,v,i,f):
        epsilon = 1e-5
        N = len(c)
        values = np.zeros((2,)*N)
        vindex = [0]*N
        values[tuple(vindex)] = f(c,v,i)
        # Evaluate the error at perturbed values of c
        for ii in range(N):
            vindex[ii] = 1
            ctest = np.array(c)
            ctest[ii] += epsilon
            values[tuple(vindex)] = f(ctest,v,i)
            vindex[ii] = 0
        # perturb combinations of indices
        for ii in range(0,N-1):
            for jj in range(ii+1,N):
                vindex[ii] = 1
                vindex[jj] = 1
                ctest = np.array(c)
                ctest[ii] += epsilon
                ctest[jj] += epsilon
                values[tuple(vindex)] = f(ctest,v,i)
                vindex[ii] = 0
                vindex[jj] = 0
        J = np.zeros((N,))
        H = np.zeros((N,N))
        # Go calculate the Hessian and the Jacobian
        # Start with the diagonal terms
        for ii in range(0,N):
            e0 = values[tuple(vindex)]
            vindex[ii] = 1
            epos = values[tuple(vindex)]
            ctest = np.array(c)
            ctest[ii] -= epsilon
            eneg = f(ctest,v,i)
            J[ii] = (epos - e0)/epsilon
            H[ii,ii] = (epos - 2*e0 + eneg)/epsilon/epsilon
            vindex[ii] = 0
        for ii in range(0,N-1):
            for jj in range(ii+1,N):
                e00 = values[tuple(vindex)]
                vindex[ii] = 1
                e10 = values[tuple(vindex)]
                vindex[jj] = 1
                e11 = values[tuple(vindex)]
                vindex[ii] = 0
                e01 = values[tuple(vindex)]
                vindex[jj] = 0
                H[ii,jj] = (e00 - e10 - e01 + e11) / epsilon / epsilon
                H[jj,ii] = H[ii,jj]
        return J,H
    
    # Initialize the c array

    thresh = 1e-6
    maxiter = 1000
    dcmax = 0.1
    # Iterate!
    v = RECORD['V'][index]
    i = RECORD['I'][index]
    # estimate some initial values
    a2 = 10.
    # estimate v0
    v0 = v[np.argmin(np.abs(i))]
    # and saturation offsets
    # grab two points in the saturation regimes
    vtest1 = -5.
    vtest2 = -7.
    itest1 = i[np.argmin(np.abs(v-vtest1))]
    itest2 = i[np.argmin(np.abs(v-vtest1))]
    a1 = (itest2-itest1)/(vtest2-vtest1)
    b1 = itest1 - vtest1*a1
    # now the positive saturation regime
    vtest1 = 5.
    vtest2 = 7.
    itest1 = i[np.argmin(np.abs(v-vtest1))]
    itest2 = i[np.argmin(np.abs(v-vtest1))]
    a3 = (itest2-itest1)/(vtest2-vtest1)
    b3 = itest1 - vtest1*a3
    
    c = np.array([a1,a2,a3,v0,b1,b3])
    iter_f = True
    count = 0
    #ee_old = float('inf')
    while iter_f and count<maxiter:
        count += 1
        try:
            J,H = hessian(c,v,i,e2)
            dc = np.linalg.solve(H,J)
        except:
            raise Exception("Inversion failed.")

        # Do some checking on the guess
        adc = abs(dc)   # What are the magnitudes of the changes?
        mdc = max(adc)  # What is the largest change?
        if mdc>dcmax:
            dc *= dcmax / mdc   # Rescale the vector
            iter_f = True   # Fail the convergence check
        else:
            iter_f = mdc < thresh

        if verbose:
            print count,":",c

        # check error criteria
#        ctest = c - dc
#        ee_new = e2(ctest,v,i)
#        while ee_new > ee_old and count<maxiter:
#            print "dc",dc
#            print "e",ee_new,ee_old
#            count += 1
#            dc *= 0.5
#            ctest = c - dc
#            ee_new = e2(ctest,v,i)
#        ee_old = ee_new
#        c = ctest
        c -= dc
    if count == maxiter:
        raise Exception("Exceeded iteration count.")

    RECORD['analysis'][index]['c'] = c
    RECORD['analysis'][index]['e2'] = e2(c,v,i)



def Rfromindex(record,regime,index):
    """Return a regime's resistivity
    Rfromindex(record,regime,index)
record is the data record dictionary
regime is 1,2, or 3 indicating negative saturation, ohmic, or saturation
index is the data set to retrieve

returns None if fitindex() has not been run successfully.
"""
    if hasattr(index,'__iter__'):
        return np.array(
    [Rfromindex(record,regime,thisindex) for thisindex in index])
    
    if 'c' in record['analysis'][index]:
        return 1./record['analysis'][index]['c'][regime-1]
    else:
        return None


def Xfromindex(record,X,index):
    """Assemble a numpy array of the dictionary item X
   ARR = Xfromindex(record,X,index)

record is the data record dictionary
X is the dictionary key to retrieve
index is the index or indices to retrieve

s = Xfromindex(record, 'standoff_in', [10,11,12])

Returns a three-element array indicating the standoff of tests 10,11, and 12."""

    if hasattr(index,'__iter__'):
        return np.array(
    [Xfromindex(record,X,thisindex) for thisindex in index])
    
    return record[X][index]
    
    
def VSfromindex(record,X,index):
    """Assemble a numpy array of the saturation voltage 
"""