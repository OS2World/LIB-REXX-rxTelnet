
/*
 *  Definitions of 3270/EBCDIC codes.
 */

                                                  /* command format  */
#define  W      CMD_WRITE                         /* cmd,flag        */
#define  EW     CMD_ERASE_WRITE
#define  EWA    CMD_ERASE_WRITE_ALTERNATE
#define  WSF    CMD_WRITE_STRUCTURED_FIELD
#define  EAU    CMD_ERASE_ALL_UNPROTECTED
#define  RB     CMD_READ_BUFFER
#define  RM     CMD_READ_MODIFIED

#define  SW     CMD_SNA_WRITE
#define  SEW    CMD_SNA_ERASE_WRITE
#define  SEWA   CMD_SNA_ERASE_WRITE_ALTERNATE
#define  SWSF   CMD_SNA_WRITE_STRUCTURED_FIELD
#define  SEAU   CMD_SNA_ERASE_ALL_UNPROTECTED
#define  SRB    CMD_SNA_READ_BUFFER
#define  SRM    CMD_SNA_READ_MODIFIED
#define  SRMA   CMD_SNA_READ_MODIFIED_ALL

                                                  /* order formats  */
#define  SF     ORDER_SF                          /* o,a            */
#define  SFE    ORDER_SFE                         /* o,n,p0,p1...pn */
#define  SBA    ORDER_SBA                         /* o,ba           */
#define  SA     ORDER_SA                          /* o,p0           */
#define  MF     ORDER_MF                          /* o,n,p0,p1...pn */
#define  IC     ORDER_IC                          /* o              */
#define  PT     ORDER_PT                          /* o              */
#define  RA     ORDER_RA                          /* o,ba,(e)c      */
#define  EUA    ORDER_EUA                         /* o,ba           */
#define  GE     ORDER_GE                          /* o,c            */


#define  BDS     0x0200            /* Begin Data Stream */
#define  EDS     0x0201            /* End Data Stream */
#define  WCC     0x0202            /* Write Control Character */
#define  B0      0x0203            /* ushort byte 0 */
#define  B1      0x0204            /* ushort byte 0 */
#define  P0      0x0205            /* attribute pair byte 0 */
#define  P1      0x0206            /* attribute pair byte 1 */
#define  SBAL    0x0300+SBA        /* SBA location */
#define  RAL     0x0300+RA         /* RA location */
#define  RAC     0x0400+RA         /* RA character or GE */
#define  RACC    0x0500+RA         /* RA character */
#define  FLD     0x0400+WSF        /* structured field length byte 0 */
#define  FLEN    0x0500+WSF        /* structured field length byte 1 */
#define  FID     0x0600+WSF        /* structured field id */
#define  FDATA   0x0700+WSF        /* structured field data */

