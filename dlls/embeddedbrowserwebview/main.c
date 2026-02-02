/*
 * WebView2 entry point
 * Copyright (C) 2024 Bernhard Kölbl for Codeweavers
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
 * License along with this library; if not, see <https://www.gnu.org/licenses/>.
 */

#include <stdarg.h>

#include "windef.h"

#include "initguid.h"
#define COBJMACROS
#include "objbase.h"

#include "webview2.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(webview2);

/******************************************************************
 *      CreateWebViewEnvironmentWithOptionsInternal (EMBEDDEDBROWSERWEBVIEW.6)
 */
HRESULT WINAPI CreateWebViewEnvironmentWithOptionsInternal ( UINT unk_arg1, PCWSTR browserExecutableFolder, PCWSTR userDataFolder,
    ICoreWebView2EnvironmentOptions* environmentOptions, ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler* environmentCreatedHandler )
{
    FIXME("unk_arg1 %u, browserExecutableFolder %s, userDataFolder %s, environmentOptions %p, environmentCreatedHandler %p stub!\n",
        unk_arg1, debugstr_w(browserExecutableFolder), debugstr_w(userDataFolder), environmentOptions, environmentCreatedHandler);

    return E_NOTIMPL;
}
