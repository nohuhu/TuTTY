#include "putty.h"
#include "filexfer.h"

static int xy_block_sizes[5] =
{
    BS_XMODEM, BS_XMODEM_CRC, BS_XMODEM_1K,
    BS_YMODEM, BS_YMODEM_G
};

static int xy_data_sizes[5] =
{
    DS_XMODEM, DS_XMODEM_CRC, DS_XMODEM_1K,
    DS_YMODEM, DS_YMODEM_G
};

static char xy_block_descriptors[5] =
{
    SOH, SOH, STX, STX, STX
};

static char xy_init_descriptors[5] =
{
    NAK, CRC, CRC, CRC, YMG
};

int xfer_initiate(filexfer_t *xfer, char *filename,
		  int direction, int proto, void *pd)
{
    memset(xfer, 0, sizeof(filexfer_t));

    xfer->direction = direction;
    xfer->proto = proto;
    xfer->pd = pd;
    strncpy(xfer->filename, filename, BUFSIZE);

    return 1;
};

int xy_chksum(char *ptr, int count);
int xy_crc16(char *ptr, int count);

int xy_process(filexfer_t *xfer);
int xy_recv(filexfer_t *xfer);
int xy_send(filexfer_t *xfer);

int z_process(filexfer_t *xfer);

int k_process(filexfer_t *xfer);

int xfer_process(filexfer_t *xfer)
{
    if (!xfer)
	return 0;

    switch (xfer->proto) {
    case PROTO_XMODEM:
    case PROTO_XMODEM_CRC:
    case PROTO_XMODEM_1K:
    case PROTO_YMODEM:
    case PROTO_YMODEM_G:
	return xy_process(xfer);
    case PROTO_ZMODEM:
	return z_process(xfer);
    case PROTO_KERMIT:
	return k_process(xfer);
    };

    return 0;
};

int xy_process(filexfer_t *xfer)
{
    switch (xfer->direction) {
    case DIR_RECV:
	return xy_recv(xfer);
    case DIR_SEND:
	return xy_send(xfer);
    case DIR_BOTH:
	return 0;   /* Can't be, X/YModem is a half-duplex protocol. */
    };

    return 0;
};

void xy_timer(void *ctx, long now)
{
    filexfer_t *xfer = (filexfer_t *) ctx;

    if (now - xfer->timeout >= 0) {
	switch (xfer->state) {
	case XY_STATE_SEND_INIT:
	case XY_STATE_SEND_WAIT_ACK:
	case XY_STATE_SEND_EOT_WAIT_ACK:
	    {
		xfer->state = XY_STATE_SEND_FINISH;
		xfer->timeout = 0;
		xfer->attempt = 0;

		xy_process(xfer);
	    };
	};
    };
};

int xy_recv(filexfer_t *xfer)
{
    switch (xfer->state) {
    case XY_STATE_NONE:
	return 0;
    case XY_STATE_RECV_INIT:
	{
	    char init;

	    if (!xfer->fh = fopen(xfer->filename, "wbS")) {
		xfer->state = XY_STATE_RECV_SEND_CAN;

		return xy_recv(xfer);
	    };

	    memset(&xfer->data, 0, BS_XMODEM_1K);

	    bufchain_init(&xfer->in);
	    bufchain_init(&xfer->out);

	    init = xy_init_descriptors[xfer->proto];

	    bufchain_add(&xfer->out, &init, 1);

	    switch (xfer->proto) {
	    case PROTO_XMODEM:
	    case PROTO_XMODEM_CRC:
	    case PROTO_XMODEM_1K:
		xfer->block_num = 1;
		break;
	    case PROTO_YMODEM:
	    case PROTO_YMODEM_G:
		xfer->block_num = 0;
		break;
	    };

	    xfer->state = XY_STATE_RECV_WAIT_DATA;
	    xfer->timeout = schedule_timer(XY_TIMEOUT_RECV, xy_timer, xfer);

	    return 1;
	};
	break;
    case XY_STATE_RECV_WAIT_DATA:
	{
	    int bsize = bufchain_size(&xfer->in);

	    if (bsize >= xy_block_sizes[xfer->proto]) {
		/*
		 * We've got a complete block of data.
		 */
		xfer->state = XY_STATE_RECV_BLOCK;

		return xy_recv(xfer);
	    };
	};
	break;
    case XY_STATE_RECV_BLOCK:
	{
	    int block_size, block_num, crc;
	    char *buffer, *bufptr, descriptor;

	    bufchain_fetch(&xfer->in, &descriptor, 1);

	    switch (descriptor) {
	    case SOH:
		block_size = xfer->proto == PROTO_XMODEM ?
			    BS_XMODEM : BS_XMODEM_CRC;
		break;
	    case STX:
		block_size = BS_XMODEM_1K;
		break;
	    default:
		/*
		 * Something wrong here. SOH/STX was hit by line glitch?
		 * Clear input and send back NAK.
		 */
		{
		    xfer->state = XY_STATE_RECV_SEND_NAK;

		    return xy_recv(xfer);
		};
	    };

	    buffer = (char *) malloc(block_size);

	    bufchain_fetch(&xfer->in, buffer, block_size);

	    block_num = (int) buffer[1];

	    if (block_num != (255 - (int) buffer[2])) {
		/*
		 * Got a glitch. Request block resend.
		 */
		xfer->state = XY_STATE_RECV_SEND_NAK;

		return xy_recv(xfer);
	    };
	};
	break;
    case XY_STATE_RECV_SEND_NAK:
	bufchain_clear(&xfer->in);

    case XY_STATE_RECV_SEND_ACK:
	{
	    char ack;

	    ack = xfer->state == XY_STATE_RECV_SEND_NAK ?
		    NAK : ACK;

	    bufchain_add(&xfer->out, &ack, 1);

	    return 1;
	};
	break;
    case XY_STATE_RECV_FINISH:
	{
	};
    };

    return 0;
};

int xy_send(filexfer_t *xfer)
{
    switch (xfer->state) {
	/* 
	 * Something weird, we shouldn't be here with this state.
	 */
    case XY_STATE_NONE:
	return 0;	

	/*
	 * First we open the file for reading, initialize timer and
	 * set up input and output buffers.
	 * Since it is not required for sender to actually send anything
	 * before receiver side initializes the transfer, we simply
	 * make a transition to the next state, which is
	 * XY_STATE_SEND_INIT_WAIT.
	 * If timeout occurs, transfer is aborted and state returned to
	 * XY_STATE_NONE.
	 */
    case XY_STATE_SEND_INIT:
	{
	    xfer->fh = fopen((const char *) xfer->filename, "rbS");

	    if (xfer->fh == NULL) {
		xfer->state = XY_STATE_NONE;
		
		return 0;
	    };

	    bufchain_init(&xfer->in);
	    bufchain_init(&xfer->out);

	    switch (xfer->proto) {
	    case PROTO_XMODEM:
		xfer->block_size = BS_XMODEM_STD;
		xfer->data_size = DS_XMODEM_STD;
		xfer->descriptor = SOH;
		break;
	    case PROTO_XMODEM_CRC:
		xfer->block_size = BS_XMODEM_CRC;
		xfer->data_size = DS_XMODEM_CRC;
		xfer->descriptor = SOH;
		break;
	    case PROTO_XMODEM_1K:
		xfer->block_size = BS_XMODEM_1K;
		xfer->data_size = DS_XMODEM_1K;
		xfer->descriptor = STX;
		break;
	    case PROTO_YMODEM:
		xfer->block_size = BS_YMODEM_STD;
		xfer->data_size = DS_YMODEM_STD;
		xfer->descriptor = STX;
		break;
	    case PROTO_YMODEM_G:
		xfer->block_size = BS_YMODEM_G;
		xfer->data_size = DS_YMODEM_G;
		xfer->descriptor = STX;
		break;
	    default:
		xfer->block_size = BS_XMODEM_STD;
		xfer->data_size = DS_XMODEM_STD;
		xfer->descriptor = SOH;
	    };

	    xfer->block_num = 1;

	    xfer->state = XY_STATE_SEND_INIT_WAIT;
	    xfer->attempt = 0;
	    xfer->timeout = schedule_timer(XYMODEM_TIMEOUT_SEND_INIT, 
					   xy_timer, xfer);
	};
	break;

	/*
	 * Now, we've got some input. It should be the first character
	 * determining the actual transmission protocol and mode.
	 * Since both X- and YModem protocols are almost the same,
	 * they're done in one state machine.
	 * Note that we don't reinitialize the original timer, just
	 * ignore any garbage that we may have in the input buffer.
	 * If we recognize starting character, we make transition to
	 * XY_STATE_SEND_BLOCK.
	 */
    case XY_STATE_SEND_INIT_WAIT_NAK:
	{
	    char c = 0;
	    int success = FALSE;
	    int bsize = bufchain_size(&xfer->in);

	    if (!bsize) /* Don't know why we're here, but still waiting. */
		break;

	    while (!success && bsize-- > 0) {
		/*
		 * Get first character from the input buffer and analyze it.
		 */
		bufchain_fetch(&xfer->in, &c, 1);

		switch (c) {
		case NAK:
		    {
			xfer->proto = PROTO_XMODEM;
			success = TRUE;
		    };
		    break;
		case CRC:
		    {
			success = TRUE;
		    };
		    break;
		case YMG:
		    {
			xfer->proto = PROTO_YMODEM_G;
			success = TRUE;
		    };
		    break;
		};

		if (success) {
		    /*
		     * We've found the needed starting character, treat it
		     * as success and start transfer by sending first block.
		     */
		    bufchain_clear(&xfer->in);
		    expire_timer_context(xfer);
		    xfer->block_num = 1;
		    xfer->state = XY_STATE_SEND_BLOCK;

		    return xy_send(xfer);
		} else
		    /*
		     * No luck. Assume we've got some garbage.
		     */
		    bufchain_consume(&xfer->in, 1);
	    };
	};
	break;
    case XY_STATE_SEND_BLOCK_RETRY:
	{
	    fseek(xfer->fh, -xfer->data_size, SEEK_CUR);
	};
    case XY_STATE_SEND_BLOCK:
	{
	    int i, dataread, crc;
	    char *buffer, *bufptr, *fbuffer;

	    fbuffer = (char *) malloc(xfer->data_size);

	    dataread = fread(fbuffer, 1, xfer->data_size, xfer->fh);

	    if (dataread == 0) {
		/*
		 * End of file reached or we can't read from it.
		 */
		free(fbuffer);

		xfer->state = XY_STATE_SEND_EOT;

		return xy_send(xfer);
	    };

	    bufptr = buffer = (char *) malloc(xfer->block_size);

	    *bufptr = xfer->descriptor;
	    *bufptr = xfer->block_num;
	    *bufptr = ~xfer->block_num;

	    for (i = 0; i < dataread; i++)
		*bufptr = fbuffer[i];

	    if (dataread < xfer->data_size)
		for (i = 0; i < (xfer->data_size - dataread); i++)
		    *bufptr = '\0';

	    crc = xfer->proto == PROTO_XMODEM ?
		xy_chksum(buffer + 3, xfer->data_size) :
		xy_crc16(buffer + 3, xfer->data_size);

	    if (xfer->proto == PROTO_XMODEM)
		*bufptr = crc & 0xff;
	    else {
		*bufptr = (crc >> 8) & 0xff;
		*bufptr = crc & 0xff;
	    };

	    bufchain_add(&xfer->out, buffer, xfer->block_size);

	    free(buffer);
	    free(fbuffer);

	    xfer->state = XY_STATE_SEND_WAIT_ACK;
	    xfer->attempt = 0;
	    xfer->timeout = schedule_timer(XYMODEM_TIMEOUT_SEND, xy_timer, xfer);
	    
	    return 1;
	};
	break;
    case XY_STATE_SEND_WAIT_ACK:
    case XY_STATE_SEND_EOT_WAIT_ACK:
	{
	    char ack;
	    int state_ack, state_nak;
	    int bufsize = bufchain_size(&xfer->in);

	    if (!bufsize)
		return 0;

	    state_ack = xfer->state == XY_STATE_SEND_WAIT_ACK ?
			XY_STATE_SEND_BLOCK :
			XY_STATE_SEND_FINISH;

	    state_nak = xfer->state == XY_STATE_SEND_WAIT_ACK ?
			XY_STATE_SEND_BLOCK :
			XY_STATE_SEND_EOT;

	    while (bufsize) {
		bufchain_fetch(&xfer->in, &ack, 1);

		switch (ack) {
		case ACK:
		    {
			bufchain_consume(&xfer->in, 1);
			xfer->state = state_ack;
			xfer->block_num++;

			expire_timer_context(xfer);

			return xy_send(xfer);
		    };
		    break;
		case NAK:
		    {
			bufchain_consume(&xfer->in, 1);
			xfer->state = state_nak;

			expire_timer_context(xfer);

			return xy_send(xfer);
		    };
		    break;
		default:
		    /*
		     * Got some garbage, ignore it.
		     */
		    {
			bufchain_consume(&xfer->in, 1);
		    };
		};

		bufsize--;
	    };
	};
	break;
    case XY_STATE_SEND_EOT:
	{
	    char eot;

	    fclose(xfer->fh);

	    eot = EOT;
	    bufchain_add(&xfer->out, &eot, 1);

	    xfer->state = XY_STATE_SEND_EOT_WAIT_ACK;
	    xfer->attempt = 0;
	    xfer->timeout = schedule_timer(XYMODEM_TIMEOUT_SEND, xy_timer, xfer);
	};
	break;
    case XY_STATE_SEND_FINISH:
	{
	};
    };

    return 0;
};

int z_process(filexfer_t *xfer)
{
    return 0;
};

int k_process(filexfer_t *xfer)
{
    return 0;
};

int xy_chksum(char *ptr, int count)
{
    unsigned int csum;

    csum = 0;
    while (--count >= 0)
	csum += *ptr++;
    while (csum > 255)
	csum -= 256;

    return csum;
};

/*
 * This	function calculates the	CRC used by the	XMODEM/CRC Protocol
 * The first argument is a pointer to the message block.
 * The second argument is the number of	bytes in the message block.
 * The function	returns	an integer which contains the CRC.
 * The low order 16 bits are the coefficients of the CRC.
 */
int xy_crc16(char *ptr, int count)
{
    int	crc, i;

    crc	= 0;
    while (--count >= 0) {
	crc = crc ^ (int)*ptr++ << 8;
	for (i = 0; i <	8; ++i)
	    if (crc & 0x8000)
		crc = crc << 1 ^ 0x1021;
	    else
		crc = crc << 1;
	};
    return (crc	& 0xffff);
};
