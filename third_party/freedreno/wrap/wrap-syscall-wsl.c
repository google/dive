/*
 * Copyright © 2023 Igalia S.L.
 * SPDX-License-Identifier: MIT
 */

#include "wrap.h"

#define __KERNEL__
#include "d3dkmthk.h"

/* Compared to kgsl libwrap:
 * - Hooks to libdxcore.so calls instead of IOCTLs
 *    because libdxcore calls are nicer to deal with
 * - DOESN'T support faking GPU.
 */

void *__rd_dlsym_wsl_helper(const char *name) {
  static void *libc_dl;
  void *func;

  if (!libc_dl)
    libc_dl = dlopen("libdxcore.so", RTLD_LAZY);
  if (!libc_dl)
    libc_dl = dlopen("/usr/lib/wsl/lib/libdxcore.so", RTLD_LAZY);

  if (!libc_dl) {
    printf("Failed to dlopen libdxcore: %s\n", dlerror());
    exit(-1);
  }

  func = dlsym(libc_dl, name);

  if (!func) {
    printf("Failed to find %s: %s\n", name, dlerror());
    exit(-1);
  }

  return func;
}

#define WSL_PROLOG(func)                                                       \
  static typeof(func) *orig_##func = NULL;                                     \
  if (!orig_##func)                                                            \
    orig_##func = __rd_dlsym_wsl_helper(#func);

#ifdef USE_PTHREADS
static pthread_mutex_t l = PTHREAD_RECURSIVE_MUTEX_INITIALIZER;
#define LOCK()   pthread_mutex_lock(&l)
#define UNLOCK() pthread_mutex_unlock(&l)
#else
#define LOCK()
#define UNLOCK()
#endif

typedef long NTSTATUS;

static const struct d3dkmthandle NULL_HANDLE = {.v = 0};

struct buffer {
    struct d3dkmthandle handle;
    void *hostptr;
    unsigned int len;
    uint64_t gpuaddr;
    struct list node;
};

static struct buffer *register_buffer(struct d3dkmthandle device,
                                      unsigned int len,
                                      struct d3dkmthandle handle)
{
    struct list *buffers_of_interest =
        wrap_get_buffers_of_interest(device.v);
    assert(buffers_of_interest);
    struct buffer *buf = calloc(1, sizeof *buf);
    buf->len = len;
    buf->handle = handle;
    list_add(&buf->node, buffers_of_interest);
    return buf;
}

static struct buffer *find_buffer(struct d3dkmthandle device, struct d3dkmthandle alloc, uint64_t gpuaddr)
{
    struct buffer *buf = NULL;
    struct list *buffers_of_interest = wrap_get_buffers_of_interest(device.v);
    if(!buffers_of_interest)
        return NULL;
    list_for_each_entry(buf, buffers_of_interest, node) {
        if (alloc.v)
            if (buf->handle.v == alloc.v)
                return buf;

        if (gpuaddr)
            if ((buf->gpuaddr <= gpuaddr) && (gpuaddr < (buf->gpuaddr + buf->len)))
                return buf;
    }
    return NULL;
}

static struct buffer *find_buffer_all(struct d3dkmthandle alloc, uint64_t gpuaddr) {
    int v;
    int index = 0;
    while(-1 != (index = wrap_get_next_fd(index, &v))) {
        struct d3dkmthandle ctx = {.v = v};
        struct buffer *buf = find_buffer(ctx, alloc, gpuaddr);
        if(buf)
            return buf;
    }

    return NULL;
}

static void unregister_buffer(struct buffer *buf)
{
    if (buf) {
        list_del(&buf->node);
        free(buf);
    }
}

struct device_ctx {
    struct d3dkmthandle device;
    struct d3dkmthandle ctx;
};

#define MAX_CONTEXTS 64
static struct device_ctx device_contexts[MAX_CONTEXTS] = {
    [0 ... MAX_CONTEXTS-1] = {NULL_HANDLE, NULL_HANDLE}
};

static void register_ctx(struct d3dkmthandle device, struct d3dkmthandle ctx)
{
    for (unsigned i = 0; i < MAX_CONTEXTS; i++) {
        if (device_contexts[i].ctx.v == NULL_HANDLE.v) {
            device_contexts[i].device = device;
            device_contexts[i].ctx = ctx;
            return;
        }
    }
    fprintf(stderr, "ERROR: too many contexts!\n");
}

static void unregister_ctx(struct d3dkmthandle ctx)
{
    for (unsigned i = 0; i < MAX_CONTEXTS; i++) {
        if (device_contexts[i].ctx.v == ctx.v) {
            device_contexts[i].device = NULL_HANDLE;
            device_contexts[i].ctx = NULL_HANDLE;
            return;
        }
    }
    fprintf(stderr, "ERROR: cannot find context!\n");
}

static struct d3dkmthandle get_device_for_ctx(struct d3dkmthandle ctx)
{
    for (unsigned i = 0; i < MAX_CONTEXTS; i++) {
        if (device_contexts[i].ctx.v == ctx.v)
            return device_contexts[i].device;
    }
    return NULL_HANDLE;
}

struct alloc_priv_info {
   __u32 struct_size;
   char _pad0[4];
   __u32 unk0; // 1
   char _pad1[4];
   __u64 size;
   __u32 alignment;
   char _pad2[20];
   __u64 allocated_size;
   __u32 unk1;   // 1
   char _pad4[8]; /* offset: 60*/
   __u32 unk2;   // 61
   char _pad5[76];
   __u32 unk3; /* offset: 148 */ // 1
   char _pad6[8];
   __u32 unk4; /* offset: 160 */ // 1
   char _pad7[44];
   __u32 unk5; /* offset: 208 */ // 3
   char _pad8[16];
   __u32 size_2; /* offset: 228 */
   __u32 unk6;   // 1
   __u32 size_3;
   __u32 size_4;
   __u32 unk7; /* offset: 244 */ // 1
   char _pad9[56];
};
static_assert(sizeof(struct alloc_priv_info) == 304);
static_assert(offsetof(struct alloc_priv_info, unk1) == 56);
static_assert(offsetof(struct alloc_priv_info, unk3) == 148);
static_assert(offsetof(struct alloc_priv_info, unk5) == 208);

struct submit_priv_ib_info {
   char _pad5[4];
   __u32 size_dwords;
   __u64 iova;
   char _pad6[8];
} __attribute__((packed));

struct submit_priv_data {
   __u32 magic0;
   char _pad0[4];
   __u32 struct_size;
   char _pad1[4];
   /* It seems that priv data can have several sub-datas
    * cmdbuf is one of them, after it there is another 8 byte struct
    * without anything useful in it.
    */
   __u32 datas_count;
   char _pad2[32];
   struct {
      __u32 magic1;
      __u32 data_size;

      struct {
         __u32 unk1;
         __u32 cmdbuf_size;
         char _pad3[32];
         __u32 ib_count;
         char _pad4[36];

         struct submit_priv_ib_info ibs[];
      } cmdbuf;
   } data0;

   //    unsigned char magic2[8];
} __attribute__((packed));
static_assert(offsetof(struct submit_priv_data, data0) == 0x34);
static_assert(offsetof(struct submit_priv_data, data0.cmdbuf.ibs) == 0x8c);

void print_pre(const char *func_name)
{
    printf("> %s\n", func_name);
}

void print_post(const char *func_name, NTSTATUS result)
{
    printf("< %s => %ld\n", func_name, result);
}

NTSTATUS D3DKMTCreateDevice(struct d3dkmt_createdevice *info)
{
    LOCK();
    WSL_PROLOG(D3DKMTCreateDevice);

    print_pre("D3DKMTCreateDevice");
    printf("\t\tflags:\t\t0x%08x\n", info->flags);

    NTSTATUS result = orig_D3DKMTCreateDevice(info);

    print_post("D3DKMTCreateDevice", result);
    printf("\t\tdevice:\t\t0x%08x\n", info->device.v);

    /* TODO: Properly fill GPU_ID and/or CHIP_ID */
    uint32_t gpu_id = 690;
    if (wrap_gpu_id()) {
        gpu_id = wrap_gpu_id();
    }
    rd_write_section(info->device.v, RD_GPU_ID, &gpu_id, sizeof(gpu_id));

    UNLOCK();
    return result;
}

NTSTATUS D3DKMTDestroyDevice(struct d3dkmt_destroydevice *info)
{
    LOCK();
    WSL_PROLOG(D3DKMTDestroyDevice);

    print_pre("D3DKMTDestroyDevice");
    printf("\t\tdevice:\t\t\t0x%08x\n", info->device.v);

    NTSTATUS result = orig_D3DKMTDestroyDevice(info);

    print_post("D3DKMTDestroyDevice", result);

    rd_end(info->device.v);

    UNLOCK();
    return result;
}

NTSTATUS D3DKMTCreateContextVirtual(struct d3dkmt_createcontextvirtual *info)
{
    LOCK();
    WSL_PROLOG(D3DKMTCreateContextVirtual);

    print_pre("D3DKMTCreateContextVirtual");
    printf("\t\tengine_affinity:\t%u\n", info->engine_affinity);
    printf("\t\tflags:\t\t\t0x%08x\n", info->flags.value);
    printf("\t\tclient_hint:\t\t%u\n", info->client_hint);
    printf("\t\tpriv_drv_data_size:\t0x%04x\n", info->priv_drv_data_size);
    hexdump(info->priv_drv_data, info->priv_drv_data_size);

    NTSTATUS result = orig_D3DKMTCreateContextVirtual(info);

    print_post("D3DKMTCreateContextVirtual", result);
    printf("\t\tcontext:\t\t0x%08x\n", info->context.v);

    register_ctx(info->device, info->context);

    UNLOCK();
    return result;
}

NTSTATUS D3DKMTDestroyContext(struct d3dkmt_destroycontext *info)
{
    LOCK();
    WSL_PROLOG(D3DKMTDestroyContext);

    print_pre("D3DKMTDestroyContext");
    printf("\t\tcontext:\t\t0x%08x\n", info->context.v);

    NTSTATUS result = orig_D3DKMTDestroyContext(info);

    print_post("D3DKMTDestroyContext", result);

    unregister_ctx(info->context);

    UNLOCK();
    return result;
}

NTSTATUS D3DKMTCreateAllocation2(struct d3dkmt_createallocation *info)
{
    LOCK();
    WSL_PROLOG(D3DKMTCreateAllocation2);

    print_pre("D3DKMTCreateAllocation2");
    printf("\t\tresource:\t\t0x%08x\n", info->resource.v);
    printf("\t\tflags:\t\t\t0x%08x\n", info->flags);
    printf("\t\talloc_count:\t\t%u\n", info->alloc_count);

    for (unsigned i = 0; i < info->alloc_count; i++) {
        struct alloc_priv_info *priv_info = info->allocation_info[i].priv_drv_data;
        printf("\t\tsize[%u]:\t\t0x%08llx\n", i, priv_info->size);
        printf("\t\taligment[%u]:\t\t0x%x\n", i, priv_info->alignment);
    }

    NTSTATUS result = orig_D3DKMTCreateAllocation2(info);

    print_post("D3DKMTCreateAllocation2", result);
    for (unsigned i = 0; i < info->alloc_count; i++) {
        struct alloc_priv_info *priv_info = info->allocation_info[i].priv_drv_data;
        printf("\t\tallocation[%u]:\t\t0x%x\n", i, info->allocation_info[i].allocation.v);
        printf("\t\tallocated_size[%u]:\t0x%08llx\n", i, priv_info->allocated_size);

        register_buffer(info->device, priv_info->size, info->allocation_info[i].allocation);
    }

    UNLOCK();
    return result;
}

NTSTATUS D3DKMTDestroyAllocation2(const struct d3dkmt_destroyallocation2 *info)
{
    LOCK();
    WSL_PROLOG(D3DKMTDestroyAllocation2);

    print_pre("D3DKMTDestroyAllocation2");
    printf("\t\tresource:\t\t0x%08x\n", info->resource.v);

    if (info->resource.v != 0) {
        /* TODO: track which allocations belong to resource in order to correctly free them */
        printf("\tD3DKMTDestroyAllocation2 is ignored because resource != 0\n");
        UNLOCK();
        return 0;
    }

    for (unsigned i = 0; i < info->alloc_count; i++) {
        printf("\t\tallocation[%u]:\t\t0x%x\n", i, info->allocations[i].v);

        struct buffer *buf = find_buffer(info->device, info->allocations[i], 0);
        if (buf) {
            unregister_buffer(buf);
        } else {
            fprintf(stderr, "\tERROR: cannot find allocation!\n");
        }
    }

    NTSTATUS result = orig_D3DKMTDestroyAllocation2(info);

    print_post("D3DKMTDestroyAllocation2", result);

    UNLOCK();
    return result;
}

NTSTATUS D3DKMTMapGpuVirtualAddress(struct d3dddi_mapgpuvirtualaddress *info)
{
    LOCK();
    WSL_PROLOG(D3DKMTMapGpuVirtualAddress);

    print_pre("D3DKMTMapGpuVirtualAddress");
    printf("\t\tallocation:\t\t0x%x\n", info->allocation.v);
    printf("\t\tbase_address:\t\t0x%08llx\n", info->base_address);
    printf("\t\tsize_in_pages:\t\t0x%08llx\n", info->size_in_pages);

    NTSTATUS result = orig_D3DKMTMapGpuVirtualAddress(info);

    print_post("D3DKMTMapGpuVirtualAddress", result);
    printf("\t\tvirtual_address:\t0x%llx\n", info->virtual_address);

    struct buffer *buf = find_buffer_all(info->allocation, 0);
    if (buf) {
        buf->gpuaddr = info->virtual_address;
    } else {
        fprintf(stderr, "\tERROR: cannot find allocation!\n");
    }

    UNLOCK();
    return result;
}

NTSTATUS D3DKMTMakeResident(struct d3dddi_makeresident *info)
{
    LOCK();
    WSL_PROLOG(D3DKMTMakeResident);

    print_pre("D3DKMTMakeResident");

    NTSTATUS result = orig_D3DKMTMakeResident(info);

    print_post("D3DKMTMakeResident", result);

    UNLOCK();
    return result;
}

NTSTATUS D3DKMTLock2(struct d3dkmt_lock2 *info)
{
    LOCK();
    WSL_PROLOG(D3DKMTLock2);

    print_pre("D3DKMTLock2");
    printf("\t\tallocation:\t\t0x%x\n", info->allocation.v);

    NTSTATUS result = orig_D3DKMTLock2(info);

    print_post("D3DKMTLock2", result);
    printf("\t\thost_address:\t\t%p\n", info->data);

    struct buffer *buf = find_buffer(info->device, info->allocation, 0);
    if (buf) {
        buf->hostptr = info->data;
    } else {
        fprintf(stderr, "\tERROR: cannot find allocation!\n");
    }

    UNLOCK();
    return result;
}

NTSTATUS D3DKMTUnlock2(struct d3dkmt_unlock2 *info)
{
    LOCK();
    WSL_PROLOG(D3DKMTUnlock2);

    print_pre("D3DKMTUnlock2");
    printf("\t\tallocation:\t\t0x%x\n", info->allocation.v);

    if (!wrap_dump_all_bo()) {
        NTSTATUS result = orig_D3DKMTUnlock2(info);

        print_post("D3DKMTUnlock2", result);

        struct buffer *buf = find_buffer(info->device, info->allocation, 0);
        if (buf) {
            buf->hostptr = NULL;
        } else {
            fprintf(stderr, "\tERROR: cannot find allocation!\n");
        }

        UNLOCK();
        return result;
    } else {
        printf("\tIgnoring D3DKMTUnlock2\n");
        UNLOCK();
        return 0;
    }
}

NTSTATUS D3DKMTSubmitCommand(const struct d3dkmt_submitcommand *submit)
{
    LOCK();
    WSL_PROLOG(D3DKMTSubmitCommand);

    print_pre("D3DKMTSubmitCommand");

    struct d3dkmthandle ctx = submit->broadcast_context[0];
    struct d3dkmthandle device = get_device_for_ctx(ctx);

    printf("\t\tdevice:\t\t\t0x%08x\n", device.v);
    printf("\t\tcommand_buffer:\t\t0x%llx\n", submit->command_buffer);
    printf("\t\tcommand_length:\t\t%u\n", submit->command_length);
    printf("\t\tpriv_drv_data_size:\t%u\n", submit->priv_drv_data_size);
    hexdump(submit->priv_drv_data, submit->priv_drv_data_size);

    struct submit_priv_data *priv_data = submit->priv_drv_data;

    struct list *buffers_of_interest = wrap_get_buffers_of_interest(device.v);
    assert(buffers_of_interest);

    struct buffer *buf;
    list_for_each_entry(buf, buffers_of_interest, node) {
        if (!buf || !buf->hostptr)
            continue;

        uint32_t dump_len = buf->len;
        if (!wrap_dump_all_bo()) {
            dump_len = min(buf->len, wrap_buf_len_cap());
        }

        log_gpuaddr(device.v, buf->gpuaddr, buf->len);
        rd_write_section(device.v, RD_BUFFER_CONTENTS, buf->hostptr, buf->len);
    }

    for (unsigned i = 0; i < priv_data->data0.cmdbuf.ib_count; i++) {
        struct submit_priv_ib_info *ib = &priv_data->data0.cmdbuf.ibs[i];
        printf("\t\tib[%u].iova:\t\t0x%llx\n", i, ib->iova);
        printf("\t\tib[%u].size_dwords:\t\t%u\n", i, ib->size_dwords);

        struct buffer *ib_buf = find_buffer(device, NULL_HANDLE, ib->iova);
        if (ib_buf) {
            log_cmdaddr(device.v, ib->iova, ib->size_dwords);
        } else {
            fprintf(stderr, "\tERROR: cannot find buffer for IB iova 0x%llx!\n", ib->iova);
        }
    }

    NTSTATUS result = orig_D3DKMTSubmitCommand(submit);

    print_post("D3DKMTSubmitCommand", result);

    UNLOCK();
    return result;
}
