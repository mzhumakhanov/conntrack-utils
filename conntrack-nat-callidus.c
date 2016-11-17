/*
 * Dæmon irrēpit callidus...
 *
 * This daemon clears conntrack entries for a destination address in case of
 * route for this destination adress is deleted.
 *
 * (c) 2016 Alexei A. Smekalkine <ikle@ikle.ru>
 */

#include <stdio.h>
#include <syslog.h>
#include <unistd.h>

#include <netlink/netlink.h>
#include <netlink/msg.h>

#include "nfct-flush-net.h"
#include "nl-monitor.h"

static int cb(struct nl_msg *m, void *ctx)
{
	struct nlmsghdr *h = nlmsg_hdr (m);
	struct rtmsg *rtm;
	struct rtattr *rta;
	int len;
	struct in_addr *address;

	struct in_net net;

	if (h->nlmsg_type != RTM_DELROUTE)
		return 0;

	rtm = nlmsg_data (h);

	if (rtm->rtm_family != AF_INET)
		return 0;

	for (
		address = NULL, rta = RTM_RTA (rtm), len = RTM_PAYLOAD (h);
		RTA_OK (rta, len);
		rta = RTA_NEXT (rta, len)
	)
		if (rta->rta_type == RTA_DST)
			address = RTA_DATA(rta);

	if (address == NULL)
		return 0;

	net.address = *address;
	net.mask.s_addr = htonl (0xffffffffL << (32 - rtm->rtm_dst_len));

	(void) nfct_flush_net (&net);

	return 0;
}

int main (void)
{
	if (daemon (0, 0) != 0) {
		perror("conntrack-nat-callidus, daemon");
		return 1;
	}

	nl_monitor (cb, NETLINK_ROUTE, RTNLGRP_IPV4_ROUTE, 0);

	openlog ("conntrack-nat-callidus", 0, LOG_DAEMON);
	syslog (LOG_ERR, "nl-monitor: %m");
	closelog ();

	return 1;
}
