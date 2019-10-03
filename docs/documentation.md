## <a name="top"></a> Documentation

Version 4.00<br>
September 2019<br>
Chris Martin<br>


## <a name="intro"></a> Introduction
The **L**aboratory **CONFIG**uration system processes human-readable configuration files to automate the configuration of the LabJack T4 and T7, the execution of the data acquisition process, and writing data files.  The base system is contained in a single c-file and exposed through a header for inclusion in an application.  There are two generic applications that accomplish most data acquisition tasks; `drun` and `dburst`.

Detailed documentation for the [LConfig API](api.md) describes the system functions and their use.  There is also a [Reference](reference.md) for a complete list of funcitons, constants, and the available configuration directives.  Finally, the `lconfig.h` header is commented with detailed documentation and a changelog.  If these sources ever contradict one another, the header file should be interpreted as the authoritative resource.


### To get started

- [Compiling and installation](compiling.md)
- [The binaries](bin.md)
- [Writing configuration files](config.md)
- [Data files](data.md)
- [Reference](reference.md)

### Using LConfig data

- [Data files](data.md)
- [Post processing](post.md)

### Writing your own binaries

- [The LConfig API](api.md)
- The LCTools API (docs coming soon: see lctools.h for documentation)
- [Compiling](compiling.md)
- [Reference](reference.md)

