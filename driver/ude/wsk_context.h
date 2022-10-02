/*
 * Copyright (C) 2022 Vadym Hrynchyshyn <vadimgrn@gmail.com>
 */

#pragma once

#include <usbip\proto.h>
#include <libdrv\mdl_cpp.h>

namespace usbip
{

struct wsk_context
{
        // transient data

        IRP *irp; // can be NULL, see send_cmd_unlink
        Mdl mdl_buf; // describes URB_FROM_IRP(irp)->TransferBuffer(MDL)

        // preallocated data

        IRP *wsk_irp;

        Mdl mdl_hdr;
        usbip_header hdr;

        Mdl mdl_isoc;
        usbip_iso_packet_descriptor *isoc;
        ULONG isoc_alloc_cnt;
        bool is_isoc;
};

_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS init_wsk_context_list(_In_ ULONG tag);

_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
void delete_wsk_context_list();

_IRQL_requires_max_(DISPATCH_LEVEL)
wsk_context *alloc_wsk_context(_In_ ULONG NumberOfPackets);

_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS prepare_isoc(_In_ wsk_context &ctx, _In_ ULONG NumberOfPackets);

_IRQL_requires_max_(DISPATCH_LEVEL)
void free(_In_opt_ wsk_context *ctx, _In_ bool reuse);

_IRQL_requires_max_(DISPATCH_LEVEL)
inline void reuse(_In_ wsk_context &ctx)
{
        IoReuseIrp(ctx.wsk_irp, STATUS_UNSUCCESSFUL);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
inline auto number_of_packets(_In_ const wsk_context &ctx)
{
        return ctx.mdl_isoc.size()/sizeof(*ctx.isoc);
}

} // namespace usbip
