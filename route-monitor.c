#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netlink/netlink.h>
#include <netlink/msg.h>

#include "nl-monitor.h"
#include "rt-label.h"

static void show_proto (unsigned char index)
{
	const char *label = rt_proto (index);

	if (index == RTPROT_KERNEL)
		return;

	if (label != NULL)
		printf (" proto %s", label);
	else
		printf (" proto %u", index);
}

static void show_scope (unsigned char index)
{
	const char *label = rt_scope (index);

	if (index == RT_SCOPE_UNIVERSE)
		return;

	if (label != NULL)
		printf (" scope %s", label);
	else
		printf (" scope %u", index);
}

static void show_table (unsigned char index)
{
	const char *label = rt_table (index);

	if (index == RT_TABLE_MAIN)
		return;

	if (label != NULL)
		printf (" table %s", label);
	else
		printf (" table %u", index);
}

static void addr_show_rta (struct ifaddrmsg *ifa, struct rtattr *rta)
{
	char buf[INET6_ADDRSTRLEN];
	const char *type = "", *p;

	switch (rta->rta_type) {
	case IFA_LOCAL:		type = "local";		break;
	case IFA_BROADCAST:	type = "broadcast";	break;
	case IFA_ANYCAST:	type = "anycast";	break;
	case IFA_MULTICAST:	type = "multicast";	break;
	}

	switch (rta->rta_type) {
	case IFA_LABEL:
		printf (" label %s", (const char *) RTA_DATA (rta));
		break;
	case IFA_ADDRESS:
		p = inet_ntop (ifa->ifa_family, RTA_DATA (rta),
			       buf, sizeof (buf));

		printf (" address %s/%d", p, ifa->ifa_prefixlen);
		break;
	case IFA_LOCAL:
	case IFA_BROADCAST:
	case IFA_ANYCAST:
	case IFA_MULTICAST:
		p = inet_ntop (ifa->ifa_family, RTA_DATA (rta),
			       buf, sizeof (buf));

		printf (" %s %s", type, p);
		break;
	case IFA_CACHEINFO:
		/* ignore it */
		break;
	default:
		printf (" type %d", rta->rta_type);
		break;
	}
}

static int process_addr (struct nlmsghdr *h, struct ifaddrmsg *ifa, void *ctx)
{
	struct rtattr *rta;
	int len;

	if (ifa->ifa_family != AF_INET && ifa->ifa_family != AF_INET6)
		return 0;

	printf ("address %s", h->nlmsg_type == RTM_NEWADDR ? "add" : "del");

	for (
		rta = IFA_RTA (ifa), len = IFA_PAYLOAD (h);
		RTA_OK (rta, len);
		rta = RTA_NEXT (rta, len)
	)
		addr_show_rta (ifa, rta);

	printf (" dev %d", ifa->ifa_index);
	show_scope (ifa->ifa_scope);
	printf ("\n");

	return 0;
}

static void route_show_rta (struct rtmsg *rtm, struct rtattr *rta)
{
	char buf[INET6_ADDRSTRLEN];
	const char *p;

	switch (rta->rta_type) {
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
	case RTA_CACHEINFO:
		/* ignore it */
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

	if (rtm->rtm_table == RT_TABLE_LOCAL)
		return 0;

	printf ("route %s", h->nlmsg_type == RTM_NEWROUTE ? "add" : "del");

	for (
		rta = RTM_RTA (rtm), len = RTM_PAYLOAD (h);
		RTA_OK (rta, len);
		rta = RTA_NEXT (rta, len)
	)
		route_show_rta (rtm, rta);

	show_table (rtm->rtm_table);
	show_proto (rtm->rtm_protocol);
	show_scope (rtm->rtm_scope);
	printf ("\n");

	return 0;
}

static int cb (struct nl_msg *m, void *ctx)
{
	struct nlmsghdr *h = nlmsg_hdr (m);

	switch (h->nlmsg_type) {
	case RTM_NEWADDR:
	case RTM_DELADDR:
		return process_addr (h, nlmsg_data (h), ctx);
	case RTM_NEWROUTE:
	case RTM_DELROUTE:
		return process_route (h, nlmsg_data (h), ctx);
	}

	return 0;
}

int main (void)
{
	if (nl_execute (cb, NETLINK_ROUTE, RTM_GETADDR) < 0 ||
	    nl_execute (cb, NETLINK_ROUTE, RTM_GETROUTE) < 0 ||
	    nl_monitor (cb, NETLINK_ROUTE, RTNLGRP_IPV4_ROUTE,
					   RTNLGRP_IPV6_ROUTE,
					   RTNLGRP_IPV4_IFADDR,
					   RTNLGRP_IPV6_IFADDR, 0) < 0) {
		nl_perror ("netlink monitor");
		return 1;
	}

	return 0;
}
