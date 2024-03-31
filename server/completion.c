/*
 * Server-side IO completion ports implementation
 *
 * Copyright (C) 2007 Andrey Turkin
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

/* FIXMEs:
 *  - built-in wait queues used which means:
 *    + threads are awaken FIFO and not LIFO as native does
 *    + "max concurrent active threads" parameter not used
 *    + completion handle is waitable, while native isn't
 */

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"

#include "object.h"
#include "file.h"
#include "handle.h"
#include "request.h"
#include "esync.h"
#include "fsync.h"

static const WCHAR completion_name[] = {'I','o','C','o','m','p','l','e','t','i','o','n'};

struct type_descr completion_type =
{
    { completion_name, sizeof(completion_name) },   /* name */
    IO_COMPLETION_ALL_ACCESS,                       /* valid_access */
    {                                               /* mapping */
        STANDARD_RIGHTS_READ | IO_COMPLETION_QUERY_STATE,
        STANDARD_RIGHTS_WRITE | IO_COMPLETION_MODIFY_STATE,
        STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
        IO_COMPLETION_ALL_ACCESS
    },
};

struct completion;

struct completion_wait
{
    struct object      obj;
    struct completion *completion;
    struct list        queue;
    unsigned int       depth;
    int                esync_fd;
    unsigned int       fsync_idx;
};

struct completion
{
    struct object           obj;
    struct completion_wait *wait;
};

static void completion_wait_dump( struct object*, int );
static int completion_wait_signaled( struct object *obj, struct wait_queue_entry *entry );
static void completion_wait_satisfied( struct object *obj, struct wait_queue_entry *entry );
static int completion_wait_get_esync_fd( struct object *obj, enum esync_type *type );
static unsigned int completion_wait_get_fsync_idx( struct object *obj, enum fsync_type *type );
static void completion_wait_destroy( struct object * );

static const struct object_ops completion_wait_ops =
{
    sizeof(struct completion_wait), /* size */
    &no_type,                       /* type */
    completion_wait_dump,           /* dump */
    add_queue,                      /* add_queue */
    remove_queue,                   /* remove_queue */
    completion_wait_signaled,       /* signaled */
    completion_wait_get_esync_fd,   /* get_esync_fd */
    completion_wait_get_fsync_idx,  /* get_fsync_idx */
    completion_wait_satisfied,      /* satisfied */
    no_signal,                      /* signal */
    no_get_fd,                      /* get_fd */
    default_map_access,             /* map_access */
    default_get_sd,                 /* get_sd */
    default_set_sd,                 /* set_sd */
    no_get_full_name,               /* get_full_name */
    no_lookup_name,                 /* lookup_name */
    no_link_name,                   /* link_name */
    NULL,                           /* unlink_name */
    no_open_file,                   /* open_file */
    no_kernel_obj_list,             /* get_kernel_obj_list */
    no_close_handle,                /* close_handle */
    completion_wait_destroy         /* destroy */
};

static void completion_dump( struct object*, int );
static int completion_add_queue( struct object *obj, struct wait_queue_entry *entry );
static void completion_remove_queue( struct object *obj, struct wait_queue_entry *entry );
static int completion_get_esync_fd( struct object *obj, enum esync_type *type );
static unsigned int completion_get_fsync_idx( struct object *obj, enum fsync_type *type );
static void completion_destroy( struct object * );

static const struct object_ops completion_ops =
{
    sizeof(struct completion), /* size */
    &completion_type,          /* type */
    completion_dump,           /* dump */
    completion_add_queue,      /* add_queue */
    completion_remove_queue,   /* remove_queue */
    NULL,                      /* signaled */
    completion_get_esync_fd,   /* get_esync_fd */
    completion_get_fsync_idx,  /* get_fsync_idx */
    no_satisfied,              /* satisfied */
    no_signal,                 /* signal */
    no_get_fd,                 /* get_fd */
    default_map_access,        /* map_access */
    default_get_sd,            /* get_sd */
    default_set_sd,            /* set_sd */
    default_get_full_name,     /* get_full_name */
    no_lookup_name,            /* lookup_name */
    directory_link_name,       /* link_name */
    default_unlink_name,       /* unlink_name */
    no_open_file,              /* open_file */
    no_kernel_obj_list,        /* get_kernel_obj_list */
    no_close_handle,           /* close_handle */
    completion_destroy         /* destroy */
};

struct comp_msg
{
    struct   list queue_entry;
    apc_param_t   ckey;
    apc_param_t   cvalue;
    apc_param_t   information;
    unsigned int  status;
};

static void completion_wait_destroy( struct object *obj)
{
    struct completion_wait *wait = (struct completion_wait *)obj;
    struct comp_msg *tmp, *next;

    LIST_FOR_EACH_ENTRY_SAFE( tmp, next, &wait->queue, struct comp_msg, queue_entry )
    {
        free( tmp );
    }

    if (do_esync())
        close( wait->esync_fd );

    if (wait->fsync_idx) fsync_free_shm_idx( wait->fsync_idx );
}

static void completion_wait_dump( struct object *obj, int verbose )
{
    struct completion_wait *wait = (struct completion_wait *)obj;

    assert( obj->ops == &completion_wait_ops );
    fprintf( stderr, "Completion depth=%u\n", wait->depth );
}

static int completion_wait_signaled( struct object *obj, struct wait_queue_entry *entry )
{
    struct completion_wait *wait = (struct completion_wait *)obj;

    assert( obj->ops == &completion_wait_ops );
    return !wait->completion || !list_empty( &wait->queue );
}

static int completion_wait_get_esync_fd( struct object *obj, enum esync_type *type )
{
    struct completion_wait *wait = (struct completion_wait *)obj;

    *type = ESYNC_MANUAL_SERVER;
    return wait->esync_fd;
}

static unsigned int completion_wait_get_fsync_idx( struct object *obj, enum fsync_type *type )
{
    struct completion_wait *wait = (struct completion_wait *)obj;

    assert( obj->ops == &completion_wait_ops );
    *type = FSYNC_MANUAL_SERVER;
    return wait->fsync_idx;
}

static void completion_wait_satisfied( struct object *obj, struct wait_queue_entry *entry )
{
    struct completion_wait *wait = (struct completion_wait *)obj;
    struct thread *thread;

    assert( obj->ops == &completion_wait_ops );
    if (wait->completion)
    {
        thread = get_wait_queue_thread( entry );
        if (thread->locked_completion) release_object( thread->locked_completion );
        thread->locked_completion = grab_object( obj );
    }
    else make_wait_abandoned( entry );
}

static void completion_dump( struct object *obj, int verbose )
{
    struct completion *completion = (struct completion *)obj;

    assert( obj->ops == &completion_ops );
    completion->wait->obj.ops->dump( &completion->wait->obj, verbose );
}

static int completion_add_queue( struct object *obj, struct wait_queue_entry *entry )
{
    struct completion *completion = (struct completion *)obj;

    assert( obj->ops == &completion_ops );
    return completion->wait->obj.ops->add_queue( &completion->wait->obj, entry );
}

static void completion_remove_queue( struct object *obj, struct wait_queue_entry *entry )
{
    struct completion *completion = (struct completion *)obj;

    assert( obj->ops == &completion_ops );
    completion->wait->obj.ops->remove_queue( &completion->wait->obj, entry );
}

static int completion_get_esync_fd( struct object *obj, enum esync_type *type )
{
    struct completion *completion = (struct completion *)obj;

    assert( obj->ops == &completion_ops );
    return completion->wait->obj.ops->get_esync_fd( &completion->wait->obj, type );
}

static unsigned int completion_get_fsync_idx( struct object *obj, enum fsync_type *type )
{
    struct completion *completion = (struct completion *)obj;

    assert( obj->ops == &completion_ops );
    return completion->wait->obj.ops->get_fsync_idx( &completion->wait->obj, type );
}

static void completion_destroy( struct object *obj )
{
    struct completion *completion = (struct completion *)obj;

    assert( obj->ops == &completion_ops );
    completion->wait->completion = NULL;
    wake_up( &completion->wait->obj, 0 );
    release_object( &completion->wait->obj );
}

static struct completion *create_completion( struct object *root, const struct unicode_str *name,
                                             unsigned int attr, unsigned int concurrent,
                                             const struct security_descriptor *sd )
{
    struct completion *completion;

    if (!(completion = create_named_object( root, &completion_ops, name, attr, sd ))) return NULL;
    if (get_error() == STATUS_OBJECT_NAME_EXISTS) return completion;
    if (!(completion->wait = alloc_object( &completion_wait_ops )))
    {
        release_object( completion );
        set_error( STATUS_NO_MEMORY );
        return NULL;
    }

    completion->wait->completion = completion;
    list_init( &completion->wait->queue );
    completion->wait->depth = 0;
    completion->wait->fsync_idx = 0;

    if (do_fsync())
        completion->wait->fsync_idx = fsync_alloc_shm( 0, 0 );

    if (do_esync())
        completion->wait->esync_fd = esync_create_fd( 0, 0 );

    return completion;
}

struct completion *get_completion_obj( struct process *process, obj_handle_t handle, unsigned int access )
{
    return (struct completion *) get_handle_obj( process, handle, access, &completion_ops );
}

void add_completion( struct completion *completion, apc_param_t ckey, apc_param_t cvalue,
                     unsigned int status, apc_param_t information )
{
    struct comp_msg *msg = mem_alloc( sizeof( *msg ) );

    if (!msg)
        return;

    msg->ckey = ckey;
    msg->cvalue = cvalue;
    msg->status = status;
    msg->information = information;

    list_add_tail( &completion->wait->queue, &msg->queue_entry );
    completion->wait->depth++;

    wake_up( &completion->wait->obj, 1 );
}

/* create a completion */
DECL_HANDLER(create_completion)
{
    struct completion *completion;
    struct unicode_str name;
    struct object *root;
    const struct security_descriptor *sd;
    const struct object_attributes *objattr = get_req_object_attributes( &sd, &name, &root );

    if (!objattr) return;

    if ((completion = create_completion( root, &name, objattr->attributes, req->concurrent, sd )))
    {
        reply->handle = alloc_handle( current->process, completion, req->access, objattr->attributes );
        release_object( completion );
    }

    if (root) release_object( root );
}

/* open a completion */
DECL_HANDLER(open_completion)
{
    struct unicode_str name = get_req_unicode_str();

    reply->handle = open_object( current->process, req->rootdir, req->access,
                                 &completion_ops, &name, req->attributes );
}


/* add completion to completion port */
DECL_HANDLER(add_completion)
{
    struct completion* completion = get_completion_obj( current->process, req->handle, IO_COMPLETION_MODIFY_STATE );

    if (!completion) return;

    add_completion( completion, req->ckey, req->cvalue, req->status, req->information );

    release_object( completion );
}

/* get completion from completion port */
DECL_HANDLER(remove_completion)
{
    struct completion* completion;
    struct completion_wait *wait;
    struct list *entry;
    struct comp_msg *msg;

    if (req->waited && (wait = (struct completion_wait *)current->locked_completion))
        current->locked_completion = NULL;
    else
    {
        if (current->locked_completion)
        {
            release_object( current->locked_completion );
            current->locked_completion = NULL;
        }
        completion = get_completion_obj( current->process, req->handle, IO_COMPLETION_MODIFY_STATE );
        if (!completion) return;

        wait = (struct completion_wait *)grab_object( completion->wait );
        release_object( completion );
    }

    assert( wait->obj.ops == &completion_wait_ops );

    entry = list_head( &wait->queue );
    if (!entry)
    {
        if (wait->completion)
        {
            if (do_fsync() || do_esync())
            {
                /* completion_wait_satisfied is not called, so lock completion here. */
                current->locked_completion = grab_object( wait );
            }
            set_error( STATUS_PENDING );
        }
        else set_error( STATUS_ABANDONED_WAIT_0 );
    }
    else
    {
        list_remove( entry );
        wait->depth--;
        msg = LIST_ENTRY( entry, struct comp_msg, queue_entry );
        reply->ckey = msg->ckey;
        reply->cvalue = msg->cvalue;
        reply->status = msg->status;
        reply->information = msg->information;
        free( msg );

        if (!completion_wait_signaled( &wait->obj, NULL ))
        {
            if (do_fsync())
                fsync_clear( &wait->obj );

            if (do_esync())
                esync_clear( wait->esync_fd );
        }
    }

    release_object( wait );
}

/* get queue depth for completion port */
DECL_HANDLER(query_completion)
{
    struct completion* completion = get_completion_obj( current->process, req->handle, IO_COMPLETION_QUERY_STATE );

    if (!completion) return;

    reply->depth = completion->wait->depth;

    release_object( completion );
}
