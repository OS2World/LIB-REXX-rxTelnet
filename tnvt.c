#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nvt.h"

HNVT hnvt;

#define MID "\x1D"

void input(void)
{
  int   count;
  char  buf[MAXLINESIZE], *p;
  int   bufsize;

  puts( MID "stdin ready" );

  for(;;)
  {
    p = gets( buf );

    if ( !p )
      break;

    bufsize = strlen( buf );

    buf[bufsize+1] = '\0';

    count = nvtputs( hnvt, buf, bufsize );

    if ( !count )
      break;

    if ( count != bufsize )
      puts( MID "nvtputs error" );
  }
}

void output(void)
{
  int   count;
  char  buf[MAXLINESIZE];

  puts( MID "stdout ready" );

  for(;;)
  {
    count = nvtgets( hnvt, buf, sizeof(buf) - 1 );

    if ( !count )
      break;

    if ( count < 0 )
      puts( MID "nvtgets error" );
    else
    {
      buf[count] = '\0';
      puts( buf );
    }
  }
}

void task( void *parm )
{
  output();
  puts( MID "press ENTER to finish" );
}

int main( int argc, char *argv[] )
{
  char *host;
  int   port;
  int   model;
  int   tid = 0;

  if ( argc < 2 || argc > 4 )
  {
    puts( "syntax:   TNVT  hostname  [ port | - ]  [ 3270-model ]" );
    return 1;
  }

  host  = argv[1];
  port  = 0;
  model = 0;

  if ( argc > 2 && strcmp( argv[2], "-" ) != 0 )
  {
    port = atoi( argv[2] );

    if ( !port )
      printf( MID "port parameter '%s' ignored \n", argv[2] );
  }

  if ( argc > 3 )
  {
    model = strtol( argv[3], 0, 0 );

    if ( model < 2 || model > 5 )
    {
      model = 0;

      printf( MID "model parameter '%s' ignored \n", argv[3] );
    }
  }

  if ( model )
    hnvt = nvt3270( host, port, model );
  else
    hnvt = nvtopen( host, port );

  if ( !hnvt )
    puts( MID "nvtopen failed" );
  else
  {
    tid = _beginthread( task, 0, 16*1024, 0 );

    if ( !tid )
      puts( MID "unable to start subtask" );
    else
      input();

    nvtclose( hnvt );
  }

  return 0;
}
