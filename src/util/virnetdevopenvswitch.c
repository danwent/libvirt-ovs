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
 */

#include <config.h>

#include "virnetdevopenvswitch.h"
#include "command.h"
#include "memory.h"
#include "virterror_internal.h"
#include "ignore-value.h"

#define VIR_FROM_THIS VIR_FROM_NONE

/**
 * virNetDevOpenvswitchAddPort:
 * @brname: the bridge name
 * @ifname: the network interface name
 * @macaddr: the mac address of the virtual interface
 *
 * Adds an interface to an OVS bridge
 *
 * Returns 0 in case of success or an errno code in case of failure.
 */
int virNetDevOpenvswitchAddPort(const char *brname, const char *ifname,
                                   const unsigned char *macaddr,
                                   virNetDevOpenvswitchPortPtr ovs_port)
{
    int ret = -1;
    virCommandPtr cmd = NULL;
    char *attachedmac_ex_id = NULL;
    char *ifaceid_ex_id = NULL;

    if (virAsprintf(&attachedmac_ex_id, "external-ids:attached-mac=%s", macaddr) < 0)
        goto cleanup;
    if (virAsprintf(&ifaceid_ex_id, "external-ids:iface-id=%s", ovs_port->InterfaceID) < 0)
        goto cleanup;

    cmd = virCommandNew(OVSVSCTL);
    virCommandAddArgList(cmd, "--", "--may-exist", "add-port",
                        brname, ifname,
                        "--", "set", "Interface", ifname, attachedmac_ex_id,
                        "--", "set", "Interface", ifname, ifaceid_ex_id,
                        "--", "set", "Interface", ifname,
                        "external-ids:iface-status=active",
                        NULL);

    if (virCommandRun(cmd, NULL) < 0)
    {
        virReportSystemError(VIR_ERR_INTERNAL_ERROR,
                             _("Unable to add port %s to OVS bridge %s"), ifname, brname);
        goto cleanup;
    }
    ret = 0;

    cleanup:
        VIR_FREE(attachedmac_ex_id);
        VIR_FREE(ifaceid_ex_id);
        virCommandFree(cmd);
        return ret;
}

