#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netlink/netlink.h>
#include <netlink/msg.h>

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

static int monitor (void)
{
	struct nl_handle *h;
	int ret = -1;

	if ((h = nl_handle_alloc ()) == NULL)
		goto no_handle;

	/* notifications do not use sequence numbers */
	nl_disable_sequence_check (h);

	nl_socket_modify_cb(h, NL_CB_VALID, NL_CB_CUSTOM, cb, NULL);

	nl_connect(h, NETLINK_ROUTE);

	nl_socket_add_membership (h, RTNLGRP_IPV4_ROUTE);

	while ((ret = nl_recvmsgs_default (h)) >= 0) {}

	nl_close (h);
	nl_handle_destroy (h);
no_handle:
	return ret;
}

int main (void)
{
	return monitor () < 0 ? 1 : 0;
}
