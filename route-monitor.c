#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netlink/netlink.h>
#include <netlink/msg.h>

#include "nl-monitor.h"

static void route_show_rta (struct rtmsg *rtm, struct rtattr *rta)
{
	char buf[INET6_ADDRSTRLEN];
	const char *p;
	int n;

	switch (rta->rta_type) {
	case RTA_TABLE:
		switch (n = *(int *) RTA_DATA (rta)) {
		case RT_TABLE_MAIN:
			printf (" main"); break;
		case RT_TABLE_LOCAL:
			printf (" local"); break;
		default:
			printf (" table %d", n);
		}
		break;
	case RTA_DST:
		p = inet_ntop (rtm->rtm_family, RTA_DATA (rta),
			       buf, sizeof (buf));

		printf (" dst %s/%d", p, rtm->rtm_dst_len);
		break;
	case RTA_GATEWAY:
		p = inet_ntop (rtm->rtm_family, RTA_DATA (rta),
			       buf, sizeof (buf));

		printf (" via %s", p);
		break;
	case RTA_OIF:
		printf (" dev %d", *(int *) RTA_DATA (rta));
		break;
	case RTA_PREFSRC:
		p = inet_ntop (rtm->rtm_family, RTA_DATA (rta),
			       buf, sizeof (buf));

		printf (" src %s", p);
		break;
	case RTA_PRIORITY:
		printf (" metric %u", *(unsigned *) RTA_DATA (rta));
		break;
	default:
		printf (" type %d", rta->rta_type);
		break;
	}
}

static int process_route (struct nlmsghdr *h, struct rtmsg *rtm, void *ctx)
{
	struct rtattr *rta;
	int len;

	if (rtm->rtm_family != AF_INET && rtm->rtm_family != AF_INET6)
		return 0;

	printf ("route %s", h->nlmsg_type == RTM_NEWROUTE ? "add" : "del");

	for (
		rta = RTM_RTA (rtm), len = RTM_PAYLOAD (h);
		RTA_OK (rta, len);
		rta = RTA_NEXT (rta, len)
	)
		route_show_rta (rtm, rta);

	printf ("\n");

	return 0;
}

static int cb (struct nl_msg *m, void *ctx)
{
	struct nlmsghdr *h = nlmsg_hdr (m);

	switch (h->nlmsg_type) {
	case RTM_NEWROUTE:
	case RTM_DELROUTE:
		return process_route (h, nlmsg_data (h), ctx);
	}

	return 0;
}

int main (void)
{
	if (nl_execute (cb, NETLINK_ROUTE, RTM_GETROUTE) < 0 ||
	    nl_monitor (cb, NETLINK_ROUTE, RTNLGRP_IPV4_ROUTE,
					   RTNLGRP_IPV6_ROUTE, 0) < 0) {
		nl_perror ("netlink monitor");
		return 1;
	}

	return 0;
}
