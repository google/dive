/*
 Copyright 2020 Google LLC

 Licensed under the Apache License, Version 2.0 (the \"License\";
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an \"AS IS\" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// WARNING!  WARNING!  WARNING!  WARNING!  WARNING!  WARNING!  WARNING!  WARNING!  WARNING! WARNING!
//
// This code has been generated automatically by generateSOAs.py. Do not hand-modify this code.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once

#include "adreno.h"
#include "common.h"
#include "dive_core/common/gpudefs.h"
#include "info_id.h"
#include "struct_of_arrays.h"
#include "third_party/Vulkan-Headers/include/vulkan/vulkan.h"

namespace Dive
{
// State info for events (draw/dispatch/sync/dma)
class EventStateInfo;
using EventStateId = InfoIdT<EventStateInfo>;

template<typename CONFIG> class EventStateInfoViewportArray
{
public:
    using Id = typename CONFIG::Id;
    using SOA = typename CONFIG::SOA;
    using Ref = typename CONFIG::Ref;
    using ViewportArray = typename CONFIG::ViewportArray;
    using ViewportConstArray = typename CONFIG::ViewportConstArray;
    EventStateInfoViewportArray() = default;
    EventStateInfoViewportArray(const EventStateInfoViewportArray& other) :
        m_obj_ptr(other.m_obj_ptr),
        m_id(other.m_id)
    {
    }
    EventStateInfoViewportArray(SOA* obj_ptr, Id id) :
        m_obj_ptr(obj_ptr),
        m_id(id)
    {
    }
    Id   id() const { return m_id; }
    SOA& obj() const { return *m_obj_ptr; }
    bool IsValid() const { return m_obj_ptr != nullptr && m_obj_ptr->IsValidId(m_id); }

    //-----------------------------------------------
    // REF FIELD Viewport: Defines the viewport transforms

    // `Get(uint32_t viewport)` returns the value of the Viewport field of the referenced object
    inline VkViewport Get(uint32_t viewport) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->Viewport(m_id, viewport);
    }

    // `Set(value)` sets the Viewport field of the referenced object
    inline const ViewportArray& Set(uint32_t viewport, VkViewport value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetViewport(m_id, viewport, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsViewportSet(uint32_t viewport) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsViewportSet(m_id, viewport);
    }

    inline const char* GetViewportName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetViewportName();
    }

    inline const char* GetViewportDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetViewportDescription();
    }

protected:
    SOA* m_obj_ptr = nullptr;
    Id   m_id;
};

template<typename CONFIG> class EventStateInfoViewportConstArray
{
public:
    using Id = typename CONFIG::Id;
    using SOA = typename CONFIG::SOA;
    using Ref = typename CONFIG::Ref;
    using ViewportArray = typename CONFIG::ViewportArray;
    using ViewportConstArray = typename CONFIG::ViewportConstArray;
    EventStateInfoViewportConstArray() = default;
    EventStateInfoViewportConstArray(const EventStateInfoViewportArray<CONFIG>& other) :
        m_obj_ptr(&other.obj()),
        m_id(other.id())
    {
    }
    EventStateInfoViewportConstArray(const EventStateInfoViewportConstArray& other) :
        m_obj_ptr(other.m_obj_ptr),
        m_id(other.m_id)
    {
    }
    EventStateInfoViewportConstArray(const SOA* obj_ptr, Id id) :
        m_obj_ptr(obj_ptr),
        m_id(id)
    {
    }
    Id         id() const { return m_id; }
    const SOA& obj() const { return *m_obj_ptr; }
    bool       IsValid() const { return m_obj_ptr != nullptr && m_obj_ptr->IsValidId(m_id); }

    //-----------------------------------------------
    // REF FIELD Viewport: Defines the viewport transforms

    // `Get(uint32_t viewport)` returns the value of the Viewport field of the referenced object
    inline VkViewport Get(uint32_t viewport) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->Viewport(m_id, viewport);
    }

    inline bool IsViewportSet(uint32_t viewport) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsViewportSet(m_id, viewport);
    }

    inline const char* GetViewportName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetViewportName();
    }

    inline const char* GetViewportDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetViewportDescription();
    }

protected:
    const SOA* m_obj_ptr = nullptr;
    Id         m_id;
};

template<typename CONFIG> class EventStateInfoScissorArray
{
public:
    using Id = typename CONFIG::Id;
    using SOA = typename CONFIG::SOA;
    using Ref = typename CONFIG::Ref;
    using ScissorArray = typename CONFIG::ScissorArray;
    using ScissorConstArray = typename CONFIG::ScissorConstArray;
    EventStateInfoScissorArray() = default;
    EventStateInfoScissorArray(const EventStateInfoScissorArray& other) :
        m_obj_ptr(other.m_obj_ptr),
        m_id(other.m_id)
    {
    }
    EventStateInfoScissorArray(SOA* obj_ptr, Id id) :
        m_obj_ptr(obj_ptr),
        m_id(id)
    {
    }
    Id   id() const { return m_id; }
    SOA& obj() const { return *m_obj_ptr; }
    bool IsValid() const { return m_obj_ptr != nullptr && m_obj_ptr->IsValidId(m_id); }

    //-----------------------------------------------
    // REF FIELD Scissor: Defines the rectangular bounds of the scissor for the corresponding
    // viewport

    // `Get(uint32_t scissor)` returns the value of the Scissor field of the referenced object
    inline VkRect2D Get(uint32_t scissor) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->Scissor(m_id, scissor);
    }

    // `Set(value)` sets the Scissor field of the referenced object
    inline const ScissorArray& Set(uint32_t scissor, VkRect2D value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetScissor(m_id, scissor, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsScissorSet(uint32_t scissor) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsScissorSet(m_id, scissor);
    }

    inline const char* GetScissorName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetScissorName();
    }

    inline const char* GetScissorDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetScissorDescription();
    }

protected:
    SOA* m_obj_ptr = nullptr;
    Id   m_id;
};

template<typename CONFIG> class EventStateInfoScissorConstArray
{
public:
    using Id = typename CONFIG::Id;
    using SOA = typename CONFIG::SOA;
    using Ref = typename CONFIG::Ref;
    using ScissorArray = typename CONFIG::ScissorArray;
    using ScissorConstArray = typename CONFIG::ScissorConstArray;
    EventStateInfoScissorConstArray() = default;
    EventStateInfoScissorConstArray(const EventStateInfoScissorArray<CONFIG>& other) :
        m_obj_ptr(&other.obj()),
        m_id(other.id())
    {
    }
    EventStateInfoScissorConstArray(const EventStateInfoScissorConstArray& other) :
        m_obj_ptr(other.m_obj_ptr),
        m_id(other.m_id)
    {
    }
    EventStateInfoScissorConstArray(const SOA* obj_ptr, Id id) :
        m_obj_ptr(obj_ptr),
        m_id(id)
    {
    }
    Id         id() const { return m_id; }
    const SOA& obj() const { return *m_obj_ptr; }
    bool       IsValid() const { return m_obj_ptr != nullptr && m_obj_ptr->IsValidId(m_id); }

    //-----------------------------------------------
    // REF FIELD Scissor: Defines the rectangular bounds of the scissor for the corresponding
    // viewport

    // `Get(uint32_t scissor)` returns the value of the Scissor field of the referenced object
    inline VkRect2D Get(uint32_t scissor) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->Scissor(m_id, scissor);
    }

    inline bool IsScissorSet(uint32_t scissor) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsScissorSet(m_id, scissor);
    }

    inline const char* GetScissorName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetScissorName();
    }

    inline const char* GetScissorDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetScissorDescription();
    }

protected:
    const SOA* m_obj_ptr = nullptr;
    Id         m_id;
};

template<typename CONFIG> class EventStateInfoLogicOpEnabledArray
{
public:
    using Id = typename CONFIG::Id;
    using SOA = typename CONFIG::SOA;
    using Ref = typename CONFIG::Ref;
    using LogicOpEnabledArray = typename CONFIG::LogicOpEnabledArray;
    using LogicOpEnabledConstArray = typename CONFIG::LogicOpEnabledConstArray;
    EventStateInfoLogicOpEnabledArray() = default;
    EventStateInfoLogicOpEnabledArray(const EventStateInfoLogicOpEnabledArray& other) :
        m_obj_ptr(other.m_obj_ptr),
        m_id(other.m_id)
    {
    }
    EventStateInfoLogicOpEnabledArray(SOA* obj_ptr, Id id) :
        m_obj_ptr(obj_ptr),
        m_id(id)
    {
    }
    Id   id() const { return m_id; }
    SOA& obj() const { return *m_obj_ptr; }
    bool IsValid() const { return m_obj_ptr != nullptr && m_obj_ptr->IsValidId(m_id); }

    //-----------------------------------------------
    // REF FIELD LogicOpEnabled: Whether to apply Logical Operations

    // `Get(uint32_t attachment)` returns the value of the LogicOpEnabled field of the referenced
    // object
    inline bool Get(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->LogicOpEnabled(m_id, attachment);
    }

    // `Set(value)` sets the LogicOpEnabled field of the referenced object
    inline const LogicOpEnabledArray& Set(uint32_t attachment, bool value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetLogicOpEnabled(m_id, attachment, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsLogicOpEnabledSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsLogicOpEnabledSet(m_id, attachment);
    }

    inline const char* GetLogicOpEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLogicOpEnabledName();
    }

    inline const char* GetLogicOpEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLogicOpEnabledDescription();
    }

protected:
    SOA* m_obj_ptr = nullptr;
    Id   m_id;
};

template<typename CONFIG> class EventStateInfoLogicOpEnabledConstArray
{
public:
    using Id = typename CONFIG::Id;
    using SOA = typename CONFIG::SOA;
    using Ref = typename CONFIG::Ref;
    using LogicOpEnabledArray = typename CONFIG::LogicOpEnabledArray;
    using LogicOpEnabledConstArray = typename CONFIG::LogicOpEnabledConstArray;
    EventStateInfoLogicOpEnabledConstArray() = default;
    EventStateInfoLogicOpEnabledConstArray(const EventStateInfoLogicOpEnabledArray<CONFIG>& other) :
        m_obj_ptr(&other.obj()),
        m_id(other.id())
    {
    }
    EventStateInfoLogicOpEnabledConstArray(const EventStateInfoLogicOpEnabledConstArray& other) :
        m_obj_ptr(other.m_obj_ptr),
        m_id(other.m_id)
    {
    }
    EventStateInfoLogicOpEnabledConstArray(const SOA* obj_ptr, Id id) :
        m_obj_ptr(obj_ptr),
        m_id(id)
    {
    }
    Id         id() const { return m_id; }
    const SOA& obj() const { return *m_obj_ptr; }
    bool       IsValid() const { return m_obj_ptr != nullptr && m_obj_ptr->IsValidId(m_id); }

    //-----------------------------------------------
    // REF FIELD LogicOpEnabled: Whether to apply Logical Operations

    // `Get(uint32_t attachment)` returns the value of the LogicOpEnabled field of the referenced
    // object
    inline bool Get(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->LogicOpEnabled(m_id, attachment);
    }

    inline bool IsLogicOpEnabledSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsLogicOpEnabledSet(m_id, attachment);
    }

    inline const char* GetLogicOpEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLogicOpEnabledName();
    }

    inline const char* GetLogicOpEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLogicOpEnabledDescription();
    }

protected:
    const SOA* m_obj_ptr = nullptr;
    Id         m_id;
};

template<typename CONFIG> class EventStateInfoLogicOpArray
{
public:
    using Id = typename CONFIG::Id;
    using SOA = typename CONFIG::SOA;
    using Ref = typename CONFIG::Ref;
    using LogicOpArray = typename CONFIG::LogicOpArray;
    using LogicOpConstArray = typename CONFIG::LogicOpConstArray;
    EventStateInfoLogicOpArray() = default;
    EventStateInfoLogicOpArray(const EventStateInfoLogicOpArray& other) :
        m_obj_ptr(other.m_obj_ptr),
        m_id(other.m_id)
    {
    }
    EventStateInfoLogicOpArray(SOA* obj_ptr, Id id) :
        m_obj_ptr(obj_ptr),
        m_id(id)
    {
    }
    Id   id() const { return m_id; }
    SOA& obj() const { return *m_obj_ptr; }
    bool IsValid() const { return m_obj_ptr != nullptr && m_obj_ptr->IsValidId(m_id); }

    //-----------------------------------------------
    // REF FIELD LogicOp: Which logical operation to apply

    // `Get(uint32_t attachment)` returns the value of the LogicOp field of the referenced object
    inline VkLogicOp Get(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->LogicOp(m_id, attachment);
    }

    // `Set(value)` sets the LogicOp field of the referenced object
    inline const LogicOpArray& Set(uint32_t attachment, VkLogicOp value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetLogicOp(m_id, attachment, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsLogicOpSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsLogicOpSet(m_id, attachment);
    }

    inline const char* GetLogicOpName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLogicOpName();
    }

    inline const char* GetLogicOpDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLogicOpDescription();
    }

protected:
    SOA* m_obj_ptr = nullptr;
    Id   m_id;
};

template<typename CONFIG> class EventStateInfoLogicOpConstArray
{
public:
    using Id = typename CONFIG::Id;
    using SOA = typename CONFIG::SOA;
    using Ref = typename CONFIG::Ref;
    using LogicOpArray = typename CONFIG::LogicOpArray;
    using LogicOpConstArray = typename CONFIG::LogicOpConstArray;
    EventStateInfoLogicOpConstArray() = default;
    EventStateInfoLogicOpConstArray(const EventStateInfoLogicOpArray<CONFIG>& other) :
        m_obj_ptr(&other.obj()),
        m_id(other.id())
    {
    }
    EventStateInfoLogicOpConstArray(const EventStateInfoLogicOpConstArray& other) :
        m_obj_ptr(other.m_obj_ptr),
        m_id(other.m_id)
    {
    }
    EventStateInfoLogicOpConstArray(const SOA* obj_ptr, Id id) :
        m_obj_ptr(obj_ptr),
        m_id(id)
    {
    }
    Id         id() const { return m_id; }
    const SOA& obj() const { return *m_obj_ptr; }
    bool       IsValid() const { return m_obj_ptr != nullptr && m_obj_ptr->IsValidId(m_id); }

    //-----------------------------------------------
    // REF FIELD LogicOp: Which logical operation to apply

    // `Get(uint32_t attachment)` returns the value of the LogicOp field of the referenced object
    inline VkLogicOp Get(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->LogicOp(m_id, attachment);
    }

    inline bool IsLogicOpSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsLogicOpSet(m_id, attachment);
    }

    inline const char* GetLogicOpName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLogicOpName();
    }

    inline const char* GetLogicOpDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLogicOpDescription();
    }

protected:
    const SOA* m_obj_ptr = nullptr;
    Id         m_id;
};

template<typename CONFIG> class EventStateInfoAttachmentArray
{
public:
    using Id = typename CONFIG::Id;
    using SOA = typename CONFIG::SOA;
    using Ref = typename CONFIG::Ref;
    using AttachmentArray = typename CONFIG::AttachmentArray;
    using AttachmentConstArray = typename CONFIG::AttachmentConstArray;
    EventStateInfoAttachmentArray() = default;
    EventStateInfoAttachmentArray(const EventStateInfoAttachmentArray& other) :
        m_obj_ptr(other.m_obj_ptr),
        m_id(other.m_id)
    {
    }
    EventStateInfoAttachmentArray(SOA* obj_ptr, Id id) :
        m_obj_ptr(obj_ptr),
        m_id(id)
    {
    }
    Id   id() const { return m_id; }
    SOA& obj() const { return *m_obj_ptr; }
    bool IsValid() const { return m_obj_ptr != nullptr && m_obj_ptr->IsValidId(m_id); }

    //-----------------------------------------------
    // REF FIELD Attachment: Per target attachment color blend states

    // `Get(uint32_t attachment)` returns the value of the Attachment field of the referenced object
    inline VkPipelineColorBlendAttachmentState Get(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->Attachment(m_id, attachment);
    }

    // `Set(value)` sets the Attachment field of the referenced object
    inline const AttachmentArray& Set(uint32_t                            attachment,
                                      VkPipelineColorBlendAttachmentState value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetAttachment(m_id, attachment, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsAttachmentSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsAttachmentSet(m_id, attachment);
    }

    inline const char* GetAttachmentName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetAttachmentName();
    }

    inline const char* GetAttachmentDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetAttachmentDescription();
    }

protected:
    SOA* m_obj_ptr = nullptr;
    Id   m_id;
};

template<typename CONFIG> class EventStateInfoAttachmentConstArray
{
public:
    using Id = typename CONFIG::Id;
    using SOA = typename CONFIG::SOA;
    using Ref = typename CONFIG::Ref;
    using AttachmentArray = typename CONFIG::AttachmentArray;
    using AttachmentConstArray = typename CONFIG::AttachmentConstArray;
    EventStateInfoAttachmentConstArray() = default;
    EventStateInfoAttachmentConstArray(const EventStateInfoAttachmentArray<CONFIG>& other) :
        m_obj_ptr(&other.obj()),
        m_id(other.id())
    {
    }
    EventStateInfoAttachmentConstArray(const EventStateInfoAttachmentConstArray& other) :
        m_obj_ptr(other.m_obj_ptr),
        m_id(other.m_id)
    {
    }
    EventStateInfoAttachmentConstArray(const SOA* obj_ptr, Id id) :
        m_obj_ptr(obj_ptr),
        m_id(id)
    {
    }
    Id         id() const { return m_id; }
    const SOA& obj() const { return *m_obj_ptr; }
    bool       IsValid() const { return m_obj_ptr != nullptr && m_obj_ptr->IsValidId(m_id); }

    //-----------------------------------------------
    // REF FIELD Attachment: Per target attachment color blend states

    // `Get(uint32_t attachment)` returns the value of the Attachment field of the referenced object
    inline VkPipelineColorBlendAttachmentState Get(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->Attachment(m_id, attachment);
    }

    inline bool IsAttachmentSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsAttachmentSet(m_id, attachment);
    }

    inline const char* GetAttachmentName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetAttachmentName();
    }

    inline const char* GetAttachmentDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetAttachmentDescription();
    }

protected:
    const SOA* m_obj_ptr = nullptr;
    Id         m_id;
};

template<typename CONFIG> class EventStateInfoBlendConstantArray
{
public:
    using Id = typename CONFIG::Id;
    using SOA = typename CONFIG::SOA;
    using Ref = typename CONFIG::Ref;
    using BlendConstantArray = typename CONFIG::BlendConstantArray;
    using BlendConstantConstArray = typename CONFIG::BlendConstantConstArray;
    EventStateInfoBlendConstantArray() = default;
    EventStateInfoBlendConstantArray(const EventStateInfoBlendConstantArray& other) :
        m_obj_ptr(other.m_obj_ptr),
        m_id(other.m_id)
    {
    }
    EventStateInfoBlendConstantArray(SOA* obj_ptr, Id id) :
        m_obj_ptr(obj_ptr),
        m_id(id)
    {
    }
    Id   id() const { return m_id; }
    SOA& obj() const { return *m_obj_ptr; }
    bool IsValid() const { return m_obj_ptr != nullptr && m_obj_ptr->IsValidId(m_id); }

    //-----------------------------------------------
    // REF FIELD BlendConstant: A color constant used for blending

    // `Get(uint32_t channel)` returns the value of the BlendConstant field of the referenced object
    inline float Get(uint32_t channel) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->BlendConstant(m_id, channel);
    }

    // `Set(value)` sets the BlendConstant field of the referenced object
    inline const BlendConstantArray& Set(uint32_t channel, float value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetBlendConstant(m_id, channel, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsBlendConstantSet(uint32_t channel) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsBlendConstantSet(m_id, channel);
    }

    inline const char* GetBlendConstantName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetBlendConstantName();
    }

    inline const char* GetBlendConstantDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetBlendConstantDescription();
    }

protected:
    SOA* m_obj_ptr = nullptr;
    Id   m_id;
};

template<typename CONFIG> class EventStateInfoBlendConstantConstArray
{
public:
    using Id = typename CONFIG::Id;
    using SOA = typename CONFIG::SOA;
    using Ref = typename CONFIG::Ref;
    using BlendConstantArray = typename CONFIG::BlendConstantArray;
    using BlendConstantConstArray = typename CONFIG::BlendConstantConstArray;
    EventStateInfoBlendConstantConstArray() = default;
    EventStateInfoBlendConstantConstArray(const EventStateInfoBlendConstantArray<CONFIG>& other) :
        m_obj_ptr(&other.obj()),
        m_id(other.id())
    {
    }
    EventStateInfoBlendConstantConstArray(const EventStateInfoBlendConstantConstArray& other) :
        m_obj_ptr(other.m_obj_ptr),
        m_id(other.m_id)
    {
    }
    EventStateInfoBlendConstantConstArray(const SOA* obj_ptr, Id id) :
        m_obj_ptr(obj_ptr),
        m_id(id)
    {
    }
    Id         id() const { return m_id; }
    const SOA& obj() const { return *m_obj_ptr; }
    bool       IsValid() const { return m_obj_ptr != nullptr && m_obj_ptr->IsValidId(m_id); }

    //-----------------------------------------------
    // REF FIELD BlendConstant: A color constant used for blending

    // `Get(uint32_t channel)` returns the value of the BlendConstant field of the referenced object
    inline float Get(uint32_t channel) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->BlendConstant(m_id, channel);
    }

    inline bool IsBlendConstantSet(uint32_t channel) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsBlendConstantSet(m_id, channel);
    }

    inline const char* GetBlendConstantName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetBlendConstantName();
    }

    inline const char* GetBlendConstantDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetBlendConstantDescription();
    }

protected:
    const SOA* m_obj_ptr = nullptr;
    Id         m_id;
};

template<typename CONFIG> class EventStateInfoUBWCEnabledArray
{
public:
    using Id = typename CONFIG::Id;
    using SOA = typename CONFIG::SOA;
    using Ref = typename CONFIG::Ref;
    using UBWCEnabledArray = typename CONFIG::UBWCEnabledArray;
    using UBWCEnabledConstArray = typename CONFIG::UBWCEnabledConstArray;
    EventStateInfoUBWCEnabledArray() = default;
    EventStateInfoUBWCEnabledArray(const EventStateInfoUBWCEnabledArray& other) :
        m_obj_ptr(other.m_obj_ptr),
        m_id(other.m_id)
    {
    }
    EventStateInfoUBWCEnabledArray(SOA* obj_ptr, Id id) :
        m_obj_ptr(obj_ptr),
        m_id(id)
    {
    }
    Id   id() const { return m_id; }
    SOA& obj() const { return *m_obj_ptr; }
    bool IsValid() const { return m_obj_ptr != nullptr && m_obj_ptr->IsValidId(m_id); }

    //-----------------------------------------------
    // REF FIELD UBWCEnabled: Whether UBWC is enabled for this attachment

    // `Get(uint32_t attachment)` returns the value of the UBWCEnabled field of the referenced
    // object
    inline bool Get(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->UBWCEnabled(m_id, attachment);
    }

    // `Set(value)` sets the UBWCEnabled field of the referenced object
    inline const UBWCEnabledArray& Set(uint32_t attachment, bool value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetUBWCEnabled(m_id, attachment, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsUBWCEnabledSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsUBWCEnabledSet(m_id, attachment);
    }

    inline const char* GetUBWCEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetUBWCEnabledName();
    }

    inline const char* GetUBWCEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetUBWCEnabledDescription();
    }

protected:
    SOA* m_obj_ptr = nullptr;
    Id   m_id;
};

template<typename CONFIG> class EventStateInfoUBWCEnabledConstArray
{
public:
    using Id = typename CONFIG::Id;
    using SOA = typename CONFIG::SOA;
    using Ref = typename CONFIG::Ref;
    using UBWCEnabledArray = typename CONFIG::UBWCEnabledArray;
    using UBWCEnabledConstArray = typename CONFIG::UBWCEnabledConstArray;
    EventStateInfoUBWCEnabledConstArray() = default;
    EventStateInfoUBWCEnabledConstArray(const EventStateInfoUBWCEnabledArray<CONFIG>& other) :
        m_obj_ptr(&other.obj()),
        m_id(other.id())
    {
    }
    EventStateInfoUBWCEnabledConstArray(const EventStateInfoUBWCEnabledConstArray& other) :
        m_obj_ptr(other.m_obj_ptr),
        m_id(other.m_id)
    {
    }
    EventStateInfoUBWCEnabledConstArray(const SOA* obj_ptr, Id id) :
        m_obj_ptr(obj_ptr),
        m_id(id)
    {
    }
    Id         id() const { return m_id; }
    const SOA& obj() const { return *m_obj_ptr; }
    bool       IsValid() const { return m_obj_ptr != nullptr && m_obj_ptr->IsValidId(m_id); }

    //-----------------------------------------------
    // REF FIELD UBWCEnabled: Whether UBWC is enabled for this attachment

    // `Get(uint32_t attachment)` returns the value of the UBWCEnabled field of the referenced
    // object
    inline bool Get(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->UBWCEnabled(m_id, attachment);
    }

    inline bool IsUBWCEnabledSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsUBWCEnabledSet(m_id, attachment);
    }

    inline const char* GetUBWCEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetUBWCEnabledName();
    }

    inline const char* GetUBWCEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetUBWCEnabledDescription();
    }

protected:
    const SOA* m_obj_ptr = nullptr;
    Id         m_id;
};

template<typename CONFIG> class EventStateInfoUBWCLosslessEnabledArray
{
public:
    using Id = typename CONFIG::Id;
    using SOA = typename CONFIG::SOA;
    using Ref = typename CONFIG::Ref;
    using UBWCLosslessEnabledArray = typename CONFIG::UBWCLosslessEnabledArray;
    using UBWCLosslessEnabledConstArray = typename CONFIG::UBWCLosslessEnabledConstArray;
    EventStateInfoUBWCLosslessEnabledArray() = default;
    EventStateInfoUBWCLosslessEnabledArray(const EventStateInfoUBWCLosslessEnabledArray& other) :
        m_obj_ptr(other.m_obj_ptr),
        m_id(other.m_id)
    {
    }
    EventStateInfoUBWCLosslessEnabledArray(SOA* obj_ptr, Id id) :
        m_obj_ptr(obj_ptr),
        m_id(id)
    {
    }
    Id   id() const { return m_id; }
    SOA& obj() const { return *m_obj_ptr; }
    bool IsValid() const { return m_obj_ptr != nullptr && m_obj_ptr->IsValidId(m_id); }

    //-----------------------------------------------
    // REF FIELD UBWCLosslessEnabled: Whether UBWC Lossless compression (A7XX+) is enabled for this
    // attachment

    // `Get(uint32_t attachment)` returns the value of the UBWCLosslessEnabled field of the
    // referenced object
    inline bool Get(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->UBWCLosslessEnabled(m_id, attachment);
    }

    // `Set(value)` sets the UBWCLosslessEnabled field of the referenced object
    inline const UBWCLosslessEnabledArray& Set(uint32_t attachment, bool value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetUBWCLosslessEnabled(m_id, attachment, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsUBWCLosslessEnabledSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsUBWCLosslessEnabledSet(m_id, attachment);
    }

    inline const char* GetUBWCLosslessEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetUBWCLosslessEnabledName();
    }

    inline const char* GetUBWCLosslessEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetUBWCLosslessEnabledDescription();
    }

protected:
    SOA* m_obj_ptr = nullptr;
    Id   m_id;
};

template<typename CONFIG> class EventStateInfoUBWCLosslessEnabledConstArray
{
public:
    using Id = typename CONFIG::Id;
    using SOA = typename CONFIG::SOA;
    using Ref = typename CONFIG::Ref;
    using UBWCLosslessEnabledArray = typename CONFIG::UBWCLosslessEnabledArray;
    using UBWCLosslessEnabledConstArray = typename CONFIG::UBWCLosslessEnabledConstArray;
    EventStateInfoUBWCLosslessEnabledConstArray() = default;
    EventStateInfoUBWCLosslessEnabledConstArray(
    const EventStateInfoUBWCLosslessEnabledArray<CONFIG>& other) :
        m_obj_ptr(&other.obj()),
        m_id(other.id())
    {
    }
    EventStateInfoUBWCLosslessEnabledConstArray(
    const EventStateInfoUBWCLosslessEnabledConstArray& other) :
        m_obj_ptr(other.m_obj_ptr),
        m_id(other.m_id)
    {
    }
    EventStateInfoUBWCLosslessEnabledConstArray(const SOA* obj_ptr, Id id) :
        m_obj_ptr(obj_ptr),
        m_id(id)
    {
    }
    Id         id() const { return m_id; }
    const SOA& obj() const { return *m_obj_ptr; }
    bool       IsValid() const { return m_obj_ptr != nullptr && m_obj_ptr->IsValidId(m_id); }

    //-----------------------------------------------
    // REF FIELD UBWCLosslessEnabled: Whether UBWC Lossless compression (A7XX+) is enabled for this
    // attachment

    // `Get(uint32_t attachment)` returns the value of the UBWCLosslessEnabled field of the
    // referenced object
    inline bool Get(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->UBWCLosslessEnabled(m_id, attachment);
    }

    inline bool IsUBWCLosslessEnabledSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsUBWCLosslessEnabledSet(m_id, attachment);
    }

    inline const char* GetUBWCLosslessEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetUBWCLosslessEnabledName();
    }

    inline const char* GetUBWCLosslessEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetUBWCLosslessEnabledDescription();
    }

protected:
    const SOA* m_obj_ptr = nullptr;
    Id         m_id;
};

//--------------------------------------------------------------------------------------------------
// Ref represents a reference to a single element.
// Fields can be accessed using e.g. `ref.MyField()` or `ref.SetMyField(42)`.
template<typename CONFIG> class EventStateInfoRefT
{
public:
    using Id = typename CONFIG::Id;
    using SOA = typename CONFIG::SOA;
    using Ref = typename CONFIG::Ref;
    using ConstRef = typename CONFIG::ConstRef;
    using ViewportArray = typename CONFIG::ViewportArray;
    using ViewportConstArray = typename CONFIG::ViewportConstArray;
    using ScissorArray = typename CONFIG::ScissorArray;
    using ScissorConstArray = typename CONFIG::ScissorConstArray;
    using LogicOpEnabledArray = typename CONFIG::LogicOpEnabledArray;
    using LogicOpEnabledConstArray = typename CONFIG::LogicOpEnabledConstArray;
    using LogicOpArray = typename CONFIG::LogicOpArray;
    using LogicOpConstArray = typename CONFIG::LogicOpConstArray;
    using AttachmentArray = typename CONFIG::AttachmentArray;
    using AttachmentConstArray = typename CONFIG::AttachmentConstArray;
    using BlendConstantArray = typename CONFIG::BlendConstantArray;
    using BlendConstantConstArray = typename CONFIG::BlendConstantConstArray;
    using UBWCEnabledArray = typename CONFIG::UBWCEnabledArray;
    using UBWCEnabledConstArray = typename CONFIG::UBWCEnabledConstArray;
    using UBWCLosslessEnabledArray = typename CONFIG::UBWCLosslessEnabledArray;
    using UBWCLosslessEnabledConstArray = typename CONFIG::UBWCLosslessEnabledConstArray;
    EventStateInfoRefT() = default;
    EventStateInfoRefT(const EventStateInfoRefT& other) :
        m_obj_ptr(other.m_obj_ptr),
        m_id(other.m_id)
    {
    }
    EventStateInfoRefT(SOA* obj_ptr, Id id) :
        m_obj_ptr(obj_ptr),
        m_id(id)
    {
    }
    Id   id() const { return m_id; }
    SOA& obj() const { return *m_obj_ptr; }
    bool IsValid() const { return m_obj_ptr != nullptr && m_obj_ptr->IsValidId(m_id); }

    //-----------------------------------------------
    // REF FIELD Topology: The primitive topology for this event

    // `Topology()` returns the value of the Topology field of the referenced object
    inline VkPrimitiveTopology Topology() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->Topology(m_id);
    }

    // `SetTopology(value)` sets the Topology field of the referenced object
    inline const Ref& SetTopology(VkPrimitiveTopology value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetTopology(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsTopologySet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsTopologySet(m_id);
    }

    inline const char* GetTopologyName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetTopologyName();
    }

    inline const char* GetTopologyDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetTopologyDescription();
    }

    //-----------------------------------------------
    // REF FIELD PrimRestartEnabled: Controls whether a special vertex index value is treated as
    // restarting the assembly of primitives

    // `PrimRestartEnabled()` returns the value of the PrimRestartEnabled field of the referenced
    // object
    inline bool PrimRestartEnabled() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->PrimRestartEnabled(m_id);
    }

    // `SetPrimRestartEnabled(value)` sets the PrimRestartEnabled field of the referenced object
    inline const Ref& SetPrimRestartEnabled(bool value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetPrimRestartEnabled(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsPrimRestartEnabledSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsPrimRestartEnabledSet(m_id);
    }

    inline const char* GetPrimRestartEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetPrimRestartEnabledName();
    }

    inline const char* GetPrimRestartEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetPrimRestartEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD PatchControlPoints: Number of control points per patch

    // `PatchControlPoints()` returns the value of the PatchControlPoints field of the referenced
    // object
    inline uint32_t PatchControlPoints() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->PatchControlPoints(m_id);
    }

    // `SetPatchControlPoints(value)` sets the PatchControlPoints field of the referenced object
    inline const Ref& SetPatchControlPoints(uint32_t value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetPatchControlPoints(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsPatchControlPointsSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsPatchControlPointsSet(m_id);
    }

    inline const char* GetPatchControlPointsName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetPatchControlPointsName();
    }

    inline const char* GetPatchControlPointsDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetPatchControlPointsDescription();
    }

    //-----------------------------------------------
    // REF FIELD Viewport: Defines the viewport transforms

    // `Viewport(uint32_t viewport)` returns the value of the Viewport field of the referenced
    // object
    inline VkViewport Viewport(uint32_t viewport) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->Viewport(m_id, viewport);
    }

    // `Viewport()` returns the array of values of the Viewport field of the referenced object
    inline ViewportArray Viewport() const { return ViewportArray(m_obj_ptr, m_id); }

    // `SetViewport(value)` sets the Viewport field of the referenced object
    inline const Ref& SetViewport(uint32_t viewport, VkViewport value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetViewport(m_id, viewport, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsViewportSet(uint32_t viewport) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsViewportSet(m_id, viewport);
    }

    inline const char* GetViewportName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetViewportName();
    }

    inline const char* GetViewportDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetViewportDescription();
    }

    //-----------------------------------------------
    // REF FIELD Scissor: Defines the rectangular bounds of the scissor for the corresponding
    // viewport

    // `Scissor(uint32_t scissor)` returns the value of the Scissor field of the referenced object
    inline VkRect2D Scissor(uint32_t scissor) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->Scissor(m_id, scissor);
    }

    // `Scissor()` returns the array of values of the Scissor field of the referenced object
    inline ScissorArray Scissor() const { return ScissorArray(m_obj_ptr, m_id); }

    // `SetScissor(value)` sets the Scissor field of the referenced object
    inline const Ref& SetScissor(uint32_t scissor, VkRect2D value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetScissor(m_id, scissor, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsScissorSet(uint32_t scissor) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsScissorSet(m_id, scissor);
    }

    inline const char* GetScissorName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetScissorName();
    }

    inline const char* GetScissorDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetScissorDescription();
    }

    //-----------------------------------------------
    // REF FIELD DepthClampEnabled: Controls whether to clamp the fragment’s depth values

    // `DepthClampEnabled()` returns the value of the DepthClampEnabled field of the referenced
    // object
    inline bool DepthClampEnabled() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->DepthClampEnabled(m_id);
    }

    // `SetDepthClampEnabled(value)` sets the DepthClampEnabled field of the referenced object
    inline const Ref& SetDepthClampEnabled(bool value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetDepthClampEnabled(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsDepthClampEnabledSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsDepthClampEnabledSet(m_id);
    }

    inline const char* GetDepthClampEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthClampEnabledName();
    }

    inline const char* GetDepthClampEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthClampEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD RasterizerDiscardEnabled: Controls whether primitives are discarded immediately
    // before the rasterization stage

    // `RasterizerDiscardEnabled()` returns the value of the RasterizerDiscardEnabled field of the
    // referenced object
    inline bool RasterizerDiscardEnabled() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->RasterizerDiscardEnabled(m_id);
    }

    // `SetRasterizerDiscardEnabled(value)` sets the RasterizerDiscardEnabled field of the
    // referenced object
    inline const Ref& SetRasterizerDiscardEnabled(bool value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetRasterizerDiscardEnabled(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsRasterizerDiscardEnabledSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsRasterizerDiscardEnabledSet(m_id);
    }

    inline const char* GetRasterizerDiscardEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetRasterizerDiscardEnabledName();
    }

    inline const char* GetRasterizerDiscardEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetRasterizerDiscardEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD PolygonMode: The triangle rendering mode

    // `PolygonMode()` returns the value of the PolygonMode field of the referenced object
    inline VkPolygonMode PolygonMode() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->PolygonMode(m_id);
    }

    // `SetPolygonMode(value)` sets the PolygonMode field of the referenced object
    inline const Ref& SetPolygonMode(VkPolygonMode value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetPolygonMode(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsPolygonModeSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsPolygonModeSet(m_id);
    }

    inline const char* GetPolygonModeName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetPolygonModeName();
    }

    inline const char* GetPolygonModeDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetPolygonModeDescription();
    }

    //-----------------------------------------------
    // REF FIELD CullMode: The triangle facing direction used for primitive culling

    // `CullMode()` returns the value of the CullMode field of the referenced object
    inline VkCullModeFlags CullMode() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->CullMode(m_id);
    }

    // `SetCullMode(value)` sets the CullMode field of the referenced object
    inline const Ref& SetCullMode(VkCullModeFlags value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetCullMode(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsCullModeSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsCullModeSet(m_id);
    }

    inline const char* GetCullModeName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetCullModeName();
    }

    inline const char* GetCullModeDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetCullModeDescription();
    }

    //-----------------------------------------------
    // REF FIELD FrontFace: A VkFrontFace value specifying the front-facing triangle orientation to
    // be used for culling

    // `FrontFace()` returns the value of the FrontFace field of the referenced object
    inline VkFrontFace FrontFace() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->FrontFace(m_id);
    }

    // `SetFrontFace(value)` sets the FrontFace field of the referenced object
    inline const Ref& SetFrontFace(VkFrontFace value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetFrontFace(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsFrontFaceSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsFrontFaceSet(m_id);
    }

    inline const char* GetFrontFaceName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetFrontFaceName();
    }

    inline const char* GetFrontFaceDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetFrontFaceDescription();
    }

    //-----------------------------------------------
    // REF FIELD DepthBiasEnabled: Whether to bias fragment depth values

    // `DepthBiasEnabled()` returns the value of the DepthBiasEnabled field of the referenced object
    inline bool DepthBiasEnabled() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->DepthBiasEnabled(m_id);
    }

    // `SetDepthBiasEnabled(value)` sets the DepthBiasEnabled field of the referenced object
    inline const Ref& SetDepthBiasEnabled(bool value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetDepthBiasEnabled(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsDepthBiasEnabledSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsDepthBiasEnabledSet(m_id);
    }

    inline const char* GetDepthBiasEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthBiasEnabledName();
    }

    inline const char* GetDepthBiasEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthBiasEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD DepthBiasConstantFactor: A scalar factor controlling the constant depth value added
    // to each fragment.

    // `DepthBiasConstantFactor()` returns the value of the DepthBiasConstantFactor field of the
    // referenced object
    inline float DepthBiasConstantFactor() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->DepthBiasConstantFactor(m_id);
    }

    // `SetDepthBiasConstantFactor(value)` sets the DepthBiasConstantFactor field of the referenced
    // object
    inline const Ref& SetDepthBiasConstantFactor(float value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetDepthBiasConstantFactor(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsDepthBiasConstantFactorSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsDepthBiasConstantFactorSet(m_id);
    }

    inline const char* GetDepthBiasConstantFactorName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthBiasConstantFactorName();
    }

    inline const char* GetDepthBiasConstantFactorDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthBiasConstantFactorDescription();
    }

    //-----------------------------------------------
    // REF FIELD DepthBiasClamp: The maximum (or minimum) depth bias of a fragment

    // `DepthBiasClamp()` returns the value of the DepthBiasClamp field of the referenced object
    inline float DepthBiasClamp() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->DepthBiasClamp(m_id);
    }

    // `SetDepthBiasClamp(value)` sets the DepthBiasClamp field of the referenced object
    inline const Ref& SetDepthBiasClamp(float value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetDepthBiasClamp(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsDepthBiasClampSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsDepthBiasClampSet(m_id);
    }

    inline const char* GetDepthBiasClampName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthBiasClampName();
    }

    inline const char* GetDepthBiasClampDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthBiasClampDescription();
    }

    //-----------------------------------------------
    // REF FIELD DepthBiasSlopeFactor: A scalar factor applied to a fragment’s slope in depth bias
    // calculations

    // `DepthBiasSlopeFactor()` returns the value of the DepthBiasSlopeFactor field of the
    // referenced object
    inline float DepthBiasSlopeFactor() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->DepthBiasSlopeFactor(m_id);
    }

    // `SetDepthBiasSlopeFactor(value)` sets the DepthBiasSlopeFactor field of the referenced object
    inline const Ref& SetDepthBiasSlopeFactor(float value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetDepthBiasSlopeFactor(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsDepthBiasSlopeFactorSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsDepthBiasSlopeFactorSet(m_id);
    }

    inline const char* GetDepthBiasSlopeFactorName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthBiasSlopeFactorName();
    }

    inline const char* GetDepthBiasSlopeFactorDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthBiasSlopeFactorDescription();
    }

    //-----------------------------------------------
    // REF FIELD LineWidth: The width of rasterized line segments

    // `LineWidth()` returns the value of the LineWidth field of the referenced object
    inline float LineWidth() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->LineWidth(m_id);
    }

    // `SetLineWidth(value)` sets the LineWidth field of the referenced object
    inline const Ref& SetLineWidth(float value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetLineWidth(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsLineWidthSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsLineWidthSet(m_id);
    }

    inline const char* GetLineWidthName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLineWidthName();
    }

    inline const char* GetLineWidthDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLineWidthDescription();
    }

    //-----------------------------------------------
    // REF FIELD RasterizationSamples: A VkSampleCountFlagBits value specifying the number of
    // samples used in rasterization

    // `RasterizationSamples()` returns the value of the RasterizationSamples field of the
    // referenced object
    inline VkSampleCountFlagBits RasterizationSamples() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->RasterizationSamples(m_id);
    }

    // `SetRasterizationSamples(value)` sets the RasterizationSamples field of the referenced object
    inline const Ref& SetRasterizationSamples(VkSampleCountFlagBits value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetRasterizationSamples(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsRasterizationSamplesSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsRasterizationSamplesSet(m_id);
    }

    inline const char* GetRasterizationSamplesName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetRasterizationSamplesName();
    }

    inline const char* GetRasterizationSamplesDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetRasterizationSamplesDescription();
    }

    //-----------------------------------------------
    // REF FIELD SampleShadingEnabled: Whether sample shading is enabled

    // `SampleShadingEnabled()` returns the value of the SampleShadingEnabled field of the
    // referenced object
    inline bool SampleShadingEnabled() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->SampleShadingEnabled(m_id);
    }

    // `SetSampleShadingEnabled(value)` sets the SampleShadingEnabled field of the referenced object
    inline const Ref& SetSampleShadingEnabled(bool value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetSampleShadingEnabled(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsSampleShadingEnabledSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsSampleShadingEnabledSet(m_id);
    }

    inline const char* GetSampleShadingEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetSampleShadingEnabledName();
    }

    inline const char* GetSampleShadingEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetSampleShadingEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD MinSampleShading: Specifies a minimum fraction of sample shading if
    // SampleShadingEnable is set to VK_TRUE

    // `MinSampleShading()` returns the value of the MinSampleShading field of the referenced object
    inline float MinSampleShading() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->MinSampleShading(m_id);
    }

    // `SetMinSampleShading(value)` sets the MinSampleShading field of the referenced object
    inline const Ref& SetMinSampleShading(float value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetMinSampleShading(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsMinSampleShadingSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsMinSampleShadingSet(m_id);
    }

    inline const char* GetMinSampleShadingName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetMinSampleShadingName();
    }

    inline const char* GetMinSampleShadingDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetMinSampleShadingDescription();
    }

    //-----------------------------------------------
    // REF FIELD SampleMask: Each bit in the sample mask is associated with a unique sample index as
    // defined for the coverage mask. If the bit is set to 0, the coverage mask bit is set to 0

    // `SampleMask()` returns the value of the SampleMask field of the referenced object
    inline VkSampleMask SampleMask() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->SampleMask(m_id);
    }

    // `SetSampleMask(value)` sets the SampleMask field of the referenced object
    inline const Ref& SetSampleMask(VkSampleMask value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetSampleMask(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsSampleMaskSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsSampleMaskSet(m_id);
    }

    inline const char* GetSampleMaskName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetSampleMaskName();
    }

    inline const char* GetSampleMaskDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetSampleMaskDescription();
    }

    //-----------------------------------------------
    // REF FIELD AlphaToCoverageEnabled: Whether a temporary coverage value is generated based on
    // the alpha component of the fragment’s first color output

    // `AlphaToCoverageEnabled()` returns the value of the AlphaToCoverageEnabled field of the
    // referenced object
    inline bool AlphaToCoverageEnabled() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->AlphaToCoverageEnabled(m_id);
    }

    // `SetAlphaToCoverageEnabled(value)` sets the AlphaToCoverageEnabled field of the referenced
    // object
    inline const Ref& SetAlphaToCoverageEnabled(bool value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetAlphaToCoverageEnabled(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsAlphaToCoverageEnabledSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsAlphaToCoverageEnabledSet(m_id);
    }

    inline const char* GetAlphaToCoverageEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetAlphaToCoverageEnabledName();
    }

    inline const char* GetAlphaToCoverageEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetAlphaToCoverageEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD DepthTestEnabled: Whether depth testing is enabled

    // `DepthTestEnabled()` returns the value of the DepthTestEnabled field of the referenced object
    inline bool DepthTestEnabled() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->DepthTestEnabled(m_id);
    }

    // `SetDepthTestEnabled(value)` sets the DepthTestEnabled field of the referenced object
    inline const Ref& SetDepthTestEnabled(bool value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetDepthTestEnabled(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsDepthTestEnabledSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsDepthTestEnabledSet(m_id);
    }

    inline const char* GetDepthTestEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthTestEnabledName();
    }

    inline const char* GetDepthTestEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthTestEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD DepthWriteEnabled: Whether depth writes are enabled. Depth writes are always
    // disabled when DepthTestEnable is false.

    // `DepthWriteEnabled()` returns the value of the DepthWriteEnabled field of the referenced
    // object
    inline bool DepthWriteEnabled() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->DepthWriteEnabled(m_id);
    }

    // `SetDepthWriteEnabled(value)` sets the DepthWriteEnabled field of the referenced object
    inline const Ref& SetDepthWriteEnabled(bool value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetDepthWriteEnabled(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsDepthWriteEnabledSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsDepthWriteEnabledSet(m_id);
    }

    inline const char* GetDepthWriteEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthWriteEnabledName();
    }

    inline const char* GetDepthWriteEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthWriteEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD DepthCompareOp: Comparison operator used for the depth test

    // `DepthCompareOp()` returns the value of the DepthCompareOp field of the referenced object
    inline VkCompareOp DepthCompareOp() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->DepthCompareOp(m_id);
    }

    // `SetDepthCompareOp(value)` sets the DepthCompareOp field of the referenced object
    inline const Ref& SetDepthCompareOp(VkCompareOp value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetDepthCompareOp(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsDepthCompareOpSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsDepthCompareOpSet(m_id);
    }

    inline const char* GetDepthCompareOpName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthCompareOpName();
    }

    inline const char* GetDepthCompareOpDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthCompareOpDescription();
    }

    //-----------------------------------------------
    // REF FIELD DepthBoundsTestEnabled: Whether depth bounds testing is enabled

    // `DepthBoundsTestEnabled()` returns the value of the DepthBoundsTestEnabled field of the
    // referenced object
    inline bool DepthBoundsTestEnabled() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->DepthBoundsTestEnabled(m_id);
    }

    // `SetDepthBoundsTestEnabled(value)` sets the DepthBoundsTestEnabled field of the referenced
    // object
    inline const Ref& SetDepthBoundsTestEnabled(bool value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetDepthBoundsTestEnabled(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsDepthBoundsTestEnabledSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsDepthBoundsTestEnabledSet(m_id);
    }

    inline const char* GetDepthBoundsTestEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthBoundsTestEnabledName();
    }

    inline const char* GetDepthBoundsTestEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthBoundsTestEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD MinDepthBounds: Minimum depth bound used in the depth bounds test

    // `MinDepthBounds()` returns the value of the MinDepthBounds field of the referenced object
    inline float MinDepthBounds() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->MinDepthBounds(m_id);
    }

    // `SetMinDepthBounds(value)` sets the MinDepthBounds field of the referenced object
    inline const Ref& SetMinDepthBounds(float value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetMinDepthBounds(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsMinDepthBoundsSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsMinDepthBoundsSet(m_id);
    }

    inline const char* GetMinDepthBoundsName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetMinDepthBoundsName();
    }

    inline const char* GetMinDepthBoundsDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetMinDepthBoundsDescription();
    }

    //-----------------------------------------------
    // REF FIELD MaxDepthBounds: Maximum depth bound used in the depth bounds test

    // `MaxDepthBounds()` returns the value of the MaxDepthBounds field of the referenced object
    inline float MaxDepthBounds() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->MaxDepthBounds(m_id);
    }

    // `SetMaxDepthBounds(value)` sets the MaxDepthBounds field of the referenced object
    inline const Ref& SetMaxDepthBounds(float value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetMaxDepthBounds(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsMaxDepthBoundsSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsMaxDepthBoundsSet(m_id);
    }

    inline const char* GetMaxDepthBoundsName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetMaxDepthBoundsName();
    }

    inline const char* GetMaxDepthBoundsDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetMaxDepthBoundsDescription();
    }

    //-----------------------------------------------
    // REF FIELD StencilTestEnabled: Whether stencil testing is enabled

    // `StencilTestEnabled()` returns the value of the StencilTestEnabled field of the referenced
    // object
    inline bool StencilTestEnabled() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->StencilTestEnabled(m_id);
    }

    // `SetStencilTestEnabled(value)` sets the StencilTestEnabled field of the referenced object
    inline const Ref& SetStencilTestEnabled(bool value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetStencilTestEnabled(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsStencilTestEnabledSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsStencilTestEnabledSet(m_id);
    }

    inline const char* GetStencilTestEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetStencilTestEnabledName();
    }

    inline const char* GetStencilTestEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetStencilTestEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD StencilOpStateFront: Front parameter of the stencil test

    // `StencilOpStateFront()` returns the value of the StencilOpStateFront field of the referenced
    // object
    inline VkStencilOpState StencilOpStateFront() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->StencilOpStateFront(m_id);
    }

    // `SetStencilOpStateFront(value)` sets the StencilOpStateFront field of the referenced object
    inline const Ref& SetStencilOpStateFront(VkStencilOpState value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetStencilOpStateFront(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsStencilOpStateFrontSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsStencilOpStateFrontSet(m_id);
    }

    inline const char* GetStencilOpStateFrontName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetStencilOpStateFrontName();
    }

    inline const char* GetStencilOpStateFrontDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetStencilOpStateFrontDescription();
    }

    //-----------------------------------------------
    // REF FIELD StencilOpStateBack: Back parameter of the stencil test

    // `StencilOpStateBack()` returns the value of the StencilOpStateBack field of the referenced
    // object
    inline VkStencilOpState StencilOpStateBack() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->StencilOpStateBack(m_id);
    }

    // `SetStencilOpStateBack(value)` sets the StencilOpStateBack field of the referenced object
    inline const Ref& SetStencilOpStateBack(VkStencilOpState value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetStencilOpStateBack(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsStencilOpStateBackSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsStencilOpStateBackSet(m_id);
    }

    inline const char* GetStencilOpStateBackName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetStencilOpStateBackName();
    }

    inline const char* GetStencilOpStateBackDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetStencilOpStateBackDescription();
    }

    //-----------------------------------------------
    // REF FIELD LogicOpEnabled: Whether to apply Logical Operations

    // `LogicOpEnabled(uint32_t attachment)` returns the value of the LogicOpEnabled field of the
    // referenced object
    inline bool LogicOpEnabled(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->LogicOpEnabled(m_id, attachment);
    }

    // `LogicOpEnabled()` returns the array of values of the LogicOpEnabled field of the referenced
    // object
    inline LogicOpEnabledArray LogicOpEnabled() const
    {
        return LogicOpEnabledArray(m_obj_ptr, m_id);
    }

    // `SetLogicOpEnabled(value)` sets the LogicOpEnabled field of the referenced object
    inline const Ref& SetLogicOpEnabled(uint32_t attachment, bool value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetLogicOpEnabled(m_id, attachment, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsLogicOpEnabledSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsLogicOpEnabledSet(m_id, attachment);
    }

    inline const char* GetLogicOpEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLogicOpEnabledName();
    }

    inline const char* GetLogicOpEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLogicOpEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD LogicOp: Which logical operation to apply

    // `LogicOp(uint32_t attachment)` returns the value of the LogicOp field of the referenced
    // object
    inline VkLogicOp LogicOp(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->LogicOp(m_id, attachment);
    }

    // `LogicOp()` returns the array of values of the LogicOp field of the referenced object
    inline LogicOpArray LogicOp() const { return LogicOpArray(m_obj_ptr, m_id); }

    // `SetLogicOp(value)` sets the LogicOp field of the referenced object
    inline const Ref& SetLogicOp(uint32_t attachment, VkLogicOp value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetLogicOp(m_id, attachment, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsLogicOpSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsLogicOpSet(m_id, attachment);
    }

    inline const char* GetLogicOpName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLogicOpName();
    }

    inline const char* GetLogicOpDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLogicOpDescription();
    }

    //-----------------------------------------------
    // REF FIELD Attachment: Per target attachment color blend states

    // `Attachment(uint32_t attachment)` returns the value of the Attachment field of the referenced
    // object
    inline VkPipelineColorBlendAttachmentState Attachment(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->Attachment(m_id, attachment);
    }

    // `Attachment()` returns the array of values of the Attachment field of the referenced object
    inline AttachmentArray Attachment() const { return AttachmentArray(m_obj_ptr, m_id); }

    // `SetAttachment(value)` sets the Attachment field of the referenced object
    inline const Ref& SetAttachment(uint32_t                            attachment,
                                    VkPipelineColorBlendAttachmentState value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetAttachment(m_id, attachment, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsAttachmentSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsAttachmentSet(m_id, attachment);
    }

    inline const char* GetAttachmentName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetAttachmentName();
    }

    inline const char* GetAttachmentDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetAttachmentDescription();
    }

    //-----------------------------------------------
    // REF FIELD BlendConstant: A color constant used for blending

    // `BlendConstant(uint32_t channel)` returns the value of the BlendConstant field of the
    // referenced object
    inline float BlendConstant(uint32_t channel) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->BlendConstant(m_id, channel);
    }

    // `BlendConstant()` returns the array of values of the BlendConstant field of the referenced
    // object
    inline BlendConstantArray BlendConstant() const { return BlendConstantArray(m_obj_ptr, m_id); }

    // `SetBlendConstant(value)` sets the BlendConstant field of the referenced object
    inline const Ref& SetBlendConstant(uint32_t channel, float value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetBlendConstant(m_id, channel, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsBlendConstantSet(uint32_t channel) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsBlendConstantSet(m_id, channel);
    }

    inline const char* GetBlendConstantName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetBlendConstantName();
    }

    inline const char* GetBlendConstantDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetBlendConstantDescription();
    }

    //-----------------------------------------------
    // REF FIELD LRZEnabled: Whether LRZ is enabled for depth

    // `LRZEnabled()` returns the value of the LRZEnabled field of the referenced object
    inline bool LRZEnabled() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->LRZEnabled(m_id);
    }

    // `SetLRZEnabled(value)` sets the LRZEnabled field of the referenced object
    inline const Ref& SetLRZEnabled(bool value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetLRZEnabled(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsLRZEnabledSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsLRZEnabledSet(m_id);
    }

    inline const char* GetLRZEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLRZEnabledName();
    }

    inline const char* GetLRZEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLRZEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD LRZWrite: Whether LRZ write is enabled

    // `LRZWrite()` returns the value of the LRZWrite field of the referenced object
    inline bool LRZWrite() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->LRZWrite(m_id);
    }

    // `SetLRZWrite(value)` sets the LRZWrite field of the referenced object
    inline const Ref& SetLRZWrite(bool value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetLRZWrite(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsLRZWriteSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsLRZWriteSet(m_id);
    }

    inline const char* GetLRZWriteName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLRZWriteName();
    }

    inline const char* GetLRZWriteDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLRZWriteDescription();
    }

    //-----------------------------------------------
    // REF FIELD LRZDirStatus: LRZ direction

    // `LRZDirStatus()` returns the value of the LRZDirStatus field of the referenced object
    inline a6xx_lrz_dir_status LRZDirStatus() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->LRZDirStatus(m_id);
    }

    // `SetLRZDirStatus(value)` sets the LRZDirStatus field of the referenced object
    inline const Ref& SetLRZDirStatus(a6xx_lrz_dir_status value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetLRZDirStatus(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsLRZDirStatusSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsLRZDirStatusSet(m_id);
    }

    inline const char* GetLRZDirStatusName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLRZDirStatusName();
    }

    inline const char* GetLRZDirStatusDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLRZDirStatusDescription();
    }

    //-----------------------------------------------
    // REF FIELD LRZDirWrite: Whether LRZ direction write is enabled

    // `LRZDirWrite()` returns the value of the LRZDirWrite field of the referenced object
    inline bool LRZDirWrite() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->LRZDirWrite(m_id);
    }

    // `SetLRZDirWrite(value)` sets the LRZDirWrite field of the referenced object
    inline const Ref& SetLRZDirWrite(bool value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetLRZDirWrite(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsLRZDirWriteSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsLRZDirWriteSet(m_id);
    }

    inline const char* GetLRZDirWriteName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLRZDirWriteName();
    }

    inline const char* GetLRZDirWriteDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLRZDirWriteDescription();
    }

    //-----------------------------------------------
    // REF FIELD ZTestMode: Depth test mode

    // `ZTestMode()` returns the value of the ZTestMode field of the referenced object
    inline a6xx_ztest_mode ZTestMode() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->ZTestMode(m_id);
    }

    // `SetZTestMode(value)` sets the ZTestMode field of the referenced object
    inline const Ref& SetZTestMode(a6xx_ztest_mode value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetZTestMode(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsZTestModeSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsZTestModeSet(m_id);
    }

    inline const char* GetZTestModeName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetZTestModeName();
    }

    inline const char* GetZTestModeDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetZTestModeDescription();
    }

    //-----------------------------------------------
    // REF FIELD BinW: Bin width

    // `BinW()` returns the value of the BinW field of the referenced object
    inline uint32_t BinW() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->BinW(m_id);
    }

    // `SetBinW(value)` sets the BinW field of the referenced object
    inline const Ref& SetBinW(uint32_t value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetBinW(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsBinWSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsBinWSet(m_id);
    }

    inline const char* GetBinWName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetBinWName();
    }

    inline const char* GetBinWDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetBinWDescription();
    }

    //-----------------------------------------------
    // REF FIELD BinH: Bin Height

    // `BinH()` returns the value of the BinH field of the referenced object
    inline uint32_t BinH() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->BinH(m_id);
    }

    // `SetBinH(value)` sets the BinH field of the referenced object
    inline const Ref& SetBinH(uint32_t value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetBinH(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsBinHSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsBinHSet(m_id);
    }

    inline const char* GetBinHName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetBinHName();
    }

    inline const char* GetBinHDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetBinHDescription();
    }

    //-----------------------------------------------
    // REF FIELD WindowScissorTLX: Window scissor Top Left X-coordinate

    // `WindowScissorTLX()` returns the value of the WindowScissorTLX field of the referenced object
    inline uint16_t WindowScissorTLX() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->WindowScissorTLX(m_id);
    }

    // `SetWindowScissorTLX(value)` sets the WindowScissorTLX field of the referenced object
    inline const Ref& SetWindowScissorTLX(uint16_t value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetWindowScissorTLX(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsWindowScissorTLXSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsWindowScissorTLXSet(m_id);
    }

    inline const char* GetWindowScissorTLXName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetWindowScissorTLXName();
    }

    inline const char* GetWindowScissorTLXDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetWindowScissorTLXDescription();
    }

    //-----------------------------------------------
    // REF FIELD WindowScissorTLY: Window scissor Top Left Y-coordinate

    // `WindowScissorTLY()` returns the value of the WindowScissorTLY field of the referenced object
    inline uint16_t WindowScissorTLY() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->WindowScissorTLY(m_id);
    }

    // `SetWindowScissorTLY(value)` sets the WindowScissorTLY field of the referenced object
    inline const Ref& SetWindowScissorTLY(uint16_t value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetWindowScissorTLY(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsWindowScissorTLYSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsWindowScissorTLYSet(m_id);
    }

    inline const char* GetWindowScissorTLYName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetWindowScissorTLYName();
    }

    inline const char* GetWindowScissorTLYDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetWindowScissorTLYDescription();
    }

    //-----------------------------------------------
    // REF FIELD WindowScissorBRX: Window scissor Bottom Right X-coordinate

    // `WindowScissorBRX()` returns the value of the WindowScissorBRX field of the referenced object
    inline uint16_t WindowScissorBRX() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->WindowScissorBRX(m_id);
    }

    // `SetWindowScissorBRX(value)` sets the WindowScissorBRX field of the referenced object
    inline const Ref& SetWindowScissorBRX(uint16_t value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetWindowScissorBRX(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsWindowScissorBRXSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsWindowScissorBRXSet(m_id);
    }

    inline const char* GetWindowScissorBRXName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetWindowScissorBRXName();
    }

    inline const char* GetWindowScissorBRXDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetWindowScissorBRXDescription();
    }

    //-----------------------------------------------
    // REF FIELD WindowScissorBRY: Window scissor Bottom Right Y-coordinate

    // `WindowScissorBRY()` returns the value of the WindowScissorBRY field of the referenced object
    inline uint16_t WindowScissorBRY() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->WindowScissorBRY(m_id);
    }

    // `SetWindowScissorBRY(value)` sets the WindowScissorBRY field of the referenced object
    inline const Ref& SetWindowScissorBRY(uint16_t value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetWindowScissorBRY(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsWindowScissorBRYSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsWindowScissorBRYSet(m_id);
    }

    inline const char* GetWindowScissorBRYName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetWindowScissorBRYName();
    }

    inline const char* GetWindowScissorBRYDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetWindowScissorBRYDescription();
    }

    //-----------------------------------------------
    // REF FIELD RenderMode: Whether in binning pass or rendering pass

    // `RenderMode()` returns the value of the RenderMode field of the referenced object
    inline a6xx_render_mode RenderMode() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->RenderMode(m_id);
    }

    // `SetRenderMode(value)` sets the RenderMode field of the referenced object
    inline const Ref& SetRenderMode(a6xx_render_mode value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetRenderMode(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsRenderModeSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsRenderModeSet(m_id);
    }

    inline const char* GetRenderModeName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetRenderModeName();
    }

    inline const char* GetRenderModeDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetRenderModeDescription();
    }

    //-----------------------------------------------
    // REF FIELD BuffersLocation: Whether the target buffer is in GMEM or SYSMEM

    // `BuffersLocation()` returns the value of the BuffersLocation field of the referenced object
    inline a6xx_buffers_location BuffersLocation() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->BuffersLocation(m_id);
    }

    // `SetBuffersLocation(value)` sets the BuffersLocation field of the referenced object
    inline const Ref& SetBuffersLocation(a6xx_buffers_location value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetBuffersLocation(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsBuffersLocationSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsBuffersLocationSet(m_id);
    }

    inline const char* GetBuffersLocationName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetBuffersLocationName();
    }

    inline const char* GetBuffersLocationDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetBuffersLocationDescription();
    }

    //-----------------------------------------------
    // REF FIELD ThreadSize: Whether the thread size is 64 or 128

    // `ThreadSize()` returns the value of the ThreadSize field of the referenced object
    inline a6xx_threadsize ThreadSize() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->ThreadSize(m_id);
    }

    // `SetThreadSize(value)` sets the ThreadSize field of the referenced object
    inline const Ref& SetThreadSize(a6xx_threadsize value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetThreadSize(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsThreadSizeSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsThreadSizeSet(m_id);
    }

    inline const char* GetThreadSizeName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetThreadSizeName();
    }

    inline const char* GetThreadSizeDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetThreadSizeDescription();
    }

    //-----------------------------------------------
    // REF FIELD EnableAllHelperLanes: Whether all helper lanes are enabled of the 2x2 quad for fine
    // derivatives

    // `EnableAllHelperLanes()` returns the value of the EnableAllHelperLanes field of the
    // referenced object
    inline bool EnableAllHelperLanes() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->EnableAllHelperLanes(m_id);
    }

    // `SetEnableAllHelperLanes(value)` sets the EnableAllHelperLanes field of the referenced object
    inline const Ref& SetEnableAllHelperLanes(bool value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetEnableAllHelperLanes(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsEnableAllHelperLanesSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsEnableAllHelperLanesSet(m_id);
    }

    inline const char* GetEnableAllHelperLanesName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetEnableAllHelperLanesName();
    }

    inline const char* GetEnableAllHelperLanesDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetEnableAllHelperLanesDescription();
    }

    //-----------------------------------------------
    // REF FIELD EnablePartialHelperLanes: Whether 3 out of 4 helper lanes are enabled of the 2x2
    // quad for coarse derivatives

    // `EnablePartialHelperLanes()` returns the value of the EnablePartialHelperLanes field of the
    // referenced object
    inline bool EnablePartialHelperLanes() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->EnablePartialHelperLanes(m_id);
    }

    // `SetEnablePartialHelperLanes(value)` sets the EnablePartialHelperLanes field of the
    // referenced object
    inline const Ref& SetEnablePartialHelperLanes(bool value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetEnablePartialHelperLanes(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsEnablePartialHelperLanesSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsEnablePartialHelperLanesSet(m_id);
    }

    inline const char* GetEnablePartialHelperLanesName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetEnablePartialHelperLanesName();
    }

    inline const char* GetEnablePartialHelperLanesDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetEnablePartialHelperLanesDescription();
    }

    //-----------------------------------------------
    // REF FIELD UBWCEnabled: Whether UBWC is enabled for this attachment

    // `UBWCEnabled(uint32_t attachment)` returns the value of the UBWCEnabled field of the
    // referenced object
    inline bool UBWCEnabled(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->UBWCEnabled(m_id, attachment);
    }

    // `UBWCEnabled()` returns the array of values of the UBWCEnabled field of the referenced object
    inline UBWCEnabledArray UBWCEnabled() const { return UBWCEnabledArray(m_obj_ptr, m_id); }

    // `SetUBWCEnabled(value)` sets the UBWCEnabled field of the referenced object
    inline const Ref& SetUBWCEnabled(uint32_t attachment, bool value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetUBWCEnabled(m_id, attachment, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsUBWCEnabledSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsUBWCEnabledSet(m_id, attachment);
    }

    inline const char* GetUBWCEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetUBWCEnabledName();
    }

    inline const char* GetUBWCEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetUBWCEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD UBWCLosslessEnabled: Whether UBWC Lossless compression (A7XX+) is enabled for this
    // attachment

    // `UBWCLosslessEnabled(uint32_t attachment)` returns the value of the UBWCLosslessEnabled field
    // of the referenced object
    inline bool UBWCLosslessEnabled(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->UBWCLosslessEnabled(m_id, attachment);
    }

    // `UBWCLosslessEnabled()` returns the array of values of the UBWCLosslessEnabled field of the
    // referenced object
    inline UBWCLosslessEnabledArray UBWCLosslessEnabled() const
    {
        return UBWCLosslessEnabledArray(m_obj_ptr, m_id);
    }

    // `SetUBWCLosslessEnabled(value)` sets the UBWCLosslessEnabled field of the referenced object
    inline const Ref& SetUBWCLosslessEnabled(uint32_t attachment, bool value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetUBWCLosslessEnabled(m_id, attachment, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsUBWCLosslessEnabledSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsUBWCLosslessEnabledSet(m_id, attachment);
    }

    inline const char* GetUBWCLosslessEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetUBWCLosslessEnabledName();
    }

    inline const char* GetUBWCLosslessEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetUBWCLosslessEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD UBWCEnabledOnDS: Whether UBWC is enabled for this depth stencil attachment

    // `UBWCEnabledOnDS()` returns the value of the UBWCEnabledOnDS field of the referenced object
    inline bool UBWCEnabledOnDS() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->UBWCEnabledOnDS(m_id);
    }

    // `SetUBWCEnabledOnDS(value)` sets the UBWCEnabledOnDS field of the referenced object
    inline const Ref& SetUBWCEnabledOnDS(bool value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetUBWCEnabledOnDS(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsUBWCEnabledOnDSSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsUBWCEnabledOnDSSet(m_id);
    }

    inline const char* GetUBWCEnabledOnDSName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetUBWCEnabledOnDSName();
    }

    inline const char* GetUBWCEnabledOnDSDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetUBWCEnabledOnDSDescription();
    }

    //-----------------------------------------------
    // REF FIELD UBWCLosslessEnabledOnDS: Whether UBWC Lossless compression (A7XX+) is enabled for
    // this depth stencil attachment

    // `UBWCLosslessEnabledOnDS()` returns the value of the UBWCLosslessEnabledOnDS field of the
    // referenced object
    inline bool UBWCLosslessEnabledOnDS() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->UBWCLosslessEnabledOnDS(m_id);
    }

    // `SetUBWCLosslessEnabledOnDS(value)` sets the UBWCLosslessEnabledOnDS field of the referenced
    // object
    inline const Ref& SetUBWCLosslessEnabledOnDS(bool value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetUBWCLosslessEnabledOnDS(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsUBWCLosslessEnabledOnDSSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsUBWCLosslessEnabledOnDSSet(m_id);
    }

    inline const char* GetUBWCLosslessEnabledOnDSName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetUBWCLosslessEnabledOnDSName();
    }

    inline const char* GetUBWCLosslessEnabledOnDSDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetUBWCLosslessEnabledOnDSDescription();
    }

    EventStateInfoRefT& operator=(const EventStateInfoRefT& other) = delete;
    void                assign(const SOA& other_obj, Id other_id) const;
    void                swap(const Ref& other) const;
    friend void         swap(const Ref& x, const Ref& y) { x.swap(y); }

protected:
    template<typename CONFIG_> friend class EventStateInfoT;
    template<typename Class, typename Id, typename RefT> friend class StructOfArraysIterator;
    SOA* m_obj_ptr = nullptr;
    Id   m_id;
};

//--------------------------------------------------------------------------------------------------
// ConstRef represents a const reference to a single element.
// Fields can be accessed using e.g. `ref.MyField()`.
template<typename CONFIG> class EventStateInfoConstRefT
{
public:
    using Id = typename CONFIG::Id;
    using SOA = typename CONFIG::SOA;
    using Ref = typename CONFIG::Ref;
    using ConstRef = typename CONFIG::ConstRef;
    using ViewportArray = typename CONFIG::ViewportArray;
    using ViewportConstArray = typename CONFIG::ViewportConstArray;
    using ScissorArray = typename CONFIG::ScissorArray;
    using ScissorConstArray = typename CONFIG::ScissorConstArray;
    using LogicOpEnabledArray = typename CONFIG::LogicOpEnabledArray;
    using LogicOpEnabledConstArray = typename CONFIG::LogicOpEnabledConstArray;
    using LogicOpArray = typename CONFIG::LogicOpArray;
    using LogicOpConstArray = typename CONFIG::LogicOpConstArray;
    using AttachmentArray = typename CONFIG::AttachmentArray;
    using AttachmentConstArray = typename CONFIG::AttachmentConstArray;
    using BlendConstantArray = typename CONFIG::BlendConstantArray;
    using BlendConstantConstArray = typename CONFIG::BlendConstantConstArray;
    using UBWCEnabledArray = typename CONFIG::UBWCEnabledArray;
    using UBWCEnabledConstArray = typename CONFIG::UBWCEnabledConstArray;
    using UBWCLosslessEnabledArray = typename CONFIG::UBWCLosslessEnabledArray;
    using UBWCLosslessEnabledConstArray = typename CONFIG::UBWCLosslessEnabledConstArray;
    EventStateInfoConstRefT() = default;
    EventStateInfoConstRefT(const Ref& other) :
        m_obj_ptr(&other.obj()),
        m_id(other.id())
    {
    }
    EventStateInfoConstRefT(const EventStateInfoConstRefT& other) :
        m_obj_ptr(other.m_obj_ptr),
        m_id(other.m_id)
    {
    }
    EventStateInfoConstRefT(const SOA* obj_ptr, Id id) :
        m_obj_ptr(obj_ptr),
        m_id(id)
    {
    }
    Id         id() const { return m_id; }
    const SOA& obj() const { return *m_obj_ptr; }
    bool       IsValid() const { return m_obj_ptr != nullptr && m_obj_ptr->IsValidId(m_id); }

    //-----------------------------------------------
    // REF FIELD Topology: The primitive topology for this event

    // `Topology()` returns the value of the Topology field of the referenced object
    inline VkPrimitiveTopology Topology() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->Topology(m_id);
    }

    inline bool IsTopologySet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsTopologySet(m_id);
    }

    inline const char* GetTopologyName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetTopologyName();
    }

    inline const char* GetTopologyDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetTopologyDescription();
    }

    //-----------------------------------------------
    // REF FIELD PrimRestartEnabled: Controls whether a special vertex index value is treated as
    // restarting the assembly of primitives

    // `PrimRestartEnabled()` returns the value of the PrimRestartEnabled field of the referenced
    // object
    inline bool PrimRestartEnabled() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->PrimRestartEnabled(m_id);
    }

    inline bool IsPrimRestartEnabledSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsPrimRestartEnabledSet(m_id);
    }

    inline const char* GetPrimRestartEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetPrimRestartEnabledName();
    }

    inline const char* GetPrimRestartEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetPrimRestartEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD PatchControlPoints: Number of control points per patch

    // `PatchControlPoints()` returns the value of the PatchControlPoints field of the referenced
    // object
    inline uint32_t PatchControlPoints() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->PatchControlPoints(m_id);
    }

    inline bool IsPatchControlPointsSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsPatchControlPointsSet(m_id);
    }

    inline const char* GetPatchControlPointsName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetPatchControlPointsName();
    }

    inline const char* GetPatchControlPointsDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetPatchControlPointsDescription();
    }

    //-----------------------------------------------
    // REF FIELD Viewport: Defines the viewport transforms

    // `Viewport(uint32_t viewport)` returns the value of the Viewport field of the referenced
    // object
    inline VkViewport Viewport(uint32_t viewport) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->Viewport(m_id, viewport);
    }

    // `Viewport()` returns the array of values of the Viewport field of the referenced object
    inline ViewportConstArray Viewport() const { return ViewportConstArray(m_obj_ptr, m_id); }

    inline bool IsViewportSet(uint32_t viewport) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsViewportSet(m_id, viewport);
    }

    inline const char* GetViewportName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetViewportName();
    }

    inline const char* GetViewportDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetViewportDescription();
    }

    //-----------------------------------------------
    // REF FIELD Scissor: Defines the rectangular bounds of the scissor for the corresponding
    // viewport

    // `Scissor(uint32_t scissor)` returns the value of the Scissor field of the referenced object
    inline VkRect2D Scissor(uint32_t scissor) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->Scissor(m_id, scissor);
    }

    // `Scissor()` returns the array of values of the Scissor field of the referenced object
    inline ScissorConstArray Scissor() const { return ScissorConstArray(m_obj_ptr, m_id); }

    inline bool IsScissorSet(uint32_t scissor) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsScissorSet(m_id, scissor);
    }

    inline const char* GetScissorName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetScissorName();
    }

    inline const char* GetScissorDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetScissorDescription();
    }

    //-----------------------------------------------
    // REF FIELD DepthClampEnabled: Controls whether to clamp the fragment’s depth values

    // `DepthClampEnabled()` returns the value of the DepthClampEnabled field of the referenced
    // object
    inline bool DepthClampEnabled() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->DepthClampEnabled(m_id);
    }

    inline bool IsDepthClampEnabledSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsDepthClampEnabledSet(m_id);
    }

    inline const char* GetDepthClampEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthClampEnabledName();
    }

    inline const char* GetDepthClampEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthClampEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD RasterizerDiscardEnabled: Controls whether primitives are discarded immediately
    // before the rasterization stage

    // `RasterizerDiscardEnabled()` returns the value of the RasterizerDiscardEnabled field of the
    // referenced object
    inline bool RasterizerDiscardEnabled() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->RasterizerDiscardEnabled(m_id);
    }

    inline bool IsRasterizerDiscardEnabledSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsRasterizerDiscardEnabledSet(m_id);
    }

    inline const char* GetRasterizerDiscardEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetRasterizerDiscardEnabledName();
    }

    inline const char* GetRasterizerDiscardEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetRasterizerDiscardEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD PolygonMode: The triangle rendering mode

    // `PolygonMode()` returns the value of the PolygonMode field of the referenced object
    inline VkPolygonMode PolygonMode() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->PolygonMode(m_id);
    }

    inline bool IsPolygonModeSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsPolygonModeSet(m_id);
    }

    inline const char* GetPolygonModeName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetPolygonModeName();
    }

    inline const char* GetPolygonModeDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetPolygonModeDescription();
    }

    //-----------------------------------------------
    // REF FIELD CullMode: The triangle facing direction used for primitive culling

    // `CullMode()` returns the value of the CullMode field of the referenced object
    inline VkCullModeFlags CullMode() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->CullMode(m_id);
    }

    inline bool IsCullModeSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsCullModeSet(m_id);
    }

    inline const char* GetCullModeName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetCullModeName();
    }

    inline const char* GetCullModeDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetCullModeDescription();
    }

    //-----------------------------------------------
    // REF FIELD FrontFace: A VkFrontFace value specifying the front-facing triangle orientation to
    // be used for culling

    // `FrontFace()` returns the value of the FrontFace field of the referenced object
    inline VkFrontFace FrontFace() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->FrontFace(m_id);
    }

    inline bool IsFrontFaceSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsFrontFaceSet(m_id);
    }

    inline const char* GetFrontFaceName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetFrontFaceName();
    }

    inline const char* GetFrontFaceDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetFrontFaceDescription();
    }

    //-----------------------------------------------
    // REF FIELD DepthBiasEnabled: Whether to bias fragment depth values

    // `DepthBiasEnabled()` returns the value of the DepthBiasEnabled field of the referenced object
    inline bool DepthBiasEnabled() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->DepthBiasEnabled(m_id);
    }

    inline bool IsDepthBiasEnabledSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsDepthBiasEnabledSet(m_id);
    }

    inline const char* GetDepthBiasEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthBiasEnabledName();
    }

    inline const char* GetDepthBiasEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthBiasEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD DepthBiasConstantFactor: A scalar factor controlling the constant depth value added
    // to each fragment.

    // `DepthBiasConstantFactor()` returns the value of the DepthBiasConstantFactor field of the
    // referenced object
    inline float DepthBiasConstantFactor() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->DepthBiasConstantFactor(m_id);
    }

    inline bool IsDepthBiasConstantFactorSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsDepthBiasConstantFactorSet(m_id);
    }

    inline const char* GetDepthBiasConstantFactorName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthBiasConstantFactorName();
    }

    inline const char* GetDepthBiasConstantFactorDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthBiasConstantFactorDescription();
    }

    //-----------------------------------------------
    // REF FIELD DepthBiasClamp: The maximum (or minimum) depth bias of a fragment

    // `DepthBiasClamp()` returns the value of the DepthBiasClamp field of the referenced object
    inline float DepthBiasClamp() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->DepthBiasClamp(m_id);
    }

    inline bool IsDepthBiasClampSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsDepthBiasClampSet(m_id);
    }

    inline const char* GetDepthBiasClampName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthBiasClampName();
    }

    inline const char* GetDepthBiasClampDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthBiasClampDescription();
    }

    //-----------------------------------------------
    // REF FIELD DepthBiasSlopeFactor: A scalar factor applied to a fragment’s slope in depth bias
    // calculations

    // `DepthBiasSlopeFactor()` returns the value of the DepthBiasSlopeFactor field of the
    // referenced object
    inline float DepthBiasSlopeFactor() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->DepthBiasSlopeFactor(m_id);
    }

    inline bool IsDepthBiasSlopeFactorSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsDepthBiasSlopeFactorSet(m_id);
    }

    inline const char* GetDepthBiasSlopeFactorName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthBiasSlopeFactorName();
    }

    inline const char* GetDepthBiasSlopeFactorDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthBiasSlopeFactorDescription();
    }

    //-----------------------------------------------
    // REF FIELD LineWidth: The width of rasterized line segments

    // `LineWidth()` returns the value of the LineWidth field of the referenced object
    inline float LineWidth() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->LineWidth(m_id);
    }

    inline bool IsLineWidthSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsLineWidthSet(m_id);
    }

    inline const char* GetLineWidthName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLineWidthName();
    }

    inline const char* GetLineWidthDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLineWidthDescription();
    }

    //-----------------------------------------------
    // REF FIELD RasterizationSamples: A VkSampleCountFlagBits value specifying the number of
    // samples used in rasterization

    // `RasterizationSamples()` returns the value of the RasterizationSamples field of the
    // referenced object
    inline VkSampleCountFlagBits RasterizationSamples() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->RasterizationSamples(m_id);
    }

    inline bool IsRasterizationSamplesSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsRasterizationSamplesSet(m_id);
    }

    inline const char* GetRasterizationSamplesName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetRasterizationSamplesName();
    }

    inline const char* GetRasterizationSamplesDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetRasterizationSamplesDescription();
    }

    //-----------------------------------------------
    // REF FIELD SampleShadingEnabled: Whether sample shading is enabled

    // `SampleShadingEnabled()` returns the value of the SampleShadingEnabled field of the
    // referenced object
    inline bool SampleShadingEnabled() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->SampleShadingEnabled(m_id);
    }

    inline bool IsSampleShadingEnabledSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsSampleShadingEnabledSet(m_id);
    }

    inline const char* GetSampleShadingEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetSampleShadingEnabledName();
    }

    inline const char* GetSampleShadingEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetSampleShadingEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD MinSampleShading: Specifies a minimum fraction of sample shading if
    // SampleShadingEnable is set to VK_TRUE

    // `MinSampleShading()` returns the value of the MinSampleShading field of the referenced object
    inline float MinSampleShading() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->MinSampleShading(m_id);
    }

    inline bool IsMinSampleShadingSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsMinSampleShadingSet(m_id);
    }

    inline const char* GetMinSampleShadingName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetMinSampleShadingName();
    }

    inline const char* GetMinSampleShadingDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetMinSampleShadingDescription();
    }

    //-----------------------------------------------
    // REF FIELD SampleMask: Each bit in the sample mask is associated with a unique sample index as
    // defined for the coverage mask. If the bit is set to 0, the coverage mask bit is set to 0

    // `SampleMask()` returns the value of the SampleMask field of the referenced object
    inline VkSampleMask SampleMask() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->SampleMask(m_id);
    }

    inline bool IsSampleMaskSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsSampleMaskSet(m_id);
    }

    inline const char* GetSampleMaskName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetSampleMaskName();
    }

    inline const char* GetSampleMaskDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetSampleMaskDescription();
    }

    //-----------------------------------------------
    // REF FIELD AlphaToCoverageEnabled: Whether a temporary coverage value is generated based on
    // the alpha component of the fragment’s first color output

    // `AlphaToCoverageEnabled()` returns the value of the AlphaToCoverageEnabled field of the
    // referenced object
    inline bool AlphaToCoverageEnabled() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->AlphaToCoverageEnabled(m_id);
    }

    inline bool IsAlphaToCoverageEnabledSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsAlphaToCoverageEnabledSet(m_id);
    }

    inline const char* GetAlphaToCoverageEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetAlphaToCoverageEnabledName();
    }

    inline const char* GetAlphaToCoverageEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetAlphaToCoverageEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD DepthTestEnabled: Whether depth testing is enabled

    // `DepthTestEnabled()` returns the value of the DepthTestEnabled field of the referenced object
    inline bool DepthTestEnabled() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->DepthTestEnabled(m_id);
    }

    inline bool IsDepthTestEnabledSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsDepthTestEnabledSet(m_id);
    }

    inline const char* GetDepthTestEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthTestEnabledName();
    }

    inline const char* GetDepthTestEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthTestEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD DepthWriteEnabled: Whether depth writes are enabled. Depth writes are always
    // disabled when DepthTestEnable is false.

    // `DepthWriteEnabled()` returns the value of the DepthWriteEnabled field of the referenced
    // object
    inline bool DepthWriteEnabled() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->DepthWriteEnabled(m_id);
    }

    inline bool IsDepthWriteEnabledSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsDepthWriteEnabledSet(m_id);
    }

    inline const char* GetDepthWriteEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthWriteEnabledName();
    }

    inline const char* GetDepthWriteEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthWriteEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD DepthCompareOp: Comparison operator used for the depth test

    // `DepthCompareOp()` returns the value of the DepthCompareOp field of the referenced object
    inline VkCompareOp DepthCompareOp() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->DepthCompareOp(m_id);
    }

    inline bool IsDepthCompareOpSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsDepthCompareOpSet(m_id);
    }

    inline const char* GetDepthCompareOpName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthCompareOpName();
    }

    inline const char* GetDepthCompareOpDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthCompareOpDescription();
    }

    //-----------------------------------------------
    // REF FIELD DepthBoundsTestEnabled: Whether depth bounds testing is enabled

    // `DepthBoundsTestEnabled()` returns the value of the DepthBoundsTestEnabled field of the
    // referenced object
    inline bool DepthBoundsTestEnabled() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->DepthBoundsTestEnabled(m_id);
    }

    inline bool IsDepthBoundsTestEnabledSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsDepthBoundsTestEnabledSet(m_id);
    }

    inline const char* GetDepthBoundsTestEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthBoundsTestEnabledName();
    }

    inline const char* GetDepthBoundsTestEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDepthBoundsTestEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD MinDepthBounds: Minimum depth bound used in the depth bounds test

    // `MinDepthBounds()` returns the value of the MinDepthBounds field of the referenced object
    inline float MinDepthBounds() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->MinDepthBounds(m_id);
    }

    inline bool IsMinDepthBoundsSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsMinDepthBoundsSet(m_id);
    }

    inline const char* GetMinDepthBoundsName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetMinDepthBoundsName();
    }

    inline const char* GetMinDepthBoundsDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetMinDepthBoundsDescription();
    }

    //-----------------------------------------------
    // REF FIELD MaxDepthBounds: Maximum depth bound used in the depth bounds test

    // `MaxDepthBounds()` returns the value of the MaxDepthBounds field of the referenced object
    inline float MaxDepthBounds() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->MaxDepthBounds(m_id);
    }

    inline bool IsMaxDepthBoundsSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsMaxDepthBoundsSet(m_id);
    }

    inline const char* GetMaxDepthBoundsName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetMaxDepthBoundsName();
    }

    inline const char* GetMaxDepthBoundsDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetMaxDepthBoundsDescription();
    }

    //-----------------------------------------------
    // REF FIELD StencilTestEnabled: Whether stencil testing is enabled

    // `StencilTestEnabled()` returns the value of the StencilTestEnabled field of the referenced
    // object
    inline bool StencilTestEnabled() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->StencilTestEnabled(m_id);
    }

    inline bool IsStencilTestEnabledSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsStencilTestEnabledSet(m_id);
    }

    inline const char* GetStencilTestEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetStencilTestEnabledName();
    }

    inline const char* GetStencilTestEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetStencilTestEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD StencilOpStateFront: Front parameter of the stencil test

    // `StencilOpStateFront()` returns the value of the StencilOpStateFront field of the referenced
    // object
    inline VkStencilOpState StencilOpStateFront() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->StencilOpStateFront(m_id);
    }

    inline bool IsStencilOpStateFrontSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsStencilOpStateFrontSet(m_id);
    }

    inline const char* GetStencilOpStateFrontName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetStencilOpStateFrontName();
    }

    inline const char* GetStencilOpStateFrontDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetStencilOpStateFrontDescription();
    }

    //-----------------------------------------------
    // REF FIELD StencilOpStateBack: Back parameter of the stencil test

    // `StencilOpStateBack()` returns the value of the StencilOpStateBack field of the referenced
    // object
    inline VkStencilOpState StencilOpStateBack() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->StencilOpStateBack(m_id);
    }

    inline bool IsStencilOpStateBackSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsStencilOpStateBackSet(m_id);
    }

    inline const char* GetStencilOpStateBackName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetStencilOpStateBackName();
    }

    inline const char* GetStencilOpStateBackDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetStencilOpStateBackDescription();
    }

    //-----------------------------------------------
    // REF FIELD LogicOpEnabled: Whether to apply Logical Operations

    // `LogicOpEnabled(uint32_t attachment)` returns the value of the LogicOpEnabled field of the
    // referenced object
    inline bool LogicOpEnabled(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->LogicOpEnabled(m_id, attachment);
    }

    // `LogicOpEnabled()` returns the array of values of the LogicOpEnabled field of the referenced
    // object
    inline LogicOpEnabledConstArray LogicOpEnabled() const
    {
        return LogicOpEnabledConstArray(m_obj_ptr, m_id);
    }

    inline bool IsLogicOpEnabledSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsLogicOpEnabledSet(m_id, attachment);
    }

    inline const char* GetLogicOpEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLogicOpEnabledName();
    }

    inline const char* GetLogicOpEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLogicOpEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD LogicOp: Which logical operation to apply

    // `LogicOp(uint32_t attachment)` returns the value of the LogicOp field of the referenced
    // object
    inline VkLogicOp LogicOp(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->LogicOp(m_id, attachment);
    }

    // `LogicOp()` returns the array of values of the LogicOp field of the referenced object
    inline LogicOpConstArray LogicOp() const { return LogicOpConstArray(m_obj_ptr, m_id); }

    inline bool IsLogicOpSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsLogicOpSet(m_id, attachment);
    }

    inline const char* GetLogicOpName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLogicOpName();
    }

    inline const char* GetLogicOpDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLogicOpDescription();
    }

    //-----------------------------------------------
    // REF FIELD Attachment: Per target attachment color blend states

    // `Attachment(uint32_t attachment)` returns the value of the Attachment field of the referenced
    // object
    inline VkPipelineColorBlendAttachmentState Attachment(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->Attachment(m_id, attachment);
    }

    // `Attachment()` returns the array of values of the Attachment field of the referenced object
    inline AttachmentConstArray Attachment() const { return AttachmentConstArray(m_obj_ptr, m_id); }

    inline bool IsAttachmentSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsAttachmentSet(m_id, attachment);
    }

    inline const char* GetAttachmentName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetAttachmentName();
    }

    inline const char* GetAttachmentDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetAttachmentDescription();
    }

    //-----------------------------------------------
    // REF FIELD BlendConstant: A color constant used for blending

    // `BlendConstant(uint32_t channel)` returns the value of the BlendConstant field of the
    // referenced object
    inline float BlendConstant(uint32_t channel) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->BlendConstant(m_id, channel);
    }

    // `BlendConstant()` returns the array of values of the BlendConstant field of the referenced
    // object
    inline BlendConstantConstArray BlendConstant() const
    {
        return BlendConstantConstArray(m_obj_ptr, m_id);
    }

    inline bool IsBlendConstantSet(uint32_t channel) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsBlendConstantSet(m_id, channel);
    }

    inline const char* GetBlendConstantName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetBlendConstantName();
    }

    inline const char* GetBlendConstantDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetBlendConstantDescription();
    }

    //-----------------------------------------------
    // REF FIELD LRZEnabled: Whether LRZ is enabled for depth

    // `LRZEnabled()` returns the value of the LRZEnabled field of the referenced object
    inline bool LRZEnabled() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->LRZEnabled(m_id);
    }

    inline bool IsLRZEnabledSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsLRZEnabledSet(m_id);
    }

    inline const char* GetLRZEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLRZEnabledName();
    }

    inline const char* GetLRZEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLRZEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD LRZWrite: Whether LRZ write is enabled

    // `LRZWrite()` returns the value of the LRZWrite field of the referenced object
    inline bool LRZWrite() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->LRZWrite(m_id);
    }

    inline bool IsLRZWriteSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsLRZWriteSet(m_id);
    }

    inline const char* GetLRZWriteName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLRZWriteName();
    }

    inline const char* GetLRZWriteDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLRZWriteDescription();
    }

    //-----------------------------------------------
    // REF FIELD LRZDirStatus: LRZ direction

    // `LRZDirStatus()` returns the value of the LRZDirStatus field of the referenced object
    inline a6xx_lrz_dir_status LRZDirStatus() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->LRZDirStatus(m_id);
    }

    inline bool IsLRZDirStatusSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsLRZDirStatusSet(m_id);
    }

    inline const char* GetLRZDirStatusName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLRZDirStatusName();
    }

    inline const char* GetLRZDirStatusDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLRZDirStatusDescription();
    }

    //-----------------------------------------------
    // REF FIELD LRZDirWrite: Whether LRZ direction write is enabled

    // `LRZDirWrite()` returns the value of the LRZDirWrite field of the referenced object
    inline bool LRZDirWrite() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->LRZDirWrite(m_id);
    }

    inline bool IsLRZDirWriteSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsLRZDirWriteSet(m_id);
    }

    inline const char* GetLRZDirWriteName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLRZDirWriteName();
    }

    inline const char* GetLRZDirWriteDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetLRZDirWriteDescription();
    }

    //-----------------------------------------------
    // REF FIELD ZTestMode: Depth test mode

    // `ZTestMode()` returns the value of the ZTestMode field of the referenced object
    inline a6xx_ztest_mode ZTestMode() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->ZTestMode(m_id);
    }

    inline bool IsZTestModeSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsZTestModeSet(m_id);
    }

    inline const char* GetZTestModeName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetZTestModeName();
    }

    inline const char* GetZTestModeDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetZTestModeDescription();
    }

    //-----------------------------------------------
    // REF FIELD BinW: Bin width

    // `BinW()` returns the value of the BinW field of the referenced object
    inline uint32_t BinW() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->BinW(m_id);
    }

    inline bool IsBinWSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsBinWSet(m_id);
    }

    inline const char* GetBinWName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetBinWName();
    }

    inline const char* GetBinWDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetBinWDescription();
    }

    //-----------------------------------------------
    // REF FIELD BinH: Bin Height

    // `BinH()` returns the value of the BinH field of the referenced object
    inline uint32_t BinH() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->BinH(m_id);
    }

    inline bool IsBinHSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsBinHSet(m_id);
    }

    inline const char* GetBinHName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetBinHName();
    }

    inline const char* GetBinHDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetBinHDescription();
    }

    //-----------------------------------------------
    // REF FIELD WindowScissorTLX: Window scissor Top Left X-coordinate

    // `WindowScissorTLX()` returns the value of the WindowScissorTLX field of the referenced object
    inline uint16_t WindowScissorTLX() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->WindowScissorTLX(m_id);
    }

    inline bool IsWindowScissorTLXSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsWindowScissorTLXSet(m_id);
    }

    inline const char* GetWindowScissorTLXName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetWindowScissorTLXName();
    }

    inline const char* GetWindowScissorTLXDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetWindowScissorTLXDescription();
    }

    //-----------------------------------------------
    // REF FIELD WindowScissorTLY: Window scissor Top Left Y-coordinate

    // `WindowScissorTLY()` returns the value of the WindowScissorTLY field of the referenced object
    inline uint16_t WindowScissorTLY() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->WindowScissorTLY(m_id);
    }

    inline bool IsWindowScissorTLYSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsWindowScissorTLYSet(m_id);
    }

    inline const char* GetWindowScissorTLYName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetWindowScissorTLYName();
    }

    inline const char* GetWindowScissorTLYDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetWindowScissorTLYDescription();
    }

    //-----------------------------------------------
    // REF FIELD WindowScissorBRX: Window scissor Bottom Right X-coordinate

    // `WindowScissorBRX()` returns the value of the WindowScissorBRX field of the referenced object
    inline uint16_t WindowScissorBRX() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->WindowScissorBRX(m_id);
    }

    inline bool IsWindowScissorBRXSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsWindowScissorBRXSet(m_id);
    }

    inline const char* GetWindowScissorBRXName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetWindowScissorBRXName();
    }

    inline const char* GetWindowScissorBRXDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetWindowScissorBRXDescription();
    }

    //-----------------------------------------------
    // REF FIELD WindowScissorBRY: Window scissor Bottom Right Y-coordinate

    // `WindowScissorBRY()` returns the value of the WindowScissorBRY field of the referenced object
    inline uint16_t WindowScissorBRY() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->WindowScissorBRY(m_id);
    }

    inline bool IsWindowScissorBRYSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsWindowScissorBRYSet(m_id);
    }

    inline const char* GetWindowScissorBRYName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetWindowScissorBRYName();
    }

    inline const char* GetWindowScissorBRYDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetWindowScissorBRYDescription();
    }

    //-----------------------------------------------
    // REF FIELD RenderMode: Whether in binning pass or rendering pass

    // `RenderMode()` returns the value of the RenderMode field of the referenced object
    inline a6xx_render_mode RenderMode() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->RenderMode(m_id);
    }

    inline bool IsRenderModeSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsRenderModeSet(m_id);
    }

    inline const char* GetRenderModeName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetRenderModeName();
    }

    inline const char* GetRenderModeDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetRenderModeDescription();
    }

    //-----------------------------------------------
    // REF FIELD BuffersLocation: Whether the target buffer is in GMEM or SYSMEM

    // `BuffersLocation()` returns the value of the BuffersLocation field of the referenced object
    inline a6xx_buffers_location BuffersLocation() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->BuffersLocation(m_id);
    }

    inline bool IsBuffersLocationSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsBuffersLocationSet(m_id);
    }

    inline const char* GetBuffersLocationName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetBuffersLocationName();
    }

    inline const char* GetBuffersLocationDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetBuffersLocationDescription();
    }

    //-----------------------------------------------
    // REF FIELD ThreadSize: Whether the thread size is 64 or 128

    // `ThreadSize()` returns the value of the ThreadSize field of the referenced object
    inline a6xx_threadsize ThreadSize() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->ThreadSize(m_id);
    }

    inline bool IsThreadSizeSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsThreadSizeSet(m_id);
    }

    inline const char* GetThreadSizeName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetThreadSizeName();
    }

    inline const char* GetThreadSizeDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetThreadSizeDescription();
    }

    //-----------------------------------------------
    // REF FIELD EnableAllHelperLanes: Whether all helper lanes are enabled of the 2x2 quad for fine
    // derivatives

    // `EnableAllHelperLanes()` returns the value of the EnableAllHelperLanes field of the
    // referenced object
    inline bool EnableAllHelperLanes() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->EnableAllHelperLanes(m_id);
    }

    inline bool IsEnableAllHelperLanesSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsEnableAllHelperLanesSet(m_id);
    }

    inline const char* GetEnableAllHelperLanesName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetEnableAllHelperLanesName();
    }

    inline const char* GetEnableAllHelperLanesDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetEnableAllHelperLanesDescription();
    }

    //-----------------------------------------------
    // REF FIELD EnablePartialHelperLanes: Whether 3 out of 4 helper lanes are enabled of the 2x2
    // quad for coarse derivatives

    // `EnablePartialHelperLanes()` returns the value of the EnablePartialHelperLanes field of the
    // referenced object
    inline bool EnablePartialHelperLanes() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->EnablePartialHelperLanes(m_id);
    }

    inline bool IsEnablePartialHelperLanesSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsEnablePartialHelperLanesSet(m_id);
    }

    inline const char* GetEnablePartialHelperLanesName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetEnablePartialHelperLanesName();
    }

    inline const char* GetEnablePartialHelperLanesDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetEnablePartialHelperLanesDescription();
    }

    //-----------------------------------------------
    // REF FIELD UBWCEnabled: Whether UBWC is enabled for this attachment

    // `UBWCEnabled(uint32_t attachment)` returns the value of the UBWCEnabled field of the
    // referenced object
    inline bool UBWCEnabled(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->UBWCEnabled(m_id, attachment);
    }

    // `UBWCEnabled()` returns the array of values of the UBWCEnabled field of the referenced object
    inline UBWCEnabledConstArray UBWCEnabled() const
    {
        return UBWCEnabledConstArray(m_obj_ptr, m_id);
    }

    inline bool IsUBWCEnabledSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsUBWCEnabledSet(m_id, attachment);
    }

    inline const char* GetUBWCEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetUBWCEnabledName();
    }

    inline const char* GetUBWCEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetUBWCEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD UBWCLosslessEnabled: Whether UBWC Lossless compression (A7XX+) is enabled for this
    // attachment

    // `UBWCLosslessEnabled(uint32_t attachment)` returns the value of the UBWCLosslessEnabled field
    // of the referenced object
    inline bool UBWCLosslessEnabled(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->UBWCLosslessEnabled(m_id, attachment);
    }

    // `UBWCLosslessEnabled()` returns the array of values of the UBWCLosslessEnabled field of the
    // referenced object
    inline UBWCLosslessEnabledConstArray UBWCLosslessEnabled() const
    {
        return UBWCLosslessEnabledConstArray(m_obj_ptr, m_id);
    }

    inline bool IsUBWCLosslessEnabledSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsUBWCLosslessEnabledSet(m_id, attachment);
    }

    inline const char* GetUBWCLosslessEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetUBWCLosslessEnabledName();
    }

    inline const char* GetUBWCLosslessEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetUBWCLosslessEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD UBWCEnabledOnDS: Whether UBWC is enabled for this depth stencil attachment

    // `UBWCEnabledOnDS()` returns the value of the UBWCEnabledOnDS field of the referenced object
    inline bool UBWCEnabledOnDS() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->UBWCEnabledOnDS(m_id);
    }

    inline bool IsUBWCEnabledOnDSSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsUBWCEnabledOnDSSet(m_id);
    }

    inline const char* GetUBWCEnabledOnDSName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetUBWCEnabledOnDSName();
    }

    inline const char* GetUBWCEnabledOnDSDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetUBWCEnabledOnDSDescription();
    }

    //-----------------------------------------------
    // REF FIELD UBWCLosslessEnabledOnDS: Whether UBWC Lossless compression (A7XX+) is enabled for
    // this depth stencil attachment

    // `UBWCLosslessEnabledOnDS()` returns the value of the UBWCLosslessEnabledOnDS field of the
    // referenced object
    inline bool UBWCLosslessEnabledOnDS() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->UBWCLosslessEnabledOnDS(m_id);
    }

    inline bool IsUBWCLosslessEnabledOnDSSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsUBWCLosslessEnabledOnDSSet(m_id);
    }

    inline const char* GetUBWCLosslessEnabledOnDSName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetUBWCLosslessEnabledOnDSName();
    }

    inline const char* GetUBWCLosslessEnabledOnDSDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetUBWCLosslessEnabledOnDSDescription();
    }

    EventStateInfoConstRefT& operator=(const EventStateInfoConstRefT& other) = delete;

protected:
    template<typename CONFIG_> friend class EventStateInfoT;
    template<typename Class, typename Id, typename RefT> friend class StructOfArraysIterator;
    const SOA* m_obj_ptr = nullptr;
    Id         m_id;
};

//--------------------------------------------------------------------------------------------------
// State info for events (draw/dispatch/sync/dma)
template<typename CONFIG> class EventStateInfoT
{
public:
    using Id = typename CONFIG::Id;
    using SOA = typename CONFIG::SOA;
    using Ref = typename CONFIG::Ref;
    using ConstRef = typename CONFIG::ConstRef;
    using Iterator = typename CONFIG::Iterator;
    using ConstIterator = typename CONFIG::ConstIterator;
    using ViewportArray = typename CONFIG::ViewportArray;
    using ViewportConstArray = typename CONFIG::ViewportConstArray;
    using ScissorArray = typename CONFIG::ScissorArray;
    using ScissorConstArray = typename CONFIG::ScissorConstArray;
    using LogicOpEnabledArray = typename CONFIG::LogicOpEnabledArray;
    using LogicOpEnabledConstArray = typename CONFIG::LogicOpEnabledConstArray;
    using LogicOpArray = typename CONFIG::LogicOpArray;
    using LogicOpConstArray = typename CONFIG::LogicOpConstArray;
    using AttachmentArray = typename CONFIG::AttachmentArray;
    using AttachmentConstArray = typename CONFIG::AttachmentConstArray;
    using BlendConstantArray = typename CONFIG::BlendConstantArray;
    using BlendConstantConstArray = typename CONFIG::BlendConstantConstArray;
    using UBWCEnabledArray = typename CONFIG::UBWCEnabledArray;
    using UBWCEnabledConstArray = typename CONFIG::UBWCEnabledConstArray;
    using UBWCLosslessEnabledArray = typename CONFIG::UBWCLosslessEnabledArray;
    using UBWCLosslessEnabledConstArray = typename CONFIG::UBWCLosslessEnabledConstArray;

    // `size()` returns the number of elements
    inline typename Id::basic_type size() const { return m_size; }

    inline bool empty() const { return m_size == 0; }

    // `capacity()` returns the maximum number of elements before re-allocating
    inline typename Id::basic_type capacity() const { return m_cap; }

    // `IsValidId` reports whether `id` identifies a valid element
    inline bool IsValidId(Id id) const { return static_cast<typename Id::basic_type>(id) < size(); }

    // 'MarkFieldSet()' marks whether a particular field was set with a value
    inline void MarkFieldSet(Id id, uint32_t field_index)
    {
        uint32_t bit = static_cast<typename Id::basic_type>(id) * kNumFields + field_index;
        m_is_set_buffer[bit / 8] |= (1 << (bit % 8));
    }

    // 'IsFieldSet()' indicates whether a given field was set
    inline bool IsFieldSet(Id id, uint32_t field_index) const
    {
        uint32_t bit = static_cast<typename Id::basic_type>(id) * kNumFields + field_index;
        return (m_is_set_buffer[bit / 8] & (1 << (bit % 8))) != 0;
    }

    //-----------------------------------------------
    // FIELD Topology: The primitive topology for this event

    // `TopologyPtr()` returns a shared pointer to an array of `size()` elements
    inline const uint32_t* TopologyPtr() const
    {
        return reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kTopologyOffset * m_cap);
    }
    inline uint32_t* TopologyPtr()
    {
        return reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kTopologyOffset * m_cap);
    }
    // `TopologyPtr()` returns a shared pointer to an array of `size()` elements
    inline const uint32_t* TopologyPtr(Id id) const
    {
        return reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kTopologyOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline uint32_t* TopologyPtr(Id id)
    {
        return reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kTopologyOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `Topology(id)` retuns the `Topology` element of the object identified by `id`
    inline VkPrimitiveTopology Topology(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return static_cast<VkPrimitiveTopology>(*TopologyPtr(id));
    }

    // `SetTopology(id,value)` sets the `Topology` element of the object identified by `id`
    inline SOA& SetTopology(Id id, VkPrimitiveTopology value)
    {
        DIVE_ASSERT(IsValidId(id));
        *TopologyPtr(id) = static_cast<uint32_t>(value);
        MarkFieldSet(id, kTopologyIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsTopologySet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kTopologyIndex);
    }

    inline const char* GetTopologyName() const { return "Topology"; }

    inline const char* GetTopologyDescription() const
    {
        return "The primitive topology for this event";
    }

    //-----------------------------------------------
    // FIELD PrimRestartEnabled: Controls whether a special vertex index value is treated as
    // restarting the assembly of primitives

    // `PrimRestartEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* PrimRestartEnabledPtr() const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kPrimRestartEnabledOffset * m_cap);
    }
    inline bool* PrimRestartEnabledPtr()
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kPrimRestartEnabledOffset * m_cap);
    }
    // `PrimRestartEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* PrimRestartEnabledPtr(Id id) const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kPrimRestartEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline bool* PrimRestartEnabledPtr(Id id)
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kPrimRestartEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `PrimRestartEnabled(id)` retuns the `PrimRestartEnabled` element of the object identified by
    // `id`
    inline bool PrimRestartEnabled(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *PrimRestartEnabledPtr(id);
    }

    // `SetPrimRestartEnabled(id,value)` sets the `PrimRestartEnabled` element of the object
    // identified by `id`
    inline SOA& SetPrimRestartEnabled(Id id, bool value)
    {
        DIVE_ASSERT(IsValidId(id));
        *PrimRestartEnabledPtr(id) = value;
        MarkFieldSet(id, kPrimRestartEnabledIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsPrimRestartEnabledSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kPrimRestartEnabledIndex);
    }

    inline const char* GetPrimRestartEnabledName() const { return "PrimRestartEnabled"; }

    inline const char* GetPrimRestartEnabledDescription() const
    {
        return "Controls whether a special vertex index value is treated as restarting the "
               "assembly of primitives";
    }

    //-----------------------------------------------
    // FIELD PatchControlPoints: Number of control points per patch

    // `PatchControlPointsPtr()` returns a shared pointer to an array of `size()` elements
    inline const uint32_t* PatchControlPointsPtr() const
    {
        return reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kPatchControlPointsOffset * m_cap);
    }
    inline uint32_t* PatchControlPointsPtr()
    {
        return reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kPatchControlPointsOffset * m_cap);
    }
    // `PatchControlPointsPtr()` returns a shared pointer to an array of `size()` elements
    inline const uint32_t* PatchControlPointsPtr(Id id) const
    {
        return reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kPatchControlPointsOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline uint32_t* PatchControlPointsPtr(Id id)
    {
        return reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kPatchControlPointsOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `PatchControlPoints(id)` retuns the `PatchControlPoints` element of the object identified by
    // `id`
    inline uint32_t PatchControlPoints(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *PatchControlPointsPtr(id);
    }

    // `SetPatchControlPoints(id,value)` sets the `PatchControlPoints` element of the object
    // identified by `id`
    inline SOA& SetPatchControlPoints(Id id, uint32_t value)
    {
        DIVE_ASSERT(IsValidId(id));
        *PatchControlPointsPtr(id) = value;
        MarkFieldSet(id, kPatchControlPointsIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsPatchControlPointsSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kPatchControlPointsIndex);
    }

    inline const char* GetPatchControlPointsName() const { return "PatchControlPoints"; }

    inline const char* GetPatchControlPointsDescription() const
    {
        return "Number of control points per patch";
    }

    //-----------------------------------------------
    // FIELD Viewport: Defines the viewport transforms

    // `ViewportPtr()` returns a shared pointer to an array of `size()` elements
    inline const VkViewport* ViewportPtr() const
    {
        return reinterpret_cast<VkViewport*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                             kViewportOffset * m_cap);
    }
    inline VkViewport* ViewportPtr()
    {
        return reinterpret_cast<VkViewport*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                             kViewportOffset * m_cap);
    }
    // `ViewportPtr()` returns a shared pointer to an array of `size()` elements
    inline const VkViewport* ViewportPtr(Id id, uint32_t viewport = 0) const
    {
        return reinterpret_cast<VkViewport*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                             kViewportOffset * m_cap) +
               static_cast<typename Id::basic_type>(id) * 16 + viewport

        ;
    }
    inline VkViewport* ViewportPtr(Id id, uint32_t viewport = 0)
    {
        return reinterpret_cast<VkViewport*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                             kViewportOffset * m_cap) +
               static_cast<typename Id::basic_type>(id) * 16 + viewport

        ;
    }
    // `Viewport(id)` retuns the `Viewport` element of the object identified by `id`
    inline VkViewport Viewport(Id id, uint32_t viewport) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *ViewportPtr(id, viewport);
    }

    // `Viewport(id)` returns the array of values of the Viewport field of the object identified by
    // `id`
    inline ViewportArray      Viewport(Id id) { return ViewportArray(static_cast<SOA*>(this), id); }
    inline ViewportConstArray Viewport(Id id) const
    {
        return ViewportConstArray(static_cast<const SOA*>(this), id);
    }

    // `SetViewport(id,value)` sets the `Viewport` element of the object identified by `id`
    inline SOA& SetViewport(Id id, uint32_t viewport, VkViewport value)
    {
        DIVE_ASSERT(IsValidId(id));
        *ViewportPtr(id, viewport) = value;
        MarkFieldSet(id, kViewportIndex + viewport);
        return static_cast<SOA&>(*this);
    }

    inline bool IsViewportSet(Id id, uint32_t viewport) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kViewportIndex + viewport);
    }

    inline const char* GetViewportName() const { return "Viewport"; }

    inline const char* GetViewportDescription() const { return "Defines the viewport transforms"; }

    //-----------------------------------------------
    // FIELD Scissor: Defines the rectangular bounds of the scissor for the corresponding viewport

    // `ScissorPtr()` returns a shared pointer to an array of `size()` elements
    inline const VkRect2D* ScissorPtr() const
    {
        return reinterpret_cast<VkRect2D*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kScissorOffset * m_cap);
    }
    inline VkRect2D* ScissorPtr()
    {
        return reinterpret_cast<VkRect2D*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kScissorOffset * m_cap);
    }
    // `ScissorPtr()` returns a shared pointer to an array of `size()` elements
    inline const VkRect2D* ScissorPtr(Id id, uint32_t scissor = 0) const
    {
        return reinterpret_cast<VkRect2D*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kScissorOffset * m_cap) +
               static_cast<typename Id::basic_type>(id) * 16 + scissor

        ;
    }
    inline VkRect2D* ScissorPtr(Id id, uint32_t scissor = 0)
    {
        return reinterpret_cast<VkRect2D*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kScissorOffset * m_cap) +
               static_cast<typename Id::basic_type>(id) * 16 + scissor

        ;
    }
    // `Scissor(id)` retuns the `Scissor` element of the object identified by `id`
    inline VkRect2D Scissor(Id id, uint32_t scissor) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *ScissorPtr(id, scissor);
    }

    // `Scissor(id)` returns the array of values of the Scissor field of the object identified by
    // `id`
    inline ScissorArray      Scissor(Id id) { return ScissorArray(static_cast<SOA*>(this), id); }
    inline ScissorConstArray Scissor(Id id) const
    {
        return ScissorConstArray(static_cast<const SOA*>(this), id);
    }

    // `SetScissor(id,value)` sets the `Scissor` element of the object identified by `id`
    inline SOA& SetScissor(Id id, uint32_t scissor, VkRect2D value)
    {
        DIVE_ASSERT(IsValidId(id));
        *ScissorPtr(id, scissor) = value;
        MarkFieldSet(id, kScissorIndex + scissor);
        return static_cast<SOA&>(*this);
    }

    inline bool IsScissorSet(Id id, uint32_t scissor) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kScissorIndex + scissor);
    }

    inline const char* GetScissorName() const { return "Scissor"; }

    inline const char* GetScissorDescription() const
    {
        return "Defines the rectangular bounds of the scissor for the corresponding viewport";
    }

    //-----------------------------------------------
    // FIELD DepthClampEnabled: Controls whether to clamp the fragment’s depth values

    // `DepthClampEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* DepthClampEnabledPtr() const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kDepthClampEnabledOffset * m_cap);
    }
    inline bool* DepthClampEnabledPtr()
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kDepthClampEnabledOffset * m_cap);
    }
    // `DepthClampEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* DepthClampEnabledPtr(Id id) const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kDepthClampEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline bool* DepthClampEnabledPtr(Id id)
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kDepthClampEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `DepthClampEnabled(id)` retuns the `DepthClampEnabled` element of the object identified by
    // `id`
    inline bool DepthClampEnabled(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *DepthClampEnabledPtr(id);
    }

    // `SetDepthClampEnabled(id,value)` sets the `DepthClampEnabled` element of the object
    // identified by `id`
    inline SOA& SetDepthClampEnabled(Id id, bool value)
    {
        DIVE_ASSERT(IsValidId(id));
        *DepthClampEnabledPtr(id) = value;
        MarkFieldSet(id, kDepthClampEnabledIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsDepthClampEnabledSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kDepthClampEnabledIndex);
    }

    inline const char* GetDepthClampEnabledName() const { return "DepthClampEnabled"; }

    inline const char* GetDepthClampEnabledDescription() const
    {
        return "Controls whether to clamp the fragment’s depth values";
    }

    //-----------------------------------------------
    // FIELD RasterizerDiscardEnabled: Controls whether primitives are discarded immediately before
    // the rasterization stage

    // `RasterizerDiscardEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* RasterizerDiscardEnabledPtr() const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kRasterizerDiscardEnabledOffset * m_cap);
    }
    inline bool* RasterizerDiscardEnabledPtr()
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kRasterizerDiscardEnabledOffset * m_cap);
    }
    // `RasterizerDiscardEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* RasterizerDiscardEnabledPtr(Id id) const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kRasterizerDiscardEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline bool* RasterizerDiscardEnabledPtr(Id id)
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kRasterizerDiscardEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `RasterizerDiscardEnabled(id)` retuns the `RasterizerDiscardEnabled` element of the object
    // identified by `id`
    inline bool RasterizerDiscardEnabled(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *RasterizerDiscardEnabledPtr(id);
    }

    // `SetRasterizerDiscardEnabled(id,value)` sets the `RasterizerDiscardEnabled` element of the
    // object identified by `id`
    inline SOA& SetRasterizerDiscardEnabled(Id id, bool value)
    {
        DIVE_ASSERT(IsValidId(id));
        *RasterizerDiscardEnabledPtr(id) = value;
        MarkFieldSet(id, kRasterizerDiscardEnabledIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsRasterizerDiscardEnabledSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kRasterizerDiscardEnabledIndex);
    }

    inline const char* GetRasterizerDiscardEnabledName() const
    {
        return "RasterizerDiscardEnabled";
    }

    inline const char* GetRasterizerDiscardEnabledDescription() const
    {
        return "Controls whether primitives are discarded immediately before the rasterization "
               "stage";
    }

    //-----------------------------------------------
    // FIELD PolygonMode: The triangle rendering mode

    // `PolygonModePtr()` returns a shared pointer to an array of `size()` elements
    inline const VkPolygonMode* PolygonModePtr() const
    {
        return reinterpret_cast<VkPolygonMode*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                kPolygonModeOffset * m_cap);
    }
    inline VkPolygonMode* PolygonModePtr()
    {
        return reinterpret_cast<VkPolygonMode*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                kPolygonModeOffset * m_cap);
    }
    // `PolygonModePtr()` returns a shared pointer to an array of `size()` elements
    inline const VkPolygonMode* PolygonModePtr(Id id) const
    {
        return reinterpret_cast<VkPolygonMode*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                kPolygonModeOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline VkPolygonMode* PolygonModePtr(Id id)
    {
        return reinterpret_cast<VkPolygonMode*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                kPolygonModeOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `PolygonMode(id)` retuns the `PolygonMode` element of the object identified by `id`
    inline VkPolygonMode PolygonMode(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *PolygonModePtr(id);
    }

    // `SetPolygonMode(id,value)` sets the `PolygonMode` element of the object identified by `id`
    inline SOA& SetPolygonMode(Id id, VkPolygonMode value)
    {
        DIVE_ASSERT(IsValidId(id));
        *PolygonModePtr(id) = value;
        MarkFieldSet(id, kPolygonModeIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsPolygonModeSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kPolygonModeIndex);
    }

    inline const char* GetPolygonModeName() const { return "PolygonMode"; }

    inline const char* GetPolygonModeDescription() const { return "The triangle rendering mode"; }

    //-----------------------------------------------
    // FIELD CullMode: The triangle facing direction used for primitive culling

    // `CullModePtr()` returns a shared pointer to an array of `size()` elements
    inline const VkCullModeFlags* CullModePtr() const
    {
        return reinterpret_cast<VkCullModeFlags*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                  kCullModeOffset * m_cap);
    }
    inline VkCullModeFlags* CullModePtr()
    {
        return reinterpret_cast<VkCullModeFlags*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                  kCullModeOffset * m_cap);
    }
    // `CullModePtr()` returns a shared pointer to an array of `size()` elements
    inline const VkCullModeFlags* CullModePtr(Id id) const
    {
        return reinterpret_cast<VkCullModeFlags*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                  kCullModeOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline VkCullModeFlags* CullModePtr(Id id)
    {
        return reinterpret_cast<VkCullModeFlags*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                  kCullModeOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `CullMode(id)` retuns the `CullMode` element of the object identified by `id`
    inline VkCullModeFlags CullMode(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *CullModePtr(id);
    }

    // `SetCullMode(id,value)` sets the `CullMode` element of the object identified by `id`
    inline SOA& SetCullMode(Id id, VkCullModeFlags value)
    {
        DIVE_ASSERT(IsValidId(id));
        *CullModePtr(id) = value;
        MarkFieldSet(id, kCullModeIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsCullModeSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kCullModeIndex);
    }

    inline const char* GetCullModeName() const { return "CullMode"; }

    inline const char* GetCullModeDescription() const
    {
        return "The triangle facing direction used for primitive culling";
    }

    //-----------------------------------------------
    // FIELD FrontFace: A VkFrontFace value specifying the front-facing triangle orientation to be
    // used for culling

    // `FrontFacePtr()` returns a shared pointer to an array of `size()` elements
    inline const VkFrontFace* FrontFacePtr() const
    {
        return reinterpret_cast<VkFrontFace*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                              kFrontFaceOffset * m_cap);
    }
    inline VkFrontFace* FrontFacePtr()
    {
        return reinterpret_cast<VkFrontFace*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                              kFrontFaceOffset * m_cap);
    }
    // `FrontFacePtr()` returns a shared pointer to an array of `size()` elements
    inline const VkFrontFace* FrontFacePtr(Id id) const
    {
        return reinterpret_cast<VkFrontFace*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                              kFrontFaceOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline VkFrontFace* FrontFacePtr(Id id)
    {
        return reinterpret_cast<VkFrontFace*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                              kFrontFaceOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `FrontFace(id)` retuns the `FrontFace` element of the object identified by `id`
    inline VkFrontFace FrontFace(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *FrontFacePtr(id);
    }

    // `SetFrontFace(id,value)` sets the `FrontFace` element of the object identified by `id`
    inline SOA& SetFrontFace(Id id, VkFrontFace value)
    {
        DIVE_ASSERT(IsValidId(id));
        *FrontFacePtr(id) = value;
        MarkFieldSet(id, kFrontFaceIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsFrontFaceSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kFrontFaceIndex);
    }

    inline const char* GetFrontFaceName() const { return "FrontFace"; }

    inline const char* GetFrontFaceDescription() const
    {
        return "A VkFrontFace value specifying the front-facing triangle orientation to be used "
               "for culling";
    }

    //-----------------------------------------------
    // FIELD DepthBiasEnabled: Whether to bias fragment depth values

    // `DepthBiasEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* DepthBiasEnabledPtr() const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kDepthBiasEnabledOffset * m_cap);
    }
    inline bool* DepthBiasEnabledPtr()
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kDepthBiasEnabledOffset * m_cap);
    }
    // `DepthBiasEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* DepthBiasEnabledPtr(Id id) const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kDepthBiasEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline bool* DepthBiasEnabledPtr(Id id)
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kDepthBiasEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `DepthBiasEnabled(id)` retuns the `DepthBiasEnabled` element of the object identified by `id`
    inline bool DepthBiasEnabled(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *DepthBiasEnabledPtr(id);
    }

    // `SetDepthBiasEnabled(id,value)` sets the `DepthBiasEnabled` element of the object identified
    // by `id`
    inline SOA& SetDepthBiasEnabled(Id id, bool value)
    {
        DIVE_ASSERT(IsValidId(id));
        *DepthBiasEnabledPtr(id) = value;
        MarkFieldSet(id, kDepthBiasEnabledIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsDepthBiasEnabledSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kDepthBiasEnabledIndex);
    }

    inline const char* GetDepthBiasEnabledName() const { return "DepthBiasEnabled"; }

    inline const char* GetDepthBiasEnabledDescription() const
    {
        return "Whether to bias fragment depth values";
    }

    //-----------------------------------------------
    // FIELD DepthBiasConstantFactor: A scalar factor controlling the constant depth value added to
    // each fragment.

    // `DepthBiasConstantFactorPtr()` returns a shared pointer to an array of `size()` elements
    inline const float* DepthBiasConstantFactorPtr() const
    {
        return reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                        kDepthBiasConstantFactorOffset * m_cap);
    }
    inline float* DepthBiasConstantFactorPtr()
    {
        return reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                        kDepthBiasConstantFactorOffset * m_cap);
    }
    // `DepthBiasConstantFactorPtr()` returns a shared pointer to an array of `size()` elements
    inline const float* DepthBiasConstantFactorPtr(Id id) const
    {
        return reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                        kDepthBiasConstantFactorOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline float* DepthBiasConstantFactorPtr(Id id)
    {
        return reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                        kDepthBiasConstantFactorOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `DepthBiasConstantFactor(id)` retuns the `DepthBiasConstantFactor` element of the object
    // identified by `id`
    inline float DepthBiasConstantFactor(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *DepthBiasConstantFactorPtr(id);
    }

    // `SetDepthBiasConstantFactor(id,value)` sets the `DepthBiasConstantFactor` element of the
    // object identified by `id`
    inline SOA& SetDepthBiasConstantFactor(Id id, float value)
    {
        DIVE_ASSERT(IsValidId(id));
        *DepthBiasConstantFactorPtr(id) = value;
        MarkFieldSet(id, kDepthBiasConstantFactorIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsDepthBiasConstantFactorSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kDepthBiasConstantFactorIndex);
    }

    inline const char* GetDepthBiasConstantFactorName() const { return "DepthBiasConstantFactor"; }

    inline const char* GetDepthBiasConstantFactorDescription() const
    {
        return "A scalar factor controlling the constant depth value added to each fragment.";
    }

    //-----------------------------------------------
    // FIELD DepthBiasClamp: The maximum (or minimum) depth bias of a fragment

    // `DepthBiasClampPtr()` returns a shared pointer to an array of `size()` elements
    inline const float* DepthBiasClampPtr() const
    {
        return reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                        kDepthBiasClampOffset * m_cap);
    }
    inline float* DepthBiasClampPtr()
    {
        return reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                        kDepthBiasClampOffset * m_cap);
    }
    // `DepthBiasClampPtr()` returns a shared pointer to an array of `size()` elements
    inline const float* DepthBiasClampPtr(Id id) const
    {
        return reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                        kDepthBiasClampOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline float* DepthBiasClampPtr(Id id)
    {
        return reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                        kDepthBiasClampOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `DepthBiasClamp(id)` retuns the `DepthBiasClamp` element of the object identified by `id`
    inline float DepthBiasClamp(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *DepthBiasClampPtr(id);
    }

    // `SetDepthBiasClamp(id,value)` sets the `DepthBiasClamp` element of the object identified by
    // `id`
    inline SOA& SetDepthBiasClamp(Id id, float value)
    {
        DIVE_ASSERT(IsValidId(id));
        *DepthBiasClampPtr(id) = value;
        MarkFieldSet(id, kDepthBiasClampIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsDepthBiasClampSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kDepthBiasClampIndex);
    }

    inline const char* GetDepthBiasClampName() const { return "DepthBiasClamp"; }

    inline const char* GetDepthBiasClampDescription() const
    {
        return "The maximum (or minimum) depth bias of a fragment";
    }

    //-----------------------------------------------
    // FIELD DepthBiasSlopeFactor: A scalar factor applied to a fragment’s slope in depth bias
    // calculations

    // `DepthBiasSlopeFactorPtr()` returns a shared pointer to an array of `size()` elements
    inline const float* DepthBiasSlopeFactorPtr() const
    {
        return reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                        kDepthBiasSlopeFactorOffset * m_cap);
    }
    inline float* DepthBiasSlopeFactorPtr()
    {
        return reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                        kDepthBiasSlopeFactorOffset * m_cap);
    }
    // `DepthBiasSlopeFactorPtr()` returns a shared pointer to an array of `size()` elements
    inline const float* DepthBiasSlopeFactorPtr(Id id) const
    {
        return reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                        kDepthBiasSlopeFactorOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline float* DepthBiasSlopeFactorPtr(Id id)
    {
        return reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                        kDepthBiasSlopeFactorOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `DepthBiasSlopeFactor(id)` retuns the `DepthBiasSlopeFactor` element of the object identified
    // by `id`
    inline float DepthBiasSlopeFactor(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *DepthBiasSlopeFactorPtr(id);
    }

    // `SetDepthBiasSlopeFactor(id,value)` sets the `DepthBiasSlopeFactor` element of the object
    // identified by `id`
    inline SOA& SetDepthBiasSlopeFactor(Id id, float value)
    {
        DIVE_ASSERT(IsValidId(id));
        *DepthBiasSlopeFactorPtr(id) = value;
        MarkFieldSet(id, kDepthBiasSlopeFactorIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsDepthBiasSlopeFactorSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kDepthBiasSlopeFactorIndex);
    }

    inline const char* GetDepthBiasSlopeFactorName() const { return "DepthBiasSlopeFactor"; }

    inline const char* GetDepthBiasSlopeFactorDescription() const
    {
        return "A scalar factor applied to a fragment’s slope in depth bias calculations";
    }

    //-----------------------------------------------
    // FIELD LineWidth: The width of rasterized line segments

    // `LineWidthPtr()` returns a shared pointer to an array of `size()` elements
    inline const float* LineWidthPtr() const
    {
        return reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                        kLineWidthOffset * m_cap);
    }
    inline float* LineWidthPtr()
    {
        return reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                        kLineWidthOffset * m_cap);
    }
    // `LineWidthPtr()` returns a shared pointer to an array of `size()` elements
    inline const float* LineWidthPtr(Id id) const
    {
        return reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                        kLineWidthOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline float* LineWidthPtr(Id id)
    {
        return reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                        kLineWidthOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `LineWidth(id)` retuns the `LineWidth` element of the object identified by `id`
    inline float LineWidth(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *LineWidthPtr(id);
    }

    // `SetLineWidth(id,value)` sets the `LineWidth` element of the object identified by `id`
    inline SOA& SetLineWidth(Id id, float value)
    {
        DIVE_ASSERT(IsValidId(id));
        *LineWidthPtr(id) = value;
        MarkFieldSet(id, kLineWidthIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsLineWidthSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kLineWidthIndex);
    }

    inline const char* GetLineWidthName() const { return "LineWidth"; }

    inline const char* GetLineWidthDescription() const
    {
        return "The width of rasterized line segments";
    }

    //-----------------------------------------------
    // FIELD RasterizationSamples: A VkSampleCountFlagBits value specifying the number of samples
    // used in rasterization

    // `RasterizationSamplesPtr()` returns a shared pointer to an array of `size()` elements
    inline const VkSampleCountFlagBits* RasterizationSamplesPtr() const
    {
        return reinterpret_cast<VkSampleCountFlagBits*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                        kRasterizationSamplesOffset * m_cap);
    }
    inline VkSampleCountFlagBits* RasterizationSamplesPtr()
    {
        return reinterpret_cast<VkSampleCountFlagBits*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                        kRasterizationSamplesOffset * m_cap);
    }
    // `RasterizationSamplesPtr()` returns a shared pointer to an array of `size()` elements
    inline const VkSampleCountFlagBits* RasterizationSamplesPtr(Id id) const
    {
        return reinterpret_cast<VkSampleCountFlagBits*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                        kRasterizationSamplesOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline VkSampleCountFlagBits* RasterizationSamplesPtr(Id id)
    {
        return reinterpret_cast<VkSampleCountFlagBits*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                        kRasterizationSamplesOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `RasterizationSamples(id)` retuns the `RasterizationSamples` element of the object identified
    // by `id`
    inline VkSampleCountFlagBits RasterizationSamples(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *RasterizationSamplesPtr(id);
    }

    // `SetRasterizationSamples(id,value)` sets the `RasterizationSamples` element of the object
    // identified by `id`
    inline SOA& SetRasterizationSamples(Id id, VkSampleCountFlagBits value)
    {
        DIVE_ASSERT(IsValidId(id));
        *RasterizationSamplesPtr(id) = value;
        MarkFieldSet(id, kRasterizationSamplesIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsRasterizationSamplesSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kRasterizationSamplesIndex);
    }

    inline const char* GetRasterizationSamplesName() const { return "RasterizationSamples"; }

    inline const char* GetRasterizationSamplesDescription() const
    {
        return "A VkSampleCountFlagBits value specifying the number of samples used in "
               "rasterization";
    }

    //-----------------------------------------------
    // FIELD SampleShadingEnabled: Whether sample shading is enabled

    // `SampleShadingEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* SampleShadingEnabledPtr() const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kSampleShadingEnabledOffset * m_cap);
    }
    inline bool* SampleShadingEnabledPtr()
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kSampleShadingEnabledOffset * m_cap);
    }
    // `SampleShadingEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* SampleShadingEnabledPtr(Id id) const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kSampleShadingEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline bool* SampleShadingEnabledPtr(Id id)
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kSampleShadingEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `SampleShadingEnabled(id)` retuns the `SampleShadingEnabled` element of the object identified
    // by `id`
    inline bool SampleShadingEnabled(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *SampleShadingEnabledPtr(id);
    }

    // `SetSampleShadingEnabled(id,value)` sets the `SampleShadingEnabled` element of the object
    // identified by `id`
    inline SOA& SetSampleShadingEnabled(Id id, bool value)
    {
        DIVE_ASSERT(IsValidId(id));
        *SampleShadingEnabledPtr(id) = value;
        MarkFieldSet(id, kSampleShadingEnabledIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsSampleShadingEnabledSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kSampleShadingEnabledIndex);
    }

    inline const char* GetSampleShadingEnabledName() const { return "SampleShadingEnabled"; }

    inline const char* GetSampleShadingEnabledDescription() const
    {
        return "Whether sample shading is enabled";
    }

    //-----------------------------------------------
    // FIELD MinSampleShading: Specifies a minimum fraction of sample shading if SampleShadingEnable
    // is set to VK_TRUE

    // `MinSampleShadingPtr()` returns a shared pointer to an array of `size()` elements
    inline const float* MinSampleShadingPtr() const
    {
        return reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                        kMinSampleShadingOffset * m_cap);
    }
    inline float* MinSampleShadingPtr()
    {
        return reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                        kMinSampleShadingOffset * m_cap);
    }
    // `MinSampleShadingPtr()` returns a shared pointer to an array of `size()` elements
    inline const float* MinSampleShadingPtr(Id id) const
    {
        return reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                        kMinSampleShadingOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline float* MinSampleShadingPtr(Id id)
    {
        return reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                        kMinSampleShadingOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `MinSampleShading(id)` retuns the `MinSampleShading` element of the object identified by `id`
    inline float MinSampleShading(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *MinSampleShadingPtr(id);
    }

    // `SetMinSampleShading(id,value)` sets the `MinSampleShading` element of the object identified
    // by `id`
    inline SOA& SetMinSampleShading(Id id, float value)
    {
        DIVE_ASSERT(IsValidId(id));
        *MinSampleShadingPtr(id) = value;
        MarkFieldSet(id, kMinSampleShadingIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsMinSampleShadingSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kMinSampleShadingIndex);
    }

    inline const char* GetMinSampleShadingName() const { return "MinSampleShading"; }

    inline const char* GetMinSampleShadingDescription() const
    {
        return "Specifies a minimum fraction of sample shading if SampleShadingEnable is set to "
               "VK_TRUE";
    }

    //-----------------------------------------------
    // FIELD SampleMask: Each bit in the sample mask is associated with a unique sample index as
    // defined for the coverage mask. If the bit is set to 0, the coverage mask bit is set to 0

    // `SampleMaskPtr()` returns a shared pointer to an array of `size()` elements
    inline const VkSampleMask* SampleMaskPtr() const
    {
        return reinterpret_cast<VkSampleMask*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                               kSampleMaskOffset * m_cap);
    }
    inline VkSampleMask* SampleMaskPtr()
    {
        return reinterpret_cast<VkSampleMask*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                               kSampleMaskOffset * m_cap);
    }
    // `SampleMaskPtr()` returns a shared pointer to an array of `size()` elements
    inline const VkSampleMask* SampleMaskPtr(Id id) const
    {
        return reinterpret_cast<VkSampleMask*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                               kSampleMaskOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline VkSampleMask* SampleMaskPtr(Id id)
    {
        return reinterpret_cast<VkSampleMask*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                               kSampleMaskOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `SampleMask(id)` retuns the `SampleMask` element of the object identified by `id`
    inline VkSampleMask SampleMask(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *SampleMaskPtr(id);
    }

    // `SetSampleMask(id,value)` sets the `SampleMask` element of the object identified by `id`
    inline SOA& SetSampleMask(Id id, VkSampleMask value)
    {
        DIVE_ASSERT(IsValidId(id));
        *SampleMaskPtr(id) = value;
        MarkFieldSet(id, kSampleMaskIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsSampleMaskSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kSampleMaskIndex);
    }

    inline const char* GetSampleMaskName() const { return "SampleMask"; }

    inline const char* GetSampleMaskDescription() const
    {
        return "Each bit in the sample mask is associated with a unique sample index as defined "
               "for the coverage mask. If the bit is set to 0, the coverage mask bit is set to 0";
    }

    //-----------------------------------------------
    // FIELD AlphaToCoverageEnabled: Whether a temporary coverage value is generated based on the
    // alpha component of the fragment’s first color output

    // `AlphaToCoverageEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* AlphaToCoverageEnabledPtr() const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kAlphaToCoverageEnabledOffset * m_cap);
    }
    inline bool* AlphaToCoverageEnabledPtr()
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kAlphaToCoverageEnabledOffset * m_cap);
    }
    // `AlphaToCoverageEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* AlphaToCoverageEnabledPtr(Id id) const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kAlphaToCoverageEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline bool* AlphaToCoverageEnabledPtr(Id id)
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kAlphaToCoverageEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `AlphaToCoverageEnabled(id)` retuns the `AlphaToCoverageEnabled` element of the object
    // identified by `id`
    inline bool AlphaToCoverageEnabled(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *AlphaToCoverageEnabledPtr(id);
    }

    // `SetAlphaToCoverageEnabled(id,value)` sets the `AlphaToCoverageEnabled` element of the object
    // identified by `id`
    inline SOA& SetAlphaToCoverageEnabled(Id id, bool value)
    {
        DIVE_ASSERT(IsValidId(id));
        *AlphaToCoverageEnabledPtr(id) = value;
        MarkFieldSet(id, kAlphaToCoverageEnabledIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsAlphaToCoverageEnabledSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kAlphaToCoverageEnabledIndex);
    }

    inline const char* GetAlphaToCoverageEnabledName() const { return "AlphaToCoverageEnabled"; }

    inline const char* GetAlphaToCoverageEnabledDescription() const
    {
        return "Whether a temporary coverage value is generated based on the alpha component of "
               "the fragment’s first color output";
    }

    //-----------------------------------------------
    // FIELD DepthTestEnabled: Whether depth testing is enabled

    // `DepthTestEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* DepthTestEnabledPtr() const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kDepthTestEnabledOffset * m_cap);
    }
    inline bool* DepthTestEnabledPtr()
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kDepthTestEnabledOffset * m_cap);
    }
    // `DepthTestEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* DepthTestEnabledPtr(Id id) const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kDepthTestEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline bool* DepthTestEnabledPtr(Id id)
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kDepthTestEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `DepthTestEnabled(id)` retuns the `DepthTestEnabled` element of the object identified by `id`
    inline bool DepthTestEnabled(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *DepthTestEnabledPtr(id);
    }

    // `SetDepthTestEnabled(id,value)` sets the `DepthTestEnabled` element of the object identified
    // by `id`
    inline SOA& SetDepthTestEnabled(Id id, bool value)
    {
        DIVE_ASSERT(IsValidId(id));
        *DepthTestEnabledPtr(id) = value;
        MarkFieldSet(id, kDepthTestEnabledIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsDepthTestEnabledSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kDepthTestEnabledIndex);
    }

    inline const char* GetDepthTestEnabledName() const { return "DepthTestEnabled"; }

    inline const char* GetDepthTestEnabledDescription() const
    {
        return "Whether depth testing is enabled";
    }

    //-----------------------------------------------
    // FIELD DepthWriteEnabled: Whether depth writes are enabled. Depth writes are always disabled
    // when DepthTestEnable is false.

    // `DepthWriteEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* DepthWriteEnabledPtr() const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kDepthWriteEnabledOffset * m_cap);
    }
    inline bool* DepthWriteEnabledPtr()
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kDepthWriteEnabledOffset * m_cap);
    }
    // `DepthWriteEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* DepthWriteEnabledPtr(Id id) const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kDepthWriteEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline bool* DepthWriteEnabledPtr(Id id)
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kDepthWriteEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `DepthWriteEnabled(id)` retuns the `DepthWriteEnabled` element of the object identified by
    // `id`
    inline bool DepthWriteEnabled(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *DepthWriteEnabledPtr(id);
    }

    // `SetDepthWriteEnabled(id,value)` sets the `DepthWriteEnabled` element of the object
    // identified by `id`
    inline SOA& SetDepthWriteEnabled(Id id, bool value)
    {
        DIVE_ASSERT(IsValidId(id));
        *DepthWriteEnabledPtr(id) = value;
        MarkFieldSet(id, kDepthWriteEnabledIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsDepthWriteEnabledSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kDepthWriteEnabledIndex);
    }

    inline const char* GetDepthWriteEnabledName() const { return "DepthWriteEnabled"; }

    inline const char* GetDepthWriteEnabledDescription() const
    {
        return "Whether depth writes are enabled. Depth writes are always disabled when "
               "DepthTestEnable is false.";
    }

    //-----------------------------------------------
    // FIELD DepthCompareOp: Comparison operator used for the depth test

    // `DepthCompareOpPtr()` returns a shared pointer to an array of `size()` elements
    inline const VkCompareOp* DepthCompareOpPtr() const
    {
        return reinterpret_cast<VkCompareOp*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                              kDepthCompareOpOffset * m_cap);
    }
    inline VkCompareOp* DepthCompareOpPtr()
    {
        return reinterpret_cast<VkCompareOp*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                              kDepthCompareOpOffset * m_cap);
    }
    // `DepthCompareOpPtr()` returns a shared pointer to an array of `size()` elements
    inline const VkCompareOp* DepthCompareOpPtr(Id id) const
    {
        return reinterpret_cast<VkCompareOp*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                              kDepthCompareOpOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline VkCompareOp* DepthCompareOpPtr(Id id)
    {
        return reinterpret_cast<VkCompareOp*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                              kDepthCompareOpOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `DepthCompareOp(id)` retuns the `DepthCompareOp` element of the object identified by `id`
    inline VkCompareOp DepthCompareOp(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *DepthCompareOpPtr(id);
    }

    // `SetDepthCompareOp(id,value)` sets the `DepthCompareOp` element of the object identified by
    // `id`
    inline SOA& SetDepthCompareOp(Id id, VkCompareOp value)
    {
        DIVE_ASSERT(IsValidId(id));
        *DepthCompareOpPtr(id) = value;
        MarkFieldSet(id, kDepthCompareOpIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsDepthCompareOpSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kDepthCompareOpIndex);
    }

    inline const char* GetDepthCompareOpName() const { return "DepthCompareOp"; }

    inline const char* GetDepthCompareOpDescription() const
    {
        return "Comparison operator used for the depth test";
    }

    //-----------------------------------------------
    // FIELD DepthBoundsTestEnabled: Whether depth bounds testing is enabled

    // `DepthBoundsTestEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* DepthBoundsTestEnabledPtr() const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kDepthBoundsTestEnabledOffset * m_cap);
    }
    inline bool* DepthBoundsTestEnabledPtr()
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kDepthBoundsTestEnabledOffset * m_cap);
    }
    // `DepthBoundsTestEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* DepthBoundsTestEnabledPtr(Id id) const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kDepthBoundsTestEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline bool* DepthBoundsTestEnabledPtr(Id id)
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kDepthBoundsTestEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `DepthBoundsTestEnabled(id)` retuns the `DepthBoundsTestEnabled` element of the object
    // identified by `id`
    inline bool DepthBoundsTestEnabled(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *DepthBoundsTestEnabledPtr(id);
    }

    // `SetDepthBoundsTestEnabled(id,value)` sets the `DepthBoundsTestEnabled` element of the object
    // identified by `id`
    inline SOA& SetDepthBoundsTestEnabled(Id id, bool value)
    {
        DIVE_ASSERT(IsValidId(id));
        *DepthBoundsTestEnabledPtr(id) = value;
        MarkFieldSet(id, kDepthBoundsTestEnabledIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsDepthBoundsTestEnabledSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kDepthBoundsTestEnabledIndex);
    }

    inline const char* GetDepthBoundsTestEnabledName() const { return "DepthBoundsTestEnabled"; }

    inline const char* GetDepthBoundsTestEnabledDescription() const
    {
        return "Whether depth bounds testing is enabled";
    }

    //-----------------------------------------------
    // FIELD MinDepthBounds: Minimum depth bound used in the depth bounds test

    // `MinDepthBoundsPtr()` returns a shared pointer to an array of `size()` elements
    inline const float* MinDepthBoundsPtr() const
    {
        return reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                        kMinDepthBoundsOffset * m_cap);
    }
    inline float* MinDepthBoundsPtr()
    {
        return reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                        kMinDepthBoundsOffset * m_cap);
    }
    // `MinDepthBoundsPtr()` returns a shared pointer to an array of `size()` elements
    inline const float* MinDepthBoundsPtr(Id id) const
    {
        return reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                        kMinDepthBoundsOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline float* MinDepthBoundsPtr(Id id)
    {
        return reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                        kMinDepthBoundsOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `MinDepthBounds(id)` retuns the `MinDepthBounds` element of the object identified by `id`
    inline float MinDepthBounds(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *MinDepthBoundsPtr(id);
    }

    // `SetMinDepthBounds(id,value)` sets the `MinDepthBounds` element of the object identified by
    // `id`
    inline SOA& SetMinDepthBounds(Id id, float value)
    {
        DIVE_ASSERT(IsValidId(id));
        *MinDepthBoundsPtr(id) = value;
        MarkFieldSet(id, kMinDepthBoundsIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsMinDepthBoundsSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kMinDepthBoundsIndex);
    }

    inline const char* GetMinDepthBoundsName() const { return "MinDepthBounds"; }

    inline const char* GetMinDepthBoundsDescription() const
    {
        return "Minimum depth bound used in the depth bounds test";
    }

    //-----------------------------------------------
    // FIELD MaxDepthBounds: Maximum depth bound used in the depth bounds test

    // `MaxDepthBoundsPtr()` returns a shared pointer to an array of `size()` elements
    inline const float* MaxDepthBoundsPtr() const
    {
        return reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                        kMaxDepthBoundsOffset * m_cap);
    }
    inline float* MaxDepthBoundsPtr()
    {
        return reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                        kMaxDepthBoundsOffset * m_cap);
    }
    // `MaxDepthBoundsPtr()` returns a shared pointer to an array of `size()` elements
    inline const float* MaxDepthBoundsPtr(Id id) const
    {
        return reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                        kMaxDepthBoundsOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline float* MaxDepthBoundsPtr(Id id)
    {
        return reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                        kMaxDepthBoundsOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `MaxDepthBounds(id)` retuns the `MaxDepthBounds` element of the object identified by `id`
    inline float MaxDepthBounds(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *MaxDepthBoundsPtr(id);
    }

    // `SetMaxDepthBounds(id,value)` sets the `MaxDepthBounds` element of the object identified by
    // `id`
    inline SOA& SetMaxDepthBounds(Id id, float value)
    {
        DIVE_ASSERT(IsValidId(id));
        *MaxDepthBoundsPtr(id) = value;
        MarkFieldSet(id, kMaxDepthBoundsIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsMaxDepthBoundsSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kMaxDepthBoundsIndex);
    }

    inline const char* GetMaxDepthBoundsName() const { return "MaxDepthBounds"; }

    inline const char* GetMaxDepthBoundsDescription() const
    {
        return "Maximum depth bound used in the depth bounds test";
    }

    //-----------------------------------------------
    // FIELD StencilTestEnabled: Whether stencil testing is enabled

    // `StencilTestEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* StencilTestEnabledPtr() const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kStencilTestEnabledOffset * m_cap);
    }
    inline bool* StencilTestEnabledPtr()
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kStencilTestEnabledOffset * m_cap);
    }
    // `StencilTestEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* StencilTestEnabledPtr(Id id) const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kStencilTestEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline bool* StencilTestEnabledPtr(Id id)
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kStencilTestEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `StencilTestEnabled(id)` retuns the `StencilTestEnabled` element of the object identified by
    // `id`
    inline bool StencilTestEnabled(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *StencilTestEnabledPtr(id);
    }

    // `SetStencilTestEnabled(id,value)` sets the `StencilTestEnabled` element of the object
    // identified by `id`
    inline SOA& SetStencilTestEnabled(Id id, bool value)
    {
        DIVE_ASSERT(IsValidId(id));
        *StencilTestEnabledPtr(id) = value;
        MarkFieldSet(id, kStencilTestEnabledIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsStencilTestEnabledSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kStencilTestEnabledIndex);
    }

    inline const char* GetStencilTestEnabledName() const { return "StencilTestEnabled"; }

    inline const char* GetStencilTestEnabledDescription() const
    {
        return "Whether stencil testing is enabled";
    }

    //-----------------------------------------------
    // FIELD StencilOpStateFront: Front parameter of the stencil test

    // `StencilOpStateFrontPtr()` returns a shared pointer to an array of `size()` elements
    inline const VkStencilOpState* StencilOpStateFrontPtr() const
    {
        return reinterpret_cast<VkStencilOpState*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                   kStencilOpStateFrontOffset * m_cap);
    }
    inline VkStencilOpState* StencilOpStateFrontPtr()
    {
        return reinterpret_cast<VkStencilOpState*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                   kStencilOpStateFrontOffset * m_cap);
    }
    // `StencilOpStateFrontPtr()` returns a shared pointer to an array of `size()` elements
    inline const VkStencilOpState* StencilOpStateFrontPtr(Id id) const
    {
        return reinterpret_cast<VkStencilOpState*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                   kStencilOpStateFrontOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline VkStencilOpState* StencilOpStateFrontPtr(Id id)
    {
        return reinterpret_cast<VkStencilOpState*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                   kStencilOpStateFrontOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `StencilOpStateFront(id)` retuns the `StencilOpStateFront` element of the object identified
    // by `id`
    inline VkStencilOpState StencilOpStateFront(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *StencilOpStateFrontPtr(id);
    }

    // `SetStencilOpStateFront(id,value)` sets the `StencilOpStateFront` element of the object
    // identified by `id`
    inline SOA& SetStencilOpStateFront(Id id, VkStencilOpState value)
    {
        DIVE_ASSERT(IsValidId(id));
        *StencilOpStateFrontPtr(id) = value;
        MarkFieldSet(id, kStencilOpStateFrontIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsStencilOpStateFrontSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kStencilOpStateFrontIndex);
    }

    inline const char* GetStencilOpStateFrontName() const { return "StencilOpStateFront"; }

    inline const char* GetStencilOpStateFrontDescription() const
    {
        return "Front parameter of the stencil test";
    }

    //-----------------------------------------------
    // FIELD StencilOpStateBack: Back parameter of the stencil test

    // `StencilOpStateBackPtr()` returns a shared pointer to an array of `size()` elements
    inline const VkStencilOpState* StencilOpStateBackPtr() const
    {
        return reinterpret_cast<VkStencilOpState*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                   kStencilOpStateBackOffset * m_cap);
    }
    inline VkStencilOpState* StencilOpStateBackPtr()
    {
        return reinterpret_cast<VkStencilOpState*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                   kStencilOpStateBackOffset * m_cap);
    }
    // `StencilOpStateBackPtr()` returns a shared pointer to an array of `size()` elements
    inline const VkStencilOpState* StencilOpStateBackPtr(Id id) const
    {
        return reinterpret_cast<VkStencilOpState*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                   kStencilOpStateBackOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline VkStencilOpState* StencilOpStateBackPtr(Id id)
    {
        return reinterpret_cast<VkStencilOpState*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                   kStencilOpStateBackOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `StencilOpStateBack(id)` retuns the `StencilOpStateBack` element of the object identified by
    // `id`
    inline VkStencilOpState StencilOpStateBack(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *StencilOpStateBackPtr(id);
    }

    // `SetStencilOpStateBack(id,value)` sets the `StencilOpStateBack` element of the object
    // identified by `id`
    inline SOA& SetStencilOpStateBack(Id id, VkStencilOpState value)
    {
        DIVE_ASSERT(IsValidId(id));
        *StencilOpStateBackPtr(id) = value;
        MarkFieldSet(id, kStencilOpStateBackIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsStencilOpStateBackSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kStencilOpStateBackIndex);
    }

    inline const char* GetStencilOpStateBackName() const { return "StencilOpStateBack"; }

    inline const char* GetStencilOpStateBackDescription() const
    {
        return "Back parameter of the stencil test";
    }

    //-----------------------------------------------
    // FIELD LogicOpEnabled: Whether to apply Logical Operations

    // `LogicOpEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* LogicOpEnabledPtr() const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kLogicOpEnabledOffset * m_cap);
    }
    inline bool* LogicOpEnabledPtr()
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kLogicOpEnabledOffset * m_cap);
    }
    // `LogicOpEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* LogicOpEnabledPtr(Id id, uint32_t attachment = 0) const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kLogicOpEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id) * 8 + attachment

        ;
    }
    inline bool* LogicOpEnabledPtr(Id id, uint32_t attachment = 0)
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kLogicOpEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id) * 8 + attachment

        ;
    }
    // `LogicOpEnabled(id)` retuns the `LogicOpEnabled` element of the object identified by `id`
    inline bool LogicOpEnabled(Id id, uint32_t attachment) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *LogicOpEnabledPtr(id, attachment);
    }

    // `LogicOpEnabled(id)` returns the array of values of the LogicOpEnabled field of the object
    // identified by `id`
    inline LogicOpEnabledArray LogicOpEnabled(Id id)
    {
        return LogicOpEnabledArray(static_cast<SOA*>(this), id);
    }
    inline LogicOpEnabledConstArray LogicOpEnabled(Id id) const
    {
        return LogicOpEnabledConstArray(static_cast<const SOA*>(this), id);
    }

    // `SetLogicOpEnabled(id,value)` sets the `LogicOpEnabled` element of the object identified by
    // `id`
    inline SOA& SetLogicOpEnabled(Id id, uint32_t attachment, bool value)
    {
        DIVE_ASSERT(IsValidId(id));
        *LogicOpEnabledPtr(id, attachment) = value;
        MarkFieldSet(id, kLogicOpEnabledIndex + attachment);
        return static_cast<SOA&>(*this);
    }

    inline bool IsLogicOpEnabledSet(Id id, uint32_t attachment) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kLogicOpEnabledIndex + attachment);
    }

    inline const char* GetLogicOpEnabledName() const { return "LogicOpEnabled"; }

    inline const char* GetLogicOpEnabledDescription() const
    {
        return "Whether to apply Logical Operations";
    }

    //-----------------------------------------------
    // FIELD LogicOp: Which logical operation to apply

    // `LogicOpPtr()` returns a shared pointer to an array of `size()` elements
    inline const VkLogicOp* LogicOpPtr() const
    {
        return reinterpret_cast<VkLogicOp*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                            kLogicOpOffset * m_cap);
    }
    inline VkLogicOp* LogicOpPtr()
    {
        return reinterpret_cast<VkLogicOp*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                            kLogicOpOffset * m_cap);
    }
    // `LogicOpPtr()` returns a shared pointer to an array of `size()` elements
    inline const VkLogicOp* LogicOpPtr(Id id, uint32_t attachment = 0) const
    {
        return reinterpret_cast<VkLogicOp*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                            kLogicOpOffset * m_cap) +
               static_cast<typename Id::basic_type>(id) * 8 + attachment

        ;
    }
    inline VkLogicOp* LogicOpPtr(Id id, uint32_t attachment = 0)
    {
        return reinterpret_cast<VkLogicOp*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                            kLogicOpOffset * m_cap) +
               static_cast<typename Id::basic_type>(id) * 8 + attachment

        ;
    }
    // `LogicOp(id)` retuns the `LogicOp` element of the object identified by `id`
    inline VkLogicOp LogicOp(Id id, uint32_t attachment) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *LogicOpPtr(id, attachment);
    }

    // `LogicOp(id)` returns the array of values of the LogicOp field of the object identified by
    // `id`
    inline LogicOpArray      LogicOp(Id id) { return LogicOpArray(static_cast<SOA*>(this), id); }
    inline LogicOpConstArray LogicOp(Id id) const
    {
        return LogicOpConstArray(static_cast<const SOA*>(this), id);
    }

    // `SetLogicOp(id,value)` sets the `LogicOp` element of the object identified by `id`
    inline SOA& SetLogicOp(Id id, uint32_t attachment, VkLogicOp value)
    {
        DIVE_ASSERT(IsValidId(id));
        *LogicOpPtr(id, attachment) = value;
        MarkFieldSet(id, kLogicOpIndex + attachment);
        return static_cast<SOA&>(*this);
    }

    inline bool IsLogicOpSet(Id id, uint32_t attachment) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kLogicOpIndex + attachment);
    }

    inline const char* GetLogicOpName() const { return "LogicOp"; }

    inline const char* GetLogicOpDescription() const { return "Which logical operation to apply"; }

    //-----------------------------------------------
    // FIELD Attachment: Per target attachment color blend states

    // `AttachmentPtr()` returns a shared pointer to an array of `size()` elements
    inline const VkPipelineColorBlendAttachmentState* AttachmentPtr() const
    {
        return reinterpret_cast<VkPipelineColorBlendAttachmentState*>(
        reinterpret_cast<uint8_t*>(m_buffer.get()) + kAttachmentOffset * m_cap);
    }
    inline VkPipelineColorBlendAttachmentState* AttachmentPtr()
    {
        return reinterpret_cast<VkPipelineColorBlendAttachmentState*>(
        reinterpret_cast<uint8_t*>(m_buffer.get()) + kAttachmentOffset * m_cap);
    }
    // `AttachmentPtr()` returns a shared pointer to an array of `size()` elements
    inline const VkPipelineColorBlendAttachmentState* AttachmentPtr(Id       id,
                                                                    uint32_t attachment = 0) const
    {
        return reinterpret_cast<VkPipelineColorBlendAttachmentState*>(
               reinterpret_cast<uint8_t*>(m_buffer.get()) + kAttachmentOffset * m_cap) +
               static_cast<typename Id::basic_type>(id) * 8 + attachment

        ;
    }
    inline VkPipelineColorBlendAttachmentState* AttachmentPtr(Id id, uint32_t attachment = 0)
    {
        return reinterpret_cast<VkPipelineColorBlendAttachmentState*>(
               reinterpret_cast<uint8_t*>(m_buffer.get()) + kAttachmentOffset * m_cap) +
               static_cast<typename Id::basic_type>(id) * 8 + attachment

        ;
    }
    // `Attachment(id)` retuns the `Attachment` element of the object identified by `id`
    inline VkPipelineColorBlendAttachmentState Attachment(Id id, uint32_t attachment) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *AttachmentPtr(id, attachment);
    }

    // `Attachment(id)` returns the array of values of the Attachment field of the object identified
    // by `id`
    inline AttachmentArray Attachment(Id id)
    {
        return AttachmentArray(static_cast<SOA*>(this), id);
    }
    inline AttachmentConstArray Attachment(Id id) const
    {
        return AttachmentConstArray(static_cast<const SOA*>(this), id);
    }

    // `SetAttachment(id,value)` sets the `Attachment` element of the object identified by `id`
    inline SOA& SetAttachment(Id id, uint32_t attachment, VkPipelineColorBlendAttachmentState value)
    {
        DIVE_ASSERT(IsValidId(id));
        *AttachmentPtr(id, attachment) = value;
        MarkFieldSet(id, kAttachmentIndex + attachment);
        return static_cast<SOA&>(*this);
    }

    inline bool IsAttachmentSet(Id id, uint32_t attachment) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kAttachmentIndex + attachment);
    }

    inline const char* GetAttachmentName() const { return "Attachment"; }

    inline const char* GetAttachmentDescription() const
    {
        return "Per target attachment color blend states";
    }

    //-----------------------------------------------
    // FIELD BlendConstant: A color constant used for blending

    // `BlendConstantPtr()` returns a shared pointer to an array of `size()` elements
    inline const float* BlendConstantPtr() const
    {
        return reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                        kBlendConstantOffset * m_cap);
    }
    inline float* BlendConstantPtr()
    {
        return reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                        kBlendConstantOffset * m_cap);
    }
    // `BlendConstantPtr()` returns a shared pointer to an array of `size()` elements
    inline const float* BlendConstantPtr(Id id, uint32_t channel = 0) const
    {
        return reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                        kBlendConstantOffset * m_cap) +
               static_cast<typename Id::basic_type>(id) * 4 + channel

        ;
    }
    inline float* BlendConstantPtr(Id id, uint32_t channel = 0)
    {
        return reinterpret_cast<float*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                        kBlendConstantOffset * m_cap) +
               static_cast<typename Id::basic_type>(id) * 4 + channel

        ;
    }
    // `BlendConstant(id)` retuns the `BlendConstant` element of the object identified by `id`
    inline float BlendConstant(Id id, uint32_t channel) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *BlendConstantPtr(id, channel);
    }

    // `BlendConstant(id)` returns the array of values of the BlendConstant field of the object
    // identified by `id`
    inline BlendConstantArray BlendConstant(Id id)
    {
        return BlendConstantArray(static_cast<SOA*>(this), id);
    }
    inline BlendConstantConstArray BlendConstant(Id id) const
    {
        return BlendConstantConstArray(static_cast<const SOA*>(this), id);
    }

    // `SetBlendConstant(id,value)` sets the `BlendConstant` element of the object identified by
    // `id`
    inline SOA& SetBlendConstant(Id id, uint32_t channel, float value)
    {
        DIVE_ASSERT(IsValidId(id));
        *BlendConstantPtr(id, channel) = value;
        MarkFieldSet(id, kBlendConstantIndex + channel);
        return static_cast<SOA&>(*this);
    }

    inline bool IsBlendConstantSet(Id id, uint32_t channel) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kBlendConstantIndex + channel);
    }

    inline const char* GetBlendConstantName() const { return "BlendConstant"; }

    inline const char* GetBlendConstantDescription() const
    {
        return "A color constant used for blending";
    }

    //-----------------------------------------------
    // FIELD LRZEnabled: Whether LRZ is enabled for depth

    // `LRZEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* LRZEnabledPtr() const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kLRZEnabledOffset * m_cap);
    }
    inline bool* LRZEnabledPtr()
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kLRZEnabledOffset * m_cap);
    }
    // `LRZEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* LRZEnabledPtr(Id id) const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kLRZEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline bool* LRZEnabledPtr(Id id)
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kLRZEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `LRZEnabled(id)` retuns the `LRZEnabled` element of the object identified by `id`
    inline bool LRZEnabled(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *LRZEnabledPtr(id);
    }

    // `SetLRZEnabled(id,value)` sets the `LRZEnabled` element of the object identified by `id`
    inline SOA& SetLRZEnabled(Id id, bool value)
    {
        DIVE_ASSERT(IsValidId(id));
        *LRZEnabledPtr(id) = value;
        MarkFieldSet(id, kLRZEnabledIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsLRZEnabledSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kLRZEnabledIndex);
    }

    inline const char* GetLRZEnabledName() const { return "LRZEnabled"; }

    inline const char* GetLRZEnabledDescription() const
    {
        return "Whether LRZ is enabled for depth";
    }

    //-----------------------------------------------
    // FIELD LRZWrite: Whether LRZ write is enabled

    // `LRZWritePtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* LRZWritePtr() const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kLRZWriteOffset * m_cap);
    }
    inline bool* LRZWritePtr()
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kLRZWriteOffset * m_cap);
    }
    // `LRZWritePtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* LRZWritePtr(Id id) const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kLRZWriteOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline bool* LRZWritePtr(Id id)
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kLRZWriteOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `LRZWrite(id)` retuns the `LRZWrite` element of the object identified by `id`
    inline bool LRZWrite(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *LRZWritePtr(id);
    }

    // `SetLRZWrite(id,value)` sets the `LRZWrite` element of the object identified by `id`
    inline SOA& SetLRZWrite(Id id, bool value)
    {
        DIVE_ASSERT(IsValidId(id));
        *LRZWritePtr(id) = value;
        MarkFieldSet(id, kLRZWriteIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsLRZWriteSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kLRZWriteIndex);
    }

    inline const char* GetLRZWriteName() const { return "LRZWrite"; }

    inline const char* GetLRZWriteDescription() const { return "Whether LRZ write is enabled"; }

    //-----------------------------------------------
    // FIELD LRZDirStatus: LRZ direction

    // `LRZDirStatusPtr()` returns a shared pointer to an array of `size()` elements
    inline const a6xx_lrz_dir_status* LRZDirStatusPtr() const
    {
        return reinterpret_cast<a6xx_lrz_dir_status*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                      kLRZDirStatusOffset * m_cap);
    }
    inline a6xx_lrz_dir_status* LRZDirStatusPtr()
    {
        return reinterpret_cast<a6xx_lrz_dir_status*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                      kLRZDirStatusOffset * m_cap);
    }
    // `LRZDirStatusPtr()` returns a shared pointer to an array of `size()` elements
    inline const a6xx_lrz_dir_status* LRZDirStatusPtr(Id id) const
    {
        return reinterpret_cast<a6xx_lrz_dir_status*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                      kLRZDirStatusOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline a6xx_lrz_dir_status* LRZDirStatusPtr(Id id)
    {
        return reinterpret_cast<a6xx_lrz_dir_status*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                      kLRZDirStatusOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `LRZDirStatus(id)` retuns the `LRZDirStatus` element of the object identified by `id`
    inline a6xx_lrz_dir_status LRZDirStatus(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *LRZDirStatusPtr(id);
    }

    // `SetLRZDirStatus(id,value)` sets the `LRZDirStatus` element of the object identified by `id`
    inline SOA& SetLRZDirStatus(Id id, a6xx_lrz_dir_status value)
    {
        DIVE_ASSERT(IsValidId(id));
        *LRZDirStatusPtr(id) = value;
        MarkFieldSet(id, kLRZDirStatusIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsLRZDirStatusSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kLRZDirStatusIndex);
    }

    inline const char* GetLRZDirStatusName() const { return "LRZDirStatus"; }

    inline const char* GetLRZDirStatusDescription() const { return "LRZ direction"; }

    //-----------------------------------------------
    // FIELD LRZDirWrite: Whether LRZ direction write is enabled

    // `LRZDirWritePtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* LRZDirWritePtr() const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kLRZDirWriteOffset * m_cap);
    }
    inline bool* LRZDirWritePtr()
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kLRZDirWriteOffset * m_cap);
    }
    // `LRZDirWritePtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* LRZDirWritePtr(Id id) const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kLRZDirWriteOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline bool* LRZDirWritePtr(Id id)
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kLRZDirWriteOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `LRZDirWrite(id)` retuns the `LRZDirWrite` element of the object identified by `id`
    inline bool LRZDirWrite(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *LRZDirWritePtr(id);
    }

    // `SetLRZDirWrite(id,value)` sets the `LRZDirWrite` element of the object identified by `id`
    inline SOA& SetLRZDirWrite(Id id, bool value)
    {
        DIVE_ASSERT(IsValidId(id));
        *LRZDirWritePtr(id) = value;
        MarkFieldSet(id, kLRZDirWriteIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsLRZDirWriteSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kLRZDirWriteIndex);
    }

    inline const char* GetLRZDirWriteName() const { return "LRZDirWrite"; }

    inline const char* GetLRZDirWriteDescription() const
    {
        return "Whether LRZ direction write is enabled";
    }

    //-----------------------------------------------
    // FIELD ZTestMode: Depth test mode

    // `ZTestModePtr()` returns a shared pointer to an array of `size()` elements
    inline const a6xx_ztest_mode* ZTestModePtr() const
    {
        return reinterpret_cast<a6xx_ztest_mode*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                  kZTestModeOffset * m_cap);
    }
    inline a6xx_ztest_mode* ZTestModePtr()
    {
        return reinterpret_cast<a6xx_ztest_mode*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                  kZTestModeOffset * m_cap);
    }
    // `ZTestModePtr()` returns a shared pointer to an array of `size()` elements
    inline const a6xx_ztest_mode* ZTestModePtr(Id id) const
    {
        return reinterpret_cast<a6xx_ztest_mode*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                  kZTestModeOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline a6xx_ztest_mode* ZTestModePtr(Id id)
    {
        return reinterpret_cast<a6xx_ztest_mode*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                  kZTestModeOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `ZTestMode(id)` retuns the `ZTestMode` element of the object identified by `id`
    inline a6xx_ztest_mode ZTestMode(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *ZTestModePtr(id);
    }

    // `SetZTestMode(id,value)` sets the `ZTestMode` element of the object identified by `id`
    inline SOA& SetZTestMode(Id id, a6xx_ztest_mode value)
    {
        DIVE_ASSERT(IsValidId(id));
        *ZTestModePtr(id) = value;
        MarkFieldSet(id, kZTestModeIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsZTestModeSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kZTestModeIndex);
    }

    inline const char* GetZTestModeName() const { return "ZTestMode"; }

    inline const char* GetZTestModeDescription() const { return "Depth test mode"; }

    //-----------------------------------------------
    // FIELD BinW: Bin width

    // `BinWPtr()` returns a shared pointer to an array of `size()` elements
    inline const uint32_t* BinWPtr() const
    {
        return reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kBinWOffset * m_cap);
    }
    inline uint32_t* BinWPtr()
    {
        return reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kBinWOffset * m_cap);
    }
    // `BinWPtr()` returns a shared pointer to an array of `size()` elements
    inline const uint32_t* BinWPtr(Id id) const
    {
        return reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kBinWOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline uint32_t* BinWPtr(Id id)
    {
        return reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kBinWOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `BinW(id)` retuns the `BinW` element of the object identified by `id`
    inline uint32_t BinW(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *BinWPtr(id);
    }

    // `SetBinW(id,value)` sets the `BinW` element of the object identified by `id`
    inline SOA& SetBinW(Id id, uint32_t value)
    {
        DIVE_ASSERT(IsValidId(id));
        *BinWPtr(id) = value;
        MarkFieldSet(id, kBinWIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsBinWSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kBinWIndex);
    }

    inline const char* GetBinWName() const { return "BinW"; }

    inline const char* GetBinWDescription() const { return "Bin width"; }

    //-----------------------------------------------
    // FIELD BinH: Bin Height

    // `BinHPtr()` returns a shared pointer to an array of `size()` elements
    inline const uint32_t* BinHPtr() const
    {
        return reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kBinHOffset * m_cap);
    }
    inline uint32_t* BinHPtr()
    {
        return reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kBinHOffset * m_cap);
    }
    // `BinHPtr()` returns a shared pointer to an array of `size()` elements
    inline const uint32_t* BinHPtr(Id id) const
    {
        return reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kBinHOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline uint32_t* BinHPtr(Id id)
    {
        return reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kBinHOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `BinH(id)` retuns the `BinH` element of the object identified by `id`
    inline uint32_t BinH(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *BinHPtr(id);
    }

    // `SetBinH(id,value)` sets the `BinH` element of the object identified by `id`
    inline SOA& SetBinH(Id id, uint32_t value)
    {
        DIVE_ASSERT(IsValidId(id));
        *BinHPtr(id) = value;
        MarkFieldSet(id, kBinHIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsBinHSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kBinHIndex);
    }

    inline const char* GetBinHName() const { return "BinH"; }

    inline const char* GetBinHDescription() const { return "Bin Height"; }

    //-----------------------------------------------
    // FIELD WindowScissorTLX: Window scissor Top Left X-coordinate

    // `WindowScissorTLXPtr()` returns a shared pointer to an array of `size()` elements
    inline const uint16_t* WindowScissorTLXPtr() const
    {
        return reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kWindowScissorTLXOffset * m_cap);
    }
    inline uint16_t* WindowScissorTLXPtr()
    {
        return reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kWindowScissorTLXOffset * m_cap);
    }
    // `WindowScissorTLXPtr()` returns a shared pointer to an array of `size()` elements
    inline const uint16_t* WindowScissorTLXPtr(Id id) const
    {
        return reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kWindowScissorTLXOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline uint16_t* WindowScissorTLXPtr(Id id)
    {
        return reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kWindowScissorTLXOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `WindowScissorTLX(id)` retuns the `WindowScissorTLX` element of the object identified by `id`
    inline uint16_t WindowScissorTLX(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *WindowScissorTLXPtr(id);
    }

    // `SetWindowScissorTLX(id,value)` sets the `WindowScissorTLX` element of the object identified
    // by `id`
    inline SOA& SetWindowScissorTLX(Id id, uint16_t value)
    {
        DIVE_ASSERT(IsValidId(id));
        *WindowScissorTLXPtr(id) = value;
        MarkFieldSet(id, kWindowScissorTLXIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsWindowScissorTLXSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kWindowScissorTLXIndex);
    }

    inline const char* GetWindowScissorTLXName() const { return "WindowScissorTLX"; }

    inline const char* GetWindowScissorTLXDescription() const
    {
        return "Window scissor Top Left X-coordinate";
    }

    //-----------------------------------------------
    // FIELD WindowScissorTLY: Window scissor Top Left Y-coordinate

    // `WindowScissorTLYPtr()` returns a shared pointer to an array of `size()` elements
    inline const uint16_t* WindowScissorTLYPtr() const
    {
        return reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kWindowScissorTLYOffset * m_cap);
    }
    inline uint16_t* WindowScissorTLYPtr()
    {
        return reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kWindowScissorTLYOffset * m_cap);
    }
    // `WindowScissorTLYPtr()` returns a shared pointer to an array of `size()` elements
    inline const uint16_t* WindowScissorTLYPtr(Id id) const
    {
        return reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kWindowScissorTLYOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline uint16_t* WindowScissorTLYPtr(Id id)
    {
        return reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kWindowScissorTLYOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `WindowScissorTLY(id)` retuns the `WindowScissorTLY` element of the object identified by `id`
    inline uint16_t WindowScissorTLY(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *WindowScissorTLYPtr(id);
    }

    // `SetWindowScissorTLY(id,value)` sets the `WindowScissorTLY` element of the object identified
    // by `id`
    inline SOA& SetWindowScissorTLY(Id id, uint16_t value)
    {
        DIVE_ASSERT(IsValidId(id));
        *WindowScissorTLYPtr(id) = value;
        MarkFieldSet(id, kWindowScissorTLYIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsWindowScissorTLYSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kWindowScissorTLYIndex);
    }

    inline const char* GetWindowScissorTLYName() const { return "WindowScissorTLY"; }

    inline const char* GetWindowScissorTLYDescription() const
    {
        return "Window scissor Top Left Y-coordinate";
    }

    //-----------------------------------------------
    // FIELD WindowScissorBRX: Window scissor Bottom Right X-coordinate

    // `WindowScissorBRXPtr()` returns a shared pointer to an array of `size()` elements
    inline const uint16_t* WindowScissorBRXPtr() const
    {
        return reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kWindowScissorBRXOffset * m_cap);
    }
    inline uint16_t* WindowScissorBRXPtr()
    {
        return reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kWindowScissorBRXOffset * m_cap);
    }
    // `WindowScissorBRXPtr()` returns a shared pointer to an array of `size()` elements
    inline const uint16_t* WindowScissorBRXPtr(Id id) const
    {
        return reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kWindowScissorBRXOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline uint16_t* WindowScissorBRXPtr(Id id)
    {
        return reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kWindowScissorBRXOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `WindowScissorBRX(id)` retuns the `WindowScissorBRX` element of the object identified by `id`
    inline uint16_t WindowScissorBRX(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *WindowScissorBRXPtr(id);
    }

    // `SetWindowScissorBRX(id,value)` sets the `WindowScissorBRX` element of the object identified
    // by `id`
    inline SOA& SetWindowScissorBRX(Id id, uint16_t value)
    {
        DIVE_ASSERT(IsValidId(id));
        *WindowScissorBRXPtr(id) = value;
        MarkFieldSet(id, kWindowScissorBRXIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsWindowScissorBRXSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kWindowScissorBRXIndex);
    }

    inline const char* GetWindowScissorBRXName() const { return "WindowScissorBRX"; }

    inline const char* GetWindowScissorBRXDescription() const
    {
        return "Window scissor Bottom Right X-coordinate";
    }

    //-----------------------------------------------
    // FIELD WindowScissorBRY: Window scissor Bottom Right Y-coordinate

    // `WindowScissorBRYPtr()` returns a shared pointer to an array of `size()` elements
    inline const uint16_t* WindowScissorBRYPtr() const
    {
        return reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kWindowScissorBRYOffset * m_cap);
    }
    inline uint16_t* WindowScissorBRYPtr()
    {
        return reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kWindowScissorBRYOffset * m_cap);
    }
    // `WindowScissorBRYPtr()` returns a shared pointer to an array of `size()` elements
    inline const uint16_t* WindowScissorBRYPtr(Id id) const
    {
        return reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kWindowScissorBRYOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline uint16_t* WindowScissorBRYPtr(Id id)
    {
        return reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kWindowScissorBRYOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `WindowScissorBRY(id)` retuns the `WindowScissorBRY` element of the object identified by `id`
    inline uint16_t WindowScissorBRY(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *WindowScissorBRYPtr(id);
    }

    // `SetWindowScissorBRY(id,value)` sets the `WindowScissorBRY` element of the object identified
    // by `id`
    inline SOA& SetWindowScissorBRY(Id id, uint16_t value)
    {
        DIVE_ASSERT(IsValidId(id));
        *WindowScissorBRYPtr(id) = value;
        MarkFieldSet(id, kWindowScissorBRYIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsWindowScissorBRYSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kWindowScissorBRYIndex);
    }

    inline const char* GetWindowScissorBRYName() const { return "WindowScissorBRY"; }

    inline const char* GetWindowScissorBRYDescription() const
    {
        return "Window scissor Bottom Right Y-coordinate";
    }

    //-----------------------------------------------
    // FIELD RenderMode: Whether in binning pass or rendering pass

    // `RenderModePtr()` returns a shared pointer to an array of `size()` elements
    inline const a6xx_render_mode* RenderModePtr() const
    {
        return reinterpret_cast<a6xx_render_mode*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                   kRenderModeOffset * m_cap);
    }
    inline a6xx_render_mode* RenderModePtr()
    {
        return reinterpret_cast<a6xx_render_mode*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                   kRenderModeOffset * m_cap);
    }
    // `RenderModePtr()` returns a shared pointer to an array of `size()` elements
    inline const a6xx_render_mode* RenderModePtr(Id id) const
    {
        return reinterpret_cast<a6xx_render_mode*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                   kRenderModeOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline a6xx_render_mode* RenderModePtr(Id id)
    {
        return reinterpret_cast<a6xx_render_mode*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                   kRenderModeOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `RenderMode(id)` retuns the `RenderMode` element of the object identified by `id`
    inline a6xx_render_mode RenderMode(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *RenderModePtr(id);
    }

    // `SetRenderMode(id,value)` sets the `RenderMode` element of the object identified by `id`
    inline SOA& SetRenderMode(Id id, a6xx_render_mode value)
    {
        DIVE_ASSERT(IsValidId(id));
        *RenderModePtr(id) = value;
        MarkFieldSet(id, kRenderModeIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsRenderModeSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kRenderModeIndex);
    }

    inline const char* GetRenderModeName() const { return "RenderMode"; }

    inline const char* GetRenderModeDescription() const
    {
        return "Whether in binning pass or rendering pass";
    }

    //-----------------------------------------------
    // FIELD BuffersLocation: Whether the target buffer is in GMEM or SYSMEM

    // `BuffersLocationPtr()` returns a shared pointer to an array of `size()` elements
    inline const a6xx_buffers_location* BuffersLocationPtr() const
    {
        return reinterpret_cast<a6xx_buffers_location*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                        kBuffersLocationOffset * m_cap);
    }
    inline a6xx_buffers_location* BuffersLocationPtr()
    {
        return reinterpret_cast<a6xx_buffers_location*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                        kBuffersLocationOffset * m_cap);
    }
    // `BuffersLocationPtr()` returns a shared pointer to an array of `size()` elements
    inline const a6xx_buffers_location* BuffersLocationPtr(Id id) const
    {
        return reinterpret_cast<a6xx_buffers_location*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                        kBuffersLocationOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline a6xx_buffers_location* BuffersLocationPtr(Id id)
    {
        return reinterpret_cast<a6xx_buffers_location*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                        kBuffersLocationOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `BuffersLocation(id)` retuns the `BuffersLocation` element of the object identified by `id`
    inline a6xx_buffers_location BuffersLocation(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *BuffersLocationPtr(id);
    }

    // `SetBuffersLocation(id,value)` sets the `BuffersLocation` element of the object identified by
    // `id`
    inline SOA& SetBuffersLocation(Id id, a6xx_buffers_location value)
    {
        DIVE_ASSERT(IsValidId(id));
        *BuffersLocationPtr(id) = value;
        MarkFieldSet(id, kBuffersLocationIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsBuffersLocationSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kBuffersLocationIndex);
    }

    inline const char* GetBuffersLocationName() const { return "BuffersLocation"; }

    inline const char* GetBuffersLocationDescription() const
    {
        return "Whether the target buffer is in GMEM or SYSMEM";
    }

    //-----------------------------------------------
    // FIELD ThreadSize: Whether the thread size is 64 or 128

    // `ThreadSizePtr()` returns a shared pointer to an array of `size()` elements
    inline const a6xx_threadsize* ThreadSizePtr() const
    {
        return reinterpret_cast<a6xx_threadsize*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                  kThreadSizeOffset * m_cap);
    }
    inline a6xx_threadsize* ThreadSizePtr()
    {
        return reinterpret_cast<a6xx_threadsize*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                  kThreadSizeOffset * m_cap);
    }
    // `ThreadSizePtr()` returns a shared pointer to an array of `size()` elements
    inline const a6xx_threadsize* ThreadSizePtr(Id id) const
    {
        return reinterpret_cast<a6xx_threadsize*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                  kThreadSizeOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline a6xx_threadsize* ThreadSizePtr(Id id)
    {
        return reinterpret_cast<a6xx_threadsize*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                  kThreadSizeOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `ThreadSize(id)` retuns the `ThreadSize` element of the object identified by `id`
    inline a6xx_threadsize ThreadSize(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *ThreadSizePtr(id);
    }

    // `SetThreadSize(id,value)` sets the `ThreadSize` element of the object identified by `id`
    inline SOA& SetThreadSize(Id id, a6xx_threadsize value)
    {
        DIVE_ASSERT(IsValidId(id));
        *ThreadSizePtr(id) = value;
        MarkFieldSet(id, kThreadSizeIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsThreadSizeSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kThreadSizeIndex);
    }

    inline const char* GetThreadSizeName() const { return "ThreadSize"; }

    inline const char* GetThreadSizeDescription() const
    {
        return "Whether the thread size is 64 or 128";
    }

    //-----------------------------------------------
    // FIELD EnableAllHelperLanes: Whether all helper lanes are enabled of the 2x2 quad for fine
    // derivatives

    // `EnableAllHelperLanesPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* EnableAllHelperLanesPtr() const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kEnableAllHelperLanesOffset * m_cap);
    }
    inline bool* EnableAllHelperLanesPtr()
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kEnableAllHelperLanesOffset * m_cap);
    }
    // `EnableAllHelperLanesPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* EnableAllHelperLanesPtr(Id id) const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kEnableAllHelperLanesOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline bool* EnableAllHelperLanesPtr(Id id)
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kEnableAllHelperLanesOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `EnableAllHelperLanes(id)` retuns the `EnableAllHelperLanes` element of the object identified
    // by `id`
    inline bool EnableAllHelperLanes(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *EnableAllHelperLanesPtr(id);
    }

    // `SetEnableAllHelperLanes(id,value)` sets the `EnableAllHelperLanes` element of the object
    // identified by `id`
    inline SOA& SetEnableAllHelperLanes(Id id, bool value)
    {
        DIVE_ASSERT(IsValidId(id));
        *EnableAllHelperLanesPtr(id) = value;
        MarkFieldSet(id, kEnableAllHelperLanesIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsEnableAllHelperLanesSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kEnableAllHelperLanesIndex);
    }

    inline const char* GetEnableAllHelperLanesName() const { return "EnableAllHelperLanes"; }

    inline const char* GetEnableAllHelperLanesDescription() const
    {
        return "Whether all helper lanes are enabled of the 2x2 quad for fine derivatives";
    }

    //-----------------------------------------------
    // FIELD EnablePartialHelperLanes: Whether 3 out of 4 helper lanes are enabled of the 2x2 quad
    // for coarse derivatives

    // `EnablePartialHelperLanesPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* EnablePartialHelperLanesPtr() const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kEnablePartialHelperLanesOffset * m_cap);
    }
    inline bool* EnablePartialHelperLanesPtr()
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kEnablePartialHelperLanesOffset * m_cap);
    }
    // `EnablePartialHelperLanesPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* EnablePartialHelperLanesPtr(Id id) const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kEnablePartialHelperLanesOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline bool* EnablePartialHelperLanesPtr(Id id)
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kEnablePartialHelperLanesOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `EnablePartialHelperLanes(id)` retuns the `EnablePartialHelperLanes` element of the object
    // identified by `id`
    inline bool EnablePartialHelperLanes(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *EnablePartialHelperLanesPtr(id);
    }

    // `SetEnablePartialHelperLanes(id,value)` sets the `EnablePartialHelperLanes` element of the
    // object identified by `id`
    inline SOA& SetEnablePartialHelperLanes(Id id, bool value)
    {
        DIVE_ASSERT(IsValidId(id));
        *EnablePartialHelperLanesPtr(id) = value;
        MarkFieldSet(id, kEnablePartialHelperLanesIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsEnablePartialHelperLanesSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kEnablePartialHelperLanesIndex);
    }

    inline const char* GetEnablePartialHelperLanesName() const
    {
        return "EnablePartialHelperLanes";
    }

    inline const char* GetEnablePartialHelperLanesDescription() const
    {
        return "Whether 3 out of 4 helper lanes are enabled of the 2x2 quad for coarse derivatives";
    }

    //-----------------------------------------------
    // FIELD UBWCEnabled: Whether UBWC is enabled for this attachment

    // `UBWCEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* UBWCEnabledPtr() const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kUBWCEnabledOffset * m_cap);
    }
    inline bool* UBWCEnabledPtr()
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kUBWCEnabledOffset * m_cap);
    }
    // `UBWCEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* UBWCEnabledPtr(Id id, uint32_t attachment = 0) const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kUBWCEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id) * 8 + attachment

        ;
    }
    inline bool* UBWCEnabledPtr(Id id, uint32_t attachment = 0)
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kUBWCEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id) * 8 + attachment

        ;
    }
    // `UBWCEnabled(id)` retuns the `UBWCEnabled` element of the object identified by `id`
    inline bool UBWCEnabled(Id id, uint32_t attachment) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *UBWCEnabledPtr(id, attachment);
    }

    // `UBWCEnabled(id)` returns the array of values of the UBWCEnabled field of the object
    // identified by `id`
    inline UBWCEnabledArray UBWCEnabled(Id id)
    {
        return UBWCEnabledArray(static_cast<SOA*>(this), id);
    }
    inline UBWCEnabledConstArray UBWCEnabled(Id id) const
    {
        return UBWCEnabledConstArray(static_cast<const SOA*>(this), id);
    }

    // `SetUBWCEnabled(id,value)` sets the `UBWCEnabled` element of the object identified by `id`
    inline SOA& SetUBWCEnabled(Id id, uint32_t attachment, bool value)
    {
        DIVE_ASSERT(IsValidId(id));
        *UBWCEnabledPtr(id, attachment) = value;
        MarkFieldSet(id, kUBWCEnabledIndex + attachment);
        return static_cast<SOA&>(*this);
    }

    inline bool IsUBWCEnabledSet(Id id, uint32_t attachment) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kUBWCEnabledIndex + attachment);
    }

    inline const char* GetUBWCEnabledName() const { return "UBWCEnabled"; }

    inline const char* GetUBWCEnabledDescription() const
    {
        return "Whether UBWC is enabled for this attachment";
    }

    //-----------------------------------------------
    // FIELD UBWCLosslessEnabled: Whether UBWC Lossless compression (A7XX+) is enabled for this
    // attachment

    // `UBWCLosslessEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* UBWCLosslessEnabledPtr() const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kUBWCLosslessEnabledOffset * m_cap);
    }
    inline bool* UBWCLosslessEnabledPtr()
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kUBWCLosslessEnabledOffset * m_cap);
    }
    // `UBWCLosslessEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* UBWCLosslessEnabledPtr(Id id, uint32_t attachment = 0) const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kUBWCLosslessEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id) * 8 + attachment

        ;
    }
    inline bool* UBWCLosslessEnabledPtr(Id id, uint32_t attachment = 0)
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kUBWCLosslessEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id) * 8 + attachment

        ;
    }
    // `UBWCLosslessEnabled(id)` retuns the `UBWCLosslessEnabled` element of the object identified
    // by `id`
    inline bool UBWCLosslessEnabled(Id id, uint32_t attachment) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *UBWCLosslessEnabledPtr(id, attachment);
    }

    // `UBWCLosslessEnabled(id)` returns the array of values of the UBWCLosslessEnabled field of the
    // object identified by `id`
    inline UBWCLosslessEnabledArray UBWCLosslessEnabled(Id id)
    {
        return UBWCLosslessEnabledArray(static_cast<SOA*>(this), id);
    }
    inline UBWCLosslessEnabledConstArray UBWCLosslessEnabled(Id id) const
    {
        return UBWCLosslessEnabledConstArray(static_cast<const SOA*>(this), id);
    }

    // `SetUBWCLosslessEnabled(id,value)` sets the `UBWCLosslessEnabled` element of the object
    // identified by `id`
    inline SOA& SetUBWCLosslessEnabled(Id id, uint32_t attachment, bool value)
    {
        DIVE_ASSERT(IsValidId(id));
        *UBWCLosslessEnabledPtr(id, attachment) = value;
        MarkFieldSet(id, kUBWCLosslessEnabledIndex + attachment);
        return static_cast<SOA&>(*this);
    }

    inline bool IsUBWCLosslessEnabledSet(Id id, uint32_t attachment) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kUBWCLosslessEnabledIndex + attachment);
    }

    inline const char* GetUBWCLosslessEnabledName() const { return "UBWCLosslessEnabled"; }

    inline const char* GetUBWCLosslessEnabledDescription() const
    {
        return "Whether UBWC Lossless compression (A7XX+) is enabled for this attachment";
    }

    //-----------------------------------------------
    // FIELD UBWCEnabledOnDS: Whether UBWC is enabled for this depth stencil attachment

    // `UBWCEnabledOnDSPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* UBWCEnabledOnDSPtr() const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kUBWCEnabledOnDSOffset * m_cap);
    }
    inline bool* UBWCEnabledOnDSPtr()
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kUBWCEnabledOnDSOffset * m_cap);
    }
    // `UBWCEnabledOnDSPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* UBWCEnabledOnDSPtr(Id id) const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kUBWCEnabledOnDSOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline bool* UBWCEnabledOnDSPtr(Id id)
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kUBWCEnabledOnDSOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `UBWCEnabledOnDS(id)` retuns the `UBWCEnabledOnDS` element of the object identified by `id`
    inline bool UBWCEnabledOnDS(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *UBWCEnabledOnDSPtr(id);
    }

    // `SetUBWCEnabledOnDS(id,value)` sets the `UBWCEnabledOnDS` element of the object identified by
    // `id`
    inline SOA& SetUBWCEnabledOnDS(Id id, bool value)
    {
        DIVE_ASSERT(IsValidId(id));
        *UBWCEnabledOnDSPtr(id) = value;
        MarkFieldSet(id, kUBWCEnabledOnDSIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsUBWCEnabledOnDSSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kUBWCEnabledOnDSIndex);
    }

    inline const char* GetUBWCEnabledOnDSName() const { return "UBWCEnabledOnDS"; }

    inline const char* GetUBWCEnabledOnDSDescription() const
    {
        return "Whether UBWC is enabled for this depth stencil attachment";
    }

    //-----------------------------------------------
    // FIELD UBWCLosslessEnabledOnDS: Whether UBWC Lossless compression (A7XX+) is enabled for this
    // depth stencil attachment

    // `UBWCLosslessEnabledOnDSPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* UBWCLosslessEnabledOnDSPtr() const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kUBWCLosslessEnabledOnDSOffset * m_cap);
    }
    inline bool* UBWCLosslessEnabledOnDSPtr()
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kUBWCLosslessEnabledOnDSOffset * m_cap);
    }
    // `UBWCLosslessEnabledOnDSPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* UBWCLosslessEnabledOnDSPtr(Id id) const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kUBWCLosslessEnabledOnDSOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline bool* UBWCLosslessEnabledOnDSPtr(Id id)
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kUBWCLosslessEnabledOnDSOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `UBWCLosslessEnabledOnDS(id)` retuns the `UBWCLosslessEnabledOnDS` element of the object
    // identified by `id`
    inline bool UBWCLosslessEnabledOnDS(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *UBWCLosslessEnabledOnDSPtr(id);
    }

    // `SetUBWCLosslessEnabledOnDS(id,value)` sets the `UBWCLosslessEnabledOnDS` element of the
    // object identified by `id`
    inline SOA& SetUBWCLosslessEnabledOnDS(Id id, bool value)
    {
        DIVE_ASSERT(IsValidId(id));
        *UBWCLosslessEnabledOnDSPtr(id) = value;
        MarkFieldSet(id, kUBWCLosslessEnabledOnDSIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsUBWCLosslessEnabledOnDSSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kUBWCLosslessEnabledOnDSIndex);
    }

    inline const char* GetUBWCLosslessEnabledOnDSName() const { return "UBWCLosslessEnabledOnDS"; }

    inline const char* GetUBWCLosslessEnabledOnDSDescription() const
    {
        return "Whether UBWC Lossless compression (A7XX+) is enabled for this depth stencil "
               "attachment";
    }

    // `operator[]` returns a reference to an element identified by `id`. E.g.:
    //   `my_soa[some_id].MyField()`
    inline Ref      operator[](Id id) { return Ref(static_cast<SOA*>(this), id); }
    inline ConstRef operator[](Id id) const { return ConstRef(static_cast<const SOA*>(this), id); }

    inline Iterator      begin() { return Iterator(static_cast<SOA*>(this), Id(0)); }
    inline Iterator      end() { return Iterator(static_cast<SOA*>(this), Id(m_size)); }
    inline ConstIterator begin() const
    {
        return ConstIterator(static_cast<const SOA*>(this), Id(0));
    }
    inline ConstIterator end() const
    {
        return ConstIterator(static_cast<const SOA*>(this), Id(m_size));
    }
    inline Ref      front() { return Ref(static_cast<SOA*>(this), Id(0)); }
    inline ConstRef front() const { return ConstRef(static_cast<const SOA*>(this), Id(0)); }
    inline Ref      back() { return Ref(static_cast<SOA*>(this), Id(m_size - 1)); }
    inline ConstRef back() const { return ConstRef(static_cast<const SOA*>(this), Id(m_size - 1)); }

    // `find(id)` returns an iterator referring to the element identified by `id`.
    // If `id` does not identify a valid element, then `find(id) == end()` and
    // `find(id)->IsValid() == false`.
    inline Iterator      find(Id id) { return Iterator(static_cast<SOA*>(this), id); }
    inline ConstIterator find(Id id) const
    {
        return ConstIterator(static_cast<const SOA*>(this), id);
    }

    // `Reserve` ensures enough room for *at least* `new_cap` elements
    // (inluding existing elements). This will re-allocate memory if necessary.
    void Reserve(typename Id::basic_type new_cap);

    // `Add` adds a single element and returns an iterator referring to the new
    // element. This will re-allocate memory if necessary
    Iterator Add();

    // `Clear` resets size to 0, but keeps the allocated memory.
    inline void Clear() { m_size = 0; }

protected:
    template<typename CONFIG_> friend class EventStateInfoRefT;
    template<typename CONFIG_> friend class EventStateInfoConstRefT;

    // The start of the array for each field will be aligned to `kAlignment`
    static constexpr size_t kAlignment = alignof(std::max_align_t);

#define PARTIAL_SIZE_EventStateInfo 0u
#define PARTIAL_INDEX_EventStateInfo 0u
    static_assert(alignof(uint32_t) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kTopologyIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kTopologyOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kTopologySize = sizeof(uint32_t);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kTopologyOffset + kTopologySize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kTopologyIndex + 1
    static_assert(alignof(bool) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kPrimRestartEnabledIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kPrimRestartEnabledOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kPrimRestartEnabledSize = sizeof(bool);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kPrimRestartEnabledOffset + kPrimRestartEnabledSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kPrimRestartEnabledIndex + 1
    static_assert(alignof(uint32_t) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kPatchControlPointsIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kPatchControlPointsOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kPatchControlPointsSize = sizeof(uint32_t);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kPatchControlPointsOffset + kPatchControlPointsSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kPatchControlPointsIndex + 1
    static_assert(alignof(VkViewport) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kViewportIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kViewportOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kViewportArrayCount = 16;
    static constexpr size_t   kViewportSize = sizeof(VkViewport) * kViewportArrayCount;
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kViewportOffset + kViewportSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kViewportIndex + kViewportArrayCount
    static_assert(alignof(VkRect2D) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kScissorIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kScissorOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kScissorArrayCount = 16;
    static constexpr size_t   kScissorSize = sizeof(VkRect2D) * kScissorArrayCount;
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kScissorOffset + kScissorSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kScissorIndex + kScissorArrayCount
    static_assert(alignof(bool) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kDepthClampEnabledIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kDepthClampEnabledOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kDepthClampEnabledSize = sizeof(bool);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kDepthClampEnabledOffset + kDepthClampEnabledSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kDepthClampEnabledIndex + 1
    static_assert(alignof(bool) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kRasterizerDiscardEnabledIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kRasterizerDiscardEnabledOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kRasterizerDiscardEnabledSize = sizeof(bool);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kRasterizerDiscardEnabledOffset + kRasterizerDiscardEnabledSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kRasterizerDiscardEnabledIndex + 1
    static_assert(alignof(VkPolygonMode) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kPolygonModeIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kPolygonModeOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kPolygonModeSize = sizeof(VkPolygonMode);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kPolygonModeOffset + kPolygonModeSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kPolygonModeIndex + 1
    static_assert(alignof(VkCullModeFlags) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kCullModeIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kCullModeOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kCullModeSize = sizeof(VkCullModeFlags);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kCullModeOffset + kCullModeSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kCullModeIndex + 1
    static_assert(alignof(VkFrontFace) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kFrontFaceIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kFrontFaceOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kFrontFaceSize = sizeof(VkFrontFace);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kFrontFaceOffset + kFrontFaceSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kFrontFaceIndex + 1
    static_assert(alignof(bool) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kDepthBiasEnabledIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kDepthBiasEnabledOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kDepthBiasEnabledSize = sizeof(bool);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kDepthBiasEnabledOffset + kDepthBiasEnabledSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kDepthBiasEnabledIndex + 1
    static_assert(alignof(float) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kDepthBiasConstantFactorIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kDepthBiasConstantFactorOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kDepthBiasConstantFactorSize = sizeof(float);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kDepthBiasConstantFactorOffset + kDepthBiasConstantFactorSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kDepthBiasConstantFactorIndex + 1
    static_assert(alignof(float) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kDepthBiasClampIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kDepthBiasClampOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kDepthBiasClampSize = sizeof(float);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kDepthBiasClampOffset + kDepthBiasClampSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kDepthBiasClampIndex + 1
    static_assert(alignof(float) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kDepthBiasSlopeFactorIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kDepthBiasSlopeFactorOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kDepthBiasSlopeFactorSize = sizeof(float);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kDepthBiasSlopeFactorOffset + kDepthBiasSlopeFactorSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kDepthBiasSlopeFactorIndex + 1
    static_assert(alignof(float) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kLineWidthIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kLineWidthOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kLineWidthSize = sizeof(float);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kLineWidthOffset + kLineWidthSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kLineWidthIndex + 1
    static_assert(alignof(VkSampleCountFlagBits) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kRasterizationSamplesIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kRasterizationSamplesOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kRasterizationSamplesSize = sizeof(VkSampleCountFlagBits);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kRasterizationSamplesOffset + kRasterizationSamplesSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kRasterizationSamplesIndex + 1
    static_assert(alignof(bool) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kSampleShadingEnabledIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kSampleShadingEnabledOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kSampleShadingEnabledSize = sizeof(bool);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kSampleShadingEnabledOffset + kSampleShadingEnabledSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kSampleShadingEnabledIndex + 1
    static_assert(alignof(float) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kMinSampleShadingIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kMinSampleShadingOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kMinSampleShadingSize = sizeof(float);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kMinSampleShadingOffset + kMinSampleShadingSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kMinSampleShadingIndex + 1
    static_assert(alignof(VkSampleMask) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kSampleMaskIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kSampleMaskOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kSampleMaskSize = sizeof(VkSampleMask);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kSampleMaskOffset + kSampleMaskSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kSampleMaskIndex + 1
    static_assert(alignof(bool) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kAlphaToCoverageEnabledIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kAlphaToCoverageEnabledOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kAlphaToCoverageEnabledSize = sizeof(bool);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kAlphaToCoverageEnabledOffset + kAlphaToCoverageEnabledSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kAlphaToCoverageEnabledIndex + 1
    static_assert(alignof(bool) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kDepthTestEnabledIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kDepthTestEnabledOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kDepthTestEnabledSize = sizeof(bool);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kDepthTestEnabledOffset + kDepthTestEnabledSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kDepthTestEnabledIndex + 1
    static_assert(alignof(bool) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kDepthWriteEnabledIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kDepthWriteEnabledOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kDepthWriteEnabledSize = sizeof(bool);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kDepthWriteEnabledOffset + kDepthWriteEnabledSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kDepthWriteEnabledIndex + 1
    static_assert(alignof(VkCompareOp) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kDepthCompareOpIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kDepthCompareOpOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kDepthCompareOpSize = sizeof(VkCompareOp);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kDepthCompareOpOffset + kDepthCompareOpSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kDepthCompareOpIndex + 1
    static_assert(alignof(bool) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kDepthBoundsTestEnabledIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kDepthBoundsTestEnabledOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kDepthBoundsTestEnabledSize = sizeof(bool);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kDepthBoundsTestEnabledOffset + kDepthBoundsTestEnabledSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kDepthBoundsTestEnabledIndex + 1
    static_assert(alignof(float) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kMinDepthBoundsIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kMinDepthBoundsOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kMinDepthBoundsSize = sizeof(float);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kMinDepthBoundsOffset + kMinDepthBoundsSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kMinDepthBoundsIndex + 1
    static_assert(alignof(float) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kMaxDepthBoundsIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kMaxDepthBoundsOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kMaxDepthBoundsSize = sizeof(float);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kMaxDepthBoundsOffset + kMaxDepthBoundsSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kMaxDepthBoundsIndex + 1
    static_assert(alignof(bool) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kStencilTestEnabledIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kStencilTestEnabledOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kStencilTestEnabledSize = sizeof(bool);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kStencilTestEnabledOffset + kStencilTestEnabledSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kStencilTestEnabledIndex + 1
    static_assert(alignof(VkStencilOpState) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kStencilOpStateFrontIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kStencilOpStateFrontOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kStencilOpStateFrontSize = sizeof(VkStencilOpState);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kStencilOpStateFrontOffset + kStencilOpStateFrontSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kStencilOpStateFrontIndex + 1
    static_assert(alignof(VkStencilOpState) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kStencilOpStateBackIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kStencilOpStateBackOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kStencilOpStateBackSize = sizeof(VkStencilOpState);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kStencilOpStateBackOffset + kStencilOpStateBackSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kStencilOpStateBackIndex + 1
    static_assert(alignof(bool) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kLogicOpEnabledIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kLogicOpEnabledOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kLogicOpEnabledArrayCount = 8;
    static constexpr size_t   kLogicOpEnabledSize = sizeof(bool) * kLogicOpEnabledArrayCount;
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kLogicOpEnabledOffset + kLogicOpEnabledSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kLogicOpEnabledIndex + kLogicOpEnabledArrayCount
    static_assert(alignof(VkLogicOp) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kLogicOpIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kLogicOpOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kLogicOpArrayCount = 8;
    static constexpr size_t   kLogicOpSize = sizeof(VkLogicOp) * kLogicOpArrayCount;
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kLogicOpOffset + kLogicOpSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kLogicOpIndex + kLogicOpArrayCount
    static_assert(alignof(VkPipelineColorBlendAttachmentState) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kAttachmentIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kAttachmentOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kAttachmentArrayCount = 8;
    static constexpr size_t   kAttachmentSize = sizeof(VkPipelineColorBlendAttachmentState) *
                                              kAttachmentArrayCount;
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kAttachmentOffset + kAttachmentSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kAttachmentIndex + kAttachmentArrayCount
    static_assert(alignof(float) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kBlendConstantIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kBlendConstantOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kBlendConstantArrayCount = 4;
    static constexpr size_t   kBlendConstantSize = sizeof(float) * kBlendConstantArrayCount;
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kBlendConstantOffset + kBlendConstantSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kBlendConstantIndex + kBlendConstantArrayCount
    static_assert(alignof(bool) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kLRZEnabledIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kLRZEnabledOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kLRZEnabledSize = sizeof(bool);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kLRZEnabledOffset + kLRZEnabledSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kLRZEnabledIndex + 1
    static_assert(alignof(bool) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kLRZWriteIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kLRZWriteOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kLRZWriteSize = sizeof(bool);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kLRZWriteOffset + kLRZWriteSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kLRZWriteIndex + 1
    static_assert(alignof(a6xx_lrz_dir_status) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kLRZDirStatusIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kLRZDirStatusOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kLRZDirStatusSize = sizeof(a6xx_lrz_dir_status);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kLRZDirStatusOffset + kLRZDirStatusSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kLRZDirStatusIndex + 1
    static_assert(alignof(bool) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kLRZDirWriteIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kLRZDirWriteOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kLRZDirWriteSize = sizeof(bool);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kLRZDirWriteOffset + kLRZDirWriteSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kLRZDirWriteIndex + 1
    static_assert(alignof(a6xx_ztest_mode) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kZTestModeIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kZTestModeOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kZTestModeSize = sizeof(a6xx_ztest_mode);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kZTestModeOffset + kZTestModeSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kZTestModeIndex + 1
    static_assert(alignof(uint32_t) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kBinWIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kBinWOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kBinWSize = sizeof(uint32_t);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kBinWOffset + kBinWSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kBinWIndex + 1
    static_assert(alignof(uint32_t) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kBinHIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kBinHOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kBinHSize = sizeof(uint32_t);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kBinHOffset + kBinHSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kBinHIndex + 1
    static_assert(alignof(uint16_t) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kWindowScissorTLXIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kWindowScissorTLXOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kWindowScissorTLXSize = sizeof(uint16_t);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kWindowScissorTLXOffset + kWindowScissorTLXSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kWindowScissorTLXIndex + 1
    static_assert(alignof(uint16_t) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kWindowScissorTLYIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kWindowScissorTLYOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kWindowScissorTLYSize = sizeof(uint16_t);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kWindowScissorTLYOffset + kWindowScissorTLYSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kWindowScissorTLYIndex + 1
    static_assert(alignof(uint16_t) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kWindowScissorBRXIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kWindowScissorBRXOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kWindowScissorBRXSize = sizeof(uint16_t);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kWindowScissorBRXOffset + kWindowScissorBRXSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kWindowScissorBRXIndex + 1
    static_assert(alignof(uint16_t) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kWindowScissorBRYIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kWindowScissorBRYOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kWindowScissorBRYSize = sizeof(uint16_t);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kWindowScissorBRYOffset + kWindowScissorBRYSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kWindowScissorBRYIndex + 1
    static_assert(alignof(a6xx_render_mode) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kRenderModeIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kRenderModeOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kRenderModeSize = sizeof(a6xx_render_mode);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kRenderModeOffset + kRenderModeSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kRenderModeIndex + 1
    static_assert(alignof(a6xx_buffers_location) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kBuffersLocationIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kBuffersLocationOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kBuffersLocationSize = sizeof(a6xx_buffers_location);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kBuffersLocationOffset + kBuffersLocationSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kBuffersLocationIndex + 1
    static_assert(alignof(a6xx_threadsize) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kThreadSizeIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kThreadSizeOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kThreadSizeSize = sizeof(a6xx_threadsize);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kThreadSizeOffset + kThreadSizeSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kThreadSizeIndex + 1
    static_assert(alignof(bool) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kEnableAllHelperLanesIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kEnableAllHelperLanesOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kEnableAllHelperLanesSize = sizeof(bool);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kEnableAllHelperLanesOffset + kEnableAllHelperLanesSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kEnableAllHelperLanesIndex + 1
    static_assert(alignof(bool) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kEnablePartialHelperLanesIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kEnablePartialHelperLanesOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kEnablePartialHelperLanesSize = sizeof(bool);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kEnablePartialHelperLanesOffset + kEnablePartialHelperLanesSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kEnablePartialHelperLanesIndex + 1
    static_assert(alignof(bool) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kUBWCEnabledIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kUBWCEnabledOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kUBWCEnabledArrayCount = 8;
    static constexpr size_t   kUBWCEnabledSize = sizeof(bool) * kUBWCEnabledArrayCount;
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kUBWCEnabledOffset + kUBWCEnabledSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kUBWCEnabledIndex + kUBWCEnabledArrayCount
    static_assert(alignof(bool) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kUBWCLosslessEnabledIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kUBWCLosslessEnabledOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kUBWCLosslessEnabledArrayCount = 8;
    static constexpr size_t   kUBWCLosslessEnabledSize = sizeof(bool) *
                                                       kUBWCLosslessEnabledArrayCount;
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kUBWCLosslessEnabledOffset + kUBWCLosslessEnabledSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kUBWCLosslessEnabledIndex + kUBWCLosslessEnabledArrayCount
    static_assert(alignof(bool) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kUBWCEnabledOnDSIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kUBWCEnabledOnDSOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kUBWCEnabledOnDSSize = sizeof(bool);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kUBWCEnabledOnDSOffset + kUBWCEnabledOnDSSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kUBWCEnabledOnDSIndex + 1
    static_assert(alignof(bool) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kUBWCLosslessEnabledOnDSIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kUBWCLosslessEnabledOnDSOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kUBWCLosslessEnabledOnDSSize = sizeof(bool);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kUBWCLosslessEnabledOnDSOffset + kUBWCLosslessEnabledOnDSSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kUBWCLosslessEnabledOnDSIndex + 1

    // Number of bytes required to store each element
    static constexpr size_t kElemSize = PARTIAL_SIZE_EventStateInfo;
#undef PARTIAL_SIZE_EventStateInfo

    // Number of fields
    static constexpr size_t kNumFields = PARTIAL_INDEX_EventStateInfo;
#undef PARTIAL_INDEX_EventStateInfo

    // Current number of elements
    typename Id::basic_type m_size = 0;

    // Maximum number of elements before needing to re-allocate
    typename Id::basic_type m_cap = 0;

    // Pointer to the memory storing all of the fields.
    // Stored as a `unique_ptr<max_align_t[]>` because:
    //   - Allocating a `max_align_t` array ensures the buffer is sufficiently
    //     aligned for any field type.
    //   - Using `max_align_t[]` instead of `max_align_t` tells `unique_ptr`
    //     to deallocate using `operator delete []` instead of
    //     `operator delete`, which is required because the memory is allocated
    //     with `operator new []`.
    std::unique_ptr<std::max_align_t[]> m_buffer;

    // Pointer to a bit-array, where each field is marked with a 1 if set, and 0
    // if not set. Unlike m_buffer, this bit-array is in a AOS rather than SOA
    // memory layout. This makes the management of this small buffer simpler.
    std::vector<uint8_t> m_is_set_buffer;

    // The following fields point to each of the arrays. These are not used,
    // but are helpful for debugging, saving you from needing to manually
    // calculate array offsets in `m_buffer`.
    //
    // NOTE: If you are debugging in Visual Studio, you can add the generated
    // ".natvis" file into the Visual Studio project to get an even nicer
    // debug view.
#ifndef NDEBUG
    uint32_t*                            DBG_topology;
    bool*                                DBG_prim_restart_enabled;
    uint32_t*                            DBG_patch_control_points;
    VkViewport*                          DBG_viewport;
    VkRect2D*                            DBG_scissor;
    bool*                                DBG_depth_clamp_enabled;
    bool*                                DBG_rasterizer_discard_enabled;
    VkPolygonMode*                       DBG_polygon_mode;
    VkCullModeFlags*                     DBG_cull_mode;
    VkFrontFace*                         DBG_front_face;
    bool*                                DBG_depth_bias_enabled;
    float*                               DBG_depth_bias_constant_factor;
    float*                               DBG_depth_bias_clamp;
    float*                               DBG_depth_bias_slope_factor;
    float*                               DBG_line_width;
    VkSampleCountFlagBits*               DBG_rasterization_samples;
    bool*                                DBG_sample_shading_enabled;
    float*                               DBG_min_sample_shading;
    VkSampleMask*                        DBG_sample_mask;
    bool*                                DBG_alpha_to_coverage_enabled;
    bool*                                DBG_depth_test_enabled;
    bool*                                DBG_depth_write_enabled;
    VkCompareOp*                         DBG_depth_compare_op;
    bool*                                DBG_depth_bounds_test_enabled;
    float*                               DBG_min_depth_bounds;
    float*                               DBG_max_depth_bounds;
    bool*                                DBG_stencil_test_enabled;
    VkStencilOpState*                    DBG_stencil_op_state_front;
    VkStencilOpState*                    DBG_stencil_op_state_back;
    bool*                                DBG_logic_op_enabled;
    VkLogicOp*                           DBG_logic_op;
    VkPipelineColorBlendAttachmentState* DBG_attachment;
    float*                               DBG_blend_constant;
    bool*                                DBG_lrz_enabled;
    bool*                                DBG_lrz_write;
    a6xx_lrz_dir_status*                 DBG_lrz_dir_status;
    bool*                                DBG_lrz_dir_write;
    a6xx_ztest_mode*                     DBG_z_test_mode;
    uint32_t*                            DBG_bin_w;
    uint32_t*                            DBG_bin_h;
    uint16_t*                            DBG_window_scissor_tlx;
    uint16_t*                            DBG_window_scissor_tly;
    uint16_t*                            DBG_window_scissor_brx;
    uint16_t*                            DBG_window_scissor_bry;
    a6xx_render_mode*                    DBG_render_mode;
    a6xx_buffers_location*               DBG_buffers_location;
    a6xx_threadsize*                     DBG_thread_size;
    bool*                                DBG_enable_all_helper_lanes;
    bool*                                DBG_enable_partial_helper_lanes;
    bool*                                DBG_ubwc_enabled;
    bool*                                DBG_ubwc_lossless_enabled;
    bool*                                DBG_ubwc_enabled_on_ds;
    bool*                                DBG_ubwc_lossless_enabled_on_ds;
#endif
};
class EventStateInfoRef;
class EventStateInfoConstRef;
class EventStateInfo;
struct EventStateInfo_CONFIG
{
    using Id = EventStateId;
    using SOA = EventStateInfo;
    using Ref = EventStateInfoRef;
    using ConstRef = EventStateInfoConstRef;
    using Iterator = StructOfArraysIterator<SOA, Id, Ref>;
    using ConstIterator = StructOfArraysConstIterator<SOA, Id, ConstRef, Ref>;
    using ViewportArray = EventStateInfoViewportArray<EventStateInfo_CONFIG>;
    using ViewportConstArray = EventStateInfoViewportConstArray<EventStateInfo_CONFIG>;
    using ScissorArray = EventStateInfoScissorArray<EventStateInfo_CONFIG>;
    using ScissorConstArray = EventStateInfoScissorConstArray<EventStateInfo_CONFIG>;
    using LogicOpEnabledArray = EventStateInfoLogicOpEnabledArray<EventStateInfo_CONFIG>;
    using LogicOpEnabledConstArray = EventStateInfoLogicOpEnabledConstArray<EventStateInfo_CONFIG>;
    using LogicOpArray = EventStateInfoLogicOpArray<EventStateInfo_CONFIG>;
    using LogicOpConstArray = EventStateInfoLogicOpConstArray<EventStateInfo_CONFIG>;
    using AttachmentArray = EventStateInfoAttachmentArray<EventStateInfo_CONFIG>;
    using AttachmentConstArray = EventStateInfoAttachmentConstArray<EventStateInfo_CONFIG>;
    using BlendConstantArray = EventStateInfoBlendConstantArray<EventStateInfo_CONFIG>;
    using BlendConstantConstArray = EventStateInfoBlendConstantConstArray<EventStateInfo_CONFIG>;
    using UBWCEnabledArray = EventStateInfoUBWCEnabledArray<EventStateInfo_CONFIG>;
    using UBWCEnabledConstArray = EventStateInfoUBWCEnabledConstArray<EventStateInfo_CONFIG>;
    using UBWCLosslessEnabledArray = EventStateInfoUBWCLosslessEnabledArray<EventStateInfo_CONFIG>;
    using UBWCLosslessEnabledConstArray = EventStateInfoUBWCLosslessEnabledConstArray<
    EventStateInfo_CONFIG>;
};
template<>
void EventStateInfoRefT<EventStateInfo_CONFIG>::assign(
const EventStateInfo&                         other_obj,
EventStateInfoRefT<EventStateInfo_CONFIG>::Id other_id) const;
class EventStateInfoConstRef : public EventStateInfoConstRefT<EventStateInfo_CONFIG>
{
public:
    EventStateInfoConstRef() = default;
    EventStateInfoConstRef(const EventStateInfoConstRef& other) = default;
    EventStateInfoConstRef(const EventStateInfoRef& other) :
        EventStateInfoConstRefT<EventStateInfo_CONFIG>(other)
    {
    }
    EventStateInfoConstRef(const EventStateInfo* obj_ptr, Id id) :
        EventStateInfoConstRefT<EventStateInfo_CONFIG>(obj_ptr, id)
    {
    }
};
class EventStateInfoRef : public EventStateInfoRefT<EventStateInfo_CONFIG>
{
public:
    EventStateInfoRef() = default;
    EventStateInfoRef(const EventStateInfoRef& other) = default;
    EventStateInfoRef(EventStateInfo* obj_ptr, Id id) :
        EventStateInfoRefT<EventStateInfo_CONFIG>(obj_ptr, id)
    {
    }
    const EventStateInfoRef& operator=(const EventStateInfoRef& other) const
    {
        assign(other.obj(), other.id());
        return *this;
    }
    const EventStateInfoRef& operator=(const EventStateInfoConstRef& other) const
    {
        assign(other.obj(), other.id());
        return *this;
    }
};
class EventStateInfo : public EventStateInfoT<EventStateInfo_CONFIG>
{};

}  // namespace Dive
