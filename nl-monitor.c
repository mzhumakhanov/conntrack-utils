#include <stdarg.h>

#include "nl-monitor.h"

/*
 * Function takes message callback, netlink type and zero-terminated list
 * of netlink groups
 */
int nl_monitor (nl_recvmsg_msg_cb_t cb, int type, ...)
{
	struct nl_handle *h;
	va_list ap;
	int group;
	int ret;

	if ((h = nl_handle_alloc ()) == NULL)
		return -1;

	/* notifications do not use sequence numbers */
	nl_disable_sequence_check (h);

	nl_socket_modify_cb(h, NL_CB_VALID, NL_CB_CUSTOM, cb, NULL);

	nl_connect(h, type);

	va_start (ap, type);

	while ((group = va_arg (ap, int)) != 0)
		nl_socket_add_membership (h, group);

	va_end (ap);

	while ((ret = nl_recvmsgs_default (h)) >= 0) {}

	nl_close (h);
	nl_handle_destroy (h);
no_handle:
	return ret;
}
