Building the application - short introduction
---------------------------------------------
(Note that the former name of program was 'USBhubConnect'.)


0. Prerequisites:
   - QT with all developement files - min. version is: 4.6
     (usually some packages called "qt4-dev" or similar need to be installed.)
   - C++ compiler with libc development files
   - usb-vhci (kernel modul and C++-library) from: http://sourceforge.net/projects/usb-vhci/

1. Unpack the tar.gz archive (filename is only an example):
		tar -xzvf wusbethernet-XX.tar.gz
   now a directory "wusbethernet" with several files (including this one) should be created.

2. Run qmake to create a makefile
		qmake -recursive CONFIG+=debug_and_release USBhubConnect.pro

3. Run make to build software:
		make
   If everything was o.k., then application is build in current directory: 'USBhubConnect'

4. No "installation" is required (at present) - you can copy the file 'USBhubConnect' to
   /usr/local/bin/ if you like...


(sko, 2011-02-23)
$Id$
