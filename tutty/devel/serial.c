#ifdef SERIAL_BACKEND

//#include <commctrl.h>

#include "putty.h"
#include "misc.h"
#include "resource.h"
#include "win_res.h"

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#ifndef PBM_SETRANGE
#define PBM_SETRANGE WM_USER + 1
#endif

#ifndef PBM_SETPOS
#define PBM_SETPOS WM_USER + 2
#endif

#ifndef PBM_SETSTEP
#define PBM_SETSTEP WM_USER + 4
#endif

#ifndef PBM_STEPIT
#define PBM_STEPIT WM_USER + 5
#endif

#define SERIAL_STATE_NONE	    0
#define SERIAL_STATE_INITIALIZING   1
#define SERIAL_STATE_RECONFIGURING  2
#define SERIAL_STATE_NOT_CONNECTED  3
#define SERIAL_STATE_DIALING        4
#define SERIAL_STATE_CONNECTED      5
#define SERIAL_STATE_XMODEMUPLOAD   6
#define SERIAL_STATE_XMODEMDOWNLOAD 7
#define SERIAL_STATE_YMODEMUPLOAD   8
#define SERIAL_STATE_YMODEMDOWNLOAD 9
#define SERIAL_STATE_ZMODEMUPLOAD   10
#define SERIAL_STATE_ZMODEMDOWNLOAD 11
#define SERIAL_STATE_KERMITUPLOAD   12
#define SERIAL_STATE_KERMITDOWNLOAD 13

#define DIAL_STATE_NONE             0
#define DIAL_STATE_INIT             1
#define DIAL_STATE_INITRESPONSE     2
#define DIAL_STATE_DIALING	    3
#define DIAL_STATE_DIALINGRESPONSE  4
#define DIAL_STATE_CONNECTED	    5
#define DIAL_STATE_RETRYING	    6
#define DIAL_STATE_ABORTED	    7
#define DIAL_STATE_CANCELED	    8

#define DIAL_RESPONSE_NONE	    0
#define DIAL_RESPONSE_OK	    1
#define DIAL_RESPONSE_ERROR         2
#define DIAL_RESPONSE_CONNECT       3
#define DIAL_RESPONSE_NO_DIALTONE   4
#define DIAL_RESPONSE_BUSY          5
#define DIAL_RESPONSE_NO_CARRIER    6
#define DIAL_RESPONSE_ABORT	    7
#define DIAL_RESPONSE_TIMEOUT	    8

#define XMODEM_STATE_NONE           0

#define YMODEM_STATE_NONE	    0

#define ZMODEM_STATE_NONE	    0

#define KERMIT_STATE_NONE	    0

#define SERIAL_MAX_BACKLOG 4096
#define SERIAL_MAX_READ_BUFFER 2048
#define SERIAL_MAX_WRITE_BUFFER 1024
#define SERIAL_XON_LIM SERIAL_MAX_READ_BUFFER % 4
#define SERIAL_XOFF_LIM SERIAL_MAX_READ_BUFFER - (SERIAL_MAX_READ_BUFFER % 4)
#define SERIAL_WAIT_TIMEOUT 10        /* 
                                       * We need to wait some time just not to 
                                       * overload system with calls
									   * to WairForMultipleObjects, I think 
                                       * this is enough (ms) 
                                       */

/* 
 * We don't really need to define different event sets
 * for different types of flow control, because flow control
 * itself is enforced by serial device driver. We just want
 * to monitor the port status. Nevertheless, I'm not really sure
 * at this point I will not want to overcome the default flow
 * control mechanism and do it myself, so I decided to leave
 * those event masks separated, just in case.
 */
#define FCTL_NONE EV_BREAK | EV_ERR | EV_RLSD | EV_RXCHAR
#define FCTL_RTSCTS EV_BREAK | EV_ERR | EV_RLSD | EV_RXCHAR
#define FCTL_XONXOFF EV_BREAK | EV_ERR | EV_RLSD | EV_RXCHAR

typedef struct serial_backend_data *Serial;

struct Socket_serial_tag {
    struct socket_function_table *fn;
    /* the above variable absolutely *must* be the first in this structure */

    char *error;

    HANDLE port;

    OVERLAPPED o_reader;
    OVERLAPPED o_status;
    OVERLAPPED o_writer;
    DCB hrdw_orig_state;
    DCB hrdw_current_state;
    COMMTIMEOUTS orig_timeouts;
    COMMTIMEOUTS current_timeouts;
    DWORD eventmask;

    Serial plug;
    Config *cfg;

    int state;

    union _protstate {
	struct _dialing {
	    int state;
	    int response;
	    int timeout;
	    UINT_PTR timer;
	    int attempt;
	} dialing;
	struct _xmodem {
	    int state;
	    int response;
	    int timeout;
	} xmodem;
	struct _ymodem {
	    int state;
	    int response;
	    int timeout;
	} ymodem;
	struct _zmodem {
	    int state;
	    int response;
	    int timeout;
	} zmodem;
	struct _kermit {
	    int state;
	    int response;
	    int timeout;
	} kermit;
    } protstate;

    /*
    struct hardware_flags {
	int cts:1;
	int rts:1;
	int cd:1;
	int dtr:1;
	int dsr:1;
    */

    void *private_ptr;
    bufchain input_data;
    bufchain output_data;
    int writable;
    int pending_error;		       /* in case send() returns error */
    int pending_read;
    int pending_status;
    int pending_write;			   /* sending is not complete yet, we have this bytes to send */
};

typedef struct Socket_serial_tag *Serial_Socket;

typedef struct serial_backend_data {
    struct plug_function_table *fn;
    /* the above variable absolutely *must* be the first in this structure */

    Serial_Socket s;
    int bufsize;
    void *frontend;
} *Serial;

static Serial_Socket s = NULL;

/*
 * Imported from window.c
 */
extern HWND hwnd;

/* */
char *serial_error(int err)
{
	char *error;
	switch (err)
	{
	case ERROR_INVALID_FUNCTION:
		error = "Incorrect function";
		break;
	case ERROR_FILE_NOT_FOUND:
		error = "The system cannot find the device specified";
		break;
	case ERROR_TOO_MANY_OPEN_FILES:
		error = "The system cannot open the file";
		break;
	case ERROR_ACCESS_DENIED:
		error = "Access is denied";
		break;
	case ERROR_INVALID_HANDLE:
		error = "The handle is invalid";
		break;
	case ERROR_NOT_ENOUGH_MEMORY:
	case ERROR_OUTOFMEMORY:
		error = "Not enough memory is available to process this command";
		break;
	case ERROR_NOT_READY:
		error = "The device is not ready";
		break;
	case ERROR_WRITE_FAULT:
		error = "The system cannot write to the specified device";
		break;
	case ERROR_READ_FAULT:
		error = "The system cannot read from the specified device";
		break;
	case ERROR_SHARING_VIOLATION:
		error = "The process cannot access the device because it is being used by another process";
		break;
	case ERROR_OPERATION_ABORTED:
		error = "The I/O operation has been aborted because of either a thread exit or an application request";
		break;
	case ERROR_IO_INCOMPLETE:
		error = "Overlapped I/O event is not in signaled state";
		break;
	default:
		error = "Unknown error";
	};

	return dupprintf("Error %d: %s.", err, error);
};

/* Socket-level functions */

static Plug sk_serial_plug(Socket sock, Plug p)
{
    Serial_Socket s = (Serial_Socket) sock;
    Serial ret = s->plug;
    if (p)
		s->plug = (Serial)p;
    return (Plug)ret;
}

static void sk_serial_close(Socket sock)
{
    Serial_Socket s = (Serial_Socket) sock;

	if (s) {
		CloseHandle(s->o_reader.hEvent);
		CloseHandle(s->o_status.hEvent);
		CloseHandle(s->o_writer.hEvent);
    
		if (s->port) {
			EscapeCommFunction(s->port, CLRDTR);
			SetCommTimeouts(s->port, &s->orig_timeouts);
			SetCommState(s->port, &s->hrdw_orig_state);
    
			CloseHandle(s->port);
		};

		sfree(s);
	};
}

/*
 * The function which tries to send on a socket once it's deemed
 * writable.
 */
void serial_try_send(Serial_Socket s)
{
    while (s->writable && bufchain_size(&s->output_data) > 0) {
		int nsent;
		DWORD err;
		void *data;
		int len;

		bufchain_prefix(&s->output_data, &data, &len);

		if (!WriteFile(s->port, data, len, &nsent, &s->o_writer)) {
			err = GetLastError();
			if (err == ERROR_IO_PENDING) {   /* We sent len bytes of data, but */
				s->writable = FALSE;         /* the operation is not complete yet, */
				s->pending_write = TRUE;     /* so we just have to wait a little. */
				bufchain_consume(&s->output_data, len);
			} else {                         /* An error occured */
				s->writable = FALSE;
				s->pending_write = FALSE;
				s->pending_error = err;
				s->error = serial_error(err);
			};
		} else {
			bufchain_consume(&s->output_data, nsent);
		};
	};
};

static int sk_serial_write(Socket sock, const char *buf, int len)
{
    Serial_Socket s = (Serial_Socket) sock;

    /*
     * Add the data to the buffer list on the socket.
     */
    bufchain_add(&s->output_data, buf, len);

    /*
     * Now try sending from the start of the buffer list.
     */
    if (s->writable)
		serial_try_send(s);

    return bufchain_size(&s->output_data);
}

/* 
 * We don't support this write oob functionality on serial ports, so just do nothing 
 */
static int sk_serial_write_oob(Socket sock, const char *buf, int len)
{
    return 0;
}

static void sk_serial_flush(Socket s)
{
    /*
     * We send data to the port as soon as we can anyway,
     * so we don't need to do anything here.  :-)
     */
}

/*
 * Each socket abstraction contains a `void *' private field in
 * which the client can keep state.
 */
static void sk_serial_set_private_ptr(Socket sock, void *ptr)
{
    Serial_Socket s = (Serial_Socket) sock;
    s->private_ptr = ptr;
}

static void *sk_serial_get_private_ptr(Socket sock)
{
    Serial_Socket s = (Serial_Socket) sock;
    return s->private_ptr;
}

/* This function is totally meaningless, so this is a stub. */
static void sk_serial_set_frozen(Socket sock, int is_frozen)
{
/*
    Serial_Socket s = (Serial_Socket) sock;

    if (s->frozen == is_frozen)
		return;
    s->frozen = is_frozen;
    if (!is_frozen && s->frozen_readable) {
		char c;
		recv(s->s, &c, 1, MSG_PEEK);
    }
    s->frozen_readable = 0;
*/
}

static char *sk_serial_socket_error(Socket sock)
{
    Serial_Socket s = (Serial_Socket) sock;
    return s->error;
}

/* Plug-level functions */

static void terminal_write(Serial sp, int len)
{
    void *buf;

    if (sp && len && (len <= bufchain_size(&sp->s->input_data))) {
	buf = smalloc(len);
	bufchain_fetch(&sp->s->input_data, buf, len);
	from_backend(sp->frontend, 0, buf, len);
	bufchain_consume(&sp->s->input_data, len);
	sfree(buf);
    };
    /* 
     * You know, I found that from_backend() will ALWAYS
     * return 0. Do we really have to call one more function
     * which will never work? Don't think so.
     */
//    sk_serial_set_frozen((Socket) serial->s, backlog > SERIAL_MAX_BACKLOG);
}

static int serial_closing(Plug plug, const char *error_msg, int error_code,
		       int calling_back)
{
	Serial sp = (Serial) plug;

    if (sp->s) {
	    sk_serial_close((Socket)sp->s);
        sp->s = NULL;
    };
    if (error_msg) {
        logevent(sp->frontend, error_msg);
        connection_fatal(sp->frontend, "%s", error_msg);
    };
    return 0;
}


static int serial_receive(Plug plug, int urgent, char *data, int len)
{
    Serial sp = (Serial) plug;
    bufchain_add(&sp->s->input_data, data, len);
    terminal_write(sp, len);
    return 1;
}

static void serial_sent(Plug plug, int bufsize)
{
    Serial sp = (Serial) plug;
    sp->bufsize = bufsize;
}


static int process_dialing(Serial_Socket s, char c);

/*
 * Called to set up the serial connection.
 * 
 * Returns an error message, or NULL on success.
 *
 * Also places the canonical host name into `realhost'. It must be
 * freed by the caller.
 */
static char *serial_init(void *frontend_handle, void **backend_handle, 
                         Config *cfg,
                         char *host, int port, char **realhost, int nodelay,
                         int keepalive)
{
    static struct plug_function_table plug_fn_table = {
	serial_closing,
	serial_receive,
	serial_sent
    }, *plug_fn_table_ptr = &plug_fn_table;

    static struct socket_function_table sk_fn_table = {
	sk_serial_plug,
	sk_serial_close,
	sk_serial_write,
	sk_serial_write_oob,
	sk_serial_flush,
	sk_serial_set_private_ptr,
	sk_serial_get_private_ptr,
	sk_serial_set_frozen,
	sk_serial_socket_error
    };

    int err;
    Serial sp;

    sp = snew(struct serial_backend_data);
    sp->fn = &plug_fn_table;
    sp->s = NULL;
    *backend_handle = sp;

    sp->frontend = frontend_handle;

    /*
     * Create Socket structure.
     */
    s = smalloc(sizeof(struct Socket_serial_tag));
    s->fn = &sk_fn_table;
    s->error = NULL;
    s->plug = sp;
    s->cfg = cfg;
    bufchain_init(&s->input_data);
    bufchain_init(&s->output_data);
    s->state = SERIAL_STATE_NONE;
    FillMemory(&s->protstate, sizeof(s->protstate), 0);
    s->writable = 0;		       /* to start with */
    s->pending_error = 0;
    s->pending_read = 0;
    s->pending_status = 0;
    s->pending_write = 0;
    FillMemory(&s->o_reader, sizeof(OVERLAPPED), 0);
    FillMemory(&s->o_status, sizeof(OVERLAPPED), 0);
    FillMemory(&s->o_writer, sizeof(OVERLAPPED), 0);
    FillMemory(&s->hrdw_orig_state, sizeof(DCB), 0);
    FillMemory(&s->hrdw_current_state, sizeof(DCB), 0);
    FillMemory(&s->orig_timeouts, sizeof(COMMTIMEOUTS), 0);
    FillMemory(&s->current_timeouts, sizeof(COMMTIMEOUTS), 0);

    s->o_reader.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    s->o_status.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    s->o_writer.hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
    
    *realhost = dupprintf("COM%d", cfg->port);

    s->state = SERIAL_STATE_INITIALIZING;
    
    s->port = CreateFile(*realhost, GENERIC_READ | GENERIC_WRITE, 0, 0,
                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL |
			 FILE_FLAG_OVERLAPPED, 0);

    if (s->port == INVALID_HANDLE_VALUE) {
	err = GetLastError();
	return dupprintf("Error initializing %s: %s", *realhost, serial_error(err));
    };

    GetCommTimeouts(s->port, &s->orig_timeouts);
    GetCommState(s->port, &s->hrdw_orig_state);
    GetCommState(s->port, &s->hrdw_current_state);
    
    s->hrdw_current_state.BaudRate = cfg->ser_baud;
    s->hrdw_current_state.ByteSize = cfg->ser_databits;
    s->hrdw_current_state.Parity = cfg->ser_parity;
    s->hrdw_current_state.StopBits = cfg->ser_stopbits;
    s->hrdw_current_state.fParity = TRUE;
    s->hrdw_current_state.fAbortOnError = TRUE;
    
    switch (cfg->ser_flowcontrol)
    {
    case NONE:
	s->hrdw_current_state.fOutxCtsFlow = FALSE;
	s->hrdw_current_state.fOutxDsrFlow = FALSE;
	s->hrdw_current_state.fDsrSensitivity = FALSE;
	s->hrdw_current_state.fRtsControl = RTS_CONTROL_ENABLE;
	s->hrdw_current_state.fDtrControl = DTR_CONTROL_ENABLE;
	s->hrdw_current_state.fInX = FALSE;
	s->hrdw_current_state.fOutX = FALSE;
	s->hrdw_current_state.fTXContinueOnXoff = TRUE;
	s->hrdw_current_state.XonLim = 0;
	s->hrdw_current_state.XoffLim = 0;
	s->eventmask = FCTL_NONE;
	break;
    case RTSCTS:
	s->hrdw_current_state.fOutxCtsFlow = TRUE;
	s->hrdw_current_state.fOutxDsrFlow = TRUE;
	s->hrdw_current_state.fDsrSensitivity = FALSE;
	s->hrdw_current_state.fRtsControl = RTS_CONTROL_HANDSHAKE;
	s->hrdw_current_state.fDtrControl = DTR_CONTROL_ENABLE; /* most modems interpret lowering DTR as a signal to */
	s->hrdw_current_state.fInX = FALSE;                     /* drop the line, so basically it's a bad idea to */
	s->hrdw_current_state.fOutX = FALSE;			           /* allow using DTR for flow control */
	s->hrdw_current_state.fTXContinueOnXoff = TRUE;
	s->hrdw_current_state.XonLim = 0;
	s->hrdw_current_state.XoffLim = 0;
	s->eventmask = FCTL_RTSCTS;
	break;
    case XONXOFF:
	s->hrdw_current_state.fOutxCtsFlow = FALSE;
	s->hrdw_current_state.fOutxDsrFlow = FALSE;
	s->hrdw_current_state.fDsrSensitivity = FALSE;
	s->hrdw_current_state.fRtsControl = RTS_CONTROL_ENABLE;
	s->hrdw_current_state.fDtrControl = DTR_CONTROL_ENABLE;
	s->hrdw_current_state.fInX = TRUE;
	s->hrdw_current_state.fOutX = TRUE;
	s->hrdw_current_state.fTXContinueOnXoff = FALSE;
	s->hrdw_current_state.XonLim = SERIAL_XON_LIM;
	s->hrdw_current_state.XoffLim = SERIAL_XOFF_LIM;
	s->eventmask = FCTL_XONXOFF;
	break;
    };

    SetCommState(s->port, &s->hrdw_current_state);
	
    SetupComm(s->port, SERIAL_MAX_READ_BUFFER, SERIAL_MAX_WRITE_BUFFER);
    EscapeCommFunction(s->port, SETDTR);
    SetCommMask(s->port, s->eventmask);

    s->state = SERIAL_STATE_NOT_CONNECTED;
    s->writable = TRUE;

    sp->s = s;

    if (strlen(host) > 0) {
	s->protstate.dialing.state = DIAL_STATE_INIT;
	process_dialing(s, '\0');
    };

    return NULL;
}

static void serial_free(void *handle)
{
    Serial sp = (Serial) handle;

    if (sp->s)
	sk_serial_close((Socket) sp->s);
    sfree(sp);
};

static void serial_reconfig(void *handle, Config *cfg)
{
    Serial sp = (Serial) handle;
    Serial_Socket s = sp->s;
    int prevstate;
    DWORD stat;

    if (!s || !s->port || s->port == INVALID_HANDLE_VALUE)
        return;

    prevstate = s->state;
    s->state = SERIAL_STATE_RECONFIGURING;

    GetCommState(s->port, &s->hrdw_current_state);

    s->hrdw_current_state.BaudRate = cfg->ser_baud;
    s->hrdw_current_state.ByteSize = cfg->ser_databits;
    s->hrdw_current_state.Parity = cfg->ser_parity;
    s->hrdw_current_state.StopBits = cfg->ser_stopbits;
    s->hrdw_current_state.fParity = TRUE;
    s->hrdw_current_state.fAbortOnError = TRUE;
    
    switch (cfg->ser_flowcontrol)
    {
    case NONE:
	s->hrdw_current_state.fOutxCtsFlow = FALSE;
	s->hrdw_current_state.fOutxDsrFlow = FALSE;
	s->hrdw_current_state.fDsrSensitivity = FALSE;
	s->hrdw_current_state.fRtsControl = RTS_CONTROL_ENABLE;
	s->hrdw_current_state.fDtrControl = DTR_CONTROL_ENABLE;
	s->hrdw_current_state.fInX = FALSE;
	s->hrdw_current_state.fOutX = FALSE;
	s->hrdw_current_state.fTXContinueOnXoff = TRUE;
	s->hrdw_current_state.XonLim = 0;
	s->hrdw_current_state.XoffLim = 0;
	s->eventmask = FCTL_NONE;
	break;
    case RTSCTS:
	s->hrdw_current_state.fOutxCtsFlow = TRUE;
	s->hrdw_current_state.fOutxDsrFlow = TRUE;
	s->hrdw_current_state.fDsrSensitivity = FALSE;
	s->hrdw_current_state.fRtsControl = RTS_CONTROL_HANDSHAKE;
	s->hrdw_current_state.fDtrControl = DTR_CONTROL_ENABLE; /* most modems interpret lowering DTR as a signal to */
	s->hrdw_current_state.fInX = FALSE;                     /* drop the line, so basically it's a bad idea to */
	s->hrdw_current_state.fOutX = FALSE;			           /* allow using DTR for flow control */
	s->hrdw_current_state.fTXContinueOnXoff = TRUE;
	s->hrdw_current_state.XonLim = 0;
	s->hrdw_current_state.XoffLim = 0;
	s->eventmask = FCTL_RTSCTS;
	break;
    case XONXOFF:
	s->hrdw_current_state.fOutxCtsFlow = FALSE;
	s->hrdw_current_state.fOutxDsrFlow = FALSE;
	s->hrdw_current_state.fDsrSensitivity = FALSE;
	s->hrdw_current_state.fRtsControl = RTS_CONTROL_ENABLE;
	s->hrdw_current_state.fDtrControl = DTR_CONTROL_ENABLE;
	s->hrdw_current_state.fInX = TRUE;
	s->hrdw_current_state.fOutX = TRUE;
	s->hrdw_current_state.fTXContinueOnXoff = FALSE;
	s->hrdw_current_state.XonLim = SERIAL_XON_LIM;
	s->hrdw_current_state.XoffLim = SERIAL_XOFF_LIM;
	s->eventmask = FCTL_XONXOFF;
	break;
    };

    SetCommState(s->port, &s->hrdw_current_state);
    SetCommMask(s->port, s->eventmask);

    GetCommModemStatus(s->port, &stat);
    if (stat & MS_RLSD_ON)
	s->state = prevstate;
    else
	s->state = SERIAL_STATE_NOT_CONNECTED;
};

/*
 * Called to send data down the serial connection.
 */
static int serial_send(void *handle, char *buf, int len)
{
    Serial sp = (Serial) handle;

    if (sp->s == NULL)
	return 0;

    sp->bufsize = sk_serial_write((Socket)sp->s, buf, len);

    return sp->bufsize;
}

/*
 * Called to query the current socket sendability status.
 */
static int serial_sendbuffer(void *handle)
{
    Serial sp = (Serial) handle;
    return sp->bufsize;
}

/*
 * Called to set the size of the window
 */
static void serial_size(void *handle, int width, int height)
{
    /* Do nothing! */
    return;
}

/*
 * Send serial special codes.
 */
static void serial_special(void *handle, Telnet_Special code)
{
    Serial sp = (Serial) handle;

    if (code == TS_NOP &&
	sp->s->state == SERIAL_STATE_NOT_CONNECTED) {
	sp->s->protstate.dialing.state = DIAL_STATE_INIT;
	process_dialing(s, '\0');
    };
    return;
}

/*
 * Return a list of the special codes that make sense in this
 * protocol.
 */
static const struct telnet_special *serial_get_specials(void *handle)
{
    return NULL;
}

static Socket serial_socket(void *handle)
{
    Serial sp = (Serial) handle;
    return (Socket)sp->s;
}

static int serial_sendok(void *handle)
{
    return 1;
}

/* This function is also a stub. */
static void serial_unthrottle(void *handle, int backlog)
{
//    Serial sp = (Serial) handle;
//    sk_serial_set_frozen((Socket)sp->s, backlog > SERIAL_MAX_BACKLOG);
}

static int serial_ldisc(void *handle, int option)
{
//    if (option == LD_EDIT || option == LD_ECHO)
//		return 1;
	/* 
	 * Most serial devices don't favor local editing or echo, so
	 * we just try to avoid using it.
	 */
    return 0;
}

static void serial_provide_ldisc(void *handle, void *ldisc)
{
    /* This is a stub. */
}

static void serial_provide_logctx(void *handle, void *logctx)
{
    /* This is a stub. */
}

static int serial_exitcode(void *handle)
{
    Serial sp = (Serial) handle;
    if (sp->s != NULL)
	return -1;					/* still connected */
    else
    /* Exit codes are a meaningless concept in the Serial protocol */
	return 0;
}

void do_serial_event(int event) 
{
};

/* 
 * We need to poll our port's events every time we want to get something or send something.
 * AND, important thing: we have static pointer to serial socket, so now we cannot open more
 * than one serial port in one instance of PuTTY. I'm going to change this later. Somehow.
 */

#define BUFSIZE 512

int do_receive(Serial_Socket s)
{
    static char buffer[BUFSIZE];
    DWORD err;
    COMSTAT comstat;
    int i, j, nread = 0, result = 0;

    ClearCommError(s->port, &err, &comstat);
    while (comstat.cbInQue) {
        i = comstat.cbInQue;
        while (i) {
    	    if (i > BUFSIZE) {
				j = BUFSIZE;
				i -= j;
			} else {
				j = i;
				i = 0;
			};
			if (!ReadFile(s->port, buffer, j, &nread, &s->o_reader)) {
				err = GetLastError();
				if (err != ERROR_IO_PENDING) {
					s->pending_error = err;
					s->error = serial_error(err);
					return result;
				} else {
					GetOverlappedResult(s->port, &s->o_reader, &nread, TRUE);
					ResetEvent(s->o_reader.hEvent);
				};
			};
			if (nread) {
				result += nread;
				bufchain_add(&s->input_data, buffer, nread);
			};
		};
		ClearCommError(s->port, &err, &comstat);
    };
    return result;
};

VOID CALLBACK tick_timer(HWND hwnd, UINT msg, UINT_PTR event, DWORD time) {
    switch (s->state) {
    case SERIAL_STATE_DIALING:
	s->protstate.dialing.timeout--;
	break;
    };
};

void update_dialing_rules(Config *cfg);
static int CALLBACK ProgressProc(HWND hwnd, UINT msg,
				 WPARAM wParam, LPARAM lParam);
HWND progress_dlg;
static HWND progress_text;
static HWND progress_bar;

/*
 * Dialing state machine. Gets characters one by one,
 * processes them and if there was some results, returns
 * TRUE. Otherwise returns FALSE;
 */
 
static int process_dialing(Serial_Socket s, char c) {
    static char string[512];
    static int sptr = 0;
    char sep[] = "\r\n";
    char number[50];
    char *in, *out, *text, *str = NULL, *lf = NULL;
    int len = 0, i = 0;

    if (!s)
	return FALSE;

    if (s->protstate.dialing.state == DIAL_STATE_NONE)
	return FALSE;

    /*
     * First of all, check for timeout or DIAL_STATE_CANCELED
     * meaning the user pressed Cancel button.
     * If any of those conditions exist, abort the operation.
     */
    if (s->protstate.dialing.state != DIAL_STATE_CONNECTED &&
	((s->protstate.dialing.state == DIAL_STATE_CANCELED &&
	  s->protstate.dialing.response == DIAL_RESPONSE_ABORT) ||
	(s->protstate.dialing.state > DIAL_STATE_INIT &&
	 s->protstate.dialing.state < DIAL_STATE_RETRYING &&
	 s->protstate.dialing.timeout <= 0))) {
	EscapeCommFunction(s->port, CLRDTR);
	Sleep(SERIAL_WAIT_TIMEOUT * 20);
	EscapeCommFunction(s->port, SETDTR);
	Sleep(SERIAL_WAIT_TIMEOUT * 20);
	out = dupprintf("%s\r\n", s->cfg->ser_modem_hangup);
	serial_send(s->plug, out, strlen(out));
	text = dupprintf("Hanging up:\n%s", out);
	sfree(out);
	if (s->protstate.dialing.timer)
	    KillTimer(NULL, s->protstate.dialing.timer);
	s->protstate.dialing.timer = 0;
	s->protstate.dialing.timeout = 0;
	if (s->protstate.dialing.state != DIAL_STATE_CANCELED) {
	    s->protstate.dialing.state = DIAL_STATE_ABORTED;
	    s->protstate.dialing.response = DIAL_RESPONSE_TIMEOUT;
	} else
	    s->protstate.dialing.response = DIAL_RESPONSE_NONE;
	if (progress_text)
	    SetWindowText(progress_text, text);
	sfree(text);
	if (progress_bar)
	    SendMessage(progress_bar, PBM_SETPOS, (WPARAM)0, 0);
	return TRUE;
    };

    switch (s->protstate.dialing.state) {
    case DIAL_STATE_NONE:
	return FALSE;
    case DIAL_STATE_INIT:
	{
	    char buf[512];

	    if (s->cfg->wintitle[0] != '\0') {
		sprintf(buf, "%s (Dialing)", s->cfg->wintitle);
	    } else {
		sprintf(buf, "%s - %s (Dialing)", s->cfg->host, appname);
	    };
	    SetWindowText(hwnd, buf);
	    Sleep(10);
	};
    case DIAL_STATE_RETRYING:
	/*
	 * Create progress dialog.
	 */
	if (!progress_dlg) {
//	    EnableWindow(hwnd, FALSE);
	    progress_dlg = CreateDialogParam(hinst, MAKEINTRESOURCE(IDD_PROGRESSBOX),
					     hwnd, ProgressProc, (LPARAM)s);
	    SetWindowText(progress_dlg, "Dialing progress");
	    progress_text = GetDlgItem(progress_dlg, IDC_PROGRESSTEXT);
	    text = dupprintf("Initializing modem:\n%s", s->cfg->ser_modem_init);
	    SetWindowText(progress_text, text);
	    sfree(text);
	    progress_bar = GetDlgItem(progress_dlg, IDC_PROGRESSBAR);
	    SendMessage(progress_bar, PBM_SETRANGE, 0, MAKELPARAM(0, 2));
	    SendMessage(progress_bar, PBM_SETSTEP, (WPARAM) 1, 0);
	    ShowWindow(progress_bar, SW_HIDE);
	    ShowWindow(progress_dlg, SW_SHOWNORMAL);
//	    EnableWindow(progress_dlg, TRUE);
//	    SetActiveWindow(progress_dlg);
	} else {
	    SetWindowText(progress_dlg, "Dialing progress");
	    text = dupprintf("Initializing modem:\n%s", s->cfg->ser_modem_init);
	    SetWindowText(progress_text, text);
	    sfree(text);
	};
	/*
	 * Start dialing. First, initialize the modem by sending
	 * it init command string. By default it is "ATZ".
	 */
	s->state = SERIAL_STATE_DIALING;
	s->protstate.dialing.state = DIAL_STATE_INIT;
	s->protstate.dialing.response = DIAL_RESPONSE_NONE;
	s->protstate.dialing.timeout = s->cfg->ser_dialtimeout;
	s->protstate.dialing.timer = SetTimer(NULL, 0, 1000, 
	    (TIMERPROC) tick_timer);
	out = dupprintf("%s\r\n", s->cfg->ser_modem_init);
	serial_send(s->plug, out, strlen(out));
	sfree(out);
	s->protstate.dialing.state = DIAL_STATE_INITRESPONSE;
	s->protstate.dialing.attempt++;
	return FALSE;
	break;
    case DIAL_STATE_INITRESPONSE:
	/*
	 * Got some response to init string.
	 * It can be either "OK" meaning that modem passed
	 * initialization correctly and is ready to accept
	 * dialing command, or it can be "ERROR" meaning
	 * the command was incorrect.
	 * If the modem is Hayes-compatible, that is.
	 */
	if (c != '\r' && c != '\n') {
	    string[sptr] = c;
	    sptr++;
	    return FALSE;
	} else {
	    if (sptr == 0)
		return FALSE;
	};
	string[sptr] = '\0';
	in = strupr(string);
	text = dupprintf("Initializing modem:\n%s", in);
	SetWindowText(progress_text, text);
	sfree(text);
	if (strstr(in, s->cfg->ser_modem_ok) == in) {
	/*
	 * Modem said OK, let's dial the number.
	 * Dialing string by default is "ATDT" for tone
	 * and "ATDP" for pulse.
	 */
	    s->protstate.dialing.response = DIAL_RESPONSE_OK;
	    if (s->cfg->ser_usewinloc)
		update_dialing_rules(s->cfg);
    	    out = dupprintf("%s%s%s\r\n",
	    		s->cfg->ser_dialmode ?
			    s->cfg->ser_modem_dial_pulse :
			    s->cfg->ser_modem_dial_tone,
			s->cfg->ser_dialprefix,
			s->cfg->host);
	    serial_send(s->plug, out, strlen(out));
	    if (s->cfg->ser_dialprefix[0] != '\0')
	        sprintf(number, "%s %s", s->cfg->ser_dialprefix, s->cfg->host);
	    else
	        sprintf(number, "%s", s->cfg->host);
	    text = dupprintf("Dialing number:\n%s", number);
	    sfree(out);
	    s->protstate.dialing.state = DIAL_STATE_DIALINGRESPONSE;
	    SetWindowText(progress_text, text);
	    sfree(text);
	    SendMessage(progress_bar, PBM_STEPIT, 0, 0);
	    sptr = 0;
	    return TRUE;
	} else if (strstr(in, s->cfg->ser_modem_error) == in) {
	    /*
	     * Modem said it didn't understand our command.
	     * We cannot correct this situation, so just abort.
	     */
	    s->protstate.dialing.response = DIAL_RESPONSE_ERROR;
	    if (s->protstate.dialing.timer)
	        KillTimer(NULL, s->protstate.dialing.timer);
	    s->protstate.dialing.timer = 0;
	    s->protstate.dialing.timeout = 0;
	    SendMessage(progress_bar, PBM_SETPOS, (WPARAM)0, 0);
	    sptr = 0;
	    return TRUE;
	};
	/*
	 * Nothing criminal, probably just the local echo from our
	 * previous command. Ignore it.
	 */
	sptr = 0;
	break;
    case DIAL_STATE_DIALINGRESPONSE:
	/*
	 * In previous state we dialed the number and now we got response
	 * from the modem. It can vary, but the common choices are:
	 * CONNECT: meaning we're connected all right;
	 * BUSY: meaning the remote side is busy;
	 * NO CARRIER: meaning either remote side is not answering or
	 * it answered but we couldn't connect because of poor line
	 * condition (or because someone answered with voice or fax not modem :);
	 * NO DIALTONE: meaning modem couln't even dial the number because
	 * it either not connected to the phone line or the line is broken.
	 * Some modems say "NO DIAL TONE" instead of "NO DIALTONE", that's
	 * why the modem strings are configurable. :)
	 * And, of course, we can get an "ERROR" response meaning that our
	 * dialing string was incorrect.
	 */
	if (c != '\r' && c != '\n') {
	    string[sptr] = c;
	    sptr++;
	    return FALSE;
	} else {
	    if (sptr == 0)
		return FALSE;
	};
	string[sptr] = '\0';
	in = strupr(string);
	text = dupprintf("Dialing number:\n%s", in);
        SetWindowText(progress_text, text);
        sfree(text);
        if (strstr(in, s->cfg->ser_modem_connect) == in) {
	    /*
	     * We're connected, hooray!
	     */
	    s->protstate.dialing.state = DIAL_STATE_CONNECTED;
	    s->protstate.dialing.response = DIAL_RESPONSE_CONNECT;
	    if (s->protstate.dialing.timer)
	        KillTimer(NULL, s->protstate.dialing.timer);
	    s->protstate.dialing.timer = 0;
	    s->protstate.dialing.timeout = 0;
	    SendMessage(progress_bar, PBM_STEPIT, 0, 0);
	    sptr = 0;
	    return TRUE;
	} else if (strstr(in, s->cfg->ser_modem_busy) == in) {
	    /*
	     * Remote side is busy.
	     */
	    s->protstate.dialing.state = DIAL_STATE_ABORTED;
	    s->protstate.dialing.response = DIAL_RESPONSE_BUSY;
	    if (s->protstate.dialing.timer)
	        KillTimer(NULL, s->protstate.dialing.timer);
	    s->protstate.dialing.timer = 0;
	    s->protstate.dialing.timeout = 0;
	    SendMessage(progress_bar, PBM_SETPOS, (WPARAM)0, 0);
	    sptr = 0;
	    return TRUE;
	} else if (strstr(in, s->cfg->ser_modem_no_carrier) == in) {
	    /*
	     * Couldn't connect.
	     */
	    s->protstate.dialing.state = DIAL_STATE_ABORTED;
	    s->protstate.dialing.response = DIAL_RESPONSE_NO_CARRIER;
	    if (s->protstate.dialing.timer)
	        KillTimer(NULL, s->protstate.dialing.timer);
	    s->protstate.dialing.timer = 0;
	    s->protstate.dialing.timeout = 0;
	    SendMessage(progress_bar, PBM_SETPOS, (WPARAM)0, 0);
	    sptr = 0;
	    return TRUE;
	} else if (strstr(in, s->cfg->ser_modem_no_dialtone) == in) {
	    /*
	     * Couldn't even dial.
	     */
	    s->protstate.dialing.state = DIAL_STATE_ABORTED;
	    s->protstate.dialing.response = DIAL_RESPONSE_NO_DIALTONE;
	    if (s->protstate.dialing.timer)
	        KillTimer(NULL, s->protstate.dialing.timer);
	    s->protstate.dialing.timer = 0;
	    s->protstate.dialing.timeout = 0;
	    SendMessage(progress_bar, PBM_SETPOS, (WPARAM)0, 0);
	    sptr = 0;
	    return TRUE;
	} else if (strstr(in, s->cfg->ser_modem_error) == in) {
	    /*
	     * Incorrect dial string.
	     */
	    s->protstate.dialing.state = DIAL_STATE_ABORTED;
	    s->protstate.dialing.response = DIAL_RESPONSE_ERROR;
	    if (s->protstate.dialing.timer)
	        KillTimer(NULL, s->protstate.dialing.timer);
	    s->protstate.dialing.timer = 0;
	    s->protstate.dialing.timeout = 0;
	    SendMessage(progress_bar, PBM_SETPOS, (WPARAM)0, 0);
	    sptr = 0;
	    return TRUE;
	};
	sptr = 0;
    };
    return FALSE;
};

static int xymodem(Serial_Socket s);
static int zmodem(Serial_Socket s);
static int kermit(Serial_Socket s);

#define ATTEMPT_TIMER 1

extern void do_serial_processing(void)
{
    char *text;
    HWND button;
    DWORD res, err, dummy;
    static DWORD commevent = 0;
    int nread = 0, nwrite = 0, i = 0, j = 0, nproc = 0;
    COMSTAT comstat;
    HANDLE events[2] = { s->o_status.hEvent, s->o_writer.hEvent };
    char buf[512];


    if (!s->pending_error && !s->pending_status) {
	if (!WaitCommEvent(s->port, &commevent, &s->o_status)) {
	    err = GetLastError();
	    if (err != ERROR_IO_PENDING) {
		s->writable = FALSE;
		s->pending_status = FALSE;
		s->pending_error = err;
		s->error = serial_error(err);
	    } else {
		s->pending_status = TRUE;
	    };
	} else {
	    switch (commevent)
	    {
	    case EV_RXCHAR:
		nread = do_receive(s);
		break;
	    };
	    s->pending_status = FALSE;
	};
    };

    if (s->pending_error) {
	/* Just clear the error and do nothing more. Yet. */
	ClearCommError(s->port, &err, &comstat);
	s->pending_error = FALSE;
	if (s->error)
	    sfree(s->error);
    };

    if (!nread) {
	res = WaitForMultipleObjects(2, events, FALSE, SERIAL_WAIT_TIMEOUT);
	switch (res)
	{
	case WAIT_OBJECT_0:
    	    GetOverlappedResult(s->port, &s->o_status, &dummy, FALSE);
	    /*
	     * Note that we're switching the same variable that was used in
	     * WaitCommEvent call, that's why I defined it as static.
	     * I don't know why we can't get incoming event with GetOverlappedResult
	     * call, but here it is: just another Windows quirk. I noted this
	     * because GetOverlappedResult after WaitCommEvent will return an undefined value
	     * in dummy, in my case it was always EV_TXEMPTY and I spent a couple of
	     * days debugging this tiny nasty condition. Please pay attention.
	     */
	    switch (commevent)
	    {
		case EV_BREAK:
		case EV_ERR:
		    /*
		     * An error occured. Just clear it, for starters.
		     */
		    ClearCommError(s->port, &err, &comstat);
		    break;
		case EV_RLSD:
		    /*
		     * In normal terms this signal is called CD, means
		     * "Carrier Detect". It is used to report whether
		     * the modem is connected to the remote side or not.
		     * So if we receive this event, it means either that
		     * connection is established or that it is dropped.
		     * We only interested in latter for now.
		     */
		    {
			DWORD ms;
			char buf[512];

			if (GetCommModemStatus(s->port, &ms)) {
			    if (ms & MS_RLSD_ON) {
				if (s->cfg->wintitle[0] != '\0')
				    sprintf(buf, "%s (Connected)", s->cfg->wintitle);
				else
				    sprintf(buf, "%s - %s (Connected)", s->cfg->host, appname);
				SetWindowText(hwnd, buf);
			    } else {
				HMENU m = GetSystemMenu(hwnd, FALSE);
				EnableMenuItem(m, 0x330, MF_BYCOMMAND | MF_ENABLED);
				s->state = SERIAL_STATE_NOT_CONNECTED;
				if (s->cfg->wintitle[0] != '\0')
				    sprintf(buf, "%s (Disconnected)", s->cfg->wintitle);
				else
				    sprintf(buf, "%s - %s (Disconnected)", s->cfg->host, appname);
				SetWindowText(hwnd, buf);
			    };
			};
		    };
		    break;
		case EV_RXCHAR:
		    nread = do_receive(s);
		    break;
	    };
	    s->pending_status = FALSE;
	    break;
	case WAIT_OBJECT_0 + 1:
	    SetLastError(ERROR_SUCCESS);
	    GetOverlappedResult(s->port, &s->o_writer, &nwrite, FALSE);
	    if (nwrite) {
		s->writable = TRUE;
		s->pending_write = FALSE;
	    };
	    ResetEvent(s->o_writer.hEvent);
	    break;
	};
    };

    /*
     * Trying to send if something is in output buffer and
     * port is writable.
     */

    if (s->writable && bufchain_size(&s->output_data))
	serial_try_send(s);

    /*
     * Check for timeouts and process if necessary.
     */

    switch (s->state) {
    case SERIAL_STATE_DIALING:
	if (s->protstate.dialing.state != DIAL_STATE_CANCELED &&
	    s->protstate.dialing.timeout <= 0) {
	    if (process_dialing(s, '\0') && 
		s->protstate.dialing.state == DIAL_STATE_ABORTED &&
		s->protstate.dialing.response == DIAL_RESPONSE_TIMEOUT) {
    		/*
		 * Operation is timed out, probably modems'
		 * handshake is taking too long because of poor
		 * line condition.
		 * Ask user whether to retry or not.
		 */
		if (s->protstate.dialing.attempt >= s->cfg->ser_redials) {
		    s->state = SERIAL_STATE_NOT_CONNECTED;
		    if (s->protstate.dialing.timer)
			KillTimer(NULL, s->protstate.dialing.timer);
		    s->protstate.dialing.timer = 0;
		    s->protstate.dialing.timeout = 0;
		    s->protstate.dialing.state = DIAL_STATE_NONE;
		    s->protstate.dialing.response = DIAL_RESPONSE_NONE;
		    s->protstate.dialing.attempt = 0;
		    if (progress_dlg)
			DestroyWindow(progress_dlg);
		    progress_dlg = 0;
		    progress_text = 0;
		    progress_bar = 0;
//		    EnableWindow(hwnd, TRUE);
		    SetFocus(hwnd);
		    break;
		};
		SetWindowText(progress_dlg, "Dialing timed out");
		text = dupprintf("Dialing operation timed out without connect acknowledge.\n"
				 "Should we try to redial?");
		SetWindowText(progress_text, text);
		sfree(text);
		SendMessage(progress_bar, PBM_SETPOS, (WPARAM)0, 0);
		button = GetDlgItem(progress_dlg, IDOK);
		SetWindowText(button, "OK");
		ShowWindow(button, SW_SHOW);
		button = GetDlgItem(progress_dlg, IDHELP);
		ShowWindow(button, SW_SHOW);
    		SetTimer(progress_dlg, (UINT_PTR)ATTEMPT_TIMER, 
			 1000, NULL);
		s->protstate.dialing.timeout = s->cfg->ser_redialtimeout;
	    };
	};
	break;
    case SERIAL_STATE_XMODEMUPLOAD:
    case SERIAL_STATE_XMODEMDOWNLOAD:
	if (s->protstate.xmodem.timeout <= 0)
	    xymodem(s);
	break;
    case SERIAL_STATE_YMODEMUPLOAD:
    case SERIAL_STATE_YMODEMDOWNLOAD:
	if (s->protstate.ymodem.timeout <= 0)
	    xymodem(s);
	break;
    case SERIAL_STATE_ZMODEMUPLOAD:
    case SERIAL_STATE_ZMODEMDOWNLOAD:
	if (s->protstate.zmodem.timeout <= 0)
	    zmodem(s);
	break;
    case SERIAL_STATE_KERMITUPLOAD:
    case SERIAL_STATE_KERMITDOWNLOAD:
	if (s->protstate.kermit.timeout <= 0)
	    kermit(s);
	break;
    };

    if (!nread &&
	!(s->state == SERIAL_STATE_DIALING &&
	  s->protstate.dialing.state == DIAL_STATE_CANCELED &&
	  s->protstate.dialing.response == DIAL_RESPONSE_NONE))
	return;

    /* 
     * So now if we have something in the input_data bufchain, let's
     * process it. There's a big state machine in here... ;)
     */
    switch (s->state) {
    case SERIAL_STATE_NONE:
    case SERIAL_STATE_INITIALIZING:
    case SERIAL_STATE_RECONFIGURING:
	/* 
	 * That should be impossible! 
	 */
	fatalbox("Internal error: impossible condition!");
	break;
    case SERIAL_STATE_NOT_CONNECTED:
    case SERIAL_STATE_CONNECTED:
	/*
	 * It doesn't matter really whether we're connected or not:
	 * just send and receive characters back and forth between
	 * the terminal and the port. Something connected to the port
	 * should answer.
	 */
	terminal_write(s->plug, nread);
	break;
    case SERIAL_STATE_DIALING:
	if (s->protstate.dialing.state == DIAL_STATE_CANCELED &&
	    s->protstate.dialing.response == DIAL_RESPONSE_NONE) {
	    /*
	     * Canceled by user, just clean up and exit;
	     */
	    s->state = SERIAL_STATE_NOT_CONNECTED;
	    if (progress_dlg)
		DestroyWindow(progress_dlg);
	    progress_dlg = 0;
	    progress_text = 0;
	    progress_bar = 0;
//	    EnableWindow(hwnd, TRUE);
	    SetFocus(hwnd);
	    if (s->protstate.dialing.timer)
		KillTimer(NULL, s->protstate.dialing.timer);
	    s->protstate.dialing.timer = 0;
	    s->protstate.dialing.timeout = 0;
	    s->protstate.dialing.attempt = 0;
	    s->protstate.dialing.state = DIAL_STATE_NONE;
	    s->protstate.dialing.response = DIAL_RESPONSE_NONE;
//	    break;
	};
	while (j = bufchain_size(&s->input_data)) {

	    if (j > 512)
		j = 512;

	    bufchain_fetch(&s->input_data, (void *)buf, j);

	    for (i = 0; i < j; i++) {
		if (process_dialing(s, buf[i])) {
		    switch (s->protstate.dialing.state) {
		    case DIAL_STATE_CONNECTED:
			s->state = SERIAL_STATE_CONNECTED;
			if (progress_dlg) {
    			    DestroyWindow(progress_dlg);
			    progress_dlg = 0;
			    progress_text = 0;
			    progress_bar = 0;
//			    EnableWindow(hwnd, TRUE);
			    SetFocus(hwnd);
			};
			break;
		    case DIAL_STATE_ABORTED:
			switch (s->protstate.dialing.response) {
			case DIAL_RESPONSE_NO_CARRIER:
			    /*
			     * Remote side is not answering. Redial?
			     */
			    if (s->protstate.dialing.attempt >= s->cfg->ser_redials) {
				s->state = SERIAL_STATE_NOT_CONNECTED;
				if (s->protstate.dialing.timer)
				    KillTimer(NULL, s->protstate.dialing.timer);
				s->protstate.dialing.timer = 0;
				s->protstate.dialing.timeout = 0;
				s->protstate.dialing.state = DIAL_STATE_NONE;
				s->protstate.dialing.response = DIAL_RESPONSE_NONE;
				s->protstate.dialing.attempt = 0;
				if (progress_dlg)
				    DestroyWindow(progress_dlg);
				progress_dlg = 0;
				progress_text = 0;
				progress_bar = 0;
//				EnableWindow(hwnd, TRUE);
				SetFocus(hwnd);
				break;
			    };
			    SetWindowText(progress_dlg, "Dialing error");
			    text = dupprintf("It seems that remote side is not answering our call.\n"
					     "Should we try to redial?");
			    SetWindowText(progress_text, text);
			    sfree(text);
			    SendMessage(progress_bar, PBM_SETPOS, (WPARAM)0, 0);
			    button = GetDlgItem(progress_dlg, IDOK);
			    ShowWindow(button, SW_SHOW);
			    button = GetDlgItem(progress_dlg, IDHELP);
			    ShowWindow(button, SW_SHOW);
			    SetTimer(progress_dlg, (UINT_PTR)ATTEMPT_TIMER, 1000, NULL);
			    s->protstate.dialing.timeout = s->cfg->ser_redialtimeout;
			    break;
			case DIAL_RESPONSE_BUSY:
			    /*
			     * Remote side is busy. Redial?
			     */
			    if (s->protstate.dialing.attempt >= s->cfg->ser_redials) {
				s->state = SERIAL_STATE_NOT_CONNECTED;
				if (s->protstate.dialing.timer)
				    KillTimer(NULL, s->protstate.dialing.timer);
				s->protstate.dialing.timer = 0;
				s->protstate.dialing.timeout = 0;
				s->protstate.dialing.state = DIAL_STATE_NONE;
				s->protstate.dialing.response = DIAL_RESPONSE_NONE;
				s->protstate.dialing.attempt = 0;
				if (progress_dlg)
				    DestroyWindow(progress_dlg);
				progress_dlg = 0;
				progress_text = 0;
				progress_bar = 0;
//				EnableWindow(hwnd, TRUE);
				SetFocus(hwnd);
				break;
			    };
			    SetWindowText(progress_dlg, "Dialing error");
			    text = dupprintf("It seems that remote side is busy with someone else's call.\n"
					     "Should we try to redial?");
			    SetWindowText(progress_text, text);
			    sfree(text);
			    SendMessage(progress_bar, PBM_SETPOS, (WPARAM)0, 0);
			    button = GetDlgItem(progress_dlg, IDOK);
			    SetWindowText(button, "OK");
			    ShowWindow(button, SW_SHOW);
			    button = GetDlgItem(progress_dlg, IDHELP);
			    ShowWindow(button, SW_SHOW);
			    SetTimer(progress_dlg, (UINT_PTR)ATTEMPT_TIMER, 1000, NULL);
			    s->protstate.dialing.timeout = s->cfg->ser_redialtimeout;
			    break;
			case DIAL_RESPONSE_NO_DIALTONE:
			    /*
			     * No dial tone in the line. It's futile to try again.
			     * Provide message for the user and abort.
			     */
			    SetWindowText(progress_dlg, "Dialing error");
			    text = dupprintf("Modem reported absence of dial tone.\n"
					     "Please check modem line.");
			    SetWindowText(progress_text, text);
			    sfree(text);
			    SendMessage(progress_bar, PBM_SETPOS, (WPARAM)0, 0);
			    s->state = SERIAL_STATE_NOT_CONNECTED;
			    s->protstate.dialing.state = DIAL_STATE_NONE;
			    s->protstate.dialing.response = DIAL_RESPONSE_NONE;
			    s->protstate.dialing.attempt = 0;
			    s->protstate.dialing.timeout = 0;
			    if (s->protstate.dialing.timer)
				KillTimer(NULL, s->protstate.dialing.timer);
			    break;
			case DIAL_RESPONSE_ERROR:
			    /*
			     * Modem said it doesn't understand our commands.
			     * Abort the operation and let user check some
			     * settings before trying again.
			     */
			    SetWindowText(progress_dlg, "Dialing error");
			    text = dupprintf("Modem reported that it didn't understand "
					     "our dialing command.\n"
					     "Please check modem settings.");
			    SetWindowText(progress_text, text);
			    sfree(text);
			    SendMessage(progress_bar, PBM_SETPOS, (WPARAM)0, 0);
			    s->state = SERIAL_STATE_NOT_CONNECTED;
			    s->protstate.dialing.state = DIAL_STATE_NONE;
			    s->protstate.dialing.response = DIAL_RESPONSE_NONE;
			    s->protstate.dialing.attempt = 0;
			    s->protstate.dialing.timeout = 0;
			    if (s->protstate.dialing.timer)
				KillTimer(NULL, s->protstate.dialing.timer);
			    break;
			};
		    case DIAL_STATE_INITRESPONSE:
			if (s->protstate.dialing.response == DIAL_RESPONSE_ERROR) {
			    SetWindowText(progress_dlg, "Modem initialization error");
			    text = dupprintf("Modem reported that it didn't understand our "
					     "initialization command.\n"
					     "Please check modem settings.");
			    SetWindowText(progress_text, text);
			    sfree(text);
			    SendMessage(progress_bar, PBM_SETPOS, (WPARAM)0, 0);
			    s->state = SERIAL_STATE_NOT_CONNECTED;
			    s->protstate.dialing.state = DIAL_STATE_NONE;
			    s->protstate.dialing.response = DIAL_RESPONSE_NONE;
			    s->protstate.dialing.attempt = 0;
			    s->protstate.dialing.timeout = 0;
			    if (s->protstate.dialing.timer)
				KillTimer(NULL, s->protstate.dialing.timer);
			};
			break;
		    case DIAL_STATE_DIALINGRESPONSE:
			if (s->protstate.dialing.response == DIAL_RESPONSE_ERROR) {
			    SetWindowText(progress_dlg, "Dialing error");
			    text = dupprintf("Modem reported that it didn't understand our"
					     " dialing command.\n"
					     "Please check modem settings.");
			    SetWindowText(progress_text, text);
			    sfree(text);
			    SendMessage(progress_bar, PBM_SETPOS, (WPARAM)0, 0);
			    s->state = SERIAL_STATE_NOT_CONNECTED;
			    s->protstate.dialing.state = DIAL_STATE_NONE;
			    s->protstate.dialing.response = DIAL_RESPONSE_NONE;
			    s->protstate.dialing.attempt = 0;
			    s->protstate.dialing.timeout = 0;
			    if (s->protstate.dialing.timer)
				KillTimer(NULL, s->protstate.dialing.timer);
			};
			break;
		    };
    		    {
			HMENU m = GetSystemMenu(hwnd, FALSE);
			EnableMenuItem(m, 0x330, MF_BYCOMMAND | 
			 	       (s->state == SERIAL_STATE_CONNECTED)
				        ? MF_GRAYED : MF_ENABLED);
		    };
		};
		/*
		 * We're in process. Check the setting and write
		 * input from the port to the terminal, if needed.
		 * Otherwise, just consume one char.
		 */
	    if (s->cfg->ser_print_when_dialing)
		terminal_write(s->plug, 1);
	    else
		bufchain_consume(&s->input_data, 1);
	    };
	};
	break;
    case SERIAL_STATE_XMODEMUPLOAD:
    case SERIAL_STATE_XMODEMDOWNLOAD:
    case SERIAL_STATE_YMODEMUPLOAD:
    case SERIAL_STATE_YMODEMDOWNLOAD:
	xymodem(s);
	break;
    case SERIAL_STATE_ZMODEMUPLOAD:
    case SERIAL_STATE_ZMODEMDOWNLOAD:
	zmodem(s);
	break;
    case SERIAL_STATE_KERMITUPLOAD:
    case SERIAL_STATE_KERMITDOWNLOAD:
	kermit(s);
	break;
    };
};

void serial_cleanup(void) {
    sk_serial_close((Socket)s);
};

Backend serial_backend = {
    serial_init,
    serial_free,
    serial_reconfig,
    serial_send,
    serial_sendbuffer,
    serial_size,
    serial_special,
    serial_get_specials,
    serial_socket,
    serial_exitcode,
    serial_sendok,
    serial_ldisc,
    serial_provide_ldisc,
    serial_provide_logctx,
    serial_unthrottle,
    1
};

/*
 * Serial file up/download protocols: X/Ymodem.
 */

#define SOH 0x01
#define STX 0x02
#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define CAN 0x18
#define CRC 0x43
#define YMG 0x47

static int xymodem(Serial_Socket s) {
    return 0;
};

static int zmodem(Serial_Socket s) {
    return 0;
};

static int kermit(Serial_Socket s) {
    return 0;
};

#define CURRENTLOCATIONROOT "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Locations"
#define CURRENTLOCATIONVALUE "CurrentID"
#define EXACTLOCATIONROOT "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Locations\\Location%d"
#define DIALINGMODE "Flags"
#define DIALINGPREFIX "OutsideAccess"

void update_dialing_rules(Config *cfg) {
    /*
     * Here we peek into Windows registry and update some config
     * settings based on registry settings.
     * Currently these are:
     * cfg->ser_dialmode -- dialing mode: tone/pulse
     * cfg->ser_dialprefix -- prefix to get access to outside line
     */
    HKEY key;
    DWORD location = 0, size = 0, type = 0, dialmode = 0;
    char path[255], dialprefix[10] = {0};

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, CURRENTLOCATIONROOT, 0,
	KEY_READ, &key) != ERROR_SUCCESS)
	return; /* Something is wrong. */

    size = sizeof(location);
    if (RegQueryValueEx(key, CURRENTLOCATIONVALUE,
	NULL, &type, (LPBYTE)&location, &size) != ERROR_SUCCESS ||
	size != sizeof(location) || type != REG_DWORD)
	goto _return;

    RegCloseKey(key);

    sprintf(path, EXACTLOCATIONROOT, location);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, path, 0,
	KEY_READ, &key) != ERROR_SUCCESS)
	return; /* Something is wrong again. */

    size = sizeof(dialmode);
    if (RegQueryValueEx(key, DIALINGMODE,
	NULL, &type, (LPBYTE)&dialmode, &size) != ERROR_SUCCESS ||
	size != sizeof(dialmode) || type != REG_DWORD)
	goto _return;

    cfg->ser_dialmode = !dialmode;

    size = sizeof(dialprefix);
    if (RegQueryValueEx(key, DIALINGPREFIX,
	NULL, &type, (LPBYTE)&dialprefix, &size) != ERROR_SUCCESS ||
	size > sizeof(dialprefix) || type != REG_SZ)
	goto _return;
	
    strcpy(cfg->ser_dialprefix, dialprefix);

_return:
    RegCloseKey(key);
};


static int CALLBACK ProgressProc(HWND hwnd, UINT msg,
				 WPARAM wParam, LPARAM lParam)
{
    static Serial_Socket s;
    char *text;
    HWND button;

    switch (msg) {
    case WM_INITDIALOG:
	s = (Serial_Socket)lParam;
	return TRUE;
    case WM_COMMAND:
	switch (LOWORD(wParam)) {
	    case IDOK:
		if (s->state == SERIAL_STATE_NOT_CONNECTED &&
		    s->protstate.dialing.state == DIAL_STATE_CANCELED)
		    return 0;
		s->state = SERIAL_STATE_DIALING;
		s->protstate.dialing.state = DIAL_STATE_RETRYING;
		s->protstate.dialing.response = DIAL_RESPONSE_NONE;
		process_dialing(s, '\0');
		button = GetDlgItem(hwnd, IDOK);
		ShowWindow(button, SW_HIDE);
		button = GetDlgItem(hwnd, IDHELP);
		ShowWindow(button, SW_HIDE);
		return TRUE;
	    case IDCANCEL:
		if (s->protstate.dialing.state > DIAL_STATE_NONE) {
		    s->state = SERIAL_STATE_DIALING;
		    s->protstate.dialing.state = DIAL_STATE_CANCELED;
		    s->protstate.dialing.response = DIAL_RESPONSE_ABORT;
		    process_dialing(s, '\0');
		};
		EndDialog(hwnd, 1);
		return TRUE;
	    case IDHELP:
		/*
		 * Do nothing now.
		 */
		return TRUE;
	}
	return FALSE;
    case WM_TIMER:
	text = dupprintf("OK (%d sec)", s->protstate.dialing.timeout);
	button = GetDlgItem(hwnd, IDOK);
	SetWindowText(button, text);
	sfree(text);
	s->protstate.dialing.timeout--;
	if (s->protstate.dialing.timeout < 0) {
	    KillTimer(hwnd, (UINT_PTR)ATTEMPT_TIMER);
	    s->protstate.dialing.timeout = 0;
	    s->protstate.dialing.timer = 0;
	    SendMessage(hwnd, WM_COMMAND, (WPARAM)IDOK, 0);
	};
	return TRUE;
    case WM_CLOSE:
	EndDialog(hwnd, 1);
	return TRUE;
    case WM_NOTIFY:
	return FALSE;
    }
    return FALSE;
}
#endif /* SERIAL_BACKEND */
