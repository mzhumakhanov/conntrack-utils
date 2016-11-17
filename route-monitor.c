#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netlink/netlink.h>
#include <netlink/msg.h>

#include "nl-monitor.h"

static int cb(struct nl_msg *m, void *ctx)
{
	struct nlmsghdr *h = nlmsg_hdr (m);
	struct rtmsg *rtm;
	int len;
	struct rtattr *rta;
	struct in_addr *address;

	if (h->nlmsg_type != RTM_NEWROUTE &&
	    h->nlmsg_type != RTM_DELROUTE)
		return 0;

	rtm = nlmsg_data (h);

	if (rtm->rtm_family != AF_INET)
		return 0;

	printf ("route %s", h->nlmsg_type == RTM_NEWROUTE ? "add" : "del");

	for (
		rta = (void *) (rtm + 1), len = nlmsg_len (h) + sizeof (*rtm);
		RTA_OK (rta, len);
		rta = RTA_NEXT (rta, len)
	)
		if (rta->rta_type == RTA_DST) {
			address = RTA_DATA(rta);

			printf (" dst %s/%d", inet_ntoa (*address),
				rtm->rtm_dst_len);
		}

	printf ("\n");

	return 0;
}

int main (void)
{
	if (nl_monitor (cb, NETLINK_ROUTE, RTNLGRP_IPV4_ROUTE, 0) < 0)
		return 1;

	return 0;
}
