/*
 * Copyright (C) 2012 Nicira, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 *
 * Authors:
 *     Dan Wendlandt <dan@nicira..com>
 *     Kyle Mestery <kmestery@cisco.com>
 */

#ifndef __VIR_NETDEV_OPENVSWITCH_H__
# define __VIR_NETDEV_OPENVSWITCH_H__

# include "internal.h"
# include "util.h"

#define VIR_NETDEV_OVS_BRNAME_LEN       64
#define VIR_NETDEV_OVS_INTERFACEID_LEN  64

/* Type of bridge interface */
enum virDomainNetBridgeType {
    VIR_DOMAIN_NET_INTERFACE_BRIDGE_TYPE_LINUX       = 0,
    VIR_DOMAIN_NET_INTERFACE_BRIDGE_TYPE_OPENVSWITCH,
};
typedef enum virDomainNetBridgeType virDomainNetBridgeType;

typedef struct _virNetDevOpenvswitchPort virNetDevOpenvswitchPort;
typedef virNetDevOpenvswitchPort *virNetDevOpenvswitchPortPtr;
struct _virNetDevOpenvswitchPort {
    char brname[VIR_NETDEV_OVS_BRNAME_LEN];
    char InterfaceID[VIR_NETDEV_OVS_INTERFACEID_LEN];
};

int virNetDevOpenvswitchAddPort(const char *brname, const char *ifname,
                                   const unsigned char *macaddr,
                                   virNetDevOpenvswitchPortPtr ovs_port)

    ATTRIBUTE_NONNULL(1) ATTRIBUTE_RETURN_CHECK;


#endif /* __VIR_NETDEV_OPENVSWITCH_H__ */
