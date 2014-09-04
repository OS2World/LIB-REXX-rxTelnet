#ifndef NVT_H
#define NVT_H

 #define  DEFAULTPORT    23
 #define  DEFAULTMODEL   2
 #define  MAXLINESIZE    256
 #define  RECVTIMEOUT    1000

 typedef  unsigned long  HNVT;

 HNVT  nvt3270( char *hostname, int port, int model );

 HNVT  nvtopen( char *hostname, int port );
 int   nvtclose( HNVT hnvt );

 int   nvtgets( HNVT hnvt, char *buf, int bufsize );
 int   nvtputs( HNVT hnvt, char *buf, int bufsize );
 int   nvtpeek( HNVT hnvt, int timeout );


 /* telnet out-of-band commands */

 #define   _GA      "GA"
 #define   _AYT     "AYT"
 #define   _AO      "AO"
 #define   _IP      "IP"
 #define   _BREAK   "BREAK"
 #define   _NOP     "NOP"
 #define   _EOR     "EOR"
 #define   _ABORT   "ABORT"
 #define   _SUSP    "SUSP"
 #define   _EOF     "EOF"

 int   nvtcommand( HNVT hnvt, char *command );


 typedef struct                        /* nvt link information       */
         {
           unsigned long   addr;       /* address of connection host */
           int             port;       /* port used for connection   */
           int             socket;     /* socket used for connection */
         }
           LNVT;

 int   nvtquery( HNVT hnvt, LNVT *link );

#endif /* NVT_H */
