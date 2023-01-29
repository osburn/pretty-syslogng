pretty
======
This program written in C takes syslog-ng formatted text from syslogngfile.log via a pipe (stdin) and allows you to display it specifying a timezone and pretties it up a little.

How to create
-------------
Use the included make file and just type: make
```
make
make clean
```

Environment variable requirements
---------------------------------
```
None needed
```

Libraries install requirements
------------------------------
```
No special libraries needed
```

Command line argument options
-----------------------------
```
./pretty -h
usage: ./pretty [-hlvz] [ZONEINFO]
   -l       List out program timezone specifiers
   -z       Specify time zone to convert too
   -v       Print version
   -h, -?   Print options

./pretty -l

./pretty -z p
```

Example usage
-------------
Ensure the program is in your search path.
```
grep -ais "nrt.*BGP_IO_ERROR_CLOSE_SESSION:" syslogngfile.log | pretty -z nrt

tail -f syslogngfile.log | pretty -z nrt
```

Explanation of options
----------------------
* ./pretty -h  OR ./pretty -?
  * Prints out quick options
* ./pretty -v
  * Prints out author and version
* ./pretty -l
  * Lists out all the programmed in TIMEZONES specifiers that you can use
* ./pretty -z p
  * Convert the piped in syslogng text to PST8PDT timezone


Additional Notes
----------------
Syslog-ng string format the piped in text should be (taken from /etc/syslog-ng/conf.d/syslogngfile.conf)
```
template("${R_ISODATE} ${SOURCEIP} ${HOST} ${PROGRAM}[${PID}]: $MSG\n");
```

To add additional Zones and/or aliases:
* Edit the string array named zonetable.
  * Lines must end with the trailing NULL
  * Last line needs to be a NULL by it's self.
  * If the number of columns exceeds 15, then increase the defined preprocessor macro named TIMEZONECOLUMNS to match.
* Update the version minor number
