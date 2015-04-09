/* THIS FILE IS GENERATED.  DO NOT EDIT. */

/*
 * XGL
 *
 * Copyright (C) 2014 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include <set>
#include <map>
#include <vector>
#include <string>
#include "xgl.h"
#include "xglDbg.h"
#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)
#include "xglWsiX11Ext.h"
#else
#include "xglWsiWinExt.h"
#endif

typedef struct _XGLAllocInfo {
    XGL_GPU_SIZE size;
    void *pData;
} XGLAllocInfo;

class objMemory {
public:
    objMemory() : m_numAllocations(0), m_pMemReqs(NULL) {}
    ~objMemory() { free(m_pMemReqs);}
    void setCount(const uint32_t num)
    {
        m_numAllocations = num;
    }

    void setReqs(const XGL_MEMORY_REQUIREMENTS *pReqs, const uint32_t num)
    {
        if (m_numAllocations != num && m_numAllocations != 0)
            glv_LogError("objMemory::setReqs, internal mismatch on number of allocations");
        if (m_pMemReqs == NULL && pReqs != NULL)
        {
            m_pMemReqs = (XGL_MEMORY_REQUIREMENTS *) glv_malloc(num * sizeof(XGL_MEMORY_REQUIREMENTS));
            if (m_pMemReqs == NULL)
            {
                glv_LogError("objMemory::setReqs out of memory");
                return;
            }
            memcpy(m_pMemReqs, pReqs, num);
        }
    }

private:
    uint32_t m_numAllocations;
    XGL_MEMORY_REQUIREMENTS *m_pMemReqs;
};

class gpuMemory {
public:
    gpuMemory() : m_pendingAlloc(false) {m_allocInfo.allocationSize = 0;}
    ~gpuMemory() {}
// memory mapping functions for app writes into mapped memory
    bool isPendingAlloc()
    {
        return m_pendingAlloc;
    }

    void setAllocInfo(const XGL_MEMORY_ALLOC_INFO *info, const bool pending)
    {
        m_pendingAlloc = pending;
        m_allocInfo = *info;
    }

    void setMemoryDataAddr(void *pBuf)
    {
        if (m_mapRange.empty())
        {
            glv_LogError("gpuMemory::setMemoryDataAddr() m_mapRange is empty\n");
            return;
        }
        MapRange mr = m_mapRange.back();
        if (mr.pData != NULL)
            glv_LogWarn("gpuMemory::setMemoryDataAddr() data already mapped overwrite old mapping\n");
        else if (pBuf == NULL)
            glv_LogWarn("gpuMemory::setMemoryDataAddr() adding NULL pointer\n");
        mr.pData = pBuf;
    }

    void setMemoryMapRange(void *pBuf, const size_t size, const size_t offset, const bool pending)
    {
        MapRange mr;
        mr.pData = pBuf;
        mr.size = size;
        mr.offset = offset;
        mr.pending = pending;
        m_mapRange.push_back(mr);
    }

    void copyMappingData(const void* pSrcData)
    {
        if (m_mapRange.empty())
        {
            glv_LogError("gpuMemory::copyMappingData() m_mapRange is empty\n");
            return;
        }
        MapRange mr = m_mapRange.back();
        if (!pSrcData || !mr.pData)
        {
            if (!pSrcData)
                glv_LogError("gpuMemory::copyMappingData() null src pointer\n");
            else
                glv_LogError("gpuMemory::copyMappingData() null dest pointer size=%u\n", m_allocInfo.allocationSize);
            m_mapRange.pop_back();
            return;
        }
        memcpy(mr.pData, pSrcData, m_allocInfo.allocationSize);
        if (!mr.pending)
            m_mapRange.pop_back();
    }

    size_t getMemoryMapSize()
    {
        return (!m_mapRange.empty()) ? m_mapRange.back().size : 0;
    }

private:
    bool m_pendingAlloc;
    struct MapRange {
        bool pending;
        size_t size;
        size_t offset;
        void* pData;
    };
    std::vector<MapRange> m_mapRange;
    XGL_MEMORY_ALLOC_INFO m_allocInfo;
};

typedef struct _imageObj {
     objMemory imageMem;
     XGL_IMAGE replayImage;
 } imageObj;

typedef struct _bufferObj {
     objMemory bufferMem;
     XGL_BUFFER replayBuffer;
 } bufferObj;

typedef struct _gpuMemObj {
     gpuMemory *pGpuMem;
     XGL_GPU_MEMORY replayGpuMem;
 } gpuMemObj;

class xglReplayObjMapper {
public:
    xglReplayObjMapper() {}
    ~xglReplayObjMapper() {}

 bool m_adjustForGPU; // true if replay adjusts behavior based on GPU
 void init_objMemCount(const XGL_BASE_OBJECT& object, const uint32_t &num)
 {
     XGL_IMAGE img = static_cast <XGL_IMAGE> (object);
     std::map<XGL_IMAGE, imageObj>::const_iterator it = m_images.find(img);
     if (it != m_images.end())
     {
         objMemory obj = it->second.imageMem;
         obj.setCount(num);
         return;
     }
     XGL_BUFFER buf = static_cast <XGL_BUFFER> (object);
     std::map<XGL_BUFFER, bufferObj>::const_iterator itb = m_buffers.find(buf);
     if (itb != m_buffers.end())
     {
         objMemory obj = itb->second.bufferMem;
         obj.setCount(num);
         return;
     }
     return;
 }

    void init_objMemReqs(const XGL_BASE_OBJECT& object, const XGL_MEMORY_REQUIREMENTS *pMemReqs, const unsigned int num)
    {
        XGL_IMAGE img = static_cast <XGL_IMAGE> (object);
        std::map<XGL_IMAGE, imageObj>::const_iterator it = m_images.find(img);
        if (it != m_images.end())
        {
            objMemory obj = it->second.imageMem;
            obj.setReqs(pMemReqs, num);
            return;
        }
        XGL_BUFFER buf = static_cast <XGL_BUFFER> (object);
        std::map<XGL_BUFFER, bufferObj>::const_iterator itb = m_buffers.find(buf);
        if (itb != m_buffers.end())
        {
            objMemory obj = itb->second.bufferMem;
            obj.setReqs(pMemReqs, num);
            return;
        }
        return;
    }

    void clear_all_map_handles()
    {
        m_bufferViews.clear();
        m_buffers.clear();
        m_cmdBuffers.clear();
        m_colorAttachmentViews.clear();
        m_depthStencilViews.clear();
        m_descriptorRegions.clear();
        m_descriptorSetLayouts.clear();
        m_descriptorSets.clear();
        m_devices.clear();
        m_dynamicCbStateObjects.clear();
        m_dynamicDsStateObjects.clear();
        m_dynamicRsStateObjects.clear();
        m_dynamicVpStateObjects.clear();
        m_events.clear();
        m_fences.clear();
        m_framebuffers.clear();
        m_gpuMemorys.clear();
        m_imageViews.clear();
        m_images.clear();
        m_instances.clear();
        m_physicalGpus.clear();
        m_pipelineDeltas.clear();
        m_pipelines.clear();
        m_queryPools.clear();
        m_queueSemaphores.clear();
        m_queues.clear();
        m_renderPasss.clear();
        m_samplers.clear();
        m_shaders.clear();
    }

    std::map<XGL_BUFFER_VIEW, XGL_BUFFER_VIEW> m_bufferViews;
    void add_to_map(XGL_BUFFER_VIEW* pTraceVal, XGL_BUFFER_VIEW* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_bufferViews[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_BUFFER_VIEW& key)
    {
        m_bufferViews.erase(key);
    }

    XGL_BUFFER_VIEW remap(const XGL_BUFFER_VIEW& value)
    {
        std::map<XGL_BUFFER_VIEW, XGL_BUFFER_VIEW>::const_iterator q = m_bufferViews.find(value);
        return (q == m_bufferViews.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_BUFFER, bufferObj> m_buffers;
    void add_to_map(XGL_BUFFER* pTraceVal, bufferObj* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_buffers[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_BUFFER& key)
    {
        m_buffers.erase(key);
    }

    XGL_BUFFER remap(const XGL_BUFFER& value)
    {
        std::map<XGL_BUFFER, bufferObj>::const_iterator q = m_buffers.find(value);
        return (q == m_buffers.end()) ? XGL_NULL_HANDLE : q->second.replayBuffer;
    }

    std::map<XGL_CMD_BUFFER, XGL_CMD_BUFFER> m_cmdBuffers;
    void add_to_map(XGL_CMD_BUFFER* pTraceVal, XGL_CMD_BUFFER* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_cmdBuffers[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_CMD_BUFFER& key)
    {
        m_cmdBuffers.erase(key);
    }

    XGL_CMD_BUFFER remap(const XGL_CMD_BUFFER& value)
    {
        std::map<XGL_CMD_BUFFER, XGL_CMD_BUFFER>::const_iterator q = m_cmdBuffers.find(value);
        return (q == m_cmdBuffers.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_COLOR_ATTACHMENT_VIEW, XGL_COLOR_ATTACHMENT_VIEW> m_colorAttachmentViews;
    void add_to_map(XGL_COLOR_ATTACHMENT_VIEW* pTraceVal, XGL_COLOR_ATTACHMENT_VIEW* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_colorAttachmentViews[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_COLOR_ATTACHMENT_VIEW& key)
    {
        m_colorAttachmentViews.erase(key);
    }

    XGL_COLOR_ATTACHMENT_VIEW remap(const XGL_COLOR_ATTACHMENT_VIEW& value)
    {
        std::map<XGL_COLOR_ATTACHMENT_VIEW, XGL_COLOR_ATTACHMENT_VIEW>::const_iterator q = m_colorAttachmentViews.find(value);
        return (q == m_colorAttachmentViews.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_DEPTH_STENCIL_VIEW, XGL_DEPTH_STENCIL_VIEW> m_depthStencilViews;
    void add_to_map(XGL_DEPTH_STENCIL_VIEW* pTraceVal, XGL_DEPTH_STENCIL_VIEW* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_depthStencilViews[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_DEPTH_STENCIL_VIEW& key)
    {
        m_depthStencilViews.erase(key);
    }

    XGL_DEPTH_STENCIL_VIEW remap(const XGL_DEPTH_STENCIL_VIEW& value)
    {
        std::map<XGL_DEPTH_STENCIL_VIEW, XGL_DEPTH_STENCIL_VIEW>::const_iterator q = m_depthStencilViews.find(value);
        return (q == m_depthStencilViews.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_DESCRIPTOR_REGION, XGL_DESCRIPTOR_REGION> m_descriptorRegions;
    void add_to_map(XGL_DESCRIPTOR_REGION* pTraceVal, XGL_DESCRIPTOR_REGION* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_descriptorRegions[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_DESCRIPTOR_REGION& key)
    {
        m_descriptorRegions.erase(key);
    }

    XGL_DESCRIPTOR_REGION remap(const XGL_DESCRIPTOR_REGION& value)
    {
        std::map<XGL_DESCRIPTOR_REGION, XGL_DESCRIPTOR_REGION>::const_iterator q = m_descriptorRegions.find(value);
        return (q == m_descriptorRegions.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_DESCRIPTOR_SET_LAYOUT, XGL_DESCRIPTOR_SET_LAYOUT> m_descriptorSetLayouts;
    void add_to_map(XGL_DESCRIPTOR_SET_LAYOUT* pTraceVal, XGL_DESCRIPTOR_SET_LAYOUT* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_descriptorSetLayouts[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_DESCRIPTOR_SET_LAYOUT& key)
    {
        m_descriptorSetLayouts.erase(key);
    }

    XGL_DESCRIPTOR_SET_LAYOUT remap(const XGL_DESCRIPTOR_SET_LAYOUT& value)
    {
        std::map<XGL_DESCRIPTOR_SET_LAYOUT, XGL_DESCRIPTOR_SET_LAYOUT>::const_iterator q = m_descriptorSetLayouts.find(value);
        return (q == m_descriptorSetLayouts.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_DESCRIPTOR_SET, XGL_DESCRIPTOR_SET> m_descriptorSets;
    void add_to_map(XGL_DESCRIPTOR_SET* pTraceVal, XGL_DESCRIPTOR_SET* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_descriptorSets[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_DESCRIPTOR_SET& key)
    {
        m_descriptorSets.erase(key);
    }

    XGL_DESCRIPTOR_SET remap(const XGL_DESCRIPTOR_SET& value)
    {
        std::map<XGL_DESCRIPTOR_SET, XGL_DESCRIPTOR_SET>::const_iterator q = m_descriptorSets.find(value);
        return (q == m_descriptorSets.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_DEVICE, XGL_DEVICE> m_devices;
    void add_to_map(XGL_DEVICE* pTraceVal, XGL_DEVICE* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_devices[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_DEVICE& key)
    {
        m_devices.erase(key);
    }

    XGL_DEVICE remap(const XGL_DEVICE& value)
    {
        std::map<XGL_DEVICE, XGL_DEVICE>::const_iterator q = m_devices.find(value);
        return (q == m_devices.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_DYNAMIC_CB_STATE_OBJECT, XGL_DYNAMIC_CB_STATE_OBJECT> m_dynamicCbStateObjects;
    void add_to_map(XGL_DYNAMIC_CB_STATE_OBJECT* pTraceVal, XGL_DYNAMIC_CB_STATE_OBJECT* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_dynamicCbStateObjects[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_DYNAMIC_CB_STATE_OBJECT& key)
    {
        m_dynamicCbStateObjects.erase(key);
    }

    XGL_DYNAMIC_CB_STATE_OBJECT remap(const XGL_DYNAMIC_CB_STATE_OBJECT& value)
    {
        std::map<XGL_DYNAMIC_CB_STATE_OBJECT, XGL_DYNAMIC_CB_STATE_OBJECT>::const_iterator q = m_dynamicCbStateObjects.find(value);
        return (q == m_dynamicCbStateObjects.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_DYNAMIC_DS_STATE_OBJECT, XGL_DYNAMIC_DS_STATE_OBJECT> m_dynamicDsStateObjects;
    void add_to_map(XGL_DYNAMIC_DS_STATE_OBJECT* pTraceVal, XGL_DYNAMIC_DS_STATE_OBJECT* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_dynamicDsStateObjects[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_DYNAMIC_DS_STATE_OBJECT& key)
    {
        m_dynamicDsStateObjects.erase(key);
    }

    XGL_DYNAMIC_DS_STATE_OBJECT remap(const XGL_DYNAMIC_DS_STATE_OBJECT& value)
    {
        std::map<XGL_DYNAMIC_DS_STATE_OBJECT, XGL_DYNAMIC_DS_STATE_OBJECT>::const_iterator q = m_dynamicDsStateObjects.find(value);
        return (q == m_dynamicDsStateObjects.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_DYNAMIC_RS_STATE_OBJECT, XGL_DYNAMIC_RS_STATE_OBJECT> m_dynamicRsStateObjects;
    void add_to_map(XGL_DYNAMIC_RS_STATE_OBJECT* pTraceVal, XGL_DYNAMIC_RS_STATE_OBJECT* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_dynamicRsStateObjects[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_DYNAMIC_RS_STATE_OBJECT& key)
    {
        m_dynamicRsStateObjects.erase(key);
    }

    XGL_DYNAMIC_RS_STATE_OBJECT remap(const XGL_DYNAMIC_RS_STATE_OBJECT& value)
    {
        std::map<XGL_DYNAMIC_RS_STATE_OBJECT, XGL_DYNAMIC_RS_STATE_OBJECT>::const_iterator q = m_dynamicRsStateObjects.find(value);
        return (q == m_dynamicRsStateObjects.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_DYNAMIC_VP_STATE_OBJECT, XGL_DYNAMIC_VP_STATE_OBJECT> m_dynamicVpStateObjects;
    void add_to_map(XGL_DYNAMIC_VP_STATE_OBJECT* pTraceVal, XGL_DYNAMIC_VP_STATE_OBJECT* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_dynamicVpStateObjects[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_DYNAMIC_VP_STATE_OBJECT& key)
    {
        m_dynamicVpStateObjects.erase(key);
    }

    XGL_DYNAMIC_VP_STATE_OBJECT remap(const XGL_DYNAMIC_VP_STATE_OBJECT& value)
    {
        std::map<XGL_DYNAMIC_VP_STATE_OBJECT, XGL_DYNAMIC_VP_STATE_OBJECT>::const_iterator q = m_dynamicVpStateObjects.find(value);
        return (q == m_dynamicVpStateObjects.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_EVENT, XGL_EVENT> m_events;
    void add_to_map(XGL_EVENT* pTraceVal, XGL_EVENT* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_events[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_EVENT& key)
    {
        m_events.erase(key);
    }

    XGL_EVENT remap(const XGL_EVENT& value)
    {
        std::map<XGL_EVENT, XGL_EVENT>::const_iterator q = m_events.find(value);
        return (q == m_events.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_FENCE, XGL_FENCE> m_fences;
    void add_to_map(XGL_FENCE* pTraceVal, XGL_FENCE* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_fences[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_FENCE& key)
    {
        m_fences.erase(key);
    }

    XGL_FENCE remap(const XGL_FENCE& value)
    {
        std::map<XGL_FENCE, XGL_FENCE>::const_iterator q = m_fences.find(value);
        return (q == m_fences.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_FRAMEBUFFER, XGL_FRAMEBUFFER> m_framebuffers;
    void add_to_map(XGL_FRAMEBUFFER* pTraceVal, XGL_FRAMEBUFFER* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_framebuffers[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_FRAMEBUFFER& key)
    {
        m_framebuffers.erase(key);
    }

    XGL_FRAMEBUFFER remap(const XGL_FRAMEBUFFER& value)
    {
        std::map<XGL_FRAMEBUFFER, XGL_FRAMEBUFFER>::const_iterator q = m_framebuffers.find(value);
        return (q == m_framebuffers.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_GPU_MEMORY, gpuMemObj> m_gpuMemorys;
    void add_to_map(XGL_GPU_MEMORY* pTraceVal, gpuMemObj* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_gpuMemorys[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_GPU_MEMORY& key)
    {
        m_gpuMemorys.erase(key);
    }

    XGL_GPU_MEMORY remap(const XGL_GPU_MEMORY& value)
    {
        std::map<XGL_GPU_MEMORY, gpuMemObj>::const_iterator q = m_gpuMemorys.find(value);
        return (q == m_gpuMemorys.end()) ? XGL_NULL_HANDLE : q->second.replayGpuMem;
    }

    std::map<XGL_IMAGE_VIEW, XGL_IMAGE_VIEW> m_imageViews;
    void add_to_map(XGL_IMAGE_VIEW* pTraceVal, XGL_IMAGE_VIEW* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_imageViews[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_IMAGE_VIEW& key)
    {
        m_imageViews.erase(key);
    }

    XGL_IMAGE_VIEW remap(const XGL_IMAGE_VIEW& value)
    {
        std::map<XGL_IMAGE_VIEW, XGL_IMAGE_VIEW>::const_iterator q = m_imageViews.find(value);
        return (q == m_imageViews.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_IMAGE, imageObj> m_images;
    void add_to_map(XGL_IMAGE* pTraceVal, imageObj* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_images[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_IMAGE& key)
    {
        m_images.erase(key);
    }

    XGL_IMAGE remap(const XGL_IMAGE& value)
    {
        std::map<XGL_IMAGE, imageObj>::const_iterator q = m_images.find(value);
        return (q == m_images.end()) ? XGL_NULL_HANDLE : q->second.replayImage;
    }

    std::map<XGL_INSTANCE, XGL_INSTANCE> m_instances;
    void add_to_map(XGL_INSTANCE* pTraceVal, XGL_INSTANCE* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_instances[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_INSTANCE& key)
    {
        m_instances.erase(key);
    }

    XGL_INSTANCE remap(const XGL_INSTANCE& value)
    {
        std::map<XGL_INSTANCE, XGL_INSTANCE>::const_iterator q = m_instances.find(value);
        return (q == m_instances.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_PHYSICAL_GPU, XGL_PHYSICAL_GPU> m_physicalGpus;
    void add_to_map(XGL_PHYSICAL_GPU* pTraceVal, XGL_PHYSICAL_GPU* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_physicalGpus[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_PHYSICAL_GPU& key)
    {
        m_physicalGpus.erase(key);
    }

    XGL_PHYSICAL_GPU remap(const XGL_PHYSICAL_GPU& value)
    {
        std::map<XGL_PHYSICAL_GPU, XGL_PHYSICAL_GPU>::const_iterator q = m_physicalGpus.find(value);
        return (q == m_physicalGpus.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_PIPELINE_DELTA, XGL_PIPELINE_DELTA> m_pipelineDeltas;
    void add_to_map(XGL_PIPELINE_DELTA* pTraceVal, XGL_PIPELINE_DELTA* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_pipelineDeltas[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_PIPELINE_DELTA& key)
    {
        m_pipelineDeltas.erase(key);
    }

    XGL_PIPELINE_DELTA remap(const XGL_PIPELINE_DELTA& value)
    {
        std::map<XGL_PIPELINE_DELTA, XGL_PIPELINE_DELTA>::const_iterator q = m_pipelineDeltas.find(value);
        return (q == m_pipelineDeltas.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_PIPELINE, XGL_PIPELINE> m_pipelines;
    void add_to_map(XGL_PIPELINE* pTraceVal, XGL_PIPELINE* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_pipelines[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_PIPELINE& key)
    {
        m_pipelines.erase(key);
    }

    XGL_PIPELINE remap(const XGL_PIPELINE& value)
    {
        std::map<XGL_PIPELINE, XGL_PIPELINE>::const_iterator q = m_pipelines.find(value);
        return (q == m_pipelines.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_QUERY_POOL, XGL_QUERY_POOL> m_queryPools;
    void add_to_map(XGL_QUERY_POOL* pTraceVal, XGL_QUERY_POOL* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_queryPools[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_QUERY_POOL& key)
    {
        m_queryPools.erase(key);
    }

    XGL_QUERY_POOL remap(const XGL_QUERY_POOL& value)
    {
        std::map<XGL_QUERY_POOL, XGL_QUERY_POOL>::const_iterator q = m_queryPools.find(value);
        return (q == m_queryPools.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_QUEUE_SEMAPHORE, XGL_QUEUE_SEMAPHORE> m_queueSemaphores;
    void add_to_map(XGL_QUEUE_SEMAPHORE* pTraceVal, XGL_QUEUE_SEMAPHORE* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_queueSemaphores[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_QUEUE_SEMAPHORE& key)
    {
        m_queueSemaphores.erase(key);
    }

    XGL_QUEUE_SEMAPHORE remap(const XGL_QUEUE_SEMAPHORE& value)
    {
        std::map<XGL_QUEUE_SEMAPHORE, XGL_QUEUE_SEMAPHORE>::const_iterator q = m_queueSemaphores.find(value);
        return (q == m_queueSemaphores.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_QUEUE, XGL_QUEUE> m_queues;
    void add_to_map(XGL_QUEUE* pTraceVal, XGL_QUEUE* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_queues[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_QUEUE& key)
    {
        m_queues.erase(key);
    }

    XGL_QUEUE remap(const XGL_QUEUE& value)
    {
        std::map<XGL_QUEUE, XGL_QUEUE>::const_iterator q = m_queues.find(value);
        return (q == m_queues.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_RENDER_PASS, XGL_RENDER_PASS> m_renderPasss;
    void add_to_map(XGL_RENDER_PASS* pTraceVal, XGL_RENDER_PASS* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_renderPasss[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_RENDER_PASS& key)
    {
        m_renderPasss.erase(key);
    }

    XGL_RENDER_PASS remap(const XGL_RENDER_PASS& value)
    {
        std::map<XGL_RENDER_PASS, XGL_RENDER_PASS>::const_iterator q = m_renderPasss.find(value);
        return (q == m_renderPasss.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_SAMPLER, XGL_SAMPLER> m_samplers;
    void add_to_map(XGL_SAMPLER* pTraceVal, XGL_SAMPLER* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_samplers[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_SAMPLER& key)
    {
        m_samplers.erase(key);
    }

    XGL_SAMPLER remap(const XGL_SAMPLER& value)
    {
        std::map<XGL_SAMPLER, XGL_SAMPLER>::const_iterator q = m_samplers.find(value);
        return (q == m_samplers.end()) ? XGL_NULL_HANDLE : q->second;
    }

    std::map<XGL_SHADER, XGL_SHADER> m_shaders;
    void add_to_map(XGL_SHADER* pTraceVal, XGL_SHADER* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_shaders[*pTraceVal] = *pReplayVal;
    }

    void rm_from_map(const XGL_SHADER& key)
    {
        m_shaders.erase(key);
    }

    XGL_SHADER remap(const XGL_SHADER& value)
    {
        std::map<XGL_SHADER, XGL_SHADER>::const_iterator q = m_shaders.find(value);
        return (q == m_shaders.end()) ? XGL_NULL_HANDLE : q->second;
    }

    XGL_DYNAMIC_STATE_OBJECT remap(const XGL_DYNAMIC_STATE_OBJECT& state)
    {
        XGL_DYNAMIC_STATE_OBJECT obj;
        if ((obj = remap(static_cast <XGL_DYNAMIC_VP_STATE_OBJECT> (state))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_DYNAMIC_RS_STATE_OBJECT> (state))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_DYNAMIC_CB_STATE_OBJECT> (state))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_DYNAMIC_DS_STATE_OBJECT> (state))) != XGL_NULL_HANDLE)
            return obj;
        return XGL_NULL_HANDLE;
    }
    void rm_from_map(const XGL_DYNAMIC_STATE_OBJECT& state)
    {
        rm_from_map(static_cast <XGL_DYNAMIC_VP_STATE_OBJECT> (state));
        rm_from_map(static_cast <XGL_DYNAMIC_RS_STATE_OBJECT> (state));
        rm_from_map(static_cast <XGL_DYNAMIC_CB_STATE_OBJECT> (state));
        rm_from_map(static_cast <XGL_DYNAMIC_DS_STATE_OBJECT> (state));
    }

    XGL_OBJECT remap(const XGL_OBJECT& object)
    {
        XGL_OBJECT obj;
        if ((obj = remap(static_cast <XGL_BUFFER> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_BUFFER_VIEW> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_IMAGE> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_IMAGE_VIEW> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_COLOR_ATTACHMENT_VIEW> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_DEPTH_STENCIL_VIEW> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_SHADER> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_PIPELINE> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_PIPELINE_DELTA> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_SAMPLER> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_DESCRIPTOR_SET> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_DESCRIPTOR_SET_LAYOUT> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_DESCRIPTOR_REGION> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_DYNAMIC_STATE_OBJECT> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_CMD_BUFFER> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_FENCE> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_QUEUE_SEMAPHORE> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_EVENT> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_QUERY_POOL> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_FRAMEBUFFER> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_RENDER_PASS> (object))) != XGL_NULL_HANDLE)
            return obj;
        return XGL_NULL_HANDLE;
    }
    void rm_from_map(const XGL_OBJECT & objKey)
    {
        rm_from_map(static_cast <XGL_BUFFER> (objKey));
        rm_from_map(static_cast <XGL_BUFFER_VIEW> (objKey));
        rm_from_map(static_cast <XGL_IMAGE> (objKey));
        rm_from_map(static_cast <XGL_IMAGE_VIEW> (objKey));
        rm_from_map(static_cast <XGL_COLOR_ATTACHMENT_VIEW> (objKey));
        rm_from_map(static_cast <XGL_DEPTH_STENCIL_VIEW> (objKey));
        rm_from_map(static_cast <XGL_SHADER> (objKey));
        rm_from_map(static_cast <XGL_PIPELINE> (objKey));
        rm_from_map(static_cast <XGL_PIPELINE_DELTA> (objKey));
        rm_from_map(static_cast <XGL_SAMPLER> (objKey));
        rm_from_map(static_cast <XGL_DESCRIPTOR_SET> (objKey));
        rm_from_map(static_cast <XGL_DESCRIPTOR_SET_LAYOUT> (objKey));
        rm_from_map(static_cast <XGL_DESCRIPTOR_REGION> (objKey));
        rm_from_map(static_cast <XGL_DYNAMIC_STATE_OBJECT> (objKey));
        rm_from_map(static_cast <XGL_CMD_BUFFER> (objKey));
        rm_from_map(static_cast <XGL_FENCE> (objKey));
        rm_from_map(static_cast <XGL_QUEUE_SEMAPHORE> (objKey));
        rm_from_map(static_cast <XGL_EVENT> (objKey));
        rm_from_map(static_cast <XGL_QUERY_POOL> (objKey));
        rm_from_map(static_cast <XGL_FRAMEBUFFER> (objKey));
        rm_from_map(static_cast <XGL_RENDER_PASS> (objKey));
    }
    XGL_BASE_OBJECT remap(const XGL_BASE_OBJECT& object)
    {
        XGL_BASE_OBJECT obj;
        if ((obj = remap(static_cast <XGL_DEVICE> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_QUEUE> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_GPU_MEMORY> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_OBJECT> (object))) != XGL_NULL_HANDLE)
            return obj;
        return XGL_NULL_HANDLE;
    }
};