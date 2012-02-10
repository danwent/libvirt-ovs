/*
 * Copyright (C) 2007-2011 Red Hat, Inc.
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
 *     Mark McLoughlin <markmc@redhat.com>
 *     Daniel P. Berrange <berrange@redhat.com>
 */

#include <config.h>

#include "virnetdevtap.h"
#include "virnetdev.h"
#include "virnetdevbridge.h"
#include "virnetdevopenvswitch.h"
#include "virterror_internal.h"
#include "virfile.h"
#include "virterror_internal.h"
#include "memory.h"
#include "logging.h"
#include "util.h"

#include <sys/ioctl.h>
#ifdef HAVE_NET_IF_H
# include <net/if.h>
#endif
#include <fcntl.h>
#ifdef __linux__
# include <linux/if_tun.h>    /* IFF_TUN, IFF_NO_PI */
#endif

#define VIR_FROM_THIS VIR_FROM_NONE

/**
 * virNetDevProbeVnetHdr:
 * @tapfd: a tun/tap file descriptor
 *
 * Check whether it is safe to enable the IFF_VNET_HDR flag on the
 * tap interface.
 *
 * Setting IFF_VNET_HDR enables QEMU's virtio_net driver to allow
 * guests to pass larger (GSO) packets, with partial checksums, to
 * the host. This greatly increases the achievable throughput.
 *
 * It is only useful to enable this when we're setting up a virtio
 * interface. And it is only *safe* to enable it when we know for
 * sure that a) qemu has support for IFF_VNET_HDR and b) the running
 * kernel implements the TUNGETIFF ioctl(), which qemu needs to query
 * the supplied tapfd.
 *
 * Returns 1 if VnetHdr is supported, 0 if not supported
 */
#ifdef IFF_VNET_HDR
static int
virNetDevProbeVnetHdr(int tapfd)
{
# if defined(IFF_VNET_HDR) && defined(TUNGETFEATURES) && defined(TUNGETIFF)
    unsigned int features;
    struct ifreq dummy;

    if (ioctl(tapfd, TUNGETFEATURES, &features) != 0) {
        VIR_INFO("Not enabling IFF_VNET_HDR; "
                 "TUNGETFEATURES ioctl() not implemented");
        return 0;
    }

    if (!(features & IFF_VNET_HDR)) {
        VIR_INFO("Not enabling IFF_VNET_HDR; "
                 "TUNGETFEATURES ioctl() reports no IFF_VNET_HDR");
        return 0;
    }

    /* The kernel will always return -1 at this point.
     * If TUNGETIFF is not implemented then errno == EBADFD.
     */
    if (ioctl(tapfd, TUNGETIFF, &dummy) != -1 || errno != EBADFD) {
        VIR_INFO("Not enabling IFF_VNET_HDR; "
                 "TUNGETIFF ioctl() not implemented");
        return 0;
    }

    VIR_INFO("Enabling IFF_VNET_HDR");

    return 1;
# else
    (void) tapfd;
    VIR_INFO("Not enabling IFF_VNET_HDR; disabled at build time");
    return 0;
# endif
}
#endif


#ifdef TUNSETIFF
/**
 * brCreateTap:
 * @ifname: the interface name
 * @vnet_hr: whether to try enabling IFF_VNET_HDR
 * @tapfd: file descriptor return value for the new tap device
 *
 * Creates a tap interface.
 * If the @tapfd parameter is supplied, the open tap device file
 * descriptor will be returned, otherwise the TAP device will be made
 * persistent and closed. The caller must use brDeleteTap to remove
 * a persistent TAP devices when it is no longer needed.
 *
 * Returns 0 in case of success or an errno code in case of failure.
 */
int virNetDevTapCreate(char **ifname,
                       int vnet_hdr ATTRIBUTE_UNUSED,
                       int *tapfd)
{
    int fd;
    struct ifreq ifr;
    int ret = -1;

    if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
        virReportSystemError(errno, "%s",
                             _("Unable to open /dev/net/tun, is tun module loaded?"));
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = IFF_TAP|IFF_NO_PI;

# ifdef IFF_VNET_HDR
    if (vnet_hdr && virNetDevProbeVnetHdr(fd))
        ifr.ifr_flags |= IFF_VNET_HDR;
# endif

    if (virStrcpyStatic(ifr.ifr_name, *ifname) == NULL) {
        virReportSystemError(ERANGE,
                             _("Network interface name '%s' is too long"),
                             *ifname);
        goto cleanup;

    }

    if (ioctl(fd, TUNSETIFF, &ifr) < 0) {
        virReportSystemError(errno,
                             _("Unable to create tap device %s"),
                             NULLSTR(*ifname));
        goto cleanup;
    }

    if (!tapfd &&
        (errno = ioctl(fd, TUNSETPERSIST, 1))) {
        virReportSystemError(errno,
                             _("Unable to set tap device %s to persistent"),
                             NULLSTR(*ifname));
        goto cleanup;
    }

    VIR_FREE(*ifname);
    if (!(*ifname = strdup(ifr.ifr_name))) {
        virReportOOMError();
        goto cleanup;
    }
    if (tapfd)
        *tapfd = fd;
    else
        VIR_FORCE_CLOSE(fd);

    ret = 0;

cleanup:
    if (ret < 0)
        VIR_FORCE_CLOSE(fd);

    return ret;
}


int virNetDevTapDelete(const char *ifname)
{
    struct ifreq try;
    int fd;
    int ret = -1;

    if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
        virReportSystemError(errno, "%s",
                             _("Unable to open /dev/net/tun, is tun module loaded?"));
        return -1;
    }

    memset(&try, 0, sizeof(struct ifreq));
    try.ifr_flags = IFF_TAP|IFF_NO_PI;

    if (virStrcpyStatic(try.ifr_name, ifname) == NULL) {
        virReportSystemError(ERANGE,
                             _("Network interface name '%s' is too long"),
                             ifname);
        goto cleanup;
    }

    if (ioctl(fd, TUNSETIFF, &try) < 0) {
        virReportSystemError(errno, "%s",
                             _("Unable to associate TAP device"));
        goto cleanup;
    }

    if (ioctl(fd, TUNSETPERSIST, 0) < 0) {
        virReportSystemError(errno, "%s",
                             _("Unable to make TAP device non-persistent"));
        goto cleanup;
    }

    ret = 0;

cleanup:
    VIR_FORCE_CLOSE(fd);
    return ret;
}
#else /* ! TUNSETIFF */
int virNetDevTapCreate(char **ifname ATTRIBUTE_UNUSED,
                       int vnet_hdr ATTRIBUTE_UNUSED,
                       int *tapfd ATTRIBUTE_UNUSED)
{
    virReportSystemError(ENOSYS, "%s",
                         _("Unable to create TAP devices on this platform"));
    return -1;
}
int virNetDevTapDelete(const char *ifname ATTRIBUTE_UNUSED)
{
    virReportSystemError(ENOSYS, "%s",
                         _("Unable to delete TAP devices on this platform"));
    return -1;
}
#endif /* ! TUNSETIFF */


/**
 * virNetDevTapCreateInBridgePort:
 * @brname: the bridge name
 * @ifname: the interface name (or name template)
 * @macaddr: desired MAC address (VIR_MAC_BUFLEN long)
 * @vnet_hdr: whether to try enabling IFF_VNET_HDR
 * @tapfd: file descriptor return value for the new tap device
 * @ovsport: Open vSwitch specific configuration
 *
 * This function creates a new tap device on a bridge. @ifname can be either
 * a fixed name or a name template with '%d' for dynamic name allocation.
 * in either case the final name for the bridge will be stored in @ifname.
 * If the @tapfd parameter is supplied, the open tap device file
 * descriptor will be returned, otherwise the TAP device will be made
 * persistent and closed. The caller must use brDeleteTap to remove
 * a persistent TAP devices when it is no longer needed.
 *
 * Returns 0 in case of success or -1 on failure
 */
int virNetDevTapCreateInBridgePort(const char *brname,
                                   char **ifname,
                                   const unsigned char *macaddr,
                                   int vnet_hdr,
                                   bool up,
                                   int *tapfd,
                                   virNetDevVPortProfilePtr ovsport)
{
    if (virNetDevTapCreate(ifname, vnet_hdr, tapfd) < 0)
        return -1;

    /* We need to set the interface MAC before adding it
     * to the bridge, because the bridge assumes the lowest
     * MAC of all enslaved interfaces & we don't want it
     * seeing the kernel allocate random MAC for the TAP
     * device before we set our static MAC.
     */
    if (virNetDevSetMAC(*ifname, macaddr) < 0)
        goto error;

    /* We need to set the interface MTU before adding it
     * to the bridge, because the bridge will have its
     * MTU adjusted automatically when we add the new interface.
     */
    if (virNetDevSetMTUFromDevice(*ifname, brname) < 0)
        goto error;

    if (ovsport) {
        if (virNetDevOpenvswitchAddPort(brname, *ifname, macaddr, ovsport) < 0)
            goto error;
    } else {
        if (virNetDevBridgeAddPort(brname, *ifname) < 0)
            goto error;
    }

    if (virNetDevSetOnline(*ifname, up) < 0)
        goto error;

    return 0;

 error:
    VIR_FORCE_CLOSE(*tapfd);

    return errno;
}

/**
 * virNetDevTapDeleteInBridgePort:
 * @ifname: the interface name (or name template)
 * @ovsport: Open vSwitch specific configuration
 *
 * This function detaches tap device from a bridge.
 *
 * Returns 0 in case of success or -1 on failure
 */
int virNetDevTapDeleteInBridgePort(char *ifname,
                                   virNetDevVPortProfilePtr ovsport)
{
    int ret = 0;
    if (ovsport)
        ret = virNetDevOpenvswitchRemovePort(ifname);
    return ret;
}
