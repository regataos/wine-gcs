/* WinRT Windows.Media.Speech implementation
 *
 * Copyright 2021 Rémi Bernon for CodeWeavers
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winstring.h"
#include "wine/debug.h"
#include "objbase.h"

#include "initguid.h"
#include "activation.h"
#include "asyncinfo.h"

#define WIDL_using_Windows_Foundation
#define WIDL_using_Windows_Foundation_Collections
#include "windows.foundation.h"
#define WIDL_using_Windows_Media_SpeechSynthesis
#include "windows.media.speechsynthesis.h"

WINE_DEFAULT_DEBUG_CHANNEL(speech);

static const char *debugstr_hstring(HSTRING hstr)
{
    const WCHAR *str;
    UINT32 len;
    if (hstr && !((ULONG_PTR)hstr >> 16)) return "(invalid)";
    str = WindowsGetStringRawBuffer(hstr, &len);
    return wine_dbgstr_wn(str, len);
}

static ISpeechSynthesisStream SpeechSynthesisDummyStream_iface;
static IAsyncOperation_SpeechSynthesisStream AsyncOp_SpeechSynthDummyStream_iface;
static IAsyncInfo AsyncInfoDummyStream_iface;

/*
 * IAsyncInfo interface for dummy stream.
 */
static HRESULT WINAPI async_info_dummy_stream_QueryInterface(IAsyncInfo *iface,
        REFIID iid, void **out)
{
    FIXME("iface %p stub!\n", iface);
    return IAsyncOperation_SpeechSynthesisStream_QueryInterface(&AsyncOp_SpeechSynthDummyStream_iface, iid, out);
}

static ULONG WINAPI async_info_dummy_stream_AddRef(IAsyncInfo *iface)
{
    FIXME("\n");
    return 2;
}

static ULONG WINAPI async_info_dummy_stream_Release(IAsyncInfo *iface)
{
    FIXME("\n");
    return 2;
}

static HRESULT WINAPI async_info_dummy_stream_GetIids(IAsyncInfo *iface,
        ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI async_info_dummy_stream_GetRuntimeClassName(IAsyncInfo *iface,
        HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI async_info_dummy_stream_GetTrustLevel(IAsyncInfo *iface,
        TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI async_info_dummy_stream_get_Id(IAsyncInfo *iface,
        UINT32 *id)
{
    FIXME("iface %p, id %p stub!\n", iface, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI async_info_dummy_stream_get_Status(IAsyncInfo *iface,
        AsyncStatus *status)
{
    FIXME("iface %p, status %p stub!\n", iface, status);
    *status = 0;
    return S_OK;
}

static HRESULT WINAPI async_info_dummy_stream_get_ErrorCode(IAsyncInfo *iface,
        HRESULT *error_code)
{
    FIXME("iface %p, error code %p stub!\n", iface, error_code);
    return E_NOTIMPL;
}

static HRESULT WINAPI async_info_dummy_stream_Cancel(IAsyncInfo *iface)
{
    FIXME("iface %p stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI async_info_dummy_stream_Close(IAsyncInfo *iface)
{
    FIXME("iface %p stub!\n", iface);
    return E_NOTIMPL;
}

static const IAsyncInfoVtbl async_info_dummy_stream_vtbl = {
    async_info_dummy_stream_QueryInterface,
    async_info_dummy_stream_AddRef,
    async_info_dummy_stream_Release,
    async_info_dummy_stream_GetIids,
    async_info_dummy_stream_GetRuntimeClassName,
    async_info_dummy_stream_GetTrustLevel,
    async_info_dummy_stream_get_Id,
    async_info_dummy_stream_get_Status,
    async_info_dummy_stream_get_ErrorCode,
    async_info_dummy_stream_Cancel,
    async_info_dummy_stream_Close,
};

/*
 * ISpeechSynthesisStream interface for dummy stream.
 */
static HRESULT WINAPI speech_synth_dummy_stream_QueryInterface(ISpeechSynthesisStream *iface,
        REFIID iid, void **out)
{
    FIXME("iface %p stub!\n", iface);
    return IAsyncOperation_SpeechSynthesisStream_QueryInterface(&AsyncOp_SpeechSynthDummyStream_iface, iid, out);
}

static ULONG WINAPI speech_synth_dummy_stream_AddRef(ISpeechSynthesisStream *iface)
{
    FIXME("\n");
    return 2;
}

static ULONG WINAPI speech_synth_dummy_stream_Release(ISpeechSynthesisStream *iface)
{
    FIXME("\n");
    return 1;
}

static HRESULT WINAPI speech_synth_dummy_stream_GetIids(ISpeechSynthesisStream *iface,
        ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI speech_synth_dummy_stream_GetRuntimeClassName(ISpeechSynthesisStream *iface,
        HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI speech_synth_dummy_stream_GetTrustLevel(ISpeechSynthesisStream *iface,
        TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI speech_synth_dummy_stream_get_Markers(ISpeechSynthesisStream *iface, IVectorView_IMediaMarker **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);

    *value = NULL;

    return S_OK;
}

static const ISpeechSynthesisStreamVtbl speech_synth_dummy_stream_vtbl = {
    speech_synth_dummy_stream_QueryInterface,
    speech_synth_dummy_stream_AddRef,
    speech_synth_dummy_stream_Release,
    speech_synth_dummy_stream_GetIids,
    speech_synth_dummy_stream_GetRuntimeClassName,
    speech_synth_dummy_stream_GetTrustLevel,
    speech_synth_dummy_stream_get_Markers,
};

/*
 * IAsyncOperation_SpeechSynthesisStream interface for dummy stream.
 */
static HRESULT WINAPI speech_synth_dummy_stream_async_op_QueryInterface(IAsyncOperation_SpeechSynthesisStream *iface,
        REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p stub!\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_ISpeechSynthesisStream))
    {
        *out = &SpeechSynthesisDummyStream_iface;
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_IAsyncInfo))
    {
        *out = &AsyncInfoDummyStream_iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI speech_synth_dummy_stream_async_op_AddRef(IAsyncOperation_SpeechSynthesisStream *iface)
{
    FIXME("\n");
    return 2;
}

static ULONG WINAPI speech_synth_dummy_stream_async_op_Release(IAsyncOperation_SpeechSynthesisStream *iface)
{
    FIXME("\n");
    return 1;
}

static HRESULT WINAPI speech_synth_dummy_stream_async_op_GetIids(IAsyncOperation_SpeechSynthesisStream *iface,
        ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI speech_synth_dummy_stream_async_op_GetRuntimeClassName(IAsyncOperation_SpeechSynthesisStream *iface,
        HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI speech_synth_dummy_stream_async_op_GetTrustLevel(IAsyncOperation_SpeechSynthesisStream *iface,
        TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI speech_synth_dummy_stream_async_op_put_Completed(IAsyncOperation_SpeechSynthesisStream *iface,
        IAsyncOperationCompletedHandler_SpeechSynthesisStream *handler)
{
    FIXME("iface %p, handler %p stub!\n", iface, handler);
    return S_OK;
}

static HRESULT WINAPI speech_synth_dummy_stream_async_op_get_Completed(IAsyncOperation_SpeechSynthesisStream *iface,
        IAsyncOperationCompletedHandler_SpeechSynthesisStream **handler)
{
    FIXME("iface %p, handler %p stub!\n", iface, handler);
    return E_NOTIMPL;
}

static HRESULT WINAPI speech_synth_dummy_stream_async_op_GetResults(IAsyncOperation_SpeechSynthesisStream *iface,
        ISpeechSynthesisStream **results)
{
    FIXME("iface %p, results %p stub!\n", iface, results);
    *results = &SpeechSynthesisDummyStream_iface;
    return S_OK;
}

static const IAsyncOperation_SpeechSynthesisStreamVtbl speech_synth_dummy_stream_async_op_vtbl = {
    speech_synth_dummy_stream_async_op_QueryInterface,
    speech_synth_dummy_stream_async_op_AddRef,
    speech_synth_dummy_stream_async_op_Release,
    speech_synth_dummy_stream_async_op_GetIids,
    speech_synth_dummy_stream_async_op_GetRuntimeClassName,
    speech_synth_dummy_stream_async_op_GetTrustLevel,
    speech_synth_dummy_stream_async_op_put_Completed,
    speech_synth_dummy_stream_async_op_get_Completed,
    speech_synth_dummy_stream_async_op_GetResults,
};

static ISpeechSynthesisStream SpeechSynthesisDummyStream_iface = {&speech_synth_dummy_stream_vtbl};
static IAsyncOperation_SpeechSynthesisStream SpeechSynthesisDummyStream_AsyncOp_iface = {&speech_synth_dummy_stream_async_op_vtbl};
static IAsyncInfo AsyncInfoDummyStream_iface = {&async_info_dummy_stream_vtbl};

struct voice_information_vector
{
    IVectorView_VoiceInformation IVectorView_VoiceInformation_iface;
    LONG ref;
};

static inline struct voice_information_vector *impl_from_IVectorView_VoiceInformation(IVectorView_VoiceInformation *iface)
{
    return CONTAINING_RECORD(iface, struct voice_information_vector, IVectorView_VoiceInformation_iface);
}

static HRESULT STDMETHODCALLTYPE vector_view_voice_information_QueryInterface(
        IVectorView_VoiceInformation *iface, REFIID iid, void **out)
{
    TRACE("iface %p, iid %s, out %p stub!\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IVectorView_VoiceInformation))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE vector_view_voice_information_AddRef(
        IVectorView_VoiceInformation *iface)
{
    struct voice_information_vector *impl = impl_from_IVectorView_VoiceInformation(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %u.\n", iface, ref);
    return ref;
}

static ULONG STDMETHODCALLTYPE vector_view_voice_information_Release(
        IVectorView_VoiceInformation *iface)
{
    struct voice_information_vector *impl = impl_from_IVectorView_VoiceInformation(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %u.\n", iface, ref);
    return ref;
}

static HRESULT STDMETHODCALLTYPE vector_view_voice_information_GetIids(
        IVectorView_VoiceInformation *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE vector_view_voice_information_GetRuntimeClassName(
        IVectorView_VoiceInformation *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE vector_view_voice_information_GetTrustLevel(
        IVectorView_VoiceInformation *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE vector_view_voice_information_GetAt(
        IVectorView_VoiceInformation *iface, UINT32 index, IVoiceInformation **value)
{
    FIXME("iface %p, index %#x, value %p stub!\n", iface, index, value);
    *value = NULL;
    return E_BOUNDS;
}

static HRESULT STDMETHODCALLTYPE vector_view_voice_information_get_Size(
        IVectorView_VoiceInformation *iface, UINT32 *value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    *value = 0;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE vector_view_voice_information_IndexOf(
        IVectorView_VoiceInformation *iface, IVoiceInformation *element, UINT32 *index, BOOLEAN *found)
{
    FIXME("iface %p, element %p, index %p, found %p stub!\n", iface, element, index, found);
    *index = 0;
    *found = FALSE;
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE vector_view_voice_information_GetMany(
        IVectorView_VoiceInformation *iface, UINT32 start_index,
        UINT32 items_size, IVoiceInformation **items, UINT *value)
{
    FIXME("iface %p, start_index %#x, items %p, value %p stub!\n", iface, start_index, items, value);
    *value = 0;
    return S_OK;
}

static const struct IVectorView_VoiceInformationVtbl vector_view_voice_information_vtbl =
{
    vector_view_voice_information_QueryInterface,
    vector_view_voice_information_AddRef,
    vector_view_voice_information_Release,
    /* IInspectable methods */
    vector_view_voice_information_GetIids,
    vector_view_voice_information_GetRuntimeClassName,
    vector_view_voice_information_GetTrustLevel,
    /* IVectorView<VoiceInformation> methods */
    vector_view_voice_information_GetAt,
    vector_view_voice_information_get_Size,
    vector_view_voice_information_IndexOf,
    vector_view_voice_information_GetMany,
};

static struct voice_information_vector all_voices =
{
    {&vector_view_voice_information_vtbl},
    0
};

struct speech_synthesizer
{
    ISpeechSynthesizer ISpeechSynthesizer_iface;
    ISpeechSynthesizer2 ISpeechSynthesizer2_iface;
    IClosable IClosable_iface;
    LONG ref;
};

static inline struct speech_synthesizer *impl_from_ISpeechSynthesizer(ISpeechSynthesizer *iface)
{
    return CONTAINING_RECORD(iface, struct speech_synthesizer, ISpeechSynthesizer_iface);
}

static inline struct speech_synthesizer *impl_from_ISpeechSynthesizer2(ISpeechSynthesizer2 *iface)
{
    return CONTAINING_RECORD(iface, struct speech_synthesizer, ISpeechSynthesizer2_iface);
}

static inline struct speech_synthesizer *impl_from_IClosable(IClosable *iface)
{
    return CONTAINING_RECORD(iface, struct speech_synthesizer, IClosable_iface);
}

static HRESULT STDMETHODCALLTYPE speech_synthesizer_QueryInterface(
        ISpeechSynthesizer *iface, REFIID iid, void **out)
{
    struct speech_synthesizer *impl = impl_from_ISpeechSynthesizer(iface);

    TRACE("iface %p, iid %s, out %p stub!\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_ISpeechSynthesizer))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_ISpeechSynthesizer2))
    {
        IUnknown_AddRef(iface);
        *out = &impl->ISpeechSynthesizer2_iface;
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_IClosable))
    {
        IUnknown_AddRef(iface);
        *out = &impl->IClosable_iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE speech_synthesizer_AddRef(
        ISpeechSynthesizer *iface)
{
    struct speech_synthesizer *impl = impl_from_ISpeechSynthesizer(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);

    TRACE("iface %p, ref %u.\n", iface, ref);

    return ref;
}

static ULONG STDMETHODCALLTYPE speech_synthesizer_Release(
        ISpeechSynthesizer *iface)
{
    struct speech_synthesizer *impl = impl_from_ISpeechSynthesizer(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);

    TRACE("iface %p, ref %u.\n", iface, ref);

    if (!ref)
        free(impl);

    return ref;
}

static HRESULT STDMETHODCALLTYPE speech_synthesizer_GetIids(
        ISpeechSynthesizer *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub.\n", iface, iid_count, iids);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE speech_synthesizer_GetRuntimeClassName(
        ISpeechSynthesizer *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub.\n", iface, class_name);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE speech_synthesizer_GetTrustLevel(
        ISpeechSynthesizer *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub.\n", iface, trust_level);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE speech_synthesizer_SynthesizeTextToStreamAsync(ISpeechSynthesizer *iface,
        HSTRING text, IAsyncOperation_SpeechSynthesisStream **operation)
{
    FIXME("iface %p, text %s, operation %p stub.\n", iface, debugstr_hstring(text), operation);

    *operation = &SpeechSynthesisDummyStream_AsyncOp_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE speech_synthesizer_SynthesizeSsmlToStreamAsync(ISpeechSynthesizer *iface,
        HSTRING ssml, IAsyncOperation_SpeechSynthesisStream **operation)
{
    FIXME("iface %p, text %s, operation %p stub.\n", iface, debugstr_hstring(ssml), operation);

    *operation = &SpeechSynthesisDummyStream_AsyncOp_iface;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE speech_synthesizer_put_Voice(ISpeechSynthesizer *iface, IVoiceInformation *value)
{
    FIXME("iface %p, value %p stub.\n", iface, value);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE speech_synthesizer_get_Voice(ISpeechSynthesizer *iface, IVoiceInformation **value)
{
    FIXME("iface %p, value %p stub.\n", iface, value);

    return E_NOTIMPL;
}

static const struct ISpeechSynthesizerVtbl speech_synthesizer_vtbl =
{
    /* IUnknown methods */
    speech_synthesizer_QueryInterface,
    speech_synthesizer_AddRef,
    speech_synthesizer_Release,
    /* IInspectable methods */
    speech_synthesizer_GetIids,
    speech_synthesizer_GetRuntimeClassName,
    speech_synthesizer_GetTrustLevel,
    /* ISpeechSynthesizer methods */
    speech_synthesizer_SynthesizeTextToStreamAsync,
    speech_synthesizer_SynthesizeSsmlToStreamAsync,
    speech_synthesizer_put_Voice,
    speech_synthesizer_get_Voice,
};


static HRESULT STDMETHODCALLTYPE speech_synthesizer2_QueryInterface(
        ISpeechSynthesizer2 *iface, REFIID iid, void **out)
{
    struct speech_synthesizer *impl = impl_from_ISpeechSynthesizer2(iface);
    return speech_synthesizer_QueryInterface(&impl->ISpeechSynthesizer_iface, iid, out);
}

static ULONG STDMETHODCALLTYPE speech_synthesizer2_AddRef(
        ISpeechSynthesizer2 *iface)
{
    struct speech_synthesizer *impl = impl_from_ISpeechSynthesizer2(iface);
    return speech_synthesizer_AddRef(&impl->ISpeechSynthesizer_iface);
}

static ULONG STDMETHODCALLTYPE speech_synthesizer2_Release(
        ISpeechSynthesizer2 *iface)
{
    struct speech_synthesizer *impl = impl_from_ISpeechSynthesizer2(iface);
    return speech_synthesizer_Release(&impl->ISpeechSynthesizer_iface);
}

static HRESULT STDMETHODCALLTYPE speech_synthesizer2_GetIids(
        ISpeechSynthesizer2 *iface, ULONG *iid_count, IID **iids)
{
    struct speech_synthesizer *impl = impl_from_ISpeechSynthesizer2(iface);
    return speech_synthesizer_GetIids(&impl->ISpeechSynthesizer_iface, iid_count, iids);
}

static HRESULT STDMETHODCALLTYPE speech_synthesizer2_GetRuntimeClassName(
        ISpeechSynthesizer2 *iface, HSTRING *class_name)
{
    struct speech_synthesizer *impl = impl_from_ISpeechSynthesizer2(iface);
    return speech_synthesizer_GetRuntimeClassName(&impl->ISpeechSynthesizer_iface, class_name);
}

static HRESULT STDMETHODCALLTYPE speech_synthesizer2_GetTrustLevel(
        ISpeechSynthesizer2 *iface, TrustLevel *trust_level)
{
    struct speech_synthesizer *impl = impl_from_ISpeechSynthesizer2(iface);
    return speech_synthesizer_GetTrustLevel(&impl->ISpeechSynthesizer_iface, trust_level);
}

static HRESULT STDMETHODCALLTYPE speech_synthesizer2_get_Options(ISpeechSynthesizer2 *iface, ISpeechSynthesizerOptions **value)
{
    FIXME("iface %p, value %p stub.\n", iface, value);

    return E_NOTIMPL;
}

static const struct ISpeechSynthesizer2Vtbl speech_synthesizer2_vtbl =
{
    /* IUnknown methods */
    speech_synthesizer2_QueryInterface,
    speech_synthesizer2_AddRef,
    speech_synthesizer2_Release,
    /* IInspectable methods */
    speech_synthesizer2_GetIids,
    speech_synthesizer2_GetRuntimeClassName,
    speech_synthesizer2_GetTrustLevel,
    /* ISpeechSynthesizer2 methods */
    speech_synthesizer2_get_Options,
};

static HRESULT STDMETHODCALLTYPE closable_QueryInterface(
        IClosable *iface, REFIID iid, void **out)
{
    struct speech_synthesizer *impl = impl_from_IClosable(iface);

    return speech_synthesizer_QueryInterface(&impl->ISpeechSynthesizer_iface, iid, out);
}

static ULONG STDMETHODCALLTYPE closable_AddRef(
        IClosable *iface)
{
    struct speech_synthesizer *impl = impl_from_IClosable(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);

    TRACE("iface %p, ref %u.\n", iface, ref);

    return ref;
}

static ULONG STDMETHODCALLTYPE closable_Release(
        IClosable *iface)
{
    struct speech_synthesizer *impl = impl_from_IClosable(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);

    TRACE("iface %p, ref %u.\n", iface, ref);

    if (!ref)
        free(impl);

    return ref;
}

static HRESULT STDMETHODCALLTYPE closable_GetIids(
        IClosable *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub.\n", iface, iid_count, iids);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE closable_GetRuntimeClassName(
        IClosable *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub.\n", iface, class_name);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE closable_GetTrustLevel(
        IClosable *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub.\n", iface, trust_level);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE closable_Close(
        IClosable *iface)
{
    FIXME("iface %p stub.\n", iface);

    return E_NOTIMPL;
}

static const struct IClosableVtbl closable_vtbl =
{
    /* IUnknown methods */
    closable_QueryInterface,
    closable_AddRef,
    closable_Release,
    /* IInspectable methods */
    closable_GetIids,
    closable_GetRuntimeClassName,
    closable_GetTrustLevel,
    /* IClosable methods */
    closable_Close,
};

struct windows_media_speech
{
    IActivationFactory IActivationFactory_iface;
    IInstalledVoicesStatic IInstalledVoicesStatic_iface;
    LONG ref;
};

static inline struct windows_media_speech *impl_from_IActivationFactory(IActivationFactory *iface)
{
    return CONTAINING_RECORD(iface, struct windows_media_speech, IActivationFactory_iface);
}

static inline struct windows_media_speech *impl_from_IInstalledVoicesStatic(IInstalledVoicesStatic *iface)
{
    return CONTAINING_RECORD(iface, struct windows_media_speech, IInstalledVoicesStatic_iface);
}

static HRESULT STDMETHODCALLTYPE windows_media_speech_QueryInterface(
        IActivationFactory *iface, REFIID iid, void **out)
{
    struct windows_media_speech *impl = impl_from_IActivationFactory(iface);

    TRACE("iface %p, iid %s, out %p stub!\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject) ||
        IsEqualGUID(iid, &IID_IActivationFactory))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_IInstalledVoicesStatic))
    {
        IUnknown_AddRef(iface);
        *out = &impl->IInstalledVoicesStatic_iface;
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE windows_media_speech_AddRef(
        IActivationFactory *iface)
{
    struct windows_media_speech *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %u.\n", iface, ref);
    return ref;
}

static ULONG STDMETHODCALLTYPE windows_media_speech_Release(
        IActivationFactory *iface)
{
    struct windows_media_speech *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %u.\n", iface, ref);
    return ref;
}

static HRESULT STDMETHODCALLTYPE windows_media_speech_GetIids(
        IActivationFactory *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE windows_media_speech_GetRuntimeClassName(
        IActivationFactory *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE windows_media_speech_GetTrustLevel(
        IActivationFactory *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE windows_media_speech_ActivateInstance(
        IActivationFactory *iface, IInspectable **instance)
{
    struct speech_synthesizer *obj;

    TRACE("iface %p, instance %p.\n", iface, instance);

    if (!(obj = calloc(1, sizeof(*obj))))
    {
        *instance = NULL;
        return E_OUTOFMEMORY;
    }

    obj->ISpeechSynthesizer_iface.lpVtbl = &speech_synthesizer_vtbl;
    obj->ISpeechSynthesizer2_iface.lpVtbl = &speech_synthesizer2_vtbl;
    obj->IClosable_iface.lpVtbl = &closable_vtbl;
    obj->ref = 1;
    *instance = (IInspectable *)&obj->ISpeechSynthesizer_iface;
    return S_OK;
}

static const struct IActivationFactoryVtbl activation_factory_vtbl =
{
    windows_media_speech_QueryInterface,
    windows_media_speech_AddRef,
    windows_media_speech_Release,
    /* IInspectable methods */
    windows_media_speech_GetIids,
    windows_media_speech_GetRuntimeClassName,
    windows_media_speech_GetTrustLevel,
    /* IActivationFactory methods */
    windows_media_speech_ActivateInstance,
};

static HRESULT STDMETHODCALLTYPE installed_voices_static_QueryInterface(
        IInstalledVoicesStatic *iface, REFIID iid, void **out)
{
    struct windows_media_speech *impl = impl_from_IInstalledVoicesStatic(iface);
    return windows_media_speech_QueryInterface(&impl->IActivationFactory_iface, iid, out);
}

static ULONG STDMETHODCALLTYPE installed_voices_static_AddRef(
        IInstalledVoicesStatic *iface)
{
    struct windows_media_speech *impl = impl_from_IInstalledVoicesStatic(iface);
    return windows_media_speech_AddRef(&impl->IActivationFactory_iface);
}

static ULONG STDMETHODCALLTYPE installed_voices_static_Release(
        IInstalledVoicesStatic *iface)
{
    struct windows_media_speech *impl = impl_from_IInstalledVoicesStatic(iface);
    return windows_media_speech_Release(&impl->IActivationFactory_iface);
}

static HRESULT STDMETHODCALLTYPE installed_voices_static_GetIids(
        IInstalledVoicesStatic *iface, ULONG *iid_count, IID **iids)
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE installed_voices_static_GetRuntimeClassName(
        IInstalledVoicesStatic *iface, HSTRING *class_name)
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE installed_voices_static_GetTrustLevel(
        IInstalledVoicesStatic *iface, TrustLevel *trust_level)
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE installed_voices_static_get_AllVoices(
    IInstalledVoicesStatic *iface, IVectorView_VoiceInformation **value)
{
    TRACE("iface %p, value %p.\n", iface, value);
    *value = &all_voices.IVectorView_VoiceInformation_iface;
    IVectorView_VoiceInformation_AddRef(*value);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE installed_voices_static_get_DefaultVoice(
    IInstalledVoicesStatic *iface, IVoiceInformation **value)
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static const struct IInstalledVoicesStaticVtbl installed_voices_static_vtbl =
{
    installed_voices_static_QueryInterface,
    installed_voices_static_AddRef,
    installed_voices_static_Release,
    /* IInspectable methods */
    installed_voices_static_GetIids,
    installed_voices_static_GetRuntimeClassName,
    installed_voices_static_GetTrustLevel,
    /* IInstalledVoicesStatic methods */
    installed_voices_static_get_AllVoices,
    installed_voices_static_get_DefaultVoice,
};

static struct windows_media_speech windows_media_speech =
{
    {&activation_factory_vtbl},
    {&installed_voices_static_vtbl},
    1
};

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void **out)
{
    FIXME("clsid %s, riid %s, out %p stub!\n", debugstr_guid(clsid), debugstr_guid(riid), out);
    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI DllGetActivationFactory(HSTRING classid, IActivationFactory **factory)
{
    TRACE("classid %s, factory %p.\n", debugstr_hstring(classid), factory);

    if (wcscmp(WindowsGetStringRawBuffer(classid, NULL), L"Windows.Media.SpeechSynthesis.SpeechSynthesizer"))
    {
        ERR("Unknown classid %s.\n", debugstr_hstring(classid));
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    *factory = &windows_media_speech.IActivationFactory_iface;
    IUnknown_AddRef(*factory);
    return S_OK;
}
