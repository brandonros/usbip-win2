#include <cassert>
#include <string>

#include <initguid.h>

#include "usbip_common.h"
#include "usbip_setupdi.h"
#include "dbgcode.h"
#include "usbip_vhci.h"

namespace
{

struct Context
{
        GUID guid;
        std::string path;
};

int walker_devpath(HDEVINFO dev_info, SP_DEVINFO_DATA *data, devno_t, void *context)
{
        auto &ctx = *reinterpret_cast<Context*>(context);

        if (auto inf = get_intf_detail(dev_info, data, ctx.guid)) {
                ctx.path = inf->DevicePath;
                return true;
        }

        return false;
}

auto get_vhci_devpath(usbip_hci version)
{
        Context r{ usbip_guid(version) };
        traverse_intfdevs(walker_devpath, r.guid, &r);
        return r.path;
}

auto usbip_vhci_get_num_ports(HANDLE hdev, ioctl_usbip_vhci_get_num_ports &r)
{
        DWORD len = 0;

        if (DeviceIoControl(hdev, IOCTL_USBIP_VHCI_GET_NUM_PORTS, nullptr, 0, &r, sizeof(r), &len, nullptr)) {
                if (len == sizeof(r)) {
                        return ERR_NONE;
                }
        }

        return ERR_GENERAL;
}

int get_num_ports(HANDLE hdev)
{
        ioctl_usbip_vhci_get_num_ports r;
        auto err = usbip_vhci_get_num_ports(hdev, r);
        return err < 0 ? err : r.num_ports;
}

} // namespace


usbip::Handle usbip_vhci_driver_open(usbip_hci version)
{
        usbip::Handle h;

        auto devpath = get_vhci_devpath(version);
        if (devpath.empty()) {
                return h;
        }
        
        dbg("device path: %s", devpath.c_str());
        
        auto fh = CreateFile(devpath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
        h.reset(fh);

        return h;
}

std::vector<ioctl_usbip_vhci_imported_dev> usbip_vhci_get_imported_devs(HANDLE hdev)
{
        std::vector<ioctl_usbip_vhci_imported_dev> idevs;

        auto cnt = get_num_ports(hdev);
        if (cnt < 0) {
                dbg("failed to get the number of used ports: %s", dbg_errcode(cnt));
                return idevs;
        }

        idevs.resize(cnt + 1);
        auto idevs_bytes = DWORD(idevs.size()*sizeof(idevs[0]));

        if (!DeviceIoControl(hdev, IOCTL_USBIP_VHCI_GET_IMPORTED_DEVICES, nullptr, 0, idevs.data(), idevs_bytes, nullptr, nullptr)) {
                dbg("failed to get imported devices: 0x%lx", GetLastError());
                idevs.clear();
        }

        return idevs;
}

bool usbip_vhci_attach_device(HANDLE hdev, ioctl_usbip_vhci_plugin &r)
{
        auto ok = DeviceIoControl(hdev, IOCTL_USBIP_VHCI_PLUGIN_HARDWARE, &r, sizeof(r), &r, sizeof(r.port), nullptr, nullptr);
        if (!ok) {
                dbg("%s: DeviceIoControl error %#x", __func__, GetLastError());
        }
        return ok;
}

int usbip_vhci_detach_device(HANDLE hdev, int port)
{
        ioctl_usbip_vhci_unplug r{ port };

        if (DeviceIoControl(hdev, IOCTL_USBIP_VHCI_UNPLUG_HARDWARE, &r, sizeof(r), nullptr, 0, nullptr, nullptr)) {
                return 0;
        }

        auto err = GetLastError();
        dbg("%s: DeviceIoControl error %#x", __func__, err);

        switch (err) {
        case ERROR_FILE_NOT_FOUND:
                return ERR_NOTEXIST;
        case ERROR_INVALID_PARAMETER:
                return ERR_INVARG;
        default:
                return ERR_GENERAL;
        }
}
