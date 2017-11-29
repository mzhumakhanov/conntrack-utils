#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netlink/netlink.h>
#include <netlink/msg.h>

#include "nl-monitor.h"

static void show_rta (struct rtmsg *rtm, struct rtattr *rta)
{
	struct in_addr *address;
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
		address = RTA_DATA (rta);

		printf (" dst %s/%d", inet_ntoa (*address), rtm->rtm_dst_len);
		break;
	case RTA_GATEWAY:
		address = RTA_DATA (rta);

		printf (" via %s", inet_ntoa (*address));
		break;
	case RTA_OIF:
		printf (" dev %d", *(int *) RTA_DATA (rta));
		break;
	case RTA_PREFSRC:
		address = RTA_DATA (rta);

		printf (" src %s", inet_ntoa (*address));
		break;
	case RTA_PRIORITY:
		printf (" metric %u", *(unsigned *) RTA_DATA (rta));
		break;
	default:
		printf (" type %d", rta->rta_type);
		break;
	}
}

static int cb (struct nl_msg *m, void *ctx)
{
	struct nlmsghdr *h = nlmsg_hdr (m);
	struct rtmsg *rtm;
	struct rtattr *rta;
	int len;

	if (h->nlmsg_type != RTM_NEWROUTE &&
	    h->nlmsg_type != RTM_DELROUTE)
		return 0;

	rtm = nlmsg_data (h);

	if (rtm->rtm_family != AF_INET)
		return 0;

	printf ("route %s", h->nlmsg_type == RTM_NEWROUTE ? "add" : "del");

	for (
		rta = RTM_RTA (rtm), len = RTM_PAYLOAD (h);
		RTA_OK (rta, len);
		rta = RTA_NEXT (rta, len)
	)
		show_rta (rtm, rta);

	printf ("\n");

	return 0;
}

int main (void)
{
	if (nl_execute (cb, NETLINK_ROUTE, RTM_GETROUTE) < 0 ||
	    nl_monitor (cb, NETLINK_ROUTE, RTNLGRP_IPV4_ROUTE, 0) < 0) {
		nl_perror ("netlink monitor");
		return 1;
	}

	return 0;
}
