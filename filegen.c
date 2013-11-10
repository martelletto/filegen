/* see copyright notice in LICENSE */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <err.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <util.h>

bool debugging = false;		/* print debugging info */
bool syncwrite = false;		/* call fsync() before closing */
bool randomise = false;		/* whether to write random bytes */
bool reporting = true;		/* report corrupt/invalid files */
long interval = 0;		/* max nanosecs between writes */
size_t maxfilesiz = 1 << 24;	/* max size of individual file */
size_t totalbytes = 1 << 30;	/* total bytes to write/verify */
const char *prefix = "";	/* prefix for test files */

static struct timespec itv;
static char path[_POSIX_PATH_MAX + 1];

size_t
nextrand(size_t limit)
{
	if (limit < 2)
		return (limit);
	size_t r = (size_t)rand();
	if (r > limit)
		return (r % limit);
	return (r);
}

size_t
nextfilesiz(void)
{
	if (maxfilesiz > totalbytes)
		return (nextrand(totalbytes));
	return (nextrand(maxfilesiz));
}

size_t
nextchunksiz(size_t size, size_t threshold)
{
	if (size > threshold)
		return (nextrand(threshold));
	return (size);
}

size_t
nextword(int n)
{
	if (randomise == false)
		return (n);
	else
		return (nextrand(RAND_MAX));
}

void
mkpath(int n)
{
	size_t len = sizeof(path) - 1;
	int r = snprintf(path, len, "%sf%04x", prefix, n);
	if (r < 0 || (size_t)r >= len)
		errx(1, "snprintf");
}

void
fillbuf(int n, void *buf, size_t size)
{
	size_t *p = buf;
	int rest = size % sizeof(size_t);
	size -= rest;

	while (size > 0) {
		*p++ = nextword(n);
		size -= sizeof(size_t);
	}

	if (rest != 0) {
		size_t last = nextword(n);
		unsigned char *cp1 = (unsigned char *)p;
		unsigned char *cp2 = (unsigned char *)&last;
		while (rest--)
			*cp1++ = *cp2++; 
	}
}

int
checkbuf(int n, void *buf, size_t size)
{
	size_t *p = buf;
	int ok = 1, rest = size % sizeof(size_t);
	size -= rest;

	while (size > 0) {
		if (*p++ != nextword(n))
			ok = 0;
		size -= sizeof(size_t);
	}

	if (rest != 0) {
		size_t last = nextword(n);
		unsigned char *cp1 = (unsigned char *)p;
		unsigned char *cp2 = (unsigned char *)&last;
		while (rest--)
			if (*cp1++ != *cp2++)
				ok = 0;
	}

	return (ok);
}

void
chkread(ssize_t nread, size_t chunksiz)
{
	if (reporting && nread != (ssize_t)chunksiz) {
		reporting = false;
		if (nread < 0)
			warn("read");
		else if (nread == 0)
			warnx("file truncated");
		else
			warnx("short read");
	}
}

void
skipchunk(int n, size_t size)
{
	int rest = size % sizeof(size_t);
	size -= rest;
	while (size > 0) {
		nextword(n);
		size -= sizeof(size_t);
	}
	if (rest != 0)
		nextword(n);
}

void
skipfile(int n, size_t size)
{
	while (size > 0) {
		size_t chunksiz = nextchunksiz(size, 64*1024);
		skipchunk(n, chunksiz);
		size -= chunksiz;
	}
}

void
verify(int n, void *buf, size_t size)
{
	ssize_t r;

	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		warn("open %s", path);
		skipfile(n, size);
		return;
	}

	reporting = true;

	while (size > 0) {
		size_t chunksiz = nextchunksiz(size, 64*1024);
		r = read(fd, buf, chunksiz);
		chkread(r, chunksiz);
		if (checkbuf(n, buf, chunksiz) == 0 && reporting) {
			warnx("file corrupt");
			reporting = false;
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
writef(int n, void *buf, size_t size)
{
	int fd = open(path, O_WRONLY|O_CREAT|O_EXCL, S_IRUSR|S_IWUSR);
	if (fd < 0)
		err(1, "open");

	/* XXX: should use adjustable threshold */
	while (size > 0) {
		size_t chunksiz = nextchunksiz(size, 64*1024);
		fillbuf(n, buf, chunksiz);
		if (write(fd, buf, chunksiz) < 0)
			err(1, "write");
		size -= chunksiz;
		if (interval)
			nanosleep(&itv, NULL);
	}

	if (syncwrite)
		fsync(fd);

	close(fd);
}

__dead void
usage(void)
{
	fprintf(stderr,
"usage: filegen [-drvS] [-f maxfilesiz] [-i interval] [-p prefix] [-s seed]\n"
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

	while ((ch = getopt(argc, argv, "df:i:p:rs:t:vS")) != -1) {
		switch (ch) {
		case 'd':
			debugging = true;
			break;
		case 'f':
			maxfilesiz = strtonum(optarg, 1, SSIZE_MAX, &errstr);
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
			randomise = false;
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
			totalbytes = strtonum(optarg, 1, SSIZE_MAX, &errstr);
			if (errstr)
				errx(1, "-t: total bytes %s", errstr);
			break;
		case 'v':
			action = "verifying";
			verifying = 1;
			break;
		case 'S':
			syncwrite = true;
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
		warnx("interval when verifying doesn't make sense; ignoring");
	if (syncwrite != 0 && verifying)
		warnx("syncing when verifying doesn't make sense; ignoring");

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
	srand(seed);

	if (debugging) {
		printf("using random seed: %u\n", seed);
		printf("max file size: %zu bytes\n", maxfilesiz);
		printf("total being written: %zu bytes\n", totalbytes);
	}

	while (totalbytes > 0) {
		size_t filesiz = nextfilesiz();
		mkpath(n);
		if (debugging)
			printf("%s %s (%zu bytes)\n", action, path, filesiz);
		if (verifying)
			verify(n++, buf, filesiz);
		else
			writef(n++, buf, filesiz);
		totalbytes -= filesiz;
	}

	exit(0);
}
