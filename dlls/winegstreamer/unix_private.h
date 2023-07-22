/*
 * winegstreamer Unix library interface
 *
 * Copyright 2020-2021 Zebediah Figura for CodeWeavers
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

#ifndef __WINE_WINEGSTREAMER_UNIX_PRIVATE_H
#define __WINE_WINEGSTREAMER_UNIX_PRIVATE_H

#include "unixlib.h"

#include <gst/gst.h>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

/* unixlib.c */

GST_DEBUG_CATEGORY_EXTERN(wine) DECLSPEC_HIDDEN;
#define GST_CAT_DEFAULT wine

extern NTSTATUS wg_init_gstreamer(void *args) DECLSPEC_HIDDEN;

extern GstStreamType stream_type_from_caps(GstCaps *caps) DECLSPEC_HIDDEN;
extern GstElement *create_element(const char *name, const char *plugin_set) DECLSPEC_HIDDEN;
extern GstElement *find_element(GstElementFactoryListType type, GstCaps *src_caps, GstCaps *sink_caps) DECLSPEC_HIDDEN;
extern bool append_element(GstElement *container, GstElement *element, GstElement **first, GstElement **last) DECLSPEC_HIDDEN;
extern bool link_src_to_element(GstPad *src_pad, GstElement *element) DECLSPEC_HIDDEN;
extern bool link_element_to_sink(GstElement *element, GstPad *sink_pad) DECLSPEC_HIDDEN;
extern GstCaps *detect_caps_from_data(const char *url, const void *data, guint size) DECLSPEC_HIDDEN;
extern GstPad *create_pad_with_caps(GstPadDirection direction, GstCaps *caps) DECLSPEC_HIDDEN;
extern GstBuffer *create_buffer_from_bytes(const void *data, guint size) DECLSPEC_HIDDEN;
extern gchar *stream_lang_from_tags(GstTagList *tags, GstCaps *caps) DECLSPEC_HIDDEN;
extern gchar *stream_name_from_tags(GstTagList *tags) DECLSPEC_HIDDEN;

/* wg_format.c */

extern void wg_format_from_caps(struct wg_format *format, const GstCaps *caps) DECLSPEC_HIDDEN;
extern bool wg_format_compare(const struct wg_format *a, const struct wg_format *b) DECLSPEC_HIDDEN;
extern GstCaps *wg_format_to_caps(const struct wg_format *format) DECLSPEC_HIDDEN;

extern gchar *wg_stream_lang_from_tags(GstTagList *tags, GstCaps *caps) DECLSPEC_HIDDEN;
extern gchar *wg_stream_name_from_tags(GstTagList *tags) DECLSPEC_HIDDEN;

/* wg_transform.c */

extern NTSTATUS wg_transform_create(void *args) DECLSPEC_HIDDEN;
extern NTSTATUS wg_transform_destroy(void *args) DECLSPEC_HIDDEN;
extern NTSTATUS wg_transform_set_output_format(void *args) DECLSPEC_HIDDEN;
extern NTSTATUS wg_transform_push_data(void *args) DECLSPEC_HIDDEN;
extern NTSTATUS wg_transform_read_data(void *args) DECLSPEC_HIDDEN;
extern NTSTATUS wg_transform_get_status(void *args) DECLSPEC_HIDDEN;
extern NTSTATUS wg_transform_drain(void *args) DECLSPEC_HIDDEN;
extern NTSTATUS wg_transform_flush(void *args) DECLSPEC_HIDDEN;

/* wg_source.c */

extern NTSTATUS wg_source_create(void *args) DECLSPEC_HIDDEN;
extern NTSTATUS wg_source_destroy(void *args) DECLSPEC_HIDDEN;
extern NTSTATUS wg_source_get_status(void *args) DECLSPEC_HIDDEN;
extern NTSTATUS wg_source_push_data(void *args) DECLSPEC_HIDDEN;
extern NTSTATUS wg_source_get_stream_format(void *args) DECLSPEC_HIDDEN;
extern NTSTATUS wg_source_get_stream_tag(void *args) DECLSPEC_HIDDEN;

/* wg_task_pool.c */

extern GstTaskPool *wg_task_pool_new(void) DECLSPEC_HIDDEN;

/* wg_allocator.c */

/* wg_allocator_release_sample can be used to release any sample that was requested. */
typedef struct wg_sample *(*wg_allocator_request_sample_cb)(gsize size, void *context);
extern GstAllocator *wg_allocator_create(wg_allocator_request_sample_cb request_sample,
        void *request_sample_context) DECLSPEC_HIDDEN;
extern void wg_allocator_destroy(GstAllocator *allocator) DECLSPEC_HIDDEN;
extern void wg_allocator_release_sample(GstAllocator *allocator, struct wg_sample *sample,
        bool discard_data) DECLSPEC_HIDDEN;

static inline void touch_h264_used_tag(void)
{
    const char *e;

    GST_LOG("h264 is used");

    if ((e = getenv("STEAM_COMPAT_SHADER_PATH")))
    {
        char buffer[PATH_MAX];
        int fd;

        snprintf(buffer, sizeof(buffer), "%s/h264-used", e);

        fd = open(buffer, O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
        if (fd == -1)
        {
            GST_WARNING("Failed to open/create \"%s/h264-used\"", e);
            return;
        }

        futimens(fd, NULL);

        close(fd);
    }
    else
    {
        GST_WARNING("STEAM_COMPAT_SHADER_PATH not set, cannot create h264-used file");
    }
}

#endif /* __WINE_WINEGSTREAMER_UNIX_PRIVATE_H */
