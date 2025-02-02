/*
 * Copyright (C) 2022 - 2023 Vadym Hrynchyshyn <vadimgrn@gmail.com>
 */

#pragma once

#include "generic_handle.h"

#include <cassert>
#include <windows.h>

namespace usbip
{

struct HandleTag {};
using Handle = generic_handle<HANDLE, HandleTag, INVALID_HANDLE_VALUE>;

template<>
inline void close_handle(Handle::type h, Handle::tag_type) noexcept
{
        [[maybe_unused]] auto ok = CloseHandle(h);
        assert(ok);
}


struct HModuleTag {};
using HModule = generic_handle<HMODULE, HModuleTag, nullptr>;

template<>
inline void close_handle(_In_ HModule::type h, _In_ HModule::tag_type) noexcept
{
        [[maybe_unused]] auto ok = FreeLibrary(h);
        assert(ok);
}

} // namespace usbip
