/**/
 hostname = arg(1)
 port = 0

 if hostname = '' then
   do
     say 'enter hostname'
     pull hostname
     say 'enter port'
     pull port
     if port = '' then port = 23
   end

 i = 0
 l = ''

 echo = ''
 retry = 0
 connected = 0

 EOT = '04'x /* diamond */
 ACK = '06'x /* spade */
 BEL = '07'x /* dot */
 OUT = '18'x /* up-arrow */
 IN  = '19'x /* down-arrow */

 signal on halt
 signal on error
 signal on syntax

 call RxFuncAdd "NvtLoadFuncs","Nvt","NvtLoadFuncs"
 call NvtLoadFuncs
 call RxFuncDrop "NvtLoadFuncs"

 socket = Telnet(hostname,port)

 if socket = ''
   then say 'telnet connection failed'
   else do
     connected = 1
     say 'socket='socket
     say 'status='tctl(socket) BEL
     do while connected
       call stdout
       call stdin
       end
     end

 say 'status='tctl(socket) BEL
 call quit
 return

stdin:
 call charout,OUT
 parse pull s
 if s = '`'
   then retry = 1
   else do
     c = Tput(socket,s)
     if c = 0 then
       echo = ACK||s
     else
       connected = 0
     end
 return

stdout:
 do forever
   if retry
     then do; l = Tget(socket,5000); retry = 0; end;
     else l = Tget(socket)
   select
     when l = EOT then leave
     when l = '' then do; connected = 0; leave; end;
     otherwise call sayit
     end
   end
 return

sayit:
 if echo <> '' & ( l = ACK | l = echo ) then do; echo = ''; return; end;
 say IN||l
 return

syntax:
 say '! syntax'
error:
 say '! error'
halt:
 say '! halt'
quit:
 call Tquit socket

 call RxFuncAdd "NvtDropFuncs","Nvt","NvtDropFuncs"
 call NvtDropFuncs
 call RxFuncDrop "NvtDropFuncs"
 return
