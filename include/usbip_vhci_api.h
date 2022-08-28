#pragma once

#include <guiddef.h>

#ifdef _NTDDK_
  #include <ntddk.h>
#else
  #include <winioctl.h>
#endif

#include <usbspec.h>

#include "ch9.h"
#include "usbip_api_consts.h"
#include "usbip_proto.h"
//
// Define an Interface Guid for bus vhci class.
// This GUID is used to register (IoRegisterDeviceInterface) 
// an instance of an interface so that vhci application 
// can send an ioctl to the bus driver.
//

enum class usbip_hci { usb2, usb3 };

DEFINE_GUID(GUID_DEVINTERFACE_EHCI_USBIP,
        0xD35F7840, 0x6A0C, 0x11d2, 0xB8, 0x41, 0x00, 0xC0, 0x4F, 0xAD, 0x51, 0x71);

DEFINE_GUID(GUID_DEVINTERFACE_XHCI_USBIP,
        0xC1B20918, 0x5628, 0x42F8, 0xA6, 0xD4, 0xA9, 0x2C, 0x8C, 0xCE, 0xB1, 0x8F);

constexpr auto& usbip_guid(usbip_hci version)
{
        return version == usbip_hci::usb3 ? GUID_DEVINTERFACE_XHCI_USBIP : GUID_DEVINTERFACE_EHCI_USBIP;
}

//
// Define a Setup Class GUID for USBIP Class. This is same
// as the TOASTSER CLASS guid in the INF files.
//
DEFINE_GUID(GUID_DEVCLASS_USBIP,
        0xB85B7C50, 0x6A01, 0x11d2, 0xB8, 0x41, 0x00, 0xC0, 0x4F, 0xAD, 0x51, 0x71);
//{B85B7C50-6A01-11d2-B841-00C04FAD5171}

//
// Define a WMI GUID to get vhci info.
//
DEFINE_GUID(USBIP_BUS_WMI_STD_DATA_GUID, 
        0x0006A660, 0x8F12, 0x11d2, 0xB8, 0x54, 0x00, 0xC0, 0x4F, 0xAD, 0x51, 0x71);
//{0006A660-8F12-11d2-B854-00C04FAD5171}

//
// Define a WMI GUID to get USBIP device info.
//
DEFINE_GUID(USBIP_WMI_STD_DATA_GUID, 
        0xBBA21300, 0x6DD3, 0x11d2, 0xB8, 0x44, 0x00, 0xC0, 0x4F, 0xAD, 0x51, 0x71);
//{BBA21300-6DD3-11d2-B844-00C04FAD5171}

//
// Define a WMI GUID to represent device arrival notification WMIEvent class.
//
DEFINE_GUID(USBIP_NOTIFY_DEVICE_ARRIVAL_EVENT, 
        0x01CDAFF1, 0xC901, 0x45B4, 0xB3, 0x59, 0xB5, 0x54, 0x27, 0x25, 0xE2, 0x9C);
// {01CDAFF1-C901-45B4-B359-B5542725E29C}

#define USBIP_VHCI_IOCTL(_index_) \
    CTL_CODE(FILE_DEVICE_BUS_EXTENDER, _index_, METHOD_BUFFERED, FILE_READ_DATA)

#define IOCTL_USBIP_VHCI_PLUGIN_HARDWARE	USBIP_VHCI_IOCTL(0x0)
#define IOCTL_USBIP_VHCI_UNPLUG_HARDWARE	USBIP_VHCI_IOCTL(0x1)
// used by usbip_vhci.c
#define IOCTL_USBIP_VHCI_GET_NUM_PORTS	        USBIP_VHCI_IOCTL(0x2)
#define IOCTL_USBIP_VHCI_GET_IMPORTED_DEVICES	USBIP_VHCI_IOCTL(0x3)

struct ioctl_usbip_vhci_get_num_ports
{
	int num_ports;
};

struct ioctl_usbip_vhci_plugin
{
        int port; // OUT, must be the first member; port# if > 0 else err_t
        char busid[USBIP_BUS_ID_SIZE];
        char service[32]; // NI_MAXSERV
        char host[1025];  // NI_MAXHOST in ws2def.h
        char serial[255];
};

struct ioctl_usbip_vhci_imported_dev : ioctl_usbip_vhci_plugin
{
        usbip_device_status status;
        unsigned short vendor;
        unsigned short product;
        UINT32 devid;
        usb_device_speed speed;
};

struct ioctl_usbip_vhci_unplug
{
        int port;
};

