ebusd - eBUS daemon
===================

ebusd is a daemon for handling communication with eBUS devices connected to a
2-wire bus system ("energy bus" used by numerous heating systems).

[![Gitter](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/john30/ebusd?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge)


Features
--------

The main features of the daemon are:

 * actively send messages to and receive answers from the eBUS
 * passively listen to messages sent on the eBUS
 * regularly poll for messages
 * scan for bus participants
 * cache all data
 * grab unknown messages
 * log messages and problems to a log file
 * dump sent/received bytes to the log file
 * dump received bytes to binary files for later playback/analysis
 * listen for client connections on a dedicated TCP port


Installation
------------

Either pick the latest release package suitable for your system
(see https://github.com/john30/ebusd/releases/latest) or build it yourself.

Building ebusd from the source requires the following packages and/or features:
 * autoconf (>=2.63)
 * automake (>=1.11)
 * g++
 * make
 * kernel with pselect or ppoll support
 * glibc with argp support or argp-standalone

To start the build process, run these commands:  
> ./autogen.sh  
> make install  


Documentation
-------------

Usage instructions and further information can be found here:
> https://github.com/john30/ebusd/wiki


Configuration
-------------

The most important part of each ebusd installation is the message
configuration. By default, only rudimentary messages are interpreted.
Check the Wiki and/or the configuration repository:
> https://github.com/john30/ebusd-configuration


Contact
-------
For bugs and missing features use github issue system.

The author can be contacted at ebusd@ebusd.eu .
