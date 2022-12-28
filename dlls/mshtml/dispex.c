/*
 * Copyright 2008-2009 Jacek Caban for CodeWeavers
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
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "mshtmdid.h"
#include "mscoree.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

#define MAX_ARGS 16

static CRITICAL_SECTION cs_dispex_static_data;
static CRITICAL_SECTION_DEBUG cs_dispex_static_data_dbg =
{
    0, 0, &cs_dispex_static_data,
    { &cs_dispex_static_data_dbg.ProcessLocksList, &cs_dispex_static_data_dbg.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": dispex_static_data") }
};
static CRITICAL_SECTION cs_dispex_static_data = { &cs_dispex_static_data_dbg, -1, 0, 0, 0, 0 };

typedef struct {
    IID iid;
    VARIANT default_value;
} func_arg_info_t;

typedef struct {
    DISPID id;
    BSTR name;
    tid_t tid;
    dispex_hook_invoke_t hook;
    SHORT call_vtbl_off;
    SHORT put_vtbl_off;
    SHORT get_vtbl_off;
    SHORT func_disp_idx;
    USHORT argc;
    USHORT default_value_cnt;
    VARTYPE prop_vt;
    VARTYPE *arg_types;
    func_arg_info_t *arg_info;
} func_info_t;

struct dispex_data_t {
    dispex_static_data_t *desc;
    compat_mode_t compat_mode;

    DWORD func_cnt;
    DWORD func_size;
    func_info_t *funcs;
    func_info_t **name_table;
    DWORD func_disp_cnt;

    struct list entry;
};

typedef struct {
    VARIANT var;
    LPWSTR name;
    DWORD flags;
} dynamic_prop_t;

struct proxy_prototype {
    IUnknown IUnknown_iface;
    DispatchEx dispex;
    LONG ref;
};

struct proxy_ctor {
    IUnknown IUnknown_iface;
    DispatchEx dispex;
    LONG ref;
};

#define DYNPROP_DELETED    0x01
#define DYNPROP_PROTREF    0x02  /* V_VT(var) == VT_EMPTY and V_UI4(var) == the ref */

typedef struct {
    DispatchEx dispex;
    IUnknown IUnknown_iface;
    LONG ref;
    union {
        DispatchEx *obj;
        DWORD idx;          /* index into function_props, used when info == NULL */
    };
    func_info_t *info;
    IDispatch *funcs[2];    /* apply, call */
} func_disp_t;

typedef struct {
    func_disp_t *func_obj;
    VARIANT val;
} func_obj_entry_t;

struct dispex_dynamic_data_t {
    DWORD buf_size;
    DWORD prop_cnt;
    dynamic_prop_t *props;
    func_obj_entry_t *func_disps;
};

#define DISPID_DYNPROP_0    0x50000000
#define DISPID_DYNPROP_MAX  0x5fffffff

#define FDEX_VERSION_MASK 0xf0000000

static ITypeLib *typelib, *typelib_private;
static ITypeInfo *typeinfos[LAST_tid];
static struct list dispex_data_list = LIST_INIT(dispex_data_list);

static REFIID tid_ids[] = {
#define XIID(iface) &IID_ ## iface,
#define XDIID(iface) &DIID_ ## iface,
TID_LIST
    NULL,
PRIVATE_TID_LIST
#undef XIID
#undef XDIID
};

const tid_t no_iface_tids[1] = { 0 };
static void proxy_prototype_init_dispex_info(dispex_data_t*,compat_mode_t);

static struct prototype_static_data {
    dispex_static_data_t dispex;
    dispex_static_data_t *desc;
} prototype_static_data[] = {
#define X(id, name, dispex, proto_id) \
{                                     \
    {                                 \
        L ## name L"Prototype",       \
        NULL,                         \
        PROTO_ID_ ## proto_id,        \
        NULL_tid,                     \
        no_iface_tids,                \
        proxy_prototype_init_dispex_info \
    },                                \
    &dispex                           \
},
COMPAT_ONLY_PROTOTYPE_LIST
COMPAT_PROTOTYPE_LIST
PROXY_PROTOTYPE_LIST
#undef X
};

static const WCHAR compat_prototype_nameW[] = L"[Interface prototype object]";
static void compat_prototype_init_dispex_info(dispex_data_t*,compat_mode_t);
static const dispex_static_data_vtbl_t compat_prototype_dispex_vtbl;

static dispex_static_data_t compat_prototype_dispex[] = {
#define X(id, name, dispex, proto_id) \
{                                     \
    compat_prototype_nameW,           \
    &compat_prototype_dispex_vtbl,    \
    PROTO_ID_NULL,                    \
    NULL_tid,                         \
    no_iface_tids,                    \
    compat_prototype_init_dispex_info \
},
COMPAT_ONLY_PROTOTYPE_LIST
COMPAT_PROTOTYPE_LIST
#undef X
};

static const dispex_static_data_vtbl_t proxy_ctor_dispex_vtbl;

static dispex_static_data_t proxy_ctor_dispex[] = {
#define X(id, name, dispex, proto_id) \
{                                     \
    L ## name,                        \
    &proxy_ctor_dispex_vtbl,          \
    PROTO_ID_Object,                  \
    NULL_tid,                         \
    no_iface_tids                     \
},
COMPAT_PROTOTYPE_LIST
PROXY_PROTOTYPE_LIST
#undef X
};

static inline dispex_data_t *proxy_prototype_object_info(struct proxy_prototype *prot)
{
    dispex_static_data_t *desc = CONTAINING_RECORD(prot->dispex.info->desc, struct prototype_static_data, dispex)->desc;
    return desc->info_cache[prot->dispex.info->compat_mode];
}

static func_disp_t *create_func_disp(DispatchEx*,func_info_t*);
static HRESULT get_dynamic_prop(DispatchEx*,const WCHAR*,DWORD,dynamic_prop_t**);
static HRESULT invoke_builtin_function(IDispatch*,func_info_t*,DISPPARAMS*,VARIANT*,EXCEPINFO*,IServiceProvider*);
static inline BOOL is_compat_prototype(IDispatch*);
static inline struct proxy_prototype *to_proxy_prototype(DispatchEx*);

static HRESULT load_typelib(void)
{
    WCHAR module_path[MAX_PATH + 3];
    HRESULT hres;
    ITypeLib *tl;
    DWORD len;

    hres = LoadRegTypeLib(&LIBID_MSHTML, 4, 0, LOCALE_SYSTEM_DEFAULT, &tl);
    if(FAILED(hres)) {
        ERR("LoadRegTypeLib failed: %08lx\n", hres);
        return hres;
    }

    if(InterlockedCompareExchangePointer((void**)&typelib, tl, NULL))
        ITypeLib_Release(tl);

    len = GetModuleFileNameW(hInst, module_path, MAX_PATH + 1);
    if (!len || len == MAX_PATH + 1)
    {
        ERR("Could not get module file name, len %lu.\n", len);
        return E_FAIL;
    }
    lstrcatW(module_path, L"\\1");

    hres = LoadTypeLibEx(module_path, REGKIND_NONE, &tl);
    if(FAILED(hres)) {
        ERR("LoadTypeLibEx failed for private typelib: %08lx\n", hres);
        return hres;
    }

    if(InterlockedCompareExchangePointer((void**)&typelib_private, tl, NULL))
        ITypeLib_Release(tl);

    return S_OK;
}

static HRESULT get_typeinfo(tid_t tid, ITypeInfo **typeinfo)
{
    HRESULT hres;

    if (!typelib)
        hres = load_typelib();
    if (!typelib)
        return hres;

    if(!typeinfos[tid]) {
        ITypeInfo *ti;

        hres = ITypeLib_GetTypeInfoOfGuid(tid > LAST_public_tid ? typelib_private : typelib, tid_ids[tid], &ti);
        if(FAILED(hres)) {
            ERR("GetTypeInfoOfGuid(%s) failed: %08lx\n", debugstr_mshtml_guid(tid_ids[tid]), hres);
            return hres;
        }

        if(InterlockedCompareExchangePointer((void**)(typeinfos+tid), ti, NULL))
            ITypeInfo_Release(ti);
    }

    *typeinfo = typeinfos[tid];
    return S_OK;
}

void release_typelib(void)
{
    dispex_data_t *iter;
    unsigned i, j;

    while(!list_empty(&dispex_data_list)) {
        iter = LIST_ENTRY(list_head(&dispex_data_list), dispex_data_t, entry);
        list_remove(&iter->entry);

        for(i = 0; i < iter->func_cnt; i++) {
            if(iter->funcs[i].default_value_cnt && iter->funcs[i].arg_info) {
                for(j = 0; j < iter->funcs[i].argc; j++)
                    VariantClear(&iter->funcs[i].arg_info[j].default_value);
            }
            heap_free(iter->funcs[i].arg_types);
            heap_free(iter->funcs[i].arg_info);
            SysFreeString(iter->funcs[i].name);
        }

        heap_free(iter->funcs);
        heap_free(iter->name_table);
        heap_free(iter);
    }

    if(!typelib)
        return;

    for(i=0; i < ARRAY_SIZE(typeinfos); i++)
        if(typeinfos[i])
            ITypeInfo_Release(typeinfos[i]);

    ITypeLib_Release(typelib);
    ITypeLib_Release(typelib_private);
    DeleteCriticalSection(&cs_dispex_static_data);
}

HRESULT get_class_typeinfo(const CLSID *clsid, ITypeInfo **typeinfo)
{
    HRESULT hres;

    if (!typelib)
        hres = load_typelib();
    if (!typelib)
        return hres;

    hres = ITypeLib_GetTypeInfoOfGuid(typelib, clsid, typeinfo);
    if (FAILED(hres))
        hres = ITypeLib_GetTypeInfoOfGuid(typelib_private, clsid, typeinfo);
    if(FAILED(hres))
        ERR("GetTypeInfoOfGuid failed: %08lx\n", hres);
    return hres;
}

/* Not all argument types are supported yet */
#define BUILTIN_ARG_TYPES_SWITCH                        \
    CASE_VT(VT_I2, INT16, V_I2);                        \
    CASE_VT(VT_UI2, UINT16, V_UI2);                     \
    CASE_VT(VT_I4, INT32, V_I4);                        \
    CASE_VT(VT_UI4, UINT32, V_UI4);                     \
    CASE_VT(VT_R4, float, V_R4);                        \
    CASE_VT(VT_BSTR, BSTR, V_BSTR);                     \
    CASE_VT(VT_DISPATCH, IDispatch*, V_DISPATCH);       \
    CASE_VT(VT_BOOL, VARIANT_BOOL, V_BOOL)

/* List all types used by IDispatchEx-based properties */
#define BUILTIN_TYPES_SWITCH                            \
    BUILTIN_ARG_TYPES_SWITCH;                           \
    CASE_VT(VT_VARIANT, VARIANT, *);                    \
    CASE_VT(VT_PTR, void*, V_BYREF);                    \
    CASE_VT(VT_UNKNOWN, IUnknown*, V_UNKNOWN);          \
    CASE_VT(VT_UI8, ULONGLONG, V_UI8)

static BOOL is_arg_type_supported(VARTYPE vt)
{
    switch(vt) {
#define CASE_VT(x,a,b) case x: return TRUE
    BUILTIN_ARG_TYPES_SWITCH;
#undef CASE_VT
    }
    return FALSE;
}

static void add_func_info(dispex_data_t *data, tid_t tid, const FUNCDESC *desc, ITypeInfo *dti,
                          dispex_hook_invoke_t hook)
{
    func_info_t *info;
    BSTR name;
    HRESULT hres;

    hres = ITypeInfo_GetDocumentation(dti, desc->memid, &name, NULL, NULL, NULL);
    if(FAILED(hres)) {
        WARN("GetDocumentation failed: %08lx\n", hres);
        return;
    }

    for(info = data->funcs; info < data->funcs+data->func_cnt; info++) {
        if(info->id == desc->memid || !wcscmp(info->name, name)) {
            if(info->tid != tid) {
                SysFreeString(name);
                return; /* Duplicated in other interface */
            }
            break;
        }
    }

    TRACE("adding %s...\n", debugstr_w(name));

    if(info == data->funcs+data->func_cnt) {
        if(data->func_cnt == data->func_size)
            data->funcs = heap_realloc_zero(data->funcs, (data->func_size <<= 1)*sizeof(func_info_t));
        info = data->funcs+data->func_cnt;

        data->func_cnt++;

        info->id = desc->memid;
        info->name = name;
        info->tid = tid;
        info->func_disp_idx = -1;
        info->prop_vt = VT_EMPTY;
        info->hook = hook;
    }else {
        SysFreeString(name);
    }

    if(desc->invkind & DISPATCH_METHOD) {
        unsigned i;

        info->func_disp_idx = data->func_disp_cnt++;
        info->argc = desc->cParams;

        assert(info->argc < MAX_ARGS);
        assert(desc->funckind == FUNC_DISPATCH);

        info->arg_info = heap_alloc_zero(sizeof(*info->arg_info) * info->argc);
        if(!info->arg_info)
            return;

        info->prop_vt = desc->elemdescFunc.tdesc.vt;
        if(info->prop_vt != VT_VOID && info->prop_vt != VT_PTR && !is_arg_type_supported(info->prop_vt)) {
            TRACE("%s: return type %d\n", debugstr_w(info->name), info->prop_vt);
            return; /* Fallback to ITypeInfo::Invoke */
        }

        info->arg_types = heap_alloc(sizeof(*info->arg_types) * (info->argc + (info->prop_vt == VT_VOID ? 0 : 1)));
        if(!info->arg_types)
            return;

        for(i=0; i < info->argc; i++)
            info->arg_types[i] = desc->lprgelemdescParam[i].tdesc.vt;

        if(info->prop_vt == VT_PTR)
            info->arg_types[info->argc] = VT_BYREF | VT_DISPATCH;
        else if(info->prop_vt != VT_VOID)
            info->arg_types[info->argc] = VT_BYREF | info->prop_vt;

        if(desc->cParamsOpt) {
            TRACE("%s: optional params\n", debugstr_w(info->name));
            return; /* Fallback to ITypeInfo::Invoke */
        }

        for(i=0; i < info->argc; i++) {
            TYPEDESC *tdesc = &desc->lprgelemdescParam[i].tdesc;
            if(tdesc->vt == VT_PTR && tdesc->u.lptdesc->vt == VT_USERDEFINED) {
                ITypeInfo *ref_type_info;
                TYPEATTR *attr;

                hres = ITypeInfo_GetRefTypeInfo(dti, tdesc->u.lptdesc->u.hreftype, &ref_type_info);
                if(FAILED(hres)) {
                    ERR("Could not get referenced type info: %08lx\n", hres);
                    return;
                }

                hres = ITypeInfo_GetTypeAttr(ref_type_info, &attr);
                if(SUCCEEDED(hres)) {
                    assert(attr->typekind == TKIND_DISPATCH);
                    info->arg_info[i].iid = attr->guid;
                    ITypeInfo_ReleaseTypeAttr(ref_type_info, attr);
                }else {
                    ERR("GetTypeAttr failed: %08lx\n", hres);
                }
                ITypeInfo_Release(ref_type_info);
                if(FAILED(hres))
                    return;
                info->arg_types[i] = VT_DISPATCH;
            }else if(!is_arg_type_supported(info->arg_types[i])) {
                TRACE("%s: unsupported arg type %s\n", debugstr_w(info->name), debugstr_vt(info->arg_types[i]));
                return; /* Fallback to ITypeInfo for unsupported arg types */
            }

            if(desc->lprgelemdescParam[i].u.paramdesc.wParamFlags & PARAMFLAG_FHASDEFAULT) {
                hres = VariantCopy(&info->arg_info[i].default_value,
                                   &desc->lprgelemdescParam[i].u.paramdesc.pparamdescex->varDefaultValue);
                if(FAILED(hres)) {
                    ERR("Could not copy default value: %08lx\n", hres);
                    return;
                }
                TRACE("%s param %d: default value %s\n", debugstr_w(info->name),
                      i, debugstr_variant(&info->arg_info[i].default_value));
                info->default_value_cnt++;
            }
        }

        assert(info->argc <= MAX_ARGS);
        assert(desc->callconv == CC_STDCALL);

        info->call_vtbl_off = desc->oVft/sizeof(void*);
    }else if(desc->invkind & (DISPATCH_PROPERTYPUT|DISPATCH_PROPERTYGET)) {
        VARTYPE vt = VT_EMPTY;

        if(desc->invkind & DISPATCH_PROPERTYGET) {
            vt = desc->elemdescFunc.tdesc.vt;
            info->get_vtbl_off = desc->oVft/sizeof(void*);
        }
        if(desc->invkind & DISPATCH_PROPERTYPUT) {
            assert(desc->cParams == 1);
            vt = desc->lprgelemdescParam->tdesc.vt;
            info->put_vtbl_off = desc->oVft/sizeof(void*);
        }

        assert(info->prop_vt == VT_EMPTY || vt == info->prop_vt);
        info->prop_vt = vt;
    }
}

static void copy_func_info(func_info_t *dst, func_info_t *src)
{
    unsigned i, argc = src->argc;

    *dst = *src;
    dst->name = SysAllocString(src->name);

    if(src->arg_types) {
        DWORD size = (argc + (src->prop_vt == VT_VOID ? 0 : 1)) * sizeof(*dst->arg_types);
        dst->arg_types = heap_alloc(size);
        if(dst->arg_types)
            memcpy(dst->arg_types, src->arg_types, size);
    }

    if(src->arg_info) {
        dst->arg_info = heap_alloc(argc * sizeof(*dst->arg_info));
        if(dst->arg_info) {
            for(i = 0; i < argc; i++) {
                dst->arg_info[i].iid = src->arg_info[i].iid;
                V_VT(&dst->arg_info[i].default_value) = VT_EMPTY;
                VariantCopy(&dst->arg_info[i].default_value, &src->arg_info[i].default_value);
            }
        }
    }
}

static HRESULT process_interface(dispex_data_t *data, tid_t tid, ITypeInfo *disp_typeinfo, const dispex_hook_t *hooks)
{
    unsigned i = 7; /* skip IDispatch functions */
    ITypeInfo *typeinfo;
    FUNCDESC *funcdesc;
    HRESULT hres;

    hres = get_typeinfo(tid, &typeinfo);
    if(FAILED(hres))
        return hres;

    while(1) {
        const dispex_hook_t *hook = NULL;

        hres = ITypeInfo_GetFuncDesc(typeinfo, i++, &funcdesc);
        if(FAILED(hres))
            break;

        if(hooks) {
            for(hook = hooks; hook->dispid != DISPID_UNKNOWN; hook++) {
                if(hook->dispid == funcdesc->memid)
                    break;
            }
            if(hook->dispid == DISPID_UNKNOWN)
                hook = NULL;
        }

        if(!hook || hook->invoke) {
            add_func_info(data, tid, funcdesc, disp_typeinfo ? disp_typeinfo : typeinfo,
                          hook ? hook->invoke : NULL);
        }

        ITypeInfo_ReleaseFuncDesc(typeinfo, funcdesc);
    }

    return S_OK;
}

void dispex_info_add_interface(dispex_data_t *info, tid_t tid, const dispex_hook_t *hooks)
{
    HRESULT hres;

    hres = process_interface(info, tid, NULL, hooks);
    if(FAILED(hres))
        ERR("process_interface failed: %08lx\n", hres);
}

static int __cdecl dispid_cmp(const void *p1, const void *p2)
{
    return ((const func_info_t*)p1)->id - ((const func_info_t*)p2)->id;
}

static int __cdecl func_name_cmp(const void *p1, const void *p2)
{
    return wcsicmp((*(func_info_t* const*)p1)->name, (*(func_info_t* const*)p2)->name);
}

static dispex_data_t *preprocess_dispex_data(dispex_static_data_t *desc, compat_mode_t compat_mode)
{
    const tid_t *tid;
    dispex_data_t *data;
    DWORD i;
    ITypeInfo *dti;
    HRESULT hres;

    if(desc->disp_tid) {
        hres = get_typeinfo(desc->disp_tid, &dti);
        if(FAILED(hres)) {
            ERR("Could not get disp type info: %08lx\n", hres);
            return NULL;
        }
    }

    data = heap_alloc(sizeof(dispex_data_t));
    if (!data) {
        ERR("Out of memory\n");
        return NULL;
    }
    data->desc = desc;
    data->compat_mode = compat_mode;
    data->func_cnt = 0;
    data->func_disp_cnt = 0;
    data->func_size = 16;
    data->funcs = heap_alloc_zero(data->func_size*sizeof(func_info_t));
    if (!data->funcs) {
        heap_free (data);
        ERR("Out of memory\n");
        return NULL;
    }
    list_add_tail(&dispex_data_list, &data->entry);

    if(desc->init_info)
        desc->init_info(data, compat_mode);

    for(tid = desc->iface_tids; *tid; tid++) {
        hres = process_interface(data, *tid, dti, NULL);
        if(FAILED(hres))
            break;
    }

    if(!data->func_cnt) {
        heap_free(data->funcs);
        data->name_table = NULL;
        data->funcs = NULL;
        data->func_size = 0;
        return data;
    }


    data->funcs = heap_realloc(data->funcs, data->func_cnt * sizeof(func_info_t));
    qsort(data->funcs, data->func_cnt, sizeof(func_info_t), dispid_cmp);

    data->name_table = heap_alloc(data->func_cnt * sizeof(func_info_t*));
    for(i=0; i < data->func_cnt; i++)
        data->name_table[i] = data->funcs+i;
    qsort(data->name_table, data->func_cnt, sizeof(func_info_t*), func_name_cmp);
    return data;
}

static int __cdecl id_cmp(const void *p1, const void *p2)
{
    return *(const DISPID*)p1 - *(const DISPID*)p2;
}

HRESULT get_dispids(tid_t tid, DWORD *ret_size, DISPID **ret)
{
    unsigned i, func_cnt;
    FUNCDESC *funcdesc;
    ITypeInfo *ti;
    TYPEATTR *attr;
    DISPID *ids;
    HRESULT hres;

    hres = get_typeinfo(tid, &ti);
    if(FAILED(hres))
        return hres;

    hres = ITypeInfo_GetTypeAttr(ti, &attr);
    if(FAILED(hres)) {
        ITypeInfo_Release(ti);
        return hres;
    }

    func_cnt = attr->cFuncs;
    ITypeInfo_ReleaseTypeAttr(ti, attr);

    ids = heap_alloc(func_cnt*sizeof(DISPID));
    if(!ids) {
        ITypeInfo_Release(ti);
        return E_OUTOFMEMORY;
    }

    for(i=0; i < func_cnt; i++) {
        hres = ITypeInfo_GetFuncDesc(ti, i, &funcdesc);
        if(FAILED(hres))
            break;

        ids[i] = funcdesc->memid;
        ITypeInfo_ReleaseFuncDesc(ti, funcdesc);
    }

    ITypeInfo_Release(ti);
    if(FAILED(hres)) {
        heap_free(ids);
        return hres;
    }

    qsort(ids, func_cnt, sizeof(DISPID), id_cmp);

    *ret_size = func_cnt;
    *ret = ids;
    return S_OK;
}

static inline BOOL is_custom_dispid(DISPID id)
{
    return MSHTML_DISPID_CUSTOM_MIN <= id && id <= MSHTML_DISPID_CUSTOM_MAX;
}

static inline BOOL is_dynamic_dispid(DISPID id)
{
    return DISPID_DYNPROP_0 <= id && id <= DISPID_DYNPROP_MAX;
}

dispex_prop_type_t get_dispid_type(DISPID id)
{
    if(is_dynamic_dispid(id))
        return DISPEXPROP_DYNAMIC;
    if(is_custom_dispid(id))
        return DISPEXPROP_CUSTOM;
    return DISPEXPROP_BUILTIN;
}

BOOL is_custom_attribute(DispatchEx *dispex, const WCHAR *name)
{
    func_info_t **funcs = dispex->info->name_table;
    DWORD i, a = 0, b = dispex->info->func_cnt;
    int c;

    while(a < b) {
        i = (a + b) / 2;
        c = wcsicmp(funcs[i]->name, name);
        if(!c)
            return (funcs[i]->func_disp_idx != -1);
        if(c > 0) b = i;
        else      a = i + 1;
    }
    return TRUE;
}

static HRESULT variant_copy(VARIANT *dest, VARIANT *src)
{
    if(V_VT(src) == VT_BSTR && !V_BSTR(src)) {
        V_VT(dest) = VT_BSTR;
        V_BSTR(dest) = NULL;
        return S_OK;
    }

    return VariantCopy(dest, src);
}

static void fixup_prop_ref(DispatchEx *This, dynamic_prop_t *prop)
{
    dynamic_prop_t *prot_prop;

    if(prop->flags & DYNPROP_DELETED) {
        if(!This->prototype ||
           FAILED(get_dynamic_prop(&This->prototype->dispex, prop->name, fdexNameCaseSensitive, &prot_prop)))
            return;
        if(!(prot_prop->flags & DYNPROP_DELETED)) {
            prop->flags = DYNPROP_PROTREF;
            V_UI4(&prop->var) = prot_prop - This->prototype->dispex.dynamic_data->props;
        }
        return;
    }

    if((prop->flags & DYNPROP_PROTREF) &&
       (This->prototype->dispex.dynamic_data->props[V_UI4(&prop->var)].flags & DYNPROP_DELETED))
        prop->flags = DYNPROP_DELETED;
}

static inline dispex_dynamic_data_t *get_dynamic_data(DispatchEx *This)
{
    if(This->dynamic_data)
        return This->dynamic_data;

    This->dynamic_data = heap_alloc_zero(sizeof(dispex_dynamic_data_t));
    if(!This->dynamic_data)
        return NULL;

    if(This->info->desc->vtbl && This->info->desc->vtbl->populate_props)
        This->info->desc->vtbl->populate_props(This);

    return This->dynamic_data;
}

static HRESULT get_dynamic_prop(DispatchEx *This, const WCHAR *name, DWORD flags, dynamic_prop_t **ret)
{
    const BOOL alloc = flags & fdexNameEnsure;
    dynamic_prop_t *prop, *prot_prop = NULL;
    dispex_dynamic_data_t *data;

    data = get_dynamic_data(This);
    if(!data)
        return E_OUTOFMEMORY;

    for(prop = data->props; prop < data->props+data->prop_cnt; prop++) {
        if(flags & fdexNameCaseInsensitive ? !wcsicmp(prop->name, name) : !wcscmp(prop->name, name)) {
            fixup_prop_ref(This, prop);
            if(prop->flags & DYNPROP_DELETED) {
                if(!alloc)
                    return DISP_E_UNKNOWNNAME;
                prop->flags &= ~DYNPROP_DELETED;
            }
            *ret = prop;
            return S_OK;
        }
    }

    if(This->prototype) {
        HRESULT hres = get_dynamic_prop(&This->prototype->dispex, name, fdexNameCaseSensitive, &prot_prop);
        if(hres != DISP_E_UNKNOWNNAME) {
            if(FAILED(hres))
                return hres;
            if(prot_prop->flags & DYNPROP_DELETED)
                prot_prop = NULL;
        }
    }

    if(!alloc && !prot_prop)
        return DISP_E_UNKNOWNNAME;

    TRACE("creating dynamic prop %s\n", debugstr_w(name));

    if(!data->buf_size) {
        data->props = heap_alloc(sizeof(dynamic_prop_t)*4);
        if(!data->props)
            return E_OUTOFMEMORY;
        data->buf_size = 4;
    }else if(data->buf_size == data->prop_cnt) {
        dynamic_prop_t *new_props;

        new_props = heap_realloc(data->props, sizeof(dynamic_prop_t)*(data->buf_size<<1));
        if(!new_props)
            return E_OUTOFMEMORY;

        data->props = new_props;
        data->buf_size <<= 1;
    }

    prop = data->props + data->prop_cnt;

    prop->name = heap_strdupW(name);
    if(!prop->name)
        return E_OUTOFMEMORY;

    VariantInit(&prop->var);
    prop->flags = 0;
    if(prot_prop) {
        prop->flags = DYNPROP_PROTREF;
        V_UI4(&prop->var) = prot_prop - This->prototype->dispex.dynamic_data->props;
    }
    data->prop_cnt++;
    *ret = prop;
    return S_OK;
}

HRESULT dispex_get_dprop_ref(DispatchEx *This, const WCHAR *name, BOOL alloc, VARIANT **ret)
{
    dynamic_prop_t *prop;
    HRESULT hres;

    hres = get_dynamic_prop(This, name, alloc ? fdexNameEnsure : 0, &prop);
    if(FAILED(hres))
        return hres;

    prop->flags &= ~DYNPROP_PROTREF;
    *ret = &prop->var;
    return S_OK;
}

HRESULT dispex_get_dynid(DispatchEx *This, const WCHAR *name, DISPID *id)
{
    dynamic_prop_t *prop;
    HRESULT hres;

    hres = get_dynamic_prop(This, name, fdexNameEnsure, &prop);
    if(FAILED(hres))
        return hres;

    prop->flags &= ~DYNPROP_PROTREF;
    *id = DISPID_DYNPROP_0 + (prop - This->dynamic_data->props);
    return S_OK;
}

static HRESULT dispex_value(DispatchEx *This, LCID lcid, WORD flags, DISPPARAMS *params,
        VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    HRESULT hres;

    switch(flags) {
    case DISPATCH_PROPERTYGET:
        V_VT(res) = VT_BSTR;
        hres = dispex_to_string(This, &V_BSTR(res));
        if(FAILED(hres))
            return hres;
        break;
    default:
        FIXME("Unimplemented flags %x\n", flags);
        return E_NOTIMPL;
    }

    return S_OK;
}

static HRESULT typeinfo_invoke(IUnknown *iface, func_info_t *func, WORD flags, DISPPARAMS *dp, VARIANT *res,
        EXCEPINFO *ei)
{
    DISPPARAMS params = {dp->rgvarg, NULL, dp->cArgs, 0};
    ITypeInfo *ti;
    UINT argerr=0;
    HRESULT hres;

    if(params.cArgs > func->argc) {
        params.rgvarg += params.cArgs - func->argc;
        params.cArgs = func->argc;
    }

    hres = get_typeinfo(func->tid, &ti);
    if(FAILED(hres)) {
        ERR("Could not get type info: %08lx\n", hres);
        return hres;
    }

    return ITypeInfo_Invoke(ti, iface, func->id, flags, &params, res, ei, &argerr);
}

static inline func_disp_t *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, func_disp_t, IUnknown_iface);
}

static HRESULT WINAPI Function_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    func_disp_t *This = impl_from_IUnknown(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IUnknown_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI Function_AddRef(IUnknown *iface)
{
    func_disp_t *This = impl_from_IUnknown(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI Function_Release(IUnknown *iface)
{
    func_disp_t *This = impl_from_IUnknown(iface);
    LONG ref = InterlockedDecrement(&This->ref);
    unsigned i;

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        assert(!This->info || !This->obj);
        for(i = 0; i < ARRAY_SIZE(This->funcs); i++)
            if(This->funcs[i])
                IDispatch_Release(This->funcs[i]);
        release_dispex(&This->dispex);
        heap_free(This);
    }

    return ref;
}

static const IUnknownVtbl FunctionUnkVtbl = {
    Function_QueryInterface,
    Function_AddRef,
    Function_Release
};

static HRESULT function_apply(func_disp_t *func, DISPPARAMS *dp, LCID lcid, VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    IDispatchEx *dispex = NULL;
    DISPPARAMS params = { 0 };
    HRESULT hres, errcode;
    IDispatch *this_obj;
    DISPID dispid;
    UINT argc = 0;
    VARIANT *arg;
    VARIANT var;

    arg = dp->rgvarg + dp->cArgs - 1;
    if(dp->cArgs < 1 || V_VT(arg) != VT_DISPATCH || !V_DISPATCH(arg))
        return CTL_E_ILLEGALFUNCTIONCALL;
    this_obj = V_DISPATCH(arg);

    errcode = is_compat_prototype(this_obj) ? MSHTML_E_INVALID_PROPERTY : CTL_E_ILLEGALFUNCTIONCALL;

    if(dp->cArgs >= 2) {
        UINT i, err = 0;
        IDispatch *disp;
        BSTR name;

        arg--;
        if((V_VT(arg) & ~VT_BYREF) != VT_DISPATCH)
            return errcode;
        disp = (V_VT(arg) & VT_BYREF) ? *(IDispatch**)(V_BYREF(arg)) : V_DISPATCH(arg);

        /* get the array length */
        if(!(name = SysAllocString(L"length")))
            return E_OUTOFMEMORY;

        hres = IDispatch_QueryInterface(disp, &IID_IDispatchEx, (void**)&dispex);
        if(SUCCEEDED(hres) && dispex)
            hres = IDispatchEx_GetDispID(dispex, name, fdexNameCaseSensitive, &dispid);
        else {
            hres = IDispatch_GetIDsOfNames(disp, &IID_NULL, &name, 1, 0, &dispid);
            dispex = NULL;
        }
        SysFreeString(name);
        if(FAILED(hres) || dispid == DISPID_UNKNOWN) {
            hres = errcode;
            goto fail;
        }

        if(dispex)
            hres = IDispatchEx_InvokeEx(dispex, dispid, lcid, DISPATCH_PROPERTYGET, &params, res, ei, caller);
        else
            hres = IDispatch_Invoke(disp, dispid, &IID_NULL, lcid, DISPATCH_PROPERTYGET, &params, res, ei, &err);
        if(FAILED(hres))
            goto fail;

        if(V_VT(res) == VT_I4)
            V_I4(&var) = V_I4(res);
        else {
            V_VT(&var) = VT_EMPTY;
            hres = change_type(&var, res, VT_I4, caller);
        }
        VariantClear(res);
        if(FAILED(hres) || V_I4(&var) < 0) {
            hres = errcode;
            goto fail;
        }
        params.cArgs = V_I4(&var);

        /* alloc new params */
        if(params.cArgs) {
            if(!(params.rgvarg = heap_alloc(params.cArgs * sizeof(VARIANTARG)))) {
                hres = E_OUTOFMEMORY;
                goto fail;
            }
            for(i = 0; i < params.cArgs; i++) {
                WCHAR buf[12];

                arg = params.rgvarg + params.cArgs - i - 1;
                swprintf(buf, ARRAY_SIZE(buf), L"%u", i);
                if(!(name = SysAllocString(buf))) {
                    hres = E_OUTOFMEMORY;
                    break;
                }
                if(dispex)
                    hres = IDispatchEx_GetDispID(dispex, name, fdexNameCaseSensitive, &dispid);
                else
                    hres = IDispatch_GetIDsOfNames(disp, &IID_NULL, &name, 1, 0, &dispid);
                SysFreeString(name);
                if(FAILED(hres)) {
                    if(hres == DISP_E_UNKNOWNNAME) {
                        V_VT(arg) = VT_EMPTY;
                        continue;
                    }
                    hres = errcode;
                    break;
                }
                if(dispex)
                    hres = IDispatchEx_InvokeEx(dispex, dispid, lcid, DISPATCH_PROPERTYGET, NULL, arg, ei, caller);
                else
                    hres = IDispatch_Invoke(disp, dispid, &IID_NULL, lcid, DISPATCH_PROPERTYGET, NULL, arg, ei, &err);
                if(FAILED(hres))
                    break;
            }
            argc = i;
            if(argc < params.cArgs)
                goto cleanup;
        }
    }

    hres = invoke_builtin_function(this_obj, func->info, &params, res, ei, caller);

cleanup:
    while(argc--)
        VariantClear(&params.rgvarg[params.cArgs - argc - 1]);
    heap_free(params.rgvarg);
fail:
    if(dispex)
        IDispatchEx_Release(dispex);
    return (hres == E_UNEXPECTED) ? errcode : hres;
}

static HRESULT function_call(func_disp_t *func, DISPPARAMS *dp, LCID lcid, VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    DISPPARAMS params = { dp->rgvarg, NULL, dp->cArgs - 1, 0 };
    VARIANT *arg;
    HRESULT hres;

    arg = dp->rgvarg + dp->cArgs - 1;
    if(dp->cArgs < 1 || V_VT(arg) != VT_DISPATCH || !V_DISPATCH(arg))
        return CTL_E_ILLEGALFUNCTIONCALL;

    hres = invoke_builtin_function(V_DISPATCH(arg), func->info, &params, res, ei, caller);

    return (hres != E_UNEXPECTED) ? hres :
           (is_compat_prototype(V_DISPATCH(arg)) ? MSHTML_E_INVALID_PROPERTY : CTL_E_ILLEGALFUNCTIONCALL);
}

static const struct {
    const WCHAR *name;
    HRESULT (*invoke)(func_disp_t*,DISPPARAMS*,LCID,VARIANT*,EXCEPINFO*,IServiceProvider*);
} function_props[] = {
    { L"apply", function_apply },
    { L"call",  function_call }
};

static inline func_disp_t *impl_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, func_disp_t, dispex);
}

static HRESULT function_value(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *params,
        VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    func_disp_t *This = impl_from_DispatchEx(dispex);
    HRESULT hres;

    switch(flags) {
    case DISPATCH_CONSTRUCT:
        return MSHTML_E_INVALID_PROPERTY;
    case DISPATCH_METHOD|DISPATCH_PROPERTYGET:
        if(!res)
            return E_INVALIDARG;
        /* fall through */
    case DISPATCH_METHOD:
        if(!This->info)
            return MSHTML_E_INVALID_PROPERTY;
        if(!This->obj)
            return E_UNEXPECTED;
        hres = invoke_builtin_function((IDispatch*)&This->obj->IDispatchEx_iface, This->info, params, res, ei, caller);
        break;
    case DISPATCH_PROPERTYGET: {
        unsigned name_len;
        WCHAR *ptr;
        BSTR str;

        static const WCHAR func_prefixW[] =
            {'\n','f','u','n','c','t','i','o','n',' '};
        static const WCHAR func_suffixW[] =
            {'(',')',' ','{','\n',' ',' ',' ',' ','[','n','a','t','i','v','e',' ','c','o','d','e',']','\n','}','\n'};

        /* FIXME: This probably should be more generic. Also we should try to get IID_IActiveScriptSite and SID_GetCaller. */
        if(!caller)
            return E_ACCESSDENIED;

        name_len = This->info ? SysStringLen(This->info->name) : wcslen(function_props[This->idx].name);
        ptr = str = SysAllocStringLen(NULL, name_len + ARRAY_SIZE(func_prefixW) + ARRAY_SIZE(func_suffixW));
        if(!str)
            return E_OUTOFMEMORY;

        memcpy(ptr, func_prefixW, sizeof(func_prefixW));
        ptr += ARRAY_SIZE(func_prefixW);

        memcpy(ptr, This->info ? This->info->name : function_props[This->idx].name, name_len*sizeof(WCHAR));
        ptr += name_len;

        memcpy(ptr, func_suffixW, sizeof(func_suffixW));

        V_VT(res) = VT_BSTR;
        V_BSTR(res) = str;
        return S_OK;
    }
    default:
        FIXME("Unimplemented flags %x\n", flags);
        hres = E_NOTIMPL;
    }

    return hres;
}

static HRESULT function_get_dispid(DispatchEx *dispex, BSTR name, DWORD flags, DISPID *dispid)
{
    func_disp_t *This = impl_from_DispatchEx(dispex);
    DWORD i;

    for(i = 0; i < ARRAY_SIZE(function_props); i++) {
        if((flags & fdexNameCaseInsensitive) ? wcsicmp(name, function_props[i].name) : wcscmp(name, function_props[i].name))
            continue;

        /* can't chain apply/call */
        if(!This->info)
            return DISP_E_UNKNOWNNAME;

        *dispid = MSHTML_DISPID_CUSTOM_MIN + i;
        return S_OK;
    }
    return DISP_E_UNKNOWNNAME;
}

static HRESULT function_invoke(DispatchEx *dispex, IDispatch *this_obj, DISPID id, LCID lcid, WORD flags,
        DISPPARAMS *params, VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    func_disp_t *This = impl_from_DispatchEx(dispex);
    DWORD idx = id - MSHTML_DISPID_CUSTOM_MIN;

    if(idx >= ARRAY_SIZE(function_props) || !This->info)
        return DISP_E_MEMBERNOTFOUND;

    switch(flags) {
    case DISPATCH_METHOD|DISPATCH_PROPERTYGET:
        if(!res)
            return E_INVALIDARG;
        /* fall through */
    case DISPATCH_METHOD:
        return function_props[idx].invoke(This, params, lcid, res, ei, caller);
    case DISPATCH_PROPERTYGET:
        if(!This->funcs[idx]) {
            func_disp_t *disp = create_func_disp(dispex, NULL);
            if(!disp)
                return E_OUTOFMEMORY;
            disp->idx = idx;
            This->funcs[idx] = (IDispatch*)&disp->dispex.IDispatchEx_iface;
        }
        V_VT(res) = VT_DISPATCH;
        V_DISPATCH(res) = This->funcs[idx];
        IDispatch_AddRef(This->funcs[idx]);
        break;
    default:
        return MSHTML_E_INVALID_PROPERTY;
    }

    return S_OK;
}

static const dispex_static_data_vtbl_t function_dispex_vtbl = {
    function_value,
    function_get_dispid,
    function_invoke,
    NULL
};

static dispex_static_data_t function_dispex = {
    L"Function",
    &function_dispex_vtbl,
    PROTO_ID_NULL,
    NULL_tid,
    no_iface_tids
};

static func_disp_t *create_func_disp(DispatchEx *obj, func_info_t *info)
{
    func_disp_t *ret;

    ret = heap_alloc_zero(sizeof(func_disp_t));
    if(!ret)
        return NULL;

    ret->IUnknown_iface.lpVtbl = &FunctionUnkVtbl;
    init_dispatch(&ret->dispex, &ret->IUnknown_iface,  &function_dispex, NULL, dispex_compat_mode(obj));
    ret->ref = 1;
    ret->obj = obj;
    ret->info = info;

    return ret;
}

static HRESULT invoke_disp_value(IDispatch *this_obj, IDispatch *func_disp, LCID lcid, WORD flags, DISPPARAMS *dp,
        VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    DISPID named_arg = DISPID_THIS;
    DISPPARAMS new_dp = {NULL, &named_arg, 0, 1};
    IDispatchEx *dispex;
    HRESULT hres;

    if(dp->cNamedArgs) {
        FIXME("named args not supported\n");
        return E_NOTIMPL;
    }

    new_dp.rgvarg = heap_alloc((dp->cArgs+1)*sizeof(VARIANTARG));
    if(!new_dp.rgvarg)
        return E_OUTOFMEMORY;

    new_dp.cArgs = dp->cArgs+1;
    memcpy(new_dp.rgvarg+1, dp->rgvarg, dp->cArgs*sizeof(VARIANTARG));

    V_VT(new_dp.rgvarg) = VT_DISPATCH;
    V_DISPATCH(new_dp.rgvarg) = this_obj;

    hres = IDispatch_QueryInterface(func_disp, &IID_IDispatchEx, (void**)&dispex);
    TRACE(">>>\n");
    if(SUCCEEDED(hres)) {
        hres = IDispatchEx_InvokeEx(dispex, DISPID_VALUE, lcid, flags, &new_dp, res, ei, caller);
        IDispatchEx_Release(dispex);
    }else {
        UINT err = 0;
        hres = IDispatch_Invoke(func_disp, DISPID_VALUE, &IID_NULL, lcid, flags, &new_dp, res, ei, &err);
    }
    if(SUCCEEDED(hres))
        TRACE("<<< %s\n", debugstr_variant(res));
    else
        WARN("<<< %08lx\n", hres);

    heap_free(new_dp.rgvarg);
    return hres;
}

static HRESULT get_func_obj_entry(DispatchEx *This, struct compat_prototype *prototype, func_info_t *func,
        func_obj_entry_t **ret)
{
    dispex_dynamic_data_t *dynamic_data;
    func_obj_entry_t *entry;

    /* Use the prototype's if it's not the default while ours is */
    if(prototype && prototype->dispex.dynamic_data && prototype->dispex.dynamic_data->func_disps &&
       prototype->dispex.dynamic_data->func_disps[func->func_disp_idx].func_obj) {
        func_obj_entry_t *prot_entry = prototype->dispex.dynamic_data->func_disps + func->func_disp_idx;

        if(V_VT(&prot_entry->val) != VT_DISPATCH ||
           V_DISPATCH(&prot_entry->val) != (IDispatch*)&prot_entry->func_obj->dispex.IDispatchEx_iface) {
            entry = NULL;
            if(This->dynamic_data && This->dynamic_data->func_disps &&
               This->dynamic_data->func_disps[func->func_disp_idx].func_obj) {
                entry = This->dynamic_data->func_disps + func->func_disp_idx;

                if(V_VT(&entry->val) == VT_DISPATCH &&
                   V_DISPATCH(&entry->val) == (IDispatch*)&entry->func_obj->dispex.IDispatchEx_iface)
                    entry = NULL;
            }
            *ret = entry ? entry : prot_entry;
            return S_OK;
        }
    }

    dynamic_data = get_dynamic_data(This);
    if(!dynamic_data)
        return E_OUTOFMEMORY;

    if(!dynamic_data->func_disps) {
        dynamic_data->func_disps = heap_alloc_zero(This->info->func_disp_cnt * sizeof(*dynamic_data->func_disps));
        if(!dynamic_data->func_disps)
            return E_OUTOFMEMORY;
    }

    entry = dynamic_data->func_disps + func->func_disp_idx;
    if(!entry->func_obj) {
        entry->func_obj = create_func_disp(This, func);
        if(!entry->func_obj)
            return E_OUTOFMEMORY;

        IDispatchEx_AddRef(&entry->func_obj->dispex.IDispatchEx_iface);
        V_VT(&entry->val) = VT_DISPATCH;
        V_DISPATCH(&entry->val) = (IDispatch*)&entry->func_obj->dispex.IDispatchEx_iface;
    }

    *ret = entry;
    return S_OK;
}

static HRESULT get_builtin_func(dispex_data_t *data, DISPID id, func_info_t **ret)
{
    int min, max, n;

    min = 0;
    max = data->func_cnt-1;

    while(min <= max) {
        n = (min+max)/2;

        if(data->funcs[n].id == id) {
            *ret = data->funcs+n;
            return S_OK;
        }

        if(data->funcs[n].id < id)
            min = n+1;
        else
            max = n-1;
    }

    WARN("invalid id %lx\n", id);
    return DISP_E_MEMBERNOTFOUND;
}

static HRESULT get_builtin_func_prot(DispatchEx *This, DISPID id, func_info_t **ret)
{
    if(This->proxy && !to_proxy_prototype(This) && id != DISPID_VALUE && This->info->desc->prototype_id >= 0)
        return DISP_E_MEMBERNOTFOUND;
    return get_builtin_func(This->info, id, ret);
}

static HRESULT get_builtin_id(DispatchEx *This, BSTR name, DWORD grfdex, DISPID *ret)
{
    int min, max, n, c;

    min = 0;
    max = This->info->func_cnt-1;

    while(min <= max) {
        n = (min+max)/2;

        c = wcsicmp(This->info->name_table[n]->name, name);
        if(!c) {
            if((grfdex & fdexNameCaseSensitive) && wcscmp(This->info->name_table[n]->name, name))
                break;

            *ret = This->info->name_table[n]->id;
            return S_OK;
        }

        if(c > 0)
            max = n-1;
        else
            min = n+1;
    }

    if(This->info->desc->vtbl) {
        HRESULT hres;

        if(This->info->desc->vtbl->get_static_dispid) {
            hres = This->info->desc->vtbl->get_static_dispid(dispex_compat_mode(This), name, grfdex, ret);
            if(hres != DISP_E_UNKNOWNNAME)
                return hres;
        }

        if(This->info->desc->vtbl->get_dispid) {
            hres = This->info->desc->vtbl->get_dispid(This, name, grfdex, ret);
            if(hres != DISP_E_UNKNOWNNAME)
                return hres;
        }
    }

    return DISP_E_UNKNOWNNAME;
}

static inline DispatchEx *get_dispex_for_hook(IUnknown *iface)
{
    IWineDispatchProxyPrivate *itf;
    DispatchEx *dispex;

    if(FAILED(IUnknown_QueryInterface(iface, &IID_IWineDispatchProxyPrivate, (void**)&itf)) || !itf)
        return NULL;

    dispex = CONTAINING_RECORD(itf->lpVtbl->GetProxyFieldRef(itf), DispatchEx, proxy);
    IDispatchEx_AddRef(&dispex->IDispatchEx_iface);

    IDispatchEx_Release((IDispatchEx*)itf);
    return dispex;
}

HRESULT change_type(VARIANT *dst, VARIANT *src, VARTYPE vt, IServiceProvider *caller)
{
    V_VT(dst) = VT_EMPTY;

    if(caller) {
        IVariantChangeType *change_type = NULL;
        HRESULT hres;

        hres = IServiceProvider_QueryService(caller, &SID_VariantConversion, &IID_IVariantChangeType, (void**)&change_type);
        if(SUCCEEDED(hres)) {
            hres = IVariantChangeType_ChangeType(change_type, dst, src, LOCALE_NEUTRAL, vt);
            IVariantChangeType_Release(change_type);
            return hres;
        }
    }

    switch(vt) {
    case VT_BOOL:
        if(V_VT(src) == VT_BSTR) {
            V_VT(dst) = VT_BOOL;
            V_BOOL(dst) = variant_bool(V_BSTR(src) && *V_BSTR(src));
            return S_OK;
        }
        break;
    }

    return VariantChangeType(dst, src, 0, vt);
}

static HRESULT builtin_propget(IUnknown *iface, func_info_t *func, DISPPARAMS *dp, VARIANT *res)
{
    HRESULT hres;

    if(dp && dp->cArgs) {
        FIXME("cArgs %d\n", dp->cArgs);
        return E_NOTIMPL;
    }

    assert(func->get_vtbl_off);

    switch(func->prop_vt) {
#define CASE_VT(vt,type,access) \
    case vt: { \
        type val; \
        hres = ((HRESULT (WINAPI*)(IUnknown*,type*))((void**)iface->lpVtbl)[func->get_vtbl_off])(iface,&val); \
        if(SUCCEEDED(hres)) \
            access(res) = val; \
        } \
        break
    BUILTIN_TYPES_SWITCH;
#undef CASE_VT
    default:
        FIXME("Unhandled vt %d\n", func->prop_vt);
        hres = E_NOTIMPL;
    }

    if(FAILED(hres))
        return hres;

    if(func->prop_vt != VT_VARIANT)
        V_VT(res) = func->prop_vt == VT_PTR ? VT_DISPATCH : func->prop_vt;
    return S_OK;
}

static HRESULT builtin_propput(DispatchEx *This, IUnknown *iface, func_info_t *func, DISPPARAMS *dp, IServiceProvider *caller)
{
    VARIANT *v, tmpv;
    HRESULT hres;

    if(dp->cArgs != 1 || (dp->cNamedArgs == 1 && *dp->rgdispidNamedArgs != DISPID_PROPERTYPUT)
            || dp->cNamedArgs > 1) {
        FIXME("invalid args\n");
        return E_INVALIDARG;
    }

    if(!func->put_vtbl_off) {
        if(dispex_compat_mode(This) >= COMPAT_MODE_IE9) {
            WARN("No setter\n");
            return S_OK;
        }
        FIXME("No setter\n");
        return E_FAIL;
    }

    v = dp->rgvarg;
    if(func->prop_vt != VT_VARIANT && V_VT(v) != func->prop_vt) {
        hres = change_type(&tmpv, v, func->prop_vt, caller);
        if(FAILED(hres))
            return hres;
        v = &tmpv;
    }

    switch(func->prop_vt) {
#define CASE_VT(vt,type,access) \
    case vt: \
        hres = ((HRESULT (WINAPI*)(IUnknown*,type))((void**)iface->lpVtbl)[func->put_vtbl_off])(iface,access(v)); \
        break
    BUILTIN_TYPES_SWITCH;
#undef CASE_VT
    default:
        FIXME("Unimplemented vt %d\n", func->prop_vt);
        hres = E_NOTIMPL;
    }

    if(v == &tmpv)
        VariantClear(v);
    return hres;
}

static HRESULT invoke_builtin_function(IDispatch *this_obj, func_info_t *func, DISPPARAMS *dp,
                                       VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    VARIANT arg_buf[MAX_ARGS], *arg_ptrs[MAX_ARGS], *arg, retv, ret_ref, vhres;
    unsigned i, nconv = 0;
    DispatchEx *dispex;
    IUnknown *iface;
    HRESULT hres;

    hres = IDispatch_QueryInterface(this_obj, tid_ids[func->tid], (void**)&iface);
    if(FAILED(hres) || !iface)
        return E_UNEXPECTED;

    if(func->hook && (dispex = get_dispex_for_hook(iface))) {
        hres = func->hook(dispex, DISPATCH_METHOD, dp, res, ei, caller);
        IDispatchEx_Release(&dispex->IDispatchEx_iface);
        if(hres != S_FALSE) {
            IUnknown_Release(iface);
            return hres;
        }
    }

    if(!func->call_vtbl_off) {
        hres = typeinfo_invoke(iface, func, DISPATCH_METHOD, dp, res, ei);
        IUnknown_Release(iface);
        return hres;
    }

    if(dp->cArgs + func->default_value_cnt < func->argc) {
        FIXME("Invalid argument count (expected %u, got %u)\n", func->argc, dp->cArgs);
        IUnknown_Release(iface);
        return E_INVALIDARG;
    }

    for(i=0; i < func->argc; i++) {
        BOOL own_value = FALSE;
        if(i >= dp->cArgs) {
            /* use default value */
            arg_ptrs[i] = &func->arg_info[i].default_value;
            continue;
        }
        arg = dp->rgvarg+dp->cArgs-i-1;
        if(func->arg_types[i] == V_VT(arg)) {
            arg_ptrs[i] = arg;
        }else {
            hres = change_type(arg_buf+nconv, arg, func->arg_types[i], caller);
            if(FAILED(hres))
                break;
            arg_ptrs[i] = arg_buf + nconv++;
            own_value = TRUE;
        }

        if(func->arg_types[i] == VT_DISPATCH && !IsEqualGUID(&func->arg_info[i].iid, &IID_NULL)
            && V_DISPATCH(arg_ptrs[i])) {
            IDispatch *iface;
            if(!own_value) {
                arg_buf[nconv] = *arg_ptrs[i];
                arg_ptrs[i] = arg_buf + nconv++;
            }
            hres = IDispatch_QueryInterface(V_DISPATCH(arg_ptrs[i]), &func->arg_info[i].iid, (void**)&iface);
            if(FAILED(hres)) {
                WARN("Could not get %s iface: %08lx\n", debugstr_guid(&func->arg_info[i].iid), hres);
                break;
            }
            if(own_value)
                IDispatch_Release(V_DISPATCH(arg_ptrs[i]));
            V_DISPATCH(arg_ptrs[i]) = iface;
        }
    }

    if(SUCCEEDED(hres)) {
        if(func->prop_vt == VT_VOID) {
            V_VT(&retv) = VT_EMPTY;
        }else {
            V_VT(&retv) = func->prop_vt;
            arg_ptrs[func->argc] = &ret_ref;
            V_VT(&ret_ref) = VT_BYREF|func->prop_vt;

            switch(func->prop_vt) {
#define CASE_VT(vt,type,access)                         \
            case vt:                                    \
                V_BYREF(&ret_ref) = &access(&retv);     \
                break
            BUILTIN_ARG_TYPES_SWITCH;
#undef CASE_VT
            case VT_PTR:
                V_VT(&retv) = VT_DISPATCH;
                V_VT(&ret_ref) = VT_BYREF | VT_DISPATCH;
                V_BYREF(&ret_ref) = &V_DISPATCH(&retv);
                break;
            default:
                assert(0);
            }
        }

        V_VT(&vhres) = VT_ERROR;
        hres = DispCallFunc(iface, func->call_vtbl_off*sizeof(void*), CC_STDCALL, VT_ERROR,
                    func->argc + (func->prop_vt == VT_VOID ? 0 : 1), func->arg_types, arg_ptrs, &vhres);
    }

    while(nconv--)
        VariantClear(arg_buf+nconv);
    IUnknown_Release(iface);
    if(FAILED(hres))
        return hres;
    if(FAILED(V_ERROR(&vhres)))
        return V_ERROR(&vhres);

    if(res)
        *res = retv;
    else
        VariantClear(&retv);
    return V_ERROR(&vhres);
}

static HRESULT func_invoke(DispatchEx *This, IDispatch *this_obj, func_info_t *func, WORD flags, DISPPARAMS *dp, VARIANT *res,
        EXCEPINFO *ei, IServiceProvider *caller)
{
    func_obj_entry_t *entry;
    HRESULT hres;

    switch(flags) {
    case DISPATCH_METHOD|DISPATCH_PROPERTYGET:
        if(!res)
            return E_INVALIDARG;
        /* fall through */
    case DISPATCH_METHOD:
        entry = NULL;

        if(This->dynamic_data && This->dynamic_data->func_disps &&
           This->dynamic_data->func_disps[func->func_disp_idx].func_obj) {
            entry = This->dynamic_data->func_disps + func->func_disp_idx;

            if(V_VT(&entry->val) == VT_DISPATCH &&
               V_DISPATCH(&entry->val) == (IDispatch*)&entry->func_obj->dispex.IDispatchEx_iface)
                entry = NULL;
        }

        if(!entry && This->prototype) {
            if(This->prototype->dispex.dynamic_data && This->prototype->dispex.dynamic_data->func_disps &&
               This->prototype->dispex.dynamic_data->func_disps[func->func_disp_idx].func_obj)
                entry = This->prototype->dispex.dynamic_data->func_disps + func->func_disp_idx;
        }

        if(entry) {
            if(V_VT(&entry->val) != VT_DISPATCH) {
                FIXME("calling %s not supported\n", debugstr_variant(&entry->val));
                return E_NOTIMPL;
            }

            if((IDispatch*)&entry->func_obj->dispex.IDispatchEx_iface != V_DISPATCH(&entry->val)) {
                if(!V_DISPATCH(&entry->val)) {
                    FIXME("Calling null\n");
                    return E_FAIL;
                }

                hres = invoke_disp_value(this_obj, V_DISPATCH(&entry->val), 0, flags, dp, res, ei, NULL);
                break;
            }
        }

        hres = invoke_builtin_function(this_obj, func, dp, res, ei, caller);
        if(hres == E_UNEXPECTED && dispex_compat_mode(This) < COMPAT_MODE_IE9)
            hres = MSHTML_E_INVALID_PROPERTY;
        break;
    case DISPATCH_PROPERTYGET:
        if(func->id == DISPID_VALUE) {
            BSTR ret;

            hres = dispex_to_string(This, &ret);
            if(FAILED(hres))
                return hres;

            V_VT(res) = VT_BSTR;
            V_BSTR(res) = ret;
            return S_OK;
        }

        hres = get_func_obj_entry(This, This->prototype, func, &entry);
        if(FAILED(hres))
            return hres;

        V_VT(res) = VT_EMPTY;
        return VariantCopy(res, &entry->val);
    case DISPATCH_PROPERTYPUT:
        if(dp->cArgs != 1 || (dp->cNamedArgs == 1 && *dp->rgdispidNamedArgs != DISPID_PROPERTYPUT)
           || dp->cNamedArgs > 1) {
            FIXME("invalid args\n");
            return E_INVALIDARG;
        }

        /*
         * NOTE: Although we have IDispatchEx tests showing, that it's not allowed to set
         * function property using InvokeEx, it's possible to do that from jscript.
         * Native probably uses some undocumented interface in this case, but it should
         * be fine for us to allow IDispatchEx handle that.
         */
        hres = get_func_obj_entry(This, NULL, func, &entry);
        if(FAILED(hres))
            return hres;

        return VariantCopy(&entry->val, dp->rgvarg);
    default:
        FIXME("Unimplemented flags %x\n", flags);
        hres = E_NOTIMPL;
    }

    return hres;
}

static HRESULT invoke_builtin_prop(DispatchEx *This, IDispatch *this_obj, DISPID id, LCID lcid, WORD flags,
        DISPPARAMS *dp, VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    DispatchEx *dispex;
    func_info_t *func;
    IUnknown *iface;
    HRESULT hres;

    if(id == DISPID_VALUE && This->info->desc->vtbl && This->info->desc->vtbl->value) {
        hres = This->info->desc->vtbl->value(This, lcid, flags, dp, res, ei, caller);
        if(hres != S_FALSE)
            return hres;
    }

    hres = get_builtin_func_prot(This, id, &func);
    if(id == DISPID_VALUE && hres == DISP_E_MEMBERNOTFOUND)
        return dispex_value(This, lcid, flags, dp, res, ei, caller);
    if(FAILED(hres))
        return hres;

    if(func->func_disp_idx != -1)
        return func_invoke(This, this_obj, func, flags, dp, res, ei, caller);

    hres = IDispatch_QueryInterface(this_obj, tid_ids[func->tid], (void**)&iface);
    if(FAILED(hres) || !iface) {
        if(dispex_compat_mode(This) >= COMPAT_MODE_IE9)
            return E_UNEXPECTED;
        if(res)
            V_VT(res) = VT_EMPTY;
        return S_OK;
    }

    if(func->hook && (dispex = get_dispex_for_hook(iface))) {
        hres = func->hook(dispex, flags, dp, res, ei, caller);
        IDispatchEx_Release(&dispex->IDispatchEx_iface);
        if(hres != S_FALSE) {
            IUnknown_Release(iface);
            return hres;
        }
    }

    switch(flags) {
    case DISPATCH_PROPERTYPUT:
        if(res)
            V_VT(res) = VT_EMPTY;
        hres = builtin_propput(This, iface, func, dp, caller);
        break;
    case DISPATCH_PROPERTYGET:
        hres = builtin_propget(iface, func, dp, res);
        break;
    default:
        if(!func->get_vtbl_off) {
            hres = typeinfo_invoke(iface, func, flags, dp, res, ei);
        }else {
            VARIANT v;

            hres = builtin_propget(iface, func, NULL, &v);
            if(FAILED(hres))
                break;

            if(flags != (DISPATCH_PROPERTYGET|DISPATCH_METHOD) || dp->cArgs) {
                if(V_VT(&v) != VT_DISPATCH) {
                    FIXME("Not a function %s flags %08x\n", debugstr_variant(&v), flags);
                    VariantClear(&v);
                    hres = E_FAIL;
                    break;
                }

                hres = invoke_disp_value(this_obj, V_DISPATCH(&v), lcid, flags, dp, res, ei, caller);
                IDispatch_Release(V_DISPATCH(&v));
            }else if(res) {
                *res = v;
            }else {
                VariantClear(&v);
            }
        }
    }

    IUnknown_Release(iface);
    return hres;
}

HRESULT dispex_call_builtin(DispatchEx *dispex, DISPID id, DISPPARAMS *dp,
                            VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    func_info_t *func;
    HRESULT hres;

    hres = get_builtin_func(dispex->info, id, &func);
    if(FAILED(hres))
        return hres;

    return invoke_builtin_function((IDispatch*)&dispex->IDispatchEx_iface, func, dp, res, ei, caller);
}

BOOL dispex_is_builtin_attribute(DispatchEx *dispex, DISPID id)
{
    func_info_t *func;

    if(id == DISPID_VALUE)
        return TRUE;

    if(get_dispid_type(id) != DISPEXPROP_BUILTIN)
        return FALSE;

    if(FAILED(get_builtin_func(dispex->info, id, &func)))
        return FALSE;

    return func->func_disp_idx == -1;
}

BOOL dispex_is_builtin_method(DispatchEx *dispex, DISPID id)
{
    func_info_t *func;

    if(get_dispid_type(id) != DISPEXPROP_BUILTIN)
        return FALSE;

    if(FAILED(get_builtin_func(dispex->info, id, &func)) || func->func_disp_idx == -1)
        return FALSE;

    if(dispex->dynamic_data && dispex->dynamic_data->func_disps) {
        func_obj_entry_t *entry = dispex->dynamic_data->func_disps + func->func_disp_idx;

        if(entry->func_obj && (V_VT(&entry->val) != VT_DISPATCH ||
           V_DISPATCH(&entry->val) != (IDispatch*)&entry->func_obj->dispex.IDispatchEx_iface))
            return FALSE;
    }

    return TRUE;
}

BOOL dispex_is_builtin_value(DispatchEx *dispex, DISPID id)
{
    func_info_t *func;

    if(get_dispid_type(id) != DISPEXPROP_BUILTIN)
        return FALSE;

    if(FAILED(get_builtin_func(dispex->info, id, &func)))
        return FALSE;

    if(func->func_disp_idx == -1)
        return TRUE;

    if(dispex->dynamic_data && dispex->dynamic_data->func_disps) {
        func_obj_entry_t *entry = dispex->dynamic_data->func_disps + func->func_disp_idx;

        if(entry->func_obj && (V_VT(&entry->val) != VT_DISPATCH ||
           V_DISPATCH(&entry->val) != (IDispatch*)&entry->func_obj->dispex.IDispatchEx_iface))
            return FALSE;
    }

    return TRUE;
}

static VARIANT_BOOL reset_builtin_func(DispatchEx *dispex, func_info_t *func)
{
    func_obj_entry_t *entry;

    if(!dispex->dynamic_data || !dispex->dynamic_data->func_disps ||
       !dispex->dynamic_data->func_disps[func->func_disp_idx].func_obj)
        return VARIANT_FALSE;

    entry = dispex->dynamic_data->func_disps + func->func_disp_idx;
    if(V_VT(&entry->val) == VT_DISPATCH &&
       V_DISPATCH(&entry->val) == (IDispatch*)&entry->func_obj->dispex.IDispatchEx_iface)
        return VARIANT_FALSE;

    VariantClear(&entry->val);
    V_VT(&entry->val) = VT_DISPATCH;
    V_DISPATCH(&entry->val) = (IDispatch*)&entry->func_obj->dispex.IDispatchEx_iface;
    IDispatch_AddRef(V_DISPATCH(&entry->val));
    return VARIANT_TRUE;
}

HRESULT remove_attribute(DispatchEx *This, DISPID id, VARIANT_BOOL *success)
{
    switch(get_dispid_type(id)) {
    case DISPEXPROP_CUSTOM:
        FIXME("DISPEXPROP_CUSTOM not supported\n");
        return E_NOTIMPL;

    case DISPEXPROP_DYNAMIC: {
        DWORD idx = id - DISPID_DYNPROP_0;
        dynamic_prop_t *prop;

        prop = This->dynamic_data->props+idx;
        if(!(prop->flags & DYNPROP_PROTREF)) {
            VariantClear(&prop->var);
            prop->flags |= DYNPROP_DELETED;
        }
        *success = VARIANT_TRUE;
        return S_OK;
    }
    case DISPEXPROP_BUILTIN: {
        VARIANT var;
        DISPPARAMS dp = {&var,NULL,1,0};
        func_info_t *func;
        IUnknown *iface;
        HRESULT hres;

        hres = get_builtin_func(This->info, id, &func);
        if(FAILED(hres))
            return hres;

        /* For builtin functions, we set their value to the original function. */
        if(func->func_disp_idx != -1) {
            *success = reset_builtin_func(This, func);
            return S_OK;
        }
        *success = VARIANT_TRUE;

        IDispatchEx_QueryInterface(&This->IDispatchEx_iface, tid_ids[func->tid], (void**)&iface);

        V_VT(&var) = VT_EMPTY;
        hres = builtin_propput(This, iface, func, &dp, NULL);
        if(FAILED(hres)) {
            VARIANT *ref;
            hres = dispex_get_dprop_ref(This, func->name, FALSE, &ref);
            if(FAILED(hres) || V_VT(ref) != VT_BSTR)
                *success = VARIANT_FALSE;
            else
                VariantClear(ref);
        }
        IUnknown_Release(iface);
        return S_OK;
    }
    default:
        assert(0);
        return E_FAIL;
    }
}

compat_mode_t dispex_compat_mode(DispatchEx *dispex)
{
    return dispex->info != dispex->info->desc->delayed_init_info
        ? dispex->info->compat_mode
        : dispex->info->desc->vtbl->get_compat_mode(dispex);
}

HRESULT dispex_to_string(DispatchEx *dispex, BSTR *ret)
{
    static const WCHAR prefix[8] = L"[object ";
    static const WCHAR suffix[] = L"]";
    WCHAR buf[ARRAY_SIZE(prefix) + 36 + ARRAY_SIZE(suffix)], *p = buf;
    compat_mode_t compat_mode = dispex_compat_mode(dispex);
    const WCHAR *name = dispex->info->desc->name;
    unsigned len;

    if(!ret)
        return E_INVALIDARG;

    memcpy(p, prefix, sizeof(prefix));
    p += ARRAY_SIZE(prefix);
    if(compat_mode < COMPAT_MODE_IE9)
        p--;
    else {
        len = wcslen(name);
        assert(len <= 36);
        memcpy(p, name, len * sizeof(WCHAR));
        p += len;
    }
    memcpy(p, suffix, sizeof(suffix));

    *ret = SysAllocString(buf);
    return *ret ? S_OK : E_OUTOFMEMORY;
}

static dispex_data_t *ensure_dispex_info(dispex_static_data_t *desc, compat_mode_t compat_mode)
{
    if(!desc->info_cache[compat_mode]) {
        EnterCriticalSection(&cs_dispex_static_data);
        if(!desc->info_cache[compat_mode])
            desc->info_cache[compat_mode] = preprocess_dispex_data(desc, compat_mode);
        LeaveCriticalSection(&cs_dispex_static_data);
    }
    return desc->info_cache[compat_mode];
}

static BOOL ensure_real_info(DispatchEx *dispex)
{
    if(dispex->info != dispex->info->desc->delayed_init_info)
        return TRUE;

    dispex->info = ensure_dispex_info(dispex->info->desc, dispex_compat_mode(dispex));
    return dispex->info != NULL;
}

static inline struct compat_prototype *compat_prototype_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct compat_prototype, IUnknown_iface);
}

static HRESULT WINAPI compat_prototype_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    struct compat_prototype *This = compat_prototype_from_IUnknown(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IUnknown_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI compat_prototype_AddRef(IUnknown *iface)
{
    struct compat_prototype *This = compat_prototype_from_IUnknown(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI compat_prototype_Release(IUnknown *iface)
{
    struct compat_prototype *This = compat_prototype_from_IUnknown(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        release_dispex(&This->dispex);
        heap_free(This);
    }
    return ref;
}

static const IUnknownVtbl compat_prototype_vtbl = {
    compat_prototype_QueryInterface,
    compat_prototype_AddRef,
    compat_prototype_Release
};

static struct compat_prototype *get_compat_prototype(HTMLInnerWindow *window, prototype_id_t prot_id,
        compat_mode_t compat_mode)
{
    struct compat_prototype *prot = window->compat_prototypes[prot_id];

    if(!prot) {
        if(!(prot = heap_alloc(sizeof(*prot))))
            return NULL;

        prot->IUnknown_iface.lpVtbl = &compat_prototype_vtbl;
        prot->ref = 1;
        prot->window = window;
        window->compat_prototypes[prot_id] = prot;

        init_dispatch(&prot->dispex, &prot->IUnknown_iface, &compat_prototype_dispex[prot_id], NULL, compat_mode);
    }

    IUnknown_AddRef(&prot->IUnknown_iface);
    return prot;
}

static inline struct compat_prototype *compat_prototype_from_DispatchEx(DispatchEx *iface)
{
    return CONTAINING_RECORD(iface, struct compat_prototype, dispex);
}

static HRESULT compat_prototype_value(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *params,
        VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    switch(flags) {
    case DISPATCH_METHOD|DISPATCH_PROPERTYGET:
        if(!res)
            return E_INVALIDARG;
        /* fall through */
    case DISPATCH_METHOD:
    case DISPATCH_CONSTRUCT:
        return MSHTML_E_INVALID_ACTION;
    case DISPATCH_PROPERTYGET:
        if(!(V_BSTR(res) = SysAllocString(compat_prototype_nameW)))
            return E_OUTOFMEMORY;
        V_VT(res) = VT_BSTR;
        break;
    case DISPATCH_PROPERTYPUTREF|DISPATCH_PROPERTYPUT:
    case DISPATCH_PROPERTYPUTREF:
    case DISPATCH_PROPERTYPUT:
        break;
    default:
        return E_INVALIDARG;
    }
    return S_OK;
}

static HRESULT compat_prototype_get_dispid(DispatchEx *dispex, BSTR name, DWORD flags, DISPID *dispid)
{
    if(dispex_compat_mode(dispex) == COMPAT_MODE_IE8) {
        if((flags & fdexNameCaseInsensitive) ? !wcsicmp(name, L"constructor") : !wcscmp(name, L"constructor")) {
            *dispid = MSHTML_DISPID_CUSTOM_MIN;
            return S_OK;
        }
    }
    return DISP_E_UNKNOWNNAME;
}

static HRESULT compat_prototype_invoke(DispatchEx *dispex, IDispatch *this_obj, DISPID id, LCID lcid, WORD flags,
        DISPPARAMS *params, VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    static WCHAR ElementW[] = L"Element";
    struct compat_prototype *This = compat_prototype_from_DispatchEx(dispex);
    prototype_id_t prot_id = This->dispex.info->desc - compat_prototype_dispex;
    DWORD idx = id - MSHTML_DISPID_CUSTOM_MIN;
    HTMLInnerWindow *window = This->window;
    DISPPARAMS dp = { 0 };

    if(idx > 0 || dispex_compat_mode(dispex) != COMPAT_MODE_IE8)
        return DISP_E_MEMBERNOTFOUND;

    if(!window)
        return E_UNEXPECTED;

    switch(flags) {
    case DISPATCH_METHOD|DISPATCH_PROPERTYGET:
        if(!res)
            return E_INVALIDARG;
        /* fall through */
    case DISPATCH_METHOD:
        return MSHTML_E_INVALID_PROPERTY;
    case DISPATCH_PROPERTYGET:
        if(prot_id < PROTO_ID_HTMLGenericElement && prot_id != PROTO_ID_HTMLUnknownElement)
            break;
        if(FAILED(IDispatchEx_GetDispID(&window->base.IDispatchEx_iface, ElementW, fdexNameCaseSensitive, &id)))
            break;
        return IDispatchEx_InvokeEx(&window->base.IDispatchEx_iface, id, lcid, DISPATCH_PROPERTYGET, &dp, res, ei, caller);
    case DISPATCH_PROPERTYPUTREF|DISPATCH_PROPERTYPUT:
    case DISPATCH_PROPERTYPUTREF:
    case DISPATCH_PROPERTYPUT:
        return S_OK;
    default:
        return MSHTML_E_INVALID_PROPERTY;
    }

    V_VT(res) = VT_NULL;
    return S_OK;
}

static const dispex_static_data_vtbl_t compat_prototype_dispex_vtbl = {
    compat_prototype_value,
    compat_prototype_get_dispid,
    compat_prototype_invoke
};

static void compat_prototype_init_dispex_info(dispex_data_t *info, compat_mode_t compat_mode)
{
    static const DISPID empty_exclude_list[] = { DISPID_UNKNOWN };
    static const DISPID window_exclude_list[] = {
        DISPID_IHTMLWINDOW2_IMAGE,
        DISPID_IHTMLWINDOW2_OPTION,
        DISPID_IHTMLWINDOW5_XMLHTTPREQUEST,
        DISPID_IWINEHTMLWINDOWPRIVATE_DOMPARSER,
        DISPID_UNKNOWN
    };
    prototype_id_t prot_id = info->desc - compat_prototype_dispex;
    dispex_data_t *data = ensure_dispex_info(prototype_static_data[prot_id].desc, compat_mode);
    const DISPID *exclude = empty_exclude_list;
    func_info_t *func;
    unsigned i, j;

    if(!data)
        return;

    if(prototype_static_data[prot_id].desc == &HTMLWindow_dispex)
        exclude = window_exclude_list;

    /* Copy the info from the object instance data */
    func = heap_realloc_zero(info->funcs, data->func_size * sizeof(*func));
    if(!func)
        return;
    info->funcs = func;
    info->func_disp_cnt = data->func_disp_cnt;
    info->func_size = data->func_size;

    for(i = 0; i < data->func_cnt; i++) {
        for(j = 0; exclude[j] != DISPID_UNKNOWN; j++)
            if(exclude[j] == data->funcs[i].id)
                break;
        if(exclude[j] != DISPID_UNKNOWN)
            continue;
        copy_func_info(func, &data->funcs[i]);
        func++;
    }
    info->func_cnt = func - info->funcs;
}

HRESULT compat_ctor_value(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *params,
        VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    switch(flags) {
    case DISPATCH_METHOD|DISPATCH_PROPERTYGET:
        if(!res)
            return E_INVALIDARG;
        /* fall through */
    case DISPATCH_METHOD:
    case DISPATCH_CONSTRUCT:
        return MSHTML_E_INVALID_ACTION;
    case DISPATCH_PROPERTYGET: {
        static const WCHAR prefix[8] = L"[object ";
        static const WCHAR suffix[] = L"]";
        WCHAR buf[ARRAY_SIZE(prefix) + 28 + ARRAY_SIZE(suffix)], *p = buf;
        const WCHAR *name = dispex->info->desc->name;
        unsigned len = wcslen(name);

        memcpy(p, prefix, sizeof(prefix));
        p += ARRAY_SIZE(prefix);
        memcpy(p, name, len * sizeof(WCHAR));
        p += len;
        memcpy(p, suffix, sizeof(suffix));

        if(!(V_BSTR(res) = SysAllocString(buf)))
            return E_OUTOFMEMORY;
        V_VT(res) = VT_BSTR;
        break;
    }
    case DISPATCH_PROPERTYPUTREF|DISPATCH_PROPERTYPUT:
    case DISPATCH_PROPERTYPUTREF:
    case DISPATCH_PROPERTYPUT:
        break;
    default:
        return E_INVALIDARG;
    }
    return S_OK;
}

HRESULT compat_ctor_get_dispid(DispatchEx *dispex, BSTR name, DWORD flags, DISPID *dispid)
{
    if((flags & fdexNameCaseInsensitive) ? !wcsicmp(name, L"prototype") : !wcscmp(name, L"prototype")) {
        *dispid = MSHTML_DISPID_CUSTOM_MIN;
        return S_OK;
    }
    return DISP_E_UNKNOWNNAME;
}

HRESULT compat_ctor_invoke(DispatchEx *dispex, IDispatch *this_obj, DISPID id, LCID lcid, WORD flags,
        DISPPARAMS *params, VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    struct compat_ctor *This = CONTAINING_RECORD(dispex, struct compat_ctor, dispex);
    DWORD idx = id - MSHTML_DISPID_CUSTOM_MIN;
    struct compat_prototype *prot;

    if(idx > 0)
        return DISP_E_MEMBERNOTFOUND;

    if(!This->window)
        return E_UNEXPECTED;

    switch(flags) {
    case DISPATCH_METHOD|DISPATCH_PROPERTYGET:
        if(!res)
            return E_INVALIDARG;
        /* fall through */
    case DISPATCH_METHOD:
        return MSHTML_E_INVALID_PROPERTY;
    case DISPATCH_PROPERTYGET:
        if(!(prot = get_compat_prototype(This->window, This->prot_id, dispex_compat_mode(dispex))))
            return E_OUTOFMEMORY;
        V_VT(res) = VT_DISPATCH;
        V_DISPATCH(res) = (IDispatch*)&prot->dispex.IDispatchEx_iface;
        break;
    default:
        return MSHTML_E_INVALID_PROPERTY;
    }

    return S_OK;
}

HRESULT compat_ctor_delete(DispatchEx *dispex, DISPID id)
{
    DWORD idx = id - MSHTML_DISPID_CUSTOM_MIN;
    return idx ? S_OK : MSHTML_E_INVALID_PROPERTY;
}

static inline struct proxy_prototype *proxy_prototype_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct proxy_prototype, IUnknown_iface);
}

static HRESULT WINAPI proxy_prototype_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    struct proxy_prototype *This = proxy_prototype_from_IUnknown(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IUnknown_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI proxy_prototype_AddRef(IUnknown *iface)
{
    struct proxy_prototype *This = proxy_prototype_from_IUnknown(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI proxy_prototype_Release(IUnknown *iface)
{
    struct proxy_prototype *This = proxy_prototype_from_IUnknown(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        release_dispex(&This->dispex);
        heap_free(This);
    }
    return ref;
}

static const IUnknownVtbl proxy_prototype_vtbl = {
    proxy_prototype_QueryInterface,
    proxy_prototype_AddRef,
    proxy_prototype_Release
};

static inline struct proxy_prototype *to_proxy_prototype(DispatchEx *dispex)
{
    return (dispex->outer->lpVtbl == &proxy_prototype_vtbl) ? proxy_prototype_from_IUnknown(dispex->outer) : NULL;
}

static void proxy_prototype_init_dispex_info(dispex_data_t *info, compat_mode_t compat_mode)
{
    dispex_static_data_t *desc = CONTAINING_RECORD(info->desc, struct prototype_static_data, dispex)->desc;
    dispex_data_t *data = ensure_dispex_info(desc, compat_mode);
    prototype_id_t prot_id;
    func_info_t *func;
    unsigned i;

    if(!data)
        return;

    /* Copy the info from the object instance data, but
       exclude builtins found up the prototype chain. */
    func = heap_realloc_zero(info->funcs, data->func_size * sizeof(*func));
    if(!func)
        return;
    info->funcs = func;
    info->func_disp_cnt = 0;

    for(i = 0; i < data->func_cnt; i++) {
        BOOL found = FALSE;

        for(prot_id = info->desc->prototype_id; prot_id >= 0; prot_id = prototype_static_data[prot_id].dispex.prototype_id) {
            dispex_data_t *chain_data = ensure_dispex_info(prototype_static_data[prot_id].desc, compat_mode);
            DWORD a = 0, b = chain_data->func_cnt;

            while(a < b) {
                DWORD idx = (a + b) / 2;
                int c = wcsicmp(chain_data->name_table[idx]->name, data->funcs[i].name);
                if(!c) {
                    found = TRUE;
                    break;
                }
                if(c > 0) b = idx;
                else      a = idx + 1;
            }
            if(found)
                break;
        }
        if(found)
            continue;

        copy_func_info(func, &data->funcs[i]);
        if(func->func_disp_idx != -1)
            info->func_disp_cnt++;
        func++;
    }

    info->func_cnt = func - info->funcs;
    info->func_size = max(info->func_cnt, 16);
    func = heap_realloc_zero(info->funcs, info->func_size * sizeof(*func));
    if(func)
        info->funcs = func;
}

static HRESULT get_prototype_builtin_id(struct proxy_prototype *prot, WCHAR *name, DISPID *id)
{
    dispex_data_t *data = prot->dispex.info;
    func_info_t **funcs = data->name_table;
    DWORD i, a = 0, b = data->func_cnt;
    int c;

    while(a < b) {
        i = (a + b) / 2;
        c = wcsicmp(funcs[i]->name, name);
        if(!c) {
            if(wcscmp(funcs[i]->name, name))
                break;
            *id = funcs[i]->id;
            return S_OK;
        }
        if(c > 0) b = i;
        else      a = i + 1;
    }

    data = proxy_prototype_object_info(prot);
    if(data->desc->vtbl && data->desc->vtbl->get_static_dispid)
        return data->desc->vtbl->get_static_dispid(dispex_compat_mode(&prot->dispex), name, fdexNameCaseSensitive, id);
    return DISP_E_UNKNOWNNAME;
}

static IDispatch *get_default_prototype(prototype_id_t prot_id, compat_mode_t compat_mode, struct proxy_prototypes **prots_ref)
{
    unsigned num_prots = ARRAY_SIZE(prototype_static_data) - COMPAT_ONLY_PROTOTYPE_COUNT;
    struct proxy_prototype *prot;
    IDispatch **entry;

    if(!*prots_ref) {
        if(!(*prots_ref = heap_alloc_zero(FIELD_OFFSET(struct proxy_prototypes, disp[num_prots]))))
            return NULL;
        (*prots_ref)->num = num_prots;
    }

    entry = &(*prots_ref)->disp[prot_id - COMPAT_ONLY_PROTOTYPE_COUNT].prototype;
    if(*entry) {
        IDispatch_AddRef(*entry);
        return *entry;
    }

    if(!(prot = heap_alloc(sizeof(*prot))))
        return NULL;

    prot->IUnknown_iface.lpVtbl = &proxy_prototype_vtbl;
    prot->ref = 2;  /* the script's ctx also holds one ref */

    init_dispatch(&prot->dispex, &prot->IUnknown_iface, &prototype_static_data[prot_id].dispex, NULL, compat_mode);

    *entry = (IDispatch*)&prot->dispex.IDispatchEx_iface;
    return *entry;
}

static inline struct proxy_ctor *proxy_ctor_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct proxy_ctor, IUnknown_iface);
}

static HRESULT WINAPI proxy_ctor_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    struct proxy_ctor *This = proxy_ctor_from_IUnknown(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        *ppv = &This->IUnknown_iface;
    }else if(dispex_query_interface(&This->dispex, riid, ppv)) {
        return *ppv ? S_OK : E_NOINTERFACE;
    }else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI proxy_ctor_AddRef(IUnknown *iface)
{
    struct proxy_ctor *This = proxy_ctor_from_IUnknown(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static ULONG WINAPI proxy_ctor_Release(IUnknown *iface)
{
    struct proxy_ctor *This = proxy_ctor_from_IUnknown(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if(!ref) {
        release_dispex(&This->dispex);
        heap_free(This);
    }
    return ref;
}

static const IUnknownVtbl proxy_ctor_vtbl = {
    proxy_ctor_QueryInterface,
    proxy_ctor_AddRef,
    proxy_ctor_Release
};

static HRESULT proxy_ctor_value(DispatchEx *dispex, LCID lcid, WORD flags, DISPPARAMS *params,
        VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    switch(flags) {
    case DISPATCH_METHOD|DISPATCH_PROPERTYGET:
        if(!res)
            return E_INVALIDARG;
        /* fall through */
    case DISPATCH_METHOD:
    case DISPATCH_CONSTRUCT:
        return MSHTML_E_INVALID_ACTION;
    case DISPATCH_PROPERTYGET:
        V_VT(res) = VT_BSTR;
        return dispex_to_string(dispex, &V_BSTR(res));
    case DISPATCH_PROPERTYPUTREF|DISPATCH_PROPERTYPUT:
    case DISPATCH_PROPERTYPUTREF:
    case DISPATCH_PROPERTYPUT:
        break;
    default:
        return E_INVALIDARG;
    }
    return S_OK;
}

static const dispex_static_data_vtbl_t proxy_ctor_dispex_vtbl = {
    proxy_ctor_value,
    NULL
};

static HRESULT WINAPI proxy_func_invoke(IDispatch *this_obj, void *context, DISPPARAMS *dp, VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    func_info_t *func = context;
    return invoke_builtin_function(this_obj, func, dp, res, ei, caller);
}

static HRESULT WINAPI proxy_getter_invoke(IDispatch *this_obj, void *context, DISPPARAMS *dp, VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    func_info_t *func = context;
    DispatchEx *dispex;
    IUnknown *iface;
    HRESULT hres;

    hres = IDispatch_QueryInterface(this_obj, tid_ids[func->tid], (void**)&iface);
    if(FAILED(hres) || !iface)
        return E_UNEXPECTED;

    if(func->hook && (dispex = get_dispex_for_hook(iface))) {
        hres = func->hook(dispex, DISPATCH_PROPERTYGET, dp, res, ei, caller);
        IDispatchEx_Release(&dispex->IDispatchEx_iface);
        if(hres != S_FALSE)
            goto done;
    }
    hres = builtin_propget(iface, func, dp, res);

done:
    IUnknown_Release(iface);
    return hres;
}

static HRESULT WINAPI proxy_setter_invoke(IDispatch *this_obj, void *context, DISPPARAMS *dp, VARIANT *res, EXCEPINFO *ei, IServiceProvider *caller)
{
    static DISPID propput_dispid = DISPID_PROPERTYPUT;
    func_info_t *func = context;
    DispatchEx *dispex;
    IUnknown *iface;
    HRESULT hres;

    dp->cNamedArgs = 1;
    dp->rgdispidNamedArgs = &propput_dispid;

    hres = IDispatch_QueryInterface(this_obj, tid_ids[func->tid], (void**)&iface);
    if(FAILED(hres) || !iface)
        return E_UNEXPECTED;

    if(func->hook && (dispex = get_dispex_for_hook(iface))) {
        hres = func->hook(dispex, DISPATCH_PROPERTYPUT, dp, res, ei, caller);
        IDispatchEx_Release(&dispex->IDispatchEx_iface);
        if(hres != S_FALSE)
            goto done;
    }
    hres = builtin_propput(NULL, iface, func, dp, caller);

done:
    IUnknown_Release(iface);
    return hres;
}

static inline DispatchEx *impl_from_IDispatchEx(IDispatchEx *iface)
{
    return CONTAINING_RECORD(iface, DispatchEx, IDispatchEx_iface);
}

static HRESULT WINAPI DispatchEx_QueryInterface(IDispatchEx *iface, REFIID riid, void **ppv)
{
    DispatchEx *This = impl_from_IDispatchEx(iface);

    return IUnknown_QueryInterface(This->outer, riid, ppv);
}

static ULONG WINAPI DispatchEx_AddRef(IDispatchEx *iface)
{
    DispatchEx *This = impl_from_IDispatchEx(iface);

    return IUnknown_AddRef(This->outer);
}

static ULONG WINAPI DispatchEx_Release(IDispatchEx *iface)
{
    DispatchEx *This = impl_from_IDispatchEx(iface);

    return IUnknown_Release(This->outer);
}

static HRESULT WINAPI DispatchEx_GetTypeInfoCount(IDispatchEx *iface, UINT *pctinfo)
{
    DispatchEx *This = impl_from_IDispatchEx(iface);

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI DispatchEx_GetTypeInfo(IDispatchEx *iface, UINT iTInfo,
                                              LCID lcid, ITypeInfo **ppTInfo)
{
    DispatchEx *This = impl_from_IDispatchEx(iface);
    HRESULT hres;

    TRACE("(%p)->(%u %lu %p)\n", This, iTInfo, lcid, ppTInfo);

    hres = get_typeinfo(This->info->desc->disp_tid, ppTInfo);
    if(FAILED(hres))
        return hres;

    ITypeInfo_AddRef(*ppTInfo);
    return S_OK;
}

static HRESULT WINAPI DispatchEx_GetIDsOfNames(IDispatchEx *iface, REFIID riid,
                                                LPOLESTR *rgszNames, UINT cNames,
                                                LCID lcid, DISPID *rgDispId)
{
    DispatchEx *This = impl_from_IDispatchEx(iface);
    UINT i;
    HRESULT hres;

    if(This->proxy)
        return IDispatchEx_GetIDsOfNames((IDispatchEx*)This->proxy, riid, rgszNames,
                                         cNames, lcid, rgDispId);

    TRACE("(%p)->(%s %p %u %lu %p)\n", This, debugstr_guid(riid), rgszNames, cNames,
          lcid, rgDispId);

    for(i=0; i < cNames; i++) {
        hres = IDispatchEx_GetDispID(&This->IDispatchEx_iface, rgszNames[i], 0, rgDispId+i);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}

static HRESULT WINAPI DispatchEx_Invoke(IDispatchEx *iface, DISPID dispIdMember,
                            REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                            VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    DispatchEx *This = impl_from_IDispatchEx(iface);

    if(This->proxy)
        return IDispatchEx_Invoke((IDispatchEx*)This->proxy, dispIdMember, riid, lcid, wFlags,
                                  pDispParams, pVarResult, pExcepInfo, puArgErr);

    TRACE("(%p)->(%ld %s %ld %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
          lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    return dispex_invoke(This, (IDispatch*)iface, dispIdMember, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, NULL);
}

static HRESULT WINAPI DispatchEx_GetDispID(IDispatchEx *iface, BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    DispatchEx *This = impl_from_IDispatchEx(iface);
    dynamic_prop_t *dprop;
    HRESULT hres;

    if(This->proxy)
        return IDispatchEx_GetDispID((IDispatchEx*)This->proxy, bstrName, grfdex, pid);

    TRACE("(%p)->(%s %lx %p)\n", This, debugstr_w(bstrName), grfdex, pid);

    if(grfdex & ~(fdexNameCaseSensitive|fdexNameCaseInsensitive|fdexNameEnsure|fdexNameImplicit|FDEX_VERSION_MASK))
        FIXME("Unsupported grfdex %lx\n", grfdex);

    if(!ensure_real_info(This))
        return E_OUTOFMEMORY;

    hres = get_builtin_id(This, bstrName, grfdex, pid);
    if(hres != DISP_E_UNKNOWNNAME)
        return hres;

    hres = get_dynamic_prop(This, bstrName, grfdex, &dprop);
    if(FAILED(hres))
        return hres;

    *pid = DISPID_DYNPROP_0 + (dprop - This->dynamic_data->props);
    return S_OK;
}

static HRESULT WINAPI DispatchEx_InvokeEx(IDispatchEx *iface, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *pvarRes, EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    DispatchEx *This = impl_from_IDispatchEx(iface);

    if(This->proxy)
        return IDispatchEx_InvokeEx((IDispatchEx*)This->proxy, id, lcid, wFlags, pdp, pvarRes, pei, pspCaller);

    TRACE("(%p)->(%lx %lx %x %p %p %p %p)\n", This, id, lcid, wFlags, pdp, pvarRes, pei, pspCaller);

    return dispex_invoke(This, (IDispatch*)iface, id, lcid, wFlags, pdp, pvarRes, pei, pspCaller);
}

static HRESULT WINAPI DispatchEx_DeleteMemberByName(IDispatchEx *iface, BSTR name, DWORD grfdex)
{
    DispatchEx *This = impl_from_IDispatchEx(iface);
    DISPID id;
    HRESULT hres;

    if(This->proxy)
        return IDispatchEx_DeleteMemberByName((IDispatchEx*)This->proxy, name, grfdex);

    TRACE("(%p)->(%s %lx)\n", This, debugstr_w(name), grfdex);

    if(dispex_compat_mode(This) < COMPAT_MODE_IE8) {
        /* Not implemented by IE */
        return E_NOTIMPL;
    }

    hres = IDispatchEx_GetDispID(&This->IDispatchEx_iface, name, grfdex & ~fdexNameEnsure, &id);
    if(FAILED(hres)) {
        TRACE("property %s not found\n", debugstr_w(name));
        return hres;
    }

    return dispex_delete_prop(This, id);
}

static HRESULT WINAPI DispatchEx_DeleteMemberByDispID(IDispatchEx *iface, DISPID id)
{
    DispatchEx *This = impl_from_IDispatchEx(iface);

    if(This->proxy)
        return IDispatchEx_DeleteMemberByDispID((IDispatchEx*)This->proxy, id);

    TRACE("(%p)->(%lx)\n", This, id);

    if(dispex_compat_mode(This) < COMPAT_MODE_IE8) {
        /* Not implemented by IE */
        return E_NOTIMPL;
    }

    return dispex_delete_prop(This, id);
}

static HRESULT WINAPI DispatchEx_GetMemberProperties(IDispatchEx *iface, DISPID id, DWORD grfdexFetch, DWORD *pgrfdex)
{
    DispatchEx *This = impl_from_IDispatchEx(iface);

    if(This->proxy)
        return IDispatchEx_GetMemberProperties((IDispatchEx*)This->proxy, id, grfdexFetch, pgrfdex);

    FIXME("(%p)->(%lx %lx %p)\n", This, id, grfdexFetch, pgrfdex);
    return E_NOTIMPL;
}

static HRESULT WINAPI DispatchEx_GetMemberName(IDispatchEx *iface, DISPID id, BSTR *pbstrName)
{
    DispatchEx *This = impl_from_IDispatchEx(iface);
    func_info_t *func;
    HRESULT hres;

    if(This->proxy)
        return IDispatchEx_GetMemberName((IDispatchEx*)This->proxy, id, pbstrName);

    TRACE("(%p)->(%lx %p)\n", This, id, pbstrName);

    if(!ensure_real_info(This))
        return E_OUTOFMEMORY;

    if(is_dynamic_dispid(id)) {
        DWORD idx = id - DISPID_DYNPROP_0;

        if(!get_dynamic_data(This) || This->dynamic_data->prop_cnt <= idx)
            return DISP_E_UNKNOWNNAME;

        *pbstrName = SysAllocString(This->dynamic_data->props[idx].name);
        if(!*pbstrName)
            return E_OUTOFMEMORY;

        return S_OK;
    }

    hres = get_builtin_func_prot(This, id, &func);
    if(FAILED(hres))
        return hres;

    *pbstrName = SysAllocString(func->name);
    if(!*pbstrName)
        return E_OUTOFMEMORY;
    return S_OK;
}

static HRESULT next_dynamic_id(DispatchEx *dispex, DWORD idx, DISPID *ret_id)
{
    /* FIXME: Go through PROTREFs? (must exclude props with same name as builtins) */
    while(idx < dispex->dynamic_data->prop_cnt &&
          dispex->dynamic_data->props[idx].flags & (DYNPROP_DELETED | DYNPROP_PROTREF))
        idx++;

    if(idx == dispex->dynamic_data->prop_cnt) {
        *ret_id = DISPID_STARTENUM;
        return S_FALSE;
    }

    *ret_id = DISPID_DYNPROP_0+idx;
    return S_OK;
}

static HRESULT WINAPI DispatchEx_GetNextDispID(IDispatchEx *iface, DWORD grfdex, DISPID id, DISPID *pid)
{
    DispatchEx *This = impl_from_IDispatchEx(iface);
    func_info_t *func;
    HRESULT hres;

    if(This->proxy)
        return IDispatchEx_GetNextDispID((IDispatchEx*)This->proxy, grfdex, id, pid);

    TRACE("(%p)->(%lx %lx %p)\n", This, grfdex, id, pid);

    if(!ensure_real_info(This))
        return E_OUTOFMEMORY;

    if(is_dynamic_dispid(id)) {
        DWORD idx = id - DISPID_DYNPROP_0;

        if(!get_dynamic_data(This) || This->dynamic_data->prop_cnt <= idx)
            return DISP_E_UNKNOWNNAME;

        return next_dynamic_id(This, idx+1, pid);
    }

    if(id == DISPID_STARTENUM) {
        func = This->info->funcs;
    }else {
        hres = get_builtin_func_prot(This, id, &func);
        if(FAILED(hres))
            return hres;
        func++;
    }

    while(func < This->info->funcs + This->info->func_cnt) {
        /* FIXME: Skip hidden properties */
        if(func->func_disp_idx == -1) {
            *pid = func->id;
            return S_OK;
        }
        func++;
    }

    if(get_dynamic_data(This) && This->dynamic_data->prop_cnt)
        return next_dynamic_id(This, 0, pid);

    *pid = DISPID_STARTENUM;
    return S_FALSE;
}

static HRESULT WINAPI DispatchEx_GetNameSpaceParent(IDispatchEx *iface, IUnknown **ppunk)
{
    DispatchEx *This = impl_from_IDispatchEx(iface);
    FIXME("(%p)->(%p)\n", This, ppunk);
    return E_NOTIMPL;
}

static inline DispatchEx *impl_from_IWineDispatchProxyPrivate(IWineDispatchProxyPrivate *iface)
{
    return impl_from_IDispatchEx((IDispatchEx*)iface);
}

static IWineDispatchProxyCbPrivate** WINAPI WineDispatchProxyPrivate_GetProxyFieldRef(IWineDispatchProxyPrivate *iface)
{
    DispatchEx *This = impl_from_IWineDispatchProxyPrivate(iface);
    return &This->proxy;
}

static IDispatch* WINAPI WineDispatchProxyPrivate_GetDefaultPrototype(IWineDispatchProxyPrivate *iface, struct proxy_prototypes **prots_ref)
{
    DispatchEx *This = impl_from_IWineDispatchProxyPrivate(iface);
    prototype_id_t prot_id = This->info->desc->prototype_id;

    if(prot_id < 0)
        return (IDispatch*)(INT_PTR)prot_id;

    if(prot_id < COMPAT_ONLY_PROTOTYPE_COUNT)
        return (IDispatch*)(INT_PTR)PROTO_ID_NULL;

    return get_default_prototype(prot_id, dispex_compat_mode(This), prots_ref);
}

static IDispatch* WINAPI WineDispatchProxyPrivate_GetDefaultConstructor(IWineDispatchProxyPrivate *iface, struct proxy_prototypes *prots)
{
    static const struct {
        prototype_id_t prot_id;
        DISPID dispid;
    } special_ctors[] = {
        { PROTO_ID_DOMParser,           DISPID_IWINEHTMLWINDOWPRIVATE_DOMPARSER },
        { PROTO_ID_HTMLXMLHttpRequest,  DISPID_IHTMLWINDOW5_XMLHTTPREQUEST },
    };
    DispatchEx *This = impl_from_IWineDispatchProxyPrivate(iface);
    struct proxy_prototype *prot = proxy_prototype_from_IUnknown(This->outer);
    struct proxy_ctor *ctor;
    prototype_id_t prot_id;
    IDispatch **entry;
    unsigned i;

    prot_id = CONTAINING_RECORD(prot->dispex.info->desc, struct prototype_static_data, dispex) - prototype_static_data;

    entry = &prots->disp[prot_id - COMPAT_ONLY_PROTOTYPE_COUNT].ctor;
    if(*entry) {
        IDispatch_AddRef(*entry);
        return *entry;
    }

    for(i = 0; i < ARRAY_SIZE(special_ctors); i++) {
        if(prot_id != special_ctors[i].prot_id)
            continue;
        *entry = This->proxy->lpVtbl->CreateConstructor(This->proxy, special_ctors[i].dispid,
                                                        proxy_ctor_dispex[prot_id - COMPAT_ONLY_PROTOTYPE_COUNT].name);
        if(*entry) {
            IDispatch_AddRef(*entry);
            return *entry;
        }
    }

    if(!(ctor = heap_alloc(sizeof(*ctor))))
        return NULL;

    ctor->IUnknown_iface.lpVtbl = &proxy_ctor_vtbl;
    ctor->ref = 2;  /* the script's ctx also holds one ref */

    init_dispatch(&ctor->dispex, &ctor->IUnknown_iface, &proxy_ctor_dispex[prot_id - COMPAT_ONLY_PROTOTYPE_COUNT],
                  NULL, dispex_compat_mode(This));

    *entry = (IDispatch*)&ctor->dispex.IDispatchEx_iface;
    return *entry;
}

static HRESULT WINAPI WineDispatchProxyPrivate_DefineConstructors(IWineDispatchProxyPrivate *iface, struct proxy_prototypes **prots_ref)
{
    static WCHAR ImageW[] = L"Image";
    static WCHAR OptionW[] = L"Option";
    static const struct {
        WCHAR *name;
        prototype_id_t proto_id;
    } extra_ctors[] = {
        { ImageW,       PROTO_ID_HTMLImgElement },
        { OptionW,      PROTO_ID_HTMLOptionElement },
    };
    DispatchEx *This = impl_from_IWineDispatchProxyPrivate(iface);
    compat_mode_t compat_mode;
    IDispatch *prot;
    unsigned int i;
    HRESULT hres;
    DISPID id;

    if(!ensure_real_info(This))
        return E_OUTOFMEMORY;
    if(This->info->desc != &HTMLWindow_dispex)
        return S_FALSE;
    compat_mode = dispex_compat_mode(This);

    for(i = 0; i < ARRAY_SIZE(proxy_ctor_dispex); i++) {
        if(!(prot = get_default_prototype(i + COMPAT_ONLY_PROTOTYPE_COUNT, compat_mode, prots_ref)))
            return E_OUTOFMEMORY;

        hres = This->proxy->lpVtbl->DefineConstructor(This->proxy, proxy_ctor_dispex[i].name, prot, DISPID_UNKNOWN);
        if(FAILED(hres))
            return hres;
    }

    for(i = 0; i < ARRAY_SIZE(extra_ctors); i++) {
        if(get_builtin_id(This, extra_ctors[i].name, fdexNameCaseSensitive, &id) != S_OK)
            continue;

        if(!(prot = get_default_prototype(extra_ctors[i].proto_id, compat_mode, prots_ref)))
            return E_OUTOFMEMORY;

        hres = This->proxy->lpVtbl->DefineConstructor(This->proxy, extra_ctors[i].name, prot, id);
        if(FAILED(hres))
            return hres;
    }

    return S_OK;
}

static BOOL WINAPI WineDispatchProxyPrivate_IsPrototype(IWineDispatchProxyPrivate *iface)
{
    DispatchEx *This = impl_from_IWineDispatchProxyPrivate(iface);
    return to_proxy_prototype(This) != NULL;
}

static BOOL WINAPI WineDispatchProxyPrivate_IsConstructor(IWineDispatchProxyPrivate *iface)
{
    DispatchEx *This = impl_from_IWineDispatchProxyPrivate(iface);
    return This->outer->lpVtbl == &proxy_ctor_vtbl;
}

static DWORD WINAPI WineDispatchProxyPrivate_PropFlags(IWineDispatchProxyPrivate *iface, DISPID id)
{
    DispatchEx *This = impl_from_IWineDispatchProxyPrivate(iface);
    func_info_t *func;

    if(is_dynamic_dispid(id))
        return PROPF_WRITABLE | PROPF_CONFIGURABLE | PROPF_ENUMERABLE;

    if(is_custom_dispid(id))
        return PROPF_WRITABLE;

    if(FAILED(get_builtin_func_prot(This, id, &func)))
        return 0;

    if(func->func_disp_idx != -1) {
        if(This->dynamic_data && This->dynamic_data->func_disps
           && This->dynamic_data->func_disps[func->func_disp_idx].func_obj) {
            func_obj_entry_t *entry = This->dynamic_data->func_disps + func->func_disp_idx;

            if((IDispatch*)&entry->func_obj->dispex.IDispatchEx_iface != V_DISPATCH(&entry->val))
                return PROPF_WRITABLE | PROPF_CONFIGURABLE;
        }
        return PROPF_METHOD | func->argc | PROPF_WRITABLE | PROPF_CONFIGURABLE;
    }

    /* FIXME: Don't add PROPF_ENUMERABLE to hidden properties */
    return PROPF_PROXY_ACCESSOR | PROPF_ENUMERABLE | PROPF_CONFIGURABLE | (func->put_vtbl_off ? PROPF_WRITABLE : 0);
}

static HRESULT WINAPI WineDispatchProxyPrivate_PropGetID(IWineDispatchProxyPrivate *iface, WCHAR *name, DISPID *id)
{
    DispatchEx *This = impl_from_IWineDispatchProxyPrivate(iface);
    struct proxy_prototype *prot = to_proxy_prototype(This);
    dynamic_prop_t *dprop;
    HRESULT hres;

    if(!ensure_real_info(This))
        return E_OUTOFMEMORY;

    if(!prot && This->info->desc->prototype_id < 0) {
        hres = get_builtin_id(This, name, fdexNameCaseSensitive, id);
        if(hres != DISP_E_UNKNOWNNAME)
            return hres;
    }else {
        if(prot) {
            hres = get_prototype_builtin_id(prot, name, id);
            if(hres != DISP_E_UNKNOWNNAME)
                return hres;
        }

        if(This->info->desc->vtbl && This->info->desc->vtbl->get_dispid) {
            hres = This->info->desc->vtbl->get_dispid(This, name, fdexNameCaseSensitive, id);
            if(hres != DISP_E_UNKNOWNNAME)
                return hres;
        }
    }

    hres = get_dynamic_prop(This, name, fdexNameCaseSensitive, &dprop);
    if(FAILED(hres))
        return hres;

    *id = DISPID_DYNPROP_0 + (dprop - This->dynamic_data->props);
    return S_OK;
}

static HRESULT WINAPI WineDispatchProxyPrivate_PropInvoke(IWineDispatchProxyPrivate *iface, IDispatch *this_obj, DISPID id,
        LCID lcid, DWORD flags, DISPPARAMS *dp, VARIANT *ret, EXCEPINFO *ei, IServiceProvider *caller)
{
    DispatchEx *This = impl_from_IWineDispatchProxyPrivate(iface);

    /* Prototypes use this for their default value, to be able
       to be distinguished from builtin ids with DISPID_VALUE. */
    if(id == DISPID_UNKNOWN)
        return dispex_value(This, lcid, flags, dp, ret, ei, caller);

    return dispex_invoke(This, this_obj, id, lcid, flags, dp, ret, ei, caller);
}

static HRESULT WINAPI WineDispatchProxyPrivate_PropDelete(IWineDispatchProxyPrivate *iface, DISPID id)
{
    DispatchEx *This = impl_from_IWineDispatchProxyPrivate(iface);

    return dispex_delete_prop(This, id);
}

static HRESULT WINAPI WineDispatchProxyPrivate_FuncInfo(IWineDispatchProxyPrivate *iface, DISPID id, struct proxy_func_invoker *ret)
{
    DispatchEx *This = impl_from_IWineDispatchProxyPrivate(iface);
    func_info_t *func;
    HRESULT hres;

    hres = get_builtin_func_prot(This, id, &func);
    if(FAILED(hres))
        return (hres == DISP_E_MEMBERNOTFOUND) ? E_UNEXPECTED : hres;
    if(func->func_disp_idx == -1)
        return E_UNEXPECTED;

    ret->invoke = proxy_func_invoke;
    ret->context = func;
    ret->name = func->name;
    return S_OK;
}

static HRESULT WINAPI WineDispatchProxyPrivate_AccessorInfo(IWineDispatchProxyPrivate *iface, DISPID id, struct proxy_func_invoker *ret)
{
    DispatchEx *This = impl_from_IWineDispatchProxyPrivate(iface);
    func_info_t *func;
    HRESULT hres;

    hres = get_builtin_func_prot(This, id, &func);
    if(FAILED(hres))
        return (hres == DISP_E_MEMBERNOTFOUND) ? E_UNEXPECTED : hres;
    if(func->func_disp_idx != -1)
        return E_UNEXPECTED;

    ret[0].invoke = func->get_vtbl_off ? proxy_getter_invoke : NULL;
    ret[1].invoke = func->put_vtbl_off ? proxy_setter_invoke : NULL;
    ret[0].context = ret[1].context = func;
    ret[0].name = ret[1].name = func->name;
    return S_OK;
}

static HRESULT WINAPI WineDispatchProxyPrivate_ToString(IWineDispatchProxyPrivate *iface, BSTR *string)
{
    DispatchEx *This = impl_from_IWineDispatchProxyPrivate(iface);
    return dispex_to_string(This, string);
}

static BOOL WINAPI WineDispatchProxyPrivate_CanGC(IWineDispatchProxyPrivate *iface)
{
    DispatchEx *This = impl_from_IWineDispatchProxyPrivate(iface);
    IUnknown *outer = This->outer;

    /* Allow garbage collection only if the proxy is the only one holding a ref to us */
    IUnknown_AddRef(outer);
    return IUnknown_Release(outer) == 1;
}

static IWineDispatchProxyPrivateVtbl WineDispatchProxyPrivateVtbl = {
    {
    DispatchEx_QueryInterface,
    DispatchEx_AddRef,
    DispatchEx_Release,
    DispatchEx_GetTypeInfoCount,
    DispatchEx_GetTypeInfo,
    DispatchEx_GetIDsOfNames,
    DispatchEx_Invoke,
    DispatchEx_GetDispID,
    DispatchEx_InvokeEx,
    DispatchEx_DeleteMemberByName,
    DispatchEx_DeleteMemberByDispID,
    DispatchEx_GetMemberProperties,
    DispatchEx_GetMemberName,
    DispatchEx_GetNextDispID,
    DispatchEx_GetNameSpaceParent
    },

    /* IWineDispatchProxyPrivate extension */
    WineDispatchProxyPrivate_GetProxyFieldRef,
    WineDispatchProxyPrivate_GetDefaultPrototype,
    WineDispatchProxyPrivate_GetDefaultConstructor,
    WineDispatchProxyPrivate_DefineConstructors,
    WineDispatchProxyPrivate_IsPrototype,
    WineDispatchProxyPrivate_IsConstructor,
    WineDispatchProxyPrivate_PropFlags,
    WineDispatchProxyPrivate_PropGetID,
    WineDispatchProxyPrivate_PropInvoke,
    WineDispatchProxyPrivate_PropDelete,
    WineDispatchProxyPrivate_FuncInfo,
    WineDispatchProxyPrivate_AccessorInfo,
    WineDispatchProxyPrivate_ToString,
    WineDispatchProxyPrivate_CanGC
};

static inline BOOL is_compat_prototype(IDispatch *disp)
{
    if(!disp || disp->lpVtbl != (const IDispatchVtbl*)&WineDispatchProxyPrivateVtbl)
        return FALSE;
    return (impl_from_IDispatchEx((IDispatchEx*)disp)->outer->lpVtbl == &compat_prototype_vtbl);
}

BOOL dispex_query_interface(DispatchEx *This, REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IDispatch, riid))
        *ppv = &This->IDispatchEx_iface;
    else if(IsEqualGUID(&IID_IDispatchEx, riid))
        *ppv = &This->IDispatchEx_iface;
    else if(IsEqualGUID(&IID_IWineDispatchProxyPrivate, riid))
        *ppv = &This->IDispatchEx_iface;
    else if(IsEqualGUID(&IID_IDispatchJS, riid))
        *ppv = NULL;
    else if(IsEqualGUID(&IID_UndocumentedScriptIface, riid))
        *ppv = NULL;
    else if(IsEqualGUID(&IID_IMarshal, riid))
        *ppv = NULL;
    else if(IsEqualGUID(&IID_IManagedObject, riid))
        *ppv = NULL;
    else
        return FALSE;

    if(*ppv)
        IUnknown_AddRef((IUnknown*)*ppv);
    return TRUE;
}

HRESULT dispex_invoke(DispatchEx *dispex, IDispatch *this_obj, DISPID id, LCID lcid, WORD wFlags, DISPPARAMS *pdp,
        VARIANT *res, EXCEPINFO *pei, IServiceProvider *caller)
{
    HRESULT hres;

    if(!ensure_real_info(dispex))
        return E_OUTOFMEMORY;

    if(wFlags == (DISPATCH_PROPERTYPUT|DISPATCH_PROPERTYPUTREF))
        wFlags = DISPATCH_PROPERTYPUT;

    switch(get_dispid_type(id)) {
    case DISPEXPROP_CUSTOM:
        if(!dispex->info->desc->vtbl || !dispex->info->desc->vtbl->invoke)
            return DISP_E_UNKNOWNNAME;
        return dispex->info->desc->vtbl->invoke(dispex, this_obj, id, lcid, wFlags, pdp, res, pei, caller);

    case DISPEXPROP_DYNAMIC: {
        DWORD idx = id - DISPID_DYNPROP_0;
        dynamic_prop_t *prop;

        if(!get_dynamic_data(dispex) || dispex->dynamic_data->prop_cnt <= idx)
            return DISP_E_UNKNOWNNAME;

        prop = dispex->dynamic_data->props+idx;

        switch(wFlags) {
        case DISPATCH_METHOD|DISPATCH_PROPERTYGET:
            if(!res)
                return E_INVALIDARG;
            /* fall through */
        case DISPATCH_METHOD:
            fixup_prop_ref(dispex, prop);
            if(prop->flags & DYNPROP_DELETED)
                return DISP_E_UNKNOWNNAME;
            if(prop->flags & DYNPROP_PROTREF)
                prop = &dispex->prototype->dispex.dynamic_data->props[V_UI4(&prop->var)];

            if(V_VT(&prop->var) != VT_DISPATCH) {
                FIXME("invoke %s\n", debugstr_variant(&prop->var));
                return E_NOTIMPL;
            }

            return invoke_disp_value(this_obj, V_DISPATCH(&prop->var), lcid, wFlags, pdp, res, pei, caller);
        case DISPATCH_PROPERTYGET:
            fixup_prop_ref(dispex, prop);
            if(prop->flags & DYNPROP_DELETED)
                return DISP_E_UNKNOWNNAME;
            if(prop->flags & DYNPROP_PROTREF)
                prop = &dispex->prototype->dispex.dynamic_data->props[V_UI4(&prop->var)];
            V_VT(res) = VT_EMPTY;
            return variant_copy(res, &prop->var);
        case DISPATCH_PROPERTYPUT:
            if(pdp->cArgs != 1 || (pdp->cNamedArgs == 1 && *pdp->rgdispidNamedArgs != DISPID_PROPERTYPUT)
               || pdp->cNamedArgs > 1) {
                FIXME("invalid args\n");
                return E_INVALIDARG;
            }

            TRACE("put %s\n", debugstr_variant(pdp->rgvarg));
            VariantClear(&prop->var);
            hres = variant_copy(&prop->var, pdp->rgvarg);
            if(FAILED(hres))
                return hres;

            prop->flags &= ~(DYNPROP_DELETED | DYNPROP_PROTREF);
            return S_OK;
        default:
            FIXME("unhandled wFlags %x\n", wFlags);
            return E_NOTIMPL;
        }
    }
    case DISPEXPROP_BUILTIN:
        if(wFlags == DISPATCH_CONSTRUCT) {
            if(id == DISPID_VALUE) {
                if(dispex->info->desc->vtbl && dispex->info->desc->vtbl->value) {
                    return dispex->info->desc->vtbl->value(dispex, lcid, wFlags, pdp, res, pei, caller);
                }
                FIXME("DISPATCH_CONSTRUCT flag but missing value function\n");
                return E_FAIL;
            }
            FIXME("DISPATCH_CONSTRUCT flag without DISPID_VALUE\n");
            return E_FAIL;
        }

        return invoke_builtin_prop(dispex, this_obj, id, lcid, wFlags, pdp, res, pei, caller);
    default:
        assert(0);
        return E_FAIL;
    }
}

HRESULT dispex_delete_prop(DispatchEx *dispex, DISPID id)
{
    HRESULT hres;

    switch(get_dispid_type(id)) {
    case DISPEXPROP_CUSTOM:
        if(dispex->info->desc->vtbl && dispex->info->desc->vtbl->delete)
            return dispex->info->desc->vtbl->delete(dispex, id);
        break;
    case DISPEXPROP_DYNAMIC: {
        DWORD idx = id - DISPID_DYNPROP_0;
        dynamic_prop_t *prop;

        if(!get_dynamic_data(dispex) || idx >= dispex->dynamic_data->prop_cnt)
            return S_OK;

        prop = dispex->dynamic_data->props + idx;
        if(!(prop->flags & DYNPROP_PROTREF)) {
            VariantClear(&prop->var);
            prop->flags |= DYNPROP_DELETED;
        }
        return S_OK;
    }
    case DISPEXPROP_BUILTIN: {
        func_info_t *func;

        if(!ensure_real_info(dispex))
            return E_OUTOFMEMORY;

        hres = get_builtin_func_prot(dispex, id, &func);
        if(FAILED(hres))
            return hres;

        if(func->func_disp_idx != -1)
            reset_builtin_func(dispex, func);
        return S_OK;
    }
    default:
        break;
    }

    return S_OK;
}

static void WINAPI dispex_traverse_cb(IDispatch *obj, void *cb)
{
    note_cc_edge((nsISupports*)obj, "dispex_data", cb);
}

void dispex_traverse(DispatchEx *This, nsCycleCollectionTraversalCallback *cb)
{
    dynamic_prop_t *prop;

    if(This->proxy)
        This->proxy->lpVtbl->Traverse(This->proxy, dispex_traverse_cb, cb);

    if(!This->dynamic_data)
        return;

    for(prop = This->dynamic_data->props; prop < This->dynamic_data->props + This->dynamic_data->prop_cnt; prop++) {
        if(V_VT(&prop->var) == VT_DISPATCH)
            note_cc_edge((nsISupports*)V_DISPATCH(&prop->var), "dispex_data", cb);
    }

    /* FIXME: Traverse func_disps */
}

void dispex_unlink(DispatchEx *This)
{
    dynamic_prop_t *prop;

    if(!This->dynamic_data)
        return;

    for(prop = This->dynamic_data->props; prop < This->dynamic_data->props + This->dynamic_data->prop_cnt; prop++) {
        if(V_VT(&prop->var) == VT_DISPATCH) {
            V_VT(&prop->var) = VT_EMPTY;
            IDispatch_Release(V_DISPATCH(&prop->var));
        }else {
            VariantClear(&prop->var);
        }
    }
}

const void *dispex_get_vtbl(DispatchEx *dispex)
{
    return dispex->info->desc->vtbl;
}

void release_dispex(DispatchEx *This)
{
    dynamic_prop_t *prop;

    if(This->proxy)
        This->proxy->lpVtbl->Unlinked(This->proxy);

    if(This->prototype)
        IUnknown_Release(&This->prototype->IUnknown_iface);

    if(!This->dynamic_data)
        return;

    for(prop = This->dynamic_data->props; prop < This->dynamic_data->props + This->dynamic_data->prop_cnt; prop++) {
        VariantClear(&prop->var);
        heap_free(prop->name);
    }

    heap_free(This->dynamic_data->props);

    if(This->dynamic_data->func_disps) {
        func_obj_entry_t *iter;

        for(iter = This->dynamic_data->func_disps; iter < This->dynamic_data->func_disps + This->info->func_disp_cnt; iter++) {
            if(iter->func_obj) {
                iter->func_obj->obj = NULL;
                IDispatchEx_Release(&iter->func_obj->dispex.IDispatchEx_iface);
            }
            VariantClear(&iter->val);
        }

        heap_free(This->dynamic_data->func_disps);
    }

    heap_free(This->dynamic_data);
}

void update_dispex(DispatchEx *dispex, dispex_static_data_t *data, HTMLDocumentNode *doc, compat_mode_t compat_mode)
{
    if(data->vtbl && data->vtbl->get_compat_mode) {
        /* delayed init */
        if(!data->delayed_init_info) {
            EnterCriticalSection(&cs_dispex_static_data);
            if(!data->delayed_init_info) {
                dispex_data_t *info = heap_alloc_zero(sizeof(*data->delayed_init_info));
                if(info) {
                    info->desc = data;
                    data->delayed_init_info = info;
                }
            }
            LeaveCriticalSection(&cs_dispex_static_data);
        }
        dispex->info = data->delayed_init_info;
    }else {
        dispex->info = ensure_dispex_info(data, compat_mode);
    }

    if(doc && doc->window && data->prototype_id < ARRAY_SIZE(doc->window->compat_prototypes))
        dispex->prototype = get_compat_prototype(doc->window, data->prototype_id, compat_mode);
}

void init_dispatch(DispatchEx *dispex, IUnknown *outer, dispex_static_data_t *data, HTMLDocumentNode *doc,
        compat_mode_t compat_mode)
{
    assert(compat_mode < COMPAT_MODE_CNT);

    dispex->IDispatchEx_iface.lpVtbl = (const IDispatchExVtbl*)&WineDispatchProxyPrivateVtbl;
    dispex->outer = outer;
    dispex->proxy = NULL;
    dispex->prototype = NULL;
    dispex->dynamic_data = NULL;

    update_dispex(dispex, data, doc, compat_mode);
}
