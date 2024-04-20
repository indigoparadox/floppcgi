# ![floppcgi icon](floppysrv.png) floppcgi

A tiny menu system for pushing floppy images to a Gotek floppy emulator via USB OTG.

Designed for the Raspberry Pi Zero and [FlashFloppy Gotek firmware](https://github.com/keirf/flashfloppy).

This project is written in C for maximum efficiency (and also because we just kinda wanted to?), as the Raspberry Pi Zero is not that powerful, but its USB-OTG functionality is essential for this purpose.

More information may be found at [https://indigoparadox.zone/projects/gotek.html#Floppy-Server](https://indigoparadox.zone/projects/gotek.html#Floppy-Server).

## Installation

To install, clone this repo to (/opt/floppcgi is recommended) and run setup.sh.

You MUST cd into the directory before running setup.sh. Do not try to run it from one level up!

WARNING: Do *not* use a USB cable with power-positive (+5V) connected! This may cause damage and will likely not work! Please see the website for more details!

