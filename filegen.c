/*
 * Copyright 2013 by Pedro Martelletto <pedro@ambientworks.net>.
 *
 * Modification and redistribution in source and binary forms is permitted
 * provided that due credit is given to the author by leaving this copyright
 * notice intact.
 */

#include <sys/time.h>

#include <err.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <util.h>

long randomise = 0;		/* whether to write random bytes */
long reporting = 1;		/* report corrupt/invalid files */
long interval = 0;		/* max nanosecs between writes */
unsigned maxfilesiz = 1 << 24;	/* max size of individual file */
unsigned totalbytes = 1 << 30;	/* total bytes to write/verify */
const char *prefix = "";	/* prefix for test files */

static struct timespec itv;
static char path[_POSIX_PATH_MAX + 1];

unsigned int
nextrand(unsigned int limit)
{
	unsigned int r;
	while ((r = (unsigned int)rand()) > limit);
	return (r);
}

unsigned int
nextfilesiz(void)
{
	if (maxfilesiz > totalbytes)
		return (nextrand(totalbytes));
	return (nextrand(maxfilesiz));
}

unsigned int
nextchunksiz(unsigned int size, unsigned int threshold)
{
	if (size > threshold)
		return (nextrand(threshold));
	return (size);
}

unsigned int
nextbyte(int n)
{
	if (randomise == 0) {
		unsigned int r;
		memset(&r, n, sizeof(r));
		return (r);
	} else
		return (nextrand(RAND_MAX));
}

void
mkpath(int n)
{
	size_t len = sizeof(path) - 1;
	int r = snprintf(path, len, "%s%x", prefix, n);
	if (r < 0 || (size_t)r >= len)
		errx(1, "snprintf");
}

void
fillbuf(int n, void *buf, unsigned int size)
{
	unsigned int *p = buf;
	int rest = size % sizeof(unsigned int);
	size -= rest;

	while (size > 0) {
		*p++ = nextbyte(n);
		size -= sizeof(unsigned int);
	}

	if (rest != 0) {
		unsigned int last = nextbyte(n);
		unsigned char *cp1 = (unsigned char *)p;
		unsigned char *cp2 = (unsigned char *)&last;
		while (rest--)
			*cp1++ = *cp2++; 
	}
}

int
checkbuf(int n, void *buf, unsigned int size)
{
	unsigned int *p = buf;
	int ok = 1, rest = size % sizeof(unsigned int);
	size -= rest;

	while (size > 0) {
		if (*p++ != nextbyte(n))
			ok = 0;
		size -= sizeof(unsigned int);
	}

	if (rest != 0) {
		unsigned int last = nextbyte(n);
		unsigned char *cp1 = (unsigned char *)p;
		unsigned char *cp2 = (unsigned char *)&last;
		while (rest--)
			if (*cp1++ != *cp2++)
				ok = 0;
	}

	return (ok);
}

void
chkread(ssize_t nread, unsigned int chunksiz)
{
	if (reporting && nread != chunksiz) {
		reporting = 0;
		if (nread < 0)
			warn("read");
		else if (nread == 0)
			warnx("file truncated");
		else
			warnx("short read");
	}
}

void
verify(int n, void *buf, unsigned int size)
{
	ssize_t r;

	int fd = open(path, O_RDONLY);
	if (fd < 0)
		err(1, "open");

	reporting = 1;

	while (size > 0) {
		unsigned int chunksiz = nextchunksiz(size, 64*1024);
		r = read(fd, buf, chunksiz);
		chkread(r, chunksiz);
		if (checkbuf(n, buf, chunksiz) == 0 && reporting) {
			warnx("file corrupt");
			reporting = 0;
		}
		size -= chunksiz;
	}

	char c;
	r = read(fd, &c, sizeof(c));
	if (r != 0 && reporting)
		warnx("file longer than expected");

	close(fd);
}

void
writef(int n, void *buf, unsigned int size)
{
	int fd = open(path, O_WRONLY | O_CREAT | O_EXCL);
	if (fd < 0)
		err(1, "open");

	/* XXX: should use adjustable threshold */
	while (size > 0) {
		unsigned int chunksiz = nextchunksiz(size, 64*1024);
		fillbuf(n, buf, chunksiz);
		if (write(fd, buf, chunksiz) < 0)
			err(1, "write");
		size -= chunksiz;
		if (interval)
			nanosleep(&itv, NULL);
	}

	close(fd);
}

__dead void
usage(void)
{
	fprintf(stderr,
"usage: filegen [-rv] [-f maxfilesiz] [-i interval] [-p prefix] [-s seed]\n"
"               [-t totalbytes] directory\n");
	exit(1);
}

int
main(int argc, char **argv)
{
	int ch, n = 0, verifying = 0;
	unsigned int seed = 0;
	const char *errstr;
	const char *action = "writing";
	void *buf;

	while ((ch = getopt(argc, argv, "f:i:p:rs:t:v")) != -1) {
		switch (ch) {
		case 'f':
			maxfilesiz = strtonum(optarg, 1, UINT_MAX, &errstr);
			if (errstr)
				errx(1, "-f: max file size %s", errstr);
			break;
		case 'i':
			interval = strtonum(optarg, 1, LONG_MAX, &errstr);
			if (errstr)
				errx(1, "-i: interval %s", errstr);
			itv.tv_sec = 0;
			itv.tv_nsec = interval;
			break;
		case 'r':
			randomise = 1;
			break;
		case 'p':
			prefix = optarg;
			break;
		case 's':
			seed = strtonum(optarg, 1, UINT_MAX, &errstr);
			if (errstr)
				errx(1, "-s: seed %s", errstr);
			break;
		case 't':
			totalbytes = strtonum(optarg, 1, UINT_MAX, &errstr);
			if (errstr)
				errx(1, "-t: total bytes %s", errstr);
			break;
		case 'v':
			action = "verifying";
			verifying = 1;
			break;
		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (argc == 0)
		usage();

	if (maxfilesiz > totalbytes)
		errx(1, "max file size bigger than total size");
	if (interval != 0 && verifying)
		errx(1, "interval when verifying doesn't make sense");

	if (chdir(argv[0]) != 0)
		err(1, "chdir");
	buf = malloc(maxfilesiz);
	if (buf == NULL)
		err(1, "malloc");
	if (seed == 0) {
		struct timeval tv;
		if (gettimeofday(&tv, NULL) != 0)
			err(1, "gettimeofday");
		seed = (unsigned)(tv.tv_sec ^ tv.tv_usec);
	}

	printf("using random seed: %u\n", seed);
	printf("max file size: %u bytes\n", maxfilesiz);
	printf("total being written: %u bytes\n", totalbytes);
	srand(seed);

	while (totalbytes > 0) {
		unsigned int filesiz = nextfilesiz();
		mkpath(n);
		printf("%s %s (%u bytes)\n", action, path, filesiz);
		if (verifying)
			verify(n++, buf, filesiz);
		else
			writef(n++, buf, filesiz);
		totalbytes -= filesiz;
	}

	exit(0);
}
