/*
 * Copyright 2016 Nikolay Sivov for CodeWeavers
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

#define COBJMACROS
#include "initguid.h"
#include "d3d11.h"
#include "d3dx11.h"
#include "wine/wined3d.h"
#include "wine/test.h"
#include <stdint.h>

#define D3DERR_INVALIDCALL 0x8876086c
#define DDS_RESOURCE_MISC_TEXTURECUBE 0x04

static bool wined3d_opengl;

#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)  \
    ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |  \
    ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))
#endif

static DXGI_FORMAT block_compressed_formats[] =
{
    DXGI_FORMAT_BC1_TYPELESS,  DXGI_FORMAT_BC1_UNORM, DXGI_FORMAT_BC1_UNORM_SRGB,
    DXGI_FORMAT_BC2_TYPELESS,  DXGI_FORMAT_BC2_UNORM, DXGI_FORMAT_BC2_UNORM_SRGB,
    DXGI_FORMAT_BC3_TYPELESS,  DXGI_FORMAT_BC3_UNORM, DXGI_FORMAT_BC3_UNORM_SRGB,
    DXGI_FORMAT_BC4_TYPELESS,  DXGI_FORMAT_BC4_UNORM, DXGI_FORMAT_BC4_SNORM,
    DXGI_FORMAT_BC5_TYPELESS,  DXGI_FORMAT_BC5_UNORM, DXGI_FORMAT_BC5_SNORM,
    DXGI_FORMAT_BC6H_TYPELESS, DXGI_FORMAT_BC6H_UF16, DXGI_FORMAT_BC6H_SF16,
    DXGI_FORMAT_BC7_TYPELESS,  DXGI_FORMAT_BC7_UNORM, DXGI_FORMAT_BC7_UNORM_SRGB
};

static BOOL is_block_compressed(DXGI_FORMAT format)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(block_compressed_formats); ++i)
        if (format == block_compressed_formats[i])
            return TRUE;

    return FALSE;
}

static BOOL dxgi_format_is_8bpp_rgba(DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
    case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
    case DXGI_FORMAT_R8G8B8A8_SINT:
        return TRUE;
    default:
        return FALSE;
    }
}

static uint32_t get_bpp_from_format(DXGI_FORMAT format)
{
    switch (format)
    {
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
            return 128;
        case DXGI_FORMAT_R32G32B32_TYPELESS:
        case DXGI_FORMAT_R32G32B32_FLOAT:
        case DXGI_FORMAT_R32G32B32_UINT:
        case DXGI_FORMAT_R32G32B32_SINT:
            return 96;
        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        case DXGI_FORMAT_R32G32_TYPELESS:
        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        case DXGI_FORMAT_Y416:
        case DXGI_FORMAT_Y210:
        case DXGI_FORMAT_Y216:
            return 64;
        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R10G10B10A2_UINT:
        case DXGI_FORMAT_R11G11B10_FLOAT:
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_R16G16_TYPELESS:
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R16G16_UNORM:
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16_SNORM:
        case DXGI_FORMAT_R16G16_SINT:
        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
        case DXGI_FORMAT_R8G8_B8G8_UNORM:
        case DXGI_FORMAT_G8R8_G8B8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        case DXGI_FORMAT_AYUV:
        case DXGI_FORMAT_Y410:
        case DXGI_FORMAT_YUY2:
            return 32;
        case DXGI_FORMAT_P010:
        case DXGI_FORMAT_P016:
            return 24;
        case DXGI_FORMAT_R8G8_TYPELESS:
        case DXGI_FORMAT_R8G8_UNORM:
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8_SNORM:
        case DXGI_FORMAT_R8G8_SINT:
        case DXGI_FORMAT_R16_TYPELESS:
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16_SNORM:
        case DXGI_FORMAT_R16_SINT:
        case DXGI_FORMAT_B5G6R5_UNORM:
        case DXGI_FORMAT_B5G5R5A1_UNORM:
        case DXGI_FORMAT_A8P8:
        case DXGI_FORMAT_B4G4R4A4_UNORM:
            return 16;
        case DXGI_FORMAT_NV12:
        case DXGI_FORMAT_420_OPAQUE:
        case DXGI_FORMAT_NV11:
            return 12;
        case DXGI_FORMAT_R8_TYPELESS:
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_R8_SINT:
        case DXGI_FORMAT_A8_UNORM:
        case DXGI_FORMAT_AI44:
        case DXGI_FORMAT_IA44:
        case DXGI_FORMAT_P8:
        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_BC5_TYPELESS:
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM:
        case DXGI_FORMAT_BC6H_TYPELESS:
        case DXGI_FORMAT_BC6H_UF16:
        case DXGI_FORMAT_BC6H_SF16:
        case DXGI_FORMAT_BC7_TYPELESS:
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            return 8;
        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC4_TYPELESS:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM:
            return 4;
        case DXGI_FORMAT_R1_UNORM:
            return 1;
        default:
            return 0;
    }
}

static HRESULT WINAPI D3DX11ThreadPump_QueryInterface(ID3DX11ThreadPump *iface, REFIID riid, void **out)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static ULONG WINAPI D3DX11ThreadPump_AddRef(ID3DX11ThreadPump *iface)
{
    return 2;
}

static ULONG WINAPI D3DX11ThreadPump_Release(ID3DX11ThreadPump *iface)
{
    return 1;
}

static int add_work_item_count = 1;

static HRESULT WINAPI D3DX11ThreadPump_AddWorkItem(ID3DX11ThreadPump *iface, ID3DX11DataLoader *loader,
        ID3DX11DataProcessor *processor, HRESULT *result, void **object)
{
    SIZE_T size;
    void *data;
    HRESULT hr;

    ok(!add_work_item_count++, "unexpected call\n");

    hr = ID3DX11DataLoader_Load(loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DX11DataLoader_Decompress(loader, &data, &size);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DX11DataProcessor_Process(processor, data, size);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DX11DataProcessor_CreateDeviceObject(processor, object);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DX11DataProcessor_Destroy(processor);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DX11DataLoader_Destroy(loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    if (result) *result = S_OK;
    return S_OK;
}

static UINT WINAPI D3DX11ThreadPump_GetWorkItemCount(ID3DX11ThreadPump *iface)
{
    ok(0, "unexpected call\n");
    return 0;
}

static HRESULT WINAPI D3DX11ThreadPump_WaitForAllItems(ID3DX11ThreadPump *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI D3DX11ThreadPump_ProcessDeviceWorkItems(ID3DX11ThreadPump *iface, UINT count)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI D3DX11ThreadPump_PurgeAllItems(ID3DX11ThreadPump *iface)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static HRESULT WINAPI D3DX11ThreadPump_GetQueueStatus(ID3DX11ThreadPump *iface, UINT *queue,
        UINT *processqueue, UINT *devicequeue)
{
    ok(0, "unexpected call\n");
    return E_NOTIMPL;
}

static ID3DX11ThreadPumpVtbl D3DX11ThreadPumpVtbl =
{
    D3DX11ThreadPump_QueryInterface,
    D3DX11ThreadPump_AddRef,
    D3DX11ThreadPump_Release,
    D3DX11ThreadPump_AddWorkItem,
    D3DX11ThreadPump_GetWorkItemCount,
    D3DX11ThreadPump_WaitForAllItems,
    D3DX11ThreadPump_ProcessDeviceWorkItems,
    D3DX11ThreadPump_PurgeAllItems,
    D3DX11ThreadPump_GetQueueStatus
};
static ID3DX11ThreadPump thread_pump = { &D3DX11ThreadPumpVtbl };

static BOOL compare_uint(unsigned int x, unsigned int y, unsigned int max_diff)
{
    unsigned int diff = x > y ? x - y : y - x;

    return diff <= max_diff;
}

/* 1x1 bmp (1 bpp) */
static const unsigned char bmp_1bpp[] =
{
    0x42,0x4d,0x42,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3e,0x00,0x00,0x00,0x28,0x00,
    0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x00,
    0x00,0x00,0x04,0x00,0x00,0x00,0x12,0x0b,0x00,0x00,0x12,0x0b,0x00,0x00,0x02,0x00,
    0x00,0x00,0x02,0x00,0x00,0x00,0xf1,0xf2,0xf3,0x80,0xf4,0xf5,0xf6,0x81,0x00,0x00,
    0x00,0x00
};

/* 1x1 bmp (2 bpp) */
static const unsigned char bmp_2bpp[] =
{
    0x42,0x4d,0x42,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3e,0x00,0x00,0x00,0x28,0x00,
    0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x02,0x00,0x00,0x00,
    0x00,0x00,0x04,0x00,0x00,0x00,0x12,0x0b,0x00,0x00,0x12,0x0b,0x00,0x00,0x02,0x00,
    0x00,0x00,0x02,0x00,0x00,0x00,0xf1,0xf2,0xf3,0x80,0xf4,0xf5,0xf6,0x81,0x00,0x00,
    0x00,0x00
};

/* 1x1 bmp (4 bpp) */
static const unsigned char bmp_4bpp[] =
{
    0x42,0x4d,0x42,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3e,0x00,0x00,0x00,0x28,0x00,
    0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x04,0x00,0x00,0x00,
    0x00,0x00,0x04,0x00,0x00,0x00,0x12,0x0b,0x00,0x00,0x12,0x0b,0x00,0x00,0x02,0x00,
    0x00,0x00,0x02,0x00,0x00,0x00,0xf1,0xf2,0xf3,0x80,0xf4,0xf5,0xf6,0x81,0x00,0x00,
    0x00,0x00
};

/* 1x1 bmp (8 bpp) */
static const unsigned char bmp_8bpp[] =
{
    0x42,0x4d,0x42,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3e,0x00,0x00,0x00,0x28,0x00,
    0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x01,0x00,0x08,0x00,0x00,0x00,
    0x00,0x00,0x04,0x00,0x00,0x00,0x12,0x0b,0x00,0x00,0x12,0x0b,0x00,0x00,0x02,0x00,
    0x00,0x00,0x02,0x00,0x00,0x00,0xf1,0xf2,0xf3,0x80,0xf4,0xf5,0xf6,0x81,0x00,0x00,
    0x00,0x00
};

/* 2x2 bmp (32 bpp XRGB) */
static const unsigned char bmp_32bpp_xrgb[] =
{
    0x42,0x4d,0x46,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x36,0x00,0x00,0x00,0x28,0x00,
    0x00,0x00,0x02,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x01,0x00,0x20,0x00,0x00,0x00,
    0x00,0x00,0x10,0x00,0x00,0x00,0x12,0x0b,0x00,0x00,0x12,0x0b,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0xa0,0xb0,0xc0,0x00,0xa1,0xb1,0xc1,0x00,0xa2,0xb2,
    0xc2,0x00,0xa3,0xb3,0xc3,0x00
};

/* 2x2 bmp (32 bpp ARGB) */
static const unsigned char bmp_32bpp_argb[] =
{
    0x42,0x4d,0x46,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x36,0x00,0x00,0x00,0x28,0x00,
    0x00,0x00,0x02,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x01,0x00,0x20,0x00,0x00,0x00,
    0x00,0x00,0x10,0x00,0x00,0x00,0x12,0x0b,0x00,0x00,0x12,0x0b,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0xa0,0xb0,0xc0,0x00,0xa1,0xb1,0xc1,0x00,0xa2,0xb2,
    0xc2,0x00,0xa3,0xb3,0xc3,0x01
};

static const unsigned char png_grayscale[] =
{
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49,
    0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x00,
    0x00, 0x00, 0x00, 0x3a, 0x7e, 0x9b, 0x55, 0x00, 0x00, 0x00, 0x0a, 0x49, 0x44,
    0x41, 0x54, 0x08, 0xd7, 0x63, 0xf8, 0x0f, 0x00, 0x01, 0x01, 0x01, 0x00, 0x1b,
    0xb6, 0xee, 0x56, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42,
    0x60, 0x82
};

/* 2x2 24-bit dds, 2 mipmaps */
static const unsigned char dds_24bit[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x0a,0x00,0x02,0x00,0x00,0x00,
    0x02,0x00,0x00,0x00,0x0c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x00,0x00,0xff,0x00,
    0x00,0xff,0x00,0x00,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x10,0x40,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff
};

/* 2x2 16-bit dds, no mipmaps */
static const unsigned char dds_16bit[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x08,0x00,0x02,0x00,0x00,0x00,
    0x02,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x7c,0x00,0x00,
    0xe0,0x03,0x00,0x00,0x1f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xff,0x7f,0xff,0x7f,0xff,0x7f,0xff,0x7f
};

/* 16x4 8-bit dds  */
static const unsigned char dds_8bit[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x0f,0x10,0x00,0x00,0x04,0x00,0x00,0x00,
    0x10,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,
    0x47,0x49,0x4d,0x50,0x2d,0x44,0x44,0x53,0x5a,0x09,0x03,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,
    0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0xec,0x27,0x00,0xff,0x8c,0xcd,0x12,0xff,
    0x78,0x01,0x14,0xff,0x50,0xcd,0x12,0xff,0x00,0x3d,0x8c,0xff,0x02,0x00,0x00,0xff,
    0x47,0x00,0x00,0xff,0xda,0x07,0x02,0xff,0x50,0xce,0x12,0xff,0xea,0x11,0x01,0xff,
    0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x08,0x3d,0x8c,0xff,0x08,0x01,0x00,0xff,
    0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x60,0xcc,0x12,0xff,
    0xa1,0xb2,0xd4,0xff,0xda,0x07,0x02,0xff,0x47,0x00,0x00,0xff,0x00,0x00,0x00,0xff,
    0x50,0xce,0x12,0xff,0x00,0x00,0x14,0xff,0xa8,0xcc,0x12,0xff,0x3c,0xb2,0xd4,0xff,
    0xda,0x07,0x02,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x01,0xff,
    0x21,0x00,0x00,0xff,0xd8,0xcb,0x12,0xff,0x54,0xcd,0x12,0xff,0x8b,0x4f,0xd5,0xff,
    0x00,0x04,0xda,0xff,0x00,0x00,0x00,0xff,0x3d,0x04,0x91,0xff,0x70,0xce,0x18,0xff,
    0xb4,0xcc,0x12,0xff,0x6b,0x4e,0xd5,0xff,0xb0,0xcc,0x12,0xff,0x00,0x00,0x00,0xff,
    0xc8,0x05,0x91,0xff,0x98,0xc7,0xcc,0xff,0x7c,0xcd,0x12,0xff,0x51,0x05,0x91,0xff,
    0x48,0x07,0x14,0xff,0x6d,0x05,0x91,0xff,0x00,0x07,0xda,0xff,0xa0,0xc7,0xcc,0xff,
    0x00,0x07,0xda,0xff,0x3a,0x77,0xd5,0xff,0xda,0x07,0x02,0xff,0x7c,0x94,0xd4,0xff,
    0xe0,0xce,0xd6,0xff,0x0a,0x80,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,
    0x78,0x9a,0xab,0xff,0xde,0x08,0x18,0xff,0xda,0x07,0x02,0xff,0x30,0x00,0x00,0xff,
    0x00,0x00,0x00,0xff,0x50,0xce,0x12,0xff,0x8c,0xcd,0x12,0xff,0xd0,0xb7,0xd8,0xff,
    0x00,0x00,0x00,0xff,0x60,0x32,0xd9,0xff,0x30,0xc1,0x1a,0xff,0xa8,0xcd,0x12,0xff,
    0xa4,0xcd,0x12,0xff,0xc0,0x1d,0x4b,0xff,0x46,0x71,0x0e,0xff,0xc0,0x1d,0x4b,0xff,
    0x09,0x87,0xd4,0xff,0x00,0x00,0x00,0xff,0xf6,0x22,0x00,0xff,0x64,0xcd,0x12,0xff,
    0x00,0x00,0x00,0xff,0xca,0x1d,0x4b,0xff,0x09,0x87,0xd4,0xff,0xaa,0x02,0x05,0xff,
    0x82,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0xc0,0x1d,0x4b,0xff,
    0xcd,0xab,0xba,0xff,0x00,0x00,0x00,0xff,0xa4,0xcd,0x12,0xff,0xc0,0x1d,0x4b,0xff,
    0xd4,0xcd,0x12,0xff,0xa6,0x4c,0xd5,0xff,0x00,0xf0,0xfd,0xff,0xd4,0xcd,0x12,0xff,
    0xf4,0x4c,0xd5,0xff,0x90,0xcd,0x12,0xff,0xc2,0x4c,0xd5,0xff,0x82,0x00,0x00,0xff,
    0xaa,0x02,0x05,0xff,0x88,0xd4,0xba,0xff,0x14,0x00,0x00,0xff,0x01,0x00,0x00,0xff,
    0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x10,0x00,0x00,0xff,0x00,0x00,0x00,0xff,
    0x0c,0x08,0x13,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,
    0xd0,0xcd,0x12,0xff,0xc6,0x84,0xf1,0xff,0x7c,0x84,0xf1,0xff,0x20,0x20,0xf5,0xff,
    0x00,0x00,0x0a,0xff,0xf0,0xb0,0x94,0xff,0x64,0x6c,0xf1,0xff,0x85,0x6c,0xf1,0xff,
    0x8b,0x4f,0xd5,0xff,0x00,0x04,0xda,0xff,0x88,0xd4,0xba,0xff,0x82,0x00,0x00,0xff,
    0x39,0xde,0xd4,0xff,0x10,0x50,0xd5,0xff,0xaa,0x02,0x05,0xff,0x00,0x00,0x00,0xff,
    0x4f,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x5c,0xce,0x12,0xff,0x00,0x00,0x00,0xff,
    0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x5c,0xce,0x12,0xff,
    0xaa,0x02,0x05,0xff,0x4c,0xce,0x12,0xff,0x39,0xe6,0xd4,0xff,0x00,0x00,0x00,0xff,
    0x82,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x5b,0xe6,0xd4,0xff,0x00,0x00,0x00,0xff,
    0x00,0x00,0x00,0xff,0x68,0x50,0xcd,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,
    0x00,0x00,0x00,0xff,0x10,0x00,0x00,0xff,0xe3,0xea,0x90,0xff,0x5c,0xce,0x12,0xff,
    0x18,0x00,0x00,0xff,0x88,0xd4,0xba,0xff,0x82,0x00,0x00,0xff,0x00,0x00,0x00,0xff,
    0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
    0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
    0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,
    0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01
};

/* 4x4 cube map dds */
static const unsigned char dds_cube_map[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x08,0x00,0x04,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x44,0x58,0x54,0x35,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x10,0x00,0x00,
    0x00,0xfe,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x50,
    0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x50,
    0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x50,
    0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x50,
    0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x50,
    0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x50
};

/* 4x4x2 volume map dds, 2 mipmaps */
static const unsigned char dds_volume_map[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x8a,0x00,0x04,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x03,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x44,0x58,0x54,0x33,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x10,0x40,0x00,
    0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x50,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xef,0x87,0x0f,0x78,0x05,0x05,0x50,0x50,
    0xff,0x00,0xff,0x00,0x00,0x00,0x00,0x00,0x2f,0x7e,0xcf,0x79,0x01,0x54,0x5c,0x5c,
    0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x84,0xef,0x7b,0xaa,0xab,0xab,0xab
};

/* invalid image file */
static const unsigned char noimage[4] =
{
    0x11,0x22,0x33,0x44
};

/* 1x1 1bpp bmp image */
static const BYTE test_bmp_1bpp[] =
{
    0x42, 0x4d, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x28, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x02, 0x00,
    0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xf1, 0xf2, 0xf3, 0x80, 0xf4, 0xf5, 0xf6, 0x81, 0x00, 0x00,
    0x00, 0x00
};
static const BYTE test_bmp_1bpp_data[] =
{
    0xf3, 0xf2, 0xf1, 0xff
};

/* 1x1 4bpp bmp image */
static const BYTE test_bmp_4bpp[] =
{
    0x42, 0x4d, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x28, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x02, 0x00,
    0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xf1, 0xf2, 0xf3, 0x80, 0xf4, 0xf5, 0xf6, 0x81, 0x00, 0x00,
    0x00, 0x00
};
static const BYTE test_bmp_4bpp_data[] =
{
    0xf3, 0xf2, 0xf1, 0xff
};

/* 1x1 8bpp bmp image */
static const BYTE test_bmp_8bpp[] =
{
    0x42, 0x4d, 0x42, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3e, 0x00, 0x00, 0x00, 0x28, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x02, 0x00,
    0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xf1, 0xf2, 0xf3, 0x80, 0xf4, 0xf5, 0xf6, 0x81, 0x00, 0x00,
    0x00, 0x00
};
static const BYTE test_bmp_8bpp_data[] =
{
    0xf3, 0xf2, 0xf1, 0xff
};

/* 1x1 16bpp bmp image */
static const BYTE test_bmp_16bpp[] =
{
    0x42, 0x4d, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x10, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0e, 0x42, 0x00, 0x00, 0x00, 0x00
};
static const BYTE test_bmp_16bpp_data[] =
{
    0x84, 0x84, 0x73, 0xff
};

/* 1x1 24bpp bmp image */
static const BYTE test_bmp_24bpp[] =
{
    0x42, 0x4d, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x73, 0x84, 0x84, 0x00, 0x00, 0x00
};
static const BYTE test_bmp_24bpp_data[] =
{
    0x84, 0x84, 0x73, 0xff
};

/* 2x2 32bpp XRGB bmp image */
static const BYTE test_bmp_32bpp_xrgb[] =
{
    0x42, 0x4d, 0x46, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00,
    0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0xb0, 0xc0, 0x00, 0xa1, 0xb1, 0xc1, 0x00, 0xa2, 0xb2,
    0xc2, 0x00, 0xa3, 0xb3, 0xc3, 0x00
};
static const BYTE test_bmp_32bpp_xrgb_data[] =
{
    0xc2, 0xb2, 0xa2, 0xff, 0xc3, 0xb3, 0xa3, 0xff, 0xc0, 0xb0, 0xa0, 0xff, 0xc1, 0xb1, 0xa1, 0xff

};

/* 2x2 32bpp ARGB bmp image */
static const BYTE test_bmp_32bpp_argb[] =
{
    0x42, 0x4d, 0x46, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00,
    0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x12, 0x0b, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xa0, 0xb0, 0xc0, 0x00, 0xa1, 0xb1, 0xc1, 0x00, 0xa2, 0xb2,
    0xc2, 0x00, 0xa3, 0xb3, 0xc3, 0x01
};
static const BYTE test_bmp_32bpp_argb_data[] =
{
    0xc2, 0xb2, 0xa2, 0xff, 0xc3, 0xb3, 0xa3, 0xff, 0xc0, 0xb0, 0xa0, 0xff, 0xc1, 0xb1, 0xa1, 0xff

};

/* 1x1 8bpp gray png image */
static const BYTE test_png_8bpp_gray[] =
{
    0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x00, 0x00, 0x00, 0x00, 0x3a, 0x7e, 0x9b,
    0x55, 0x00, 0x00, 0x00, 0x0a, 0x49, 0x44, 0x41, 0x54, 0x08, 0xd7, 0x63, 0xf8, 0x0f, 0x00, 0x01,
    0x01, 0x01, 0x00, 0x1b, 0xb6, 0xee, 0x56, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae,
    0x42, 0x60, 0x82
};
static const BYTE test_png_8bpp_gray_data[] =
{
    0xff, 0xff, 0xff, 0xff
};

/* 1x1 jpg image */
static const BYTE test_jpg[] =
{
    0xff, 0xd8, 0xff, 0xe0, 0x00, 0x10, 0x4a, 0x46, 0x49, 0x46, 0x00, 0x01, 0x01, 0x01, 0x01, 0x2c,
    0x01, 0x2c, 0x00, 0x00, 0xff, 0xdb, 0x00, 0x43, 0x00, 0x05, 0x03, 0x04, 0x04, 0x04, 0x03, 0x05,
    0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x06, 0x07, 0x0c, 0x08, 0x07, 0x07, 0x07, 0x07, 0x0f, 0x0b,
    0x0b, 0x09, 0x0c, 0x11, 0x0f, 0x12, 0x12, 0x11, 0x0f, 0x11, 0x11, 0x13, 0x16, 0x1c, 0x17, 0x13,
    0x14, 0x1a, 0x15, 0x11, 0x11, 0x18, 0x21, 0x18, 0x1a, 0x1d, 0x1d, 0x1f, 0x1f, 0x1f, 0x13, 0x17,
    0x22, 0x24, 0x22, 0x1e, 0x24, 0x1c, 0x1e, 0x1f, 0x1e, 0xff, 0xdb, 0x00, 0x43, 0x01, 0x05, 0x05,
    0x05, 0x07, 0x06, 0x07, 0x0e, 0x08, 0x08, 0x0e, 0x1e, 0x14, 0x11, 0x14, 0x1e, 0x1e, 0x1e, 0x1e,
    0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,
    0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,
    0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0xff, 0xc0,
    0x00, 0x11, 0x08, 0x00, 0x01, 0x00, 0x01, 0x03, 0x01, 0x22, 0x00, 0x02, 0x11, 0x01, 0x03, 0x11,
    0x01, 0xff, 0xc4, 0x00, 0x15, 0x00, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0xff, 0xc4, 0x00, 0x14, 0x10, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xc4,
    0x00, 0x14, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xff, 0xc4, 0x00, 0x14, 0x11, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xda, 0x00, 0x0c, 0x03, 0x01,
    0x00, 0x02, 0x11, 0x03, 0x11, 0x00, 0x3f, 0x00, 0xb2, 0xc0, 0x07, 0xff, 0xd9
};
static const BYTE test_jpg_data[] =
{
    0xff, 0xff, 0xff, 0xff
};

/* 1x1 gif image */
static const BYTE test_gif[] =
{
    0x47, 0x49, 0x46, 0x38, 0x37, 0x61, 0x01, 0x00, 0x01, 0x00, 0x80, 0x00, 0x00, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0x2c, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x02, 0x02, 0x44,
    0x01, 0x00, 0x3b
};
static const BYTE test_gif_data[] =
{
    0xff, 0xff, 0xff, 0xff
};

/* 1x1 tiff image */
static const BYTE test_tiff[] =
{
    0x49, 0x49, 0x2a, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0xfe, 0x00,
    0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x02, 0x01, 0x03, 0x00, 0x03, 0x00, 0x00, 0x00, 0xd2, 0x00, 0x00, 0x00, 0x03, 0x01,
    0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x06, 0x01, 0x03, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x0d, 0x01, 0x02, 0x00, 0x1b, 0x00, 0x00, 0x00, 0xd8, 0x00,
    0x00, 0x00, 0x11, 0x01, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x12, 0x01,
    0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x15, 0x01, 0x03, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x16, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x40, 0x00,
    0x00, 0x00, 0x17, 0x01, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x1a, 0x01,
    0x05, 0x00, 0x01, 0x00, 0x00, 0x00, 0xf4, 0x00, 0x00, 0x00, 0x1b, 0x01, 0x05, 0x00, 0x01, 0x00,
    0x00, 0x00, 0xfc, 0x00, 0x00, 0x00, 0x1c, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x28, 0x01, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x08, 0x00, 0x08, 0x00, 0x08, 0x00, 0x2f, 0x68, 0x6f, 0x6d, 0x65, 0x2f, 0x6d, 0x65,
    0x68, 0x2f, 0x44, 0x65, 0x73, 0x6b, 0x74, 0x6f, 0x70, 0x2f, 0x74, 0x65, 0x73, 0x74, 0x2e, 0x74,
    0x69, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x48,
    0x00, 0x00, 0x00, 0x01
};
static const BYTE test_tiff_data[] =
{
    0x00, 0x00, 0x00, 0xff
};

/* 1x1 alpha dds image */
static const BYTE test_dds_alpha[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x07, 0x10, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff
};
static const BYTE test_dds_alpha_data[] =
{
    0xff
};

/* 1x1 luminance dds image */
static const BYTE test_dds_luminance[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x07, 0x10, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x82
};
static const BYTE test_dds_luminance_data[] =
{
    0x82, 0x82, 0x82, 0xff
};

/* 1x1 16bpp dds image */
static const BYTE test_dds_16bpp[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x07, 0x10, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x7c, 0x00, 0x00,
    0xe0, 0x03, 0x00, 0x00, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0e, 0x42
};
static const BYTE test_dds_16bpp_data[] =
{
    0x84, 0x84, 0x73, 0xff
};

/* 1x1 24bpp dds image */
static const BYTE test_dds_24bpp[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x07, 0x10, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00,
    0x00, 0xff, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x70, 0x81, 0x83
};
static const BYTE test_dds_24bpp_data[] =
{
    0x83, 0x81, 0x70, 0xff
};

/* 1x1 32bpp dds image */
static const BYTE test_dds_32bpp[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x07, 0x10, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00,
    0x00, 0xff, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x70, 0x81, 0x83, 0xff
};
static const BYTE test_dds_32bpp_data[] =
{
    0x83, 0x81, 0x70, 0xff
};

/* 1x1 64bpp dds image */
static const BYTE test_dds_64bpp[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x0f, 0x10, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x83, 0x83, 0x81, 0x81, 0x70, 0x70, 0xff, 0xff
};
static const BYTE test_dds_64bpp_data[] =
{
    0x83, 0x83, 0x81, 0x81, 0x70, 0x70, 0xff, 0xff
};

/* 1x1 96bpp dds image */
static const BYTE test_dds_96bpp[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x0f, 0x10, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x44, 0x58, 0x31, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x84, 0x83, 0x03, 0x3f, 0x82, 0x81, 0x01, 0x3f, 0xe2, 0xe0, 0xe0, 0x3e
};
static const BYTE test_dds_96bpp_data[] =
{
    0x84, 0x83, 0x03, 0x3f, 0x82, 0x81, 0x01, 0x3f, 0xe2, 0xe0, 0xe0, 0x3e
};

/* 1x1 128bpp dds image */
static const BYTE test_dds_128bpp[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x0f, 0x10, 0x02, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x74, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x84, 0x83, 0x03, 0x3f, 0x82, 0x81, 0x01, 0x3f, 0xe2, 0xe0, 0xe0, 0x3e, 0x00, 0x00, 0x80, 0x3f
};
static const BYTE test_dds_128bpp_data[] =
{
    0x84, 0x83, 0x03, 0x3f, 0x82, 0x81, 0x01, 0x3f, 0xe2, 0xe0, 0xe0, 0x3e, 0x00, 0x00, 0x80, 0x3f

};

/* 4x4 DXT1 dds image */
static const BYTE test_dds_dxt1[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x07, 0x10, 0x08, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x44, 0x58, 0x54, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x2a, 0x31, 0xf5, 0xbc, 0xe3, 0x6e, 0x2a, 0x3a
};
static const BYTE test_dds_dxt1_data[] =
{
    0x2a, 0x31, 0xf5, 0xbc, 0xe3, 0x6e, 0x2a, 0x3a
};

/* 4x8 DXT1 dds image */
static const BYTE test_dds_dxt1_4x8[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x07, 0x10, 0x0a, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x44, 0x58, 0x54, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x92, 0xce, 0x09, 0x7a, 0x5d, 0xdd, 0xa7, 0x26, 0x55, 0xde, 0xaf, 0x52, 0xbc, 0xf8, 0x6c, 0x44,
    0x53, 0xbd, 0x8b, 0x72, 0x55, 0x33, 0x88, 0xaa, 0xb2, 0x9c, 0x6c, 0x93, 0x55, 0x00, 0x55, 0x00,
    0x0f, 0x9c, 0x0f, 0x9c, 0x00, 0x00, 0x00, 0x00,
};
static const BYTE test_dds_dxt1_4x8_data[] =
{
    0x92, 0xce, 0x09, 0x7a, 0x5d, 0xdd, 0xa7, 0x26, 0x55, 0xde, 0xaf, 0x52, 0xbc, 0xf8, 0x6c, 0x44,
};

/* 4x4 DXT2 dds image */
static const BYTE test_dds_dxt2[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x07, 0x10, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x44, 0x58, 0x54, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfd, 0xde, 0xc4, 0x10, 0x2f, 0xbf, 0xff, 0x7b,
    0xff, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x57, 0x53, 0x00, 0x00, 0x52, 0x52, 0x55, 0x55,
    0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xce, 0x59, 0x00, 0x00, 0x54, 0x55, 0x55, 0x55
};
static const BYTE test_dds_dxt2_data[] =
{
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfd, 0xde, 0xc4, 0x10, 0x2f, 0xbf, 0xff, 0x7b

};

/* 1x3 DXT3 dds image */
static const BYTE test_dds_dxt3[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x07, 0x10, 0x0a, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x44, 0x58, 0x54, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0c, 0x92, 0x38, 0x84, 0x00, 0xff, 0x55, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x53, 0x8b, 0x53, 0x8b, 0x00, 0x00, 0x00, 0x00
};
static const BYTE test_dds_dxt3_data[] =
{
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x4e, 0x92, 0xd6, 0x83, 0x00, 0xaa, 0x55, 0x55

};

/* 4x4 DXT4 dds image */
static const BYTE test_dds_dxt4[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x07, 0x10, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x44, 0x58, 0x54, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfd, 0xde, 0xc4, 0x10, 0x2f, 0xbf, 0xff, 0x7b,
    0xff, 0x00, 0x40, 0x02, 0x24, 0x49, 0x92, 0x24, 0x57, 0x53, 0x00, 0x00, 0x52, 0x52, 0x55, 0x55,
    0xff, 0x00, 0x48, 0x92, 0x24, 0x49, 0x92, 0x24, 0xce, 0x59, 0x00, 0x00, 0x54, 0x55, 0x55, 0x55
};
static const BYTE test_dds_dxt4_data[] =
{
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfd, 0xde, 0xc4, 0x10, 0x2f, 0xbf, 0xff, 0x7b

};

/* 4x2 DXT5 dds image */
static const BYTE test_dds_dxt5[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x07, 0x10, 0x08, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x44, 0x58, 0x54, 0x35, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xef, 0x87, 0x0f, 0x78, 0x05, 0x05, 0x50, 0x50
};
static const BYTE test_dds_dxt5_data[] =
{
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xef, 0x87, 0x0f, 0x78, 0x05, 0x05, 0x05, 0x05

};

/* 8x8 DXT5 dds image */
static const BYTE test_dds_dxt5_8x8[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x07, 0x10, 0x0a, 0x00, 0x08, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x44, 0x58, 0x54, 0x35, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4b, 0x8a, 0x72, 0x39, 0x5e, 0x5e, 0xfa, 0xa8,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0xd7, 0xd5, 0x4a, 0x2d, 0x2d, 0xad, 0xfd,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x9a, 0x73, 0x83, 0xa0, 0xf0, 0x78, 0x78,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x5b, 0x06, 0x19, 0x00, 0xe8, 0x78, 0x58,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x39, 0xbe, 0x8c, 0x49, 0x35, 0xb5, 0xff, 0x7f,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x96, 0x84, 0xab, 0x59, 0x11, 0xff, 0x11, 0xff,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x6a, 0xf0, 0x6a, 0x00, 0x00, 0x00, 0x00,
};
static const BYTE test_dds_dxt5_8x8_data[] =
{
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4b, 0x8a, 0x72, 0x39, 0x5e, 0x5e, 0xfa, 0xa8,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1c, 0xd7, 0xd5, 0x4a, 0x2d, 0x2d, 0xad, 0xfd,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x47, 0x9a, 0x73, 0x83, 0xa0, 0xf0, 0x78, 0x78,
    0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x5b, 0x06, 0x19, 0x00, 0xe8, 0x78, 0x58,
};

/* 4x4 BC4 dds image */
static const BYTE test_dds_bc4[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x07, 0x10, 0x0a, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x42, 0x43, 0x34, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xd9, 0x15, 0xbc, 0x41, 0x5b, 0xa3, 0x3d, 0x3a, 0x8f, 0x3d, 0x45, 0x81, 0x20, 0x45, 0x81, 0x20,
    0x6f, 0x6f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static const BYTE test_dds_bc4_data[] =
{
    0xd9, 0x15, 0xbc, 0x41, 0x5b, 0xa3, 0x3d, 0x3a
};

/* 6x3 BC5 dds image */
static const BYTE test_dds_bc5[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x07, 0x10, 0x0a, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x42, 0x43, 0x35, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x40, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x9f, 0x28, 0x73, 0xac, 0xd5, 0x80, 0xaa, 0xd5, 0x70, 0x2c, 0x4e, 0xd6, 0x76, 0x1d, 0xd6, 0x76,
    0xd5, 0x0f, 0xc3, 0x50, 0x96, 0xcf, 0x53, 0x96, 0xdf, 0x16, 0xc3, 0x50, 0x96, 0xcf, 0x53, 0x96,
    0x83, 0x55, 0x08, 0x83, 0x30, 0x08, 0x83, 0x30, 0x79, 0x46, 0x31, 0x1c, 0xc3, 0x31, 0x1c, 0xc3,
    0x6d, 0x6d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x5c, 0x5c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static const BYTE test_dds_bc5_data[] =
{
    0x95, 0x35, 0xe2, 0xa3, 0xf5, 0xd2, 0x28, 0x68, 0x65, 0x32, 0x7c, 0x4e, 0xdb, 0xe4, 0x56, 0x0a,
    0xb9, 0x33, 0xaf, 0xf0, 0x52, 0xbe, 0xed, 0x27, 0xb4, 0x2e, 0xa6, 0x60, 0x4e, 0xb6, 0x5d, 0x3f

};

/* 4x4 DXT1 cube map */
static const BYTE test_dds_cube[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x07, 0x10, 0x0a, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x44, 0x58, 0x54, 0x31, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x40, 0x00,
    0x00, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xf5, 0xa7, 0x08, 0x69, 0x74, 0xc0, 0xbf, 0xd7, 0x32, 0x96, 0x0b, 0x7b, 0xcc, 0x55, 0xcc, 0x55,
    0x0e, 0x84, 0x0e, 0x84, 0x00, 0x00, 0x00, 0x00, 0xf5, 0xa7, 0x08, 0x69, 0x74, 0xc0, 0xbf, 0xd7,
    0x32, 0x96, 0x0b, 0x7b, 0xcc, 0x55, 0xcc, 0x55, 0x0e, 0x84, 0x0e, 0x84, 0x00, 0x00, 0x00, 0x00,
    0xf5, 0xa7, 0x08, 0x69, 0x74, 0xc0, 0xbf, 0xd7, 0x32, 0x96, 0x0b, 0x7b, 0xcc, 0x55, 0xcc, 0x55,
    0x0e, 0x84, 0x0e, 0x84, 0x00, 0x00, 0x00, 0x00, 0xf5, 0xa7, 0x08, 0x69, 0x74, 0xc0, 0xbf, 0xd7,
    0x32, 0x96, 0x0b, 0x7b, 0xcc, 0x55, 0xcc, 0x55, 0x0e, 0x84, 0x0e, 0x84, 0x00, 0x00, 0x00, 0x00,
    0xf5, 0xa7, 0x08, 0x69, 0x74, 0xc0, 0xbf, 0xd7, 0x32, 0x96, 0x0b, 0x7b, 0xcc, 0x55, 0xcc, 0x55,
    0x0e, 0x84, 0x0e, 0x84, 0x00, 0x00, 0x00, 0x00, 0xf5, 0xa7, 0x08, 0x69, 0x74, 0xc0, 0xbf, 0xd7,
    0x32, 0x96, 0x0b, 0x7b, 0xcc, 0x55, 0xcc, 0x55, 0x0e, 0x84, 0x0e, 0x84, 0x00, 0x00, 0x00, 0x00
};
static const BYTE test_dds_cube_data[] =
{
    0xf5, 0xa7, 0x08, 0x69, 0x74, 0xc0, 0xbf, 0xd7,
    0xf5, 0xa7, 0x08, 0x69, 0x74, 0xc0, 0xbf, 0xd7,
    0xf5, 0xa7, 0x08, 0x69, 0x74, 0xc0, 0xbf, 0xd7,
    0xf5, 0xa7, 0x08, 0x69, 0x74, 0xc0, 0xbf, 0xd7,
    0xf5, 0xa7, 0x08, 0x69, 0x74, 0xc0, 0xbf, 0xd7,
    0xf5, 0xa7, 0x08, 0x69, 0x74, 0xc0, 0xbf, 0xd7
};

/* 4x4x2 DXT3 volume dds, 2 mipmaps */
static const BYTE test_dds_volume[] =
{
    0x44, 0x44, 0x53, 0x20, 0x7c, 0x00, 0x00, 0x00, 0x07, 0x10, 0x8a, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00,
    0x04, 0x00, 0x00, 0x00, 0x44, 0x58, 0x54, 0x33, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x40, 0x00,
    0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xef, 0x87, 0x0f, 0x78, 0x05, 0x05, 0x50, 0x50,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xef, 0x87, 0x0f, 0x78, 0x05, 0x05, 0x50, 0x50,
    0xff, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x2f, 0x7e, 0xcf, 0x79, 0x01, 0x54, 0x5c, 0x5c,
    0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x84, 0xef, 0x7b, 0xaa, 0xab, 0xab, 0xab
};
static const BYTE test_dds_volume_data[] =
{
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xef, 0x87, 0x0f, 0x78, 0x05, 0x05, 0x50, 0x50,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xef, 0x87, 0x0f, 0x78, 0x05, 0x05, 0x50, 0x50,
};

/*
 * 4x4x4 24-bit volume dds, 3 mipmaps. Level 0 is red, level 1 is green, level 2 is
 * blue.
 */
static const uint8_t dds_volume_24bit_4_4_4[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x0f,0x10,0x82,0x00,0x04,0x00,0x00,0x00,
    0x04,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x03,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x00,0x00,0xff,0x00,
    0x00,0xff,0x00,0x00,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x10,0x40,0x00,
    0x00,0x00,0x20,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,
    0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,
    0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,
    0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,
    0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,
    0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,
    0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,
    0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,
    0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,
    0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,
    0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,
    0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,
    0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,
    0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0xff,0x00,0x00
};

/*
 * 8x8 24-bit dds, 4 mipmaps. Level 0 is red, level 1 is green, level 2 is
 * blue, and level 3 is black.
 */
static const uint8_t dds_24bit_8_8[] =
{
    0x44,0x44,0x53,0x20,0x7c,0x00,0x00,0x00,0x07,0x10,0x0a,0x00,0x08,0x00,0x00,0x00,
    0x08,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x00,0x00,0x00,
    0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x00,0x00,0x00,0x00,0x00,0xff,0x00,
    0x00,0xff,0x00,0x00,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08,0x10,0x40,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,
    0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,
    0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,
    0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,
    0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,
    0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,
    0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,
    0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,
    0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,
    0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,
    0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,
    0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,
    0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,
    0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,
    0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,
    0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0xff,0x00,0x00,0x00,0x00,0x00
};

/* 1x1 wmp image */
static const BYTE test_wmp[] =
{
    0x49, 0x49, 0xbc, 0x01, 0x20, 0x00, 0x00, 0x00, 0x24, 0xc3, 0xdd, 0x6f, 0x03, 0x4e, 0xfe, 0x4b,
    0xb1, 0x85, 0x3d, 0x77, 0x76, 0x8d, 0xc9, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x08, 0x00, 0x01, 0xbc, 0x01, 0x00, 0x10, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x02, 0xbc,
    0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0xbc, 0x04, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x81, 0xbc, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x82, 0xbc, 0x0b, 0x00, 0x01, 0x00, 0x00, 0x00, 0x25, 0x06, 0xc0, 0x42, 0x83, 0xbc,
    0x0b, 0x00, 0x01, 0x00, 0x00, 0x00, 0x25, 0x06, 0xc0, 0x42, 0xc0, 0xbc, 0x04, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x86, 0x00, 0x00, 0x00, 0xc1, 0xbc, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x92, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x57, 0x4d, 0x50, 0x48, 0x4f, 0x54, 0x4f, 0x00, 0x11, 0x45,
    0xc0, 0x71, 0x00, 0x00, 0x00, 0x00, 0x60, 0x00, 0xc0, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0xc0,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x08, 0x00, 0x25, 0xff, 0xff, 0x00, 0x00, 0x01,
    0x01, 0xc8, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x04, 0x10, 0x10, 0xa6, 0x18, 0x8c, 0x21,
    0x00, 0xc4, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x03, 0x4e, 0x0f, 0x3a, 0x4c, 0x94, 0x9d, 0xba, 0x79, 0xe7, 0x38,
    0x4c, 0xcf, 0x14, 0xc3, 0x43, 0x91, 0x88, 0xfb, 0xdc, 0xe0, 0x7c, 0x34, 0x70, 0x9b, 0x28, 0xa9,
    0x18, 0x74, 0x62, 0x87, 0x8e, 0xe4, 0x68, 0x5f, 0xb9, 0xcc, 0x0e, 0xe1, 0x8c, 0x76, 0x3a, 0x9b,
    0x82, 0x76, 0x71, 0x13, 0xde, 0x50, 0xd4, 0x2d, 0xc2, 0xda, 0x1e, 0x3b, 0xa6, 0xa1, 0x62, 0x7b,
    0xca, 0x1a, 0x85, 0x4b, 0x6e, 0x74, 0xec, 0x60
};
static const BYTE test_wmp_data[] =
{
    0xff, 0xff, 0xff, 0xff
};

static const struct test_image
{
    const BYTE *data;
    unsigned int size;
    const BYTE *expected_data;
    D3DX11_IMAGE_INFO expected_info;
    D3D11_SRV_DIMENSION expected_srv_dimension;
}
test_image[] =
{
    {
        test_bmp_1bpp,       sizeof(test_bmp_1bpp),          test_bmp_1bpp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_BMP},
        D3D11_SRV_DIMENSION_TEXTURE2D
    },
    {
        test_bmp_4bpp,       sizeof(test_bmp_4bpp),          test_bmp_4bpp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_BMP},
        D3D11_SRV_DIMENSION_TEXTURE2D
    },
    {
        test_bmp_8bpp,       sizeof(test_bmp_8bpp),          test_bmp_8bpp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_BMP},
        D3D11_SRV_DIMENSION_TEXTURE2D
    },
    {
        test_bmp_16bpp,      sizeof(test_bmp_16bpp),         test_bmp_16bpp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_BMP},
        D3D11_SRV_DIMENSION_TEXTURE2D
    },
    {
        test_bmp_24bpp,      sizeof(test_bmp_24bpp),         test_bmp_24bpp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_BMP},
        D3D11_SRV_DIMENSION_TEXTURE2D
    },
    {
        test_bmp_32bpp_xrgb, sizeof(test_bmp_32bpp_xrgb),    test_bmp_32bpp_xrgb_data,
        {2, 2, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_BMP},
        D3D11_SRV_DIMENSION_TEXTURE2D
    },
    {
        test_bmp_32bpp_argb, sizeof(test_bmp_32bpp_argb),    test_bmp_32bpp_argb_data,
        {2, 2, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_BMP},
        D3D11_SRV_DIMENSION_TEXTURE2D
    },
    {
        test_png_8bpp_gray,  sizeof(test_png_8bpp_gray),     test_png_8bpp_gray_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_PNG},
        D3D11_SRV_DIMENSION_TEXTURE2D
    },
    {
        test_jpg,            sizeof(test_jpg),               test_jpg_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_JPG},
        D3D11_SRV_DIMENSION_TEXTURE2D
    },
    {
        test_gif,            sizeof(test_gif),               test_gif_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_GIF},
        D3D11_SRV_DIMENSION_TEXTURE2D
    },
    {
        test_tiff,           sizeof(test_tiff),              test_tiff_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_TIFF},
        D3D11_SRV_DIMENSION_TEXTURE2D
    },
    {
        test_dds_alpha,      sizeof(test_dds_alpha),         test_dds_alpha_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_A8_UNORM,           D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS},
        D3D11_SRV_DIMENSION_TEXTURE2D
    },
    {
        test_dds_luminance,  sizeof(test_dds_luminance),     test_dds_luminance_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS},
        D3D11_SRV_DIMENSION_TEXTURE2D
    },
    {
        test_dds_16bpp,      sizeof(test_dds_16bpp),         test_dds_16bpp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS},
        D3D11_SRV_DIMENSION_TEXTURE2D
    },
    {
        test_dds_24bpp,      sizeof(test_dds_24bpp),         test_dds_24bpp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS},
        D3D11_SRV_DIMENSION_TEXTURE2D
    },
    {
        test_dds_32bpp,      sizeof(test_dds_32bpp),         test_dds_32bpp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS},
        D3D11_SRV_DIMENSION_TEXTURE2D
    },
    {
        test_dds_64bpp,      sizeof(test_dds_64bpp),         test_dds_64bpp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R16G16B16A16_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS},
        D3D11_SRV_DIMENSION_TEXTURE2D
    },
    {
        test_dds_96bpp,      sizeof(test_dds_96bpp),         test_dds_96bpp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R32G32B32_FLOAT,    D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS},
        D3D11_SRV_DIMENSION_TEXTURE2D
    },
    {
        test_dds_128bpp,     sizeof(test_dds_128bpp),        test_dds_128bpp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R32G32B32A32_FLOAT, D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS},
        D3D11_SRV_DIMENSION_TEXTURE2D
    },
    {
        test_dds_dxt1,       sizeof(test_dds_dxt1),          test_dds_dxt1_data,
        {4, 4, 1, 1, 1, 0,   DXGI_FORMAT_BC1_UNORM,          D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS},
        D3D11_SRV_DIMENSION_TEXTURE2D
    },
    {
        test_dds_dxt1_4x8,   sizeof(test_dds_dxt1_4x8),      test_dds_dxt1_4x8_data,
        {4, 8, 1, 1, 4, 0,   DXGI_FORMAT_BC1_UNORM,          D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS},
        D3D11_SRV_DIMENSION_TEXTURE2D
    },
    {
        test_dds_dxt2,       sizeof(test_dds_dxt2),          test_dds_dxt2_data,
        {4, 4, 1, 1, 3, 0,   DXGI_FORMAT_BC2_UNORM,          D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS},
        D3D11_SRV_DIMENSION_TEXTURE2D
    },
    {
        test_dds_dxt3,       sizeof(test_dds_dxt3),          test_dds_dxt3_data,
        {1, 3, 1, 1, 2, 0,   DXGI_FORMAT_BC2_UNORM,          D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS},
        D3D11_SRV_DIMENSION_TEXTURE2D
    },
    {
        test_dds_dxt4,       sizeof(test_dds_dxt4),          test_dds_dxt4_data,
        {4, 4, 1, 1, 3, 0,   DXGI_FORMAT_BC3_UNORM,          D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS},
        D3D11_SRV_DIMENSION_TEXTURE2D
    },
    {
        test_dds_dxt5,       sizeof(test_dds_dxt5),          test_dds_dxt5_data,
        {4, 2, 1, 1, 1, 0,   DXGI_FORMAT_BC3_UNORM,          D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS},
        D3D11_SRV_DIMENSION_TEXTURE2D
    },
    {
        test_dds_dxt5_8x8,   sizeof(test_dds_dxt5_8x8),      test_dds_dxt5_8x8_data,
        {8, 8, 1, 1, 4, 0,   DXGI_FORMAT_BC3_UNORM,          D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS},
        D3D11_SRV_DIMENSION_TEXTURE2D
    },
    {
        test_dds_bc4,        sizeof(test_dds_bc4),           test_dds_bc4_data,
        {4, 4, 1, 1, 3, 0,   DXGI_FORMAT_BC4_UNORM,          D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS},
        D3D11_SRV_DIMENSION_TEXTURE2D
    },
    {
        test_dds_bc5,        sizeof(test_dds_bc5),           test_dds_bc5_data,
        {6, 3, 1, 1, 3, 0,   DXGI_FORMAT_BC5_UNORM,          D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS},
        D3D11_SRV_DIMENSION_TEXTURE2D
    },
    {
        test_dds_cube,       sizeof(test_dds_cube),          test_dds_cube_data,
        {4, 4, 1, 6, 3, 0x4, DXGI_FORMAT_BC1_UNORM,          D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS},
        D3D11_SRV_DIMENSION_TEXTURECUBE
    },
    {
        test_dds_volume,     sizeof(test_dds_volume),        test_dds_volume_data,
        {4, 4, 2, 1, 3, 0,   DXGI_FORMAT_BC2_UNORM,          D3D11_RESOURCE_DIMENSION_TEXTURE3D, D3DX11_IFF_DDS},
        D3D11_SRV_DIMENSION_TEXTURE3D
    },
    {
        test_wmp,            sizeof(test_wmp),               test_wmp_data,
        {1, 1, 1, 1, 1, 0,   DXGI_FORMAT_R8G8B8A8_UNORM,     D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_WMP},
        D3D11_SRV_DIMENSION_TEXTURE2D
    },
};

static const struct test_image_load_info
{
    const uint8_t *data;
    uint32_t size;
    D3DX11_IMAGE_LOAD_INFO load_info;
    HRESULT expected_hr;

    D3D11_SRV_DIMENSION expected_srv_dimension;
    D3D11_RESOURCE_DIMENSION expected_type;
    union
    {
        D3D11_TEXTURE2D_DESC desc_2d;
        D3D11_TEXTURE3D_DESC desc_3d;
    } expected_resource_desc;
    D3DX11_IMAGE_INFO expected_info;
    BOOL todo_resource_desc;
}
test_image_load_info[] =
{
    /*
     * FirstMipLevel set to 1 - Does not match D3DX_SKIP_DDS_MIP_LEVELS
     * behavior from d3dx9, image info values represent mip level 0, and
     * texture values are pulled from this. The texture data is loaded
     * starting from the specified mip level, however.
     */
    {
        dds_volume_24bit_4_4_4, sizeof(dds_volume_24bit_4_4_4),
        { D3DX11_FROM_FILE, D3DX11_DEFAULT, 0,              1,              D3DX11_DEFAULT, (D3D11_USAGE)D3DX11_DEFAULT,
          D3DX11_DEFAULT,   D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT },
        S_OK, D3D11_SRV_DIMENSION_TEXTURE3D, D3D11_RESOURCE_DIMENSION_TEXTURE3D,
        { .desc_3d = { 4, 4, 4, 3, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0 } },
        { 4, 4, 4, 1, 3, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE3D, D3DX11_IFF_DDS },
    },
    /*
     * Autogen mips misc flag specified. In the case of a cube texture image,
     * the autogen mips flag is OR'd against D3D11_RESOURCE_MISC_TEXTURECUBE,
     * even if it isn't specified.
     */
    {
        test_dds_cube,       sizeof(test_dds_cube),
        { D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, (D3D11_USAGE)D3DX11_DEFAULT,
          (D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET),        D3DX11_DEFAULT, D3D11_RESOURCE_MISC_GENERATE_MIPS,
          DXGI_FORMAT_R8G8B8A8_UNORM, D3DX11_DEFAULT, D3DX11_DEFAULT },
        S_OK, D3D11_SRV_DIMENSION_TEXTURECUBE, D3D11_RESOURCE_DIMENSION_TEXTURE2D,
        { .desc_2d = { 4, 4, 3, 6, DXGI_FORMAT_R8G8B8A8_UNORM, { 1, 0 }, D3D11_USAGE_DEFAULT,
                       (D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET), 0,
                       (D3D11_RESOURCE_MISC_GENERATE_MIPS | D3D11_RESOURCE_MISC_TEXTURECUBE) } },
        { 4, 4, 1, 6, 3, DDS_RESOURCE_MISC_TEXTURECUBE, DXGI_FORMAT_BC1_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D,
          D3DX11_IFF_DDS },
    },
    /*
     * Even with the autogen mips misc flag specified, the mip levels argument
     * of load info is respected.
     */
    {
        test_dds_cube,       sizeof(test_dds_cube),
        { D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, 2,              (D3D11_USAGE)D3DX11_DEFAULT,
          (D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET),        D3DX11_DEFAULT, D3D11_RESOURCE_MISC_GENERATE_MIPS,
          DXGI_FORMAT_R8G8B8A8_UNORM, D3DX11_DEFAULT, D3DX11_DEFAULT },
        S_OK, D3D11_SRV_DIMENSION_TEXTURECUBE, D3D11_RESOURCE_DIMENSION_TEXTURE2D,
        { .desc_2d = { 4, 4, 2, 6, DXGI_FORMAT_R8G8B8A8_UNORM, { 1, 0 }, D3D11_USAGE_DEFAULT,
                       (D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET), 0,
                       (D3D11_RESOURCE_MISC_GENERATE_MIPS | D3D11_RESOURCE_MISC_TEXTURECUBE) } },
        { 4, 4, 1, 6, 3, DDS_RESOURCE_MISC_TEXTURECUBE, DXGI_FORMAT_BC1_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D,
          D3DX11_IFF_DDS },
    },
};

static const struct test_invalid_image_load_info
{
    const uint8_t *data;
    uint32_t size;
    D3DX11_IMAGE_LOAD_INFO load_info;
    HRESULT expected_hr;
    HRESULT expected_process_hr;
    HRESULT expected_create_device_object_hr;
    BOOL todo_hr;
    BOOL todo_process_hr;
    BOOL todo_create_device_object_hr;
}
test_invalid_image_load_info[] =
{
    /*
     * A depth value that isn't D3DX11_FROM_FILE/D3DX11_DEFAULT/0 on a 2D
     * texture results in failure.
     */
    {
        test_dds_32bpp, sizeof(test_dds_32bpp),
        { D3DX11_DEFAULT, D3DX11_DEFAULT, 2,              D3DX11_DEFAULT, D3DX11_DEFAULT, (D3D11_USAGE)D3DX11_DEFAULT,
          D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT },
        E_FAIL, E_FAIL,
    },
    /* Invalid filter value. */
    {
        test_dds_32bpp, sizeof(test_dds_32bpp),
        { D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, (D3D11_USAGE)D3DX11_DEFAULT,
          D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, 7,              D3DX11_DEFAULT },
        D3DERR_INVALIDCALL, D3DERR_INVALIDCALL,
    },
    /* Invalid mipfilter value, only checked if mips are generated. */
    {
        test_dds_32bpp, sizeof(test_dds_32bpp),
        { D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, (D3D11_USAGE)D3DX11_DEFAULT,
          D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, 7 },
        S_OK, S_OK, S_OK
    },
    /* Invalid mipfilter value. */
    {
        test_dds_32bpp, sizeof(test_dds_32bpp),
        { 2,              2,              D3DX11_DEFAULT, D3DX11_DEFAULT, 2,              (D3D11_USAGE)D3DX11_DEFAULT,
          D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, 7 },
        D3DERR_INVALIDCALL, D3DERR_INVALIDCALL,
    },
    /*
     * Usage/BindFlags/CpuAccessFlags are validated in the call to
     * CreateDeviceObject().
     */
    {
        test_dds_32bpp, sizeof(test_dds_32bpp),
        { D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3D11_CPU_ACCESS_READ, D3D11_USAGE_DYNAMIC,
          D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT },
        E_INVALIDARG, S_OK, E_INVALIDARG,
    },
    {
        test_dds_32bpp, sizeof(test_dds_32bpp),
        { D3DX11_DEFAULT,           D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3D11_USAGE_DEFAULT,
          D3D11_BIND_DEPTH_STENCIL, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT },
        E_INVALIDARG, S_OK, E_INVALIDARG,
    },
    /*
     * D3D11_RESOURCE_MISC_GENERATE_MIPS requires binding as a shader resource
     * and a render target.
     */
    {
        test_dds_32bpp, sizeof(test_dds_32bpp),
        { D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT,    D3DX11_DEFAULT, D3D11_USAGE_DEFAULT,
          D3DX11_DEFAULT, D3DX11_DEFAULT, D3D11_RESOURCE_MISC_GENERATE_MIPS, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT },
        E_INVALIDARG, S_OK, E_INVALIDARG,
    },
    /* Can't set the cube texture flag if the image isn't a cube texture. */
    {
        test_dds_32bpp, sizeof(test_dds_32bpp),
        { D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT,    D3DX11_DEFAULT, D3D11_USAGE_DEFAULT,
          D3DX11_DEFAULT, D3DX11_DEFAULT, D3D11_RESOURCE_MISC_TEXTURECUBE,   D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT },
        E_INVALIDARG, S_OK, E_INVALIDARG
    },
};

static void check_image_info(D3DX11_IMAGE_INFO *image_info, const struct test_image *image, unsigned int line)
{
    ok_(__FILE__, line)(image_info->Width == image->expected_info.Width,
            "Got unexpected Width %u, expected %u.\n",
            image_info->Width, image->expected_info.Width);
    ok_(__FILE__, line)(image_info->Height == image->expected_info.Height,
            "Got unexpected Height %u, expected %u.\n",
            image_info->Height, image->expected_info.Height);
    ok_(__FILE__, line)(image_info->Depth == image->expected_info.Depth,
            "Got unexpected Depth %u, expected %u.\n",
            image_info->Depth, image->expected_info.Depth);
    ok_(__FILE__, line)(image_info->ArraySize == image->expected_info.ArraySize,
            "Got unexpected ArraySize %u, expected %u.\n",
            image_info->ArraySize, image->expected_info.ArraySize);
    ok_(__FILE__, line)(image_info->MipLevels == image->expected_info.MipLevels,
            "Got unexpected MipLevels %u, expected %u.\n",
            image_info->MipLevels, image->expected_info.MipLevels);
    ok_(__FILE__, line)(image_info->MiscFlags == image->expected_info.MiscFlags,
            "Got unexpected MiscFlags %#x, expected %#x.\n",
            image_info->MiscFlags, image->expected_info.MiscFlags);
    ok_(__FILE__, line)(image_info->Format == image->expected_info.Format,
            "Got unexpected Format %#x, expected %#x.\n",
            image_info->Format, image->expected_info.Format);
    ok_(__FILE__, line)(image_info->ResourceDimension == image->expected_info.ResourceDimension,
            "Got unexpected ResourceDimension %u, expected %u.\n",
            image_info->ResourceDimension, image->expected_info.ResourceDimension);
    ok_(__FILE__, line)(image_info->ImageFileFormat == image->expected_info.ImageFileFormat,
            "Got unexpected ImageFileFormat %u, expected %u.\n",
            image_info->ImageFileFormat, image->expected_info.ImageFileFormat);
}

#define check_image_info_values(info, width, height, depth, array_size, mip_levels, misc_flags, format, resource_dimension, \
                                image_file_format, wine_todo) \
    check_image_info_values_(__LINE__, info, width, height, depth, array_size, mip_levels, misc_flags, format, resource_dimension, \
            image_file_format, wine_todo)
static inline void check_image_info_values_(uint32_t line, const D3DX11_IMAGE_INFO *info, uint32_t width,
        uint32_t height, uint32_t depth, uint32_t array_size, uint32_t mip_levels, uint32_t misc_flags,
        DXGI_FORMAT format, D3D11_RESOURCE_DIMENSION resource_dimension, D3DX11_IMAGE_FILE_FORMAT image_file_format,
        BOOL wine_todo)
{
    const D3DX11_IMAGE_INFO expected_info = { width, height, depth, array_size, mip_levels, misc_flags, format,
                                              resource_dimension, image_file_format };
    BOOL matched;

    matched = !memcmp(&expected_info, info, sizeof(*info));
    todo_wine_if(wine_todo) ok_(__FILE__, line)(matched, "Got unexpected image info values.\n");
    if (matched)
        return;

    todo_wine_if(wine_todo && info->Width != width)
        ok_(__FILE__, line)(info->Width == width, "Expected width %u, got %u.\n", width, info->Width);
    todo_wine_if(wine_todo && info->Height != height)
        ok_(__FILE__, line)(info->Height == height, "Expected height %u, got %u.\n", height, info->Height);
    todo_wine_if(wine_todo && info->Depth != depth)
        ok_(__FILE__, line)(info->Depth == depth, "Expected depth %u, got %u.\n", depth, info->Depth);
    todo_wine_if(wine_todo && info->ArraySize != array_size)
        ok_(__FILE__, line)(info->ArraySize == array_size, "Expected array_size %u, got %u.\n", array_size,
                info->ArraySize);
    todo_wine_if(wine_todo && info->MipLevels != mip_levels)
        ok_(__FILE__, line)(info->MipLevels == mip_levels, "Expected mip_levels %u, got %u.\n", mip_levels,
                info->MipLevels);
    todo_wine_if(wine_todo && info->MiscFlags != misc_flags)
        ok_(__FILE__, line)(info->MiscFlags == misc_flags, "Expected misc_flags %u, got %u.\n", misc_flags,
                info->MiscFlags);
    ok_(__FILE__, line)(info->Format == format, "Expected texture format %d, got %d.\n", format, info->Format);
    todo_wine_if(wine_todo && info->ResourceDimension != resource_dimension)
        ok_(__FILE__, line)(info->ResourceDimension == resource_dimension, "Expected resource_dimension %d, got %d.\n",
                resource_dimension, info->ResourceDimension);
    ok_(__FILE__, line)(info->ImageFileFormat == image_file_format, "Expected image_file_format %d, got %d.\n",
            image_file_format, info->ImageFileFormat);
}

#define check_texture2d_desc_values(desc, width, height, mip_levels, array_size, format, sample_count, sample_quality, \
                                usage, bind_flags, cpu_access_flags, misc_flags, wine_todo) \
    check_texture2d_desc_values_(__LINE__, desc, width, height, mip_levels, array_size, format, sample_count, sample_quality, \
                                usage, bind_flags, cpu_access_flags, misc_flags, wine_todo)
static inline void check_texture2d_desc_values_(uint32_t line, const D3D11_TEXTURE2D_DESC *desc, uint32_t width,
        uint32_t height, uint32_t mip_levels, uint32_t array_size, DXGI_FORMAT format, uint32_t sample_count,
        uint32_t sample_quality, D3D11_USAGE usage, uint32_t bind_flags, uint32_t cpu_access_flags, uint32_t misc_flags,
        BOOL wine_todo)
{
    const D3D11_TEXTURE2D_DESC expected_desc = { width, height, mip_levels, array_size, format, { sample_count, sample_quality },
                                                 usage, bind_flags, cpu_access_flags, misc_flags };
    BOOL matched;

    matched = !memcmp(&expected_desc, desc, sizeof(*desc));
    todo_wine_if(wine_todo) ok_(__FILE__, line)(matched, "Got unexpected 2D texture desc values.\n");
    if (matched)
        return;

    todo_wine_if(wine_todo && desc->Width != width)
        ok_(__FILE__, line)(desc->Width == width, "Expected width %u, got %u.\n", width, desc->Width);
    todo_wine_if(wine_todo && desc->Height != height)
        ok_(__FILE__, line)(desc->Height == height, "Expected height %u, got %u.\n", height, desc->Height);
    todo_wine_if(wine_todo && desc->ArraySize != array_size)
        ok_(__FILE__, line)(desc->ArraySize == array_size, "Expected array_size %u, got %u.\n", array_size,
                desc->ArraySize);
    todo_wine_if(wine_todo && desc->MipLevels != mip_levels)
        ok_(__FILE__, line)(desc->MipLevels == mip_levels, "Expected mip_levels %u, got %u.\n", mip_levels,
                desc->MipLevels);
    ok_(__FILE__, line)(desc->Format == format, "Expected texture format %#x, got %#x.\n", format, desc->Format);
    todo_wine_if(wine_todo && desc->SampleDesc.Count != sample_count)
        ok_(__FILE__, line)(desc->SampleDesc.Count == sample_count, "Expected sample_count %u, got %u.\n", sample_count,
                desc->SampleDesc.Count);
    todo_wine_if(wine_todo && desc->SampleDesc.Quality != sample_quality)
        ok_(__FILE__, line)(desc->SampleDesc.Quality == sample_quality, "Expected sample_quality %u, got %u.\n", sample_quality,
                desc->SampleDesc.Quality);
    todo_wine_if(wine_todo && desc->Usage != usage)
        ok_(__FILE__, line)(desc->Usage == usage, "Expected usage %u, got %u.\n", usage,
                desc->Usage);
    todo_wine_if(wine_todo && desc->BindFlags != bind_flags)
        ok_(__FILE__, line)(desc->BindFlags == bind_flags, "Expected bind_flags %#x, got %#x.\n", bind_flags,
                desc->BindFlags);
    todo_wine_if(wine_todo && desc->CPUAccessFlags != cpu_access_flags)
        ok_(__FILE__, line)(desc->CPUAccessFlags == cpu_access_flags, "Expected cpu_access_flags %#x, got %#x.\n",
                cpu_access_flags, desc->CPUAccessFlags);
    todo_wine_if(wine_todo && desc->MiscFlags != misc_flags)
        ok_(__FILE__, line)(desc->MiscFlags == misc_flags, "Expected misc_flags %#x, got %#x.\n", misc_flags,
                desc->MiscFlags);
}

#define check_texture3d_desc_values(desc, width, height, depth, mip_levels, format, usage, bind_flags, cpu_access_flags, \
                                    misc_flags, wine_todo) \
    check_texture3d_desc_values_(__LINE__, desc, width, height, depth, mip_levels, format, usage, bind_flags, \
            cpu_access_flags, misc_flags, wine_todo)
static inline void check_texture3d_desc_values_(uint32_t line, const D3D11_TEXTURE3D_DESC *desc, uint32_t width,
        uint32_t height, uint32_t depth, uint32_t mip_levels, DXGI_FORMAT format, D3D11_USAGE usage, uint32_t bind_flags,
        uint32_t cpu_access_flags, uint32_t misc_flags, BOOL wine_todo)
{
    const D3D11_TEXTURE3D_DESC expected_desc = { width, height, depth, mip_levels, format, usage, bind_flags,
                                                 cpu_access_flags, misc_flags };
    BOOL matched;

    matched = !memcmp(&expected_desc, desc, sizeof(*desc));
    todo_wine_if(wine_todo) ok_(__FILE__, line)(matched, "Got unexpected 3D texture desc values.\n");
    if (matched)
        return;

    todo_wine_if(wine_todo && desc->Width != width)
        ok_(__FILE__, line)(desc->Width == width, "Expected width %u, got %u.\n", width, desc->Width);
    todo_wine_if(wine_todo && desc->Height != height)
        ok_(__FILE__, line)(desc->Height == height, "Expected height %u, got %u.\n", height, desc->Height);
    todo_wine_if(wine_todo && desc->Depth != depth)
        ok_(__FILE__, line)(desc->Depth == depth, "Expected depth %u, got %u.\n", depth, desc->Depth);
    todo_wine_if(wine_todo && desc->MipLevels != mip_levels)
        ok_(__FILE__, line)(desc->MipLevels == mip_levels, "Expected mip_levels %u, got %u.\n", mip_levels,
                desc->MipLevels);
    ok_(__FILE__, line)(desc->Format == format, "Expected texture format %#x, got %#x.\n", format, desc->Format);
    todo_wine_if(wine_todo && desc->Usage != usage)
        ok_(__FILE__, line)(desc->Usage == usage, "Expected usage %u, got %u.\n", usage,
                desc->Usage);
    todo_wine_if(wine_todo && desc->BindFlags != bind_flags)
        ok_(__FILE__, line)(desc->BindFlags == bind_flags, "Expected bind_flags %#x, got %#x.\n", bind_flags,
                desc->BindFlags);
    todo_wine_if(wine_todo && desc->CPUAccessFlags != cpu_access_flags)
        ok_(__FILE__, line)(desc->CPUAccessFlags == cpu_access_flags, "Expected cpu_access_flags %#x, got %#x.\n",
                cpu_access_flags, desc->CPUAccessFlags);
    todo_wine_if(wine_todo && desc->MiscFlags != misc_flags)
        ok_(__FILE__, line)(desc->MiscFlags == misc_flags, "Expected misc_flags %#x, got %#x.\n", misc_flags,
                desc->MiscFlags);
}

/*
 * Taken from the d3d11 tests. If there's a missing resource type or
 * texture format checking function, check to see if it exists there first.
 */
struct resource_readback
{
    ID3D11Resource *resource;
    D3D11_MAPPED_SUBRESOURCE map_desc;
    ID3D11DeviceContext *immediate_context;
    uint32_t width, height, depth, sub_resource_idx;
};

static void init_resource_readback(ID3D11Resource *resource, ID3D11Resource *readback_resource,
        uint32_t width, uint32_t height, uint32_t depth, uint32_t sub_resource_idx,
        ID3D11Device *device, struct resource_readback *rb)
{
    HRESULT hr;

    rb->resource = readback_resource;
    rb->width = width;
    rb->height = height;
    rb->depth = depth;
    rb->sub_resource_idx = sub_resource_idx;

    ID3D11Device_GetImmediateContext(device, &rb->immediate_context);

    ID3D11DeviceContext_CopyResource(rb->immediate_context, rb->resource, resource);
    if (FAILED(hr = ID3D11DeviceContext_Map(rb->immediate_context,
            rb->resource, sub_resource_idx, D3D11_MAP_READ, 0, &rb->map_desc)))
    {
        trace("Failed to map resource, hr %#lx.\n", hr);
        ID3D11Resource_Release(rb->resource);
        rb->resource = NULL;
        ID3D11DeviceContext_Release(rb->immediate_context);
        rb->immediate_context = NULL;
    }
}

static void get_texture_readback(ID3D11Texture2D *texture, uint32_t sub_resource_idx,
        struct resource_readback *rb)
{
    D3D11_TEXTURE2D_DESC texture_desc;
    ID3D11Resource *rb_texture;
    uint32_t miplevel;
    ID3D11Device *device;
    HRESULT hr;

    memset(rb, 0, sizeof(*rb));

    ID3D11Texture2D_GetDevice(texture, &device);

    ID3D11Texture2D_GetDesc(texture, &texture_desc);
    texture_desc.Usage = D3D11_USAGE_STAGING;
    texture_desc.BindFlags = 0;
    texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    texture_desc.MiscFlags = 0;
    if (FAILED(hr = ID3D11Device_CreateTexture2D(device, &texture_desc, NULL, (ID3D11Texture2D **)&rb_texture)))
    {
        trace("Failed to create texture, hr %#lx.\n", hr);
        ID3D11Device_Release(device);
        return;
    }

    miplevel = sub_resource_idx % texture_desc.MipLevels;
    init_resource_readback((ID3D11Resource *)texture, rb_texture,
            max(1, texture_desc.Width >> miplevel),
            max(1, texture_desc.Height >> miplevel),
            1, sub_resource_idx, device, rb);

    ID3D11Device_Release(device);
}

static void get_texture3d_readback(ID3D11Texture3D *texture, unsigned int sub_resource_idx,
        struct resource_readback *rb)
{
    D3D11_TEXTURE3D_DESC texture_desc;
    ID3D11Resource *rb_texture;
    unsigned int miplevel;
    ID3D11Device *device;
    HRESULT hr;

    memset(rb, 0, sizeof(*rb));

    ID3D11Texture3D_GetDevice(texture, &device);

    ID3D11Texture3D_GetDesc(texture, &texture_desc);
    texture_desc.Usage = D3D11_USAGE_STAGING;
    texture_desc.BindFlags = 0;
    texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    texture_desc.MiscFlags = 0;
    if (FAILED(hr = ID3D11Device_CreateTexture3D(device, &texture_desc, NULL, (ID3D11Texture3D **)&rb_texture)))
    {
        trace("Failed to create texture, hr %#lx.\n", hr);
        ID3D11Device_Release(device);
        return;
    }

    miplevel = sub_resource_idx % texture_desc.MipLevels;
    init_resource_readback((ID3D11Resource *)texture, rb_texture,
            max(1, texture_desc.Width >> miplevel),
            max(1, texture_desc.Height >> miplevel),
            max(1, texture_desc.Depth >> miplevel),
            sub_resource_idx, device, rb);

    ID3D11Device_Release(device);
}

static void *get_readback_data(struct resource_readback *rb,
        uint32_t x, uint32_t y, uint32_t z, unsigned byte_width)
{
    return (uint8_t *)rb->map_desc.pData + z * rb->map_desc.DepthPitch + y * rb->map_desc.RowPitch + x * byte_width;
}

static uint32_t get_readback_u32(struct resource_readback *rb, uint32_t x, uint32_t y, uint32_t z)
{
    return *(uint32_t *)get_readback_data(rb, x, y, z, sizeof(uint32_t));
}

static uint32_t get_readback_color(struct resource_readback *rb, uint32_t x, uint32_t y, uint32_t z)
{
    return get_readback_u32(rb, x, y, z);
}

static void release_resource_readback(struct resource_readback *rb)
{
    ID3D11DeviceContext_Unmap(rb->immediate_context, rb->resource, rb->sub_resource_idx);
    ID3D11Resource_Release(rb->resource);
    ID3D11DeviceContext_Release(rb->immediate_context);
}

static BOOL compare_color(uint32_t c1, uint32_t c2, uint8_t max_diff)
{
    return compare_uint(c1 & 0xff, c2 & 0xff, max_diff)
            && compare_uint((c1 >> 8) & 0xff, (c2 >> 8) & 0xff, max_diff)
            && compare_uint((c1 >> 16) & 0xff, (c2 >> 16) & 0xff, max_diff)
            && compare_uint((c1 >> 24) & 0xff, (c2 >> 24) & 0xff, max_diff);
}

#define check_readback_data_color(a, b, c, d) check_readback_data_color_(__LINE__, a, b, c, d)
static void check_readback_data_color_(uint32_t line, struct resource_readback *rb,
        const RECT *rect, uint32_t expected_color, uint8_t max_diff)
{
    uint32_t x = 0, y = 0, z = 0, color = 0;
    BOOL all_match = FALSE;
    RECT default_rect;

    if (!rect)
    {
        SetRect(&default_rect, 0, 0, rb->width, rb->height);
        rect = &default_rect;
    }

    for (z = 0; z < rb->depth; ++z)
    {
        for (y = rect->top; y < rect->bottom; ++y)
        {
            for (x = rect->left; x < rect->right; ++x)
            {
                color = get_readback_color(rb, x, y, z);
                if (!compare_color(color, expected_color, max_diff))
                    goto done;
            }
        }
    }
    all_match = TRUE;

done:
    ok_(__FILE__, line)(all_match,
            "Got 0x%08x, expected 0x%08x at (%u, %u, %u), sub-resource %u.\n",
            color, expected_color, x, y, z, rb->sub_resource_idx);
}

#define check_texture_sub_resource_color(a, b, c, d, e) check_texture_sub_resource_color_(__LINE__, a, b, c, d, e)
static void check_texture_sub_resource_color_(uint32_t line, ID3D11Texture2D *texture,
        uint32_t sub_resource_idx, const RECT *rect, uint32_t expected_color, uint8_t max_diff)
{
    struct resource_readback rb;

    get_texture_readback(texture, sub_resource_idx, &rb);
    check_readback_data_color_(line, &rb, rect, expected_color, max_diff);
    release_resource_readback(&rb);
}

static void set_d3dx11_image_load_info(D3DX11_IMAGE_LOAD_INFO *info, uint32_t width, uint32_t height, uint32_t depth,
        uint32_t first_mip_level, uint32_t mip_levels, D3D11_USAGE usage, uint32_t bind_flags, uint32_t cpu_access_flags,
        uint32_t misc_flags, DXGI_FORMAT format, uint32_t filter, uint32_t mip_filter, D3DX11_IMAGE_INFO *src_info)
{
    info->Width = width;
    info->Height = height;
    info->Depth = depth;
    info->FirstMipLevel = first_mip_level;
    info->MipLevels = mip_levels;
    info->Usage = usage;
    info->BindFlags = bind_flags;
    info->CpuAccessFlags = cpu_access_flags;
    info->MiscFlags = misc_flags;
    info->Format = format;
    info->Filter = filter;
    info->MipFilter = mip_filter;
    info->pSrcInfo = src_info;
}

#define check_test_image_load_info_resource(resource, image_load_info) \
    check_test_image_load_info_resource_(__LINE__, resource, image_load_info)
static void check_test_image_load_info_resource_(uint32_t line, ID3D11Resource *resource,
        const struct test_image_load_info *image_load_info)
{
    D3D11_RESOURCE_DIMENSION resource_dimension;
    HRESULT hr;

    ID3D11Resource_GetType(resource, &resource_dimension);
    ok(resource_dimension == image_load_info->expected_type, "Got unexpected ResourceDimension %u, expected %u.\n",
             resource_dimension, image_load_info->expected_type);

    switch (resource_dimension)
    {
    case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
    {
        const D3D11_TEXTURE2D_DESC *expected_desc_2d = &image_load_info->expected_resource_desc.desc_2d;
        D3D11_TEXTURE2D_DESC desc_2d;
        ID3D11Texture2D *tex_2d;

        hr = ID3D11Resource_QueryInterface(resource, &IID_ID3D11Texture2D, (void **)&tex_2d);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n",  hr);
        ID3D11Texture2D_GetDesc(tex_2d, &desc_2d);
        check_texture2d_desc_values_(line, &desc_2d, expected_desc_2d->Width, expected_desc_2d->Height,
                expected_desc_2d->MipLevels, expected_desc_2d->ArraySize, expected_desc_2d->Format,
                expected_desc_2d->SampleDesc.Count, expected_desc_2d->SampleDesc.Quality, expected_desc_2d->Usage,
                expected_desc_2d->BindFlags, expected_desc_2d->CPUAccessFlags, expected_desc_2d->MiscFlags,
                image_load_info->todo_resource_desc);
        ID3D11Texture2D_Release(tex_2d);
        break;
    }

    case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
    {
        const D3D11_TEXTURE3D_DESC *expected_desc_3d = &image_load_info->expected_resource_desc.desc_3d;
        D3D11_TEXTURE3D_DESC desc_3d;
        ID3D11Texture3D *tex_3d;

        hr = ID3D11Resource_QueryInterface(resource, &IID_ID3D11Texture3D, (void **)&tex_3d);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n",  hr);
        ID3D11Texture3D_GetDesc(tex_3d, &desc_3d);
        check_texture3d_desc_values_(line, &desc_3d, expected_desc_3d->Width, expected_desc_3d->Height,
                expected_desc_3d->Depth, expected_desc_3d->MipLevels, expected_desc_3d->Format, expected_desc_3d->Usage,
                expected_desc_3d->BindFlags, expected_desc_3d->CPUAccessFlags, expected_desc_3d->MiscFlags,
                image_load_info->todo_resource_desc);
        ID3D11Texture3D_Release(tex_3d);
        break;
    }

    default:
        break;
    }
}

#define check_test_image_load_info_srv(srv, image_load_info) \
    check_test_image_load_info_srv_(__LINE__, srv, image_load_info)
static void check_test_image_load_info_srv_(uint32_t line, ID3D11ShaderResourceView *srv,
        const struct test_image_load_info *image_load_info)
{
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
    ID3D11Resource *resource;

    ID3D11ShaderResourceView_GetDesc(srv, &srv_desc);
    ok_(__FILE__, line)(srv_desc.ViewDimension == image_load_info->expected_srv_dimension, "Got unexpected ViewDimension %u, expected %u.\n",
            srv_desc.ViewDimension, image_load_info->expected_srv_dimension);
    if (srv_desc.ViewDimension != image_load_info->expected_srv_dimension)
        return;

    ID3D11ShaderResourceView_GetResource(srv, &resource);
    check_test_image_load_info_resource_(line, resource, image_load_info);
    ID3D11Resource_Release(resource);
    switch (srv_desc.ViewDimension)
    {
    case D3D11_SRV_DIMENSION_TEXTURE2D:
        ok_(__FILE__, line)(srv_desc.Format == image_load_info->expected_resource_desc.desc_2d.Format,
                "Got unexpected Format %u, expected %u.\n", srv_desc.Format, image_load_info->expected_resource_desc.desc_2d.Format);
        ok_(__FILE__, line)(!srv_desc.Texture2D.MostDetailedMip, "Unexpected MostDetailedMip %u.\n",
                srv_desc.Texture2D.MostDetailedMip);
        ok_(__FILE__, line)(srv_desc.Texture2D.MipLevels == image_load_info->expected_resource_desc.desc_2d.MipLevels,
                "Unexpected MipLevels %u.\n", srv_desc.Texture2D.MipLevels);
        break;

    case D3D11_SRV_DIMENSION_TEXTURE2DARRAY:
        ok_(__FILE__, line)(srv_desc.Format == image_load_info->expected_resource_desc.desc_2d.Format,
                "Got unexpected Format %u, expected %u.\n", srv_desc.Format, image_load_info->expected_resource_desc.desc_2d.Format);
        ok_(__FILE__, line)(!srv_desc.Texture2DArray.MostDetailedMip, "Unexpected MostDetailedMip %u.\n",
                srv_desc.Texture2DArray.MostDetailedMip);
        ok_(__FILE__, line)(srv_desc.Texture2DArray.MipLevels == image_load_info->expected_resource_desc.desc_2d.MipLevels,
                "Unexpected MipLevels %u.\n", srv_desc.Texture2DArray.MipLevels);
        ok_(__FILE__, line)(!srv_desc.Texture2DArray.FirstArraySlice, "Unexpected FirstArraySlice %u.\n",
                srv_desc.Texture2DArray.FirstArraySlice);
        ok_(__FILE__, line)(srv_desc.Texture2DArray.ArraySize == image_load_info->expected_resource_desc.desc_2d.ArraySize,
                "Unexpected ArraySize %u.\n", srv_desc.Texture2DArray.ArraySize);
        break;

    case D3D11_SRV_DIMENSION_TEXTURECUBE:
        ok_(__FILE__, line)(srv_desc.Format == image_load_info->expected_resource_desc.desc_2d.Format,
                "Got unexpected Format %u, expected %u.\n", srv_desc.Format, image_load_info->expected_resource_desc.desc_2d.Format);
        ok_(__FILE__, line)(!srv_desc.TextureCube.MostDetailedMip, "Unexpected MostDetailedMip %u.\n",
                srv_desc.TextureCube.MostDetailedMip);
        ok_(__FILE__, line)(srv_desc.TextureCube.MipLevels == image_load_info->expected_resource_desc.desc_2d.MipLevels,
                "Unexpected MipLevels %u.\n", srv_desc.TextureCube.MipLevels);
        break;

    case D3D11_SRV_DIMENSION_TEXTURE3D:
        ok_(__FILE__, line)(srv_desc.Format == image_load_info->expected_resource_desc.desc_3d.Format,
                "Got unexpected Format %u, expected %u.\n", srv_desc.Format, image_load_info->expected_resource_desc.desc_3d.Format);
        ok_(__FILE__, line)(!srv_desc.Texture3D.MostDetailedMip, "Unexpected MostDetailedMip %u.\n",
                srv_desc.Texture3D.MostDetailedMip);
        ok_(__FILE__, line)(srv_desc.Texture3D.MipLevels == image_load_info->expected_resource_desc.desc_3d.MipLevels,
                "Unexpected MipLevels %u.\n", srv_desc.Texture3D.MipLevels);
        break;

    default:
        ok_(__FILE__, line)(0, "Unexpected ViewDimension %u.\n", srv_desc.ViewDimension);
        break;
    }
}

static void check_resource_info(ID3D11Resource *resource, const struct test_image *image, uint32_t line)
{
    unsigned int expected_mip_levels, expected_width, expected_height, max_dimension;
    D3D11_RESOURCE_DIMENSION resource_dimension;
    D3D11_TEXTURE2D_DESC desc_2d;
    D3D11_TEXTURE3D_DESC desc_3d;
    ID3D11Texture2D *texture_2d;
    ID3D11Texture3D *texture_3d;
    HRESULT hr;

    expected_width = image->expected_info.Width;
    expected_height = image->expected_info.Height;
    if (is_block_compressed(image->expected_info.Format))
    {
        expected_width = (expected_width + 3) & ~3;
        expected_height = (expected_height + 3) & ~3;
    }
    expected_mip_levels = 0;
    max_dimension = max(expected_width, expected_height);
    while (max_dimension)
    {
        ++expected_mip_levels;
        max_dimension >>= 1;
    }

    ID3D11Resource_GetType(resource, &resource_dimension);
    ok(resource_dimension == image->expected_info.ResourceDimension,
            "Got unexpected ResourceDimension %u, expected %u.\n",
             resource_dimension, image->expected_info.ResourceDimension);

    switch (resource_dimension)
    {
        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
            hr = ID3D11Resource_QueryInterface(resource, &IID_ID3D11Texture2D, (void **)&texture_2d);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n",  hr);
            ID3D11Texture2D_GetDesc(texture_2d, &desc_2d);
            ok_(__FILE__, line)(desc_2d.Width == expected_width,
                    "Got unexpected Width %u, expected %u.\n",
                     desc_2d.Width, expected_width);
            ok_(__FILE__, line)(desc_2d.Height == expected_height,
                    "Got unexpected Height %u, expected %u.\n",
                     desc_2d.Height, expected_height);
            ok_(__FILE__, line)(desc_2d.MipLevels == expected_mip_levels,
                    "Got unexpected MipLevels %u, expected %u.\n",
                     desc_2d.MipLevels, expected_mip_levels);
            ok_(__FILE__, line)(desc_2d.ArraySize == image->expected_info.ArraySize,
                    "Got unexpected ArraySize %u, expected %u.\n",
                     desc_2d.ArraySize, image->expected_info.ArraySize);
            ok_(__FILE__, line)(desc_2d.Format == image->expected_info.Format,
                    "Got unexpected Format %u, expected %u.\n",
                     desc_2d.Format, image->expected_info.Format);
            ok_(__FILE__, line)(desc_2d.SampleDesc.Count == 1,
                    "Got unexpected SampleDesc.Count %u, expected %u\n",
                     desc_2d.SampleDesc.Count, 1);
            ok_(__FILE__, line)(desc_2d.SampleDesc.Quality == 0,
                    "Got unexpected SampleDesc.Quality %u, expected %u\n",
                     desc_2d.SampleDesc.Quality, 0);
            ok_(__FILE__, line)(desc_2d.Usage == D3D11_USAGE_DEFAULT,
                    "Got unexpected Usage %u, expected %u\n",
                     desc_2d.Usage, D3D11_USAGE_DEFAULT);
            ok_(__FILE__, line)(desc_2d.BindFlags == D3D11_BIND_SHADER_RESOURCE,
                    "Got unexpected BindFlags %#x, expected %#x\n",
                     desc_2d.BindFlags, D3D11_BIND_SHADER_RESOURCE);
            ok_(__FILE__, line)(desc_2d.CPUAccessFlags == 0,
                    "Got unexpected CPUAccessFlags %#x, expected %#x\n",
                     desc_2d.CPUAccessFlags, 0);
            ok_(__FILE__, line)(desc_2d.MiscFlags == image->expected_info.MiscFlags,
                    "Got unexpected MiscFlags %#x, expected %#x.\n",
                     desc_2d.MiscFlags, image->expected_info.MiscFlags);

            ID3D11Texture2D_Release(texture_2d);
            break;

        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
            hr = ID3D11Resource_QueryInterface(resource, &IID_ID3D11Texture3D, (void **)&texture_3d);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n",  hr);
            ID3D11Texture3D_GetDesc(texture_3d, &desc_3d);
            ok_(__FILE__, line)(desc_3d.Width == expected_width,
                    "Got unexpected Width %u, expected %u.\n",
                     desc_3d.Width, expected_width);
            ok_(__FILE__, line)(desc_3d.Height == expected_height,
                    "Got unexpected Height %u, expected %u.\n",
                     desc_3d.Height, expected_height);
            ok_(__FILE__, line)(desc_3d.Depth == image->expected_info.Depth,
                    "Got unexpected Depth %u, expected %u.\n",
                     desc_3d.Depth, image->expected_info.Depth);
            ok_(__FILE__, line)(desc_3d.MipLevels == expected_mip_levels,
                    "Got unexpected MipLevels %u, expected %u.\n",
                     desc_3d.MipLevels, expected_mip_levels);
            ok_(__FILE__, line)(desc_3d.Format == image->expected_info.Format,
                    "Got unexpected Format %u, expected %u.\n",
                     desc_3d.Format, image->expected_info.Format);
            ok_(__FILE__, line)(desc_3d.Usage == D3D11_USAGE_DEFAULT,
                    "Got unexpected Usage %u, expected %u\n",
                     desc_3d.Usage, D3D11_USAGE_DEFAULT);
            ok_(__FILE__, line)(desc_3d.BindFlags == D3D11_BIND_SHADER_RESOURCE,
                    "Got unexpected BindFlags %#x, expected %#x\n",
                     desc_3d.BindFlags, D3D11_BIND_SHADER_RESOURCE);
            ok_(__FILE__, line)(desc_3d.CPUAccessFlags == 0,
                    "Got unexpected CPUAccessFlags %#x, expected %#x\n",
                     desc_3d.CPUAccessFlags, 0);
            ok_(__FILE__, line)(desc_3d.MiscFlags == image->expected_info.MiscFlags,
                    "Got unexpected MiscFlags %#x, expected %#x.\n",
                     desc_3d.MiscFlags, image->expected_info.MiscFlags);
            ID3D11Texture3D_Release(texture_3d);
            break;

        default:
            break;
    }
}

static void check_texture2d_data(ID3D11Texture2D *texture, const struct test_image *image, unsigned int line)
{
    unsigned int width, height, stride, i, array_slice;
    struct resource_readback rb;
    D3D11_TEXTURE2D_DESC desc;
    const BYTE *expected_data;
    BOOL line_match;

    ID3D11Texture2D_GetDesc(texture, &desc);
    width = desc.Width;
    height = desc.Height;
    stride = (width * get_bpp_from_format(desc.Format) + 7) / 8;
    if (is_block_compressed(desc.Format))
    {
        stride *= 4;
        height /= 4;
    }

    expected_data = image->expected_data;
    for (array_slice = 0; array_slice < desc.ArraySize; ++array_slice)
    {
        get_texture_readback(texture, array_slice * desc.MipLevels, &rb);
        for (i = 0; i < height; ++i)
        {
            const uint8_t *rb_data = get_readback_data(&rb, 0, i, 0, 0);

            line_match = !memcmp(expected_data + stride * i, rb_data, stride);
            todo_wine_if(is_block_compressed(image->expected_info.Format) && image->data != test_dds_dxt5
                    && (image->expected_info.Width % 4 != 0 || image->expected_info.Height % 4 != 0))
                ok_(__FILE__, line)(line_match, "Data mismatch for line %u, array slice %u.\n", i, array_slice);
            if (!line_match)
                break;
        }
        expected_data += stride * height;
        release_resource_readback(&rb);
    }
}

static void check_texture3d_data(ID3D11Texture3D *texture, const struct test_image *image, unsigned int line)
{
    unsigned int width, height, depth, stride, i, j;
    struct resource_readback rb;
    D3D11_TEXTURE3D_DESC desc;
    const BYTE *expected_data;
    BOOL line_match;

    ID3D11Texture3D_GetDesc(texture, &desc);
    width = desc.Width;
    height = desc.Height;
    depth = desc.Depth;
    stride = (width * get_bpp_from_format(desc.Format) + 7) / 8;
    if (is_block_compressed(desc.Format))
    {
        stride *= 4;
        height /= 4;
    }

    expected_data = image->expected_data;
    get_texture3d_readback(texture, 0, &rb);
    for (j = 0; j < depth; ++j)
    {
        const BYTE *expected_data_slice = expected_data + ((stride * height) * j);

        for (i = 0; i < height; ++i)
        {
            const uint8_t *rb_data = get_readback_data(&rb, 0, i, j, 0);

            line_match = !memcmp(expected_data_slice + stride * i, rb_data, stride);
            ok_(__FILE__, line)(line_match, "Data mismatch for line %u.\n", i);
            if (!line_match)
            {
                for (unsigned int k = 0; k < stride; ++k)
                    trace("%02x\n", *((BYTE *)get_readback_data(&rb, k, i, j, 1)));
                break;
            }
        }
    }
    release_resource_readback(&rb);
}

static void check_resource_data(ID3D11Resource *resource, const struct test_image *image, unsigned int line)
{
    ID3D11Texture3D *texture3d;
    ID3D11Texture2D *texture2d;

    if (SUCCEEDED(ID3D11Resource_QueryInterface(resource, &IID_ID3D11Texture3D, (void **)&texture3d)))
    {
        if (wined3d_opengl && is_block_compressed(image->expected_info.Format))
            skip("Skipping compressed format 3D texture readback test.\n");
        else
            check_texture3d_data(texture3d, image, line);
        ID3D11Texture3D_Release(texture3d);
    }
    else if (SUCCEEDED(ID3D11Resource_QueryInterface(resource, &IID_ID3D11Texture2D, (void **)&texture2d)))
    {
        check_texture2d_data(texture2d, image, line);
        ID3D11Texture2D_Release(texture2d);
    }
    else
    {
        ok(0, "Failed to get 2D or 3D texture interface.\n");
    }
}

static void check_shader_resource_view_info(ID3D11ShaderResourceView *srv, const struct test_image *image, uint32_t line)
{
    uint32_t expected_mip_levels, expected_width, expected_height, max_dimension;
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
    ID3D11Resource *resource;

    expected_width = image->expected_info.Width;
    expected_height = image->expected_info.Height;
    if (is_block_compressed(image->expected_info.Format))
    {
        expected_width = (expected_width + 3) & ~3;
        expected_height = (expected_height + 3) & ~3;
    }
    expected_mip_levels = 0;
    max_dimension = max(max(expected_width, expected_height), image->expected_info.Depth);
    while (max_dimension)
    {
        ++expected_mip_levels;
        max_dimension >>= 1;
    }

    ID3D11ShaderResourceView_GetDesc(srv, &srv_desc);
    ok_(__FILE__, line)(srv_desc.Format == image->expected_info.Format, "Got unexpected Format %u, expected %u.\n",
            srv_desc.Format, image->expected_info.Format);
    ok_(__FILE__, line)(srv_desc.ViewDimension == image->expected_srv_dimension, "Got unexpected ViewDimension %u, expected %u.\n",
            srv_desc.ViewDimension, image->expected_srv_dimension);
    if (srv_desc.ViewDimension != image->expected_srv_dimension)
        return;

    ID3D11ShaderResourceView_GetResource(srv, &resource);
    check_resource_info(resource, image, line);
    check_resource_data(resource, image, line);
    ID3D11Resource_Release(resource);

    switch (srv_desc.ViewDimension)
    {
    case D3D11_SRV_DIMENSION_TEXTURE2D:
        ok_(__FILE__, line)(!srv_desc.Texture2D.MostDetailedMip, "Unexpected MostDetailedMip %u.\n",
                srv_desc.Texture2D.MostDetailedMip);
        ok_(__FILE__, line)(srv_desc.Texture2D.MipLevels == expected_mip_levels, "Unexpected MipLevels %u.\n",
                srv_desc.Texture2D.MipLevels);
        break;

    case D3D11_SRV_DIMENSION_TEXTURE2DARRAY:
        ok_(__FILE__, line)(!srv_desc.Texture2DArray.MostDetailedMip, "Unexpected MostDetailedMip %u.\n",
                srv_desc.Texture2DArray.MostDetailedMip);
        ok_(__FILE__, line)(srv_desc.Texture2DArray.MipLevels == expected_mip_levels, "Unexpected MipLevels %u.\n",
                srv_desc.Texture2DArray.MipLevels);
        ok_(__FILE__, line)(!srv_desc.Texture2DArray.FirstArraySlice, "Unexpected FirstArraySlice %u.\n",
                srv_desc.Texture2DArray.FirstArraySlice);
        ok_(__FILE__, line)(srv_desc.Texture2DArray.ArraySize == image->expected_info.ArraySize, "Unexpected ArraySize %u.\n",
                srv_desc.Texture2DArray.ArraySize);
        break;

    case D3D11_SRV_DIMENSION_TEXTURECUBE:
        ok_(__FILE__, line)(!srv_desc.TextureCube.MostDetailedMip, "Unexpected MostDetailedMip %u.\n",
                srv_desc.TextureCube.MostDetailedMip);
        ok_(__FILE__, line)(srv_desc.TextureCube.MipLevels == expected_mip_levels, "Unexpected MipLevels %u.\n",
                srv_desc.TextureCube.MipLevels);
        break;

    case D3D11_SRV_DIMENSION_TEXTURE3D:
        ok_(__FILE__, line)(!srv_desc.Texture3D.MostDetailedMip, "Unexpected MostDetailedMip %u.\n",
                srv_desc.Texture3D.MostDetailedMip);
        ok_(__FILE__, line)(srv_desc.Texture3D.MipLevels == expected_mip_levels, "Unexpected MipLevels %u.\n",
                srv_desc.Texture3D.MipLevels);
        break;

    default:
        ok_(__FILE__, line)(0, "Unexpected ViewDimension %u.\n", srv_desc.ViewDimension);
        break;
    }
}

static WCHAR temp_dir[MAX_PATH];

static char *get_str_a(const WCHAR *wstr)
{
    static char buffer[MAX_PATH];

    WideCharToMultiByte(CP_ACP, 0, wstr, -1, buffer, sizeof(buffer), NULL, NULL);
    return buffer;
}

static BOOL create_file(const WCHAR *filename, const void *data, unsigned int size, WCHAR *out_path)
{
    WCHAR path[MAX_PATH];
    DWORD written;
    HANDLE file;

    if (!temp_dir[0])
        GetTempPathW(ARRAY_SIZE(temp_dir), temp_dir);
    lstrcpyW(path, temp_dir);
    lstrcatW(path, filename);

    file = CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (file == INVALID_HANDLE_VALUE)
        return FALSE;

    if (WriteFile(file, data, size, &written, NULL))
    {
        CloseHandle(file);

        if (out_path)
            lstrcpyW(out_path, path);
        return TRUE;
    }

    CloseHandle(file);
    return FALSE;
}

static void delete_file(const WCHAR *filename)
{
    WCHAR path[MAX_PATH];

    lstrcpyW(path, temp_dir);
    lstrcatW(path, filename);
    DeleteFileW(path);
}

static HMODULE create_resource_module(const WCHAR *filename, const void *data, unsigned int size)
{
    WCHAR resource_module_path[MAX_PATH], current_module_path[MAX_PATH];
    HANDLE resource;
    HMODULE module;
    BOOL ret;

    if (!temp_dir[0])
        GetTempPathW(ARRAY_SIZE(temp_dir), temp_dir);
    lstrcpyW(resource_module_path, temp_dir);
    lstrcatW(resource_module_path, filename);

    GetModuleFileNameW(NULL, current_module_path, ARRAY_SIZE(current_module_path));
    ret = CopyFileW(current_module_path, resource_module_path, FALSE);
    ok(ret, "CopyFileW failed, error %lu.\n", GetLastError());
    SetFileAttributesW(resource_module_path, FILE_ATTRIBUTE_NORMAL);

    resource = BeginUpdateResourceW(resource_module_path, TRUE);
    UpdateResourceW(resource, (LPCWSTR)RT_RCDATA, filename, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), (void *)data, size);
    EndUpdateResourceW(resource, FALSE);

    module = LoadLibraryExW(resource_module_path, NULL, LOAD_LIBRARY_AS_DATAFILE);

    return module;
}

static void delete_resource_module(const WCHAR *filename, HMODULE module)
{
    WCHAR path[MAX_PATH];

    FreeLibrary(module);

    lstrcpyW(path, temp_dir);
    lstrcatW(path, filename);
    DeleteFileW(path);
}

static BOOL create_directory(const WCHAR *dir)
{
    WCHAR path[MAX_PATH];

    lstrcpyW(path, temp_dir);
    lstrcatW(path, dir);
    return CreateDirectoryW(path, NULL);
}

static void delete_directory(const WCHAR *dir)
{
    WCHAR path[MAX_PATH];

    lstrcpyW(path, temp_dir);
    lstrcatW(path, dir);
    RemoveDirectoryW(path);
}

static ID3D11Device *create_device(void)
{
    HRESULT (WINAPI *pD3D11CreateDevice)(IDXGIAdapter *, D3D_DRIVER_TYPE, HMODULE, UINT, const D3D_FEATURE_LEVEL *,
                                         UINT, UINT, ID3D11Device **,  D3D_FEATURE_LEVEL *, ID3D11DeviceContext **);
    HMODULE d3d11_mod = LoadLibraryA("d3d11.dll");
    ID3D11Device *device;


    if (!d3d11_mod)
    {
        win_skip("d3d11.dll not present\n");
        return NULL;
    }

    pD3D11CreateDevice = (void *)GetProcAddress(d3d11_mod, "D3D11CreateDevice");
    if (SUCCEEDED(pD3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0,
            NULL, 0, D3D11_SDK_VERSION, &device, NULL, NULL)))
        return device;
    if (SUCCEEDED(pD3D11CreateDevice(NULL, D3D_DRIVER_TYPE_WARP, NULL, 0,
            NULL, 0, D3D11_SDK_VERSION, &device, NULL, NULL)))
        return device;
    if (SUCCEEDED(pD3D11CreateDevice(NULL, D3D_DRIVER_TYPE_REFERENCE, NULL, 0,
            NULL, 0, D3D11_SDK_VERSION, &device, NULL, NULL)))
        return device;

    return NULL;
}

static void test_D3DX11CreateAsyncMemoryLoader(void)
{
    ID3DX11DataLoader *loader;
    SIZE_T size;
    DWORD data;
    HRESULT hr;
    void *ptr;

    hr = D3DX11CreateAsyncMemoryLoader(NULL, 0, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11CreateAsyncMemoryLoader(NULL, 0, &loader);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11CreateAsyncMemoryLoader(&data, 0, &loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    size = 100;
    hr = ID3DX11DataLoader_Decompress(loader, &ptr, &size);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(ptr == &data, "Got data pointer %p, original %p.\n", ptr, &data);
    ok(!size, "Got unexpected data size.\n");

    /* Load() is no-op. */
    hr = ID3DX11DataLoader_Load(loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID3DX11DataLoader_Destroy(loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    data = 0;
    hr = D3DX11CreateAsyncMemoryLoader(&data, sizeof(data), &loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Load() is no-op. */
    hr = ID3DX11DataLoader_Load(loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID3DX11DataLoader_Decompress(loader, &ptr, &size);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(ptr == &data, "Got data pointer %p, original %p.\n", ptr, &data);
    ok(size == sizeof(data), "Got unexpected data size.\n");

    hr = ID3DX11DataLoader_Destroy(loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
}

static void create_testfile(WCHAR *path, const void *data, int data_len)
{
    DWORD written;
    HANDLE file;
    BOOL ret;

    GetTempPathW(MAX_PATH, path);
    lstrcatW(path, L"asyncloader.data");

    file = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "Test file creation failed, at %s, error %ld.\n",
            wine_dbgstr_w(path), GetLastError());

    ret = WriteFile(file, data, data_len, &written, NULL);
    ok(ret, "Write to test file failed.\n");

    CloseHandle(file);
}

static void test_D3DX11CreateAsyncFileLoader(void)
{
    static const char test_data1[] = "test data";
    static const char test_data2[] = "more test data";
    ID3DX11DataLoader *loader;
    WCHAR path[MAX_PATH];
    SIZE_T size;
    HRESULT hr;
    void *ptr;
    BOOL ret;

    hr = D3DX11CreateAsyncFileLoaderA(NULL, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11CreateAsyncFileLoaderA(NULL, &loader);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11CreateAsyncFileLoaderA("nonexistentfilename", &loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID3DX11DataLoader_Decompress(loader, &ptr, &size);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = ID3DX11DataLoader_Load(loader);
    ok(hr == D3D11_ERROR_FILE_NOT_FOUND, "Got unexpected hr %#lx.\n", hr);

    hr = ID3DX11DataLoader_Decompress(loader, &ptr, &size);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = ID3DX11DataLoader_Destroy(loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Test file sharing using dummy empty file. */
    create_testfile(path, test_data1, sizeof(test_data1));

    hr = D3DX11CreateAsyncFileLoaderW(path, &loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ret = DeleteFileW(path);
    ok(ret, "Got unexpected ret %#x, error %ld.\n", ret, GetLastError());

    /* File was removed before Load(). */
    hr = ID3DX11DataLoader_Load(loader);
    ok(hr == D3D11_ERROR_FILE_NOT_FOUND, "Got unexpected hr %#lx.\n", hr);

    /* Create it again. */
    create_testfile(path, test_data1, sizeof(test_data1));
    hr = ID3DX11DataLoader_Load(loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    /* Already loaded. */
    hr = ID3DX11DataLoader_Load(loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ret = DeleteFileW(path);
    ok(ret, "Got unexpected ret %#x, error %ld.\n", ret, GetLastError());

    /* Already loaded, file removed. */
    hr = ID3DX11DataLoader_Load(loader);
    ok(hr == D3D11_ERROR_FILE_NOT_FOUND, "Got unexpected hr %#lx.\n", hr);

    /* Decompress still works. */
    ptr = NULL;
    hr = ID3DX11DataLoader_Decompress(loader, &ptr, &size);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(ptr != NULL, "Got unexpected ptr %p.\n", ptr);
    ok(size == sizeof(test_data1), "Got unexpected decompressed size.\n");
    if (size == sizeof(test_data1))
        ok(!memcmp(ptr, test_data1, size), "Got unexpected file data.\n");

    /* Create it again, with different data. */
    create_testfile(path, test_data2, sizeof(test_data2));

    hr = ID3DX11DataLoader_Load(loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ptr = NULL;
    hr = ID3DX11DataLoader_Decompress(loader, &ptr, &size);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(ptr != NULL, "Got unexpected ptr %p.\n", ptr);
    ok(size == sizeof(test_data2), "Got unexpected decompressed size.\n");
    if (size == sizeof(test_data2))
        ok(!memcmp(ptr, test_data2, size), "Got unexpected file data.\n");

    hr = ID3DX11DataLoader_Destroy(loader);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    ret = DeleteFileW(path);
    ok(ret, "Got unexpected ret %#x, error %ld.\n", ret, GetLastError());
}

static void test_D3DX11CreateAsyncResourceLoader(void)
{
    ID3DX11DataLoader *loader;
    HRESULT hr;

    hr = D3DX11CreateAsyncResourceLoaderA(NULL, NULL, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11CreateAsyncResourceLoaderA(NULL, NULL, &loader);
    ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11CreateAsyncResourceLoaderA(NULL, "noname", &loader);
    ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11CreateAsyncResourceLoaderW(NULL, NULL, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11CreateAsyncResourceLoaderW(NULL, NULL, &loader);
    ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11CreateAsyncResourceLoaderW(NULL, L"noname", &loader);
    ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
}

static void test_D3DX11CreateAsyncTextureInfoProcessor(void)
{
    ID3DX11DataProcessor *dp;
    D3DX11_IMAGE_INFO info;
    HRESULT hr;
    int i;

    CoInitialize(NULL);

    hr = D3DX11CreateAsyncTextureInfoProcessor(NULL, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11CreateAsyncTextureInfoProcessor(&info, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11CreateAsyncTextureInfoProcessor(NULL, &dp);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    if (0)
    {
        /* Crashes on native. */
        hr = ID3DX11DataProcessor_Process(dp, (void *)test_image[0].data, test_image[0].size);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    }

    hr = ID3DX11DataProcessor_Destroy(dp);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11CreateAsyncTextureInfoProcessor(&info, &dp);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID3DX11DataProcessor_Process(dp, (void *)test_image[0].data, 0);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DX11DataProcessor_Process(dp, NULL, test_image[0].size);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(test_image); ++i)
    {
        winetest_push_context("Test %u", i);

        hr = ID3DX11DataProcessor_Process(dp, (void *)test_image[i].data, test_image[i].size);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX11_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        if (hr == S_OK)
            check_image_info(&info, test_image + i, __LINE__);

        winetest_pop_context();
    }

    hr = ID3DX11DataProcessor_CreateDeviceObject(dp, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = ID3DX11DataProcessor_Destroy(dp);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    CoUninitialize();
}

static void test_D3DX11CreateAsyncTextureProcessor(void)
{
    ID3DX11DataProcessor *dp;
    ID3D11Resource *resource;
    ID3D11Device *device;
    HRESULT hr;
    int i;

    device = create_device();
    if (!device)
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    CoInitialize(NULL);

    hr = D3DX11CreateAsyncTextureProcessor(device, NULL, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11CreateAsyncTextureProcessor(NULL, NULL, &dp);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11CreateAsyncTextureProcessor(device, NULL, &dp);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DX11DataProcessor_Process(dp, (void *)test_image[0].data, 0);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DX11DataProcessor_Process(dp, NULL, test_image[0].size);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DX11DataProcessor_Destroy(dp);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(test_image); ++i)
    {
        winetest_push_context("Test %u", i);

        hr = D3DX11CreateAsyncTextureProcessor(device, NULL, &dp);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        hr = ID3DX11DataProcessor_Process(dp, (void *)test_image[i].data, test_image[i].size);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX11_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        if (hr == S_OK)
        {
            hr = ID3DX11DataProcessor_CreateDeviceObject(dp, (void **)&resource);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
            check_resource_info(resource, test_image + i, __LINE__);
            check_resource_data(resource, test_image + i, __LINE__);
            ID3D11Resource_Release(resource);
        }

        hr = ID3DX11DataProcessor_Destroy(dp);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        winetest_pop_context();
    }

    for (i = 0; i < ARRAY_SIZE(test_invalid_image_load_info); ++i)
    {
        const struct test_invalid_image_load_info *test_load_info = &test_invalid_image_load_info[i];
        D3DX11_IMAGE_LOAD_INFO load_info = test_load_info->load_info;

        winetest_push_context("Test %u", i);

        hr = D3DX11CreateAsyncTextureProcessor(device, &load_info, &dp);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        hr = ID3DX11DataProcessor_Process(dp, (void *)test_load_info->data, test_load_info->size);
        todo_wine_if(test_load_info->todo_process_hr)
            ok(hr == test_load_info->expected_process_hr, "Got unexpected hr %#lx.\n", hr);
        if (hr == S_OK)
        {
            resource = NULL;
            hr = ID3DX11DataProcessor_CreateDeviceObject(dp, (void **)&resource);
            todo_wine_if(test_load_info->todo_create_device_object_hr)
                ok(hr == test_load_info->expected_create_device_object_hr, "Got unexpected hr %#lx.\n", hr);
            if (SUCCEEDED(hr))
                ID3D11Resource_Release(resource);
        }

        hr = ID3DX11DataProcessor_Destroy(dp);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        winetest_pop_context();
    }

    CoUninitialize();

    ok(!ID3D11Device_Release(device), "Unexpected refcount.\n");
}

static void test_D3DX11CreateAsyncShaderResourceViewProcessor(void)
{
    ID3D11ShaderResourceView *resource_view;
    ID3DX11DataProcessor *dp;
    ID3D11Device *device;
    HRESULT hr;
    uint32_t i;

    device = create_device();
    if (!device)
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    CoInitialize(NULL);

    hr = D3DX11CreateAsyncShaderResourceViewProcessor(device, NULL, NULL);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11CreateAsyncShaderResourceViewProcessor(NULL, NULL, &dp);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11CreateAsyncShaderResourceViewProcessor(device, NULL, &dp);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DX11DataProcessor_Process(dp, (void *)test_image[0].data, 0);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DX11DataProcessor_Process(dp, NULL, test_image[0].size);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DX11DataProcessor_Destroy(dp);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    for (i = 0; i < ARRAY_SIZE(test_image); ++i)
    {
        winetest_push_context("Test %u", i);

        hr = D3DX11CreateAsyncShaderResourceViewProcessor(device, NULL, &dp);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        hr = ID3DX11DataProcessor_Process(dp, (void *)test_image[i].data, test_image[i].size);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX11_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        if (hr == S_OK)
        {
            hr = ID3DX11DataProcessor_CreateDeviceObject(dp, (void **)&resource_view);
            ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

            check_shader_resource_view_info(resource_view, test_image + i, __LINE__);
            ID3D11ShaderResourceView_Release(resource_view);
        }

        hr = ID3DX11DataProcessor_Destroy(dp);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        winetest_pop_context();
    }

    for (i = 0; i < ARRAY_SIZE(test_invalid_image_load_info); ++i)
    {
        const struct test_invalid_image_load_info *test_load_info = &test_invalid_image_load_info[i];
        D3DX11_IMAGE_LOAD_INFO load_info = test_load_info->load_info;

        winetest_push_context("Test %u", i);

        hr = D3DX11CreateAsyncShaderResourceViewProcessor(device, &load_info, &dp);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        hr = ID3DX11DataProcessor_Process(dp, (void *)test_load_info->data, test_load_info->size);
        todo_wine_if(test_load_info->todo_process_hr)
            ok(hr == test_load_info->expected_process_hr, "Got unexpected hr %#lx.\n", hr);
        if (hr == S_OK)
        {
            resource_view = NULL;
            hr = ID3DX11DataProcessor_CreateDeviceObject(dp, (void **)&resource_view);
            todo_wine_if(test_load_info->todo_create_device_object_hr)
                ok(hr == test_load_info->expected_create_device_object_hr, "Got unexpected hr %#lx.\n", hr);
            if (SUCCEEDED(hr))
                ID3D11ShaderResourceView_Release(resource_view);
        }

        hr = ID3DX11DataProcessor_Destroy(dp);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        winetest_pop_context();
    }

    CoUninitialize();

    ok(!ID3D11Device_Release(device), "Unexpected refcount.\n");
}

static HRESULT WINAPI test_d3dinclude_open(ID3DInclude *iface, D3D_INCLUDE_TYPE include_type,
        const char *filename, const void *parent_data, const void **data, UINT *bytes)
{
    static const char include1[] =
        "#define LIGHT float4(0.0f, 0.2f, 0.5f, 1.0f)\n";
    static const char include2[] =
        "#include \"include1.h\"\n"
        "float4 light_color = LIGHT;\n";
    char *buffer;

    trace("filename %s.\n", filename);
    trace("parent_data %p: %s.\n", parent_data, parent_data ? (char *)parent_data : "(null)");

    if (!strcmp(filename, "include1.h"))
    {
        buffer = malloc(strlen(include1));
        memcpy(buffer, include1, strlen(include1));
        *bytes = strlen(include1);
        ok(include_type == D3D_INCLUDE_LOCAL, "Unexpected include type %d.\n", include_type);
        ok(!strncmp(include2, parent_data, strlen(include2)),
                "Unexpected parent_data value.\n");
    }
    else if (!strcmp(filename, "include\\include2.h"))
    {
        buffer = malloc(strlen(include2));
        memcpy(buffer, include2, strlen(include2));
        *bytes = strlen(include2);
        ok(!parent_data, "Unexpected parent_data value.\n");
        ok(include_type == D3D_INCLUDE_LOCAL, "Unexpected include type %d.\n", include_type);
    }
    else
    {
        ok(0, "Unexpected #include for file %s.\n", filename);
        return E_INVALIDARG;
    }

    *data = buffer;
    return S_OK;
}

static HRESULT WINAPI test_d3dinclude_close(ID3DInclude *iface, const void *data)
{
    free((void *)data);
    return S_OK;
}

static const struct ID3DIncludeVtbl test_d3dinclude_vtbl =
{
    test_d3dinclude_open,
    test_d3dinclude_close
};

struct test_d3dinclude
{
    ID3DInclude ID3DInclude_iface;
};

static void test_D3DX11CompileFromFile(void)
{
    struct test_d3dinclude include = {{&test_d3dinclude_vtbl}};
    WCHAR filename[MAX_PATH], directory[MAX_PATH];
    ID3D10Blob *blob = NULL, *errors = NULL;
    CHAR filename_a[MAX_PATH];
    HRESULT hr, result;
    DWORD len;
    static const char ps_code[] =
        "#include \"include\\include2.h\"\n"
        "\n"
        "float4 main() : COLOR\n"
        "{\n"
        "    return light_color;\n"
        "}";
    static const char include1[] =
        "#define LIGHT float4(0.0f, 0.2f, 0.5f, 1.0f)\n";
    static const char include1_wrong[] =
        "#define LIGHT nope\n";
    static const char include2[] =
        "#include \"include1.h\"\n"
        "float4 light_color = LIGHT;\n";

    create_file(L"source.ps", ps_code, strlen(ps_code), filename);
    create_directory(L"include");
    create_file(L"include\\include1.h", include1_wrong, strlen(include1_wrong), NULL);
    create_file(L"include1.h", include1, strlen(include1), NULL);
    create_file(L"include\\include2.h", include2, strlen(include2), NULL);

    hr = D3DX11CompileFromFileW(filename, NULL, &include.ID3DInclude_iface,
            "main", "ps_2_0", 0, 0, NULL, &blob, &errors, &result);
    todo_wine ok(hr == S_OK && hr == result, "Got unexpected hr %#lx, result %#lx.\n", hr, result);
    todo_wine ok(!!blob, "Got unexpected blob.\n");
    todo_wine ok(!errors, "Got unexpected errors.\n");
    if (errors)
    {
        ID3D10Blob_Release(errors);
        errors = NULL;
    }
    if (blob)
    {
        ID3D10Blob_Release(blob);
        blob = NULL;
    }

    /* Windows always seems to resolve includes from the initial file location
     * instead of using the immediate parent, as it would be the case for
     * standard C preprocessor includes. */
    hr = D3DX11CompileFromFileW(filename, NULL, NULL, "main", "ps_2_0", 0, 0, NULL, &blob, &errors, &result);
    todo_wine ok(hr == S_OK && hr == result, "Got unexpected hr %#lx, result %#lx.\n", hr, result);
    todo_wine ok(!!blob, "Got unexpected blob.\n");
    todo_wine ok(!errors, "Got unexpected errors.\n");
    if (errors)
    {
        ID3D10Blob_Release(errors);
        errors = NULL;
    }
    if (blob)
    {
        ID3D10Blob_Release(blob);
        blob = NULL;
    }

    len = WideCharToMultiByte(CP_ACP, 0, filename, -1, NULL, 0, NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, filename, -1, filename_a, len, NULL, NULL);
    hr = D3DX11CompileFromFileA(filename_a, NULL, NULL, "main", "ps_2_0", 0, 0, NULL, &blob, &errors, &result);
    todo_wine ok(hr == S_OK && hr == result, "Got unexpected hr %#lx, result %#lx.\n", hr, result);
    todo_wine ok(!!blob, "Got unexpected blob.\n");
    todo_wine ok(!errors, "Got unexpected errors.\n");
    if (errors)
    {
        ID3D10Blob_Release(errors);
        errors = NULL;
    }
    if (blob)
    {
        ID3D10Blob_Release(blob);
        blob = NULL;
    }

    GetCurrentDirectoryW(MAX_PATH, directory);
    SetCurrentDirectoryW(temp_dir);

    hr = D3DX11CompileFromFileW(L"source.ps", NULL, NULL, "main", "ps_2_0", 0, 0, NULL, &blob, &errors, &result);
    todo_wine ok(hr == S_OK && hr == result, "Got unexpected hr %#lx, result %#lx.\n", hr, result);
    todo_wine ok(!!blob, "Got unexpected blob.\n");
    todo_wine ok(!errors, "Got unexpected errors.\n");
    if (errors)
    {
        ID3D10Blob_Release(errors);
        errors = NULL;
    }
    if (blob)
    {
        ID3D10Blob_Release(blob);
        blob = NULL;
    }

    SetCurrentDirectoryW(directory);

    delete_file(L"source.ps");
    delete_file(L"include\\include1.h");
    delete_file(L"include1.h");
    delete_file(L"include\\include2.h");
    delete_directory(L"include");
}

static DWORD main_tid;
static DWORD io_tid;

struct data_object
{
    ID3DX11DataLoader ID3DX11DataLoader_iface;
    ID3DX11DataProcessor ID3DX11DataProcessor_iface;

    HANDLE load_started;
    HANDLE load_done;
    HANDLE decompress_done;
    HRESULT load_ret;

    DWORD process_tid;
};

static struct data_object *data_object_from_ID3DX11DataLoader(ID3DX11DataLoader *iface)
{
    return CONTAINING_RECORD(iface, struct data_object, ID3DX11DataLoader_iface);
}

static LONG data_loader_load_count;
static WINAPI HRESULT data_loader_Load(ID3DX11DataLoader *iface)
{
    struct data_object *data_object = data_object_from_ID3DX11DataLoader(iface);
    DWORD ret;

    ok(InterlockedDecrement(&data_loader_load_count) >= 0, "Got unexpected call.\n");

    if (!io_tid)
        io_tid = GetCurrentThreadId();
    ok(io_tid != main_tid, "Load called in main thread.\n");
    ok(io_tid == GetCurrentThreadId(), "Load called in wrong thread.\n");

    SetEvent(data_object->load_started);
    ret = WaitForSingleObject(data_object->load_done, INFINITE);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %#lx.\n", ret);
    return data_object->load_ret;
}

static LONG data_loader_decompress_count;
static WINAPI HRESULT data_loader_Decompress(ID3DX11DataLoader *iface, void **data, SIZE_T *bytes)
{
    struct data_object *data_object = data_object_from_ID3DX11DataLoader(iface);
    DWORD ret;

    ok(InterlockedDecrement(&data_loader_decompress_count) >= 0, "Got unexpected call.\n");
    ok(!!data, "Got unexpected data %p.\n", data);
    ok(!!bytes, "Got unexpected bytes %p.\n", bytes);

    data_object->process_tid = GetCurrentThreadId();
    ok(data_object->process_tid != main_tid, "Decompress called in main thread.\n");
    ok(data_object->process_tid != io_tid, "Decompress called in IO thread.\n");

    *data = (void *)0xdeadbeef;
    *bytes = 0xdead;
    ret = WaitForSingleObject(data_object->decompress_done, INFINITE);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %#lx.\n", ret);
    return S_OK;
}

static LONG data_loader_destroy_count;
static WINAPI HRESULT data_loader_Destroy(ID3DX11DataLoader *iface)
{
    ok(InterlockedDecrement(&data_loader_destroy_count) >= 0, "Got unexpected call.\n");
    return S_OK;
}

static ID3DX11DataLoaderVtbl D3DX11DataLoaderVtbl =
{
    data_loader_Load,
    data_loader_Decompress,
    data_loader_Destroy
};

static struct data_object* data_object_from_ID3DX11DataProcessor(ID3DX11DataProcessor *iface)
{
    return CONTAINING_RECORD(iface, struct data_object, ID3DX11DataProcessor_iface);
}

static LONG data_processor_process_count;
static HRESULT WINAPI data_processor_Process(ID3DX11DataProcessor *iface, void *data, SIZE_T bytes)
{
    struct data_object *data_object = data_object_from_ID3DX11DataProcessor(iface);

    ok(InterlockedDecrement(&data_processor_process_count) >= 0, "Got unexpected call.\n");
    ok(data_object->process_tid == GetCurrentThreadId(), "Process called in unexpected thread.\n");

    ok(data == (void *)0xdeadbeef, "Got unexpected data %p.\n", data);
    ok(bytes == 0xdead, "Got unexpected bytes %Iu.\n", bytes);
    return S_OK;
}

static LONG data_processor_create_count;
static HRESULT WINAPI data_processor_CreateDeviceObject(ID3DX11DataProcessor *iface, void **object)
{
    ok(InterlockedDecrement(&data_processor_create_count) >= 0, "Got unexpected call.\n");
    ok(main_tid == GetCurrentThreadId(), "CreateDeviceObject not called in main thread.\n");

    *object = (void *)0xdeadf00d;
    return S_OK;
}

static LONG data_processor_destroy_count;
static HRESULT WINAPI data_processor_Destroy(ID3DX11DataProcessor *iface)
{
    struct data_object *data_object = data_object_from_ID3DX11DataProcessor(iface);

    ok(InterlockedDecrement(&data_processor_destroy_count) >= 0, "Got unexpected call.\n");

    CloseHandle(data_object->load_started);
    CloseHandle(data_object->load_done);
    CloseHandle(data_object->decompress_done);
    free(data_object);
    return S_OK;
}

static ID3DX11DataProcessorVtbl D3DX11DataProcessorVtbl =
{
    data_processor_Process,
    data_processor_CreateDeviceObject,
    data_processor_Destroy
};

static struct data_object *create_data_object(HRESULT load_ret)
{
    struct data_object *data_object = malloc(sizeof(*data_object));

    data_object->ID3DX11DataLoader_iface.lpVtbl = &D3DX11DataLoaderVtbl;
    data_object->ID3DX11DataProcessor_iface.lpVtbl = &D3DX11DataProcessorVtbl;

    data_object->load_started = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(!!data_object->load_started, "CreateEvent failed, error %lu.\n", GetLastError());
    data_object->load_done = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(!!data_object->load_done, "CreateEvent failed, error %lu.\n", GetLastError());
    data_object->decompress_done = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(!!data_object->decompress_done, "CreateEvent failed, error %lu.\n", GetLastError());
    data_object->load_ret = load_ret;

    return data_object;
}

static void test_D3DX11CreateThreadPump(void)
{
    UINT io_count, process_count, device_count, count;
    struct data_object *data_object[2];
    ID3DX11DataProcessor *processor;
    D3DX11_IMAGE_INFO image_info;
    ID3DX11DataLoader *loader;
    HRESULT hr, work_item_hr;
    ID3D11Resource *resource;
    ID3DX11ThreadPump *pump;
    ID3D11Device *device;
    SYSTEM_INFO info;
    void *object;
    DWORD ret;
    int i;

    main_tid = GetCurrentThreadId();

    hr = D3DX11CreateThreadPump(1024, 0, &pump);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    hr = D3DX11CreateThreadPump(0, 1024, &pump);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    GetSystemInfo(&info);
    if (info.dwNumberOfProcessors > 1)
        hr = D3DX11CreateThreadPump(0, 0, &pump);
    else
        hr = D3DX11CreateThreadPump(0, 2, &pump);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    count = ID3DX11ThreadPump_GetWorkItemCount(pump);
    ok(!count, "GetWorkItemCount returned %u.\n", count);
    hr = ID3DX11ThreadPump_GetQueueStatus(pump, &io_count, &process_count, &device_count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!io_count, "Got unexpected io_count %u.\n", io_count);
    ok(!process_count, "Got unexpected process_count %u.\n", process_count);
    ok(!device_count, "Got unexpected device_count %u.\n", device_count);

    data_object[0] = create_data_object(E_NOTIMPL);
    data_object[1] = create_data_object(S_OK);

    data_loader_load_count = 1;
    hr = ID3DX11ThreadPump_AddWorkItem(pump, &data_object[0]->ID3DX11DataLoader_iface,
            &data_object[0]->ID3DX11DataProcessor_iface, &work_item_hr, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ret = WaitForSingleObject(data_object[0]->load_started, INFINITE);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %#lx.\n", ret);
    ok(!data_loader_load_count, "Got unexpected data_loader_load_count %ld.\n",
            data_loader_load_count);
    count = ID3DX11ThreadPump_GetWorkItemCount(pump);
    ok(count == 1, "GetWorkItemCount returned %u.\n", count);
    hr = ID3DX11ThreadPump_GetQueueStatus(pump, &io_count, &process_count, &device_count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!io_count, "Got unexpected io_count %u.\n", io_count);
    ok(!process_count, "Got unexpected process_count %u.\n", process_count);
    ok(!device_count, "Got unexpected device_count %u.\n", device_count);

    hr = ID3DX11ThreadPump_AddWorkItem(pump, &data_object[1]->ID3DX11DataLoader_iface,
            &data_object[1]->ID3DX11DataProcessor_iface, NULL, &object);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ret = WaitForSingleObject(data_object[0]->load_started, 50);
    ok(ret == WAIT_TIMEOUT, "WaitForSingleObject returned %#lx.\n", ret);
    count = ID3DX11ThreadPump_GetWorkItemCount(pump);
    ok(count == 2, "GetWorkItemCount returned %u.\n", count);
    hr = ID3DX11ThreadPump_GetQueueStatus(pump, &io_count, &process_count, &device_count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(io_count == 1, "Got unexpected io_count %u.\n", io_count);
    ok(!process_count, "Got unexpected process_count %u.\n", process_count);
    ok(!device_count, "Got unexpected device_count %u.\n", device_count);

    data_loader_load_count = 1;
    data_loader_destroy_count = 1;
    data_processor_destroy_count = 1;
    SetEvent(data_object[0]->load_done);
    ret = WaitForSingleObject(data_object[1]->load_started, INFINITE);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %#lx.\n", ret);
    ok(work_item_hr == E_NOTIMPL, "Got unexpected work_item_hr %#lx.\n", work_item_hr);
    ok(!data_loader_destroy_count, "Got unexpected data_loader_destroy_count %ld.\n",
            data_loader_destroy_count);
    ok(!data_processor_destroy_count, "Got unexpected data_processor_destroy_count %ld.\n",
            data_processor_destroy_count);
    ok(!data_loader_load_count, "Got unexpected data_loader_load_count %ld.\n",
            data_loader_load_count);

    data_loader_decompress_count = 1;
    data_processor_process_count = 1;
    SetEvent(data_object[1]->load_done);
    SetEvent(data_object[1]->decompress_done);

    data_processor_create_count = 1;
    data_loader_destroy_count = 1;
    data_processor_destroy_count = 1;
    hr = ID3DX11ThreadPump_WaitForAllItems(pump);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(object == (void *)0xdeadf00d, "Got unexpected object %p.\n", object);
    ok(!data_loader_decompress_count, "Got unexpected data_loader_decompress_count %ld.\n",
            data_loader_decompress_count);
    ok(!data_processor_process_count, "Got unexpected data_processor_process_count %ld.\n",
            data_processor_process_count);
    ok(!data_processor_create_count, "Got unexpected data_processor_create_count %ld.\n",
            data_processor_create_count);
    ok(!data_loader_destroy_count, "Got unexpected data_loader_destroy_count %ld.\n",
            data_loader_destroy_count);
    ok(!data_processor_destroy_count, "Got unexpected data_processor_destroy_count %ld.\n",
            data_processor_destroy_count);

    data_object[0] = create_data_object(S_OK);
    data_object[1] = create_data_object(S_OK);
    SetEvent(data_object[0]->load_done);
    SetEvent(data_object[1]->load_done);
    SetEvent(data_object[1]->decompress_done);

    data_loader_load_count = 2;
    data_loader_decompress_count = 2;
    data_processor_process_count = 1;
    hr = ID3DX11ThreadPump_AddWorkItem(pump, &data_object[0]->ID3DX11DataLoader_iface,
            &data_object[0]->ID3DX11DataProcessor_iface, NULL, &object);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DX11ThreadPump_AddWorkItem(pump, &data_object[1]->ID3DX11DataLoader_iface,
            &data_object[1]->ID3DX11DataProcessor_iface, NULL, &object);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    for (;;)
    {
        hr = ID3DX11ThreadPump_GetQueueStatus(pump, &io_count, &process_count, &device_count);
        if (hr != S_OK || device_count)
            break;
        Sleep(1);
    }
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!io_count, "Got unexpected io_count %u.\n", io_count);
    ok(!process_count, "Got unexpected process_count %u.\n", process_count);
    ok(device_count == 1, "Got unexpected device_count %u.\n", device_count);
    ok(!data_loader_load_count, "Got unexpected data_loader_load_count %ld.\n",
            data_loader_load_count);
    ok(!data_loader_decompress_count, "Got unexpected data_loader_decompress_count %ld.\n",
            data_loader_decompress_count);
    ok(!data_processor_process_count, "Got unexpected data_processor_process_count %ld.\n",
            data_processor_process_count);

    hr = ID3DX11ThreadPump_ProcessDeviceWorkItems(pump, 0);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DX11ThreadPump_GetQueueStatus(pump, &io_count, &process_count, &device_count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!io_count, "Got unexpected io_count %u.\n", io_count);
    ok(!process_count, "Got unexpected process_count %u.\n", process_count);
    ok(device_count == 1, "Got unexpected device_count %u.\n", device_count);

    data_processor_create_count = 1;
    data_loader_destroy_count = 1;
    data_processor_destroy_count = 1;
    hr = ID3DX11ThreadPump_ProcessDeviceWorkItems(pump, 1);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    hr = ID3DX11ThreadPump_GetQueueStatus(pump, &io_count, &process_count, &device_count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(!io_count, "Got unexpected io_count %u.\n", io_count);
    ok(!process_count, "Got unexpected process_count %u.\n", process_count);
    ok(!device_count, "Got unexpected device_count %u.\n", device_count);
    ok(!data_processor_create_count, "Got unexpected data_processor_create_count %ld.\n",
            data_processor_create_count);
    ok(!data_loader_destroy_count, "Got unexpected data_loader_destroy_count %ld.\n",
            data_loader_destroy_count);
    ok(!data_processor_destroy_count, "Got unexpected data_processor_destroy_count %ld.\n",
            data_processor_destroy_count);

    data_processor_process_count = 1;
    data_loader_destroy_count = 1;
    data_processor_destroy_count = 1;
    SetEvent(data_object[0]->decompress_done);
    hr = ID3DX11ThreadPump_PurgeAllItems(pump);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    data_processor_process_count = 0;
    ok(!data_loader_destroy_count, "Got unexpected data_loader_destroy_count %ld.\n",
            data_loader_destroy_count);
    ok(!data_processor_destroy_count, "Got unexpected data_processor_destroy_count %ld.\n",
            data_processor_destroy_count);

    device = create_device();
    if (!device)
    {
        skip("Failed to create device, skipping tests.\n");
        ID3DX11ThreadPump_Release(pump);
        return;
    }

    for (i = 0; i < ARRAY_SIZE(test_image); ++i)
    {
        winetest_push_context("Test %u", i);

        hr = D3DX11CreateAsyncMemoryLoader(test_image[i].data, test_image[i].size, &loader);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = D3DX11CreateAsyncTextureInfoProcessor(&image_info, &processor);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = ID3DX11ThreadPump_AddWorkItem(pump, loader, processor, &work_item_hr, NULL);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = ID3DX11ThreadPump_WaitForAllItems(pump);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(work_item_hr == S_OK || (work_item_hr == E_FAIL
                    && test_image[i].expected_info.ImageFileFormat == D3DX11_IFF_WMP),
                "Got unexpected hr %#lx.\n", work_item_hr);
        if (work_item_hr == S_OK)
            check_image_info(&image_info, test_image + i, __LINE__);

        hr = D3DX11CreateAsyncMemoryLoader(test_image[i].data, test_image[i].size, &loader);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = D3DX11CreateAsyncTextureProcessor(device, NULL, &processor);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = ID3DX11ThreadPump_AddWorkItem(pump, loader, processor, &work_item_hr, (void **)&resource);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        hr = ID3DX11ThreadPump_WaitForAllItems(pump);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        ok(work_item_hr == S_OK || (work_item_hr == E_FAIL
                    && test_image[i].expected_info.ImageFileFormat == D3DX11_IFF_WMP),
                "Got unexpected hr %#lx.\n", work_item_hr);
        if (work_item_hr == S_OK)
        {
            check_resource_info(resource, test_image + i, __LINE__);
            check_resource_data(resource, test_image + i, __LINE__);
            ID3D11Resource_Release(resource);
        }

        winetest_pop_context();
    }

    ok(!ID3D11Device_Release(device), "Got unexpected refcount.\n");

    ret = ID3DX11ThreadPump_Release(pump);
    ok(!ret, "Got unexpected refcount %lu.\n", ret);
}

/* dds_header.flags */
#define DDS_CAPS 0x00000001
#define DDS_HEIGHT 0x00000002
#define DDS_WIDTH 0x00000004
#define DDS_PITCH 0x00000008
#define DDS_PIXELFORMAT 0x00001000
#define DDS_MIPMAPCOUNT 0x00020000
#define DDS_LINEARSIZE 0x00080000
#define DDS_DEPTH 0x00800000

/* dds_header.caps */
#define DDSCAPS_ALPHA    0x00000002
#define DDS_CAPS_COMPLEX 0x00000008
#define DDS_CAPS_TEXTURE 0x00001000

/* dds_header.caps2 */
#define DDS_CAPS2_VOLUME  0x00200000
#define DDS_CAPS2_CUBEMAP 0x00000200
#define DDS_CAPS2_CUBEMAP_POSITIVEX 0x00000400
#define DDS_CAPS2_CUBEMAP_NEGATIVEX 0x00000800
#define DDS_CAPS2_CUBEMAP_POSITIVEY 0x00001000
#define DDS_CAPS2_CUBEMAP_NEGATIVEY 0x00002000
#define DDS_CAPS2_CUBEMAP_POSITIVEZ 0x00004000
#define DDS_CAPS2_CUBEMAP_NEGATIVEZ 0x00008000
#define DDS_CAPS2_CUBEMAP_ALL_FACES ( DDS_CAPS2_CUBEMAP_POSITIVEX | DDS_CAPS2_CUBEMAP_NEGATIVEX \
                                    | DDS_CAPS2_CUBEMAP_POSITIVEY | DDS_CAPS2_CUBEMAP_NEGATIVEY \
                                    | DDS_CAPS2_CUBEMAP_POSITIVEZ | DDS_CAPS2_CUBEMAP_NEGATIVEZ )

/* dds_pixel_format.flags */
#define DDS_PF_ALPHA 0x00000001
#define DDS_PF_ALPHA_ONLY 0x00000002
#define DDS_PF_FOURCC 0x00000004
#define DDS_PF_INDEXED 0x00000020
#define DDS_PF_RGB 0x00000040
#define DDS_PF_LUMINANCE 0x00020000
#define DDS_PF_BUMPLUMINANCE 0x00040000
#define DDS_PF_BUMPDUDV 0x00080000

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

struct dds_header_dxt10
{
    DWORD dxgi_format;
    DWORD resource_dimension;
    DWORD misc_flag;
    DWORD array_size;
    DWORD misc_flags2;
};

/* fills dds_header with reasonable default values */
static void fill_dds_header(struct dds_header *header)
{
    memset(header, 0, sizeof(*header));

    header->size = sizeof(*header);
    header->flags = DDS_CAPS | DDS_WIDTH | DDS_HEIGHT | DDS_PIXELFORMAT;
    header->height = 4;
    header->width = 4;
    header->pixel_format.size = sizeof(header->pixel_format);
    /* X8R8G8B8 */
    header->pixel_format.flags = DDS_PF_RGB;
    header->pixel_format.fourcc = 0;
    header->pixel_format.bpp = 32;
    header->pixel_format.rmask = 0xff0000;
    header->pixel_format.gmask = 0x00ff00;
    header->pixel_format.bmask = 0x0000ff;
    header->pixel_format.amask = 0;
    header->caps = DDS_CAPS_TEXTURE;
}

static void set_dxt10_dds_header(struct dds_header *header, uint32_t append_flags, uint32_t width, uint32_t height,
        uint32_t depth, uint32_t mip_levels, uint32_t pitch, uint32_t caps, uint32_t caps2)
{
    memset(header, 0, sizeof(*header));

    header->size = sizeof(*header);
    header->flags = DDS_CAPS | DDS_PIXELFORMAT | append_flags;
    header->height = height;
    header->width = width;
    header->depth = depth;
    header->miplevels = mip_levels;
    header->pitch_or_linear_size = pitch;
    header->pixel_format.size = sizeof(header->pixel_format);
    header->pixel_format.flags = DDS_PF_FOURCC;
    header->pixel_format.fourcc = MAKEFOURCC('D','X','1','0');
    header->caps = caps;
    header->caps2 = caps2;
}

static void set_dds_header_dxt10(struct dds_header_dxt10 *dxt10, DXGI_FORMAT format, uint32_t resource_dimension,
        uint32_t misc_flag, uint32_t array_size, uint32_t misc_flags2)
{
    dxt10->dxgi_format = format;
    dxt10->resource_dimension = resource_dimension;
    dxt10->misc_flag = misc_flag;
    dxt10->array_size = array_size;
    dxt10->misc_flags2 = misc_flags2;
}

#define check_dds_pixel_format(flags, fourcc, bpp, rmask, gmask, bmask, amask, format) \
        check_dds_pixel_format_(__LINE__, flags, fourcc, bpp, rmask, gmask, bmask, amask, format)
static void check_dds_pixel_format_(unsigned int line, DWORD flags, DWORD fourcc, DWORD bpp,
        DWORD rmask, DWORD gmask, DWORD bmask, DWORD amask, DXGI_FORMAT expected_format)
{
    D3DX11_IMAGE_INFO info;
    HRESULT hr;
    struct
    {
        DWORD magic;
        struct dds_header header;
        PALETTEENTRY palette[256];
        BYTE data[256];
    } dds;

    dds.magic = MAKEFOURCC('D','D','S',' ');
    fill_dds_header(&dds.header);
    dds.header.pixel_format.flags = flags;
    dds.header.pixel_format.fourcc = fourcc;
    dds.header.pixel_format.bpp = bpp;
    dds.header.pixel_format.rmask = rmask;
    dds.header.pixel_format.gmask = gmask;
    dds.header.pixel_format.bmask = bmask;
    dds.header.pixel_format.amask = amask;
    memset(dds.data, 0, sizeof(dds.data));

    hr = D3DX11GetImageInfoFromMemory(&dds, sizeof(dds), NULL, &info, NULL);
    ok_(__FILE__, line)(hr == S_OK, "Got unexpected hr %#lx for pixel format %#x.\n", hr, expected_format);
    if (SUCCEEDED(hr))
    {
        ok_(__FILE__, line)(info.Format == expected_format, "Unexpected format %#x, expected %#x\n",
                info.Format, expected_format);
    }
}

#define check_dds_pixel_format_unsupported(flags, fourcc, bpp, rmask, gmask, bmask, amask, expected_hr) \
        check_dds_pixel_format_unsupported_(__LINE__, flags, fourcc, bpp, rmask, gmask, bmask, amask, expected_hr)
static void check_dds_pixel_format_unsupported_(unsigned int line, DWORD flags, DWORD fourcc, DWORD bpp,
        DWORD rmask, DWORD gmask, DWORD bmask, DWORD amask, HRESULT expected_hr)
{
    D3DX11_IMAGE_INFO info;
    HRESULT hr;
    struct
    {
        DWORD magic;
        struct dds_header header;
        PALETTEENTRY palette[256];
        BYTE data[256];
    } dds;

    dds.magic = MAKEFOURCC('D','D','S',' ');
    fill_dds_header(&dds.header);
    dds.header.pixel_format.flags = flags;
    dds.header.pixel_format.fourcc = fourcc;
    dds.header.pixel_format.bpp = bpp;
    dds.header.pixel_format.rmask = rmask;
    dds.header.pixel_format.gmask = gmask;
    dds.header.pixel_format.bmask = bmask;
    dds.header.pixel_format.amask = amask;
    memset(dds.data, 0, sizeof(dds.data));

    hr = D3DX11GetImageInfoFromMemory(&dds, sizeof(dds), NULL, &info, NULL);
    ok_(__FILE__, line)(hr == expected_hr, "Got unexpected hr %#lx, expected %#lx.\n", hr, expected_hr);
}

#define check_dds_dx10_format(format, expected_format, wine_todo) \
        check_dds_dx10_format_(__LINE__, format, expected_format, wine_todo)
static void check_dds_dx10_format_(uint32_t line, DXGI_FORMAT format, DXGI_FORMAT expected_format, BOOL wine_todo)
{
    const uint32_t stride = (4 * get_bpp_from_format(format) + 7) / 8;
    D3DX11_IMAGE_INFO info;
    HRESULT hr;
    struct
    {
        DWORD magic;
        struct dds_header header;
        struct dds_header_dxt10 dxt10;
        BYTE data[256];
    } dds;

    dds.magic = MAKEFOURCC('D','D','S',' ');
    set_dxt10_dds_header(&dds.header, 0, 4, 4, 1, 1, stride, 0, 0);
    set_dds_header_dxt10(&dds.dxt10, format, D3D11_RESOURCE_DIMENSION_TEXTURE2D, 0, 1, 0);

    hr = D3DX11GetImageInfoFromMemory(&dds, sizeof(dds), NULL, &info, NULL);
    ok_(__FILE__, line)(hr == S_OK, "Got unexpected hr %#lx for DXGI format %#x.\n", hr, format);
    if (SUCCEEDED(hr))
    {
        todo_wine_if(wine_todo) ok_(__FILE__, line)(info.Format == expected_format, "Unexpected format %#x, expected %#x.\n",
                info.Format, expected_format);
    }
}

#define check_dds_dx10_format_unsupported(format, expected_hr) \
        check_dds_dx10_format_unsupported_(__LINE__, format, expected_hr)
static void check_dds_dx10_format_unsupported_(uint32_t line, DXGI_FORMAT format, HRESULT expected_hr)
{
    const uint32_t stride = (4 * get_bpp_from_format(format) + 7) / 8;
    D3DX11_IMAGE_INFO info;
    HRESULT hr;
    struct
    {
        DWORD magic;
        struct dds_header header;
        struct dds_header_dxt10 dxt10;
        BYTE data[256];
    } dds;

    dds.magic = MAKEFOURCC('D','D','S',' ');
    set_dxt10_dds_header(&dds.header, 0, 4, 4, 1, 1, stride, 0, 0);
    set_dds_header_dxt10(&dds.dxt10, format, D3D11_RESOURCE_DIMENSION_TEXTURE2D, 0, 1, 0);

    hr = D3DX11GetImageInfoFromMemory(&dds, sizeof(dds), NULL, &info, NULL);
    ok_(__FILE__, line)(hr == expected_hr, "Got unexpected hr %#lx for DXGI format %#x.\n", hr, format);
}

static void test_dds_header_image_info(void)
{
    struct expected
    {
        HRESULT hr;
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        uint32_t array_size;
        uint32_t mip_levels;
        uint32_t misc_flags;
        DXGI_FORMAT format;
        D3D11_RESOURCE_DIMENSION resource_dimension;
    };
    static const struct
    {
        uint32_t flags;
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        uint32_t row_pitch;
        uint32_t mip_levels;
        uint32_t caps;
        uint32_t caps2;
        struct expected expected;
        uint32_t pixel_data_size;
        BOOL todo_hr;
        BOOL todo_info;
    } tests[] = {
        /* File size validation isn't done on d3dx10. */
        { (DDS_CAPS | DDS_WIDTH | DDS_HEIGHT | DDS_PIXELFORMAT), 4, 4, 1, (4 * 4), 3, 0, 0,
          { S_OK, 4, 4, 1, 1, 3, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D, }, 0, },
        /* Depth value set to 4, but no caps bits are set. Depth is ignored. */
        { (DDS_CAPS | DDS_WIDTH | DDS_HEIGHT | DDS_PIXELFORMAT), 4, 4, 4, (4 * 4), 3, 0, 0,
          { S_OK, 4, 4, 1, 1, 3, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D, }, 292 },
        /* The volume texture caps2 field is ignored. */
        { (DDS_CAPS | DDS_WIDTH | DDS_HEIGHT | DDS_PIXELFORMAT), 4, 4, 4, (4 * 4), 3,
          (DDS_CAPS_TEXTURE | DDS_CAPS_COMPLEX), DDS_CAPS2_VOLUME,
          { S_OK, 4, 4, 1, 1, 3, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D, }, 292 },
        /*
         * The DDS_DEPTH flag is the only thing checked to determine if a DDS
         * file represents a 3D texture.
         */
        { (DDS_CAPS | DDS_WIDTH | DDS_HEIGHT | DDS_PIXELFORMAT | DDS_DEPTH), 4, 4, 4, (4 * 4), 3,
          0, 0,
          { S_OK, 4, 4, 4, 1, 3, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE3D, }, 292 },
        /* Even if the depth field is set to 0, it's still a 3D texture. */
        { (DDS_CAPS | DDS_WIDTH | DDS_HEIGHT | DDS_PIXELFORMAT | DDS_DEPTH), 4, 4, 0, (4 * 4), 3,
          0, 0,
          { S_OK, 4, 4, 1, 1, 3, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE3D, }, 292 },
        /* The DDS_DEPTH flag overrides cubemap caps. */
        { (DDS_CAPS | DDS_WIDTH | DDS_HEIGHT | DDS_PIXELFORMAT | DDS_DEPTH), 4, 4, 4, (4 * 4), 3,
          (DDS_CAPS_TEXTURE | DDS_CAPS_COMPLEX), (DDS_CAPS2_CUBEMAP | DDS_CAPS2_CUBEMAP_ALL_FACES),
          { S_OK, 4, 4, 4, 1, 3, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE3D, }, 292 },
        /* Cubemap where width field does not equal height. */
        { (DDS_CAPS | DDS_WIDTH | DDS_HEIGHT | DDS_PIXELFORMAT), 4, 5, 1, (4 * 4), 1,
          (DDS_CAPS_TEXTURE | DDS_CAPS_COMPLEX), (DDS_CAPS2_CUBEMAP | DDS_CAPS2_CUBEMAP_ALL_FACES),
          { S_OK, 4, 5, 1, 6, 1, D3D11_RESOURCE_MISC_TEXTURECUBE, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D, }, (80 * 6) },
        /* Partial cubemaps are not supported. */
        { (DDS_CAPS | DDS_WIDTH | DDS_HEIGHT | DDS_PIXELFORMAT), 4, 4, 1, (4 * 4), 1,
          (DDS_CAPS_TEXTURE | DDS_CAPS_COMPLEX), (DDS_CAPS2_CUBEMAP | DDS_CAPS2_CUBEMAP_POSITIVEX),
          { E_FAIL, }, (64 * 6), },
    };
    static const struct
    {
        uint32_t append_flags;
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        uint32_t row_pitch;
        uint32_t mip_levels;
        uint32_t caps;
        uint32_t caps2;
        struct dds_header_dxt10 dxt10;
        struct expected expected;
        uint32_t pixel_data_size;
        BOOL todo_hr;
        BOOL todo_info;
    } dxt10_tests[] = {
        /* File size validation isn't done on d3dx10. */
        { 0, 4, 4, 0, (4 * 4), 1, 0, 0,
          { DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D, 0, 1, 0, },
          { S_OK, 4, 4, 1, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D, }, 0, },
        /*
         * Setting the misc_flags2 field to anything other than 0 results in
         * E_FAIL.
         */
        { 0, 4, 4, 0, (4 * 4), 1, 0, 0,
          { DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D, 0, 1, 1, },
          { E_FAIL }, (4 * 4 * 4), },
        /*
         * The misc_flags field isn't passed through directly, only the
         * cube texture flag is (if it's set).
         */
        { 0, 4, 4, 0, (4 * 4), 1, 0, 0,
          { DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D, 0xfffffffb, 1, 0, },
          { S_OK, 4, 4, 1, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D, }, (4 * 4 * 4) },
        /* Resource dimension field of the header isn't validated. */
        { 0, 4, 4, 0, (4 * 4), 1, 0, 0,
          { DXGI_FORMAT_R8G8B8A8_UNORM, 500, 0, 1, 0, },
          { S_OK, 4, 4, 1, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM, 500, }, (4 * 4 * 4), .todo_hr = TRUE },
        /* Depth value of 2, but D3D11_RESOURCE_DIMENSION_TEXTURE2D. */
        { DDS_DEPTH, 4, 4, 2, (4 * 4), 1, 0, 0,
          { DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D, 0, 1, 0, },
          { S_OK, 4, 4, 2, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D, }, (4 * 4 * 4 * 2) },
        /* Depth field value is ignored if DDS_DEPTH isn't set. */
        { 0, 4, 4, 2, (4 * 4), 1, 0, 0,
          { DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE3D, 0, 1, 0, },
          { S_OK, 4, 4, 1, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE3D, }, (4 * 4 * 4 * 2), },
        /*
         * 3D texture with an array size larger than 1. Technically there's no
         * such thing as a 3D texture array, but it succeeds.
         */
        { DDS_DEPTH, 4, 4, 2, (4 * 4), 1, 0, 0,
          { DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE3D, 0, 2, 0, },
          { S_OK, 4, 4, 2, 2, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE3D, }, (4 * 4 * 4 * 2 * 2) },
        /* Cubemap caps are ignored for DXT10 files. */
        { 0, 4, 4, 1, (4 * 4), 1, 0, DDS_CAPS2_CUBEMAP | DDS_CAPS2_CUBEMAP_ALL_FACES,
          { DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D, 0, 1, 0, },
          { S_OK, 4, 4, 1, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D }, (4 * 4 * 4 * 6) },
        /* Array size value is multiplied by 6 for cubemap files. */
        { 0, 4, 4, 1, (4 * 4), 1, 0, 0,
          { DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D, DDS_RESOURCE_MISC_TEXTURECUBE, 2, 0, },
          { S_OK, 4, 4, 1, 12, 1, D3D11_RESOURCE_MISC_TEXTURECUBE, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D }, (4 * 4 * 4 * 12) },
        /* Resource dimension is validated for cube textures. */
        { 0, 4, 4, 1, (4 * 4), 1, 0, 0,
          { DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE3D, DDS_RESOURCE_MISC_TEXTURECUBE, 2, 0, },
          { E_FAIL }, (4 * 4 * 4 * 12), },
        /* 1D Texture cube, invalid. */
        { 0, 4, 4, 1, (4 * 4), 1, 0, 0,
          { DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE1D, DDS_RESOURCE_MISC_TEXTURECUBE, 2, 0, },
          { E_FAIL }, (4 * 4 * 4 * 12), },
    };
    D3DX11_IMAGE_INFO info;
    uint32_t i, file_size;
    struct
    {
        DWORD magic;
        struct dds_header header;
        struct dds_header_dxt10 dxt10;
    } dds;
    HRESULT hr;

    for (i = 0; i < ARRAY_SIZE(tests); ++i)
    {
        winetest_push_context("Test %u", i);
        file_size = sizeof(dds.magic) + sizeof(dds.header) + tests[i].pixel_data_size;

        dds.magic = MAKEFOURCC('D','D','S',' ');
        fill_dds_header(&dds.header);
        dds.header.flags = tests[i].flags;
        dds.header.width = tests[i].width;
        dds.header.height = tests[i].height;
        dds.header.depth = tests[i].depth;
        dds.header.pitch_or_linear_size = tests[i].row_pitch;
        dds.header.miplevels = tests[i].mip_levels;
        dds.header.caps = tests[i].caps;
        dds.header.caps2 = tests[i].caps2;

        memset(&info, 0, sizeof(info));
        hr = D3DX11GetImageInfoFromMemory(&dds, file_size, NULL, &info, NULL);
        todo_wine_if(tests[i].todo_hr) ok(hr == tests[i].expected.hr, "Got unexpected hr %#lx, expected %#lx.\n",
                hr, tests[i].expected.hr);
        if (SUCCEEDED(hr) && SUCCEEDED(tests[i].expected.hr))
            check_image_info_values(&info, tests[i].expected.width, tests[i].expected.height,
                    tests[i].expected.depth, tests[i].expected.array_size, tests[i].expected.mip_levels,
                    tests[i].expected.misc_flags, tests[i].expected.format,
                    tests[i].expected.resource_dimension, D3DX11_IFF_DDS, tests[i].todo_info);

        winetest_pop_context();
    }

    for (i = 0; i < ARRAY_SIZE(dxt10_tests); ++i)
    {
        winetest_push_context("Test %u", i);
        file_size = sizeof(dds) + dxt10_tests[i].pixel_data_size;

        dds.magic = MAKEFOURCC('D','D','S',' ');
        set_dxt10_dds_header(&dds.header, dxt10_tests[i].append_flags, dxt10_tests[i].width, dxt10_tests[i].height,
                dxt10_tests[i].depth, dxt10_tests[i].mip_levels, dxt10_tests[i].row_pitch, dxt10_tests[i].caps, dxt10_tests[i].caps2);
        dds.dxt10 = dxt10_tests[i].dxt10;

        memset(&info, 0, sizeof(info));
        hr = D3DX11GetImageInfoFromMemory(&dds, file_size, NULL, &info, NULL);
        todo_wine_if(dxt10_tests[i].todo_hr) ok(hr == dxt10_tests[i].expected.hr, "Got unexpected hr %#lx, expected %#lx.\n",
                hr, dxt10_tests[i].expected.hr);
        if (SUCCEEDED(hr) && SUCCEEDED(dxt10_tests[i].expected.hr))
            check_image_info_values(&info, dxt10_tests[i].expected.width, dxt10_tests[i].expected.height,
                    dxt10_tests[i].expected.depth, dxt10_tests[i].expected.array_size, dxt10_tests[i].expected.mip_levels,
                    dxt10_tests[i].expected.misc_flags, dxt10_tests[i].expected.format,
                    dxt10_tests[i].expected.resource_dimension, D3DX11_IFF_DDS, dxt10_tests[i].todo_info);

        winetest_pop_context();
    }

    /*
     * Image size (e.g, the size of the pixels) isn't validated, but header
     * size is.
     */
    dds.magic = MAKEFOURCC('D','D','S',' ');
    set_dxt10_dds_header(&dds.header, dxt10_tests[0].append_flags, dxt10_tests[0].width, dxt10_tests[0].height,
            dxt10_tests[0].depth, dxt10_tests[0].mip_levels, dxt10_tests[0].row_pitch, dxt10_tests[0].caps, dxt10_tests[0].caps2);
    dds.dxt10 = dxt10_tests[0].dxt10;

    hr = D3DX11GetImageInfoFromMemory(&dds, sizeof(dds) - 1, NULL, &info, NULL);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    hr = D3DX11GetImageInfoFromMemory(&dds, sizeof(dds), NULL, &info, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* Non DXT10 header. */
    dds.magic = MAKEFOURCC('D','D','S',' ');
    fill_dds_header(&dds.header);

    hr = D3DX11GetImageInfoFromMemory(&dds, sizeof(dds) - sizeof(dds.dxt10) - 1, NULL, &info, NULL);
    ok(hr == E_FAIL, "Unexpected hr %#lx.\n", hr);

    hr = D3DX11GetImageInfoFromMemory(&dds, sizeof(dds) - sizeof(dds.dxt10), NULL, &info, NULL);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
}

static void test_get_image_info(void)
{
    static const WCHAR test_resource_name[] = L"resource.data";
    static const WCHAR test_filename[] = L"image.data";
    HMODULE resource_module;
    D3DX11_IMAGE_INFO info;
    WCHAR path[MAX_PATH];
    HRESULT hr, hr2;
    uint32_t i;

    CoInitialize(NULL);

    hr = D3DX11GetImageInfoFromMemory(bmp_1bpp, sizeof(bmp_1bpp), NULL, &info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11GetImageInfoFromMemory(bmp_1bpp, sizeof(bmp_1bpp) + 5, NULL, &info, NULL); /* too large size */
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11GetImageInfoFromMemory(noimage, sizeof(noimage), NULL, NULL, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11GetImageInfoFromMemory(noimage, sizeof(noimage), NULL, &info, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11GetImageInfoFromMemory(bmp_1bpp, sizeof(bmp_1bpp) - 1, NULL, &info, NULL);
    todo_wine ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11GetImageInfoFromMemory(bmp_1bpp + 1, sizeof(bmp_1bpp) - 1, NULL, &info, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11GetImageInfoFromMemory(bmp_1bpp, 0, NULL, &info, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11GetImageInfoFromMemory(bmp_1bpp, 0, NULL, NULL, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11GetImageInfoFromMemory(noimage, 0, NULL, &info, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11GetImageInfoFromMemory(noimage, 0, NULL, NULL, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11GetImageInfoFromMemory(noimage, 0, NULL, &info, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11GetImageInfoFromMemory(NULL, 4, NULL, NULL, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11GetImageInfoFromMemory(NULL, 4, NULL, &info, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    hr = D3DX11GetImageInfoFromMemory(NULL, 0, NULL, NULL, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    /* test BMP support */
    memset(&info, 0, sizeof(info));
    hr = D3DX11GetImageInfoFromMemory(bmp_1bpp, sizeof(bmp_1bpp), NULL, &info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_image_info_values(&info, 1, 1, 1, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D,
            D3DX11_IFF_BMP, FALSE);

    hr = D3DX11GetImageInfoFromMemory(bmp_2bpp, sizeof(bmp_2bpp), NULL, &info, NULL);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);

    memset(&info, 0, sizeof(info));
    hr = D3DX11GetImageInfoFromMemory(bmp_4bpp, sizeof(bmp_4bpp), NULL, &info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_image_info_values(&info, 1, 1, 1, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D,
            D3DX11_IFF_BMP, FALSE);

    memset(&info, 0, sizeof(info));
    hr = D3DX11GetImageInfoFromMemory(bmp_8bpp, sizeof(bmp_8bpp), NULL, &info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_image_info_values(&info, 1, 1, 1, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D,
            D3DX11_IFF_BMP, FALSE);

    memset(&info, 0, sizeof(info));
    hr = D3DX11GetImageInfoFromMemory(bmp_32bpp_xrgb, sizeof(bmp_32bpp_xrgb), NULL, &info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_image_info_values(&info, 2, 2, 1, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D,
            D3DX11_IFF_BMP, FALSE);

    memset(&info, 0, sizeof(info));
    hr = D3DX11GetImageInfoFromMemory(bmp_32bpp_argb, sizeof(bmp_32bpp_argb), NULL, &info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_image_info_values(&info, 2, 2, 1, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D,
            D3DX11_IFF_BMP, FALSE);

    /* Grayscale PNG */
    memset(&info, 0, sizeof(info));
    hr = D3DX11GetImageInfoFromMemory(png_grayscale, sizeof(png_grayscale), NULL, &info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_image_info_values(&info, 1, 1, 1, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D,
            D3DX11_IFF_PNG, FALSE);

    /* test DDS support */
    memset(&info, 0, sizeof(info));
    hr = D3DX11GetImageInfoFromMemory(dds_24bit, sizeof(dds_24bit), NULL, &info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_image_info_values(&info, 2, 2, 1, 1, 2, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D,
            D3DX11_IFF_DDS, FALSE);

    memset(&info, 0, sizeof(info));
    hr = D3DX11GetImageInfoFromMemory(dds_24bit, sizeof(dds_24bit) - 1, NULL, &info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_image_info_values(&info, 2, 2, 1, 1, 2, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D,
            D3DX11_IFF_DDS, FALSE);

    memset(&info, 0, sizeof(info));
    hr = D3DX11GetImageInfoFromMemory(dds_16bit, sizeof(dds_16bit), NULL, &info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_image_info_values(&info, 2, 2, 1, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D,
            D3DX11_IFF_DDS, FALSE);

    memset(&info, 0, sizeof(info));
    hr = D3DX11GetImageInfoFromMemory(dds_16bit, sizeof(dds_16bit) - 1, NULL, &info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_image_info_values(&info, 2, 2, 1, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D,
            D3DX11_IFF_DDS, FALSE);

    memset(&info, 0, sizeof(info));
    hr = D3DX11GetImageInfoFromMemory(dds_8bit, sizeof(dds_8bit), NULL, &info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_image_info_values(&info, 16, 4, 1, 1, 1, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D,
            D3DX11_IFF_DDS, FALSE);

    memset(&info, 0, sizeof(info));
    hr = D3DX11GetImageInfoFromMemory(dds_cube_map, sizeof(dds_cube_map), NULL, &info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_image_info_values(&info, 4, 4, 1, 6, 1, D3D11_RESOURCE_MISC_TEXTURECUBE, DXGI_FORMAT_BC3_UNORM,
            D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS, FALSE);

    memset(&info, 0, sizeof(info));
    hr = D3DX11GetImageInfoFromMemory(dds_cube_map, sizeof(dds_cube_map) - 1, NULL, &info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_image_info_values(&info, 4, 4, 1, 6, 1, D3D11_RESOURCE_MISC_TEXTURECUBE, DXGI_FORMAT_BC3_UNORM,
            D3D11_RESOURCE_DIMENSION_TEXTURE2D, D3DX11_IFF_DDS, FALSE);

    memset(&info, 0, sizeof(info));
    hr = D3DX11GetImageInfoFromMemory(dds_volume_map, sizeof(dds_volume_map), NULL, &info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_image_info_values(&info, 4, 4, 2, 1, 3, 0, DXGI_FORMAT_BC2_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE3D,
            D3DX11_IFF_DDS, FALSE);

    hr = D3DX11GetImageInfoFromMemory(dds_volume_map, sizeof(dds_volume_map) - 1, NULL, &info, NULL);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_image_info_values(&info, 4, 4, 2, 1, 3, 0, DXGI_FORMAT_BC2_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE3D,
            D3DX11_IFF_DDS, FALSE);

    for (i = 0; i < ARRAY_SIZE(test_image); ++i)
    {
        winetest_push_context("Test %u", i);

        hr2 = 0xdeadbeef;
        hr = D3DX11GetImageInfoFromMemory(test_image[i].data, test_image[i].size, NULL, &info, &hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX11_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        if (hr == S_OK)
            check_image_info(&info, test_image + i, __LINE__);

        winetest_pop_context();
    }

    hr2 = 0xdeadbeef;
    add_work_item_count = 0;
    hr = D3DX11GetImageInfoFromMemory(test_image[0].data, test_image[0].size, &thread_pump, &info, &hr2);
    ok(add_work_item_count == 1, "Got unexpected add_work_item_count %u.\n", add_work_item_count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
    check_image_info(&info, test_image, __LINE__);

    hr2 = 0xdeadbeef;
    hr = D3DX11GetImageInfoFromFileW(NULL, NULL, &info, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX11GetImageInfoFromFileW(L"deadbeaf", NULL, &info, &hr2);
    ok(hr == D3D11_ERROR_FILE_NOT_FOUND, "Got unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX11GetImageInfoFromFileA(NULL, NULL, &info, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX11GetImageInfoFromFileA("deadbeaf", NULL, &info, &hr2);
    ok(hr == D3D11_ERROR_FILE_NOT_FOUND, "Got unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);

    for (i = 0; i < ARRAY_SIZE(test_image); ++i)
    {
        winetest_push_context("Test %u", i);
        create_file(test_filename, test_image[i].data, test_image[i].size, path);

        hr2 = 0xdeadbeef;
        hr = D3DX11GetImageInfoFromFileW(path, NULL, &info, &hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX11_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        if (hr == S_OK)
            check_image_info(&info, test_image + i, __LINE__);

        hr2 = 0xdeadbeef;
        hr = D3DX11GetImageInfoFromFileA(get_str_a(path), NULL, &info, &hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX11_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        if (hr == S_OK)
            check_image_info(&info, test_image + i, __LINE__);

        delete_file(test_filename);
        winetest_pop_context();
    }

    /* D3DX11GetImageInfoFromResource tests */
    hr2 = 0xdeadbeef;
    hr = D3DX11GetImageInfoFromResourceW(NULL, NULL, NULL, &info, &hr2);
    ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX11GetImageInfoFromResourceW(NULL, L"deadbeaf", NULL, &info, &hr2);
    ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX11GetImageInfoFromResourceA(NULL, NULL, NULL, &info, &hr2);
    ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX11GetImageInfoFromResourceA(NULL, "deadbeaf", NULL, &info, &hr2);
    ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);

    for (i = 0; i < ARRAY_SIZE(test_image); ++i)
    {
        winetest_push_context("Test %u", i);
        resource_module = create_resource_module(test_resource_name, test_image[i].data, test_image[i].size);

        hr2 = 0xdeadbeef;
        hr = D3DX11GetImageInfoFromResourceW(resource_module, L"deadbeef", NULL, &info, &hr2);
        ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
        ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);

        hr2 = 0xdeadbeef;
        hr = D3DX11GetImageInfoFromResourceW(resource_module, test_resource_name, NULL, &info, &hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX11_IFF_WMP)
                || broken(hr == D3DX11_ERR_INVALID_DATA) /* Vista */,
                "Got unexpected hr %#lx.\n", hr);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        if (hr == S_OK)
            check_image_info(&info, test_image + i, __LINE__);

        hr2 = 0xdeadbeef;
        hr = D3DX11GetImageInfoFromResourceA(resource_module, get_str_a(test_resource_name), NULL, &info, &hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX11_IFF_WMP)
                || broken(hr == D3DX11_ERR_INVALID_DATA) /* Vista */,
                "Got unexpected hr %#lx.\n", hr);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        if (hr == S_OK)
            check_image_info(&info, test_image + i, __LINE__);

        delete_resource_module(test_resource_name, resource_module);
        winetest_pop_context();
    }

    check_dds_pixel_format(DDS_PF_FOURCC, MAKEFOURCC('D','X','T','1'), 0, 0, 0, 0, 0, DXGI_FORMAT_BC1_UNORM);
    check_dds_pixel_format(DDS_PF_FOURCC, MAKEFOURCC('D','X','T','2'), 0, 0, 0, 0, 0, DXGI_FORMAT_BC2_UNORM);
    check_dds_pixel_format(DDS_PF_FOURCC, MAKEFOURCC('D','X','T','3'), 0, 0, 0, 0, 0, DXGI_FORMAT_BC2_UNORM);
    check_dds_pixel_format(DDS_PF_FOURCC, MAKEFOURCC('D','X','T','4'), 0, 0, 0, 0, 0, DXGI_FORMAT_BC3_UNORM);
    check_dds_pixel_format(DDS_PF_FOURCC, MAKEFOURCC('D','X','T','5'), 0, 0, 0, 0, 0, DXGI_FORMAT_BC3_UNORM);
    check_dds_pixel_format(DDS_PF_FOURCC, MAKEFOURCC('R','G','B','G'), 0, 0, 0, 0, 0, DXGI_FORMAT_R8G8_B8G8_UNORM);
    check_dds_pixel_format(DDS_PF_FOURCC, MAKEFOURCC('G','R','G','B'), 0, 0, 0, 0, 0, DXGI_FORMAT_G8R8_G8B8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB, 0, 16, 0xf800, 0x07e0, 0x001f, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB | DDS_PF_ALPHA, 0, 16, 0x7c00, 0x03e0, 0x001f, 0x8000, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB | DDS_PF_ALPHA, 0, 16, 0x0f00, 0x00f0, 0x000f, 0xf000, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB, 0, 8, 0xe0, 0x1c, 0x03, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_ALPHA_ONLY, 0, 8, 0, 0, 0, 0xff, DXGI_FORMAT_A8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB | DDS_PF_ALPHA, 0, 16, 0x00e0, 0x001c, 0x0003, 0xff00, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB, 0, 16, 0xf00, 0x0f0, 0x00f, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000, DXGI_FORMAT_R10G10B10A2_UNORM);
    check_dds_pixel_format(DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x000003ff, 0x000ffc00, 0x3ff00000, 0xc0000000, DXGI_FORMAT_R10G10B10A2_UNORM);
    check_dds_pixel_format(DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB | DDS_PF_ALPHA, 0, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB, 0, 32, 0xff0000, 0x00ff00, 0x0000ff, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB, 0, 32, 0x0000ff, 0x00ff00, 0xff0000, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB, 0, 24, 0xff0000, 0x00ff00, 0x0000ff, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_RGB, 0, 32, 0x0000ffff, 0xffff0000, 0, 0, DXGI_FORMAT_R16G16_UNORM);
    check_dds_pixel_format(DDS_PF_LUMINANCE, 0, 8, 0xff, 0, 0, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_LUMINANCE, 0, 16, 0xffff, 0, 0, 0, DXGI_FORMAT_R16G16B16A16_UNORM);
    check_dds_pixel_format(DDS_PF_LUMINANCE | DDS_PF_ALPHA, 0, 16, 0x00ff, 0, 0, 0xff00, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_LUMINANCE | DDS_PF_ALPHA, 0, 8, 0x0f, 0, 0, 0xf0, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_INDEXED, 0, 8, 0, 0, 0, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_INDEXED | DDS_PF_ALPHA, 0, 16, 0, 0, 0, 0xff00, DXGI_FORMAT_R8G8B8A8_UNORM);
    check_dds_pixel_format(DDS_PF_FOURCC, 0x24, 0, 0, 0, 0, 0, DXGI_FORMAT_R16G16B16A16_UNORM); /* D3DFMT_A16B16G16R16 */
    check_dds_pixel_format(DDS_PF_FOURCC, 0x6e, 0, 0, 0, 0, 0, DXGI_FORMAT_R16G16B16A16_SNORM); /* D3DFMT_Q16W16V16U16 */
    check_dds_pixel_format(DDS_PF_FOURCC, 0x6f, 0, 0, 0, 0, 0, DXGI_FORMAT_R16_FLOAT); /* D3DFMT_R16F */
    check_dds_pixel_format(DDS_PF_FOURCC, 0x70, 0, 0, 0, 0, 0, DXGI_FORMAT_R16G16_FLOAT); /* D3DFMT_G16R16F */
    check_dds_pixel_format(DDS_PF_FOURCC, 0x71, 0, 0, 0, 0, 0, DXGI_FORMAT_R16G16B16A16_FLOAT); /* D3DFMT_A16B16G16R16F */
    check_dds_pixel_format(DDS_PF_FOURCC, 0x72, 0, 0, 0, 0, 0, DXGI_FORMAT_R32_FLOAT); /* D3DFMT_R32F */
    check_dds_pixel_format(DDS_PF_FOURCC, 0x73, 0, 0, 0, 0, 0, DXGI_FORMAT_R32G32_FLOAT); /* D3DFMT_G32R32F */
    check_dds_pixel_format(DDS_PF_FOURCC, 0x74, 0, 0, 0, 0, 0, DXGI_FORMAT_R32G32B32A32_FLOAT); /* D3DFMT_A32B32G32R32F */

    /* Test for DDS pixel formats that are valid on d3dx9, but not d3dx10. */
    check_dds_pixel_format_unsupported(DDS_PF_FOURCC, MAKEFOURCC('U','Y','V','Y'), 0, 0, 0, 0, 0, E_FAIL);
    check_dds_pixel_format_unsupported(DDS_PF_FOURCC, MAKEFOURCC('Y','U','Y','2'), 0, 0, 0, 0, 0, E_FAIL);
    /* Bumpmap formats aren't supported. */
    check_dds_pixel_format_unsupported(DDS_PF_BUMPDUDV, 0, 16, 0x00ff, 0xff00, 0, 0, E_FAIL);
    check_dds_pixel_format_unsupported(DDS_PF_BUMPDUDV, 0, 32, 0x0000ffff, 0xffff0000, 0, 0, E_FAIL);
    check_dds_pixel_format_unsupported(DDS_PF_BUMPDUDV, 0, 32, 0xff, 0xff00, 0x00ff0000, 0xff000000, E_FAIL);
    check_dds_pixel_format_unsupported(DDS_PF_BUMPLUMINANCE, 0, 32, 0x0000ff, 0x00ff00, 0xff0000, 0, E_FAIL);

    /* Newer fourCC formats. */
    check_dds_pixel_format(DDS_PF_FOURCC, MAKEFOURCC('B','C','4','U'), 0, 0, 0, 0, 0, DXGI_FORMAT_BC4_UNORM);
    check_dds_pixel_format(DDS_PF_FOURCC, MAKEFOURCC('B','C','5','U'), 0, 0, 0, 0, 0, DXGI_FORMAT_BC5_UNORM);
    check_dds_pixel_format(DDS_PF_FOURCC, MAKEFOURCC('B','C','4','S'), 0, 0, 0, 0, 0, DXGI_FORMAT_BC4_SNORM);
    check_dds_pixel_format(DDS_PF_FOURCC, MAKEFOURCC('B','C','5','S'), 0, 0, 0, 0, 0, DXGI_FORMAT_BC5_SNORM);

    check_dds_dx10_format_unsupported(DXGI_FORMAT_B5G6R5_UNORM, E_FAIL);
    check_dds_dx10_format_unsupported(DXGI_FORMAT_B5G5R5A1_UNORM, E_FAIL);
    check_dds_dx10_format_unsupported(DXGI_FORMAT_B4G4R4A4_UNORM, E_FAIL);

    /*
     * These formats should map 1:1 from the DXT10 header, unlike legacy DDS
     * file equivalents.
     */
    check_dds_dx10_format(DXGI_FORMAT_R8_UNORM, DXGI_FORMAT_R8_UNORM, FALSE);
    check_dds_dx10_format(DXGI_FORMAT_R16_UNORM, DXGI_FORMAT_R16_UNORM, FALSE);
    check_dds_dx10_format(DXGI_FORMAT_R8G8_UNORM, DXGI_FORMAT_R8G8_UNORM, FALSE);
    check_dds_dx10_format(DXGI_FORMAT_B8G8R8X8_UNORM, DXGI_FORMAT_B8G8R8X8_UNORM, FALSE);
    check_dds_dx10_format(DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_FORMAT_B8G8R8A8_UNORM, FALSE);
    /* Formats unsupported on d3dx10, but now supported on d3dx11. */
    todo_wine check_dds_dx10_format(DXGI_FORMAT_BC6H_UF16, DXGI_FORMAT_BC6H_UF16, FALSE);
    todo_wine check_dds_dx10_format(DXGI_FORMAT_BC6H_SF16, DXGI_FORMAT_BC6H_SF16, FALSE);
    todo_wine check_dds_dx10_format(DXGI_FORMAT_BC7_UNORM, DXGI_FORMAT_BC7_UNORM, FALSE);

    test_dds_header_image_info();

    CoUninitialize();
}

static void test_create_texture(void)
{
    static const uint32_t dds_24bit_8_8_mip_level_expected[] = { 0xff0000ff, 0xff00ff00, 0xffff0000, 0xff000000 };
    static const WCHAR test_resource_name[] = L"resource.data";
    static const WCHAR test_filename[] = L"image.data";
    D3D11_TEXTURE2D_DESC tex_2d_desc;
    D3DX11_IMAGE_LOAD_INFO load_info;
    D3DX11_IMAGE_INFO img_info;
    ID3D11Resource *resource;
    ID3D11Texture2D *tex_2d;
    HMODULE resource_module;
    ID3D11Device *device;
    uint32_t i, mip_level;
    WCHAR path[MAX_PATH];
    HRESULT hr, hr2;

    device = create_device();
    if (!device)
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    CoInitialize(NULL);

    /* D3DX11CreateTextureFromMemory tests */
    resource = (ID3D11Resource *)0xdeadbeef;
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateTextureFromMemory(NULL, test_bmp_1bpp, sizeof(test_bmp_1bpp), NULL, NULL, &resource, &hr2);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    ok(resource == (ID3D11Resource *)0xdeadbeef, "Got unexpected resource %p.\n", resource);

    resource = (ID3D11Resource *)0xdeadbeef;
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateTextureFromMemory(device, NULL, 0, NULL, NULL, &resource, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    ok(resource == (ID3D11Resource *)0xdeadbeef, "Got unexpected resource %p.\n", resource);

    resource = (ID3D11Resource *)0xdeadbeef;
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateTextureFromMemory(device, NULL, sizeof(test_bmp_1bpp), NULL, NULL, &resource, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    ok(resource == (ID3D11Resource *)0xdeadbeef, "Got unexpected resource %p.\n", resource);

    resource = (ID3D11Resource *)0xdeadbeef;
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateTextureFromMemory(device, test_bmp_1bpp, 0, NULL, NULL, &resource, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
    ok(resource == (ID3D11Resource *)0xdeadbeef, "Got unexpected resource %p.\n", resource);

    resource = (ID3D11Resource *)0xdeadbeef;
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateTextureFromMemory(device, test_bmp_1bpp, sizeof(test_bmp_1bpp) - 1, NULL, NULL, &resource, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
    ok(resource == (ID3D11Resource *)0xdeadbeef, "Got unexpected resource %p.\n", resource);

    for (i = 0; i < ARRAY_SIZE(test_image); ++i)
    {
        winetest_push_context("Test %u", i);

        hr2 = 0xdeadbeef;
        hr = D3DX11CreateTextureFromMemory(device, test_image[i].data, test_image[i].size, NULL, NULL, &resource, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX11_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        if (hr == S_OK)
        {
            check_resource_info(resource, test_image + i, __LINE__);
            check_resource_data(resource, test_image + i, __LINE__);
            ID3D11Resource_Release(resource);
        }

        winetest_pop_context();
    }

    for (i = 0; i < ARRAY_SIZE(test_invalid_image_load_info); ++i)
    {
        const struct test_invalid_image_load_info *test_load_info = &test_invalid_image_load_info[i];

        winetest_push_context("Test %u", i);

        hr2 = 0xdeadbeef;
        load_info = test_load_info->load_info;
        hr = D3DX11CreateTextureFromMemory(device, test_load_info->data, test_load_info->size, &load_info, NULL, &resource, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        todo_wine_if(test_load_info->todo_hr) ok(hr == test_load_info->expected_hr, "Got unexpected hr %#lx.\n", hr);
        if (SUCCEEDED(hr))
            ID3D11Resource_Release(resource);

        winetest_pop_context();
    }

    for (i = 0; i < ARRAY_SIZE(test_image_load_info); ++i)
    {
        const struct test_image_load_info *test_load_info = &test_image_load_info[i];

        winetest_push_context("Test %u", i);

        load_info = test_load_info->load_info;
        load_info.pSrcInfo = &img_info;

        resource = NULL;
        hr2 = 0xdeadbeef;
        hr = D3DX11CreateTextureFromMemory(device, test_load_info->data, test_load_info->size, &load_info, NULL, &resource, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        ok(hr == test_load_info->expected_hr, "Got unexpected hr %#lx.\n", hr);
        if (SUCCEEDED(hr))
        {
            check_test_image_load_info_resource(resource, test_load_info);
            ID3D11Resource_Release(resource);
        }

        winetest_pop_context();
    }

    /* Check behavior of the FirstMipLevel argument. */
    for (i = 0; i < 2; ++i)
    {
        winetest_push_context("FirstMipLevel %u", i);
        memset(&img_info, 0, sizeof(img_info));
        set_d3dx11_image_load_info(&load_info, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, i, D3DX11_FROM_FILE,
                D3D11_USAGE_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT,
                D3DX11_DEFAULT, &img_info);

        resource = NULL;
        hr2 = 0xdeadbeef;
        hr = D3DX11CreateTextureFromMemory(device, dds_24bit_8_8, sizeof(dds_24bit_8_8), &load_info, NULL, &resource, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        check_image_info_values(&img_info, 8, 8, 1, 1, 4, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D,
                D3DX11_IFF_DDS, FALSE);

        hr = ID3D11Resource_QueryInterface(resource, &IID_ID3D11Texture2D, (void **)&tex_2d);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        ID3D11Texture2D_GetDesc(tex_2d, &tex_2d_desc);
        check_texture2d_desc_values(&tex_2d_desc, 8, 8, 4, 1, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 0, D3D11_USAGE_DEFAULT,
                D3D11_BIND_SHADER_RESOURCE, 0, 0, FALSE);
        for (mip_level = 0; mip_level < 4; ++mip_level)
        {
            winetest_push_context("MipLevel %u", mip_level);
            check_texture_sub_resource_color(tex_2d, mip_level, NULL,
                    dds_24bit_8_8_mip_level_expected[min(3, mip_level + i)], 0);
            winetest_pop_context();
        }

        ID3D11Texture2D_Release(tex_2d);
        ID3D11Resource_Release(resource);
        winetest_pop_context();
    }

    /*
     * If FirstMipLevel is set to a value that is larger than the total number
     * of mip levels in the image, it falls back to 0.
     */
    memset(&img_info, 0, sizeof(img_info));
    set_d3dx11_image_load_info(&load_info, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, 5, D3DX11_FROM_FILE,
            D3D11_USAGE_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT,
            D3DX11_DEFAULT, &img_info);

    resource = NULL;
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateTextureFromMemory(device, dds_24bit_8_8, sizeof(dds_24bit_8_8), &load_info, NULL, &resource, &hr2);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_image_info_values(&img_info, 8, 8, 1, 1, 4, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D,
            D3DX11_IFF_DDS, FALSE);

    hr = ID3D11Resource_QueryInterface(resource, &IID_ID3D11Texture2D, (void **)&tex_2d);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID3D11Texture2D_GetDesc(tex_2d, &tex_2d_desc);
    check_texture2d_desc_values(&tex_2d_desc, 8, 8, 4, 1, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 0, D3D11_USAGE_DEFAULT,
            D3D11_BIND_SHADER_RESOURCE, 0, 0, FALSE);
    for (mip_level = 0; mip_level < 4; ++mip_level)
    {
        winetest_push_context("MipLevel %u", mip_level);
        check_texture_sub_resource_color(tex_2d, mip_level, NULL, dds_24bit_8_8_mip_level_expected[mip_level], 0);
        winetest_pop_context();
    }

    ID3D11Texture2D_Release(tex_2d);
    ID3D11Resource_Release(resource);

    hr2 = 0xdeadbeef;
    add_work_item_count = 0;
    hr = D3DX11CreateTextureFromMemory(device, test_image[0].data, test_image[0].size,
            NULL, &thread_pump, &resource, &hr2);
    ok(add_work_item_count == 1, "Got unexpected add_work_item_count %u.\n", add_work_item_count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
    check_resource_info(resource, test_image, __LINE__);
    check_resource_data(resource, test_image, __LINE__);
    ID3D11Resource_Release(resource);

    /* D3DX11CreateTextureFromFile tests */
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateTextureFromFileW(device, NULL, NULL, NULL, &resource, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateTextureFromFileW(device, L"deadbeef", NULL, NULL, &resource, &hr2);
    ok(hr == D3D11_ERROR_FILE_NOT_FOUND, "Got unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateTextureFromFileA(device, NULL, NULL, NULL, &resource, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateTextureFromFileA(device, "deadbeef", NULL, NULL, &resource, &hr2);
    ok(hr == D3D11_ERROR_FILE_NOT_FOUND, "Got unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);

    for (i = 0; i < ARRAY_SIZE(test_image); ++i)
    {
        winetest_push_context("Test %u", i);
        create_file(test_filename, test_image[i].data, test_image[i].size, path);

        hr2 = 0xdeadbeef;
        hr = D3DX11CreateTextureFromFileW(device, path, NULL, NULL, &resource, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX11_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        if (hr == S_OK)
        {
            check_resource_info(resource, test_image + i, __LINE__);
            check_resource_data(resource, test_image + i, __LINE__);
            ID3D11Resource_Release(resource);
        }

        hr2 = 0xdeadbeef;
        hr = D3DX11CreateTextureFromFileA(device, get_str_a(path), NULL, NULL, &resource, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX11_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        if (hr == S_OK)
        {
            check_resource_info(resource, test_image + i, __LINE__);
            check_resource_data(resource, test_image + i, __LINE__);
            ID3D11Resource_Release(resource);
        }

        delete_file(test_filename);
        winetest_pop_context();
    }

    for (i = 0; i < ARRAY_SIZE(test_invalid_image_load_info); ++i)
    {
        const struct test_invalid_image_load_info *test_load_info = &test_invalid_image_load_info[i];

        winetest_push_context("Test %u", i);
        create_file(test_filename, test_image[i].data, test_image[i].size, path);
        load_info = test_load_info->load_info;

        hr2 = 0xdeadbeef;
        hr = D3DX11CreateTextureFromFileW(device, path, &load_info, NULL, &resource, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        todo_wine_if(test_load_info->todo_hr) ok(hr == test_load_info->expected_hr, "Got unexpected hr %#lx.\n", hr);
        if (SUCCEEDED(hr))
            ID3D11Resource_Release(resource);

        hr = D3DX11CreateTextureFromFileA(device, get_str_a(path), &load_info, NULL, &resource, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        todo_wine_if(test_load_info->todo_hr) ok(hr == test_load_info->expected_hr, "Got unexpected hr %#lx.\n", hr);
        if (SUCCEEDED(hr))
            ID3D11Resource_Release(resource);

        delete_file(test_filename);
        winetest_pop_context();
    }

    /* D3DX11CreateTextureFromResource tests */
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateTextureFromResourceW(device, NULL, NULL, NULL, NULL, &resource, &hr2);
    ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateTextureFromResourceW(device, NULL, L"deadbeef", NULL, NULL, &resource, &hr2);
    ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateTextureFromResourceA(device, NULL, NULL, NULL, NULL, &resource, &hr2);
    ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateTextureFromResourceA(device, NULL, "deadbeef", NULL, NULL, &resource, &hr2);
    ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);

    for (i = 0; i < ARRAY_SIZE(test_image); ++i)
    {
        winetest_push_context("Test %u", i);
        resource_module = create_resource_module(test_resource_name, test_image[i].data, test_image[i].size);

        hr2 = 0xdeadbeef;
        hr = D3DX11CreateTextureFromResourceW(device, resource_module, L"deadbeef", NULL, NULL, &resource, &hr2);
        ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
        ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);

        hr2 = 0xdeadbeef;
        hr = D3DX11CreateTextureFromResourceW(device, resource_module,
                test_resource_name, NULL, NULL, &resource, &hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX11_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        if (hr == S_OK)
        {
            check_resource_info(resource, test_image + i, __LINE__);
            check_resource_data(resource, test_image + i, __LINE__);
            ID3D11Resource_Release(resource);
        }

        hr2 = 0xdeadbeef;
        hr = D3DX11CreateTextureFromResourceA(device, resource_module,
                get_str_a(test_resource_name), NULL, NULL, &resource, &hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX11_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        if (hr == S_OK)
        {
            check_resource_info(resource, test_image + i, __LINE__);
            check_resource_data(resource, test_image + i, __LINE__);
            ID3D11Resource_Release(resource);
        }

        delete_resource_module(test_resource_name, resource_module);
        winetest_pop_context();
    }

    for (i = 0; i < ARRAY_SIZE(test_invalid_image_load_info); ++i)
    {
        const struct test_invalid_image_load_info *test_load_info = &test_invalid_image_load_info[i];

        winetest_push_context("Test %u", i);
        resource_module = create_resource_module(test_resource_name, test_load_info->data, test_load_info->size);
        load_info = test_load_info->load_info;

        hr2 = 0xdeadbeef;
        hr = D3DX11CreateTextureFromResourceW(device, resource_module,
                test_resource_name, &load_info, NULL, &resource, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        todo_wine_if(test_load_info->todo_hr) ok(hr == test_load_info->expected_hr, "Got unexpected hr %#lx.\n", hr);
        if (SUCCEEDED(hr))
            ID3D11Resource_Release(resource);

        hr2 = 0xdeadbeef;
        hr = D3DX11CreateTextureFromResourceA(device, resource_module,
                get_str_a(test_resource_name), &load_info, NULL, &resource, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        todo_wine_if(test_load_info->todo_hr) ok(hr == test_load_info->expected_hr, "Got unexpected hr %#lx.\n", hr);
        if (SUCCEEDED(hr))
            ID3D11Resource_Release(resource);

        delete_resource_module(test_resource_name, resource_module);
        winetest_pop_context();
    }

    CoUninitialize();

    ok(!ID3D11Device_Release(device), "Unexpected refcount.\n");
}

static void test_create_shader_resource_view(void)
{
    static const uint32_t dds_24bit_8_8_mip_level_expected[] = { 0xff0000ff, 0xff00ff00, 0xffff0000, 0xff000000 };
    static const WCHAR test_resource_name[] = L"resource.data";
    static const WCHAR test_filename[] = L"image.data";
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
    D3D11_TEXTURE2D_DESC tex_2d_desc;
    D3DX11_IMAGE_LOAD_INFO load_info;
    ID3D11ShaderResourceView *srv;
    D3DX11_IMAGE_INFO img_info;
    ID3D11Resource *resource;
    ID3D11Texture2D *tex_2d;
    HMODULE resource_module;
    ID3D11Device *device;
    WCHAR path[MAX_PATH];
    uint32_t i, mip_level;
    HRESULT hr, hr2;

    device = create_device();
    if (!device)
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    CoInitialize(NULL);

    /* D3DX11CreateShaderResourceViewFromMemory tests. */
    srv = (ID3D11ShaderResourceView *)0xdeadbeef;
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateShaderResourceViewFromMemory(NULL, test_bmp_1bpp, sizeof(test_bmp_1bpp), NULL, NULL, &srv, &hr2);
    ok(hr == E_INVALIDARG, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    ok(srv == (ID3D11ShaderResourceView *)0xdeadbeef, "Got unexpected srv %p.\n", srv);

    srv = (ID3D11ShaderResourceView *)0xdeadbeef;
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateShaderResourceViewFromMemory(device, NULL, 0, NULL, NULL, &srv, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    ok(srv == (ID3D11ShaderResourceView *)0xdeadbeef, "Got unexpected srv %p.\n", srv);

    srv = (ID3D11ShaderResourceView *)0xdeadbeef;
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateShaderResourceViewFromMemory(device, NULL, sizeof(test_bmp_1bpp), NULL, NULL, &srv, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    ok(srv == (ID3D11ShaderResourceView *)0xdeadbeef, "Got unexpected srv %p.\n", srv);

    srv = (ID3D11ShaderResourceView *)0xdeadbeef;
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateShaderResourceViewFromMemory(device, test_bmp_1bpp, 0, NULL, NULL, &srv, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
    ok(srv == (ID3D11ShaderResourceView *)0xdeadbeef, "Got unexpected srv %p.\n", srv);

    srv = (ID3D11ShaderResourceView *)0xdeadbeef;
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateShaderResourceViewFromMemory(device, test_bmp_1bpp, sizeof(test_bmp_1bpp) - 1, NULL, NULL, &srv, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
    ok(srv == (ID3D11ShaderResourceView *)0xdeadbeef, "Got unexpected srv %p.\n", srv);

    for (i = 0; i < ARRAY_SIZE(test_image); ++i)
    {
        winetest_push_context("Test %u", i);

        hr2 = 0xdeadbeef;
        hr = D3DX11CreateShaderResourceViewFromMemory(device, test_image[i].data, test_image[i].size, NULL, NULL, &srv, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX11_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        if (hr == S_OK)
        {
            check_shader_resource_view_info(srv, test_image + i, __LINE__);
            ID3D11ShaderResourceView_Release(srv);
        }

        winetest_pop_context();
    }

    for (i = 0; i < ARRAY_SIZE(test_invalid_image_load_info); ++i)
    {
        const struct test_invalid_image_load_info *test_load_info = &test_invalid_image_load_info[i];

        winetest_push_context("Test %u", i);

        hr2 = 0xdeadbeef;
        load_info = test_load_info->load_info;
        hr = D3DX11CreateShaderResourceViewFromMemory(device, test_load_info->data, test_load_info->size, &load_info, NULL, &srv, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        todo_wine_if(test_load_info->todo_hr) ok(hr == test_load_info->expected_hr, "Got unexpected hr %#lx.\n", hr);
        if (SUCCEEDED(hr))
            ID3D11ShaderResourceView_Release(srv);

        winetest_pop_context();
    }

    for (i = 0; i < ARRAY_SIZE(test_image_load_info); ++i)
    {
        const struct test_image_load_info *test_load_info = &test_image_load_info[i];

        winetest_push_context("Test %u", i);

        load_info = test_load_info->load_info;
        load_info.pSrcInfo = &img_info;

        srv = NULL;
        hr2 = 0xdeadbeef;
        hr = D3DX11CreateShaderResourceViewFromMemory(device, test_load_info->data, test_load_info->size, &load_info, NULL, &srv, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        ok(hr == test_load_info->expected_hr, "Got unexpected hr %#lx.\n", hr);
        if (SUCCEEDED(hr))
        {
            check_test_image_load_info_srv(srv, test_load_info);
            ID3D11ShaderResourceView_Release(srv);
        }

        winetest_pop_context();
    }

    /* Check behavior of the FirstMipLevel argument. */
    for (i = 0; i < 2; ++i)
    {
        winetest_push_context("FirstMipLevel %u", i);
        memset(&img_info, 0, sizeof(img_info));
        set_d3dx11_image_load_info(&load_info, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, i, D3DX11_FROM_FILE,
                D3D11_USAGE_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT,
                D3DX11_DEFAULT, &img_info);

        srv = NULL;
        hr2 = 0xdeadbeef;
        hr = D3DX11CreateShaderResourceViewFromMemory(device, dds_24bit_8_8, sizeof(dds_24bit_8_8), &load_info, NULL, &srv, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
        check_image_info_values(&img_info, 8, 8, 1, 1, 4, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D,
                D3DX11_IFF_DDS, FALSE);

        ID3D11ShaderResourceView_GetDesc(srv, &srv_desc);
        ok(srv_desc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE2D, "Got unexpected ViewDimension %u.\n", srv_desc.ViewDimension);
        ok(srv_desc.Format == img_info.Format, "Got unexpected Format %#x.\n", srv_desc.Format);
        ok(!srv_desc.Texture2D.MostDetailedMip, "Unexpected MostDetailedMip %u.\n", srv_desc.Texture2D.MostDetailedMip);
        ok(srv_desc.Texture2D.MipLevels == img_info.MipLevels, "Unexpected MipLevels %u.\n", srv_desc.Texture2D.MipLevels);

        ID3D11ShaderResourceView_GetResource(srv, &resource);
        hr = ID3D11Resource_QueryInterface(resource, &IID_ID3D11Texture2D, (void **)&tex_2d);
        ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);

        ID3D11Texture2D_GetDesc(tex_2d, &tex_2d_desc);
        check_texture2d_desc_values(&tex_2d_desc, 8, 8, 4, 1, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 0, D3D11_USAGE_DEFAULT,
                D3D11_BIND_SHADER_RESOURCE, 0, 0, FALSE);
        for (mip_level = 0; mip_level < 4; ++mip_level)
        {
            winetest_push_context("MipLevel %u", mip_level);
            check_texture_sub_resource_color(tex_2d, mip_level, NULL,
                    dds_24bit_8_8_mip_level_expected[min(3, mip_level + i)], 0);
            winetest_pop_context();
        }

        ID3D11Texture2D_Release(tex_2d);
        ID3D11Resource_Release(resource);
        ID3D11ShaderResourceView_Release(srv);
        winetest_pop_context();
    }

    /*
     * If FirstMipLevel is set to a value that is larger than the total number
     * of mip levels in the image, it falls back to 0.
     */
    memset(&img_info, 0, sizeof(img_info));
    set_d3dx11_image_load_info(&load_info, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, 5, D3DX11_FROM_FILE,
            D3D11_USAGE_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT, D3DX11_DEFAULT,
            D3DX11_DEFAULT, &img_info);

    resource = NULL;
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateShaderResourceViewFromMemory(device, dds_24bit_8_8, sizeof(dds_24bit_8_8), &load_info, NULL, &srv, &hr2);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    check_image_info_values(&img_info, 8, 8, 1, 1, 4, 0, DXGI_FORMAT_R8G8B8A8_UNORM, D3D11_RESOURCE_DIMENSION_TEXTURE2D,
            D3DX11_IFF_DDS, FALSE);

    ID3D11ShaderResourceView_GetDesc(srv, &srv_desc);
    ok(srv_desc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE2D, "Got unexpected ViewDimension %u.\n", srv_desc.ViewDimension);
    ok(srv_desc.Format == img_info.Format, "Got unexpected Format %#x.\n", srv_desc.Format);
    ok(!srv_desc.Texture2D.MostDetailedMip, "Unexpected MostDetailedMip %u.\n", srv_desc.Texture2D.MostDetailedMip);
    ok(srv_desc.Texture2D.MipLevels == img_info.MipLevels, "Unexpected MipLevels %u.\n", srv_desc.Texture2D.MipLevels);

    ID3D11ShaderResourceView_GetResource(srv, &resource);
    hr = ID3D11Resource_QueryInterface(resource, &IID_ID3D11Texture2D, (void **)&tex_2d);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ID3D11Texture2D_GetDesc(tex_2d, &tex_2d_desc);
    check_texture2d_desc_values(&tex_2d_desc, 8, 8, 4, 1, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 0, D3D11_USAGE_DEFAULT,
            D3D11_BIND_SHADER_RESOURCE, 0, 0, FALSE);
    for (mip_level = 0; mip_level < 4; ++mip_level)
    {
        winetest_push_context("MipLevel %u", mip_level);
        check_texture_sub_resource_color(tex_2d, mip_level, NULL, dds_24bit_8_8_mip_level_expected[mip_level], 0);
        winetest_pop_context();
    }

    ID3D11Texture2D_Release(tex_2d);
    ID3D11Resource_Release(resource);
    ID3D11ShaderResourceView_Release(srv);

    hr2 = 0xdeadbeef;
    add_work_item_count = 0;
    hr = D3DX11CreateShaderResourceViewFromMemory(device, test_image[0].data, test_image[0].size,
            NULL, &thread_pump, &srv, &hr2);
    ok(add_work_item_count == 1, "Got unexpected add_work_item_count %u.\n", add_work_item_count);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
    check_shader_resource_view_info(srv, test_image, __LINE__);
    ID3D11ShaderResourceView_Release(srv);

    /* D3DX11CreateShaderResourceViewFromFile tests */
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateShaderResourceViewFromFileW(device, NULL, NULL, NULL, &srv, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateShaderResourceViewFromFileW(device, L"deadbeef", NULL, NULL, &srv, &hr2);
    ok(hr == D3D11_ERROR_FILE_NOT_FOUND, "Got unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateShaderResourceViewFromFileA(device, NULL, NULL, NULL, &srv, &hr2);
    ok(hr == E_FAIL, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateShaderResourceViewFromFileA(device, "deadbeef", NULL, NULL, &srv, &hr2);
    ok(hr == D3D11_ERROR_FILE_NOT_FOUND, "Got unexpected hr %#lx.\n", hr);
    ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);

    for (i = 0; i < ARRAY_SIZE(test_image); ++i)
    {
        winetest_push_context("Test %u", i);
        create_file(test_filename, test_image[i].data, test_image[i].size, path);

        hr2 = 0xdeadbeef;
        hr = D3DX11CreateShaderResourceViewFromFileW(device, path, NULL, NULL, &srv, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX11_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        if (hr == S_OK)
        {
            check_shader_resource_view_info(srv, test_image + i, __LINE__);
            ID3D11ShaderResourceView_Release(srv);
        }

        hr2 = 0xdeadbeef;
        hr = D3DX11CreateShaderResourceViewFromFileA(device, get_str_a(path), NULL, NULL, &srv, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX11_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        if (hr == S_OK)
        {
            check_shader_resource_view_info(srv, test_image + i, __LINE__);
            ID3D11ShaderResourceView_Release(srv);
        }

        delete_file(test_filename);
        winetest_pop_context();
    }

    for (i = 0; i < ARRAY_SIZE(test_invalid_image_load_info); ++i)
    {
        const struct test_invalid_image_load_info *test_load_info = &test_invalid_image_load_info[i];

        winetest_push_context("Test %u", i);
        create_file(test_filename, test_image[i].data, test_image[i].size, path);
        load_info = test_load_info->load_info;

        hr2 = 0xdeadbeef;
        hr = D3DX11CreateShaderResourceViewFromFileW(device, path, &load_info, NULL, &srv, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        todo_wine_if(test_load_info->todo_hr) ok(hr == test_load_info->expected_hr, "Got unexpected hr %#lx.\n", hr);
        if (SUCCEEDED(hr))
            ID3D11ShaderResourceView_Release(srv);

        hr = D3DX11CreateShaderResourceViewFromFileA(device, get_str_a(path), &load_info, NULL, &srv, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        todo_wine_if(test_load_info->todo_hr) ok(hr == test_load_info->expected_hr, "Got unexpected hr %#lx.\n", hr);
        if (SUCCEEDED(hr))
            ID3D11ShaderResourceView_Release(srv);

        delete_file(test_filename);
        winetest_pop_context();
    }

    /* D3DX11CreateShaderResourceViewFromResource tests */
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateShaderResourceViewFromResourceW(device, NULL, NULL, NULL, NULL, &srv, &hr2);
    ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateShaderResourceViewFromResourceW(device, NULL, L"deadbeef", NULL, NULL, &srv, &hr2);
    ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateShaderResourceViewFromResourceA(device, NULL, NULL, NULL, NULL, &srv, &hr2);
    ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);
    hr2 = 0xdeadbeef;
    hr = D3DX11CreateShaderResourceViewFromResourceA(device, NULL, "deadbeef", NULL, NULL, &srv, &hr2);
    ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
    ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);

    for (i = 0; i < ARRAY_SIZE(test_image); ++i)
    {
        winetest_push_context("Test %u", i);
        resource_module = create_resource_module(test_resource_name, test_image[i].data, test_image[i].size);

        hr2 = 0xdeadbeef;
        hr = D3DX11CreateShaderResourceViewFromResourceW(device, resource_module, L"deadbeef", NULL, NULL, &srv, &hr2);
        ok(hr == D3DX11_ERR_INVALID_DATA, "Got unexpected hr %#lx.\n", hr);
        ok(hr2 == 0xdeadbeef, "Got unexpected hr2 %#lx.\n", hr2);

        hr2 = 0xdeadbeef;
        hr = D3DX11CreateShaderResourceViewFromResourceW(device, resource_module,
                test_resource_name, NULL, NULL, &srv, &hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX11_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        if (hr == S_OK)
        {
            check_shader_resource_view_info(srv, test_image + i, __LINE__);
            ID3D11ShaderResourceView_Release(srv);
        }

        hr2 = 0xdeadbeef;
        hr = D3DX11CreateShaderResourceViewFromResourceA(device, resource_module,
                get_str_a(test_resource_name), NULL, NULL, &srv, &hr2);
        ok(hr == S_OK || broken(hr == E_FAIL && test_image[i].expected_info.ImageFileFormat == D3DX11_IFF_WMP),
                "Got unexpected hr %#lx.\n", hr);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        if (hr == S_OK)
        {
            check_shader_resource_view_info(srv, test_image + i, __LINE__);
            ID3D11ShaderResourceView_Release(srv);
        }

        delete_resource_module(test_resource_name, resource_module);
        winetest_pop_context();
    }

    for (i = 0; i < ARRAY_SIZE(test_invalid_image_load_info); ++i)
    {
        const struct test_invalid_image_load_info *test_load_info = &test_invalid_image_load_info[i];

        winetest_push_context("Test %u", i);
        resource_module = create_resource_module(test_resource_name, test_load_info->data, test_load_info->size);
        load_info = test_load_info->load_info;

        hr2 = 0xdeadbeef;
        hr = D3DX11CreateShaderResourceViewFromResourceW(device, resource_module,
                test_resource_name, &load_info, NULL, &srv, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        todo_wine_if(test_load_info->todo_hr) ok(hr == test_load_info->expected_hr, "Got unexpected hr %#lx.\n", hr);
        if (SUCCEEDED(hr))
            ID3D11ShaderResourceView_Release(srv);

        hr2 = 0xdeadbeef;
        hr = D3DX11CreateShaderResourceViewFromResourceA(device, resource_module,
                get_str_a(test_resource_name), &load_info, NULL, &srv, &hr2);
        ok(hr == hr2, "Got unexpected hr2 %#lx.\n", hr2);
        todo_wine_if(test_load_info->todo_hr) ok(hr == test_load_info->expected_hr, "Got unexpected hr %#lx.\n", hr);
        if (SUCCEEDED(hr))
            ID3D11ShaderResourceView_Release(srv);

        delete_resource_module(test_resource_name, resource_module);
        winetest_pop_context();
    }

    CoUninitialize();

    ok(!ID3D11Device_Release(device), "Unexpected refcount.\n");
}

/*
 * 4x4 RGBA cubemap with faces in the following order: blue, green, red,
 * green/blue, red/blue, red/green.
 */
static const uint8_t rgba_4_4_cubemap[] =
{
    0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,
    0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,
    0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,
    0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,
    0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,
    0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,
    0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,
    0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,
    0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,
    0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,
    0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,
    0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,
    0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,
    0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,
    0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,
    0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,
    0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,
    0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,
    0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,
    0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,
    0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,
    0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,
    0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,
    0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,0xff,0xff,0x00,0xff,
};

/*
 * 8x8 (BC1-BC3) image data, four 4x4 blocks:
 * +-----+-----+
 * |Blue |Green|
 * |     |     |
 * +-----+-----+
 * |Red  |Black|
 * |     |     |
 * +-----+-----+
 */
static const uint8_t bc1_8_8[] =
{
    0x1f,0x00,0x1f,0x00,0xaa,0xaa,0xaa,0xaa,0xe0,0x07,0xe0,0x07,0xaa,0xaa,0xaa,0xaa,
    0x00,0xf8,0x00,0xf8,0xaa,0xaa,0xaa,0xaa,0x00,0x00,0x00,0x00,0xaa,0xaa,0xaa,0xaa,
};

static const uint8_t bc2_8_8[] =
{
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x1f,0x00,0x1f,0x00,0xaa,0xaa,0xaa,0xaa,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xe0,0x07,0xe0,0x07,0xaa,0xaa,0xaa,0xaa,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0xf8,0x00,0xf8,0xaa,0xaa,0xaa,0xaa,
    0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,0xaa,0xaa,0xaa,0xaa,
};

static const uint8_t bc3_8_8[] =
{
    0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1f,0x00,0x1f,0x00,0x00,0x00,0x00,0x00,
    0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xe0,0x07,0xe0,0x07,0x00,0x00,0x00,0x00,
    0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xf8,0x00,0xf8,0x00,0x00,0x00,0x00,
    0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

static const uint8_t bc1_to_bc3_8_8_decompressed[] =
{
    0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,
    0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,
    0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,
    0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,
    0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,
    0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,
    0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,
    0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00,0xff,
    0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,
    0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,
    0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,
    0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,
    0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,
    0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,
    0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,0xff,0x00,0x00,0xff,
    0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,0x00,0x00,0x00,0xff,
};

static const uint8_t bc4_unorm_8_8[] =
{
    0xff,0xff,0x49,0x92,0x24,0x49,0x92,0x24,0x80,0x80,0x49,0x92,0x24,0x49,0x92,0x24,
    0x00,0x00,0x49,0x92,0x24,0x49,0x92,0x24,0x40,0x40,0x49,0x92,0x24,0x49,0x92,0x24,
};

static const uint8_t r8_unorm_8_8_decompressed[] =
{
    0xff,0xff,0xff,0xff,0x80,0x80,0x80,0x80,0xff,0xff,0xff,0xff,0x80,0x80,0x80,0x80,
    0xff,0xff,0xff,0xff,0x80,0x80,0x80,0x80,0xff,0xff,0xff,0xff,0x80,0x80,0x80,0x80,
    0x00,0x00,0x00,0x00,0x40,0x40,0x40,0x40,0x00,0x00,0x00,0x00,0x40,0x40,0x40,0x40,
    0x00,0x00,0x00,0x00,0x40,0x40,0x40,0x40,0x00,0x00,0x00,0x00,0x40,0x40,0x40,0x40,
};

static const uint8_t bc5_unorm_8_8[] =
{
    0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xdf,0xdf,0x00,0x00,0x00,0x00,0x00,0x00,
    0xbf,0xbf,0x00,0x00,0x00,0x00,0x00,0x00,0x9f,0x9f,0x00,0x00,0x00,0x00,0x00,0x00,
    0x7f,0x7f,0x00,0x00,0x00,0x00,0x00,0x00,0x5f,0x5f,0x00,0x00,0x00,0x00,0x00,0x00,
    0x3f,0x3f,0x00,0x00,0x00,0x00,0x00,0x00,0x1f,0x1f,0x00,0x00,0x00,0x00,0x00,0x00,
};

static const uint8_t r8g8_unorm_8_8_decompressed[] =
{
    0xff,0xdf,0xff,0xdf,0xff,0xdf,0xff,0xdf,0xbf,0x9f,0xbf,0x9f,0xbf,0x9f,0xbf,0x9f,
    0xff,0xdf,0xff,0xdf,0xff,0xdf,0xff,0xdf,0xbf,0x9f,0xbf,0x9f,0xbf,0x9f,0xbf,0x9f,
    0xff,0xdf,0xff,0xdf,0xff,0xdf,0xff,0xdf,0xbf,0x9f,0xbf,0x9f,0xbf,0x9f,0xbf,0x9f,
    0xff,0xdf,0xff,0xdf,0xff,0xdf,0xff,0xdf,0xbf,0x9f,0xbf,0x9f,0xbf,0x9f,0xbf,0x9f,
    0x7f,0x5f,0x7f,0x5f,0x7f,0x5f,0x7f,0x5f,0x3f,0x1f,0x3f,0x1f,0x3f,0x1f,0x3f,0x1f,
    0x7f,0x5f,0x7f,0x5f,0x7f,0x5f,0x7f,0x5f,0x3f,0x1f,0x3f,0x1f,0x3f,0x1f,0x3f,0x1f,
    0x7f,0x5f,0x7f,0x5f,0x7f,0x5f,0x7f,0x5f,0x3f,0x1f,0x3f,0x1f,0x3f,0x1f,0x3f,0x1f,
    0x7f,0x5f,0x7f,0x5f,0x7f,0x5f,0x7f,0x5f,0x3f,0x1f,0x3f,0x1f,0x3f,0x1f,0x3f,0x1f,
};

/*
 * DXGI_FORMAT_BC{4,5}_SNORM compression/decompression is bugged in
 * native D3DX10/D3DX11. When decompressing, it seems to read the decompressed
 * 8-bit channel values as signed normalized integers, but then clamps them to the
 * unsigned normalized integer range. That means 0x00-0x7f present unique values,
 * but anything from 0x80-0xff just gives the equivalent of 0x00. When this gets
 * converted to an SNORM format such as DXGI_FORMAT_R8_SNORM, it gets mapped
 * to the SNORM range, where 0x00 is -1.0f, and 0x7f is 1.0f. So effectively,
 * it ends up with half of the range.
 */
static const uint8_t bc4_snorm_8_8[] =
{
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,0x20,0x00,0x00,0x00,0x00,0x00,0x00,
    0x7f,0x7f,0x00,0x00,0x00,0x00,0x00,0x00,0x5f,0x5f,0x00,0x00,0x00,0x00,0x00,0x00,
};

static const uint8_t r8_snorm_8_8_decompressed[] =
{
    0x81,0x81,0x81,0x81,0xc1,0xc1,0xc1,0xc1,0x81,0x81,0x81,0x81,0xc1,0xc1,0xc1,0xc1,
    0x81,0x81,0x81,0x81,0xc1,0xc1,0xc1,0xc1,0x81,0x81,0x81,0x81,0xc1,0xc1,0xc1,0xc1,
    0x7F,0x7F,0x7F,0x7F,0x3F,0x3F,0x3F,0x3F,0x7F,0x7F,0x7F,0x7F,0x3F,0x3F,0x3F,0x3F,
    0x7F,0x7F,0x7F,0x7F,0x3F,0x3F,0x3F,0x3F,0x7F,0x7F,0x7F,0x7F,0x3F,0x3F,0x3F,0x3F,
};

static const uint8_t bc5_snorm_8_8[] =
{
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x10,0x00,0x00,0x00,0x00,0x00,0x00,
    0x1f,0x1f,0x00,0x00,0x00,0x00,0x00,0x00,0x2f,0x2f,0x00,0x00,0x00,0x00,0x00,0x00,
    0x3e,0x3e,0x00,0x00,0x00,0x00,0x00,0x00,0x4e,0x4e,0x00,0x00,0x00,0x00,0x00,0x00,
    0x5d,0x5d,0x00,0x00,0x00,0x00,0x00,0x00,0x6d,0x6d,0x00,0x00,0x00,0x00,0x00,0x00,
};

static const uint8_t r8g8_snorm_8_8_decompressed[] =
{
    0x81,0xa1,0x81,0xa1,0x81,0xa1,0x81,0xa1,0xbf,0xdf,0xbf,0xdf,0xbf,0xdf,0xbf,0xdf,
    0x81,0xa1,0x81,0xa1,0x81,0xa1,0x81,0xa1,0xbf,0xdf,0xbf,0xdf,0xbf,0xdf,0xbf,0xdf,
    0x81,0xa1,0x81,0xa1,0x81,0xa1,0x81,0xa1,0xbf,0xdf,0xbf,0xdf,0xbf,0xdf,0xbf,0xdf,
    0x81,0xa1,0x81,0xa1,0x81,0xa1,0x81,0xa1,0xbf,0xdf,0xbf,0xdf,0xbf,0xdf,0xbf,0xdf,
    0xfd,0x1d,0xfd,0x1d,0xfd,0x1d,0xfd,0x1d,0x3b,0x5b,0x3b,0x5b,0x3b,0x5b,0x3b,0x5b,
    0xfd,0x1d,0xfd,0x1d,0xfd,0x1d,0xfd,0x1d,0x3b,0x5b,0x3b,0x5b,0x3b,0x5b,0x3b,0x5b,
    0xfd,0x1d,0xfd,0x1d,0xfd,0x1d,0xfd,0x1d,0x3b,0x5b,0x3b,0x5b,0x3b,0x5b,0x3b,0x5b,
    0xfd,0x1d,0xfd,0x1d,0xfd,0x1d,0xfd,0x1d,0x3b,0x5b,0x3b,0x5b,0x3b,0x5b,0x3b,0x5b,
};

static const uint8_t rgba_unorm_4_4[] =
{
    0x10,0x20,0x30,0x40,0x50,0x60,0x70,0x80,0x90,0xa0,0xb0,0xc0,0xd0,0xe0,0xf0,0xff,
    0x14,0x24,0x34,0x44,0x54,0x64,0x74,0x84,0x94,0xa4,0xb4,0xc4,0xd4,0xe4,0xf4,0xff,
    0x18,0x28,0x38,0x48,0x58,0x68,0x78,0x88,0x98,0xa8,0xb8,0xc8,0xd8,0xe8,0xf8,0xff,
    0x1c,0x2c,0x3c,0x4c,0x5c,0x6c,0x7c,0x8c,0x9c,0xac,0xbc,0xcc,0xdc,0xec,0xfc,0xff,
};

static const uint8_t rgba_snorm_4_4[] =
{
    0x91,0xa1,0xb1,0xc1,0xd1,0xe1,0xf1,0x00,0x10,0x20,0x30,0x40,0x50,0x60,0x70,0x7f,
    0x95,0xa5,0xb5,0xc5,0xd5,0xe5,0xf5,0x04,0x14,0x24,0x34,0x44,0x54,0x64,0x74,0x7f,
    0x99,0xa9,0xb9,0xc9,0xd9,0xe9,0xf9,0x08,0x18,0x28,0x38,0x48,0x58,0x68,0x78,0x7f,
    0x9d,0xad,0xbd,0xcd,0xdd,0xed,0xfd,0x0c,0x1c,0x2c,0x3c,0x4c,0x5c,0x6c,0x7c,0x7f,
};

/* Conversion to/from uint/sint. */
static const uint8_t rgba_uint_4_4[] =
{
    0x00,0x04,0x08,0x0c,0x10,0x14,0x18,0x1c,0x20,0x24,0x28,0x2c,0x30,0x34,0x38,0x3c,
    0x40,0x44,0x48,0x4c,0x50,0x54,0x58,0x5c,0x60,0x64,0x68,0x6c,0x70,0x74,0x78,0x7c,
    0x80,0x84,0x88,0x8c,0x90,0x94,0x98,0x9c,0xa0,0xa4,0xa8,0xac,0xb0,0xb4,0xb8,0xbc,
    0xc0,0xc4,0xc8,0xcc,0xd0,0xd4,0xd8,0xdc,0xe0,0xe4,0xe8,0xec,0xf0,0xf4,0xf8,0xfc,
};

static const uint8_t rgba_uint_to_sint_4_4[] =
{
    0x00,0x04,0x08,0x0c,0x10,0x14,0x18,0x1c,0x20,0x24,0x28,0x2c,0x30,0x34,0x38,0x3c,
    0x40,0x44,0x48,0x4c,0x50,0x54,0x58,0x5c,0x60,0x64,0x68,0x6c,0x70,0x74,0x78,0x7c,
    0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,
    0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,
};

static const uint8_t rgba_sint_4_4[] =
{
    0x80,0x84,0x88,0x8c,0x90,0x94,0x98,0x9c,0xa0,0xa4,0xa8,0xac,0xb0,0xb4,0xb8,0xbc,
    0xc0,0xc4,0xc8,0xcc,0xd0,0xd4,0xd8,0xdc,0xe0,0xe4,0xe8,0xec,0xf0,0xf4,0xf8,0xfc,
    0x00,0x04,0x08,0x0c,0x10,0x14,0x18,0x1c,0x20,0x24,0x28,0x2c,0x30,0x34,0x38,0x3c,
    0x40,0x44,0x48,0x4c,0x50,0x54,0x58,0x5c,0x60,0x64,0x68,0x6c,0x70,0x74,0x78,0x7c,
};

static const uint8_t rgba_sint_to_uint_4_4[] =
{
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x04,0x08,0x0c,0x10,0x14,0x18,0x1c,0x20,0x24,0x28,0x2c,0x30,0x34,0x38,0x3c,
    0x40,0x44,0x48,0x4c,0x50,0x54,0x58,0x5c,0x60,0x64,0x68,0x6c,0x70,0x74,0x78,0x7c,
};

/* Conversion to/from SRGB. */
static const uint8_t rgba_unorm_srgb_4_4[] =
{
    0x10,0x20,0x30,0x40,0x50,0x60,0x70,0x80,0x90,0xa0,0xb0,0xc0,0xd0,0xe0,0xf0,0xff,
    0x14,0x24,0x34,0x44,0x54,0x64,0x74,0x84,0x94,0xa4,0xb4,0xc4,0xd4,0xe4,0xf4,0xff,
    0x18,0x28,0x38,0x48,0x58,0x68,0x78,0x88,0x98,0xa8,0xb8,0xc8,0xd8,0xe8,0xf8,0xff,
    0x1c,0x2c,0x3c,0x4c,0x5c,0x6c,0x7c,0x8c,0x9c,0xac,0xbc,0xcc,0xdc,0xec,0xfc,0xff,
};

static const uint8_t rgba_unorm_srgb_to_unorm_non_srgb_4_4[] =
{
    0x01,0x03,0x06,0x40,0x14,0x1e,0x2a,0x80,0x49,0x5b,0x71,0xc0,0xa3,0xc0,0xdf,0xff,
    0x01,0x03,0x08,0x44,0x16,0x21,0x2d,0x84,0x4d,0x61,0x77,0xc4,0xaa,0xc7,0xe7,0xff,
    0x01,0x04,0x09,0x48,0x19,0x23,0x31,0x88,0x52,0x66,0x7c,0xc8,0xb1,0xcf,0xf0,0xff,
    0x02,0x05,0x0b,0x4c,0x1b,0x27,0x34,0x8c,0x57,0x6b,0x82,0xcc,0xb8,0xd7,0xf8,0xff,
};

static const uint8_t rgba_unorm_non_srgb_4_4[] =
{
    0x00,0x20,0x50,0x40,0x50,0x60,0x70,0x80,0x90,0xa0,0xb0,0xc0,0xd0,0xe0,0xf0,0xff,
    0x00,0x24,0x54,0x44,0x54,0x64,0x74,0x84,0x94,0xa4,0xb4,0xc4,0xd4,0xe4,0xf4,0xff,
    0x00,0x28,0x58,0x48,0x58,0x68,0x78,0x88,0x98,0xa8,0xb8,0xc8,0xd8,0xe8,0xf8,0xff,
    0x00,0x2c,0x5c,0x4c,0x5c,0x6c,0x7c,0x8c,0x9c,0xac,0xbc,0xcc,0xdc,0xec,0xfc,0xff,
};

static const uint8_t rgba_unorm_non_srgb_to_unorm_srgb_4_4[] =
{
    0x00,0x63,0x97,0x40,0x97,0xa4,0xaf,0x80,0xc5,0xce,0xd7,0xc0,0xe8,0xf0,0xf8,0xff,
    0x00,0x69,0x9a,0x44,0x9a,0xa7,0xb2,0x84,0xc7,0xd1,0xda,0xc4,0xea,0xf2,0xfa,0xff,
    0x00,0x6e,0x9d,0x48,0x9d,0xaa,0xb5,0x88,0xca,0xd3,0xdc,0xc8,0xec,0xf4,0xfc,0xff,
    0x00,0x73,0xa0,0x4c,0xa0,0xad,0xb8,0x8c,0xcc,0xd5,0xde,0xcc,0xee,0xf6,0xfe,0xff,
};

static const struct test_texture_format_conversion
{
    D3D11_TEXTURE2D_DESC src_desc;
    const uint8_t *src_data;

    DXGI_FORMAT dst_format;
    const uint8_t *expected_dst_data;

    uint8_t max_diff;
    BOOL todo_hr;
    BOOL todo_data;
}
test_texture_format_conversion[] =
{
    {
        { 8, 8, 1, 1, DXGI_FORMAT_BC1_UNORM,      { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0 },
        bc1_8_8,      DXGI_FORMAT_R8G8B8A8_UNORM, bc1_to_bc3_8_8_decompressed
    },
    {
        { 8, 8, 1, 1, DXGI_FORMAT_BC2_UNORM,      { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0 },
        bc2_8_8,      DXGI_FORMAT_R8G8B8A8_UNORM, bc1_to_bc3_8_8_decompressed
    },
    {
        { 8, 8, 1, 1, DXGI_FORMAT_BC3_UNORM,      { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0 },
        bc3_8_8,      DXGI_FORMAT_R8G8B8A8_UNORM, bc1_to_bc3_8_8_decompressed
    },
    {
        { 8, 8, 1, 1,  DXGI_FORMAT_BC4_UNORM, { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0 },
        bc4_unorm_8_8, DXGI_FORMAT_R8_UNORM,  r8_unorm_8_8_decompressed
    },
    {
        { 8, 8, 1, 1,  DXGI_FORMAT_BC4_SNORM, { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0 },
        bc4_snorm_8_8, DXGI_FORMAT_R8_SNORM,  r8_snorm_8_8_decompressed, .todo_data = TRUE
    },
    {
        { 8, 8, 1, 1,  DXGI_FORMAT_BC5_UNORM,  { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0 },
        bc5_unorm_8_8, DXGI_FORMAT_R8G8_UNORM, r8g8_unorm_8_8_decompressed
    },
    {
        { 8, 8, 1, 1,  DXGI_FORMAT_BC5_SNORM,      { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0 },
        bc5_snorm_8_8, DXGI_FORMAT_R8G8_SNORM,  r8g8_snorm_8_8_decompressed, .todo_data = TRUE
    },
    {
        { 4, 4, 1, 1,   DXGI_FORMAT_R8G8B8A8_UNORM, { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0 },
        /*
         * Wine's UNORM->SNORM conversion doesn't always match Window's,
         * worst case is a difference of +/- 1.
         */
        rgba_unorm_4_4, DXGI_FORMAT_R8G8B8A8_SNORM, rgba_snorm_4_4, .max_diff = 1
    },
    {
        { 4, 4, 1, 1,  DXGI_FORMAT_R8G8B8A8_UINT, { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0 },
        rgba_uint_4_4, DXGI_FORMAT_R8G8B8A8_SINT, rgba_uint_to_sint_4_4, .todo_hr = TRUE
    },
    {
        { 4, 4, 1, 1,  DXGI_FORMAT_R8G8B8A8_SINT, { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0 },
        rgba_sint_4_4, DXGI_FORMAT_R8G8B8A8_UINT, rgba_sint_to_uint_4_4, .todo_hr = TRUE
    },
    {
        { 4, 4, 1, 1,        DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0 },
        rgba_unorm_srgb_4_4, DXGI_FORMAT_R8G8B8A8_UNORM,      rgba_unorm_srgb_to_unorm_non_srgb_4_4, .max_diff = 3
    },
    {
        { 4, 4, 1, 1,            DXGI_FORMAT_R8G8B8A8_UNORM,      { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0 },
        rgba_unorm_non_srgb_4_4, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, rgba_unorm_non_srgb_to_unorm_srgb_4_4, .max_diff = 2
    },
};

static const struct test_texture_compression
{
    D3D11_TEXTURE2D_DESC src_desc;
    DXGI_FORMAT compressed_format;
    const BYTE *src_data;
}
test_texture_compression[] =
{
    {
        { 8, 8, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0 },
        DXGI_FORMAT_BC1_UNORM, bc1_to_bc3_8_8_decompressed
    },
    {
        { 8, 8, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0 },
        DXGI_FORMAT_BC2_UNORM, bc1_to_bc3_8_8_decompressed
    },
    {
        { 8, 8, 1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0 },
        DXGI_FORMAT_BC3_UNORM, bc1_to_bc3_8_8_decompressed
    },
    {
        { 8, 8, 1, 1, DXGI_FORMAT_R8_UNORM, { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0 },
        DXGI_FORMAT_BC4_UNORM, r8_unorm_8_8_decompressed
    },
    {
        { 8, 8, 1, 1, DXGI_FORMAT_R8G8_UNORM, { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0 },
        DXGI_FORMAT_BC5_UNORM, r8g8_unorm_8_8_decompressed
    },
};

#define check_texture_data_level(tex_pixels, tex_row_pitch, tex_slice_pitch, width, height, depth, format, exp_pixels, max_diff, wine_todo) \
        check_texture_data_level_(tex_pixels, tex_row_pitch, tex_slice_pitch, width, height, depth, format, exp_pixels, max_diff, wine_todo, __LINE__)
static void check_texture_data_level_(const uint8_t *tex_pixels, uint32_t tex_row_pitch, uint32_t tex_slice_pitch, uint32_t width,
        uint32_t height, uint32_t depth, DXGI_FORMAT format, const uint8_t *exp_pixels, uint8_t max_diff, BOOL wine_todo, uint32_t line)
{
    uint32_t line_height, exp_row_pitch, exp_slice_pitch, i, j, k;
    BOOL line_match = FALSE;

    line_height = 1;
    exp_row_pitch = (width * get_bpp_from_format(format) + 7) / 8;
    exp_slice_pitch = exp_row_pitch * height;
    if (is_block_compressed(format))
    {
        exp_row_pitch *= 4;
        line_height = 4;
    }

    for (i = 0; i < depth; ++i)
    {
        const uint8_t *exp_slice = exp_pixels + (i * (exp_slice_pitch));
        const uint8_t *pixel_slice = tex_pixels + (i * tex_slice_pitch);

        for (j = 0; j < height; j += line_height)
        {
            if (dxgi_format_is_8bpp_rgba(format))
            {
                for (k = 0; k < width; ++k)
                {
                    const uint32_t exp_pixel = ((const uint32_t *)(exp_slice + j * exp_row_pitch))[k];
                    const uint32_t tex_pixel = ((const uint32_t *)(pixel_slice + j * tex_row_pitch))[k];
                    const BOOL pixel_match = compare_color(tex_pixel, exp_pixel, max_diff);

                    todo_wine_if(wine_todo) ok_(__FILE__, line)(pixel_match, "Data mismatch for pixel (%ux%ux%u).\n", k, j, i);
                    line_match = pixel_match;
                    if (!pixel_match)
                        break;
                }
            }
            else
            {
                line_match = !memcmp(exp_slice + exp_row_pitch * (j / line_height),
                        pixel_slice + tex_row_pitch * (j / line_height), exp_row_pitch);
                todo_wine_if(wine_todo) ok_(__FILE__, line)(line_match, "Data mismatch for line %u, slice %u.\n", j, i);
            }
            if (!line_match)
                break;
        }
    }
}

static void set_d3dx11_texture_load_info(D3DX11_TEXTURE_LOAD_INFO *load_info, D3D11_BOX *src_box, D3D11_BOX *dst_box,
        uint32_t src_first_mip, uint32_t dst_first_mip, uint32_t num_mips, uint32_t src_first_element,
        uint32_t dst_first_element, uint32_t num_elems, uint32_t filter, uint32_t mip_filter)
{
    load_info->pSrcBox = src_box;
    load_info->pDstBox = dst_box;
    load_info->SrcFirstMip = src_first_mip;
    load_info->DstFirstMip = dst_first_mip;
    load_info->NumMips = num_mips;
    load_info->SrcFirstElement = src_first_element;
    load_info->DstFirstElement = dst_first_element;
    load_info->NumElements = num_elems;
    load_info->Filter = filter;
    load_info->MipFilter = mip_filter;
}

static void set_d3d11_2d_texture_desc(D3D11_TEXTURE2D_DESC *desc, uint32_t width, uint32_t height, uint32_t mip_levels,
        uint32_t array_size, DXGI_FORMAT format, uint32_t sample_count, uint32_t sample_quality, uint32_t usage,
        uint32_t bind_flags, uint32_t cpu_access_flags, uint32_t misc_flags)
{
    desc->Width = width;
    desc->Height = height;
    desc->MipLevels = mip_levels;
    desc->ArraySize = array_size;
    desc->Format = format;
    desc->SampleDesc.Count = sample_count;
    desc->SampleDesc.Quality = sample_quality;
    desc->Usage = usage;
    desc->BindFlags = bind_flags;
    desc->CPUAccessFlags = cpu_access_flags;
    desc->MiscFlags = misc_flags;
}

static void init_subresource_data(D3D11_SUBRESOURCE_DATA *subresources, const void *data, uint32_t width, uint32_t height,
        uint32_t depth, uint32_t mip_levels, uint32_t array_size, DXGI_FORMAT format)
{
    const uint8_t *pixel_ptr = data;
    uint32_t i, j;

    for (i = 0; i < array_size; ++i)
    {
        uint32_t tmp_width, tmp_height, tmp_depth;

        tmp_width = width;
        tmp_height = height;
        tmp_depth = depth;
        for (j = 0; j < mip_levels; ++j)
        {
            D3D11_SUBRESOURCE_DATA *subresource = &subresources[(i * mip_levels) + j];

            subresource->pSysMem = pixel_ptr;
            subresource->SysMemPitch = (tmp_width * get_bpp_from_format(format) + 7) / 8;
            subresource->SysMemSlicePitch = subresource->SysMemPitch * tmp_height;
            if (is_block_compressed(format))
                subresource->SysMemPitch *= 4;
            pixel_ptr += (subresource->SysMemSlicePitch * tmp_depth);

            tmp_width  = max(tmp_width  / 2, 1);
            tmp_height = max(tmp_height / 2, 1);
            tmp_depth  = max(tmp_depth  / 2, 1);
        }
    }
}

static uint8_t *init_buffer_color(uint8_t *buf, uint32_t color, uint32_t width, uint32_t height, uint32_t depth)
{
    uint32_t i, total_pixels = width * height * depth;
    uint32_t *color_buf = (uint32_t *)buf;

    for (i = 0; i < total_pixels; ++i)
        color_buf[i] = color;
    return buf + (total_pixels * 4);
}

static void set_texture_sub_resource_color(ID3D11DeviceContext *context, ID3D11Resource *rsrc, uint32_t idx, uint32_t color,
        uint32_t width, uint32_t height, uint32_t depth)
{
    uint8_t tmp_buf[1024];

    init_buffer_color(tmp_buf, color, width, height, depth);
    ID3D11DeviceContext_UpdateSubresource(context, rsrc, idx, NULL, (const void *)tmp_buf, width * sizeof(color),
            width * height * sizeof(color));
}

static void test_D3DX11LoadTextureFromTexture(void)
{
    static const uint32_t test_cubemap_face_colors[] = { 0xffff0000, 0xff00ff00, 0xff0000ff, 0xffffff00, 0xffff00ff, 0xff00ffff };
    static const uint32_t test_tex_2d_array_colors[] = { 0xff0000ff, 0xff00ff00, 0xffff0000, 0xff000000 };
    D3D11_SUBRESOURCE_DATA sub_resource_data[6] = { 0 };
    D3DX11_TEXTURE_LOAD_INFO load_info;
    ID3D11Texture2D *tex_2d, *tex2_2d;
    D3D11_TEXTURE2D_DESC tex_2d_desc;
    uint8_t tmp_buf[1024], *tmp_ptr;
    ID3D11DeviceContext *context;
    ID3D11Device *device;
    RECT tmp_rect;
    uint32_t i;
    HRESULT hr;

    device = create_device();
    if (!device)
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    CoInitialize(NULL);
    ID3D11Device_GetImmediateContext(device, &context);

    /*
     * Tests that still need to be written:
     * -pSrcBox/pDstBox, how they behave WRT mip levels, too large, too small.
     * -3D texture test.
     */

    /* 8x8 2D texture array with 2 elements and 2 mip levels. */
    tmp_ptr = tmp_buf;
    for (i = 0; i < ARRAY_SIZE(test_tex_2d_array_colors); ++i)
        tmp_ptr = init_buffer_color(tmp_ptr, test_tex_2d_array_colors[i], !(i & 0x1) ? 8 : 4, !(i & 0x1) ? 8 : 4, 1);

    set_d3d11_2d_texture_desc(&tex_2d_desc, 8, 8, 2, 2, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 0, D3D11_USAGE_DEFAULT,
            D3D11_BIND_SHADER_RESOURCE, 0, 0);
    init_subresource_data(sub_resource_data, (const void *)tmp_buf, tex_2d_desc.Width, tex_2d_desc.Height, 1,
            tex_2d_desc.MipLevels, tex_2d_desc.ArraySize, tex_2d_desc.Format);
    hr = ID3D11Device_CreateTexture2D(device, &tex_2d_desc, sub_resource_data, &tex_2d);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    for (i = 0; i < ARRAY_SIZE(test_tex_2d_array_colors); ++i)
        check_texture_sub_resource_color(tex_2d, i, NULL, test_tex_2d_array_colors[i], 0);

    /* 8x8 2D texture with 4 mip levels. */
    memset(tmp_buf, 0xff, sizeof(tmp_buf));
    set_d3d11_2d_texture_desc(&tex_2d_desc, 8, 8, 4, 1, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 0, D3D11_USAGE_DEFAULT,
            D3D11_BIND_SHADER_RESOURCE, 0, 0);
    init_subresource_data(sub_resource_data, (const void *)tmp_buf, tex_2d_desc.Width, tex_2d_desc.Height, 1,
            tex_2d_desc.MipLevels, tex_2d_desc.ArraySize, tex_2d_desc.Format);
    hr = ID3D11Device_CreateTexture2D(device, &tex_2d_desc, sub_resource_data, &tex2_2d);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    for (i = 0; i < 4; ++i)
        check_texture_sub_resource_color(tex2_2d, i, NULL, 0xffffffff, 0);

    /*
     * If a NULL load info argument is supplied, the default D3DX11_TEXTURE_LOAD_INFO
     * values are used. The first 2 mip levels from tex_2d are loaded, and
     * the last 2 mip levels of tex2_2d are generated from mip level 1.
     */
    hr = D3DX11LoadTextureFromTexture(context, (ID3D11Resource *)tex_2d, NULL, (ID3D11Resource *)tex2_2d);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    for (i = 0; i < 4; ++i)
        check_texture_sub_resource_color(tex2_2d, i, NULL, test_tex_2d_array_colors[min(i, 1)], 0);

    /* NULL device context. */
    hr = D3DX11LoadTextureFromTexture(NULL, (ID3D11Resource *)tex_2d, NULL, (ID3D11Resource *)tex2_2d);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);

    /* Invalid Filter argument. */
    set_d3dx11_texture_load_info(&load_info, NULL, NULL, 0, 0, 0, 0, 0, 0, 9, D3DX11_DEFAULT);
    hr = D3DX11LoadTextureFromTexture(context, (ID3D11Resource *)tex_2d, &load_info, (ID3D11Resource *)tex2_2d);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);

    /* Filter argument of 0 is invalid. */
    set_d3dx11_texture_load_info(&load_info, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, D3DX11_DEFAULT);
    hr = D3DX11LoadTextureFromTexture(context, (ID3D11Resource *)tex_2d, &load_info, (ID3D11Resource *)tex2_2d);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);

    /* Invalid MipFilter argument. */
    set_d3dx11_texture_load_info(&load_info, NULL, NULL, 0, 0, 0, 0, 0, 0, D3DX11_DEFAULT, 9);
    hr = D3DX11LoadTextureFromTexture(context, (ID3D11Resource *)tex_2d, &load_info, (ID3D11Resource *)tex2_2d);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);

    /* MipFilter argument is only validated if mip levels are generated. */
    set_d3dx11_texture_load_info(&load_info, NULL, NULL, 0, 0, 2, 0, 0, 0, D3DX11_DEFAULT, 9);
    hr = D3DX11LoadTextureFromTexture(context, (ID3D11Resource *)tex_2d, &load_info, (ID3D11Resource *)tex2_2d);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /*
     * Cannot use the same source/destination texture with matching first mip
     * level and array element.
     */
    set_d3dx11_texture_load_info(&load_info, NULL, NULL, 0, 0, 0, 0, 0, 0, D3DX11_DEFAULT, D3DX11_DEFAULT);
    hr = D3DX11LoadTextureFromTexture(context, (ID3D11Resource *)tex_2d, &load_info, (ID3D11Resource *)tex_2d);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);

    /* Same first mip level, different first element. */
    set_d3dx11_texture_load_info(&load_info, NULL, NULL, 0, 0, 0, 0, 1, 0, D3DX11_DEFAULT, D3DX11_DEFAULT);
    hr = D3DX11LoadTextureFromTexture(context, (ID3D11Resource *)tex_2d, &load_info, (ID3D11Resource *)tex_2d);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    for (i = 0; i < 2; ++i)
        check_texture_sub_resource_color(tex_2d, 2 + i, NULL, test_tex_2d_array_colors[i], 0);

    /* Restore values. */
    for (i = 0; i < ARRAY_SIZE(test_tex_2d_array_colors); ++i)
    {
        set_texture_sub_resource_color(context, (ID3D11Resource *)tex_2d, i, test_tex_2d_array_colors[i], !(i & 0x1) ? 8 : 4,
                !(i & 0x1) ? 8 : 4, 1);
        check_texture_sub_resource_color(tex_2d, i, NULL, test_tex_2d_array_colors[i], 0);
    }

    /* Same first array element, but different FirstMips. */
    set_d3dx11_texture_load_info(&load_info, NULL, NULL, 0, 1, 0, 0, 0, 0, D3DX11_DEFAULT, D3DX11_DEFAULT);
    hr = D3DX11LoadTextureFromTexture(context, (ID3D11Resource *)tex_2d, &load_info, (ID3D11Resource *)tex_2d);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    for (i = 0; i < 4; ++i)
        check_texture_sub_resource_color(tex_2d, i, NULL, test_tex_2d_array_colors[(i < 2) ? 0 : 2], 0);

    for (i = 0; i < ARRAY_SIZE(test_tex_2d_array_colors); ++i)
    {
        set_texture_sub_resource_color(context, (ID3D11Resource *)tex_2d, i, test_tex_2d_array_colors[i], !(i & 0x1) ? 8 : 4,
                !(i & 0x1) ? 8 : 4, 1);
        check_texture_sub_resource_color(tex_2d, i, NULL, test_tex_2d_array_colors[i], 0);
    }

    /*
     * If SrcFirstElement/DstFirstElement are greater than the total number of
     * elements, element 0 is used.
     */
    set_d3dx11_texture_load_info(&load_info, NULL, NULL, 0, 0, 2, 2, 0, 0, D3DX11_DEFAULT, D3DX11_DEFAULT);
    hr = D3DX11LoadTextureFromTexture(context, (ID3D11Resource *)tex_2d, &load_info, (ID3D11Resource *)tex2_2d);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    for (i = 0; i < 2; ++i)
        check_texture_sub_resource_color(tex2_2d, i, NULL, test_tex_2d_array_colors[i], 0);

    set_d3dx11_texture_load_info(&load_info, NULL, NULL, 0, 0, 2, 1, 0, 0, D3DX11_DEFAULT, D3DX11_DEFAULT);
    hr = D3DX11LoadTextureFromTexture(context, (ID3D11Resource *)tex_2d, &load_info, (ID3D11Resource *)tex2_2d);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    for (i = 0; i < 2; ++i)
        check_texture_sub_resource_color(tex2_2d, i, NULL, test_tex_2d_array_colors[2 + i], 0);

    for (i = 0; i < 4; ++i)
    {
        set_texture_sub_resource_color(context, (ID3D11Resource *)tex2_2d, i, 0xff000000 | (0xff << (8 * i)), 8 >> i, 8 >> i, 1);
        check_texture_sub_resource_color(tex2_2d, i, NULL, 0xff000000 | (0xff << (8 * i)), 0);
    }

    /* DstFirstElement value tests. */
    set_d3dx11_texture_load_info(&load_info, NULL, NULL, 0, 0, 2, 0, 2, 0, D3DX11_DEFAULT, D3DX11_DEFAULT);
    hr = D3DX11LoadTextureFromTexture(context, (ID3D11Resource *)tex2_2d, &load_info, (ID3D11Resource *)tex_2d);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    for (i = 0; i < 2; ++i)
        check_texture_sub_resource_color(tex_2d, i, NULL, 0xff000000 | (0xff << (8 * i)), 0);
    for (i = 0; i < 2; ++i)
        check_texture_sub_resource_color(tex_2d, 2 + i, NULL, test_tex_2d_array_colors[2 + i], 0);

    set_d3dx11_texture_load_info(&load_info, NULL, NULL, 0, 0, 2, 0, 1, 0, D3DX11_DEFAULT, D3DX11_DEFAULT);
    hr = D3DX11LoadTextureFromTexture(context, (ID3D11Resource *)tex2_2d, &load_info, (ID3D11Resource *)tex_2d);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    for (i = 0; i < 4; ++i)
        check_texture_sub_resource_color(tex_2d, i, NULL, !(i & 0x01) ? 0xff0000ff : 0xff00ff00, 0);

    for (i = 0; i < 4; ++i)
    {
        set_texture_sub_resource_color(context, (ID3D11Resource *)tex_2d, i, test_tex_2d_array_colors[i], !(i & 0x1) ? 8 : 4,
                !(i & 0x1) ? 8 : 4, 1);
        check_texture_sub_resource_color(tex_2d, i, NULL, test_tex_2d_array_colors[i], 0);

        set_texture_sub_resource_color(context, (ID3D11Resource *)tex2_2d, i, 0xff000000 | (0xff << (8 * i)), 8 >> i, 8 >> i, 1);
        check_texture_sub_resource_color(tex2_2d, i, NULL, 0xff000000 | (0xff << (8 * i)), 0);
    }

    /*
     * If SrcFirstMip is greater than the total number of mip levels, the
     * final mip level is used, E.g, if SrcFirstMip is set to 3, but the
     * texture only has 2 mip levels, mip level 2 is used.
     */
    set_d3dx11_texture_load_info(&load_info, NULL, NULL, 2, 0, 1, 0, 0, 0, D3DX11_DEFAULT, D3DX11_DEFAULT);
    hr = D3DX11LoadTextureFromTexture(context, (ID3D11Resource *)tex_2d, &load_info, (ID3D11Resource *)tex2_2d);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_texture_sub_resource_color(tex2_2d, 0, NULL, test_tex_2d_array_colors[1], 0);

    set_d3dx11_texture_load_info(&load_info, NULL, NULL, 0, 0, 1, 0, 0, 0, D3DX11_DEFAULT, D3DX11_DEFAULT);
    hr = D3DX11LoadTextureFromTexture(context, (ID3D11Resource *)tex_2d, &load_info, (ID3D11Resource *)tex2_2d);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_texture_sub_resource_color(tex2_2d, 0, NULL, test_tex_2d_array_colors[0], 0);

    set_d3dx11_texture_load_info(&load_info, NULL, NULL, 1, 0, 1, 0, 0, 0, D3DX11_DEFAULT, D3DX11_DEFAULT);
    hr = D3DX11LoadTextureFromTexture(context, (ID3D11Resource *)tex_2d, &load_info, (ID3D11Resource *)tex2_2d);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_texture_sub_resource_color(tex2_2d, 0, NULL, test_tex_2d_array_colors[1], 0);

    /*
     * If DstFirstMip is greater than the total number of mips, nothing is
     * loaded.
     */
    set_d3dx11_texture_load_info(&load_info, NULL, NULL, 0, 5, 1, 0, 0, 0, D3DX11_DEFAULT, D3DX11_DEFAULT);
    hr = D3DX11LoadTextureFromTexture(context, (ID3D11Resource *)tex_2d, &load_info, (ID3D11Resource *)tex2_2d);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_texture_sub_resource_color(tex2_2d, 0, NULL, test_tex_2d_array_colors[1], 0);
    check_texture_sub_resource_color(tex2_2d, 3, NULL, 0xff000000, 0);

    set_d3dx11_texture_load_info(&load_info, NULL, NULL, 0, 3, 1, 0, 0, 0, D3DX11_DEFAULT, D3DX11_DEFAULT);
    hr = D3DX11LoadTextureFromTexture(context, (ID3D11Resource *)tex_2d, &load_info, (ID3D11Resource *)tex2_2d);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_texture_sub_resource_color(tex2_2d, 3, NULL, test_tex_2d_array_colors[0], 0);

    ID3D11Texture2D_Release(tex2_2d);
    ID3D11Texture2D_Release(tex_2d);

    for (i = 0; i < ARRAY_SIZE(test_texture_format_conversion); ++i)
    {
        const struct test_texture_format_conversion *test = &test_texture_format_conversion[i];
        const D3D11_TEXTURE2D_DESC *src_desc = &test->src_desc;
        D3D11_TEXTURE2D_DESC tmp_desc = test->src_desc;
        ID3D11Texture2D *src_texture, *dst_texture;
        D3D11_MAPPED_SUBRESOURCE map = { 0 };

        winetest_push_context("Texture format conversion test %u", i);
        src_texture = dst_texture = NULL;

        init_subresource_data(sub_resource_data, test->src_data, src_desc->Width, src_desc->Height, 1,
                src_desc->MipLevels, src_desc->ArraySize, src_desc->Format);
        hr = ID3D11Device_CreateTexture2D(device, src_desc, sub_resource_data, &src_texture);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        if (FAILED(hr))
            goto cleanup_next_texture;

        tmp_desc.Format = test->dst_format;
        tmp_desc.Usage = D3D11_USAGE_STAGING;
        tmp_desc.BindFlags = 0;
        tmp_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        hr = ID3D11Device_CreateTexture2D(device, &tmp_desc, NULL, &dst_texture);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        if (FAILED(hr))
            goto cleanup_next_texture;

        hr = D3DX11LoadTextureFromTexture(context, (ID3D11Resource *)src_texture, NULL, (ID3D11Resource *)dst_texture);
        todo_wine_if(test->todo_hr) ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        if (FAILED(hr))
            goto cleanup_next_texture;

        hr = ID3D11DeviceContext_Map(context, (ID3D11Resource *)dst_texture, 0, D3D11_MAP_READ, 0, &map);
        ok(hr == S_OK, "Failed to map destination texture, hr %#lx.\n", hr);
        if (FAILED(hr))
            goto cleanup_next_texture;

        check_texture_data_level(map.pData, map.RowPitch, 0, tmp_desc.Width, tmp_desc.Height, 1, test->dst_format,
                test->expected_dst_data, test->max_diff, test->todo_data);
cleanup_next_texture:
        if (src_texture)
            ID3D11Texture2D_Release(src_texture);
        if (map.pData)
            ID3D11DeviceContext_Unmap(context, (ID3D11Resource *)dst_texture, 0);
        if (dst_texture)
            ID3D11Texture2D_Release(dst_texture);
        winetest_pop_context();
    }

    /*
     * Texture compression tests. Rather than checking against compressed values
     * (which we're unlikely to match due to differences in compression
     * algorithms), we'll:
     * Load our uncompressed source texture into a compressed destination texture.
     * Load our compressed destination texture into a new decompressed destination texture.
     * Check that the data in the new decompressed texture matches the
     * original data.
     */
    for (i = 0; i < ARRAY_SIZE(test_texture_compression); ++i)
    {
        const struct test_texture_compression *test = &test_texture_compression[i];
        const D3D11_TEXTURE2D_DESC *src_desc = &test->src_desc;
        ID3D11Texture2D *src_texture, *dst_texture;
        D3D11_TEXTURE2D_DESC tmp_desc = *src_desc;
        D3D11_MAPPED_SUBRESOURCE map = { 0 };

        winetest_push_context("Texture compression test %u", i);
        src_texture = dst_texture = NULL;

        /* Create the uncompressed source texture. */
        init_subresource_data(sub_resource_data, (const void *)test->src_data, src_desc->Width, src_desc->Height, 1,
                src_desc->MipLevels, src_desc->ArraySize, src_desc->Format);
        hr = ID3D11Device_CreateTexture2D(device, src_desc, sub_resource_data, &src_texture);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        if (FAILED(hr))
            goto cleanup_next_compressed_texture;

        /* Create the compressed destination texture. */
        tmp_desc.Format = test->compressed_format;
        hr = ID3D11Device_CreateTexture2D(device, &tmp_desc, NULL, &dst_texture);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        if (FAILED(hr))
            goto cleanup_next_compressed_texture;

        /* Load the uncompressed source into the compressed destination. */
        hr = D3DX11LoadTextureFromTexture(context, (ID3D11Resource *)src_texture, NULL, (ID3D11Resource *)dst_texture);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        if (FAILED(hr))
            goto cleanup_next_compressed_texture;

        ID3D11Texture2D_Release(src_texture);
        src_texture = dst_texture;

        /* Now create the uncompressed destination texture to decompress into. */
        tmp_desc = *src_desc;
        tmp_desc.Usage = D3D11_USAGE_STAGING;
        tmp_desc.BindFlags = 0;
        tmp_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        memset(tmp_buf, 0, sizeof(tmp_buf));
        init_subresource_data(sub_resource_data, (const void *)tmp_buf, src_desc->Width, src_desc->Height, 1,
                src_desc->MipLevels, src_desc->ArraySize, src_desc->Format);
        hr = ID3D11Device_CreateTexture2D(device, &tmp_desc, sub_resource_data, &dst_texture);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        if (FAILED(hr))
            goto cleanup_next_compressed_texture;

        hr = D3DX11LoadTextureFromTexture(context, (ID3D11Resource *)src_texture, NULL, (ID3D11Resource *)dst_texture);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
        if (FAILED(hr))
            goto cleanup_next_compressed_texture;

        hr = ID3D11DeviceContext_Map(context, (ID3D11Resource *)dst_texture, 0, D3D11_MAP_READ, 0, &map);
        ok(hr == S_OK, "Failed to map destination texture, hr %#lx.\n", hr);
        if (FAILED(hr))
            goto cleanup_next_compressed_texture;

        check_texture_data_level(map.pData, map.RowPitch, 0, tmp_desc.Width, tmp_desc.Height, 1, tmp_desc.Format,
                test->src_data, 0, FALSE);
cleanup_next_compressed_texture:
        if (src_texture)
            ID3D11Texture2D_Release(src_texture);
        if (map.pData)
            ID3D11DeviceContext_Unmap(context, (ID3D11Resource *)dst_texture, 0);
        if (dst_texture)
            ID3D11Texture2D_Release(dst_texture);
        winetest_pop_context();
    }

    /*
     * Use D3DX11LoadTextureFromTexture to generate mip levels. Some games
     * (Total War: Shogun 2) do this instead of using D3DX11FilterTexture.
     */
    set_d3dx11_texture_load_info(&load_info, NULL, NULL, 0, 1, 2, 0, 0, 1, D3DX11_FILTER_POINT, D3DX11_FILTER_NONE);
    set_d3d11_2d_texture_desc(&tex_2d_desc, 8, 8, 3, 1, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 0, D3D11_USAGE_DEFAULT,
            D3D11_BIND_SHADER_RESOURCE, 0, 0);

    memset(tmp_buf, 0, sizeof(tmp_buf));
    memcpy(tmp_buf, bc1_to_bc3_8_8_decompressed, sizeof(bc1_to_bc3_8_8_decompressed));
    init_subresource_data(sub_resource_data, (const void *)tmp_buf, tex_2d_desc.Width, tex_2d_desc.Height, 1,
            tex_2d_desc.MipLevels, tex_2d_desc.ArraySize, tex_2d_desc.Format);

    hr = ID3D11Device_CreateTexture2D(device, &tex_2d_desc, sub_resource_data, &tex_2d);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_texture_sub_resource_color(tex_2d, 1, NULL, 0x00000000, 0);
    check_texture_sub_resource_color(tex_2d, 2, NULL, 0x00000000, 0);

    hr = D3DX11LoadTextureFromTexture(context, (ID3D11Resource *)tex_2d, &load_info, (ID3D11Resource *)tex_2d);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    SetRect(&tmp_rect, 0, 0, 2, 2);
    check_texture_sub_resource_color(tex_2d, 1, &tmp_rect, 0xffff0000, 0);
    SetRect(&tmp_rect, 2, 0, 4, 2);
    check_texture_sub_resource_color(tex_2d, 1, &tmp_rect, 0xff00ff00, 0);
    SetRect(&tmp_rect, 0, 2, 2, 4);
    check_texture_sub_resource_color(tex_2d, 1, &tmp_rect, 0xff0000ff, 0);
    SetRect(&tmp_rect, 2, 2, 4, 4);
    check_texture_sub_resource_color(tex_2d, 1, &tmp_rect, 0xff000000, 0);

    SetRect(&tmp_rect, 0, 0, 1, 1);
    check_texture_sub_resource_color(tex_2d, 2, &tmp_rect, 0xffff0000, 0);
    SetRect(&tmp_rect, 1, 0, 2, 1);
    check_texture_sub_resource_color(tex_2d, 2, &tmp_rect, 0xff00ff00, 0);
    SetRect(&tmp_rect, 0, 1, 1, 2);
    check_texture_sub_resource_color(tex_2d, 2, &tmp_rect, 0xff0000ff, 0);
    SetRect(&tmp_rect, 1, 1, 2, 2);
    check_texture_sub_resource_color(tex_2d, 2, &tmp_rect, 0xff000000, 0);

    ID3D11Texture2D_Release(tex_2d);

    /* Cubemap. */
    set_d3d11_2d_texture_desc(&tex_2d_desc, 4, 4, 1, 6, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 0, D3D11_USAGE_DEFAULT,
            D3D11_BIND_SHADER_RESOURCE, 0, D3D11_RESOURCE_MISC_TEXTURECUBE);
    init_subresource_data(sub_resource_data, (const void *)rgba_4_4_cubemap, tex_2d_desc.Width, tex_2d_desc.Height, 1,
            tex_2d_desc.MipLevels, tex_2d_desc.ArraySize, tex_2d_desc.Format);

    hr = ID3D11Device_CreateTexture2D(device, &tex_2d_desc, sub_resource_data, &tex_2d);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    for (i = 0; i < 6; ++i)
        check_texture_sub_resource_color(tex_2d, i, NULL, test_cubemap_face_colors[i], 0);

    /* Load individual faces of a cubemap texture into a non-cubemap texture. */
    set_d3d11_2d_texture_desc(&tex_2d_desc, 4, 4, 3, 1, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 0, D3D11_USAGE_DEFAULT,
            D3D11_BIND_SHADER_RESOURCE, 0, 0);
    hr = ID3D11Device_CreateTexture2D(device, &tex_2d_desc, NULL, &tex2_2d);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    for (i = 0; i < 6; ++i)
    {
        /* On top of loading individual faces, we'll also generate mips. */
        set_d3dx11_texture_load_info(&load_info, NULL, NULL, 0, 0, 0, i, 0, 1, D3DX11_FILTER_POINT, D3DX11_FILTER_POINT);
        hr = D3DX11LoadTextureFromTexture(context, (ID3D11Resource *)tex_2d, &load_info, (ID3D11Resource *)tex2_2d);
        ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

        check_texture_sub_resource_color(tex2_2d, 0, NULL, test_cubemap_face_colors[i], 0);
        check_texture_sub_resource_color(tex2_2d, 1, NULL, test_cubemap_face_colors[i], 0);
        check_texture_sub_resource_color(tex2_2d, 2, NULL, test_cubemap_face_colors[i], 0);
    }

    ID3D11Texture2D_Release(tex2_2d);
    ID3D11Texture2D_Release(tex_2d);

    CoUninitialize();

    ID3D11DeviceContext_Release(context);
    ok(!ID3D11Device_Release(device), "Unexpected refcount.\n");
}

static void test_D3DX11FilterTexture(void)
{
    D3D11_SUBRESOURCE_DATA sub_resource_data[4] = { 0 };
    D3D11_TEXTURE2D_DESC tex_2d_desc;
    ID3D11DeviceContext *context;
    ID3D11Texture2D *tex_2d;
    uint8_t tmp_buf[1024];
    ID3D11Device *device;
    RECT tmp_rect;
    HRESULT hr;

    device = create_device();
    if (!device)
    {
        skip("Failed to create device, skipping tests.\n");
        return;
    }

    CoInitialize(NULL);

    ID3D11Device_GetImmediateContext(device, &context);
    set_d3d11_2d_texture_desc(&tex_2d_desc, 8, 8, 4, 1, DXGI_FORMAT_R8G8B8A8_UNORM, 1, 0, D3D11_USAGE_DEFAULT,
            D3D11_BIND_SHADER_RESOURCE, 0, 0);
    memset(tmp_buf, 0, sizeof(tmp_buf));
    memcpy(tmp_buf, bc1_to_bc3_8_8_decompressed, sizeof(bc1_to_bc3_8_8_decompressed));
    init_subresource_data(sub_resource_data, (const void *)tmp_buf, tex_2d_desc.Width, tex_2d_desc.Height, 1,
            tex_2d_desc.MipLevels, tex_2d_desc.ArraySize, tex_2d_desc.Format);

    hr = ID3D11Device_CreateTexture2D(device, &tex_2d_desc, sub_resource_data, &tex_2d);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);
    check_texture_sub_resource_color(tex_2d, 1, NULL, 0x00000000, 0);
    check_texture_sub_resource_color(tex_2d, 2, NULL, 0x00000000, 0);
    check_texture_sub_resource_color(tex_2d, 3, NULL, 0x00000000, 0);

    /* Invalid filter arguments. */
    hr = D3DX11FilterTexture(context, (ID3D11Resource *)tex_2d, 0, 0);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);

    hr = D3DX11FilterTexture(context, (ID3D11Resource *)tex_2d, 0, 9);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);

    /*
     * Filter argument isn't validated if src_level argument is greater than
     * total mip levels.
     */
    hr = D3DX11FilterTexture(context, (ID3D11Resource *)tex_2d, 5, 9);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    /* NULL device context. */
    hr = D3DX11FilterTexture(NULL, (ID3D11Resource *)tex_2d, 5, 9);
    ok(hr == D3DERR_INVALIDCALL, "Unexpected hr %#lx.\n", hr);

    hr = D3DX11FilterTexture(context, (ID3D11Resource *)tex_2d, 0, D3DX11_FILTER_POINT);
    ok(hr == S_OK, "Unexpected hr %#lx.\n", hr);

    SetRect(&tmp_rect, 0, 0, 2, 2);
    check_texture_sub_resource_color(tex_2d, 1, &tmp_rect, 0xffff0000, 0);
    SetRect(&tmp_rect, 2, 0, 4, 2);
    check_texture_sub_resource_color(tex_2d, 1, &tmp_rect, 0xff00ff00, 0);
    SetRect(&tmp_rect, 0, 2, 2, 4);
    check_texture_sub_resource_color(tex_2d, 1, &tmp_rect, 0xff0000ff, 0);
    SetRect(&tmp_rect, 2, 2, 4, 4);
    check_texture_sub_resource_color(tex_2d, 1, &tmp_rect, 0xff000000, 0);

    SetRect(&tmp_rect, 0, 0, 1, 1);
    check_texture_sub_resource_color(tex_2d, 2, &tmp_rect, 0xffff0000, 0);
    SetRect(&tmp_rect, 1, 0, 2, 1);
    check_texture_sub_resource_color(tex_2d, 2, &tmp_rect, 0xff00ff00, 0);
    SetRect(&tmp_rect, 0, 1, 1, 2);
    check_texture_sub_resource_color(tex_2d, 2, &tmp_rect, 0xff0000ff, 0);
    SetRect(&tmp_rect, 1, 1, 2, 2);
    check_texture_sub_resource_color(tex_2d, 2, &tmp_rect, 0xff000000, 0);

    check_texture_sub_resource_color(tex_2d, 3, NULL, 0xffff0000, 0);

    ID3D11Texture2D_Release(tex_2d);

    CoUninitialize();

    ID3D11DeviceContext_Release(context);
    ok(!ID3D11Device_Release(device), "Unexpected refcount.\n");
}

START_TEST(d3dx11)
{
    HMODULE wined3d;

    if ((wined3d = GetModuleHandleA("wined3d.dll")))
    {
        enum wined3d_renderer (CDECL *p_wined3d_get_renderer)(void);

        if ((p_wined3d_get_renderer = (void *)GetProcAddress(wined3d, "wined3d_get_renderer"))
                && p_wined3d_get_renderer() == WINED3D_RENDERER_OPENGL)
            wined3d_opengl = true;
    }

    test_D3DX11CreateAsyncMemoryLoader();
    test_D3DX11CreateAsyncFileLoader();
    test_D3DX11CreateAsyncResourceLoader();
    test_D3DX11CreateAsyncTextureInfoProcessor();
    test_D3DX11CreateAsyncTextureProcessor();
    test_D3DX11CreateAsyncShaderResourceViewProcessor();
    test_D3DX11CompileFromFile();
    test_D3DX11LoadTextureFromTexture();
    test_D3DX11FilterTexture();
    test_D3DX11CreateThreadPump();
    test_get_image_info();
    test_create_texture();
    test_create_shader_resource_view();
}
