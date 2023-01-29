/**
 *
 * Author:	Tim Osburn <tim@osburn.com>
 * Date:	210703
 * Language:	C
 * Purpose:	Convert syslog-ng formatted log lines to a pretty format and convert ISO times to format specified.
 * 	syslog-ng format:
 * 		template("${R_ISODATE} ${SOURCEIP} ${HOST} ${PROGRAM}[${PID}]: $MSG\n");
 *
 * Instructions: To compile
 *			gcc pretty.c -O3 -o pretty -Wall -Werror; strip pretty
 *			OR
 *			clang pretty.c -O3 -o pretty -Wall -Werror; strip pretty
 *
 * Example of usage:
 *			grep -ais "nrt.*BGP_IO_ERROR_CLOSE_SESSION:" syslogngfile.log | pretty -z nrt
 *			tail -f syslogngfile.log | pretty -z nrt
 *			tail -f syslogngfile.log | pretty -z dc13
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>

#define PRETTYVERSION "1.0.1"
#define TIMEZONECOLUMNS 15


time_t parseiso8601utc_fixed(const char *date);
void printzonedata(char *zonetable[][TIMEZONECOLUMNS]);
void validate_zoneinfo(char** string_ptr, char *zonetable[][TIMEZONECOLUMNS]);
void uppercase(char *text);
void truncateatdigits(char *text);


int main(int argc, char *const argv[])
{
	int ch;
	int newtime = 0;
	char syslogsection[4096];
	struct tm tsinfo;
	int tokencount = 0;
	char* line = NULL;
	size_t linecap;

	char* zoneselection = malloc(40);
	// Bailout if unable to allocate memory for zoneselection pointers
	if (zoneselection == NULL) {
		printf("Could not allocate zoneselection memory!");
		exit(1);
	}

	/**
	 * Timezone matrix table, end each line with NULL, last line should also be NULL
	 * 1st entry is the timezone to be used from /usr/share/zoneinfo
	 * remainder entries per line are possible aliases you can use on the CLI for that TZ
	 * If you have more columns then the defined TIMEZONECOLUMNS, you'll need to increase that value.
	 * Case doesn't matter, I just happen to use upper.
	 * Also case at the CLI doesn't matter either, as we convert everything to uppercase before comparing.
	 */

	char *zonetable[][TIMEZONECOLUMNS] = {
	{"UTC", "GMT", "ZULU", "U", NULL},
	{"MST7MDT", "M", NULL},
	{"PST8PDT", "PST", "PDT", "SEA", "SE", "P", "LA", "SV", "SJC", "LAX", "PAO", NULL},
	{"EST5EDT", "EST", "EDT", "NY", "E", "DC", "ATL", "JFK", "BOS", "DTW", "EWR", "GSP", "IAD", "PIT", NULL},
	{"CST6CDT", "CST", "CDT", "DA", "C", "CH", "DA", "DFW", "IAH", "MCI", "ORD", NULL},
	{"Asia/Tokyo", "JST", "JP", "TY", "JAPAN", "NRT", "TOKYO", "J", NULL},
	{"Asia/Hong_Kong", "HK", "HKG", "H", NULL},
	{"Asia/Singapore", "SG", "SIN", "S", NULL},
	{"Asia/Seoul", "SL", NULL},
	{"Asia/Manila", "PH", "MANILA", NULL},
	{"America/Sao_Paulo", "SP", NULL},
	{"America/Toronto", "TR", "YYZ", NULL},
	{"America/Phoenix", "PHX", "PHOENIX", NULL},
	{"Europe/Amsterdam", "AMS", "AM", "AM", "A", NULL},
	{"Europe/London", "LD", "L", NULL},
	{"Europe/Madrid", "MD", "SPAIN", "MADRID", NULL},
	{"Australia/Sydney", "SY", "SYD", "SYDNEY", NULL},
	{"CET", "FR", "FRA", "FRANKFURT", "MRS", "MARSEILLE", "PA", NULL},
	{NULL}
	};


	while ((ch = getopt(argc, argv, "lhvz:")) != -1) {
		switch (ch) {
		case 'l':
			printzonedata(zonetable);
			exit(1);
		case 'z':
			zoneselection = optarg;
			break;
		case 'v':
			fputs("pretty log formatter, version: ", stdout);
			fputs(PRETTYVERSION, stdout);
			fputs("\nCreated by Tim Osburn\n", stdout);
			fputs("210705\n", stdout);
			exit(1);
		case 'h':
		case '?':
			printf("usage: %s [-hlvz] [ZONEINFO]\n", argv[0]);
			fputs("\
   -l       List out program timezone specifiers\n\
   -z       Specify time zone to convert too\n\
   -v       Print version\n\
   -h, -?   Print options\n", stdout);
			exit(1);
		default:
			zoneselection = "UTC";
			printf("No selected zoneinfo, useing %s\n",zoneselection);
		}
	}

	// If nothing was selected default to UTC
	if (strlen(zoneselection) == 0) {
		zoneselection = "UTC";
	}

	uppercase(zoneselection);
	truncateatdigits(zoneselection);
	validate_zoneinfo(&zoneselection, zonetable);
	printf("Timezone: %s\n", zoneselection);

	/**
	 * Parse standardin (from pipe) and splitout each line into tokens
	 * seperated by spaces and get the time field and convert. Then print out.
	 */

	while (getline(&line, &linecap, stdin) != -1) {
		char * token = strtok(line, " ");
		while (token != NULL) {
			tokencount++;
			if (tokencount == 1) {
				setenv("TZ", zoneselection?zoneselection:"UTC", 1);
				tzset();
				newtime = parseiso8601utc_fixed(token);
				time_t rawtime=newtime;
				tsinfo = *localtime(&rawtime);
				strftime(syslogsection, sizeof(syslogsection), "%b %d %H:%M:%S (%Z)", &tsinfo);
				printf("%s ", syslogsection);
				token = strtok(NULL, " ");
			} else if (tokencount == 2) {
				token = strtok(NULL, " ");
				continue;
			} else {
				printf(" %s", token );
				token = strtok(NULL, " ");
			}
		}
		tokencount = 0;
	}
	return 0;
}


// Convert SYSLOG ISO8601 time to UNIX epoch time.


time_t parseiso8601utc_fixed(const char *date)
{
	struct tm tt = {0};
	double seconds;
	if (sscanf(date, "%04d-%02d-%02dT%02d:%02d:%lfZ",
		&tt.tm_year, &tt.tm_mon, &tt.tm_mday,
		&tt.tm_hour, &tt.tm_min, &seconds) != 6)
		return -1;
	tt.tm_year -= 1900;
	tt.tm_isdst = 0;
	tt.tm_sec = (int)seconds;
	tt.tm_mon--;
	return timegm(&tt);
}


// Print out TIMEZONE Matrix to screen


void printzonedata(char *zonetable[][TIMEZONECOLUMNS])
{
	int arrayrow = 0;
	int arraycolumn = 0;

	for (arrayrow = 0; zonetable[arrayrow][0] != NULL; arrayrow++) {
		printf("%s: ", zonetable[arrayrow][0]);
		for (arraycolumn = 0; zonetable[arrayrow][arraycolumn] != NULL; arraycolumn++) {
			printf("%s ", zonetable[arrayrow][arraycolumn]);
		}
		printf("\n");
	}
}


// Take input timezone text and match up with with matrix table then return index 0 entry.


void validate_zoneinfo(char** string_ptr, char *zonetable[][TIMEZONECOLUMNS])
{
	char *ptr = malloc(40);
	char *zonecaseholder = malloc(40);
	if (ptr == NULL) {
		printf("Could not allocate ptr memory!");
		exit(1);
	}

	int arrayrow = 0;
	int arraycolumn = 0;
	for (arrayrow = 0; zonetable[arrayrow][0] != NULL; arrayrow++) {
		for (arraycolumn = 0; zonetable[arrayrow][arraycolumn] != NULL; arraycolumn++) {
			strncpy(zonecaseholder, zonetable[arrayrow][arraycolumn], 40);
			uppercase(zonecaseholder);
			if (strcmp(zonecaseholder, *string_ptr) == 0) {
				ptr = zonetable[arrayrow][0];
			}
		}
	}
	*string_ptr = ptr;
	free(zonecaseholder);
}

void uppercase(char *text)
{
	// change to upper case
	for (int i = 0; text[i] != '\0'; i++) {
		if (text[i] >= 'a' && text[i] <= 'z') {
			text[i] = text[i] -32;
		}
	}
}

void truncateatdigits(char *text)
{
	// Truncate string where we encounter digits with a \0
	for (int i = 0; text[i] != '\0'; i++) {
		if (isdigit(text[i])) {
			text[i] = '\0';
		}
	}
}

