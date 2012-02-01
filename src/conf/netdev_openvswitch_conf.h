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
 */

#ifndef __VIR_NETDEV_OPENVSWITCH_CONF_H__
# define __VIR_NETDEV_OPENVSWITCH_CONF_H__

# include "internal.h"
# include "virnetdevopenvswitch.h"
# include "buf.h"
# include "xml.h"

virNetDevOpenvswitchPortPtr
virNetDevOpenvswitchPortParse(xmlNodePtr node);

int
virNetDevOpenvswitchPortFormat(virNetDevOpenvswitchPortPtr ovsPort,
                            virBufferPtr buf);


#endif /* __VIR_NETDEV_OPENVSWITCH_CONF_H__ */
