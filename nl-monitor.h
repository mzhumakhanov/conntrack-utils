#ifndef _NL_MONITOR_H
#define _NL_MONITOR_H  1

#include <netlink/netlink.h>

/*
 * Function takes message callback, netlink type and zero-terminated list
 * of netlink groups
 */
int nl_monitor (nl_recvmsg_msg_cb_t cb, int type, ...);

#endif  /* _NL_MONITOR_H */
