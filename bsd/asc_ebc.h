/*-
 * Copyright (c) 1988, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      @(#)/usr/src/usr.bin/tn3270/api/asc_ebc.h   8.1 (Berkeley) 6/6/93
 */

/*
 * Definitions of translate tables used for ascii<->ebcdic translation.
 */

#define INCLUDED_ASCEBC

/*
 * ascii/ebcdic translation information
 */

#define NASCII  128             /* number of ascii characters */

#define NEBC    256             /* number of ebcdic characters */

extern unsigned char
        asc_ebc[NASCII], ebc_asc[NEBC];


/* excerpted from  @(#)/usr/src/usr.bin/tn3270/ctlr/screen.h    8.1 (Berkeley) 6/6/93 */

#define BAIC(x)                 ((x)&0x3f)
#define CIAB(x)                 (CIABuffer[(x)&0x3f])
#define BufferTo3270_0(x)       (CIABuffer[(int)((x)/0x40)])
#define BufferTo3270_1(x)       (CIABuffer[(x)&0x3f])
#define Addr3270(x,y)           (BAIC(x)*64+BAIC(y))
#define SetBufferAddress(x,y)   ((x)*NumberColumns+(y))

 extern char
        CIABuffer[];
