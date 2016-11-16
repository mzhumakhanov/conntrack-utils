#include <errno.h>
#include <stdio.h>
#include <libnetfilter_conntrack/libnetfilter_conntrack.h>

struct in_net {
	struct in_addr address, mask;
};

struct nl_ctx {
	struct nfct_handle *handle;
	struct in_net *net;
};

static int nl_flush_cb (enum nf_conntrack_msg_type type,
			struct nf_conntrack *ct, void *data)
{
	struct nl_ctx *c = data;
	struct in_addr dest;

	if (type != NFCT_T_UPDATE ||
	    !nfct_attr_is_set (ct, ATTR_IPV4_DST))
		return NFCT_CB_CONTINUE;

	dest.s_addr = nfct_get_attr_u32 (ct, ATTR_IPV4_DST);

	if ((dest.s_addr & c->net->mask.s_addr) != c->net->address.s_addr)
		return NFCT_CB_CONTINUE;

	(void) nfct_query(c->handle, NFCT_Q_DESTROY, ct);

	return NFCT_CB_CONTINUE;
}

static int nl_flush_net (struct in_net *net)
{
	struct nl_ctx c;
	const int family = AF_INET;
	int ret;

	if ((c.handle = nfct_open (CONNTRACK, 0)) == NULL)
		return -1;

	nfct_callback_register (c.handle, NFCT_T_ALL, nl_flush_cb, &c);

	ret = nfct_query (c.handle, NFCT_Q_DUMP, &family);

	nfct_close (c.handle);

	return ret;
}

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

	if (nl_flush_net (&net) != 0) {
		perror ("netlink");
		return 1;
	}

	return 0;
}
