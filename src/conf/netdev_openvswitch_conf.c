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
 *     Dan Wendlandt <dan@nicira.com>
 *     Kyle Mestery <kmestery@cisco.com>
 *     Ansis Atteka <aatteka@nicira.com>
 */

#include <config.h>

#include "netdev_openvswitch_conf.h"
#include "virterror_internal.h"
#include "memory.h"
#include "uuid.h"

#define VIR_FROM_THIS VIR_FROM_NONE
#define virNetDevError(code, ...)                                       \
    virReportErrorHelper(VIR_FROM_THIS, code, __FILE__,                 \
                         __FUNCTION__, __LINE__, __VA_ARGS__)


virNetDevOpenvswitchPortPtr
virNetDevOpenvswitchPortParse(xmlNodePtr node)
{
    char *InterfaceID = NULL;
    virNetDevOpenvswitchPortPtr ovsPort = NULL;
    xmlNodePtr cur = node->children;

    if (VIR_ALLOC(ovsPort) < 0) {
        virReportOOMError();
        goto error;
    }

    while (cur != NULL) {
        if (xmlStrEqual(cur->name, BAD_CAST "parameters")) {
            InterfaceID = virXMLPropString(cur, "interfaceid");
            break;
        }
        cur = cur->next;
    }

    if (InterfaceID == NULL || (strlen(InterfaceID) == 0)) {
        // interfaceID does not have to be a UUID,
        // but a UUID is a reasonable default
        if (virUUIDGenerateStr(ovsPort->InterfaceID)) {
            virNetDevError(VIR_ERR_XML_ERROR, "%s",
                        _("cannot generate a random uuid for interfaceid"));
            goto error;
        }
    } else {
        if (virStrcpyStatic(ovsPort->InterfaceID, InterfaceID) == NULL) {
            virNetDevError(VIR_ERR_XML_ERROR, "%s",
                           _("InterfaceID parameter too long"));
            goto error;
        }
    }

cleanup:
    return ovsPort;

error:
    VIR_FREE(ovsPort);
    goto cleanup;
}


int
virNetDevOpenvswitchPortFormat(virNetDevOpenvswitchPortPtr ovsPort,
                            virBufferPtr buf)
{
    if (ovsPort == NULL)
        return 0;

    virBufferAsprintf(buf, "<virtualport type='openvswitch'>\n");
    virBufferAsprintf(buf, "  <parameters interfaceid='%s'/>\n",
                      ovsPort->InterfaceID);
    virBufferAddLit(buf, "</virtualport>\n");
    return 0;
}
