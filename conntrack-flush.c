#include <errno.h>
#include <stdio.h>

#include "nfct-flush-net.h"

static in_addr_aton (const char *from, struct in_net *net)
{
	unsigned a, b, c, d, m;
	int n;

	n = sscanf (from, "%u.%u.%u.%u/%u", &a, &b, &c, &d, &m);

	if (n < 4 || a > 255 || b > 255 || c > 255 || d > 255 ||
	    (n == 5 && m > 32) || n > 5) {
		errno = EINVAL;
		return 0;
	}

	net->address.s_addr = a | b << 8 | c << 16 | d << 24;
	net->mask.s_addr = (n < 5) ?
		0xffffffffL :
		htonl (0xffffffffL << (32 - m));

	return n == 5 ? 2 : 1;
}

int main (int argc, char *argv[])
{
	struct in_net net;

	if (argc != 2) {
		fprintf (stderr, "Usage:\n"
				 "\tconntrack-flush <dest-addr/mask>\n");
		return 1;
	}

	if (in_addr_aton (argv[1], &net) < 1) {
		fprintf (stderr, "Wrong address/network format\n");
		return 1;
	}

	if (nfct_flush_net (&net) != 0) {
		perror ("netlink");
		return 1;
	}

	return 0;
}
