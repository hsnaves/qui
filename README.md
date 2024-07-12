# QUI

## Synopsis

An in-progress virtual stack machine.

## Getting started

### Prerequisites

QUI requires GCC, MAKE and SDL2 to be installed on the system.

### Building

To build QUI, simply type the following command line on the root directory of the QUI source code repository (assuming all dependencies are installed):

```sh
$ make
```

This will compile QUI with no debugging information and with compiler optimizations enabled. To enable debugging information and disable optimizations, the user can specify `DEBUG=1` in the command line above, such as:

```sh
$ DEBUG=1 OPTIMIZE=0 make
```

### Installing

In order to install QUI, type:

```sh
$ make install
```

To install it using in a separate location other the default `${HOME}/.local/`, type:

```sh
$ PREFIX=/usr/local/ make install
```

