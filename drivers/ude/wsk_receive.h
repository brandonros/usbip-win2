﻿/*
 * Copyright (C) 2022 - 2023 Vadym Hrynchyshyn <vadimgrn@gmail.com>
 */

#pragma once

#include <libdrv\codeseg.h>
#include <libdrv/wdf_cpp.h>

namespace usbip
{

struct device_ctx;

_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
void complete(_In_ WDFREQUEST request, _In_ NTSTATUS status);

/*
 * ret_submit() set URB.UrbHeader.Status, atomic_complete set IRP.IoStatus.Status
 */
_IRQL_requires_same_
_IRQL_requires_max_(DISPATCH_LEVEL)
inline void complete(_In_ WDFREQUEST request)
{
        complete(request, WdfRequestGetStatus(request));
}

_IRQL_requires_same_
_IRQL_requires_(PASSIVE_LEVEL)
PAGED NTSTATUS init_receive_usbip_header(_In_ device_ctx &ctx);

} // namespace usbip
