import matplotlib.pyplot as plt
import matplotlib as mpl


AX1_LABEL = 'LPLOT_AX1'
AX2_LABEL = 'LPLOT_AX2'

def set_defaults(font_size=12., legend_font_size=12., figure_size=(4., 3.), 
                 screen_dpi = 96., export_dpi=300.):
    """Applies plot style defaults to rcParams
"""
    plt.rcParams['font.size'] = font_size
    plt.rcParams['legend.fontsize'] = legend_font_size
    plt.rcParams['figure.dpi'] = screen_dpi
    plt.rcParams['savefig.dpi'] = export_dpi
    plt.rcParams['figure.figsize'] = figure_size
    

def get_ax(fig):
    """Retrieve the primary and secondary axes from an LPLOT figure
    ax1, ax2 = get_ax(fig)
        OR
    ax1, ax2 = get_ax(obj)

Can be called with a figure handle or the handle of any matplotlib object in 
that figure with a get_figure() method.  get_ax() searches all the figure's 
child axes for axes with labels 'LPLOT_AX1' and 'LPLOT_AX2'
"""
    if isinstance(fig, mpl.figure.Figure):
        ax1,ax2 = None,None
        for this in fig.get_axes():
            if this.get_label() == AX1_LABEL:
                ax1 = this
            elif this.get_label() == AX2_LABEL:
                ax2 = this
        return ax1, ax2
    elif hasattr(fig, 'get_figure'):
        return get_ax(fig.get_figure())
    else:
        raise Exception('Unhandled matplotlib handle type: %s'%repr(type(fig)))


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
    ax = f.add_axes([padding_fraction, padding_fraction, size_fraction, size_fraction], label='ruler')

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


def init_fig(xlabel,ylabel,label_size=12.,figure_size=(8.,6.)):
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
    ax = f.add_axes([left,bottom,w,h],label=AX1_LABEL)
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
    ax2 = f.add_axes(p,label=AX2_LABEL)
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
    
    
def scale_xxyy(obj, xscale=1., xoffset=0., yscale=1., yoffset=0.):
    """SCALE_XXYY  apply a linear scaling factor and offset to an xxyy plot
    scale_xxyy(obj, xscale=1., xoffset=0., yscale=1., yoffset=0.)

SCALE_XXYY can be called with a handle to any object in a figure prepared by
init_xxyy().  It calls get_ax() to identify the primary and secondary axes.
Once plotting is complete and the ax1 y and x limits have been set (either 
automatically or manually), scale_xxyy() can be called to set the second axes 
to match those limits.

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
    ax1,ax2 = get_ax(obj)
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

If ax1 is not an axis, adjust ax will call get_ax() to attempt to find the
axes from the containing plot.
"""
    if not isinstance(ax1,mpl.axes.Axes):
        ax1,ax2 = get_ax(ax1)
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
        
        
def floating_legend(fig, loc, fmt, loc_edge='lt', vpadding_inches=.05, hpadding_inches=.1, markerw_inches=.2, label_size=None):
    """FLOATING_LEGEND generate a new axes that serves as a floating legend
    fmt.append([{'lc':'k', 'marker':'o', 'mfc':'w', 'mec':'k'}, 'Data set 1' ])
    fmt.append([{'lc':'k', 'marker':'D', 'mfc':'w', 'mec':'k'}, 
            {'lc':'k', 'marker':'s', 'mfc':'w', 'mec':'k'}, 'Sets 2 and 3'])
"""

    if label_size is None:
        label_size = plt.rcParams['legend.fontsize']
    dpi = fig.get_dpi()
    char_height_inches = label_size / dpi
    char_width_inches = 0.75 * char_height_inches

    # Calculate column widths
    # and check for conformity of the fmt lists
    mcol_width_inches = 0.
    lcol_width_inches = 0.

    for row in fmt:
        if not isinstance(row[-1],str):
            raise Exception('Found non-string label in legend format: %s'%repr(row[-1]))
        # estimate the text width
        lcol_width_inches = max(
            len(row[-1])*char_width_inches, 
            lcol_width_inches)
        # estimate the marker width
        mcol_width_inches = max(
            (len(row)-1)*(markerw_inches + hpadding_inches)-hpadding_inches,
            mcol_width_inches)

    # Calculate the total width
    width_inches = mcol_width_inches + lcol_width_inches + 3*hpadding_inches
    # Calculate the total height
    height_inches = len(fmt) * (char_height_inches + vpadding_inches) + vpadding_inches
    
    # Get the figure size
    fig_width_inches, fig_height_inches = fig.get_size_inches()

    # Fractional location in figure coordinates
    width = width_inches / fig_width_inches
    height = height_inches / fig_height_inches
    # Fractional dimensions in axes coordinates
    char_height = char_height_inches / height_inches
    char_width = char_width_inches / width_inches
    lcol_width = lcol_width_inches / width_inches
    mcol_width = mcol_width_inches / width_inches
    markerw = markerw_inches / width_inches
    vpadding = vpadding_inches / height_inches
    hpadding = hpadding_inches / width_inches
    
    # Generate the axis position
    if loc_edge[1] == 't':
        bottom = loc[1] - height
    elif loc_edge[1] == 'c':
        bottom = loc[1] - height/2.
    elif loc_edge[1] == 'b':
        bottom = loc[1]
    else:
        raise Exception('loc_edge must be a two-character string "[t|c|b][l|c|r]"')

    if loc_edge[0] == 'l':
        left = loc[0]
    elif loc_edge[0] == 'c':
        left = loc[0] - width/2.
    elif loc_edge[0] == 'r':
        left = loc[0] - width
    else:
        raise Exception('loc_edge must be a two-character string "[t|c|b][l|c|r]"')

    ax = fig.add_axes([left,bottom,width,height], label='LPLOT_LEGEND')
    ax.set_xticks([])
    ax.set_yticks([])
    ax.set_xlim([0,1])
    ax.set_ylim([0,1])
    
    # Start filling in row-by-row
    y = 1. - vpadding - char_height/2.
    for row in fmt:
        yy = [y,y,y]    # Marker y-array
        # Bias the markers to the right
        nmarker = len(row)-1
        # Set an interval that includes the marker width and padding
        dx = markerw + hpadding
        
        x = 2*hpadding+mcol_width - (nmarker)*dx + markerw*0.5
        # Loop through all the markers indicated on the row
        for index in range(nmarker):
            xx = [x-markerw/2., x, x+markerw/2.]
            ax.add_line(mpl.lines.Line2D(xx,yy,markevery=(1,2),**row[index]))
            x += dx
        # Add the label
        x = hpadding*2 + mcol_width
        ax.text(x,y,row[-1],verticalalignment='center')
        y -= vpadding + char_height
        
    fig.canvas.draw()