A Rexx Telnet API
by Ben Ravago <bjr@hq.sequel.net>


NEW
"""

1. miscellaneous bug fixes
2. compiled with "IBM C and C++ Compilers, Version 3.6.5"
3. simple TN3270 support



DESCRIPTION
"""""""""""

This package provides a simple Network Virtual Terminal (NVT)
as defined by the "Telnet Protocol Specification (RFC 854)".
It provides both C and Rexx callable functions in a .dll.



CONTENTS
""""""""

1. README    - this file
2. NVT       - makefile
3. NVT.C     - contains Network Virtual Terminal functions
4. NVT.H     - NVT functions
5. RNVT.C    - contains Rexx interface to NVT functions
6. TNVT.C    - example telnet client showing use of nvt..() functions
7. XNVT.CMD  - example telnet client showing use of rexx telnet api
8. ASCII.H   - some ASCII definitions
9. TELNET.H  - TELNET definitions; copied from BSD system




C API
"""""

The C api provides basic functions for accessing a telnet 'stream'.
It is patterned from the standard C fopen/fclose/fgets/fputs functions.


1. HNVT nvtopen( char *hostname, int port );

       nvtopen() starts a telnet session.  'hostname' specifies
       the name, alias, or internet address of a remote host.
       'port' can be 0, in which case the default telnet port is used.

       nvtopen() returns a handle to the telnet session if successful,
       otherwise it returns a -1.  Multiple telnet sessions can be open
       at the same time.


2. int nvtclose( HNVT hnvt );

       nvtclose() stops a telnet session identified by the nvt handle.


3. int nvtgets( HNVT hnvt, char *buf, int bufsize );

       nvtgets() reads a line from the nvt's input queue and copies it
       into 'buf' to a maximum of 'bufsize'.  If no data is in the
       input queue, nvtgets() blocks until a line is available.

       A line consists of all characters up to the new-line sequence
       (generally CRLF).  The new line sequence is replaced by a null
       character (\0) in 'buf'.

       nvtgets() returns the length of the string copied to 'buf'.

       If the string returned by nvtgets() begins with an EOT (\4)
       then this indicates that the nvtgets() has timed out.

       If the string returned by nvtgets() begins with an ACK (\6)
       then the rest of the string is an echo of the preceding nvtputs().


4. int nvtputs( HNVT hnvt, char *buf, int bufsize );


       nvtputs() writes a line to the nvt's output queue.
       The line is copied from 'buf' for a length of 'bufsize'.
       A new-line sequence is appended to the string.

       nvtputs() returns the length of the string transmitted from 'buf'.


5. int nvtpeek( HNVT hnvt, int timeout );

       nvtpeek() waits for a line to appear in the nvt's input queue.
       If there is a line ready, nvtpeek() returns immediately.
       If the input queue doesn't have a line ready, nvtpeek() waits
       until the queue has a line up to 'timeout' milliseconds.

       nvtpeek() returns 1 if a line is ready or 0 if a timeout occurred
       and no data is available.


6. int nvtcommand( HNVT hnvt, char *command );

       nvtcommand() sends a telnet out-of-band command (such as 'BREAK').

       nvtcommand() returns a non-zero if it was able to send the command;
       else it returns a zero.


7. int nvtquery( HNVT hnvt, LNVT *link );

       nvtquery() fills link information from the nvt's session.


8. HNVT nvt3270( char *hostname, int port, int model );

       nvt3270() is similar in function to nvtopen() except it starts
       an EBCDIC TN3270 session rather than an ASCII Telnet session.
       The model parameter should be either 2, 3, 4, or 5 corresponding
       to the emulated 3270 model number.

       All the other functions should work the same as after an nvtopen()
       ASCII/EBCDIC conversion is performed internally;  both nvtgets()
       and nvtputs() still work with ASCII strings.



REXX API
""""""""

The Rexx api builds on the C nvt functions above.


1. nvt = TELNET(ÄÄhostnameÄÂÄÄÄÄÄÄÄÂÄÄ)
                           ÀÄ,portÄÙ

   Telnet() starts a telnet session.  If successful, it returns
   an NVT handle.  If not, it returns a null string.

   Multiple telnet sessions can be simultaneously active.


2. string = TGET(ÄÄnvtÄÄÂÄÄÄÄÄÄÄÄÄÄÂÄÄ)
                        ÀÄ,timeoutÄÙ

   Tget() retrieves a string from the nvt's input queue.
   'timeout' can be specified to override the default timeout
   (which is 1000 milliseconds).


3. result = TPUT(ÄÄnvtÄÄ,ÄÄstringÄÄÄ)

   Tput() inserts a string into the nvt's output queue.


4. result = TCTL(ÄÄnvtÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄ)
                        ÀÄ,ÄÂÄ'AO'ÄÄÄÄ´
                            ÃÄ'AYT'ÄÄÄ´
                            ÃÄ'BRK'ÄÄÄ´
                            ÃÄ'IP'ÄÄÄÄ´
                            ÃÄ'SYNCH'Ä´
                            ÀÄÄetc.ÄÄÄÙ


   Tctl() returns link information about the nvt session or,
   if the second argument is specified, sends an out-of-band
   telnet command.


5. result = TQUIT(ÄÄnvtÄÄ)

   Tquit() closes the specified nvt session.


6. nvt = TN3270(ÄÄhostnameÄÂÄÄÄÄÄÄÄÂÄÄÂÄÄÄÄÄÄÄÄÂÄÄ)
                           ÀÄ,portÄÙ  ÀÄ,modelÄÙ

   Tn3270() starts a 3270 telnet session; similar to telnet().
   Model defaults to 2 (can also be 3, 4, or 5).



ACKNOWLEDGEMENTS
""""""""""""""""

The following files were copied from the FreeBSD 2.1 distribution.

These files are part of the TN3270 v4.1.1 component.

  /usr/src/usr.bin/tn3270/api/asc_ebc.c
  /usr/src/usr.bin/tn3270/api/asc_ebc.h
  /usr/src/usr.bin/tn3270/ctlr/hostctlr.h
  /usr/src/usr.bin/tn3270/ctlr/outbound.c              (excerpted)
  /usr/src/usr.bin/tn3270/ctlr/screen.h                (excerpted)
  /usr/src/usr.bin/tn3270/distribution/arpa/telnet.h

  The outbound.c excerpt is included in asc_ebc.c and
  the screen.h excerpt is included in asc_ebc.h.

A number of users of RxTelnet suggested comments and bug fixes.
My thanks to them.

Unfortunately, I've lost the list of their names and credits.
My apologies as well.



DISCLAIMER
""""""""""

 My part of this software is freeware.  It's not intended for
 commercial purposes.  It has been tested in a working (production)
 environment but is not guaranteed to work anywhere else. ;-)

 If you have any comments or suggestions feel free to contact me
 at bjr@hq.sequel.net

