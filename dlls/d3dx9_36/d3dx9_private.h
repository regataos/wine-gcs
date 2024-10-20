/*
 * Copyright (C) 2002 Raphael Junqueira
 * Copyright (C) 2008 David Adam
 * Copyright (C) 2008 Tony Wasserka
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
 *
 */

#ifndef __WINE_D3DX9_PRIVATE_H
#define __WINE_D3DX9_PRIVATE_H

#include <stdint.h>
#include "wine/debug.h"
#include "wine/rbtree.h"
#include "d3dx_helpers.h"

#define COBJMACROS
#include "d3dx9.h"

#define ULONG64_MAX (~(ULONG64)0)

#define FOURCC_TX_1 0x54580100

#define D3DX9_FILTER_INVALID_BITS 0xff80fff8
static inline HRESULT d3dx9_validate_filter(uint32_t filter)
{
    if ((filter & D3DX9_FILTER_INVALID_BITS) || !(filter & 0x7) || ((filter & 0x7) > D3DX_FILTER_BOX))
        return D3DERR_INVALIDCALL;

    return D3D_OK;
}

static inline HRESULT d3dx9_handle_load_filter(DWORD *filter)
{
    if (*filter == D3DX_DEFAULT)
        *filter = D3DX_FILTER_TRIANGLE | D3DX_FILTER_DITHER;

    return d3dx9_validate_filter(*filter);
}

/* dds_header.flags */
#define DDS_CAPS 0x1
#define DDS_HEIGHT 0x2
#define DDS_WIDTH 0x4
#define DDS_PITCH 0x8
#define DDS_PIXELFORMAT 0x1000
#define DDS_MIPMAPCOUNT 0x20000
#define DDS_LINEARSIZE 0x80000
#define DDS_DEPTH 0x800000

/* dds_header.caps */
#define DDS_CAPS_COMPLEX 0x8
#define DDS_CAPS_TEXTURE 0x1000
#define DDS_CAPS_MIPMAP 0x400000

/* dds_header.caps2 */
#define DDS_CAPS2_CUBEMAP 0x200
#define DDS_CAPS2_CUBEMAP_POSITIVEX 0x400
#define DDS_CAPS2_CUBEMAP_NEGATIVEX 0x800
#define DDS_CAPS2_CUBEMAP_POSITIVEY 0x1000
#define DDS_CAPS2_CUBEMAP_NEGATIVEY 0x2000
#define DDS_CAPS2_CUBEMAP_POSITIVEZ 0x4000
#define DDS_CAPS2_CUBEMAP_NEGATIVEZ 0x8000
#define DDS_CAPS2_CUBEMAP_ALL_FACES ( DDS_CAPS2_CUBEMAP_POSITIVEX | DDS_CAPS2_CUBEMAP_NEGATIVEX \
                                    | DDS_CAPS2_CUBEMAP_POSITIVEY | DDS_CAPS2_CUBEMAP_NEGATIVEY \
                                    | DDS_CAPS2_CUBEMAP_POSITIVEZ | DDS_CAPS2_CUBEMAP_NEGATIVEZ )
#define DDS_CAPS2_VOLUME 0x200000

/* dds_pixel_format.flags */
#define DDS_PF_ALPHA 0x1
#define DDS_PF_ALPHA_ONLY 0x2
#define DDS_PF_FOURCC 0x4
#define DDS_PF_INDEXED 0x20
#define DDS_PF_RGB 0x40
#define DDS_PF_YUV 0x200
#define DDS_PF_LUMINANCE 0x20000
#define DDS_PF_BUMPLUMINANCE 0x40000
#define DDS_PF_BUMPDUDV 0x80000

struct dds_pixel_format
{
    DWORD size;
    DWORD flags;
    DWORD fourcc;
    DWORD bpp;
    DWORD rmask;
    DWORD gmask;
    DWORD bmask;
    DWORD amask;
};

struct dds_header
{
    DWORD signature;
    DWORD size;
    DWORD flags;
    DWORD height;
    DWORD width;
    DWORD pitch_or_linear_size;
    DWORD depth;
    DWORD miplevels;
    DWORD reserved[11];
    struct dds_pixel_format pixel_format;
    DWORD caps;
    DWORD caps2;
    DWORD caps3;
    DWORD caps4;
    DWORD reserved2;
};

struct vec4
{
    float x, y, z, w;
};

void d3dximage_info_from_d3dx_image(D3DXIMAGE_INFO *info, struct d3dx_image *image);

struct d3dx_include_from_file
{
    ID3DXInclude ID3DXInclude_iface;
};

extern CRITICAL_SECTION from_file_mutex;
extern const struct ID3DXIncludeVtbl d3dx_include_from_file_vtbl;

static inline BOOL is_unknown_format(const struct pixel_format_desc *format)
{
    return (format->format == D3DX_PIXEL_FORMAT_COUNT);
}

static inline BOOL is_index_format(const struct pixel_format_desc *format)
{
    return (format->fmt_type_desc.a_type == CTYPE_INDEX || format->fmt_type_desc.rgb_type == CTYPE_INDEX);
}

static inline BOOL is_compressed_format(const struct pixel_format_desc *format)
{
    return !!(format->fmt_type_desc.fmt_flags & FMT_FLAG_DXT);
}

static inline BOOL is_packed_format(const struct pixel_format_desc *format)
{
    return !!(format->fmt_type_desc.fmt_flags & FMT_FLAG_PACKED);
}

static inline BOOL format_types_match(const struct pixel_format_desc *src, const struct pixel_format_desc *dst)
{
    const struct pixel_format_type_desc *src_type = &src->fmt_type_desc;
    const struct pixel_format_type_desc *dst_type = &dst->fmt_type_desc;

    if ((src_type->a_type != dst_type->a_type) && (src_type->a_type != CTYPE_EMPTY) &&
            (dst_type->a_type != CTYPE_EMPTY))
        return FALSE;

    if ((src_type->rgb_type != dst_type->rgb_type) && (src_type->rgb_type != CTYPE_EMPTY) &&
            (dst_type->rgb_type != CTYPE_EMPTY))
        return FALSE;

    if (src_type->fmt_flags != dst_type->fmt_flags)
        return FALSE;

    return (src_type->rgb_type == dst_type->rgb_type || src_type->a_type == dst_type->a_type);
}

static inline BOOL filter_flags_match(uint32_t filter_flags)
{
    if (!!(filter_flags & D3DX_FILTER_PMA_IN) != !!((filter_flags & D3DX_FILTER_PMA_OUT)))
        return FALSE;
    if (!!(filter_flags & D3DX_FILTER_SRGB_IN) != !!((filter_flags & D3DX_FILTER_SRGB_OUT)))
        return FALSE;

    return TRUE;
}

static inline BOOL is_conversion_from_supported(const struct pixel_format_desc *format)
{
    return !is_packed_format(format) && !is_unknown_format(format);
}

static inline BOOL is_conversion_to_supported(const struct pixel_format_desc *format)
{
    return !is_index_format(format) && !is_packed_format(format) && !is_unknown_format(format);
}

HRESULT map_view_of_file(const WCHAR *filename, void **buffer, DWORD *length);
HRESULT load_resource_into_memory(HMODULE module, HRSRC resinfo, void **buffer, DWORD *length);

HRESULT write_buffer_to_file(const WCHAR *filename, ID3DXBuffer *buffer);

enum d3dx_pixel_format_id wic_guid_to_d3dx_pixel_format_id(const GUID *guid);
const GUID *d3dx_pixel_format_id_to_wic_guid(enum d3dx_pixel_format_id format);
HRESULT d3dx_pixel_format_to_dds_pixel_format(struct dds_pixel_format *pixel_format,
        enum d3dx_pixel_format_id format);

D3DFORMAT d3dformat_from_d3dx_pixel_format_id(enum d3dx_pixel_format_id format);
enum d3dx_pixel_format_id d3dx_pixel_format_id_from_d3dformat(D3DFORMAT format);
const struct pixel_format_desc *get_format_info(D3DFORMAT format);
const struct pixel_format_desc *get_format_info_idx(int idx);

void format_to_vec4(const struct pixel_format_desc *format, const BYTE *src, const PALETTEENTRY *palette,
        struct vec4 *dst);
void format_from_vec4(const struct pixel_format_desc *format, const struct vec4 *src,
        const struct pixel_format_type_desc *src_type, BYTE *dst);

HRESULT lock_surface(IDirect3DSurface9 *surface, const RECT *surface_rect, D3DLOCKED_RECT *lock,
        IDirect3DSurface9 **temp_surface, BOOL write);
HRESULT unlock_surface(IDirect3DSurface9 *surface, const RECT *surface_rect,
        IDirect3DSurface9 *temp_surface, BOOL update);
void copy_pixels(const BYTE *src, UINT src_row_pitch, UINT src_slice_pitch,
        BYTE *dst, UINT dst_row_pitch, UINT dst_slice_pitch, const struct volume *size,
        const struct pixel_format_desc *format);
void convert_argb_pixels(const BYTE *src, UINT src_row_pitch, UINT src_slice_pitch, const struct volume *src_size,
        const struct pixel_format_desc *src_format, BYTE *dst, UINT dst_row_pitch, UINT dst_slice_pitch,
        const struct volume *dst_size, const struct pixel_format_desc *dst_format, D3DCOLOR color_key,
        const PALETTEENTRY *palette, uint32_t filter_flags);
void get_aligned_rect(uint32_t left, uint32_t top, uint32_t right, uint32_t bottom, uint32_t width, uint32_t height,
        const struct pixel_format_desc *fmt_desc, RECT *aligned_rect);

unsigned short float_32_to_16(const float in);
float float_16_to_32(const unsigned short in);

/* debug helpers */
const char *debug_d3dxparameter_class(D3DXPARAMETER_CLASS c);
const char *debug_d3dxparameter_type(D3DXPARAMETER_TYPE t);
const char *debug_d3dxparameter_registerset(D3DXREGISTER_SET r);

/* parameter type conversion helpers */
static inline BOOL get_bool(D3DXPARAMETER_TYPE type, const void *data)
{
    switch (type)
    {
        case D3DXPT_FLOAT:
        case D3DXPT_INT:
        case D3DXPT_BOOL:
            return !!*(DWORD *)data;

        case D3DXPT_VOID:
            return *(BOOL *)data;

        default:
            return FALSE;
    }
}

static inline int get_int(D3DXPARAMETER_TYPE type, const void *data)
{
    switch (type)
    {
        case D3DXPT_FLOAT:
            return (int)(*(float *)data);

        case D3DXPT_INT:
        case D3DXPT_VOID:
            return *(int *)data;

        case D3DXPT_BOOL:
            return get_bool(type, data);

        default:
            return 0;
    }
}

static inline float get_float(D3DXPARAMETER_TYPE type, const void *data)
{
    switch (type)
    {
        case D3DXPT_FLOAT:
        case D3DXPT_VOID:
            return *(float *)data;

        case D3DXPT_INT:
            return (float)(*(int *)data);

        case D3DXPT_BOOL:
            return (float)get_bool(type, data);

        default:
            return 0.0f;
    }
}

static inline void set_number(void *outdata, D3DXPARAMETER_TYPE outtype, const void *indata, D3DXPARAMETER_TYPE intype)
{
    if (outtype == intype)
    {
        *(DWORD *)outdata = *(DWORD *)indata;
        return;
    }

    switch (outtype)
    {
        case D3DXPT_FLOAT:
            *(float *)outdata = get_float(intype, indata);
            break;

        case D3DXPT_BOOL:
            *(BOOL *)outdata = get_bool(intype, indata);
            break;

        case D3DXPT_INT:
            *(int *)outdata = get_int(intype, indata);
            break;

        default:
            *(DWORD *)outdata = 0;
            break;
    }
}

static inline BOOL is_param_type_sampler(D3DXPARAMETER_TYPE type)
{
    return type == D3DXPT_SAMPLER
            || type == D3DXPT_SAMPLER1D || type == D3DXPT_SAMPLER2D
            || type == D3DXPT_SAMPLER3D || type == D3DXPT_SAMPLERCUBE;
}

/* Returns the smallest power of 2 which is greater than or equal to num */
static inline uint32_t make_pow2(uint32_t num)
{
    DWORD index;

    return BitScanReverse(&index, num - 1) ? 1u << (index + 1) : 1;
}

struct d3dx_parameter;

enum pres_reg_tables
{
    PRES_REGTAB_IMMED,
    PRES_REGTAB_CONST,
    PRES_REGTAB_INPUT,
    PRES_REGTAB_OCONST,
    PRES_REGTAB_OBCONST,
    PRES_REGTAB_OICONST,
    PRES_REGTAB_TEMP,
    PRES_REGTAB_COUNT,
    PRES_REGTAB_FIRST_SHADER = PRES_REGTAB_CONST,
};

struct d3dx_const_param_eval_output
{
    struct d3dx_parameter *param;
    enum pres_reg_tables table;
    enum D3DXPARAMETER_CLASS constant_class;
    unsigned int register_index;
    unsigned int register_count;
    BOOL direct_copy;
    unsigned int element_count;
};

struct d3dx_const_tab
{
    unsigned int input_count;
    D3DXCONSTANT_DESC *inputs;
    struct d3dx_parameter **inputs_param;
    unsigned int const_set_count;
    unsigned int const_set_size;
    struct d3dx_const_param_eval_output *const_set;
    const enum pres_reg_tables *regset2table;
    ULONG64 update_version;
};

struct d3dx_regstore
{
    void *tables[PRES_REGTAB_COUNT];
    unsigned int table_sizes[PRES_REGTAB_COUNT]; /* registers count */
};

struct d3dx_pres_ins;

struct d3dx_preshader
{
    struct d3dx_regstore regs;

    unsigned int ins_count;
    struct d3dx_pres_ins *ins;

    struct d3dx_const_tab inputs;
};

struct d3dx_param_eval
{
    D3DXPARAMETER_TYPE param_type;

    struct d3dx_preshader pres;
    struct d3dx_const_tab shader_inputs;

    ULONG64 *version_counter;
};

struct param_rb_entry
{
    struct wine_rb_entry entry;
    char *full_name;
    struct d3dx_parameter *param;
};

struct d3dx_shared_data;
struct d3dx_top_level_parameter;

struct d3dx_parameter
{
    char magic_string[4];
    struct d3dx_top_level_parameter *top_level_param;
    struct d3dx_param_eval *param_eval;
    char *name;
    void *data;
    D3DXPARAMETER_CLASS class;
    D3DXPARAMETER_TYPE  type;
    unsigned int rows;
    unsigned int columns;
    unsigned int element_count;
    unsigned int member_count;
    uint32_t flags;
    unsigned int bytes;
    unsigned int object_id;

    struct d3dx_parameter *members;
    char *semantic;

    char *full_name;
    struct wine_rb_entry rb_entry;
};

struct d3dx_top_level_parameter
{
    struct d3dx_parameter param;
    unsigned int annotation_count;
    struct d3dx_parameter *annotations;
    ULONG64 update_version;
    ULONG64 *version_counter;
    struct d3dx_shared_data *shared_data;
};

struct d3dx_shared_data
{
    void *data;
    struct d3dx_top_level_parameter **parameters;
    unsigned int size, count;
    ULONG64 update_version;
};

struct d3dx_effect;

static inline BOOL is_top_level_parameter(struct d3dx_parameter *param)
{
    return &param->top_level_param->param == param;
}

static inline struct d3dx_top_level_parameter
        *top_level_parameter_from_parameter(struct d3dx_parameter *param)
{
    return CONTAINING_RECORD(param, struct d3dx_top_level_parameter, param);
}

static inline ULONG64 next_update_version(ULONG64 *version_counter)
{
    return ++*version_counter;
}

static inline BOOL is_top_level_param_dirty(struct d3dx_top_level_parameter *param, ULONG64 update_version)
{
    struct d3dx_shared_data *shared_data;

    if ((shared_data = param->shared_data))
        return update_version < shared_data->update_version;
    else
        return update_version < param->update_version;
}

static inline BOOL is_param_dirty(struct d3dx_parameter *param, ULONG64 update_version)
{
    return is_top_level_param_dirty(param->top_level_param, update_version);
}

struct d3dx_parameters_store
{
    struct wine_rb_tree tree;
    struct d3dx_top_level_parameter *parameters;
    unsigned int count;

    char *full_name_tmp;
    unsigned int full_name_tmp_size;
};

HRESULT d3dx_init_parameters_store(struct d3dx_parameters_store *store, unsigned int count);
void d3dx_parameters_store_cleanup(struct d3dx_parameters_store *store);
struct d3dx_parameter *get_parameter_by_name(struct d3dx_parameters_store *store,
        struct d3dx_parameter *parameter, const char *name);

#define SET_D3D_STATE_(manager, device, method, ...) (manager ? manager->lpVtbl->method(manager, __VA_ARGS__) \
        : device->lpVtbl->method(device, __VA_ARGS__))
#define SET_D3D_STATE(base_effect, ...) SET_D3D_STATE_(base_effect->manager, base_effect->device, __VA_ARGS__)

HRESULT d3dx_create_param_eval(struct d3dx_parameters_store *parameters, void *byte_code,
        unsigned int byte_code_size, D3DXPARAMETER_TYPE type,
        struct d3dx_param_eval **peval, ULONG64 *version_counter,
        const char **skip_constants, unsigned int skip_constants_count);
void d3dx_free_param_eval(struct d3dx_param_eval *peval);
HRESULT d3dx_evaluate_parameter(struct d3dx_param_eval *peval,
        const struct d3dx_parameter *param, void *param_value);
HRESULT d3dx_param_eval_set_shader_constants(ID3DXEffectStateManager *manager, struct IDirect3DDevice9 *device,
        struct d3dx_param_eval *peval, BOOL update_all);
BOOL is_param_eval_input_dirty(struct d3dx_param_eval *peval, ULONG64 update_version);

struct ctab_constant {
    D3DXCONSTANT_DESC desc;
    WORD constantinfo_reserved;
    struct ctab_constant *constants;
};

const struct ctab_constant *d3dx_shader_get_ctab_constant(ID3DXConstantTable *iface,
        D3DXHANDLE constant);

#endif /* __WINE_D3DX9_PRIVATE_H */
