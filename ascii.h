/*
 * Definitions of ASCII codes.
 */

#define  NUL       0               /* No Operation */
#define  SOH       1
#define  STX       2
#define  ETX       3
#define  EOT       4
#define  ENQ       5
#define  ACK       6               /* Acknowledge */
#define  BEL       7               /* Bell */
#define  BS        8               /* Back Space */
#define  HT        9               /* Horizontal Tab */
#define  LF        10              /* Line Feed */
#define  VT        11              /* Vertical Tab */
#define  FF        12              /* Form Feed */
#define  CR        13              /* Carriage Return */
#define  SYN       22              /* SYN */
#define  CAN       24              /* Cancel */

#define  ESC       27              /* Escape */

#define  LO_ASCII  32              /* ' ' - low ascii character */
#define  HI_ASCII  126             /* '~' - high ascii character */

#define  _ascii(z) (((z)>=LO_ASCII)&&((z)<=HI_ASCII))

#define  TEXT      0x0100
#define  SKIP      0x0101
#define  SBSE      0xFAF0

