#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>

#include <memory.h>
#include <stddef.h>
#include <stdlib.h>

#define  TCPV40HDRS
#include <netdb.h>
#include <sys\socket.h>

#include "bsd\telnet.h"
#include "bsd\asc_ebc.h"
#include "bsd\hostctlr.h"

#include "nvt.h"
#include "ascii.h"
#include "ebcdic.h"


 /* structure definitions */

 #pragma pack(2)

 typedef
   struct _linkitem                     /* linked list items         */
   {
     struct _linkitem   *next;          /* next in list              */
     short               length;        /* length of item            */
     char                data[2];       /* start of item data        */
   }
     linkitem;

 typedef linkitem *linked;

 typedef
   struct _NVT                           /* NVT control area          */
   {
     PVOID            pool;              /* common storage pool       */
     TID              device;            /* receiver thread id        */
     int              sockit;            /* socket handle             */
     HEV              available;         /* storage available event   */
     HMTX             lock;              /* mutex lock semaphore      */
     HMTX             sending;           /* send() lock               */
     HEV              ready;             /* inbound data event        */
     linked           pending;           /* ready data queue          */
     int              echoing;           /* echoes pending            */

     int              m3270;             /* 3270 model                */
     int              ind;               /* operational mode          */
     int              cursor;            /* last cursor position      */
   }
     NVT;

 /* NVT.ind */

 #define KB_RESTORE   0x0001              /* 3270 input acceptable    */

 #pragma pack()


 /* function prototypes */

#define intern static

intern            int  cmdkey( char *c );
intern          char*  cmdstr( int i );

intern  unsigned long  hostaddress( char *cp );

intern            int  pop( HMTX lock, linked *list, linked *item );
intern            int  push( HMTX lock, linked *list, linked item );
intern            int  queue( HMTX lock, linked *list, linked item );

intern            int  wait( HEV event, int timeout );
intern            int  post( HEV event );

intern          void*  allocpool( void );
intern          void*  relsepool( void* pool );
intern          void*  alloc( void* pool, int bufsize );
intern          void*  relse( void* pool, void* buf, int bufsize );

intern           HNVT  nvtconnect( char *hostname, int port, int model );

intern            int  nvtenq( NVT *vt, char *buf, int bufsize );
intern            int  nvtdeq( NVT *vt, char *buf, int bufsize );

intern           void  nvterrno( NVT *vt, char *line );
intern            int  nvtline( NVT *vt, char *line, int used );

intern            int  nvtransmit( NVT *vt, char *buf, int bufsize );
intern  VOID APIENTRY  nvtprotocol( ULONG parm );

intern            int  nvtputs3270( NVT *vt, char *buf, int bufsize );
intern  VOID APIENTRY  nvtprotocol3270( ULONG parm );

#define                nvtptr(handle)   (NVT*)(handle)


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

HNVT nvt3270( char *hostname, int port, int model )
{
  if ( !model )
    model = DEFAULTMODEL;
  else
    if ( model < 2 || model > 5 )
      return ERROR_BAD_DEV_TYPE;

  return nvtconnect( hostname, port, model );
}

HNVT nvtopen( char *hostname, int port )
{
  return nvtconnect( hostname, port, 0 );
}

HNVT nvtconnect( char *hostname, int port, int model )
{
  int   rc = ERROR_NOT_ENOUGH_MEMORY;

  NVT    *vt = NULL;
  PVOID pool = allocpool();

  if ( pool )
  {
    vt = alloc( pool, sizeof(*vt) );

    if ( vt )
    {
      memset( vt, 0, sizeof(*vt) );

      vt->pool = pool;

      rc = ( DosCreateEventSem( NULL, &(vt->available), 0, FALSE ) ||
             DosCreateEventSem( NULL, &(vt->ready), 0, FALSE ) ||
             DosCreateMutexSem( NULL, &(vt->sending), 0, FALSE ) ||
             DosCreateMutexSem( NULL, &(vt->lock), 0, FALSE ) );

      if ( !rc )
      {
        vt->m3270 = model;

        rc = DosCreateThread( &(vt->device),
                              vt->m3270 ? nvtprotocol3270 : nvtprotocol,
                              (ULONG)vt,
                              CREATE_SUSPENDED |
                              STACK_SPARSE,
                              65536UL );
        if ( !rc )
        {
          unsigned long host = hostaddress( hostname );

          if ( host == 0 || host == INADDR_NONE )
            rc = 1;
          else
          {
            vt->sockit = socket( PF_INET, SOCK_STREAM, 0 );

            if ( vt->sockit == -1 )
              rc = 1;
            else
            {
              struct sockaddr_in server;

              memset( &server, 0, sizeof(server) );

              if ( !port ) port = DEFAULTPORT;

              server.sin_family      = AF_INET;
              server.sin_port        = htons(port);
              server.sin_addr.s_addr = host;

              rc = connect( vt->sockit,
                            (struct sockaddr *)&server,
                            sizeof(server) );

              if ( !rc )
              {
                vt->pending = NULL;
                vt->echoing = 0;

                rc = DosResumeThread( vt->device );
              }
            }
          }
        }
      }

      if ( rc )
      {
        nvtclose( (HNVT)vt );
        vt = NULL;
      }
    }
  }

  return (HNVT)vt;
}

int nvtclose( HNVT hnvt )
{
  int rc;
  NVT *vt = nvtptr(hnvt);

  if ( vt->device )
    rc = DosKillThread( vt->device );

  if ( vt->available )
    rc = DosCloseEventSem( vt->available );

  if ( vt->ready )
    rc = DosCloseEventSem( vt->ready );

  if ( vt->sending )
    rc = DosCloseMutexSem( vt->sending );

  if ( vt->lock )
    rc = DosCloseMutexSem( vt->lock );

  if ( vt->sockit )
  {
    rc = shutdown( vt->sockit, 2 );
    rc = soclose( vt->sockit );
  }

  if ( vt->pool )
    relsepool( vt->pool );

  return 0;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int nvtquery( HNVT hnvt, LNVT* link )
{
  if ( link )
  {
    NVT *vt = nvtptr(hnvt);

    struct sockaddr_in peer;
    int    len = sizeof(peer);

    if ( getpeername( vt->sockit, (struct sockaddr *)&peer, &len ) == 0 )
    {
      link->addr = ntohl(peer.sin_addr.s_addr);
      link->port = ntohs(peer.sin_port);
    }
    else
      link->addr = link->port = 0;

    link->socket = vt->sockit;
  }
  return sock_errno();
}

int nvtcommand( HNVT hnvt, char *command )
{
  int count = 0;
  char cmd[3];

  cmd[0] = IAC;
  cmd[1] = cmdkey( command );

  if ( cmd[1] )
    count = nvtransmit( nvtptr(hnvt), cmd, 2 );

  return count;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int nvtpeek( HNVT hnvt, int timeout )
{
  NVT *vt = nvtptr(hnvt);

  wait( vt->ready, timeout );

  return vt->pending ? 1 : 0;
}

int nvtgets( HNVT hnvt, char *buf, int bufsize )
{
  int count = nvtdeq( nvtptr(hnvt), buf, bufsize-1 );

  if ( count >= 0 )
    buf[count] = '\0';

  return count;
}

int nvtputs( HNVT hnvt, char *buf, int bufsize )
{
  if ( bufsize > MAXLINESIZE )
    return ERROR_BAD_LENGTH;
  else
  {
    NVT *vt = nvtptr(hnvt);

    if ( bufsize == 0 && vt->m3270 )
      return 0;
    else
    {
      if ( vt->echoing )
        vt->echoing += 1;

      if ( vt->m3270 )
        return nvtputs3270( vt, buf, bufsize );
      else
        if ( bufsize > 0 )
          return nvtransmit( vt, buf, -bufsize );
        else
          return nvtransmit( vt, "\r\n", 2 ) == 2 ? 0 : -1;
    }
  }
}

                               /*  1 ,  2 , 1 , 2 , .n. , 1 , 1  */
 #define   DSCONTROLSIZE   8   /* AID,CPOS,SBA,POS, ... ,IAC,EOR */

int nvtputs3270( NVT *vt, char *buf, int bufsize )
{
  int  i;
  char *p, ds[ MAXLINESIZE + DSCONTROLSIZE ];

  p = ds;

  if ( !buf )
  {
    bufsize = 0; buf = ds; buf[0] = 0;
  }

  if ( buf[0] == ESC )
  {
    *p++ = buf[1]; buf += 2; bufsize -= 2;
  }
  else
  {
    *p++ = AID_ENTER;
  }

  *p++ = BufferTo3270_0(vt->cursor);
  *p++ = BufferTo3270_1(vt->cursor);
  *p++ = ORDER_SBA;
  *p++ = ds[1];
  *p++ = ds[2];

  for ( i = bufsize; i; i-- ) *p++ = asc_ebc[ 0x7f & (*buf++) ];

  *p++ = IAC;
  *p++ = EOR;

  i = nvtransmit( vt, ds, bufsize + DSCONTROLSIZE );

  return ( i < DSCONTROLSIZE ) ? i : i - DSCONTROLSIZE;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int nvtenq( NVT *vt, char *buf, int bufsize )
{
  linked i = NULL;

  do { i = alloc( vt->pool, bufsize + offsetof(linkitem,data) ); }
  while ( !i && 0 == wait( vt->available, -1 ) );

  if ( i )
  {
    if ( bufsize )
      memcpy( i->data, buf, bufsize );

    i->length = bufsize;

    queue( vt->lock, &(vt->pending), i );

    post( vt->ready );
  }
  else
  {
    bufsize = -bufsize;
  }

  return bufsize;
}

int nvtdeq( NVT *vt, char *buf, int bufsize )
{
  linked i = NULL;

  do { pop( vt->lock, &(vt->pending), &i ); }
  while ( !i && 0 == wait( vt->ready, -1 ) );

  if ( !i )
  {
    buf[0] = '\0';
    bufsize = 0;
  }
  else
  {
    if ( i->length > bufsize )
    {
      buf[0] = '\0';
      bufsize = -(i->length);

      push( vt->lock, &(vt->pending), i );
    }
    else
    {
      if ( i->length )
      {
        memcpy( buf, i->data, i->length );
        bufsize = i->length;
      }
      else
      {
        buf[0] = '\r'; buf[1] = '\0';
        bufsize = 1;
      }

      relse( vt->pool, i, i->length + offsetof(linkitem,data) );
      post( vt->available );
    }
  }

  return bufsize;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int nvtransmit( NVT *vt, char *buf, int bufsize )
{
  APIRET rc = 0;
  int count = 0;
  int used  = 0;
  int crlf  = 0;

  if ( bufsize < 0 )
  {
    crlf = 1;
    bufsize = -bufsize;
  }

  rc = DosRequestMutexSem( vt->sending, SEM_INDEFINITE_WAIT );
  if ( !rc )
  {
    do
    {
      count    = send( vt->sockit, buf, bufsize, 0 );
      bufsize -= count;
      buf     += count;
      used    += count;
    }
    while ( count > 0 && bufsize );

    if ( crlf )
      count = send( vt->sockit, "\r\n", 2, 0 );

    rc = DosReleaseMutexSem( vt->sending );
  }

  return used;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

intern int copy( char *to, char *from )
{
  int count = 0;
  while ( *from ) { *to++ = *from++; count++; }
  return count;
}

int nvtline( NVT *vt, char *line, int used )
{
  if ( vt->echoing > 1 )
  {
    vt->echoing -= 1;

    line -= 1;
    used += 1;

    *line = ACK;
  }

  if ( used )
    used = nvtenq( vt, line, used );

  return used;
}

void nvterrno( NVT *vt, char *line )
{
  line[0] = ENQ;
  line[1] = 'E';
  _itoa( sock_errno(), line+2, 10 );

  nvtline( vt, line, strlen(line) );
}


#define WIDTH(z,y) ((unsigned long)((z))-(unsigned long)((y)))

VOID APIENTRY nvtprotocol( ULONG parm )
{
  NVT *vt = nvtptr(parm);

  char buf[MAXLINESIZE];
  char *next;
  char ch;

  long timeout = RECVTIMEOUT;
  int  socks[1];

  char will[4]    = { IAC, WILL, 0, 0 };
  char willnot[4] = { IAC, WONT, 0, 0 };
  char doit[4]    = { IAC, DO, 0, 0 };
  char donot[4]   = { IAC, DONT, 0, 0 };

  char text[MAXLINESIZE+1];

  char *line    = text+1;
  int  count    = 0;
  int  col      = 0;
  int  mode     = TEXT;
  int  maxcol   = sizeof(text)-1;


  for(;;)
  {
    if ( count < 1 )
    {
      socks[0] = vt->sockit;

      count = select( socks, 1, 0, 0, timeout );

      if ( count == 0 )
      {
        timeout = -1;

        if ( col ) buf[count++] = LF;          /* flush pending line */

        buf[count++] = EOT; buf[count++] = LF; /* simulate EOT line  */
      }
      else
      if ( count > 0 )
      {
        timeout = RECVTIMEOUT;

        count = recv( vt->sockit, buf, sizeof(buf)-1, 0 );

        /* if count < 0 then socket error  */
        /* if count = 0 then socket closed */

        if ( count <= 0 )
          break; /* exit for(;;) */
      }

      next = buf;
    }

    /* begin telnet stream state machine */

    ch = *next;

    switch ( mode ) /* ANSI nvt state machine */
    {
    case TEXT: switch ( ch )
               {
               case CR:
               case IAC:
                        mode = ch;
                        goto NEXT;

               case LF: goto EOL;

               case EOT: goto USE;
               }
               if ( ch < 0x80 ) /* _isascii(ch) */
                 goto USE;
               else
                 goto NEXT;

    case CR: switch( ch )
             {
             case NUL:
             case LF:
                     mode = TEXT;
                     goto EOL;
             }
             mode = TEXT;
             goto NEXT;

    case IAC: switch( ch )
              {
              case WONT:
              case WILL:
              case DONT:
              case DO:
                      mode = ch;
                      goto NEXT;

              case SB: mode = ch;
                       goto NEXT;

              case EC: if ( col ) col--;
                       mode = TEXT;
                       goto NEXT;

              case EL: mode = TEXT;
                       goto NL;

                         /* generate a command message */
              case ABORT:
              case AO:
              case AYT:
              case BREAK:
              case EOR:
              case xEOF:
              case GA:
              case IP:
              case NOP:
              case SUSP:
                        nvtline( vt, line, col );

                        line[0] = ENQ;
                        col = 1 + copy( line+1, cmdstr(ch) );
                        mode = TEXT;
                        goto EOL;

              case DM: mode = TEXT;
                       goto NL;
              }
              mode = TEXT;
              goto NEXT;

    case DONT:
    case DO:
            willnot[2] = ch;
            nvtransmit( vt, willnot, 3 );
            mode = TEXT;
            goto NEXT;

    case WILL: switch( ch )
               {
               case TELOPT_ECHO: vt->echoing = 1;
               }
               mode = TEXT;
               goto NEXT;

    case WONT: switch( ch )
               {
               case TELOPT_ECHO: vt->echoing = 0;
               }
               mode = TEXT;
               goto NEXT;

    case SB: switch( ch )
             {
             case IAC: mode = SBSE;
                       goto NEXT;
             }
             goto NEXT;

    case SBSE: switch( ch )
               {
               case SE: mode = TEXT;
                        goto NEXT;
               }
               mode = SB;
               goto NEXT;

    default: goto NEXT;

    } /* switch(mode) */

    /* common nvt state machine end-points */

    USE:
          line[col++] = ch;
          if ( col < maxcol ) goto NEXT;
    EOL:
          nvtline( vt, line, col );
    NL:
          col = 0;
    NEXT:
          next++;
          count--;

    /* end telnet stream state machine */

  } /* for(;;) */

  if ( col )
    nvtline( vt, line, col );

  if ( count < 0 ) nvterrno( vt, line );  /* socket error */

  DosPostEventSem( vt->ready );
  DosCloseEventSem( vt->ready );

  DosExit(0,0);
}

#define ROW(z)     (((z))/maxcol)
#define COL(z)     (((z))%maxcol)

VOID APIENTRY nvtprotocol3270( ULONG parm )
{
  NVT *vt = nvtptr(parm);

  char buf[MAXLINESIZE];
  char *next;
  char ch;

  long timeout = RECVTIMEOUT;
  int  socks[1];

  char will[4]    = { IAC, WILL, 0, 0 };
  char willnot[4] = { IAC, WONT, 0, 0 };
  char doit[4]    = { IAC, DO, 0, 0 };
  char donot[4]   = { IAC, DONT, 0, 0 };

  char text[MAXLINESIZE+1];

  char *line    = text+1;
  int  count    = 0;
  int  col      = 0;
  int  mode     = BDS;
  char nonblank = 0;

  int  maxcol, maxrow, maxloc;
  int  loc, to, from;
  int  pairs, sflen;
  char *reply;

  char cleared[5] = { AID_NONE, 0x40, 0x40, IAC, EOR };

  char t3270[16]  = { IAC, SB, TELOPT_TTYPE, TELQUAL_IS,
                      'I','B','M','-','3','2','7','8','-','?',
                      IAC, SE };

  t3270[13] = vt->m3270 | '0';

  switch ( vt->m3270 )
  {
  case 2: maxrow = 24; maxcol = 80;  break;
  case 3: maxrow = 32; maxcol = 80;  break;
  case 4: maxrow = 43; maxcol = 80;  break;
  case 5: maxrow = 27; maxcol = 132; break;
  }

  maxloc = ( maxcol * maxrow ) - 1;

  for(;;)
  {
    if ( count < 1 )
    {
      socks[0] = vt->sockit;

      count = select( socks, 1, 0, 0, timeout );

      if ( count == 0 )
      {
        timeout = -1;

        if ( nonblank != 0 )
          nvtline( vt, line, col );            /* flush pending line */

        line[0] = EOT; col = 1;                /* simulate EOT line  */
        mode = BDS; goto EOL;                  /* short circuit FSM  */
      }
      else
      if ( count > 0 )
      {
        timeout = RECVTIMEOUT;

        count = recv( vt->sockit, buf, sizeof(buf)-1, 0 );

        /* if count < 0 then socket error  */
        /* if count = 0 then socket closed */

        if ( count <= 0 )
          break; /* exit for(;;) */
      }

      next = buf;
    }

    /* begin telnet stream state machine */

    ch = *next;

    switch ( mode ) /* 3270 nvt state machine */
    {
    case BDS: switch ( ch ) /* begin data stream */
              {
              case SEWA:
              case SEW:
              case EWA:
              case EW: /* erase-write operations */
                      loc = 0;
                      mode = WCC;
                      if ( col ) goto EOL;
                      goto NL;

              case SW:
              case W: /* write operations */
                     mode = WCC;
                     goto NEXT;

              case SRMA:
              case SRM:
              case SRB:
              case RM:
              case RB: /* read operations */
                      nvtransmit( vt, cleared, 5 );
                      mode = SKIP;
                      goto NEXT;

              case SEAU:
              case EAU: /* no data follows */
                       mode = SKIP;
                       goto NEXT;

              case SWSF:
              case WSF: /* skip following structured fields */
                       mode = FLD;
                       goto NEXT;

              case IAC: mode = ch;
                        goto NEXT;
              }
              goto TRY_TEXT;

    TRY_TEXT:  /* drop into text mode */
    case TEXT: switch ( ch )
               {
               case EUA:
               case GE:
               case MF:
               case RA:
               case SA:
               case SBA:
               case SF:
               case SFE:
               case IAC: /* orders */
                        mode = ch;
                        goto NEXT;

               case IC: /* ic */
                        vt->cursor = loc;
                        goto NEXT;

               case PT: /* pt */
                        goto NEXT;
               }
               ch = ebc_asc[ch];
               goto USE;

    case SKIP: switch( ch )
               {
               case IAC: mode = ch;
                         goto NEXT;
               }
               goto NEXT;

    case IAC: switch( ch )
              {
              case WONT:
              case WILL:
              case DONT:
              case DO:
                      mode = ch;
                      goto NEXT;

              case SB: mode = ch;
                       goto NEXT;

              case EOR: mode = BDS;
                        goto NEXT;
              }
              goto NEXT;

    case DO: switch ( ch )
             {
             case TELOPT_BINARY:
             case TELOPT_TTYPE:
             case TELOPT_EOR:
                             reply = will;
                             goto NEGOTIATE;
             }
             reply = willnot;
             goto NEGOTIATE;

    case DONT: reply = willnot;
               goto NEGOTIATE;

    case WILL: switch ( ch )
               {
               case TELOPT_BINARY:
               case TELOPT_TTYPE:
               case TELOPT_EOR:
                               reply = doit;
                               goto NEGOTIATE;
               }
               reply = donot;
               goto NEGOTIATE;

    case WONT: reply = donot;
               goto NEGOTIATE;

    case SB: switch( ch )
             {
             case TELOPT_TTYPE: mode = ch;
                                goto NEXT;

             case IAC: mode = SBSE;
                       goto NEXT;
             }
             goto NEXT;

    case SBSE: switch( ch )
               {
               case SE: mode = TEXT;
                        goto NEXT;
               }
               mode = SB;
               goto NEXT;

    case TELOPT_TTYPE: switch( ch )
                       {
                       case TELQUAL_SEND: nvtransmit( vt, t3270, 16 );
                                          mode = SB;
                                          goto NEXT;
                       }
                       mode = SB;
                       goto NEXT;

    case WCC: switch ( ch )
              {
              case IAC: mode = ch;
                        goto NEXT;

              default: vt->ind = ( ch & WCC_RESTORE );
                       mode = TEXT;
                       goto NEXT;
              }
              goto NEXT;

    case SF: /* sf,a */
             ch = ' ';
             mode = TEXT;
             goto USE;

    case SBA: to = ( ch & 0x3F ) << 6;
              mode = SBAL;
              goto NEXT;

    case SBAL: to |= ( ch & 0x3F );
               goto JUMP;

    case RA: /* ra,a0,a1,c */
             to = ( ch & 0x3F ) << 6;
             mode = RAL;
             goto NEXT;

    case RAL: to |= ( ch & 0x3F );
              mode = RAC;
              goto NEXT;

    case RAC: if ( ch == GE )
              {
                mode = RACC;
                goto NEXT;
              }
              /* fill = ch; */
              goto JUMP;

    case RACC: /* fill = ch; */
               goto JUMP;

    case GE: /* ge,c */
             if ( ch < 0x40 || ch > 0xFE )
               goto NEXT;

             ch = ebc_asc[ch];
             goto USE;

    case EUA: /* eu,p0 */
    case SA:  /* sa,p0 */
              mode = B0;
              goto NEXT;

    case B0: mode = B1;     /* ignore byte 0 */
             goto NEXT;

    case B1: mode = TEXT;   /* ignore byte 1 */
             goto NEXT;

    case SFE: /* sfe,n,p0,p1...pn */
    case MF:  /* mf,n,p0,p1...pn */
              pairs = ch;
              mode = P0;
              goto NEXT;

    case P0: mode = P1;
             goto NEXT;

    case P1: mode = (--pairs) ? P0 : TEXT;
             goto NEXT;

    case FLD: switch ( ch )
              {
              case IAC: mode = ch;
                        goto NEXT;

              default: sflen = ch << 8;
                       mode = FLEN;
                       goto NEXT;
              }
              goto NEXT;

    case FLEN: sflen |= ch;
               mode = FID;
               goto NEXT;

    case FID: sflen -= 3;
              mode = FDATA;
              goto NEXT;

    case FDATA: if ( --sflen > 0 ) mode = FLD;
                goto NEXT;

    default: goto NEXT;

    } /* switch(mode) */

    /* common nvt state machine end-points */

    NEGOTIATE: reply[2] = ch;
               nvtransmit( vt, reply, 3 );
               mode = BDS;
               goto NEXT;

    JUMP: mode = TEXT;
          from = loc;
          loc = to;
          if ( ROW(from) != ROW(to) ) goto EOL;
          goto NEXT;

    USE:
          to = COL(loc++);
          if ( ch != ' ' ) nonblank = line[to] = ch;
          if ( col < ++to ) col = to;
          if ( to < maxcol ) goto NEXT;
          if ( loc > maxloc ) loc = 0;
    EOL:
          if ( nonblank != 0 )
            nvtline( vt, line, col );
    NL:
          memset( line, ' ', maxcol );
          nonblank = col = 0;
    NEXT:
          next++;
          count--;

    /* end telnet stream state machine */

  } /* for(;;) */

  if ( nonblank != 0 )
    nvtline( vt, line, col );

  if ( count < 0 )  nvterrno( vt, line ); /* socket error */

  DosPostEventSem( vt->ready );
  DosCloseEventSem( vt->ready );

  DosExit(0,0);
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

 typedef struct { int i; char *c; } tcmd;

 intern tcmd tcmds[] = { GA,    _GA,
                         AYT,   _AYT,
                         AO,    _AO,
                         IP,    _IP,
                         BREAK, _BREAK,
                         NOP,   _NOP,
                         EOR,   _EOR,
                         ABORT, _ABORT,
                         SUSP,  _SUSP,
                         xEOF,  _EOF,
                         0,     "?" };


#define upper(z) ((z)&~0x20) /* _toupper() */

intern int different( char* l, char* s )
{
  while ( *l && *s && *l == upper(*s) ) { l++; s++; }
  return ( *l || *s ) ? 1 : 0;
}

int cmdkey( char *c )
{
  tcmd *t = tcmds;
  while ( t->i && different(t->c,c) ) t++;
  return t->i;
}

char* cmdstr( int i )
{
  tcmd *t = tcmds;
  while ( t->i && t->i != i ) t++;
  return t->c;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int pop( HMTX lock, linked *list, linked *item )
{
  linked i = NULL;
  APIRET rc = DosRequestMutexSem( lock, SEM_INDEFINITE_WAIT );

  if ( !rc )
  {
    i = *list;

    if ( i ) *list = i->next;

    rc = DosReleaseMutexSem( lock );
  }
  *item = i;

  return rc ? 0 : 1;
}

int push( HMTX lock, linked *list, linked item )
{
  APIRET rc = DosRequestMutexSem( lock, SEM_INDEFINITE_WAIT );

  if ( !rc )
  {
    item->next = *list;
    *list = item;

    rc = DosReleaseMutexSem( lock );
  }
  return rc ? 0 : 1;
}

int queue( HMTX lock, linked *list, linked item )
{
  APIRET rc = DosRequestMutexSem( lock, SEM_INDEFINITE_WAIT );

  if ( !rc )
  {
    item->next = NULL;

    if ( *list )
    {
      linked i = *list;
      while ( i && i->next ) i = i->next;
      i->next = item;
    }
    else
    {
      *list = item;
    }
    rc = DosReleaseMutexSem( lock );
  }
  return rc ? 0 : 1;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define PAGESIZE 4096

void* allocpool()
{
  APIRET rc;
  PVOID  pool = NULL;

  rc = DosAllocMem( &pool,
                    PAGESIZE,
                    PAG_READ | PAG_WRITE );
  if ( !rc )
    rc = DosSubSetMem( pool,
                       DOSSUB_INIT |
                       DOSSUB_SERIALIZE |
                       DOSSUB_SPARSE_OBJ,
                       PAGESIZE );
  return pool;
}

void* relsepool( void* pool )
{
  if ( pool )
  {
    DosSubUnsetMem( pool );
    DosFreeMem( pool );
    pool = NULL;
  }
  return pool;
}

void* alloc( void* pool, int bufsize )
{
  if ( pool )
  {
    if ( DosSubAllocMem( pool, (PPVOID)&pool, bufsize ) != 0 )
      pool = 0;
  }
  return pool;
}

void* relse( void* pool, void* buf, int bufsize )
{
  APIRET rc = 1;

  if ( pool )
    rc = DosSubFreeMem( pool, buf, bufsize );

  return rc ? buf : NULL;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int wait( HEV event, int timeout )
{
  int rc = DosWaitEventSem( event, timeout );

  if ( !rc )
    rc = DosResetEventSem( event, (PULONG)&timeout );

  return rc;
}

int post( HEV event )
{
  return DosPostEventSem( event );
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

unsigned long hostaddress( char *cp )
{
  unsigned long address = inet_addr( cp );

  if ( address == 0 || address == INADDR_NONE )
  {
    struct hostent *hp = gethostbyname( cp );

    if ( hp )
      address = *((unsigned long *)(hp->h_addr));
  }
  return address;
}

