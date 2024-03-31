/* WinRT Windows.Media.SpeechRecognition implementation
 *
 * Copyright 2022 Bernhard Kölbl
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

#include "private.h"

#include "initguid.h"
#include "audioclient.h"
#include "mmdeviceapi.h"

#include "wine/debug.h"

#include "unixlib.h"
#include "wine/unixlib.h"

WINE_DEFAULT_DEBUG_CHANNEL(speech);

static const char *debugstr_hstring(HSTRING hstr)
{
    const WCHAR *str;
    UINT32 len;
    if (hstr && !((ULONG_PTR)hstr >> 16)) return "(invalid)";
    str = WindowsGetStringRawBuffer(hstr, &len);
    return wine_dbgstr_wn(str, len);
}

struct map_view_hstring_vector_view_hstring
{
    IMapView_HSTRING_IVectorView_HSTRING IMapView_HSTRING_IVectorView_HSTRING_iface;
    LONG ref;
};

static inline struct map_view_hstring_vector_view_hstring *impl_from_IMapView_HSTRING_IVectorView_HSTRING( IMapView_HSTRING_IVectorView_HSTRING *iface )
{
    return CONTAINING_RECORD(iface, struct map_view_hstring_vector_view_hstring, IMapView_HSTRING_IVectorView_HSTRING_iface);
}

HRESULT WINAPI map_view_hstring_vector_view_hstring_QueryInterface( IMapView_HSTRING_IVectorView_HSTRING *iface, REFIID iid, void **out )
{
    struct map_view_hstring_vector_view_hstring *impl = impl_from_IMapView_HSTRING_IVectorView_HSTRING(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IMapView_HSTRING_IVectorView_HSTRING))
    {
        IInspectable_AddRef((*out = &impl->IMapView_HSTRING_IVectorView_HSTRING_iface));
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

ULONG WINAPI map_view_hstring_vector_view_hstring_AddRef( IMapView_HSTRING_IVectorView_HSTRING *iface )
{
    struct map_view_hstring_vector_view_hstring *impl = impl_from_IMapView_HSTRING_IVectorView_HSTRING(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

ULONG WINAPI map_view_hstring_vector_view_hstring_Release( IMapView_HSTRING_IVectorView_HSTRING *iface )
{
    struct map_view_hstring_vector_view_hstring *impl = impl_from_IMapView_HSTRING_IVectorView_HSTRING(iface);

    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);

    if(!ref)
        free(impl);

    return ref;
}

HRESULT WINAPI map_view_hstring_vector_view_hstring_GetIids( IMapView_HSTRING_IVectorView_HSTRING *iface, ULONG *iidCount, IID **iids )
{
    FIXME("iface %p stub!\n", iface);
    return E_NOTIMPL;
}

HRESULT WINAPI map_view_hstring_vector_view_hstring_GetRuntimeClassName( IMapView_HSTRING_IVectorView_HSTRING *iface, HSTRING *className )
{
    FIXME("iface %p stub!\n", iface);
    return E_NOTIMPL;
}

HRESULT WINAPI map_view_hstring_vector_view_hstring_GetTrustLevel( IMapView_HSTRING_IVectorView_HSTRING *iface, TrustLevel *trustLevel )
{
    FIXME("iface %p stub!\n", iface);
    return E_NOTIMPL;
}

HRESULT WINAPI map_view_hstring_vector_view_hstring_Lookup( IMapView_HSTRING_IVectorView_HSTRING *iface, HSTRING key, IVectorView_HSTRING **value )
{
    FIXME("iface %p stub!\n", iface);
    return E_NOTIMPL;
}

HRESULT WINAPI map_view_hstring_vector_view_hstring_get_Size( IMapView_HSTRING_IVectorView_HSTRING *iface, unsigned int *size )
{
    FIXME("iface %p stub!\n", iface);
    *size = 0;
    return S_OK;
}

HRESULT WINAPI map_view_hstring_vector_view_hstring_HasKey( IMapView_HSTRING_IVectorView_HSTRING *iface, HSTRING key, boolean *found )
{
    FIXME("iface %p stub!\n", iface);
    return E_NOTIMPL;
}

HRESULT WINAPI map_view_hstring_vector_view_hstring_Split( IMapView_HSTRING_IVectorView_HSTRING *iface, IMapView_HSTRING_IVectorView_HSTRING **first, IMapView_HSTRING_IVectorView_HSTRING **second )
{
    FIXME("iface %p stub!\n", iface);
    return E_NOTIMPL;
}

static const struct IMapView_HSTRING_IVectorView_HSTRINGVtbl map_view_hstring_vector_view_hstring_vtbl =
{
    /* IUnknown methods */
    map_view_hstring_vector_view_hstring_QueryInterface,
    map_view_hstring_vector_view_hstring_AddRef,
    map_view_hstring_vector_view_hstring_Release,
    /* IInspectable methods */
    map_view_hstring_vector_view_hstring_GetIids,
    map_view_hstring_vector_view_hstring_GetRuntimeClassName,
    map_view_hstring_vector_view_hstring_GetTrustLevel,
    /* IMapView<HSTRING,IVectorView<HSTRING >* > methods */
    map_view_hstring_vector_view_hstring_Lookup,
    map_view_hstring_vector_view_hstring_get_Size,
    map_view_hstring_vector_view_hstring_HasKey,
    map_view_hstring_vector_view_hstring_Split
};


static HRESULT map_view_hstring_vector_view_hstring_create( IMapView_HSTRING_IVectorView_HSTRING **out )
{
    struct map_view_hstring_vector_view_hstring *impl;

    TRACE("out %p.\n", out);

    if (!(impl = calloc(1, sizeof(*impl))))
    {
        *out = NULL;
        return E_OUTOFMEMORY;
    }

    impl->IMapView_HSTRING_IVectorView_HSTRING_iface.lpVtbl = &map_view_hstring_vector_view_hstring_vtbl;
    impl->ref = 1;

    *out = &impl->IMapView_HSTRING_IVectorView_HSTRING_iface;
    TRACE("created %p\n", *out);
    return S_OK;
}

struct semantic_interpretation
{
    ISpeechRecognitionSemanticInterpretation ISpeechRecognitionSemanticInterpretation_iface;
    LONG ref;
};

static inline struct semantic_interpretation *impl_from_ISpeechRecognitionSemanticInterpretation( ISpeechRecognitionSemanticInterpretation *iface )
{
    return CONTAINING_RECORD(iface, struct semantic_interpretation, ISpeechRecognitionSemanticInterpretation_iface);
}

HRESULT WINAPI semantic_interpretation_QueryInterface( ISpeechRecognitionSemanticInterpretation *iface, REFIID iid, void **out )
{
    struct semantic_interpretation *impl = impl_from_ISpeechRecognitionSemanticInterpretation(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_ISpeechRecognitionSemanticInterpretation))
    {
        IInspectable_AddRef((*out = &impl->ISpeechRecognitionSemanticInterpretation_iface));
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

ULONG WINAPI semantic_interpretation_AddRef( ISpeechRecognitionSemanticInterpretation *iface )
{
    struct semantic_interpretation *impl = impl_from_ISpeechRecognitionSemanticInterpretation(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

ULONG WINAPI semantic_interpretation_Release( ISpeechRecognitionSemanticInterpretation *iface )
{
    struct semantic_interpretation *impl = impl_from_ISpeechRecognitionSemanticInterpretation(iface);

    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);

    if(!ref)
        free(impl);

    return ref;
}

HRESULT WINAPI semantic_interpretation_GetIids( ISpeechRecognitionSemanticInterpretation *iface, ULONG *iid_count, IID **iids )
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

HRESULT WINAPI semantic_interpretation_GetRuntimeClassName( ISpeechRecognitionSemanticInterpretation *iface, HSTRING *class_name )
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

HRESULT WINAPI semantic_interpretation_GetTrustLevel( ISpeechRecognitionSemanticInterpretation *iface, TrustLevel *trust_level )
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

HRESULT WINAPI semantic_interpretation_get_Properties( ISpeechRecognitionSemanticInterpretation *iface, IMapView_HSTRING_IVectorView_HSTRING **value )
{
    FIXME("iface %p stub!\n", iface);
    return map_view_hstring_vector_view_hstring_create(value);
}

static const struct ISpeechRecognitionSemanticInterpretationVtbl semantic_interpretation_vtbl =
{
    /* IUnknown methods */
    semantic_interpretation_QueryInterface,
    semantic_interpretation_AddRef,
    semantic_interpretation_Release,
    /* IInspectable methods */
    semantic_interpretation_GetIids,
    semantic_interpretation_GetRuntimeClassName,
    semantic_interpretation_GetTrustLevel,
    /* ISpeechRecognitionSemanticInterpretation methods */
    semantic_interpretation_get_Properties
};


static HRESULT semantic_interpretation_create( ISpeechRecognitionSemanticInterpretation **out )
{
    struct semantic_interpretation *impl;

    TRACE("out %p.\n", out);

    if (!(impl = calloc(1, sizeof(*impl))))
    {
        *out = NULL;
        return E_OUTOFMEMORY;
    }

    impl->ISpeechRecognitionSemanticInterpretation_iface.lpVtbl = &semantic_interpretation_vtbl;
    impl->ref = 1;

    *out = &impl->ISpeechRecognitionSemanticInterpretation_iface;
    TRACE("created %p\n", *out);
    return S_OK;
}

struct recognition_result
{
    ISpeechRecognitionResult ISpeechRecognitionResult_iface;
    ISpeechRecognitionResult2 ISpeechRecognitionResult2_iface;
    LONG ref;

    ISpeechRecognitionConstraint *constraint;
    HSTRING text;
};

static inline struct recognition_result *impl_from_ISpeechRecognitionResult( ISpeechRecognitionResult *iface )
{
    return CONTAINING_RECORD(iface, struct recognition_result, ISpeechRecognitionResult_iface);
}

static HRESULT WINAPI recognition_result_QueryInterface( ISpeechRecognitionResult *iface, REFIID iid, void **out )
{
    struct recognition_result *impl = impl_from_ISpeechRecognitionResult(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject) ||
        IsEqualGUID(iid, &IID_ISpeechRecognitionResult))
    {
        IInspectable_AddRef((*out = &impl->ISpeechRecognitionResult_iface));
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_ISpeechRecognitionResult2))
    {
        IInspectable_AddRef((*out = &impl->ISpeechRecognitionResult2_iface));
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI recognition_result_AddRef( ISpeechRecognitionResult *iface )
{
    struct recognition_result *impl = impl_from_ISpeechRecognitionResult(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI recognition_result_Release( ISpeechRecognitionResult *iface )
{
    struct recognition_result *impl = impl_from_ISpeechRecognitionResult(iface);

    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);

    if(!ref)
    {
        ISpeechRecognitionConstraint_Release(impl->constraint);
        WindowsDeleteString(impl->text);
        free(impl);
    }

    return ref;
}

static HRESULT WINAPI recognition_result_GetIids( ISpeechRecognitionResult *iface, ULONG *iid_count, IID **iids )
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognition_result_GetRuntimeClassName( ISpeechRecognitionResult *iface, HSTRING *class_name )
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognition_result_GetTrustLevel( ISpeechRecognitionResult *iface, TrustLevel *trust_level )
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognition_result_get_Status( ISpeechRecognitionResult *iface, SpeechRecognitionResultStatus *value )
{
    FIXME("iface %p, operation %p stub!\n", iface, value);
    *value = SpeechRecognitionResultStatus_Success;
    return S_OK;
}

static HRESULT WINAPI recognition_result_get_Text( ISpeechRecognitionResult *iface, HSTRING *value )
{
    struct recognition_result *impl = impl_from_ISpeechRecognitionResult(iface);
    TRACE("iface %p, operation %p, text: %s.\n", iface, value, debugstr_hstring(impl->text));
    return WindowsDuplicateString(impl->text, value);
}

static HRESULT WINAPI recognition_result_get_Confidence( ISpeechRecognitionResult *iface, SpeechRecognitionConfidence *value )
{
    FIXME("iface %p, operation %p semi stub!\n", iface, value);
    *value = SpeechRecognitionConfidence_High;
    return S_OK;
}

static HRESULT WINAPI recognition_result_get_SemanticInterpretation( ISpeechRecognitionResult *iface,
                                                                     ISpeechRecognitionSemanticInterpretation **value )
{
    FIXME("iface %p, operation %p stub!\n", iface, value);
    return semantic_interpretation_create(value);
}

static HRESULT WINAPI recognition_result_GetAlternates( ISpeechRecognitionResult *iface,
                                                        UINT32 max_amount,
                                                        IVectorView_SpeechRecognitionResult **results )
{
    IVector_IInspectable *vector;
    struct vector_iids constraints_iids =
    {
        .iterable = &IID_IVectorView_SpeechRecognitionResult,
        .iterator = &IID_IVectorView_SpeechRecognitionResult,
        .vector = &IID_IVector_IInspectable,
        .view = &IID_IVectorView_SpeechRecognitionResult,
    };

    FIXME("iface %p, max_amount %u, results %p stub!\n", iface, max_amount, results);

    vector_inspectable_create(&constraints_iids, (IVector_IInspectable **)&vector);
    IVector_IInspectable_GetView(vector, (IVectorView_IInspectable **)results);
    IVector_IInspectable_Release(vector);
    return S_OK;
}

static HRESULT WINAPI recognition_result_get_Constraint( ISpeechRecognitionResult *iface, ISpeechRecognitionConstraint **value )
{
    struct recognition_result *impl = impl_from_ISpeechRecognitionResult(iface);
    TRACE("iface %p, operation %p.\n", iface, value);
    ISpeechRecognitionConstraint_AddRef((*value = impl->constraint));
    return S_OK;
}

static HRESULT WINAPI recognition_result_get_RulePath( ISpeechRecognitionResult *iface, IVectorView_HSTRING **value )
{
    FIXME("iface %p stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognition_result_get_RawConfidence( ISpeechRecognitionResult *iface, DOUBLE *value )
{
    FIXME("iface %p stub!\n", iface);
    return E_NOTIMPL;
}

static const struct ISpeechRecognitionResultVtbl recognition_result_vtbl =
{
    /* IUnknown methods */
    recognition_result_QueryInterface,
    recognition_result_AddRef,
    recognition_result_Release,
    /* IInspectable methods */
    recognition_result_GetIids,
    recognition_result_GetRuntimeClassName,
    recognition_result_GetTrustLevel,
    /* ISpeechRecognitionResult methods */
    recognition_result_get_Status,
    recognition_result_get_Text,
    recognition_result_get_Confidence,
    recognition_result_get_SemanticInterpretation,
    recognition_result_GetAlternates,
    recognition_result_get_Constraint,
    recognition_result_get_RulePath,
    recognition_result_get_RawConfidence
};

DEFINE_IINSPECTABLE(recognition_result2, ISpeechRecognitionResult2, struct recognition_result, ISpeechRecognitionResult_iface)

static HRESULT WINAPI recognition_result2_get_PhraseStartTime( ISpeechRecognitionResult2 *iface, DateTime *value )
{
    DateTime dt = { .UniversalTime = 0 };
    FIXME("iface %p, value %p stub!\n", iface, value);
    *value = dt;
    return S_OK;
}


static HRESULT WINAPI recognition_result2_get_PhraseDuration( ISpeechRecognitionResult2 *iface, TimeSpan *value )
{
    TimeSpan ts = { .Duration = 50000000LL }; /* Use 5 seconds as stub value. */
    FIXME("iface %p, value %p stub!\n", iface, value);
    *value = ts;
    return S_OK;
}

static const struct ISpeechRecognitionResult2Vtbl recognition_result2_vtbl =
{
    /* IUnknown methods */
    recognition_result2_QueryInterface,
    recognition_result2_AddRef,
    recognition_result2_Release,
    /* IInspectable methods */
    recognition_result2_GetIids,
    recognition_result2_GetRuntimeClassName,
    recognition_result2_GetTrustLevel,
    /* ISpeechRecognitionResult2 methods */
    recognition_result2_get_PhraseStartTime,
    recognition_result2_get_PhraseDuration
};

static HRESULT WINAPI recognition_result_create( ISpeechRecognitionConstraint *constraint,
                                                 HSTRING result_text,
                                                 ISpeechRecognitionResult **out )
{
    struct recognition_result *impl;

    TRACE("out %p.\n", out);

    if (!(impl = calloc(1, sizeof(*impl))))
    {
        *out = NULL;
        return E_OUTOFMEMORY;
    }

    impl->ISpeechRecognitionResult_iface.lpVtbl = &recognition_result_vtbl;
    impl->ISpeechRecognitionResult2_iface.lpVtbl = &recognition_result2_vtbl;
    impl->ref = 1;

    if (constraint) ISpeechRecognitionConstraint_AddRef((impl->constraint = constraint));
    WindowsDuplicateString(result_text, &impl->text);

    *out = &impl->ISpeechRecognitionResult_iface;

    TRACE("created %p.\n", *out);

    return S_OK;
}

struct recognition_result_event_args
{
    ISpeechContinuousRecognitionResultGeneratedEventArgs ISpeechContinuousRecognitionResultGeneratedEventArgs_iface;
    LONG ref;

    ISpeechRecognitionResult *result;
};

static inline struct recognition_result_event_args *impl_from_ISpeechContinuousRecognitionResultGeneratedEventArgs( ISpeechContinuousRecognitionResultGeneratedEventArgs *iface )
{
    return CONTAINING_RECORD(iface, struct recognition_result_event_args, ISpeechContinuousRecognitionResultGeneratedEventArgs_iface);
}

static HRESULT WINAPI recognition_result_event_args_QueryInterface( ISpeechContinuousRecognitionResultGeneratedEventArgs *iface, REFIID iid, void **out )
{
    struct recognition_result_event_args *impl = impl_from_ISpeechContinuousRecognitionResultGeneratedEventArgs(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject) ||
        IsEqualGUID(iid, &IID_ISpeechContinuousRecognitionResultGeneratedEventArgs))
    {
        IInspectable_AddRef((*out = &impl->ISpeechContinuousRecognitionResultGeneratedEventArgs_iface));
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI recognition_result_event_args_AddRef( ISpeechContinuousRecognitionResultGeneratedEventArgs *iface )
{
    struct recognition_result_event_args *impl = impl_from_ISpeechContinuousRecognitionResultGeneratedEventArgs(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI recognition_result_event_args_Release( ISpeechContinuousRecognitionResultGeneratedEventArgs *iface )
{
    struct recognition_result_event_args *impl = impl_from_ISpeechContinuousRecognitionResultGeneratedEventArgs(iface);

    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);

    if (!ref)
    {
        if (impl->result) ISpeechRecognitionResult_Release(impl->result);
        free(impl);
    }

    return ref;
}

static HRESULT WINAPI recognition_result_event_args_GetIids( ISpeechContinuousRecognitionResultGeneratedEventArgs *iface, ULONG *iid_count, IID **iids )
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognition_result_event_args_GetRuntimeClassName( ISpeechContinuousRecognitionResultGeneratedEventArgs *iface, HSTRING *class_name )
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognition_result_event_args_GetTrustLevel( ISpeechContinuousRecognitionResultGeneratedEventArgs *iface, TrustLevel *trust_level )
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognition_result_event_args_get_Result( ISpeechContinuousRecognitionResultGeneratedEventArgs *iface,
                                                                ISpeechRecognitionResult **value )
{
    struct recognition_result_event_args *impl = impl_from_ISpeechContinuousRecognitionResultGeneratedEventArgs(iface);
    FIXME("iface %p value %p stub!\n", iface, value);
    ISpeechRecognitionResult_AddRef((*value = impl->result));
    return S_OK;
}

static const struct ISpeechContinuousRecognitionResultGeneratedEventArgsVtbl recognition_result_event_args_vtbl =
{
    /* IUnknown methods */
    recognition_result_event_args_QueryInterface,
    recognition_result_event_args_AddRef,
    recognition_result_event_args_Release,
    /* IInspectable methods */
    recognition_result_event_args_GetIids,
    recognition_result_event_args_GetRuntimeClassName,
    recognition_result_event_args_GetTrustLevel,
    /* ISpeechContinuousRecognitionResultGeneratedEventArgs methods */
    recognition_result_event_args_get_Result
};

static HRESULT WINAPI recognition_result_event_args_create( ISpeechRecognitionResult *result,
                                                            ISpeechContinuousRecognitionResultGeneratedEventArgs **out )
{
    struct recognition_result_event_args *impl;

    TRACE("out %p.\n", out);

    if (!(impl = calloc(1, sizeof(*impl))))
    {
        *out = NULL;
        return E_OUTOFMEMORY;
    }

    impl->ISpeechContinuousRecognitionResultGeneratedEventArgs_iface.lpVtbl = &recognition_result_event_args_vtbl;
    impl->ref = 1;
    if (result) ISpeechRecognitionResult_AddRef((impl->result = result));

    *out = &impl->ISpeechContinuousRecognitionResultGeneratedEventArgs_iface;

    TRACE("created %p.\n", *out);
    return S_OK;
}

/*
 *
 * ISpeechRecognitionCompilationResult
 *
 */

struct compilation_result
{
    ISpeechRecognitionCompilationResult ISpeechRecognitionCompilationResult_iface;
    LONG ref;

    SpeechRecognitionResultStatus status;
};

static inline struct compilation_result *impl_from_ISpeechRecognitionCompilationResult( ISpeechRecognitionCompilationResult *iface )
{
    return CONTAINING_RECORD(iface, struct compilation_result, ISpeechRecognitionCompilationResult_iface);
}

static HRESULT WINAPI compilation_result_QueryInterface( ISpeechRecognitionCompilationResult *iface, REFIID iid, void **out )
{
    struct compilation_result *impl = impl_from_ISpeechRecognitionCompilationResult(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject) ||
        IsEqualGUID(iid, &IID_ISpeechRecognitionCompilationResult))
    {
        IInspectable_AddRef((*out = &impl->ISpeechRecognitionCompilationResult_iface));
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI compilation_result_AddRef( ISpeechRecognitionCompilationResult *iface )
{
    struct compilation_result *impl = impl_from_ISpeechRecognitionCompilationResult(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI compilation_result_Release( ISpeechRecognitionCompilationResult *iface )
{
    struct compilation_result *impl = impl_from_ISpeechRecognitionCompilationResult(iface);

    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);

    if (!ref)
        free(impl);

    return ref;
}

static HRESULT WINAPI compilation_result_GetIids( ISpeechRecognitionCompilationResult *iface, ULONG *iid_count, IID **iids )
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI compilation_result_GetRuntimeClassName( ISpeechRecognitionCompilationResult *iface, HSTRING *class_name )
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI compilation_result_GetTrustLevel( ISpeechRecognitionCompilationResult *iface, TrustLevel *trust_level )
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI compilation_result_get_Status( ISpeechRecognitionCompilationResult *iface, SpeechRecognitionResultStatus *value )
{
    struct compilation_result *impl = impl_from_ISpeechRecognitionCompilationResult(iface);
    TRACE("iface %p, value %p.\n", iface, value);
    *value = impl->status;
    return S_OK;
}

static const struct ISpeechRecognitionCompilationResultVtbl compilation_result_vtbl =
{
    /* IUnknown methods */
    compilation_result_QueryInterface,
    compilation_result_AddRef,
    compilation_result_Release,
    /* IInspectable methods */
    compilation_result_GetIids,
    compilation_result_GetRuntimeClassName,
    compilation_result_GetTrustLevel,
    /* ISpeechRecognitionCompilationResult methods */
    compilation_result_get_Status
};


static HRESULT compilation_result_create( SpeechRecognitionResultStatus status, ISpeechRecognitionCompilationResult **out )
{
    struct compilation_result *impl;

    TRACE("out %p.\n", out);

    if (!(impl = calloc(1, sizeof(*impl))))
    {
        *out = NULL;
        return E_OUTOFMEMORY;
    }

    impl->ISpeechRecognitionCompilationResult_iface.lpVtbl = &compilation_result_vtbl;
    impl->ref = 1;
    impl->status = status;

    *out = &impl->ISpeechRecognitionCompilationResult_iface;
    TRACE("created %p\n", *out);
    return S_OK;
}

/*
 *
 * SpeechContinuousRecognitionSession
 *
 */

struct session
{
    ISpeechContinuousRecognitionSession ISpeechContinuousRecognitionSession_iface;
    LONG ref;

    IVector_ISpeechRecognitionConstraint *constraints;

    SpeechRecognizerState recognizer_state;

    struct list completed_handlers;
    struct list result_handlers;

    IAudioClient *audio_client;
    IAudioCaptureClient *capture_client;
    WAVEFORMATEX capture_wfx;

    speech_recognizer_handle unix_handle;

    HANDLE worker_thread, worker_control_event, audio_buf_event;
    BOOLEAN worker_running, worker_paused;
    CRITICAL_SECTION cs;
};

/*
 *
 * ISpeechContinuousRecognitionSession
 *
 */

static inline struct session *impl_from_ISpeechContinuousRecognitionSession( ISpeechContinuousRecognitionSession *iface )
{
    return CONTAINING_RECORD(iface, struct session, ISpeechContinuousRecognitionSession_iface);
}

static HRESULT session_find_constraint_by_string(struct session *session, WCHAR *str, HSTRING *hstr_out, ISpeechRecognitionConstraint **out)
{
    ISpeechRecognitionListConstraint *list_constraint;
    IIterable_IInspectable *constraints_iterable;
    IIterator_IInspectable *constraints_iterator;
    ISpeechRecognitionConstraint *constraint;
    IIterable_HSTRING *commands_iterable;
    IIterator_HSTRING *commands_iterator;
    boolean has_constraint, has_command;
    IVector_HSTRING *commands;
    const WCHAR *command_str;
    HSTRING command;
    HRESULT hr;

    TRACE("session %p, str %s, out %p.\n", session, debugstr_w(str), out);

    if (FAILED(hr = IVector_ISpeechRecognitionConstraint_QueryInterface(session->constraints, &IID_IIterable_ISpeechRecognitionConstraint, (void **)&constraints_iterable)))
        return hr;

    if (FAILED(hr = IIterable_IInspectable_First(constraints_iterable, &constraints_iterator)))
    {
        IIterable_IInspectable_Release(constraints_iterable);
        return hr;
    }

    *out = NULL;

    for (hr = IIterator_IInspectable_get_HasCurrent(constraints_iterator, &has_constraint); SUCCEEDED(hr) && has_constraint && !(*out); hr = IIterator_IInspectable_MoveNext(constraints_iterator, &has_constraint))
    {
        list_constraint = NULL;
        commands_iterable = NULL;
        commands_iterator = NULL;
        commands = NULL;

        if (FAILED(IIterator_IInspectable_get_Current(constraints_iterator, (IInspectable **)&constraint)))
            goto skip;

        if (FAILED(ISpeechRecognitionConstraint_QueryInterface(constraint, &IID_ISpeechRecognitionListConstraint, (void**)&list_constraint)))
            goto skip;

        if (FAILED(ISpeechRecognitionListConstraint_get_Commands(list_constraint, &commands)))
            goto skip;

        if (FAILED(IVector_HSTRING_QueryInterface(commands, &IID_IIterable_HSTRING, (void **)&commands_iterable)))
            goto skip;

        if (FAILED(IIterable_HSTRING_First(commands_iterable, &commands_iterator)))
            goto skip;

        for (hr = IIterator_HSTRING_get_HasCurrent(commands_iterator, &has_command); SUCCEEDED(hr) && has_command && !(*out); hr = IIterator_HSTRING_MoveNext(commands_iterator, &has_command))
        {
            if (FAILED(IIterator_HSTRING_get_Current(commands_iterator, &command)))
                continue;

            command_str = WindowsGetStringRawBuffer(command, NULL);

            TRACE("Comparing str %s to command_str %s.\n", debugstr_w(str), debugstr_w(command_str));

            if (!wcsicmp(str, command_str))
            {
                TRACE("constraint %p has str %s.\n", constraint, debugstr_w(str));
                ISpeechRecognitionConstraint_AddRef((*out = constraint));
                WindowsDuplicateString(command, hstr_out);
            }

            WindowsDeleteString(command);
        }

skip:
        if (commands_iterator) IIterator_HSTRING_Release(commands_iterator);
        if (commands_iterable) IIterable_HSTRING_Release(commands_iterable);
        if (commands) IVector_HSTRING_Release(commands);

        if (list_constraint) ISpeechRecognitionListConstraint_Release(list_constraint);
        if (constraint) ISpeechRecognitionConstraint_Release(constraint);
    }

    IIterator_IInspectable_Release(constraints_iterator);
    IIterable_IInspectable_Release(constraints_iterable);

    hr = (*out) ? S_OK : COR_E_KEYNOTFOUND;
    return hr;
}

static DWORD CALLBACK session_worker_thread_cb( void *args )
{
    ISpeechContinuousRecognitionSession *iface = args;
    struct session *impl = impl_from_ISpeechContinuousRecognitionSession(iface);
    struct speech_get_recognition_result_params recognition_result_params;
    struct speech_recognize_audio_params recognize_audio_params;
    ISpeechContinuousRecognitionResultGeneratedEventArgs *event_args;
    ISpeechRecognitionConstraint *constraint;
    ISpeechRecognitionResult *result;
    BOOLEAN running = TRUE, paused = FALSE;
    UINT32 frame_count, tmp_buf_size;
    BYTE *audio_buf, *tmp_buf;
    WCHAR *recognized_text;
    DWORD flags, status;
    NTSTATUS nt_status;
    HANDLE events[2];
    HSTRING hstring;
    HRESULT hr;

    SetThreadDescription(GetCurrentThread(), L"wine_speech_recognition_session_worker");

    if (FAILED(hr = IAudioClient_Start(impl->audio_client)))
        goto error;

    if (FAILED(hr = IAudioClient_GetBufferSize(impl->audio_client, &frame_count)))
        goto error;

    tmp_buf_size = sizeof(*tmp_buf) * frame_count * impl->capture_wfx.nBlockAlign;
    if (!(tmp_buf = malloc(tmp_buf_size)))
    {
        ERR("Memory allocation failed.\n");
        return 1;
    }

    while (running)
    {
        BOOLEAN old_paused = paused;
        UINT32 count = 0;

        events[count++] = impl->worker_control_event;
        if (!paused) events[count++] = impl->audio_buf_event;

        status = WaitForMultipleObjects(count, events, FALSE, INFINITE);
        if (status == 0) /* worker_control_event signaled */
        {
            EnterCriticalSection(&impl->cs);
            paused = impl->worker_paused;
            running = impl->worker_running;
            LeaveCriticalSection(&impl->cs);

            if (old_paused < paused)
            {
                if (FAILED(hr = IAudioClient_Stop(impl->audio_client))) goto error;
                if (FAILED(hr = IAudioClient_Reset(impl->audio_client))) goto error;
                TRACE("session worker paused.\n");
            }
            else if (old_paused > paused)
            {
                if (FAILED(hr = IAudioClient_Start(impl->audio_client))) goto error;
                TRACE("session worker resumed.\n");
            }
        }
        else if (status == 1) /* audio_buf_event signaled */
        {
            SIZE_T packet_size = 0, tmp_buf_offset = 0;
            UINT32 frames_available = 0;
            INT recognized_text_len = 0;

            while (tmp_buf_offset < tmp_buf_size
                   && IAudioCaptureClient_GetBuffer(impl->capture_client, &audio_buf, &frames_available, &flags, NULL, NULL) == S_OK)
            {
                packet_size = frames_available * impl->capture_wfx.nBlockAlign;
                if (tmp_buf_offset + packet_size > tmp_buf_size)
                {
                    /* Defer processing until the next iteration of the worker loop. */
                    IAudioCaptureClient_ReleaseBuffer(impl->capture_client, 0);
                    SetEvent(impl->audio_buf_event);
                    break;
                }

                memcpy(tmp_buf + tmp_buf_offset, audio_buf, packet_size);
                tmp_buf_offset += packet_size;

                IAudioCaptureClient_ReleaseBuffer(impl->capture_client, frames_available);
            }

            recognize_audio_params.handle = impl->unix_handle;
            recognize_audio_params.samples = tmp_buf;
            recognize_audio_params.samples_size = tmp_buf_offset;
            recognize_audio_params.status = RECOGNITION_STATUS_EXCEPTION;

            if (NT_ERROR(nt_status = WINE_UNIX_CALL(unix_speech_recognize_audio, &recognize_audio_params)))
                WARN("unix_speech_recognize_audio failed with status %#lx.\n", nt_status);

            if (recognize_audio_params.status != RECOGNITION_STATUS_RESULT_AVAILABLE)
                continue;

            recognition_result_params.handle = impl->unix_handle;
            recognition_result_params.result_buf = NULL;
            recognition_result_params.result_buf_size = 512;

            do
            {
                recognition_result_params.result_buf = realloc(recognition_result_params.result_buf, recognition_result_params.result_buf_size);
            }
            while (WINE_UNIX_CALL(unix_speech_get_recognition_result, &recognition_result_params) == STATUS_BUFFER_TOO_SMALL &&
                   recognition_result_params.result_buf);

            if (!recognition_result_params.result_buf)
            {
                WARN("memory allocation failed.\n");
                break;
            }

            /* Silence was recognized. */
            if (!strcmp(recognition_result_params.result_buf, ""))
            {
                free(recognition_result_params.result_buf);
                continue;
            }

            recognized_text_len = MultiByteToWideChar(CP_UTF8, 0, recognition_result_params.result_buf, -1, NULL, 0);

            if (!(recognized_text = malloc(recognized_text_len * sizeof(WCHAR))))
            {
                free(recognition_result_params.result_buf);
                WARN("memory allocation failed.\n");
                break;
            }

            MultiByteToWideChar(CP_UTF8, 0, recognition_result_params.result_buf, -1, recognized_text, recognized_text_len);

            if (SUCCEEDED(hr = session_find_constraint_by_string(impl, recognized_text, &hstring, &constraint)))
            {
                recognition_result_create(constraint, hstring, &result);
                recognition_result_event_args_create(result, &event_args);

                typed_event_handlers_notify(&impl->result_handlers,
                                            (IInspectable *)&impl->ISpeechContinuousRecognitionSession_iface,
                                            (IInspectable *)event_args);

                ISpeechContinuousRecognitionResultGeneratedEventArgs_Release(event_args);
                ISpeechRecognitionResult_Release(result);
                WindowsDeleteString(hstring);
                ISpeechRecognitionConstraint_Release(constraint);
            }

            free(recognized_text);
            free(recognition_result_params.result_buf);
        }
        else
        {
            ERR("Unexpected state entered. Aborting worker.\n");
            break;
        }
    }

    if (FAILED(hr = IAudioClient_Stop(impl->audio_client)))
        ERR("IAudioClient_Stop failed with %#lx.\n", hr);

    if (FAILED(hr = IAudioClient_Reset(impl->audio_client)))
        ERR("IAudioClient_Reset failed with %#lx.\n", hr);

    free(tmp_buf);

    return 0;

error:
    ERR("The recognition session worker encountered a serious error and needs to stop. hr: %lx.\n", hr);
    return 1;
}

static HRESULT WINAPI session_QueryInterface( ISpeechContinuousRecognitionSession *iface, REFIID iid, void **out )
{
    struct session *impl = impl_from_ISpeechContinuousRecognitionSession(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_ISpeechContinuousRecognitionSession))
    {
        IInspectable_AddRef((*out = &impl->ISpeechContinuousRecognitionSession_iface));
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI session_AddRef( ISpeechContinuousRecognitionSession *iface )
{
    struct session *impl = impl_from_ISpeechContinuousRecognitionSession(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI session_Release( ISpeechContinuousRecognitionSession *iface )
{
    struct session *impl = impl_from_ISpeechContinuousRecognitionSession(iface);
    struct speech_release_recognizer_params release_params;
    ULONG ref = InterlockedDecrement(&impl->ref);

    TRACE("iface %p, ref %lu.\n", iface, ref);

    if (!ref)
    {
        HANDLE thread;

        EnterCriticalSection(&impl->cs);
        thread = impl->worker_thread;
        impl->worker_running = FALSE;
        impl->worker_thread = INVALID_HANDLE_VALUE;
        LeaveCriticalSection(&impl->cs);

        SetEvent(impl->worker_control_event);
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);

        typed_event_handlers_clear(&impl->completed_handlers);
        typed_event_handlers_clear(&impl->result_handlers);

        IAudioCaptureClient_Release(impl->capture_client);
        IAudioClient_Release(impl->audio_client);

        impl->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&impl->cs);

        release_params.handle = impl->unix_handle;
        WINE_UNIX_CALL(unix_speech_release_recognizer, &release_params);

        IVector_ISpeechRecognitionConstraint_Release(impl->constraints);
        free(impl);
    }

    return ref;
}

static HRESULT WINAPI session_GetIids( ISpeechContinuousRecognitionSession *iface, ULONG *iid_count, IID **iids )
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI session_GetRuntimeClassName( ISpeechContinuousRecognitionSession *iface, HSTRING *class_name )
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI session_GetTrustLevel( ISpeechContinuousRecognitionSession *iface, TrustLevel *trust_level )
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI session_get_AutoStopSilenceTimeout( ISpeechContinuousRecognitionSession *iface, TimeSpan *value )
{
    FIXME("iface %p, value %p stub!\n", iface, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI session_set_AutoStopSilenceTimeout( ISpeechContinuousRecognitionSession *iface, TimeSpan value )
{
    FIXME("iface %p, value %#I64x stub!\n", iface, value.Duration);
    return E_NOTIMPL;
}

static HRESULT session_start_async( IInspectable *invoker )
{
    return S_OK;
}

static HRESULT WINAPI session_StartAsync( ISpeechContinuousRecognitionSession *iface, IAsyncAction **action )
{
    struct session *impl = impl_from_ISpeechContinuousRecognitionSession(iface);
    HRESULT hr;

    TRACE("iface %p, action %p.\n", iface, action);

    if (FAILED(hr = async_action_create(NULL, session_start_async, action)))
        return hr;

    EnterCriticalSection(&impl->cs);
    if (impl->worker_running || impl->worker_thread)
    {
        hr = COR_E_INVALIDOPERATION;
    }
    else if (!(impl->worker_thread = CreateThread(NULL, 0, session_worker_thread_cb, impl, 0, NULL)))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        impl->worker_running = FALSE;
    }
    else
    {
        impl->worker_running = TRUE;
        impl->recognizer_state = SpeechRecognizerState_Capturing;
    }
    LeaveCriticalSection(&impl->cs);

    if (FAILED(hr))
    {
        IAsyncAction_Release(*action);
        *action = NULL;
    }

    return hr;
}

static HRESULT WINAPI session_StartWithModeAsync( ISpeechContinuousRecognitionSession *iface,
                                                  SpeechContinuousRecognitionMode mode,
                                                  IAsyncAction **action )
{
    FIXME("iface %p, mode %u, action %p stub!\n", iface, mode, action);
    return E_NOTIMPL;
}

static HRESULT session_stop_async( IInspectable *invoker )
{
    return S_OK;
}

static HRESULT WINAPI session_StopAsync( ISpeechContinuousRecognitionSession *iface, IAsyncAction **action )
{
    struct session *impl = impl_from_ISpeechContinuousRecognitionSession(iface);
    HANDLE thread;
    HRESULT hr;

    TRACE("iface %p, action %p.\n", iface, action);

    if (FAILED(hr = async_action_create(NULL, session_stop_async, action)))
        return hr;

    EnterCriticalSection(&impl->cs);
    if (impl->worker_running && impl->worker_thread)
    {
        thread = impl->worker_thread;
        impl->worker_thread = INVALID_HANDLE_VALUE;
        impl->worker_running = FALSE;
        impl->worker_paused = FALSE;
        impl->recognizer_state = SpeechRecognizerState_Idle;
    }
    else
    {
        hr = COR_E_INVALIDOPERATION;
    }
    LeaveCriticalSection(&impl->cs);

    if (SUCCEEDED(hr))
    {
        SetEvent(impl->worker_control_event);
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);

        EnterCriticalSection(&impl->cs);
        impl->worker_thread = NULL;
        LeaveCriticalSection(&impl->cs);
    }
    else
    {
        IAsyncAction_Release(*action);
        *action = NULL;
    }

    return hr;
}

static HRESULT WINAPI session_CancelAsync( ISpeechContinuousRecognitionSession *iface, IAsyncAction **action )
{
    FIXME("iface %p, action %p stub!\n", iface, action);
    return E_NOTIMPL;
}

static HRESULT session_pause_async( IInspectable *invoker )
{
    return S_OK;
}

static HRESULT WINAPI session_PauseAsync( ISpeechContinuousRecognitionSession *iface, IAsyncAction **action )
{
    struct session *impl = impl_from_ISpeechContinuousRecognitionSession(iface);
    HRESULT hr = S_OK;

    TRACE("iface %p, action %p.\n", iface, action);

    *action = NULL;

    if (FAILED(hr = async_action_create(NULL, session_pause_async, action)))
        return hr;

    EnterCriticalSection(&impl->cs);
    if (impl->worker_running)
    {
        impl->worker_paused = TRUE;
        impl->recognizer_state = SpeechRecognizerState_Paused;
    }
    LeaveCriticalSection(&impl->cs);

    SetEvent(impl->worker_control_event);

    return hr;
}

static HRESULT WINAPI session_Resume( ISpeechContinuousRecognitionSession *iface )
{
    struct session *impl = impl_from_ISpeechContinuousRecognitionSession(iface);

    TRACE("iface %p.\n", iface);

    EnterCriticalSection(&impl->cs);
    if (impl->worker_running)
    {
        impl->worker_paused = FALSE;
        impl->recognizer_state = SpeechRecognizerState_Capturing;
    }
    LeaveCriticalSection(&impl->cs);

    SetEvent(impl->worker_control_event);

    return S_OK;
}

static HRESULT WINAPI session_add_Completed( ISpeechContinuousRecognitionSession *iface,
                                             ITypedEventHandler_SpeechContinuousRecognitionSession_SpeechContinuousRecognitionCompletedEventArgs *handler,
                                             EventRegistrationToken *token )
{
    struct session *impl = impl_from_ISpeechContinuousRecognitionSession(iface);
    TRACE("iface %p, handler %p, token %p.\n", iface, handler, token);
    if (!handler) return E_INVALIDARG;
    return typed_event_handlers_append(&impl->completed_handlers, (ITypedEventHandler_IInspectable_IInspectable *)handler, token);
}

static HRESULT WINAPI session_remove_Completed( ISpeechContinuousRecognitionSession *iface, EventRegistrationToken token )
{
    struct session *impl = impl_from_ISpeechContinuousRecognitionSession(iface);
    TRACE("iface %p, token.value %#I64x.\n", iface, token.value);
    return typed_event_handlers_remove(&impl->completed_handlers, &token);
}

static HRESULT WINAPI session_add_ResultGenerated( ISpeechContinuousRecognitionSession *iface,
                                                   ITypedEventHandler_SpeechContinuousRecognitionSession_SpeechContinuousRecognitionResultGeneratedEventArgs *handler,
                                                   EventRegistrationToken *token)
{
    struct session *impl = impl_from_ISpeechContinuousRecognitionSession(iface);
    TRACE("iface %p, handler %p, token %p.\n", iface, handler, token);
    if (!handler) return E_INVALIDARG;
    return typed_event_handlers_append(&impl->result_handlers, (ITypedEventHandler_IInspectable_IInspectable *)handler, token);
}

static HRESULT WINAPI session_remove_ResultGenerated( ISpeechContinuousRecognitionSession *iface, EventRegistrationToken token )
{
    struct session *impl = impl_from_ISpeechContinuousRecognitionSession(iface);
    TRACE("iface %p, token.value %#I64x.\n", iface, token.value);
    return typed_event_handlers_remove(&impl->result_handlers, &token);
}

static const struct ISpeechContinuousRecognitionSessionVtbl session_vtbl =
{
    /* IUnknown methods */
    session_QueryInterface,
    session_AddRef,
    session_Release,
    /* IInspectable methods */
    session_GetIids,
    session_GetRuntimeClassName,
    session_GetTrustLevel,
    /* ISpeechContinuousRecognitionSession methods */
    session_get_AutoStopSilenceTimeout,
    session_set_AutoStopSilenceTimeout,
    session_StartAsync,
    session_StartWithModeAsync,
    session_StopAsync,
    session_CancelAsync,
    session_PauseAsync,
    session_Resume,
    session_add_Completed,
    session_remove_Completed,
    session_add_ResultGenerated,
    session_remove_ResultGenerated
};

/*
 *
 * SpeechRecognizer
 *
 */

struct recognizer
{
    ISpeechRecognizer ISpeechRecognizer_iface;
    IClosable IClosable_iface;
    ISpeechRecognizer2 ISpeechRecognizer2_iface;
    LONG ref;

    ISpeechContinuousRecognitionSession *session;
};

/*
 *
 * ISpeechRecognizer
 *
 */

static inline struct recognizer *impl_from_ISpeechRecognizer( ISpeechRecognizer *iface )
{
    return CONTAINING_RECORD(iface, struct recognizer, ISpeechRecognizer_iface);
}

static HRESULT WINAPI recognizer_QueryInterface( ISpeechRecognizer *iface, REFIID iid, void **out )
{
    struct recognizer *impl = impl_from_ISpeechRecognizer(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject) ||
        IsEqualGUID(iid, &IID_ISpeechRecognizer))
    {
        IInspectable_AddRef((*out = &impl->ISpeechRecognizer_iface));
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_IClosable))
    {
        IInspectable_AddRef((*out = &impl->IClosable_iface));
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_ISpeechRecognizer2))
    {
        IInspectable_AddRef((*out = &impl->ISpeechRecognizer2_iface));
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI recognizer_AddRef( ISpeechRecognizer *iface )
{
    struct recognizer *impl = impl_from_ISpeechRecognizer(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI recognizer_Release( ISpeechRecognizer *iface )
{
    struct recognizer *impl = impl_from_ISpeechRecognizer(iface);

    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);

    if (!ref)
    {
        ISpeechContinuousRecognitionSession_Release(impl->session);
        free(impl);
    }

    return ref;
}

static HRESULT WINAPI recognizer_GetIids( ISpeechRecognizer *iface, ULONG *iid_count, IID **iids )
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer_GetRuntimeClassName( ISpeechRecognizer *iface, HSTRING *class_name )
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer_GetTrustLevel( ISpeechRecognizer *iface, TrustLevel *trust_level )
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer_get_Constraints( ISpeechRecognizer *iface, IVector_ISpeechRecognitionConstraint **vector )
{
    struct recognizer *impl = impl_from_ISpeechRecognizer(iface);
    struct session *session = impl_from_ISpeechContinuousRecognitionSession(impl->session);

    TRACE("iface %p, operation %p.\n", iface, vector);

    IVector_ISpeechRecognitionConstraint_AddRef((*vector = session->constraints));
    return S_OK;
}

static HRESULT WINAPI recognizer_get_CurrentLanguage( ISpeechRecognizer *iface, ILanguage **language )
{
    FIXME("iface %p, operation %p stub!\n", iface, language);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer_get_Timeouts( ISpeechRecognizer *iface, ISpeechRecognizerTimeouts **timeouts )
{
    FIXME("iface %p, operation %p stub!\n", iface, timeouts);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer_get_UIOptions( ISpeechRecognizer *iface, ISpeechRecognizerUIOptions **options )
{
    FIXME("iface %p, operation %p stub!\n", iface, options);
    return E_NOTIMPL;
}

static HRESULT recognizer_create_unix_instance( struct session *session, const char **grammar, UINT32 grammar_size )
{
    struct speech_create_recognizer_params create_params = { 0 };
    WCHAR locale[LOCALE_NAME_MAX_LENGTH];
    NTSTATUS status;
    INT len;

    if (!(len = GetUserDefaultLocaleName(locale, LOCALE_NAME_MAX_LENGTH)))
        return E_FAIL;

    if (CharLowerBuffW(locale, len) != len)
        return E_FAIL;

    if (!WideCharToMultiByte(CP_ACP, 0, locale, len, create_params.locale, ARRAY_SIZE(create_params.locale), NULL, NULL))
        return HRESULT_FROM_WIN32(GetLastError());

    create_params.sample_rate = (FLOAT)session->capture_wfx.nSamplesPerSec;
    create_params.grammar = grammar;
    create_params.grammar_size = grammar_size;

    if ((status = WINE_UNIX_CALL(unix_speech_create_recognizer, &create_params)))
    {
        ERR("Unable to create Vosk instance for locale %s, status %#lx. Speech recognition won't work.\n", debugstr_a(create_params.locale), status);
        return SPERR_WINRT_INTERNAL_ERROR;
    }

    session->unix_handle = create_params.handle;

    return S_OK;
}

static HRESULT recognizer_compile_constraints_async( IInspectable *invoker, IInspectable **result )
{
    struct recognizer *impl = impl_from_ISpeechRecognizer((ISpeechRecognizer *)invoker);
    struct session *session = impl_from_ISpeechContinuousRecognitionSession(impl->session);
    struct speech_release_recognizer_params release_params;
    ISpeechRecognitionListConstraint *list_constraint;
    IIterable_IInspectable *constraints_iterable;
    IIterator_IInspectable *constraints_iterator;
    ISpeechRecognitionConstraint *constraint;
    IIterable_HSTRING *commands_iterable;
    IIterator_HSTRING *commands_iterator;
    boolean has_constraint, has_command;
    IVector_HSTRING *commands;
    const WCHAR *command_str;
    UINT32 grammar_size = 0, i = 0;
    char **grammar = NULL;
    HSTRING command;
    UINT32 size = 0;
    HRESULT hr;

    if (FAILED(hr = IVector_ISpeechRecognitionConstraint_QueryInterface(session->constraints, &IID_IIterable_ISpeechRecognitionConstraint, (void **)&constraints_iterable)))
        return hr;

    if (FAILED(hr = IIterable_IInspectable_First(constraints_iterable, &constraints_iterator)))
    {
        IIterable_IInspectable_Release(constraints_iterable);
        return hr;
    }

    for (hr = IIterator_IInspectable_get_HasCurrent(constraints_iterator, &has_constraint); SUCCEEDED(hr) && has_constraint; hr = IIterator_IInspectable_MoveNext(constraints_iterator, &has_constraint))
    {
        list_constraint = NULL;
        commands_iterable = NULL;
        commands_iterator = NULL;
        commands = NULL;

        if (FAILED(IIterator_IInspectable_get_Current(constraints_iterator, (IInspectable **)&constraint)))
            goto skip;

        if (FAILED(ISpeechRecognitionConstraint_QueryInterface(constraint, &IID_ISpeechRecognitionListConstraint, (void**)&list_constraint)))
            goto skip;

        if (FAILED(ISpeechRecognitionListConstraint_get_Commands(list_constraint, &commands)))
            goto skip;

        if (FAILED(IVector_HSTRING_QueryInterface(commands, &IID_IIterable_HSTRING, (void **)&commands_iterable)))
            goto skip;

        if (FAILED(IIterable_HSTRING_First(commands_iterable, &commands_iterator)))
            goto skip;

        if (FAILED(IVector_HSTRING_get_Size(commands, &size)))
            goto skip;

        grammar_size += size;
        grammar = realloc(grammar, grammar_size * sizeof(char *));

        for (hr = IIterator_HSTRING_get_HasCurrent(commands_iterator, &has_command); SUCCEEDED(hr) && has_command; hr = IIterator_HSTRING_MoveNext(commands_iterator, &has_command))
        {
            if (FAILED(IIterator_HSTRING_get_Current(commands_iterator, &command)))
                continue;

            command_str = WindowsGetStringRawBuffer(command, NULL);

            if (command_str)
            {
                WCHAR *wstr = wcsdup(command_str);
                size_t len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, grammar[i], 0, NULL, NULL);
                grammar[i] = malloc(len * sizeof(char));

                CharLowerW(wstr);
                WideCharToMultiByte(CP_UTF8, 0, wstr, -1, grammar[i], len, NULL, NULL);
                free(wstr);
                i++;
            }

            WindowsDeleteString(command);
        }

skip:
        if (commands_iterator) IIterator_HSTRING_Release(commands_iterator);
        if (commands_iterable) IIterable_HSTRING_Release(commands_iterable);
        if (commands) IVector_HSTRING_Release(commands);

        if (list_constraint) ISpeechRecognitionListConstraint_Release(list_constraint);
        if (constraint) ISpeechRecognitionConstraint_Release(constraint);
    }

    IIterator_IInspectable_Release(constraints_iterator);
    IIterable_IInspectable_Release(constraints_iterable);

    if (session->unix_handle)
    {
        release_params.handle = session->unix_handle;
        WINE_UNIX_CALL(unix_speech_release_recognizer, &release_params);
        session->unix_handle = 0;
    }

    hr = recognizer_create_unix_instance(session, (const char **)grammar, grammar_size);

    for(i = 0; i < grammar_size; ++i)
        free(grammar[i]);
    free(grammar);

    if (FAILED(hr))
    {
        WARN("Failed to created recognizer instance with grammar.\n");
        return compilation_result_create(SpeechRecognitionResultStatus_GrammarCompilationFailure, (ISpeechRecognitionCompilationResult **) result);
    }
    else return compilation_result_create(SpeechRecognitionResultStatus_Success, (ISpeechRecognitionCompilationResult **) result);
}

static HRESULT WINAPI recognizer_CompileConstraintsAsync( ISpeechRecognizer *iface,
                                                          IAsyncOperation_SpeechRecognitionCompilationResult **operation )
{
    IAsyncOperation_IInspectable **value = (IAsyncOperation_IInspectable **)operation;
    TRACE("iface %p, operation %p semi-stub!\n", iface, operation);
    return async_operation_inspectable_create(&IID_IAsyncOperation_SpeechRecognitionCompilationResult, (IInspectable *)iface, recognizer_compile_constraints_async, value);
}

static HRESULT WINAPI recognizer_RecognizeAsync( ISpeechRecognizer *iface,
                                                 IAsyncOperation_SpeechRecognitionResult **operation )
{
    FIXME("iface %p, operation %p stub!\n", iface, operation);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer_RecognizeWithUIAsync( ISpeechRecognizer *iface,
                                                       IAsyncOperation_SpeechRecognitionResult **operation )
{
    FIXME("iface %p, operation %p stub!\n", iface, operation);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer_add_RecognitionQualityDegrading( ISpeechRecognizer *iface,
                                                                  ITypedEventHandler_SpeechRecognizer_SpeechRecognitionQualityDegradingEventArgs *handler,
                                                                  EventRegistrationToken *token )
{
    FIXME("iface %p, operation %p, token %p, stub!\n", iface, handler, token);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer_remove_RecognitionQualityDegrading( ISpeechRecognizer *iface, EventRegistrationToken token )
{
    FIXME("iface %p, token.value %#I64x, stub!\n", iface, token.value);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer_add_StateChanged( ISpeechRecognizer *iface,
                                                   ITypedEventHandler_SpeechRecognizer_SpeechRecognizerStateChangedEventArgs *handler,
                                                   EventRegistrationToken *token )
{
    FIXME("iface %p, operation %p, token %p, stub!\n", iface, handler, token);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer_remove_StateChanged( ISpeechRecognizer *iface, EventRegistrationToken token )
{
    FIXME("iface %p, token.value %#I64x, stub!\n", iface, token.value);
    return E_NOTIMPL;
}

static const struct ISpeechRecognizerVtbl speech_recognizer_vtbl =
{
    /* IUnknown methods */
    recognizer_QueryInterface,
    recognizer_AddRef,
    recognizer_Release,
    /* IInspectable methods */
    recognizer_GetIids,
    recognizer_GetRuntimeClassName,
    recognizer_GetTrustLevel,
    /* ISpeechRecognizer methods */
    recognizer_get_CurrentLanguage,
    recognizer_get_Constraints,
    recognizer_get_Timeouts,
    recognizer_get_UIOptions,
    recognizer_CompileConstraintsAsync,
    recognizer_RecognizeAsync,
    recognizer_RecognizeWithUIAsync,
    recognizer_add_RecognitionQualityDegrading,
    recognizer_remove_RecognitionQualityDegrading,
    recognizer_add_StateChanged,
    recognizer_remove_StateChanged,
};

/*
 *
 * IClosable
 *
 */

DEFINE_IINSPECTABLE(closable, IClosable, struct recognizer, ISpeechRecognizer_iface)

static HRESULT WINAPI closable_Close( IClosable *iface )
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

/*
 *
 * ISpeechRecognizer2
 *
 */

DEFINE_IINSPECTABLE(recognizer2, ISpeechRecognizer2, struct recognizer, ISpeechRecognizer_iface)

static HRESULT WINAPI recognizer2_get_ContinuousRecognitionSession( ISpeechRecognizer2 *iface,
                                                                    ISpeechContinuousRecognitionSession **session )
{
    struct recognizer *impl = impl_from_ISpeechRecognizer2(iface);
    TRACE("iface %p, session %p.\n", iface, session);
    ISpeechContinuousRecognitionSession_QueryInterface(impl->session, &IID_ISpeechContinuousRecognitionSession, (void **)session);
    return S_OK;
}

static HRESULT WINAPI recognizer2_get_State( ISpeechRecognizer2 *iface, SpeechRecognizerState *state )
{
    struct recognizer *impl = impl_from_ISpeechRecognizer2(iface);
    struct session *session = impl_from_ISpeechContinuousRecognitionSession(impl->session);

    FIXME("iface %p, state %p not all states are supported, yet.\n", iface, state);

    if (!state)
        return E_POINTER;

    EnterCriticalSection(&session->cs);
    *state = session->recognizer_state;
    LeaveCriticalSection(&session->cs);

    return S_OK;
}

static HRESULT WINAPI recognizer2_StopRecognitionAsync( ISpeechRecognizer2 *iface, IAsyncAction **action )
{
    FIXME("iface %p, action %p stub!\n", iface, action);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer2_add_HypothesisGenerated( ISpeechRecognizer2 *iface,
                                                           ITypedEventHandler_SpeechRecognizer_SpeechRecognitionHypothesisGeneratedEventArgs *handler,
                                                           EventRegistrationToken *token )
{
    FIXME("iface %p, operation %p, token %p, stub!\n", iface, handler, token);
    return E_NOTIMPL;
}

static HRESULT WINAPI recognizer2_remove_HypothesisGenerated( ISpeechRecognizer2 *iface, EventRegistrationToken token )
{
    FIXME("iface %p, token.value %#I64x, stub!\n", iface, token.value);
    return E_NOTIMPL;
}

static const struct ISpeechRecognizer2Vtbl speech_recognizer2_vtbl =
{
    /* IUnknown methods */
    recognizer2_QueryInterface,
    recognizer2_AddRef,
    recognizer2_Release,
    /* IInspectable methods */
    recognizer2_GetIids,
    recognizer2_GetRuntimeClassName,
    recognizer2_GetTrustLevel,
    /* ISpeechRecognizer2 methods */
    recognizer2_get_ContinuousRecognitionSession,
    recognizer2_get_State,
    recognizer2_StopRecognitionAsync,
    recognizer2_add_HypothesisGenerated,
    recognizer2_remove_HypothesisGenerated,
};

/*
 *
 * Statics for SpeechRecognizer
 *
 */

struct recognizer_statics
{
    IActivationFactory IActivationFactory_iface;
    ISpeechRecognizerFactory ISpeechRecognizerFactory_iface;
    ISpeechRecognizerStatics ISpeechRecognizerStatics_iface;
    ISpeechRecognizerStatics2 ISpeechRecognizerStatics2_iface;
    LONG ref;
};

/*
 *
 * IActivationFactory
 *
 */

static inline struct recognizer_statics *impl_from_IActivationFactory( IActivationFactory *iface )
{
    return CONTAINING_RECORD(iface, struct recognizer_statics, IActivationFactory_iface);
}

static HRESULT WINAPI activation_factory_QueryInterface( IActivationFactory *iface, REFIID iid, void **out )
{
    struct recognizer_statics *impl = impl_from_IActivationFactory(iface);

    TRACE("iface %p, iid %s, out %p stub!\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IInspectable) ||
        IsEqualGUID(iid, &IID_IAgileObject) ||
        IsEqualGUID(iid, &IID_IActivationFactory))
    {
        IInspectable_AddRef((*out = &impl->IActivationFactory_iface));
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_ISpeechRecognizerFactory))
    {
        IInspectable_AddRef((*out = &impl->ISpeechRecognizerFactory_iface));
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_ISpeechRecognizerStatics))
    {
        IInspectable_AddRef((*out = &impl->ISpeechRecognizerStatics_iface));
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_ISpeechRecognizerStatics2))
    {
        IInspectable_AddRef((*out = &impl->ISpeechRecognizerStatics2_iface));
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI activation_factory_AddRef( IActivationFactory *iface )
{
    struct recognizer_statics *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI activation_factory_Release( IActivationFactory *iface )
{
    struct recognizer_statics *impl = impl_from_IActivationFactory(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);
    TRACE("iface %p, ref %lu.\n", iface, ref);
    return ref;
}

static HRESULT WINAPI activation_factory_GetIids( IActivationFactory *iface, ULONG *iid_count, IID **iids )
{
    FIXME("iface %p, iid_count %p, iids %p stub!\n", iface, iid_count, iids);
    return E_NOTIMPL;
}

static HRESULT WINAPI activation_factory_GetRuntimeClassName( IActivationFactory *iface, HSTRING *class_name )
{
    FIXME("iface %p, class_name %p stub!\n", iface, class_name);
    return E_NOTIMPL;
}

static HRESULT WINAPI activation_factory_GetTrustLevel( IActivationFactory *iface, TrustLevel *trust_level )
{
    FIXME("iface %p, trust_level %p stub!\n", iface, trust_level);
    return E_NOTIMPL;
}

static HRESULT WINAPI activation_factory_ActivateInstance( IActivationFactory *iface, IInspectable **instance )
{
    struct recognizer_statics *impl = impl_from_IActivationFactory(iface);
    TRACE("iface %p, instance %p.\n", iface, instance);
    return ISpeechRecognizerFactory_Create(&impl->ISpeechRecognizerFactory_iface, NULL, (ISpeechRecognizer **)instance);
}

static const struct IActivationFactoryVtbl activation_factory_vtbl =
{
    /* IUnknown methods */
    activation_factory_QueryInterface,
    activation_factory_AddRef,
    activation_factory_Release,
    /* IInspectable methods */
    activation_factory_GetIids,
    activation_factory_GetRuntimeClassName,
    activation_factory_GetTrustLevel,
    /* IActivationFactory methods */
    activation_factory_ActivateInstance,
};

/*
 *
 * ISpeechRecognizerFactory
 *
 */

DEFINE_IINSPECTABLE(recognizer_factory, ISpeechRecognizerFactory, struct recognizer_statics, IActivationFactory_iface)

static HRESULT recognizer_factory_create_audio_capture(struct session *session)
{
    const REFERENCE_TIME buffer_duration = 5000000; /* 0.5 second */
    IMMDeviceEnumerator *mm_enum = NULL;
    IMMDevice *mm_device = NULL;
    WAVEFORMATEX wfx = { 0 };
    WCHAR *str = NULL;
    HRESULT hr = S_OK;

    if (!(session->audio_buf_event = CreateEventW(NULL, FALSE, FALSE, NULL)))
        return HRESULT_FROM_WIN32(GetLastError());

    if (FAILED(hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_INPROC_SERVER, &IID_IMMDeviceEnumerator, (void **)&mm_enum)))
        goto cleanup;

    if (FAILED(hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(mm_enum, eCapture, eMultimedia, &mm_device)))
        goto cleanup;

    if (FAILED(hr = IMMDevice_Activate(mm_device, &IID_IAudioClient, CLSCTX_INPROC_SERVER, NULL, (void **)&session->audio_client)))
        goto cleanup;

    hr = IMMDevice_GetId(mm_device, &str);
    TRACE("selected capture device ID: %s, hr %#lx\n", debugstr_w(str), hr);

    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nSamplesPerSec = 16000;
    wfx.nChannels = 1;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = (wfx.wBitsPerSample + 7) / 8 * wfx.nChannels;
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    TRACE("wfx tag %u, channels %u, samples %lu, bits %u, align %u.\n", wfx.wFormatTag, wfx.nChannels, wfx.nSamplesPerSec, wfx.wBitsPerSample, wfx.nBlockAlign);

    if (FAILED(hr = IAudioClient_Initialize(session->audio_client, AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_EVENTCALLBACK, buffer_duration, 0, &wfx, NULL)))
        goto cleanup;

    if (FAILED(hr = IAudioClient_SetEventHandle(session->audio_client, session->audio_buf_event)))
        goto cleanup;

    hr = IAudioClient_GetService(session->audio_client, &IID_IAudioCaptureClient, (void **)&session->capture_client);

    session->capture_wfx = wfx;

cleanup:
    if (mm_device) IMMDevice_Release(mm_device);
    if (mm_enum) IMMDeviceEnumerator_Release(mm_enum);
    CoTaskMemFree(str);
    return hr;
}

static HRESULT WINAPI recognizer_factory_Create( ISpeechRecognizerFactory *iface, ILanguage *language, ISpeechRecognizer **speechrecognizer )
{
    struct recognizer *impl;
    struct session *session;
    struct vector_iids constraints_iids =
    {
        .iterable = &IID_IIterable_ISpeechRecognitionConstraint,
        .iterator = &IID_IIterator_ISpeechRecognitionConstraint,
        .vector = &IID_IVector_ISpeechRecognitionConstraint,
        .view = &IID_IVectorView_ISpeechRecognitionConstraint,
    };
    HRESULT hr;

    TRACE("iface %p, language %p, speechrecognizer %p.\n", iface, language, speechrecognizer);

    *speechrecognizer = NULL;

    if (!(impl = calloc(1, sizeof(*impl)))) return E_OUTOFMEMORY;
    if (!(session = calloc(1, sizeof(*session))))
    {
        hr = E_OUTOFMEMORY;
        goto error;
    }

    if (language)
        FIXME("language parameter unused. Stub!\n");

    /* Init ISpeechContinuousRecognitionSession */
    session->ISpeechContinuousRecognitionSession_iface.lpVtbl = &session_vtbl;
    session->ref = 1;

    list_init(&session->completed_handlers);
    list_init(&session->result_handlers);

    if (!(session->worker_control_event = CreateEventW(NULL, FALSE, FALSE, NULL)))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto error;
    }

    if (FAILED(hr = vector_inspectable_create(&constraints_iids, (IVector_IInspectable**)&session->constraints)))
        goto error;

    if (FAILED(hr = recognizer_factory_create_audio_capture(session)))
        goto error;

    InitializeCriticalSection(&session->cs);
    session->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": recognition_session.cs");

    /* Init ISpeechRecognizer */
    impl->ISpeechRecognizer_iface.lpVtbl = &speech_recognizer_vtbl;
    impl->IClosable_iface.lpVtbl = &closable_vtbl;
    impl->ISpeechRecognizer2_iface.lpVtbl = &speech_recognizer2_vtbl;
    impl->session = &session->ISpeechContinuousRecognitionSession_iface;
    impl->ref = 1;

    *speechrecognizer = &impl->ISpeechRecognizer_iface;
    TRACE("created SpeechRecognizer %p.\n", *speechrecognizer);
    return S_OK;

error:
    if (session->capture_client) IAudioCaptureClient_Release(session->capture_client);
    if (session->audio_client) IAudioClient_Release(session->audio_client);
    if (session->audio_buf_event) CloseHandle(session->audio_buf_event);
    if (session->constraints) IVector_ISpeechRecognitionConstraint_Release(session->constraints);
    if (session->worker_control_event) CloseHandle(session->worker_control_event);
    free(session);
    free(impl);

    return hr;
}

static const struct ISpeechRecognizerFactoryVtbl speech_recognizer_factory_vtbl =
{
    /* IUnknown methods */
    recognizer_factory_QueryInterface,
    recognizer_factory_AddRef,
    recognizer_factory_Release,
    /* IInspectable methods */
    recognizer_factory_GetIids,
    recognizer_factory_GetRuntimeClassName,
    recognizer_factory_GetTrustLevel,
    /* ISpeechRecognizerFactory methods */
    recognizer_factory_Create
};

/*
 *
 * ISpeechRecognizerStatics
 *
 */

DEFINE_IINSPECTABLE(statics, ISpeechRecognizerStatics, struct recognizer_statics, IActivationFactory_iface)

static HRESULT WINAPI statics_get_SystemSpeechLanguage( ISpeechRecognizerStatics *iface, ILanguage **language )
{
    FIXME("iface %p, language %p stub!\n", iface, language);
    return E_NOTIMPL;
}

static HRESULT WINAPI statics_get_SupportedTopicLanguages( ISpeechRecognizerStatics *iface, IVectorView_Language **languages )
{
    FIXME("iface %p, languages %p stub!\n", iface, languages);
    return E_NOTIMPL;
}

static HRESULT WINAPI statics_get_SupportedGrammarLanguages( ISpeechRecognizerStatics *iface, IVectorView_Language **languages )
{
    FIXME("iface %p, languages %p stub!\n", iface, languages);
    return E_NOTIMPL;
}

static const struct ISpeechRecognizerStaticsVtbl speech_recognizer_statics_vtbl =
{
    /* IUnknown methods */
    statics_QueryInterface,
    statics_AddRef,
    statics_Release,
    /* IInspectable methods */
    statics_GetIids,
    statics_GetRuntimeClassName,
    statics_GetTrustLevel,
    /* ISpeechRecognizerStatics2 methods */
    statics_get_SystemSpeechLanguage,
    statics_get_SupportedTopicLanguages,
    statics_get_SupportedGrammarLanguages
};

/*
 *
 * ISpeechRecognizerStatics2
 *
 */

DEFINE_IINSPECTABLE(statics2, ISpeechRecognizerStatics2, struct recognizer_statics, IActivationFactory_iface)

static HRESULT WINAPI statics2_TrySetSystemSpeechLanguageAsync( ISpeechRecognizerStatics2 *iface,
                                                                ILanguage *language,
                                                                IAsyncOperation_boolean **operation)
{
    FIXME("iface %p, operation %p stub!\n", iface, operation);
    return E_NOTIMPL;
}

static const struct ISpeechRecognizerStatics2Vtbl speech_recognizer_statics2_vtbl =
{
    /* IUnknown methods */
    statics2_QueryInterface,
    statics2_AddRef,
    statics2_Release,
    /* IInspectable methods */
    statics2_GetIids,
    statics2_GetRuntimeClassName,
    statics2_GetTrustLevel,
    /* ISpeechRecognizerStatics2 methods */
    statics2_TrySetSystemSpeechLanguageAsync,
};

static struct recognizer_statics recognizer_statics =
{
    .IActivationFactory_iface = {&activation_factory_vtbl},
    .ISpeechRecognizerFactory_iface = {&speech_recognizer_factory_vtbl},
    .ISpeechRecognizerStatics_iface = {&speech_recognizer_statics_vtbl},
    .ISpeechRecognizerStatics2_iface = {&speech_recognizer_statics2_vtbl},
    .ref = 1
};

IActivationFactory *recognizer_factory = &recognizer_statics.IActivationFactory_iface;
