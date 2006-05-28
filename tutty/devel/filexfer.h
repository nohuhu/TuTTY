#ifndef _FILEXFER_H
#define _FILEXFER_H

#include "misc.h"

#ifndef BUFSIZE
#define BUFSIZE 2048
#endif /* BUFSIZE */

/*
 * Special characters meaningful for X- and YModem protocols.
 */

#define SOH			    0x01
#define STX			    0x02
#define	EOT			    0x04
#define	ACK			    0x06
#define NAK			    0x15
#define	CAN			    0x18
#define CRC			    0x43
#define YMG			    0x47

/*
 * Directions. X/YModem protocols are half-duplex but ZModem
 * and Kermit are full-duplex.
 */

#define DIR_RECV		    0
#define DIR_SEND		    1
#define DIR_BOTH		    2

/*
 * File transfer protocols.
 */

#define PROTO_XMODEM		    0
#define PROTO_XMODEM_CRC	    1
#define PROTO_XMODEM_1K		    2
#define PROTO_YMODEM		    3
#define PROTO_YMODEM_G		    4
#define PROTO_ZMODEM		    5
#define PROTO_KERMIT		    6

/*
 * Default number of retries.
 */

#define XYMODEM_RETRIES_RECV	    10
#define XYMODEM_RETRIES_SEND	    10

/*
 * Default timeout values in seconds.
 */

#define XYMODEM_TIMEOUT_RECV_INIT   10
#define XYMODEM_TIMEOUT_RECV	    1

#define XYMODEM_TIMEOUT_SEND_INIT   60
#define XYMODEM_TIMEOUT_SEND	    10

/*
 * Protocol block sizes (fixed).
 */

#define BS_XMODEM_STD		    132
#define BS_XMODEM_CRC		    133
#define BS_XMODEM_1K		    1029

#define BS_YMODEM_STD		    1029
#define BS_YMODEM_G		    1029

/*
 * Protocol block data sizes (fixed).
 */

#define	DS_XMODEM_STD		    128
#define	DS_XMODEM_CRC		    128
#define	DS_XMODEM_1K		    1024

#define DS_YMODEM_STD		    1024
#define DS_YMODEM_G		    1024

/*
 * Protocol state definitions.
 */

#define XY_STATE_NONE		    0

#define	XY_STATE_RECV_INIT	    1
#define	XY_STATE_RECV_WAIT_DATA	    2
#define XY_STATE_RECV_BLOCK	    3
#define XY_STATE_RECV_SEND_ACK	    4
#define XY_STATE_RECV_SEND_NAK	    5
#define XY_STATE_RECV_SEND_CAN	    6
#define XY_STATE_RECV_FINISH	    7

#define XY_STATE_SEND_INIT	    11
#define XY_STATE_SEND_INIT_WAIT_NAK 12
#define XY_STATE_SEND_BLOCK	    13
#define XY_STATE_SEND_BLOCK_RETRY   14
#define XY_STATE_SEND_WAIT_ACK	    15
#define XY_STATE_SEND_EOT	    16
#define XY_STATE_SEND_EOT_WAIT_ACK  17
#define XY_STATE_SEND_FINISH	    18

/*
 * The main structure for all protocols.
 */

typedef struct _filexfer_t {
    int direction;
    int proto;
    int state;
    int block_num;
    int response;
    int attempt;
    int timeout;
    bufchain in;
    bufchain out;
    void *pd;
    void *fh;
    char filename[BUFSIZE];
} filexfer_t;


/*
 * This function should be called to initiate file transfer.
 * It handles all structure initialization tasks and such.
 * One is free to call xfer_process() immediately after
 * initiating.
 */

int xfer_initiate(filexfer_t *xfer, char *filename,
		  int direction, int proto, void *pd);

/*
 * The only function used for processing file transfer data,
 * state transitions and such. Also it does update progress
 * dialog information.
 * Return value is FALSE if there's nothing to send or
 * TRUE if there's something in the output bufchain that we
 * need to send to the other side.
 */

int xfer_process(filexfer_t *xfer);

#endif /* _FILEXFER_H */