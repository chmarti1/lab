[back](documentation.md)

Version 4.00<br>
September 2019<br>
Chris Martin<br>

## <a name="compiling"></a> Compiling

The LConfig system is written in C using LabJack's LabJackM interface, and is written for a POSIX environment (Linux/Unix).  To obtain the latest version of LConfig

```bash
$ git clone http://githuh.com/chmarti1/lconfig
```

If you already have a working copy of LConfig, you can upgrade it to the latest version using
```bash
$ cd /path/to/lconfig
$ git pull origin master
```

To compile in a Debian-based system, first be certain that the GNU C Compiler is installed.

```bash
$ sudo apt update
$ sudo apt install gcc build-essential
```

Next, be certain that the [LabJackM](https://labjack.com/support/software/installers/ljm) driver is installed.  Download the appropriate release for your system and follow the instructions.

To compile the built-in `lcrun` and `lcburst` binaries and automatically place them on your system,
```bash
$ cd /path/to/lconfig
$ sudo make install
```
Alternately, this can be done step-by-step
```bash
$ make clean
$ make lconfig.o
$ make lctools.o
$ make lcburst.bin
$ make lcrun.bin
$ sudo make install
```

### Writing your own binaries

User applications that use the LConfig system should include the `lconfig.h` header.  There are also tools for interacting with data and for building simple terminal interfaces in the `lctools.h` header.  Somewhere at the top of your c-file, the line below should appear.  

```C
#include "lconfig.h"
#include "lctools.h"
```

In the current LConfig design, the configuration system is NOT installed as a system library.  You should simply copy `lconfig.h`, `lconfig.c`, `lctools.h`, and `lctools.c` into your project.  This design may change in the future, but for now it is quite functional.  Most of all, it means I do not need to be aware of your system's library directory structure when I design my installation.

When you are compiling your applicaiton, you will need to compile the `lconfig.o` object file (and `lctools.o` if you are using `lctools.h`.  Then, you will need to link your executable with `lconfig.o` (+ `lctools.o`), the LJM library, and the cmath library.  Take a look at the makefile for how the LConfig project does this for `lcrun` and `lcburst`.  For a user application called `mycode.c` producing an executable `mybinary`, the following compilation should work.
```
$ gcc -c lconfig.c -o lconfig.o
$ gcc -c lctools.c -o lctools.o
$ gcc -Wall -lLabJackM -lm lconfig.o lctools.o mycode.c -o mybinary
```
The `-Wall` flag is good practice for development, but can be removed once you are confident in your code.

To learn how to implement LConfig, read the docs on the [api](api.md), take a peak at the [reference](reference.md), and when in doubt, the `lconfig.h` and `lctools.h` headers are thoroughly commented to serve as their own documentation.

[top](#compiling)
