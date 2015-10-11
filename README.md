# x11log

x11log is a tiny, simplistic, non-privileged, unobtrusive local/remote
userspace keylogger for X11 sessions. It only requires libX11 and, if
you want remote logging to a HTTP server, libcurl.

 - No root privileges required. It logs all keystrokes, including meta-keys and
   modifiers of the user who is running the application, as long as he/she has 
   an actively running X session. However, it's not necessary to run the logger
   from within the X environment (e.g. a graphical xterminal) - it also works 
   when called via SSH or on a virtual console.

 - Stealthy daemon mode. You may run the logger as background-daemon, with the
   possibility of completely omitting any console output, altering its name
   within the process list, and fully hide given command-line arguments. By
   default, it's renaming itself to a kernel thread.
 
 - Local logging to file
 
 - Remote logging to TCP port. It's easily possible to simply send the keystrokes
   to a specified port on a host on the net. The other end of the connection just 
   needs a program which is actively listening for TCP connections, typically done
   through netcat (e.g. via BSD netcat: nc -l 4000 -k).
 
 - Remote logging to your web-server. Alternatively, you may choose a web-server,
   to which the keystrokes are sent, encapsulated within a HTTP GET request, and 
   "hidden" in a HTTP header field (User-Agent by default).

