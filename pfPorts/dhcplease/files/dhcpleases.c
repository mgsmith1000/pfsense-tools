/*-
 * Copyright (c) 2010 Ermal Lu�i <eri@pfsense.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


/* 
 * The parsing code is taken from dnsmasq isc.c file and modified to work
 * in this code. 
 */
/* dnsmasq is Copyright (c) 2000-2007 Simon Kelley

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 dated June, 1991, or
   (at your option) version 3 dated 29 June, 2007.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/* Code in this file is based on contributions by John Volpe. */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/event.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/queue.h>

#include <netinet/in.h>
#include <arpa/nameser.h>
#include <arpa/inet.h>

#include <syslog.h>
#include <stdarg.h>
#include <time.h>
#include <signal.h>

#define _WITH_DPRINTF
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <errno.h>

#define MAXTOK 64

struct isc_lease {
	char *name, *fqdn;
	time_t expires;
	struct in_addr addr;
	LIST_ENTRY(isc_lease) next;
};

LIST_HEAD(isc_leases, isc_lease) leases =
	LIST_HEAD_INITIALIZER(leases);
static char *leasefile = NULL;
static char *pidfile = NULL;
static char *HOSTS = NULL;
static FILE *fp = NULL;
static char *domain_suffix = NULL;
static size_t hostssize = 0;

/* Check if file exists */
static int
fexist(char * filename)
{
        struct stat buf;

        if (( stat (filename, &buf)) < 0)
                return (0);

        if (! S_ISREG(buf.st_mode))
                return (0);

        return(1);
}

static int
fsize(char * filename)
{
        struct stat buf;

        if (( stat (filename, &buf)) < 0)
                return (-1);

        if (! S_ISREG(buf.st_mode))
                return (-1);

        return(buf.st_size);
}

/*
 * check for legal char a-z A-Z 0-9 -
 * (also / , used for RFC2317 and _ used in windows queries
 * and space, for DNS-SD stuff)
 */
static int
legal_char(char c) {
	if ((c >= 'A' && c <= 'Z') ||
	    (c >= 'a' && c <= 'z') ||
	    (c >= '0' && c <= '9') ||
	    c == '-' || c == '/' || c == '_' || c == ' ')
		return (1);
	return (0);
}

/*
 * check for legal chars and remove trailing .
 * also fail empty string and label > 63 chars
 */
static int
canonicalise(char *s) {
	size_t dotgap = 0, l = strlen(s);
	char c;
	int nowhite = 0;

	if (l == 0 || l > MAXDNAME)
		return (0);

	if (s[l-1] == '.') {
		if (l == 1)
			return (0);
		s[l-1] = 0;
	}

	while ((c = *s)) {
		if (c == '.')
			dotgap = 0;
		else if (!legal_char(c) || (++dotgap > MAXLABEL))
			return (0);
		else if (c != ' ')
			nowhite = 1;
		s++;
	}

	return (nowhite);
}

/* don't use strcasecmp and friends here - they may be messed up by LOCALE */
static int
hostname_isequal(char *a, char *b) {
	unsigned int c1, c2;

	do {
		c1 = (unsigned char) *a++;
		c2 = (unsigned char) *b++;

		if (c1 >= 'A' && c1 <= 'Z')
			c1 += 'a' - 'A';
		if (c2 >= 'A' && c2 <= 'Z')
			c2 += 'a' - 'A';

		if (c1 != c2)
			return 0;
	} while (c1);

	return (1);
}

static int
next_token (char *token, int buffsize, FILE * fp)
{
	int c, count = 0;
	char *cp = token;

	while((c = getc(fp)) != EOF) {
		if (c == '#')
			do {
				c = getc(fp);
			} while (c != '\n' && c != EOF);

		if (c == ' ' || c == '\t' || c == '\n' || c == ';') {
			if (count)
				break;
		} else if ((c != '"') && (count<buffsize-1)) {
			*cp++ = c;
			count++;
		}
	}

	*cp = 0;
	return count ? 1 : 0;
}

/*
 * There doesn't seem to be a universally available library function
 * which converts broken-down _GMT_ time to seconds-in-epoch.
 * The following was borrowed from ISC dhcpd sources, where
 * it is noted that it might not be entirely accurate for odd seconds.
 * Since we're trying to get the same answer as dhcpd, that's just
 * fine here.
 */
static time_t
convert_time(struct tm lease_time) {
	static const int months [11] = { 31, 59, 90, 120, 151, 181,
						212, 243, 273, 304, 334 };
	time_t time = ((((((365 * (lease_time.tm_year - 1970) + /* Days in years since '70 */
			    (lease_time.tm_year - 1969) / 4 +   /* Leap days since '70 */
			    (lease_time.tm_mon > 1		/* Days in months this year */
				? months [lease_time.tm_mon - 2]
				: 0) +
			    (lease_time.tm_mon > 2 &&		/* Leap day this year */
			    !((lease_time.tm_year - 1972) & 3)) +
			    lease_time.tm_mday - 1) * 24) +	/* Day of month */
			    lease_time.tm_hour) * 60) +
			    lease_time.tm_min) * 60) + lease_time.tm_sec;

	return (time);
}

static int
load_dhcp(time_t now) {
	char namebuff[256];
	char *hostname = namebuff;
	char token[MAXTOK], *dot;
	struct in_addr host_address;
	time_t ttd, tts;
	struct isc_lease *lease, *tmp;

	LIST_INIT(&leases);

	while ((next_token(token, MAXTOK, fp))) {
		if (strcmp(token, "lease") == 0) {
			hostname[0] = '\0';
			ttd = tts = (time_t)(-1);
			if (next_token(token, MAXTOK, fp) && 
			    (inet_pton(AF_INET, token, &host_address))) {
				if (next_token(token, MAXTOK, fp) && *token == '{') {
					while (next_token(token, MAXTOK, fp) && *token != '}') {
						if ((strcmp(token, "client-hostname") == 0) ||
						    (strcmp(token, "hostname") == 0)) {
							if (next_token(hostname, MAXDNAME, fp))
								if (!canonicalise(hostname)) {
									*hostname = 0;
									syslog(LOG_ERR, "bad name in %s", leasefile); 
								}
						} else if ((strcmp(token, "ends") == 0) ||
							    (strcmp(token, "starts") == 0)) {
								struct tm lease_time;
								int is_ends = (strcmp(token, "ends") == 0);
								if (next_token(token, MAXTOK, fp) &&  /* skip weekday */
								    next_token(token, MAXTOK, fp) &&  /* Get date from lease file */
								    sscanf (token, "%d/%d/%d", 
									&lease_time.tm_year,
									&lease_time.tm_mon,
									&lease_time.tm_mday) == 3 &&
								    next_token(token, MAXTOK, fp) &&
								    sscanf (token, "%d:%d:%d:", 
									&lease_time.tm_hour,
									&lease_time.tm_min, 
									&lease_time.tm_sec) == 3) {
									if (is_ends)
										ttd = convert_time(lease_time);
									else
										tts = convert_time(lease_time);
								}
						}
					}
				/* missing info? */
				if (!*hostname)
					continue;
				if (ttd == (time_t)(-1))
					continue;

				/* We use 0 as infinite in ttd */
				if ((tts != -1) && (ttd == tts - 1))
					ttd = (time_t)0;
				else if (difftime(now, ttd) > 0)
					continue;

				if ((dot = strchr(hostname, '.'))) {
					if (!domain_suffix || hostname_isequal(dot+1, domain_suffix)) {
						syslog(LOG_WARNING, 
							"Ignoring DHCP lease for %s because it has an illegal domain part", 
							hostname);
						continue;
					}
					*dot = 0;
				}

				LIST_FOREACH(lease, &leases, next) {
					if (hostname_isequal(lease->name, hostname)) {
						lease->expires = ttd;
						lease->addr = host_address;
						break;
					}
				}

				if (!lease) { 
					if ((lease = malloc(sizeof(struct isc_lease))) == NULL)
						continue;
					lease->expires = ttd;
					lease->addr = host_address;
					lease->fqdn =  NULL;
					LIST_INSERT_HEAD(&leases, lease, next); 
				} else {
					if (lease->name != NULL)
						free(lease->name);
					if (lease->fqdn != NULL)
						free(lease->fqdn);
				}

				if (!(lease->name = malloc(strlen(hostname)+1)))
					free(lease);
				strcpy(lease->name, hostname);
				if ((lease->fqdn = malloc(strlen(hostname) + strlen(domain_suffix) + 2)) != NULL) {
					strcpy(lease->fqdn, hostname);
					strcat(lease->fqdn, ".");
					strcat(lease->fqdn, domain_suffix);
				} else {
					LIST_REMOVE(lease, next);
					free(lease->name);
					free(lease);
				}
				}
			}
		}
	}

  
	/* prune expired leases */
	LIST_FOREACH_SAFE(lease, &leases, next, tmp) {
		if (lease->expires != (time_t)0 && difftime(now, lease->expires) > 0) {
			if (lease->name)
				free(lease->name);
			if (lease->fqdn)
				free(lease->fqdn);
			LIST_REMOVE(lease, next);
			free(lease);
		}
	}

	return (0);
}

static int
write_status() {
	struct isc_lease *lease;
	int fd;
	
	fd = open(HOSTS, O_RDWR | O_CREAT | O_FSYNC);
        if (fd < 0) {
		return 1;
	}
	ftruncate(fd, hostssize);
	if (lseek(fd, 0, SEEK_END) < 0) {
		close(fd);
		return 2;
	}
	/* write the tmp hosts file */
	LIST_FOREACH(lease, &leases, next) {
		dprintf(fd, "%s\t%s %s\t\t# dynamic entry from dhcpd.leases\n", inet_ntoa(lease->addr),
			lease->fqdn ? lease->fqdn  : "empty", lease->name ? lease->name : "empty");
	}
	close(fd);

	return (0);
}

static void
cleanup() {
	struct isc_lease *lease, *tmp;

	LIST_FOREACH_SAFE(lease, &leases, next, tmp) {
		if (lease->fqdn)
			free(lease->fqdn);
		if (lease->name)
			free(lease->name);
		LIST_REMOVE(lease, next);
		free(lease);
	}

	return;
}

static void
signal_process() {
	FILE *fd;
	size_t size = 0;
	char *pid = NULL, *pc;
	int c, pidno;

	if (pidfile == NULL)
		goto error;
	size = fsize(pidfile);
	if (size < 0)
		goto error;

	fd = fopen(pidfile, "r");
	if (fd == NULL)
		goto error;

	pid = calloc(size, size);
	if (pid == NULL) {
		fclose(fd);
		goto error;
	}
	pc = pid;
	while ((c = getc(fd)) != EOF) {
		if (c == '\n')
			break;
		*pc++ = c;
	}
	fclose(fd);

	pidno = atoi(pid);
	free(pid);

	if (kill((pid_t)pidno, SIGHUP) < 0)
		goto error;

	return;
error:
	syslog(LOG_ERR, "Could not deliver signal HUP to process because its pidfile does not exist, %m.");
	return;
}

int
main(int argc, char **argv) {
	struct kevent evlist;    /* events we want to monitor */
	struct kevent chlist;    /* events that were triggered */
	time_t	now;
	int kq, nev, leasefd = 0;

	if (argc != 5) {
		perror("Wrong number of arguments given."); /* XXX: usage */
		exit(2);
	}

	leasefile = argv[1];
	domain_suffix = argv[2];
	if (domain_suffix == NULL);
		domain_suffix = "local";

	pidfile = argv[3];
	if (pidfile == NULL) {
		perror("You nned to pass a pid file.");
		exit(7);
	}
	HOSTS = argv[4];
	if (HOSTS == NULL || !fexist(HOSTS)) {
		perror("You need to specify the hosts file path.");
		exit(8);
	}

	if ((hostssize = fsize(HOSTS)) < 0) {
		perror("Error while getting /etc/hosts file size.");
		exit(6);
	}
	if (!fexist(leasefile)) {
		perror("The leases file passed as argument does not exist.");
		exit(3);
	}

	if (daemon(0, 0) < 0) {
		perror("Could not daemonize");
		exit(4);
	}

	leasefd = open(leasefile, O_RDONLY);
	if (leasefd < 0) {
		perror("Could not get descriptor");
		exit(6);
	}

	fp = fdopen(leasefd, "r");
	if (fp == NULL) {
		perror("could not open leasefile");
		exit(5);
	}

	/* Create a new kernel event queue */
	if ((kq = kqueue()) == -1)
		exit(1);

	/* Initialise kevent structure */
	EV_SET(&chlist, leasefd, EVFILT_VNODE, EV_ADD | EV_ENABLE | EV_ONESHOT,
		NOTE_WRITE, 0, NULL);
	/* Loop forever */
	for (;;) {
		nev = kevent(kq, &chlist, 1, &evlist, 1, NULL);
		if (nev == -1)
			perror("kevent()");
		else if (nev > 0) {
			if (evlist.flags & EV_ERROR) {
				syslog(LOG_ERR, "EV_ERROR: %s\n", strerror(evlist.data));
				break;
			}
			now = time(NULL);
			if (load_dhcp(now)) {
				syslog(LOG_ERR, "could not parse %s file", leasefile);
				break;
			}

			write_status();
			//syslog(LOG_INFO, "written temp hosts file after modification event.");

			cleanup();
			//syslog(LOG_INFO, "Cleaned up.");

			signal_process();
		}
	}

	fclose(fp);

	return (0);
}
