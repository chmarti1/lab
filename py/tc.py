"""Functions for converting thermocouple voltages and temperatures

These are based on the ITS-90 TC coefficients taken from
https://srdata.nist.gov/its90/main/its90_main_page.html

::Use::
    Below is an example using the tc package to calculate the temperature of
    a type K thermocouple reading 4.53mV in a 24.5 C room.
    
>>> import tc
>>> mV = 4.53 # a hypothetical mV measurement of a type K thermocouple
>>> Tcj = 24.5 # a hypothetical cold junction temperature measurement
>>> T = tc.K.T(mV, units='C', Tcj = Tcj)

    All functions accept array-line arguments to execute multiple simultaneous
    evaluations.  TC uses numpy to efficiently perform array operations.
>>> mV = tc.K.mV([100., 120., 140.])

::Provides objects::
    E, B, J, K, N, R, S, and T which provide calibration functions for 
    thermocouples of the same names.

    'provides' is a list containing these _tc objects.
    
::Provides methods::
    test() plots the characteristics of each TC and estimates temperature error
    by T - T(mv(T))
    
::Provides classes::
    _tc, which is the template class used by all thermocouple types

"""
import numpy as np


class _tc:
    """Thermocouple type class
    _tc(tctype, Tlim, Tcoef, Vlim, Vcoef)
    
tctype      Thermocouple type character.  Identifies the thermocouple type 
            by its letter name (e.g. 'K', 'J', etc)
Tlim        Temperature limits list.  In the form [T0, T1, T2], this defines
            two groups of temperature coefficients; one valid between T0 and T1
            and one valid between T1 and T2
            
Tcoef       Temperature coefficients list.  In the form [C0, C1], this defines
            coefficients of the ITS-90 polynomial.  C0 is valid between T0 and 
            T1, C1 is valid between T1 and T2.  Each CX is a list of 
            coefficients forming a polynomial returning the TC voltage in mV as
            a function of temperature in degrees C.
            
Vlim        Identical to the temperature limits, this list identifies the
            inverse limits in mV.
            
Vcoef       Identical to the Tcoef list, this determines the inverse polynomial
            coefficients.
"""
    def __init__(self, tctype, Tlim, Tcoef, Vlim, Vcoef):
        self.tcType = tctype
        self.Tlim = list(Tlim)
        self.Tcoef = list(Tcoef)
        self.Vlim = list(Vlim)
        self.Vcoef = list(Vcoef)
        # Error checking
        if len(self.Tlim)-1 != len(self.Tcoef):
            raise Exception('Tlim and Tcoef size missmatch')
        for this in self.Tcoef:
            if not isinstance(this,list):
                raise Exception('Tcoef must be a list of lists.')
        if len(self.Vlim)-1 != len(self.Vcoef):
            raise Exception('Vlim and Vcoef size missmatch')
        for this in self.Vcoef:
            if not isinstance(this,list):
                raise Exception('Vcoef must be a list of lists.')
        
    def _polyval(self, Clim, C, x):
        if not isinstance(x,np.ndarray):
            x = np.array(x)
        if x.ndim < 1:
            x = x.reshape((x.size,))
        y = np.zeros_like(x)
        Idone = (x < Clim[0])
        if Idone.any():
            raise Exception('Values are too low')
        for lindex in range(len(C)):
            I = (x <= Clim[lindex+1]) * (~Idone)
            Idone += I
            y[I] = C[lindex][-1]
            for c in C[lindex][-2::-1]:
                y[I] = (y[I]*x[I] + c)
        if not Idone.all():
            raise Exception('Values are too high')
        return y
        
    def toKelvin(Tcelcius):
        """Convert from C to K"""
        return Tcelcius + 273.15
    def fromKelvin(Tkelvin):
        """Convert from K to C"""
        return Tkelvin - 273.15
    def toRankine(Tcelcius):
        """Convert from C to R"""
        return (Tcelcius+273.15)*1.8
    def fromRankine(Trankine):
        """Convert from R to C"""
        return Trankine/1.8 - 273.15
    def toFarenheit(Tcelcius):
        """Convert from C to F"""
        return Tcelcius*1.8 + 32.
    def fromFarenheit(Tfarenheit):
        """Convert from F to C"""
        return (Tfarenheit-32.)/1.8
        
    def mV(self,T, units='C'):
        """Calculate the TC voltage in mV
    mV(T, units='C')

T is the temperature in units determined by the units keyword.
units may be 'C' for celcius, 'K' for kelvin, 'F' for Farenheit, or 'R' for
Rankine.
"""
        if units=='K':
            T = fromKelvin(T)
        elif units=='R':
            T = fromRankine(T)
        elif units=='F':
            T = fromFarenheit(T)
        elif units=='C':
            pass
        else:
            raise Exception('Unrecognized temperature units')
        return self._polyval(self.Tlim,self.Tcoef,T) 


    def T(self,mV, units='C', Tcj = None):
        """Calculate the TC temperature from mV
    T(mV, units='C', Tcj = None)

mV is the thermocouple voltage in units determined by the units keyword.

units may be 'C' for celcius, 'K' for kelvin, 'F' for Farenheit, or 'R' for
Rankine.

Tcj is the cold junction temperature given in the units indicated by the
'units' keyword.  When Tcj is None, the ITS-90 default of 0 degrees C is used.
When Tcj is present, the mV function is called to add the cold-junciton
voltage from the measured voltage, and T is called recursively.
    tcobj.T(mV, Tcj=Tcj)
is equivalent to 
    tcobj.T(mV+tcobj.mV(Tcj), Tcj=None)
"""

        if Tcj is not None:
            mV += self.mV(Tcj,units=units)
        T = self._polyval(self.Vlim,self.Vcoef,mV)
        if units=='K':
            T = toKelvin(T)
        elif units=='R':
            T = toRankine(T)
        elif units=='F':
            T = toFarenheit(T)
        elif units=='C':
            pass
        else:
            raise Exception('Unrecognized temperature units')
        return T




def test():
    print "Importing matplotlib for plots..."
    import matplotlib.pyplot as plt
    
    cal_fig = plt.figure(1)
    cal_fig.clf()
    cal_ax = cal_fig.add_subplot(111)
    cal_ax.set_xlabel('Temperature (C)')
    cal_ax.set_ylabel('TC voltage (mV)')
    cal_ax.grid('on')

    err_fig = plt.figure(2)
    err_fig.clf()
    err_ax = err_fig.add_subplot(111)
    err_ax.set_xlabel('Temperature (C)')
    err_ax.set_ylabel('Approx. Error(C)')
    err_ax.grid('on')
    
    for this in provides:
        Tlow = this.Tlim[0]
        Thigh = this.Tlim[-1]
        T = np.arange(Tlow, Thigh, 10.)
        cal_ax.plot(T, this.mV(T), label=this.tcType)
        # adjust the temperature limits to be sensitive to the mV limits
        Tlow = max(Tlow, this.T(this.Vlim[0]))
        Thigh = min(Thigh, this.T(this.Vlim[-1]))
        T = np.arange(Tlow, Thigh, 10.)
        err_ax.plot(T, T - this.T(this.mV(T)), label=this.tcType)
    cal_ax.legend(loc=0)
    err_ax.legend(loc=0)


E = _tc('E',
    [-270., 0., 1000.],
    [[0.000000000000, 0.586655087080e-1, 0.454109771240e-4, -0.779980486860e-6, -0.258001608430e-7, -0.594525830570e-9, -0.932140586670e-11, -0.102876055340e-12, -0.803701236210e-15, -0.439794973910e-17, -0.164147763550e-19, -0.396736195160e-22, -0.558273287210e-25, -0.346578420130e-28],
     [0.000000000000, 0.586655087100e-1, 0.450322755820e-4, 0.289084072120e-7, -0.330568966520e-9, 0.650244032700e-12, -0.191974955040e-15, -0.125366004970e-17, 0.214892175690e-20, -0.143880417820e-23, 0.359608994810e-27]],
    [-8.825, 0., 76.373],
    [[0.0000000, 1.6977288e1, -4.3514970e-1, -1.5859697e-1, -9.2502871e-2, -2.6084314e-2, -4.1360199e-3, -3.4034030e-4, -1.1564890e-5],
     [0.0000000, 1.7057035e1, -2.3301759e-1, 6.5435585e-3, -7.3562749e-5, -1.7896001e-6, 8.4036165e-8, -1.3735879e-9, 1.0629823e-11, -3.2447087e-14]]
    )

B = _tc('B',
    [0., 630.615, 1820.],
    [[0.000000000000, -0.246508183460e-3, 0.590404211710e-5, -0.132579316360e-8, 0.156682919010e-11, -0.169445292400e-14, 0.629903470940e-18],
     [-0.389381686210e1, 0.285717474700e-1, -0.848851047850e-4, 0.157852801640e-6, -0.168353448640e-9, 0.111097940130e-12, -0.445154310330e-16, 0.989756408210e-20, -0.937913302890e-24]],
    [0.291, 2.431, 13.820],
    [[9.8423321e1, 6.9971500e2, -8.4765304e2, 1.0052644e3, -8.3345952e2, 4.5508542e2, -1.5523037e2, 2.9886750e1, -2.4742860],
     [2.1315071e2, 2.8510504e2, -5.2742887e1, 9.9160804, -1.2965303, 1.1195870e-1, -6.0625199e-3, 1.8661696e-4, -2.4878585e-6]]
    )

J = _tc('J',
    [-210., 760., 1200.], 
    [[0.000000000000, 0.503811878150e-1, 0.304758369300e-4, -0.856810657200e-7, 0.132281952950e-9, -0.170529583370e-12, 0.209480906970e-15, -0.125383953360e-18, 0.156317256970e-22],
     [0.296456256810e3, -0.149761277860e1, 0.317871039240e-2, -0.318476867010e-5, 0.157208190040e-8, -0.306913690560e-12]],
    [-8.095, 0., 42.919, 69.553],
    [[0.0000000, 1.9528268e1, -1.2286185, -1.0752178, -5.9086933e-1, -1.7256713e-1, -2.8131513e-2, -2.3963370e-3, -8.3823321e-5],
     [0.000000, 1.978425e1, -2.001204e-1, 1.036969e-2, -2.549687e-4, 3.585153e-6, -5.344285e-8, 5.099890e-10],
     [-3.11358187e3, 3.00543684e2, -9.94773230, 1.70276630e-1, -1.43033468e-3, 4.73886084e-6]]
    )

K = _tc('K',
    [-270., 0., 1372.],
    [[0.000000000000, 0.394501280250e-1, 0.236223735980e-4, -0.328589067840e-6, -0.499048287770e-8, -0.675090591730e-10, -0.574103274280e-12, -0.310888728940e-14, -0.104516093650e-16, -0.198892668780e-19, -0.163226974860e-22],
     [-0.176004136860e-1, 0.389212049750e-1, 0.185587700320e-4, -0.994575928740e-7, 0.318409457190e-9, -0.560728448890e-12, 0.560750590590e-15, -0.320207200030e-18, 0.971511471520e-22, -0.121047212750e-25]],
    [-5.891, 0., 20.644, 54.886],
    [[0.000000, 2.5173462e1, -1.1662878, -1.0833638, -8.9773540e-1, -3.7342377e-1, -8.6632643e-2, -1.0450598e-2, -5.1920577e-4],
     [0.000000, 2.508355e1, 7.860106e-2, -2.503131e-1, 8.315270e-2, -1.228034e-2, 9.804036e-4, -4.413030e-5, 1.057734e-6, -1.052755e-8],
     [-1.318058e2, 4.830222e1, -1.646031e0, 5.464731e-2, -9.650715e-4, 8.802193e-6, -3.110810e-8]]
     )

N = _tc('N',
    [-270., 0., 1300.],
    [[0.000000000000E+00,
      0.261591059620E-01,
      0.109574842280E-04,
      -0.938411115540E-07,
      -0.464120397590E-10,
      -0.263033577160E-11,
      -0.226534380030E-13,
      -0.760893007910E-16,
      -0.934196678350E-19],
     [0.000000000000E+00,
       0.259293946010E-01,
       0.157101418800E-04,
       0.438256272370E-07,
       -0.252611697940E-09,
       0.643118193390E-12,
       -0.100634715190E-14,
       0.997453389920E-18,
       -0.608632456070E-21,
       0.208492293390E-24,
       -0.306821961510E-28]],
    [-3.99, 0., 20.613, 47.513],
    [[0.0000000E+00,
      3.8436847E+01,
      1.1010485E+00,
      5.2229312E+00,
      7.2060525E+00,
      5.8488586E+00,
      2.7754916E+00,
      7.7075166E-01,
      1.1582665E-01,
      7.3138868E-03],
     [0.0000000E+00,
      3.8689600E+01,
      -1.0826700E+00,
      4.7020500E-02,
      -2.1216900E-06,
      -1.1727200E-04,
      5.3928000E-06,
      -7.9815600E-08],
     [1.9724850E+01,
      3.3009430E+01,
      -3.9151590E-01,
      9.8553910E-03,
      -1.2743710E-04,
      7.7670220E-07]]
    )

R = _tc('R',
    [-50., 1064.180, 1664.5, 1768.1],
    [[0.000000000000E+00,
      0.528961729765E-02,
      0.139166589782E-04,
      -0.238855693017E-07,
      0.356916001063E-10,
      -0.462347666298E-13,
      0.500777441034E-16,
      -0.373105886191E-19,
      0.157716482367E-22,
      -0.281038625251E-26],
     [0.295157925316E+01,
      -0.252061251332E-02,
      0.159564501865E-04,
      -0.764085947576E-08,
      0.205305291024E-11,
      -0.293359668173E-15],
     [0.152232118209E+03,
      -0.268819888545E+00,
      0.171280280471E-03,
      -0.345895706453E-07,
      -0.934633971046E-14]],
    [-.226, 1.923, 13.228, 19.739, 21.103],
    [[0.0000000E+00,
      1.8891380E+02,
      -9.3835290E+01,
      1.3068619E+02,
      -2.2703580E+02,
      3.5145659E+02,
      -3.8953900E+02,
      2.8239471E+02,
      -1.2607281E+02,
      3.1353611E+01,
      -3.3187769E+00],
     [1.3345845E+01,
      1.4726446E+02,
      -1.8440248E+01,
      4.0311297E+00,
      -6.2494284E-01,
      6.4684120E-02,
      -4.4587504E-03,
      1.9947101E-04,
      -5.3134018E-06,
      6.4819762E-08],
     [-8.1995994E+01,
      1.5539620E+02,
      -8.3421977E+00,
      4.2794335E-01,
      -1.1915779E-02,
      1.4922901E-04],
     [3.4061778E+04,
      -7.0237292E+03,
      5.5829038E+02,
      -1.9523946E+01,
      2.5607402E-01]]
    )

S = _tc('S',
    [-50., 1064.18, 1664.5, 1768.1],
    [[0.000000000000E+00,
      0.540313308631E-02,
      0.125934289740E-04,
      -0.232477968689E-07,
      0.322028823036E-10,
      -0.331465196389E-13,
      0.255744251786E-16,
      -0.125068871393E-19,
      0.271443176145E-23],
     [0.132900444085E+01,
      0.334509311344E-02,
      0.654805192818E-05,
      -0.164856259209E-08,
      0.129989605174E-13],
     [0.146628232636E+03,
      -0.258430516752E+00,
      0.163693574641E-03,
      -0.330439046987E-07,
      -0.943223690612E-14]],
    [-.235, 1.874, 11.950, 17.536, 18.693],
    [[0.0000000E+00,
      1.8494946E+02,
      -8.0050406E+01,
      1.0223743E+02,
      -1.5224859E+02,
      1.8882134E+02,
      -1.5908594E+02,
      8.2302788E+01,
      -2.3418194E+01,
      2.7978626E+00],
     [1.2915072E+01,
      1.4662989E+02,
      -1.5347134E+01,
      3.1459460E+00,
      -4.1632578E-01,
      3.1879638E-02,
      -1.2916375E-03,
      2.1834751E-05,
      -1.4473795E-07,
      8.2112721E-09],
     [-8.0878011E+01,
      1.6215731E+02,
      -8.5368695E+00,
      4.7196870E-01,
      -1.4416937E-02,
      2.0816189E-04],
     [5.3338751E+04,
      -1.2358923E+04,
      1.0926576E+03,
      -4.2656937E+01,
      6.2472054E-01]]
    )

T = _tc('T',
    [-270., 0., 400.],
    [[0.000000000000, 0.387481063640e-1, 0.441944343470e-4, 0.118443231050e-6, 0.200329735540e-7, 0.901380195590e-9, 0.226511565930e-10, 0.360711542050e-12, 0.384939398830e-14, 0.282135219250e-16, 0.142515947790e-18, 0.487686622860e-21, 0.107955392700e-23, 0.139450270620e-26, 0.797951539270e-30],
     [0.000000000000, 0.387481063640e-1, 0.332922278800e-4, 0.206182434040e-6, -0.218822568460e-8, 0.109968809280e-10, -0.308157587720e-13, 0.454791352900e-16, -0.275129016730e-19]],
    [-5.603, 0., 20.872],
    [[0.0000000, 2.5949192e1, -2.1316967e-1, 7.9018692e-1, 4.2527777e-1, 1.3304473e-1, 2.0241446e-2, 1.2668171e-3],
     [0.000000, 2.592800e1, -7.602961e-1, 4.637791e-2, -2.165394e-3, 6.048144e-5, -7.293422e-7]]
    )
    


# Overload the mV function for type K to include the exponential terms
# Type K thermocouple calibrations are reported differently in ITS-90.  They
# share the same polynomial coefficients as the other thermocouples, but they
# have additional exponential terms.
def KmV(self, T, units='C'):
    a0 = 0.118597600000
    a1 = -0.118343200000e-3
    a2 = 0.126968600000e3
    if units=='K':
        T = fromKelvin(T)
    elif units=='R':
        T = fromRankine(T)
    elif units=='F':
        T = fromFarenheit(T)
    elif units=='C':
        pass
    else:
        raise Exception('Unrecognized temperature units')
    if not isinstance(T,np.ndarray):
        T = np.array(T)
    if T.ndim < 1:
        T = T.reshape((T.size,))
    mV = _tc.mV(self,T,'C')
    I = T>0.
    mV[I] += a0 * np.exp(a1*(T[I]-a2)**2.)
    return mV

K.mV = type(K.mV)(KmV, K, _tc)
del KmV


# Build the list of available thermocouples
provides = [B, E, J, K, N, R, S, T]