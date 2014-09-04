#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define INCL_REXXSAA
#include <rexxsaa.h>

//nclude "wto.h"
#include "nvt.h"


/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

int numeric( char *str, int len, int *number )
{
  int minus = 0;
  int value = 0;

  if ( !len ) return 0;

  if ( *str == '+' ) { minus = 0; str++; len--; }
  else
  if ( *str == '-' ) { minus = 1; str++; len--; }

  if ( !len ) return 0;

  while ( *str && len )
  {
    if ( *str < '0' || *str > '9' ) return 0;

    value = ( value * 10 ) + ( *str - '0' );
    str++; len--;
  }

  *number = minus ? -value : value;
  return 1;
}

HNVT rnvthandle( RXSTRING *var )
{
  HNVT hnvt = 0;

  if ( var->strptr[0] >= '0' && var->strptr[0] <= '9' )
    numeric( var->strptr, var->strlength, (int*)&hnvt );

  return hnvt;
}

int rnvtvar( PRXSTRING var, HNVT hnvt )
{
  _ultoa( (unsigned long)hnvt, var->strptr, 10 );
  var->strlength = strlen(var->strptr);

  return var->strlength;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

 #define INVALID_CALL_TO_FUNCTION 40

 #define RexxFunction(k) \
             APIRET APIENTRY  k  ( PSZ function,        \
                                   LONG argc,           \
                                   RXSTRING argv[],     \
                                   PSZ queue,           \
                                   PRXSTRING result )


RexxFunction( Telnet )
{
  int rc = 1;

  if ( argc == 1 || argc == 2 )
  {
    int port = DEFAULTPORT;

    if ( argc > 1 && argv[1].strlength )
      if ( numeric( argv[1].strptr, argv[1].strlength, &port ) )
        { if ( !port ) port = DEFAULTPORT; }
      else
        { port = 0; }

    if ( argv[0].strlength && port )
    {
      HNVT hnvt = nvtopen( argv[0].strptr, port );

      if ( hnvt )
        rnvtvar( result, hnvt );
      else
        result->strptr[0] = result->strlength = 0;

      rc = 0;
    }
  }

  return rc ? INVALID_CALL_TO_FUNCTION : 0;
}

RexxFunction( Tquit )
{
  int rc = 1;

  if ( argc == 1 )
  {
    HNVT hnvt = rnvthandle( argv+0 );

    if ( hnvt )
      rc = nvtclose( hnvt );

    result->strptr[0] = rc ? '1' : '0';
    result->strptr[1] = '\0';
    result->strlength = 1;
    rc = 0;
  }

  return rc ? INVALID_CALL_TO_FUNCTION : 0;
}

RexxFunction( Tget )
{
  int rc = 1;

  if ( argc == 1 || argc == 2 )
  {
    int timeout = 0;

    rc = ( argc == 1 ) ? 0 : numeric( argv[1].strptr,
                                      argv[1].strlength,
                                      &timeout ) ? 0 : 1;
    if ( !rc )
    {
      HNVT hnvt = rnvthandle( argv+0 );

      if ( hnvt )
      {
        if ( timeout )
        {
          if ( nvtpeek( hnvt, timeout ) )
            timeout = 0;
          else
            result->strptr[0] = result->strlength = 0;
        }

        if ( !timeout )
          result->strlength = nvtgets( hnvt, result->strptr,
                                             result->strlength );
      }
    }
  }

  return rc ? INVALID_CALL_TO_FUNCTION : 0;
}

RexxFunction( Tput )
{
  int rc = 1;

  if ( argc == 2 )
  {
    if ( argv[1].strlength <= MAXLINESIZE )
    {
      HNVT hnvt = rnvthandle( argv+0 );

      if ( hnvt )
      {
        int count = nvtputs( hnvt, argv[1].strptr, argv[1].strlength );

        result->strptr[0] = ( count == argv[1].strlength ) ? '0' : '1';
        result->strptr[1] = '\0';
        result->strlength = 1;
        rc = 0;
      }
    }
  }

  return rc ? INVALID_CALL_TO_FUNCTION : 0;
}

RexxFunction( Tctl )
{
  int rc = 1;

  if ( argc == 1 )
  {
    HNVT hnvt = rnvthandle( argv+0 );

    if ( hnvt )
    {
      union { unsigned long l; char c[4]; } a;

      LNVT ln  = {0,0,0};
      int  err = nvtquery( hnvt, &ln );

      a.l = ln.addr;  /* result -> errno socket ip.address port */

      result->strlength = sprintf( result->strptr,
                                   "%i %i %i.%i.%i.%i %i",
                                    err,
                                       ln.socket,
                                          a.c[3], a.c[2], a.c[1], a.c[0],
                                                      ln.port );
      rc = 0;
    }
  }
  else
  if ( argc == 2 )
  {
    HNVT hnvt = rnvthandle( argv+0 );

    if ( hnvt )
    {
      int count = nvtcommand( hnvt, argv[1].strptr );

      result->strptr[0] = ( count > 0 ) ? '0' : '1';
      result->strptr[1] = '\0';
      result->strlength = 1;
      rc = 0;
    }
  }

  return rc ? INVALID_CALL_TO_FUNCTION : 0;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

RexxFunction( Tn3270 )
{
  int rc = 1;

  if ( argc == 1 || argc == 2 || argc == 3 )
  {
    int port  = DEFAULTPORT;
    int model = DEFAULTMODEL;

    if ( argc > 1 && argv[1].strlength )
      if ( numeric( argv[1].strptr, argv[1].strlength, &port ) )
        { if ( !port ) port = DEFAULTPORT; }
      else
        { port = 0; }

    if ( argc > 2 && argv[2].strlength )
      if ( numeric( argv[2].strptr, argv[2].strlength, &model ) )
      {
        if ( !model ) model = DEFAULTPORT;
        else if ( model < 2 || model > 5 ) model = 0;
      }
      else
      { model = 0; }

    if ( argv[0].strlength && port && model )
    {
      HNVT hnvt = nvt3270( argv[0].strptr, port, model );

      if ( hnvt )
        rnvtvar( result, hnvt );
      else
        result->strptr[0] = result->strlength = 0;

      rc = 0;
    }
  }

  return rc ? INVALID_CALL_TO_FUNCTION : 0;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

RexxFunction( NvtLoadFuncs )
{
  RexxRegisterFunctionDll( "Telnet", "Nvt", "Telnet" );
  RexxRegisterFunctionDll( "Tget",   "Nvt", "Tget"   );
  RexxRegisterFunctionDll( "Tput",   "Nvt", "Tput"   );
  RexxRegisterFunctionDll( "Tctl",   "Nvt", "Tctl"   );
  RexxRegisterFunctionDll( "Tquit",  "Nvt", "Tquit"  );
  RexxRegisterFunctionDll( "Tn3270", "Nvt", "Tn3270" );

  result->strptr[0] = result->strlength = 0;
  return 0;
}

RexxFunction( NvtDropFuncs )
{
  RexxDeregisterFunction( "Telnet" );
  RexxDeregisterFunction( "Tget"   );
  RexxDeregisterFunction( "Tput"   );
  RexxDeregisterFunction( "Tctl"   );
  RexxDeregisterFunction( "Tquit"  );
  RexxDeregisterFunction( "Tn3270" );

  result->strptr[0] = result->strlength = 0;
  return 0;
}
