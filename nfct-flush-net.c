#include <libnetfilter_conntrack/libnetfilter_conntrack.h>

#include "nfct-flush-net.h"

struct ctx {
	struct nfct_handle *handle;
	struct in_net *net;
};

static int flush_cb (enum nf_conntrack_msg_type type,
			struct nf_conntrack *ct, void *data)
{
	struct ctx *c = data;
	struct in_addr dest;

	if ((type != NFCT_T_NEW && type != NFCT_T_UPDATE) ||
	    !nfct_attr_is_set (ct, ATTR_IPV4_DST))
		return NFCT_CB_CONTINUE;

	dest.s_addr = nfct_get_attr_u32 (ct, ATTR_IPV4_DST);

	if ((dest.s_addr & c->net->mask.s_addr) != c->net->address.s_addr)
		return NFCT_CB_CONTINUE;

	(void) nfct_query(c->handle, NFCT_Q_DESTROY, ct);

	return NFCT_CB_CONTINUE;
}

int nfct_flush_net (struct in_net *net)
{
	struct ctx c;
	const int family = AF_INET;
	int ret;

	if ((c.handle = nfct_open (CONNTRACK, 0)) == NULL)
		return -1;

	c.net = net;

	nfct_callback_register (c.handle, NFCT_T_ALL, flush_cb, &c);

	ret = nfct_query (c.handle, NFCT_Q_DUMP, &family);

	nfct_close (c.handle);

	return ret;
}
