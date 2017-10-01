import matplotlib.pyplot as plt




def set_defaults(font_size=12., figure_size=(4., 3.), 
                 screen_dpi = 112., export_dpi=300.):
    """Applies plot style defaults to rcParams
"""
    plt.rcParams['font.size'] = font_size
    plt.rcParams['figure.dpi'] = screen_dpi
    plt.rcParams['savefig.dpi'] = export_dpi
    plt.rcParams['figure.figsize'] = figure_size
    


def make_ruler(size=1., units='in'):
    """Produce a figure with axis ticks like a ruler
    ax = make_ruler(size=1., units='in')
    
This is usfeul for testing the scaling produced by the 'screen_dpi' option
of the set_defaults() function.

The units can be 'cm' or 'in'.

The size indicates the size of the ruler in the units specified.
"""
    padding_in = .5
    if units=='in':
        divs = 4
        size_in = size
    elif units=='cm':
        divs = 10
        size_in = size / 2.54
    figure_in = size_in + 2*padding_in
    f = plt.figure(figsize = 2*(figure_in,))
    
    padding_fraction = padding_in / figure_in
    size_fraction = size_in / figure_in
    ax = f.add_axes([padding_fraction, padding_fraction, size_fraction, size_fraction], label='primary')

    ax.set_xlim([0,size])
    ax.set_ylim([0,size])

    ticks = [this/float(divs) for this in range(divs*int(size)+1)]
    ticklabels = len(ticks)*['']
    for this in range(int(size)+1):
        ticklabels[this*divs] = '%d'%this
    ax.set_xticks(ticks)
    ax.set_xticklabels(ticklabels)
    ax.set_yticks(ticks)
    ax.set_yticklabels(ticklabels)
    
    return ax


def init_fig(xlabel,ylabel,size=None,label_size=12.,figure_size=(8.,6.)):
    """set up a figure with a single axes
    ax = init_fig(xlabel,ylabel,size=None)
Returns the axis for plotting    
"""
    dpi = plt.rcParams['figure.dpi']
    font_size = plt.rcParams['font.size']
    figure_height = figure_size[1] * dpi
    figure_width = figure_size[0] * dpi
    padding = 0.05
    
    # Use 2x font size for the y tick labels
    # double the sum to accommodate the padding used by the axes
    left = 2*(label_size + 3.*font_size)/figure_width
    w = 1.-padding-left
    bottom = 2*(label_size + font_size)/figure_height
    h = 1.-padding-bottom

    f = plt.figure(figsize=figure_size)
    ax = f.add_axes([left,bottom,w,h],label='primary')
    ax.set_xlabel(xlabel,fontsize=label_size)
    ax.set_ylabel(ylabel,fontsize=label_size)
    ax.grid('on')
    return ax


def init_xxyy(xlabel,ylabel,x2label=None,y2label=None,label_size=12.,figure_size=(8.,6.)):
    """set up a figure with two axes overlayed
    ax1,ax2 = init_xxyy(xlabel,ylabel,x2label=None,y2label=None)

Used for making dual x or y axis plots, ax1 contains all data and the primary
x and y axes.  ax2 is used solely to create top (right) x (y) ticks with a 
different scale.

If x2label and/or y2label are specified, then their axes on top and right 
respectively will be visible.

Objects should be added to the figure by using ax1 commands
>>> ax1.plot(...)
>>> ax1.legend(...)

Once plotting is complete, ax2  needs to be scaled appropriately.  Use the 
scale_xxyy() function.
"""
    ax1 = init_fig(xlabel,ylabel,label_size=label_size, figure_size=figure_size)
    f = ax1.get_figure()
    p = ax1.get_position()
    ax2 = f.add_axes(p,label='secondary')
    ax2.set_axis_bgcolor('none')
    axis = ax2.get_xaxis()
    axis.set_ticks_position('top')
    axis.set_label_position('top')
    
    dpi = plt.rcParams['figure.dpi']
    font_size = plt.rcParams['font.size']
    figure_height = figure_size[1] * dpi
    figure_width = figure_size[0] * dpi
    padding = .05

    left = 2.*(label_size + 3.*font_size)/figure_width
    bottom = 2.*(label_size + font_size)/figure_height

    if x2label==None:
        ax2.set_xticks([])
        top = 1.-padding
    else:
        ax2.set_xlabel(x2label)
        top=1.-2*(label_size + font_size)/figure_height
    axis = ax2.get_yaxis()
    axis.set_ticks_position('right')
    axis.set_label_position('right')
    if y2label==None:
        ax2.set_yticks([])
        right = 1.-padding
    else:
        ax2.set_ylabel(y2label)
        right = 1.-2*(label_size + 2*font_size)/figure_width
    p.x0 = left
    p.x1 = right
    p.y0 = bottom
    p.y1 = top
    ax1.set_position(p)
    ax2.set_position(p)
    return ax1,ax2
    
    
def scale_xxyy(ax1, ax2, xscale=1., xoffset=0., yscale=1., yoffset=0.):
    """SCALE_XXYY  apply a linear scaling factor and offset to an xxyy plot
    scale_xxyy(ax1, ax2, xscale=1., xoffset=0., yscale=1., yoffset=0.)
    
ax1 and ax2 are the axes returned by the init_xxyy() function.  Once plotting
is complete and the ax1 y and x limits have been set (either automatically or
manually), scale_xxyy() can be called to set the second axes to match those
limits.

xscale and xoffset define the relationship between the two x axes:
   x2 = xscale * x1 + xoffset
and the same is true for the y axes

For example, these lines produce complimentary Farenheit and Celsius 
temperature axes:
>>> ax1, ax2 = init_xxyy(xlabel='Temperature (C)', ylabel='Resistance (Ohms)', 
                         xlabel2='Temperature (F)')
... do some plotting ...
>>> scale_xxyy(ax1,ax2,xscale=1.8,xoffset=32.)
"""
    ax2.set_xlim([xscale * xx + xoffset for xx in ax1.get_xlim()])
    ax2.set_ylim([yscale * yy + yoffset for yy in ax1.get_ylim()])
    
    
def adjust_ax(ax1, ax2=None, left=None, right=None, bottom=None, top=None):
    """Tweak the bounds on axes in a figure made by lplot
    adjust_ax(ax1, ax2=None, left=None, right=None, bottom=None, top=None)

Any of the left, right, top, bottom keywords specified will interpreted as the
fractional location (between 0 and 1) for that edge of the figure in the plot.
    
If ax1 and ax2 are both specified, they are presumed to be redundant axes for
an xxyy plot.  After ax1 is adjusted, ax2 will be forced to the same shape.
This can be useful if you do something that gets ax1 and ax2 out of sync - just
call 
>>> adjust_ax(ax1=ax1, ax2=ax2)
No resizing will happen in ax1, but ax2 will now share ax1's shape.
"""
    p = ax1.get_position()
    if left:
        p.x0 = left
    if right:
        p.x1 = right
    if top:
        p.y1 = top
    if bottom:
        p.y0 = bottom
    ax1.set_position(p)
    if ax2:
        ax2.set_position(p)
        
        
#def floating_legend():
# Coming soon...