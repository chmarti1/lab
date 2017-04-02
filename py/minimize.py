"""MINIMIZE

Numerical minimization for scalar functions of many dimensions

bracket1d()     Given an initial guess, find an interval bracketing
                a minimum in 1D
min1d()         Minimize a 1D function.

"""

import numpy as np
import sys

golden = (3. - np.sqrt(5.))/2.

class wfunNd:
    """Wrapper function evaluating a function of the form
    y = f(x0 + s*dx)
where x and dx are N-dimensional vectors, and s and y are scalars.
For example:
    f = lambda(x):np.linalg.norm(x)
    wf = wfunNd(f)

wf(s) will return the value of f() along a line in x-space that goes 
through the point, x0 in a direction dx.  wf will have members
    x0 - the point through which the line passes
    dx - the direction of the line
    invert - a True/False flag.  When True, wf() returns -fun()
    fun - the function being evaluated

    f = lambda(x):np.linalg.norm(x)
    wf = wfunNd(f)
    wf.x0 = np.array((2,2))
    wf.dx = np.array((1,-.5))
    wf.invert = False
    wf(0.5)
returns 3.0516389039334255, equivalent to f((2.5, 1.75))
    wf(1.0)
returns 3.3541019662496847, equivalent to f((3., 1.5))
"""
    def __init__(self, fun):
        self.fun = fun
        self.invert = False
        self.x0 = None
        self.dx = None
        
    def __call__(self,s,*varg,**kwarg):
        x = self.x0 + s*self.dx
        if self.invert:
            return -self.fun(x,*varg,**kwarg)
        else:
            return self.fun(x,*varg,**kwarg)




def bracket1d(fun, x0, small=1e-10, epsilon=1e-5, dx=None, varg=(), kwarg={}, 
              Nmax=50, verbose=False):
    """BRACKET1D
    Given a one-dimensional function and an initial guess for a minimum,
find search for an interval that brackets a minimum.  Returns six values
    a,b,c,f(a),f(b),f(c) = bracket1d(f, x0)
    
The values are ordered such that b is between a and c, and the golden
ratio is usually used to place b between a and c.  However there are no 
garantees on how a and c will be ordered, and there are cases where the
spacing of b between a and c is not carefully controlled.

BRACKET1D estimates the first and second derivative of f() at x0 and 
uses them find the down hill direction and a length over which the 
first derivative can be expected to approach zero.  BRACKET1D 
repeatedly increases or decreases that step size by the golden ratio
until a triplet containing a minimum has been found.
    
Optional parameters are
small   A small number; used to determine the smallest allowable step
        Smaller numbers are considered "effectively" zero. (1e-10)
epsilon Fractional perturbation; used to perturb x to estimate 
        derivatives on f.  dx = max(small, epsilon*abs(x))
dx      User-specified length scale.  Instead of estimating derivatives,
        the user can specify a perturbation over which to start the
        search for a minimum.
varg    A tuple of arguments to pass to f()
kwarg   A dict of keyword arguments to pass to f()
Nmax    Maximum number of function calls permitted
verbose
    A boolean flag indicating whether to print the series of estimates 
    to stdout.
"""
    y0 = fun(x0, *varg, **kwarg)
    if verbose:
        sys.stdout.write('\nBRACKET1D   Starting at\n')
        sys.stdout.write(' f(%.5e) = %.5e\n'%(x0,y0))
    # If the user has specified an initial perturbation in x
    if dx is not None:
        if verbose:
            sys.stdout.write('Given length scale: %f\n'%dx)
        x1 = x0 + dx
        y1 = fun(x1, *varg, **kwarg)
        Nmax-=1
        if verbose:
            sys.stdout.write(' +-- f(%.5e) = %.5e\n'%(x0,y0))
        # If the user pointed us uphill, swap!
        if y1 > y0:
            if verbose:
                sys.stdout.write(' +-- It is pointing uphill; reversing.\n')
            temp = x0
            x0 = x1
            x1 = temp
            temp = y0
            y0 = y1
            y1 = temp
            dx = -dx
    # If there is no initial perturbation, we need to determine one
    else:
        #Estimate the 1st and 2nd derivatives
        y0 = fun(x0, *varg, **kwarg)
        Nmax-=1
        dx = max(abs(x0*epsilon), small)
        x1 = x0 + dx
        y1 = fun(x1, *varg, **kwarg)
        Nmax-=1
        x2 = x0 - dx
        y2 = fun(x2, *varg, **kwarg)
        Nmax-=1
        
        df = (y1 - y2)/2./dx
        ddf = (y1 - 2.*y0 + y2)/dx/dx

        if verbose:
            sys.stdout.write('Automatically detecting a length scale\n')
            sys.stdout.write(' +-- df/dx   = %.5e\n +-- d2f/dx2 = %.5e\n'%(df,ddf))

        # Do a sanity check on the derivatives
        if df == 0.:
            if verbose:
                sys.stdout.write(' +-- The derivative is zero.  This is an extremum!\n')
            if ddf<0:
                return x1,x0,x2,y1,y0,y2
            else:
                raise Exception('BRACKET1D: The initial guess seems to be at a maximum.')
        # If the local curvature is very small, use the first derivative
        elif abs(ddf) < max(small, epsilon*abs(df)):
            # Go downhill, but with a length scale determined by the size of
            # f() and f'()
            dx = abs(y0/df)
            dx = -0.5*np.sign(df)*max(small, dx)
            if verbose:
                sys.stdout.write(' +-- There appears to be very little curvature here.\n')
                sys.stdout.write(' +-- Estimated an x scale from f(x) and df/dx\n +-- dx= %.5e\n'%dx)
        else:
            # Go downhill with a scale determined by the curvature
            dx = -df / abs(ddf)
            if verbose:
                sys.stdout.write(' +-- Parabolic projection for the minimum\n')
                sys.stdout.write(' +-- dx = %.5e\n'%dx)
        # Re-evaluate the x1,y1 pair
        x1 = x0 + dx
        y1 = fun(x1, *varg, **kwarg)
        Nmax-=1
        if verbose:
            sys.stdout.write('f(%.5e) = %.5e\n'%(x1,y1))
    # Now, get started
    multiplier = golden / (1.-golden)
    dx /= multiplier
    for count in range(Nmax):
        x2 = x1 + dx
        y2 = fun(x2, *varg, **kwarg)
        if y2 > y1:
            if verbose:
                sys.stdout.write('Found an interval bracketing a minimum.\n')
                sys.stdout.write('[-!-]       0             1             2\n')
                sys.stdout.write('  x   %13.5e %13.5e %13.5e\n'%(x0,x1,x2))
                sys.stdout.write('  y   %13.5e %13.5e %13.5e\n\n'%(y0,y1,y2))
            return x0,x1,x2,y0,y1,y2
        if verbose:
            sys.stdout.write('[%3d] f(%.5e) = %.5e\n'%(count,x2,y2))
        # quadratic approximation for the derivatives
        df = (y2-y0)/(x2-x0)
        x20 = x2-x0
        x10 = x1-x0
        x21 = x2-x1
        ddf = 2*(y0*x21 - y1*x20 + y2*x10)/(x20*x10*x21)
        # If the length scale is longer than dx, grow it
        if abs(df) > abs(ddf*dx):
            dx /= multiplier
            if verbose:
                sys.stdout.write('      Growing x increment to %.5e\n'%dx)
        # otherwise shrink it
        else:
            dx *= multiplier
            if verbose:
                sys.stdout.write('      Shrinking x increment to %.5e\n'%dx)
        # Shift out x0
        x0 = x1
        y0 = y1
        x1 = x2
        y1 = y2
        
    raise Exception('BRACKET1D: Failed to bracket an interval with a minimum.')
    

def min1d(fun, x0, x1=None, xy=False, Nmax=200, 
          small=1e-10, epsilon=1e-5, varg=(), kwarg={}, verbose=False):
    """MIN1D  Minimize a 1-D function
    x = min1d(f, x0)
        OR
    x,y = min1d(f, x0, xy=True)

Mandatory arguments:
f   is the scalar function to be minimized.  It must answer to the call
    signature y = f(x), where y and x are scalar floats.
    
x0  is the initial x-value to use while searching for the minimum

Optional arguments:
x1  a second value (in addition to x0) used to bracket a solution.  If
    x1 is not specified, then bracket1d will be called with the small 
    and epsilon parameters.  Otherwise, x1 and x0 are used to specify a
    range on x in which to search for a minimum.

xy  Normally, min1d only returns the converged value for x.  If xy is
    set to True, then min1d also returns the function's value, y in a
    tuple with x;
        x,y = min1d(f,x0,xy=True)

Nmax The maximum number of function calls permitted.  If BRACKET1D is
    called, the function calls there are NOT counted against Nmax.

verbose
    A boolean flag indicating whether to print iteration progress to 
    stdout
"""
    if verbose:
        sys.stdout.write('\nMIN1D\n')

    if x1 is None:
        x0,x1,x2,y0,y1,y2 = bracket1d(fun,x0,varg=varg,kwarg=kwarg,verbose=verbose)
        # Force the points to be ordered so that the smallest interval is 
        # x0 to x1.
        if abs(x1-x0)>abs(x2-x1):
            temp = y2
            y2 = y0
            y0 = temp
            temp = x2
            x2 = x0
            x0 = temp
        if verbose:
            sys.stdout.write('Resuming minimization...\n')
    else:
        y0 = fun(x0,*varg,**kwarg)
        Nmax -= 1
        x2 = x1
        y2 = fun(x2,*varg,**kwarg)
        Nmax -= 1
        x1 = golden * (x2 - x0) + x0
        y1 = fun(x1,*varg,**kwarg)
        Nmax -= 1
        
    for count in range(Nmax):
        if verbose:
            sys.stdout.write('[%3d]       0             1             2\n'%count)
            sys.stdout.write('  x   %13.5e %13.5e %13.5e\n'%(x0,x1,x2))
            sys.stdout.write('  y   %13.5e %13.5e %13.5e\n\n'%(y0,y1,y2))
        xnew = golden * (x0 - x2) + x2
        ynew = fun(xnew, *varg, **kwarg)
        if ynew > y1:
            y2 = y0
            x2 = x0
            y0 = ynew
            x0 = xnew
        else:
            y0 = y1
            x0 = x1
            y1 = ynew
            x1 = xnew
        dx20 = abs(x2-x0)
        dx21 = abs(x2-x1)
        # Test for convergence
        if dx20 < max(small,epsilon*abs(x1)):
            if xy:
                return x1,y1
            else:
                return x1
        # Test for a badly scaled initial bracketing interval that may have
        # caused xnew and x1 to cross one another
        elif dx21/dx20 < 0.5:
            temp = y2
            y2 = y0
            y0 = temp
            temp = x2
            x2 = x0
            x0 = temp
                
    raise Exception('Failed to converge to a minimum!')
    
