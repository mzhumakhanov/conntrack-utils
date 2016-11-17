#ifndef _NFCT_FLUSH_NET_H
#define _NFCT_FLUSH_NET_H  1_

#include <netinet/in.h>

struct in_net {
	struct in_addr address, mask;
};

int nfct_flush_net (struct in_net *net);

#endif  /* _NFCT_FLUSH_NET_H */
