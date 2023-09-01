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

template<typename CONFIG> class EventStateInfoDccEnabledArray
{
public:
    using Id = typename CONFIG::Id;
    using SOA = typename CONFIG::SOA;
    using Ref = typename CONFIG::Ref;
    using DccEnabledArray = typename CONFIG::DccEnabledArray;
    using DccEnabledConstArray = typename CONFIG::DccEnabledConstArray;
    EventStateInfoDccEnabledArray() = default;
    EventStateInfoDccEnabledArray(const EventStateInfoDccEnabledArray& other) :
        m_obj_ptr(other.m_obj_ptr),
        m_id(other.m_id)
    {
    }
    EventStateInfoDccEnabledArray(SOA* obj_ptr, Id id) :
        m_obj_ptr(obj_ptr),
        m_id(id)
    {
    }
    Id   id() const { return m_id; }
    SOA& obj() const { return *m_obj_ptr; }
    bool IsValid() const { return m_obj_ptr != nullptr && m_obj_ptr->IsValidId(m_id); }

    //-----------------------------------------------
    // REF FIELD DccEnabled: Whether DCC-based bandwidth-saving color compression is enabled

    // `Get(uint32_t attachment)` returns the value of the DccEnabled field of the referenced object
    inline bool Get(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->DccEnabled(m_id, attachment);
    }

    // `Set(value)` sets the DccEnabled field of the referenced object
    inline const DccEnabledArray& Set(uint32_t attachment, bool value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetDccEnabled(m_id, attachment, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsDccEnabledSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsDccEnabledSet(m_id, attachment);
    }

    inline const char* GetDccEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDccEnabledName();
    }

    inline const char* GetDccEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDccEnabledDescription();
    }

protected:
    SOA* m_obj_ptr = nullptr;
    Id   m_id;
};

template<typename CONFIG> class EventStateInfoDccEnabledConstArray
{
public:
    using Id = typename CONFIG::Id;
    using SOA = typename CONFIG::SOA;
    using Ref = typename CONFIG::Ref;
    using DccEnabledArray = typename CONFIG::DccEnabledArray;
    using DccEnabledConstArray = typename CONFIG::DccEnabledConstArray;
    EventStateInfoDccEnabledConstArray() = default;
    EventStateInfoDccEnabledConstArray(const EventStateInfoDccEnabledArray<CONFIG>& other) :
        m_obj_ptr(&other.obj()),
        m_id(other.id())
    {
    }
    EventStateInfoDccEnabledConstArray(const EventStateInfoDccEnabledConstArray& other) :
        m_obj_ptr(other.m_obj_ptr),
        m_id(other.m_id)
    {
    }
    EventStateInfoDccEnabledConstArray(const SOA* obj_ptr, Id id) :
        m_obj_ptr(obj_ptr),
        m_id(id)
    {
    }
    Id         id() const { return m_id; }
    const SOA& obj() const { return *m_obj_ptr; }
    bool       IsValid() const { return m_obj_ptr != nullptr && m_obj_ptr->IsValidId(m_id); }

    //-----------------------------------------------
    // REF FIELD DccEnabled: Whether DCC-based bandwidth-saving color compression is enabled

    // `Get(uint32_t attachment)` returns the value of the DccEnabled field of the referenced object
    inline bool Get(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->DccEnabled(m_id, attachment);
    }

    inline bool IsDccEnabledSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsDccEnabledSet(m_id, attachment);
    }

    inline const char* GetDccEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDccEnabledName();
    }

    inline const char* GetDccEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDccEnabledDescription();
    }

protected:
    const SOA* m_obj_ptr = nullptr;
    Id         m_id;
};

template<typename CONFIG> class EventStateInfoColorFormatArray
{
public:
    using Id = typename CONFIG::Id;
    using SOA = typename CONFIG::SOA;
    using Ref = typename CONFIG::Ref;
    using ColorFormatArray = typename CONFIG::ColorFormatArray;
    using ColorFormatConstArray = typename CONFIG::ColorFormatConstArray;
    EventStateInfoColorFormatArray() = default;
    EventStateInfoColorFormatArray(const EventStateInfoColorFormatArray& other) :
        m_obj_ptr(other.m_obj_ptr),
        m_id(other.m_id)
    {
    }
    EventStateInfoColorFormatArray(SOA* obj_ptr, Id id) :
        m_obj_ptr(obj_ptr),
        m_id(id)
    {
    }
    Id   id() const { return m_id; }
    SOA& obj() const { return *m_obj_ptr; }
    bool IsValid() const { return m_obj_ptr != nullptr && m_obj_ptr->IsValidId(m_id); }

    //-----------------------------------------------
    // REF FIELD ColorFormat: Per target attachment hardware color format

    // `Get(uint32_t attachment)` returns the value of the ColorFormat field of the referenced
    // object
    inline Dive::Legacy::ColorFormat Get(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->ColorFormat(m_id, attachment);
    }

    // `Set(value)` sets the ColorFormat field of the referenced object
    inline const ColorFormatArray& Set(uint32_t attachment, Dive::Legacy::ColorFormat value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetColorFormat(m_id, attachment, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsColorFormatSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsColorFormatSet(m_id, attachment);
    }

    inline const char* GetColorFormatName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetColorFormatName();
    }

    inline const char* GetColorFormatDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetColorFormatDescription();
    }

protected:
    SOA* m_obj_ptr = nullptr;
    Id   m_id;
};

template<typename CONFIG> class EventStateInfoColorFormatConstArray
{
public:
    using Id = typename CONFIG::Id;
    using SOA = typename CONFIG::SOA;
    using Ref = typename CONFIG::Ref;
    using ColorFormatArray = typename CONFIG::ColorFormatArray;
    using ColorFormatConstArray = typename CONFIG::ColorFormatConstArray;
    EventStateInfoColorFormatConstArray() = default;
    EventStateInfoColorFormatConstArray(const EventStateInfoColorFormatArray<CONFIG>& other) :
        m_obj_ptr(&other.obj()),
        m_id(other.id())
    {
    }
    EventStateInfoColorFormatConstArray(const EventStateInfoColorFormatConstArray& other) :
        m_obj_ptr(other.m_obj_ptr),
        m_id(other.m_id)
    {
    }
    EventStateInfoColorFormatConstArray(const SOA* obj_ptr, Id id) :
        m_obj_ptr(obj_ptr),
        m_id(id)
    {
    }
    Id         id() const { return m_id; }
    const SOA& obj() const { return *m_obj_ptr; }
    bool       IsValid() const { return m_obj_ptr != nullptr && m_obj_ptr->IsValidId(m_id); }

    //-----------------------------------------------
    // REF FIELD ColorFormat: Per target attachment hardware color format

    // `Get(uint32_t attachment)` returns the value of the ColorFormat field of the referenced
    // object
    inline Dive::Legacy::ColorFormat Get(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->ColorFormat(m_id, attachment);
    }

    inline bool IsColorFormatSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsColorFormatSet(m_id, attachment);
    }

    inline const char* GetColorFormatName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetColorFormatName();
    }

    inline const char* GetColorFormatDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetColorFormatDescription();
    }

protected:
    const SOA* m_obj_ptr = nullptr;
    Id         m_id;
};

template<typename CONFIG> class EventStateInfoMip0HeightArray
{
public:
    using Id = typename CONFIG::Id;
    using SOA = typename CONFIG::SOA;
    using Ref = typename CONFIG::Ref;
    using Mip0HeightArray = typename CONFIG::Mip0HeightArray;
    using Mip0HeightConstArray = typename CONFIG::Mip0HeightConstArray;
    EventStateInfoMip0HeightArray() = default;
    EventStateInfoMip0HeightArray(const EventStateInfoMip0HeightArray& other) :
        m_obj_ptr(other.m_obj_ptr),
        m_id(other.m_id)
    {
    }
    EventStateInfoMip0HeightArray(SOA* obj_ptr, Id id) :
        m_obj_ptr(obj_ptr),
        m_id(id)
    {
    }
    Id   id() const { return m_id; }
    SOA& obj() const { return *m_obj_ptr; }
    bool IsValid() const { return m_obj_ptr != nullptr && m_obj_ptr->IsValidId(m_id); }

    //-----------------------------------------------
    // REF FIELD Mip0Height: Per target attachment mip0 height

    // `Get(uint32_t attachment)` returns the value of the Mip0Height field of the referenced object
    inline uint32_t Get(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->Mip0Height(m_id, attachment);
    }

    // `Set(value)` sets the Mip0Height field of the referenced object
    inline const Mip0HeightArray& Set(uint32_t attachment, uint32_t value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetMip0Height(m_id, attachment, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsMip0HeightSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsMip0HeightSet(m_id, attachment);
    }

    inline const char* GetMip0HeightName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetMip0HeightName();
    }

    inline const char* GetMip0HeightDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetMip0HeightDescription();
    }

    inline uint32_t Min() const
    {
        uint32_t res = std::numeric_limits<uint32_t>::max();
        for (uint32_t attachment = 0; attachment < 8; ++attachment)
        {
            auto value = m_obj_ptr->Mip0Height(m_id, attachment);
            res = std::min<uint32_t>(res, value);
        }

        return res;
    }
    inline uint32_t Max() const
    {
        uint32_t res = std::numeric_limits<uint32_t>::min();
        for (uint32_t attachment = 0; attachment < 8; ++attachment)
        {
            auto value = m_obj_ptr->Mip0Height(m_id, attachment);
            res = std::max<uint32_t>(res, value);
        }

        return res;
    }
    inline uint32_t Sum() const
    {
        uint32_t res = 0;
        for (uint32_t attachment = 0; attachment < 8; ++attachment)
        {
            auto value = m_obj_ptr->Mip0Height(m_id, attachment);
            res = res + value;
        }

        return res;
    }
    inline uint32_t Avg() const
    {
        uint32_t res = 0;
        uint32_t count = 0;
        for (uint32_t attachment = 0; attachment < 8; ++attachment)
        {
            auto value = m_obj_ptr->Mip0Height(m_id, attachment);
            res = res + value;
            ++count;
        }

        return res / count;
    }

protected:
    SOA* m_obj_ptr = nullptr;
    Id   m_id;
};

template<typename CONFIG> class EventStateInfoMip0HeightConstArray
{
public:
    using Id = typename CONFIG::Id;
    using SOA = typename CONFIG::SOA;
    using Ref = typename CONFIG::Ref;
    using Mip0HeightArray = typename CONFIG::Mip0HeightArray;
    using Mip0HeightConstArray = typename CONFIG::Mip0HeightConstArray;
    EventStateInfoMip0HeightConstArray() = default;
    EventStateInfoMip0HeightConstArray(const EventStateInfoMip0HeightArray<CONFIG>& other) :
        m_obj_ptr(&other.obj()),
        m_id(other.id())
    {
    }
    EventStateInfoMip0HeightConstArray(const EventStateInfoMip0HeightConstArray& other) :
        m_obj_ptr(other.m_obj_ptr),
        m_id(other.m_id)
    {
    }
    EventStateInfoMip0HeightConstArray(const SOA* obj_ptr, Id id) :
        m_obj_ptr(obj_ptr),
        m_id(id)
    {
    }
    Id         id() const { return m_id; }
    const SOA& obj() const { return *m_obj_ptr; }
    bool       IsValid() const { return m_obj_ptr != nullptr && m_obj_ptr->IsValidId(m_id); }

    //-----------------------------------------------
    // REF FIELD Mip0Height: Per target attachment mip0 height

    // `Get(uint32_t attachment)` returns the value of the Mip0Height field of the referenced object
    inline uint32_t Get(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->Mip0Height(m_id, attachment);
    }

    inline bool IsMip0HeightSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsMip0HeightSet(m_id, attachment);
    }

    inline const char* GetMip0HeightName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetMip0HeightName();
    }

    inline const char* GetMip0HeightDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetMip0HeightDescription();
    }

    inline uint32_t Min() const
    {
        uint32_t res = std::numeric_limits<uint32_t>::max();
        for (uint32_t attachment = 0; attachment < 8; ++attachment)
        {
            auto value = m_obj_ptr->Mip0Height(m_id, attachment);
            res = std::min<uint32_t>(res, value);
        }

        return res;
    }
    inline uint32_t Max() const
    {
        uint32_t res = std::numeric_limits<uint32_t>::min();
        for (uint32_t attachment = 0; attachment < 8; ++attachment)
        {
            auto value = m_obj_ptr->Mip0Height(m_id, attachment);
            res = std::max<uint32_t>(res, value);
        }

        return res;
    }
    inline uint32_t Sum() const
    {
        uint32_t res = 0;
        for (uint32_t attachment = 0; attachment < 8; ++attachment)
        {
            auto value = m_obj_ptr->Mip0Height(m_id, attachment);
            res = res + value;
        }

        return res;
    }
    inline uint32_t Avg() const
    {
        uint32_t res = 0;
        uint32_t count = 0;
        for (uint32_t attachment = 0; attachment < 8; ++attachment)
        {
            auto value = m_obj_ptr->Mip0Height(m_id, attachment);
            res = res + value;
            ++count;
        }

        return res / count;
    }

protected:
    const SOA* m_obj_ptr = nullptr;
    Id         m_id;
};

template<typename CONFIG> class EventStateInfoMip0WidthArray
{
public:
    using Id = typename CONFIG::Id;
    using SOA = typename CONFIG::SOA;
    using Ref = typename CONFIG::Ref;
    using Mip0WidthArray = typename CONFIG::Mip0WidthArray;
    using Mip0WidthConstArray = typename CONFIG::Mip0WidthConstArray;
    EventStateInfoMip0WidthArray() = default;
    EventStateInfoMip0WidthArray(const EventStateInfoMip0WidthArray& other) :
        m_obj_ptr(other.m_obj_ptr),
        m_id(other.m_id)
    {
    }
    EventStateInfoMip0WidthArray(SOA* obj_ptr, Id id) :
        m_obj_ptr(obj_ptr),
        m_id(id)
    {
    }
    Id   id() const { return m_id; }
    SOA& obj() const { return *m_obj_ptr; }
    bool IsValid() const { return m_obj_ptr != nullptr && m_obj_ptr->IsValidId(m_id); }

    //-----------------------------------------------
    // REF FIELD Mip0Width: Per target attachment mip0 width

    // `Get(uint32_t attachment)` returns the value of the Mip0Width field of the referenced object
    inline uint32_t Get(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->Mip0Width(m_id, attachment);
    }

    // `Set(value)` sets the Mip0Width field of the referenced object
    inline const Mip0WidthArray& Set(uint32_t attachment, uint32_t value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetMip0Width(m_id, attachment, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsMip0WidthSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsMip0WidthSet(m_id, attachment);
    }

    inline const char* GetMip0WidthName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetMip0WidthName();
    }

    inline const char* GetMip0WidthDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetMip0WidthDescription();
    }

    inline uint32_t Min() const
    {
        uint32_t res = std::numeric_limits<uint32_t>::max();
        for (uint32_t attachment = 0; attachment < 8; ++attachment)
        {
            auto value = m_obj_ptr->Mip0Width(m_id, attachment);
            res = std::min<uint32_t>(res, value);
        }

        return res;
    }
    inline uint32_t Max() const
    {
        uint32_t res = std::numeric_limits<uint32_t>::min();
        for (uint32_t attachment = 0; attachment < 8; ++attachment)
        {
            auto value = m_obj_ptr->Mip0Width(m_id, attachment);
            res = std::max<uint32_t>(res, value);
        }

        return res;
    }
    inline uint32_t Sum() const
    {
        uint32_t res = 0;
        for (uint32_t attachment = 0; attachment < 8; ++attachment)
        {
            auto value = m_obj_ptr->Mip0Width(m_id, attachment);
            res = res + value;
        }

        return res;
    }
    inline uint32_t Avg() const
    {
        uint32_t res = 0;
        uint32_t count = 0;
        for (uint32_t attachment = 0; attachment < 8; ++attachment)
        {
            auto value = m_obj_ptr->Mip0Width(m_id, attachment);
            res = res + value;
            ++count;
        }

        return res / count;
    }

protected:
    SOA* m_obj_ptr = nullptr;
    Id   m_id;
};

template<typename CONFIG> class EventStateInfoMip0WidthConstArray
{
public:
    using Id = typename CONFIG::Id;
    using SOA = typename CONFIG::SOA;
    using Ref = typename CONFIG::Ref;
    using Mip0WidthArray = typename CONFIG::Mip0WidthArray;
    using Mip0WidthConstArray = typename CONFIG::Mip0WidthConstArray;
    EventStateInfoMip0WidthConstArray() = default;
    EventStateInfoMip0WidthConstArray(const EventStateInfoMip0WidthArray<CONFIG>& other) :
        m_obj_ptr(&other.obj()),
        m_id(other.id())
    {
    }
    EventStateInfoMip0WidthConstArray(const EventStateInfoMip0WidthConstArray& other) :
        m_obj_ptr(other.m_obj_ptr),
        m_id(other.m_id)
    {
    }
    EventStateInfoMip0WidthConstArray(const SOA* obj_ptr, Id id) :
        m_obj_ptr(obj_ptr),
        m_id(id)
    {
    }
    Id         id() const { return m_id; }
    const SOA& obj() const { return *m_obj_ptr; }
    bool       IsValid() const { return m_obj_ptr != nullptr && m_obj_ptr->IsValidId(m_id); }

    //-----------------------------------------------
    // REF FIELD Mip0Width: Per target attachment mip0 width

    // `Get(uint32_t attachment)` returns the value of the Mip0Width field of the referenced object
    inline uint32_t Get(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->Mip0Width(m_id, attachment);
    }

    inline bool IsMip0WidthSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsMip0WidthSet(m_id, attachment);
    }

    inline const char* GetMip0WidthName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetMip0WidthName();
    }

    inline const char* GetMip0WidthDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetMip0WidthDescription();
    }

    inline uint32_t Min() const
    {
        uint32_t res = std::numeric_limits<uint32_t>::max();
        for (uint32_t attachment = 0; attachment < 8; ++attachment)
        {
            auto value = m_obj_ptr->Mip0Width(m_id, attachment);
            res = std::min<uint32_t>(res, value);
        }

        return res;
    }
    inline uint32_t Max() const
    {
        uint32_t res = std::numeric_limits<uint32_t>::min();
        for (uint32_t attachment = 0; attachment < 8; ++attachment)
        {
            auto value = m_obj_ptr->Mip0Width(m_id, attachment);
            res = std::max<uint32_t>(res, value);
        }

        return res;
    }
    inline uint32_t Sum() const
    {
        uint32_t res = 0;
        for (uint32_t attachment = 0; attachment < 8; ++attachment)
        {
            auto value = m_obj_ptr->Mip0Width(m_id, attachment);
            res = res + value;
        }

        return res;
    }
    inline uint32_t Avg() const
    {
        uint32_t res = 0;
        uint32_t count = 0;
        for (uint32_t attachment = 0; attachment < 8; ++attachment)
        {
            auto value = m_obj_ptr->Mip0Width(m_id, attachment);
            res = res + value;
            ++count;
        }

        return res / count;
    }

protected:
    const SOA* m_obj_ptr = nullptr;
    Id         m_id;
};

template<typename CONFIG> class EventStateInfoVgprArray
{
public:
    using Id = typename CONFIG::Id;
    using SOA = typename CONFIG::SOA;
    using Ref = typename CONFIG::Ref;
    using VgprArray = typename CONFIG::VgprArray;
    using VgprConstArray = typename CONFIG::VgprConstArray;
    EventStateInfoVgprArray() = default;
    EventStateInfoVgprArray(const EventStateInfoVgprArray& other) :
        m_obj_ptr(other.m_obj_ptr),
        m_id(other.m_id)
    {
    }
    EventStateInfoVgprArray(SOA* obj_ptr, Id id) :
        m_obj_ptr(obj_ptr),
        m_id(id)
    {
    }
    Id   id() const { return m_id; }
    SOA& obj() const { return *m_obj_ptr; }
    bool IsValid() const { return m_obj_ptr != nullptr && m_obj_ptr->IsValidId(m_id); }

    //-----------------------------------------------
    // REF FIELD Vgpr: Per shader stage vector general purpose register count. Always rounded up to
    // nearest multiple of 4.

    // `Get(Dive::ShaderStage stage)` returns the value of the Vgpr field of the referenced object
    inline uint16_t Get(Dive::ShaderStage stage) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->Vgpr(m_id, stage);
    }

    // `Set(value)` sets the Vgpr field of the referenced object
    inline const VgprArray& Set(Dive::ShaderStage stage, uint16_t value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetVgpr(m_id, stage, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsVgprSet(Dive::ShaderStage stage) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsVgprSet(m_id, stage);
    }

    inline const char* GetVgprName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetVgprName();
    }

    inline const char* GetVgprDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetVgprDescription();
    }

protected:
    SOA* m_obj_ptr = nullptr;
    Id   m_id;
};

template<typename CONFIG> class EventStateInfoVgprConstArray
{
public:
    using Id = typename CONFIG::Id;
    using SOA = typename CONFIG::SOA;
    using Ref = typename CONFIG::Ref;
    using VgprArray = typename CONFIG::VgprArray;
    using VgprConstArray = typename CONFIG::VgprConstArray;
    EventStateInfoVgprConstArray() = default;
    EventStateInfoVgprConstArray(const EventStateInfoVgprArray<CONFIG>& other) :
        m_obj_ptr(&other.obj()),
        m_id(other.id())
    {
    }
    EventStateInfoVgprConstArray(const EventStateInfoVgprConstArray& other) :
        m_obj_ptr(other.m_obj_ptr),
        m_id(other.m_id)
    {
    }
    EventStateInfoVgprConstArray(const SOA* obj_ptr, Id id) :
        m_obj_ptr(obj_ptr),
        m_id(id)
    {
    }
    Id         id() const { return m_id; }
    const SOA& obj() const { return *m_obj_ptr; }
    bool       IsValid() const { return m_obj_ptr != nullptr && m_obj_ptr->IsValidId(m_id); }

    //-----------------------------------------------
    // REF FIELD Vgpr: Per shader stage vector general purpose register count. Always rounded up to
    // nearest multiple of 4.

    // `Get(Dive::ShaderStage stage)` returns the value of the Vgpr field of the referenced object
    inline uint16_t Get(Dive::ShaderStage stage) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->Vgpr(m_id, stage);
    }

    inline bool IsVgprSet(Dive::ShaderStage stage) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsVgprSet(m_id, stage);
    }

    inline const char* GetVgprName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetVgprName();
    }

    inline const char* GetVgprDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetVgprDescription();
    }

protected:
    const SOA* m_obj_ptr = nullptr;
    Id         m_id;
};

template<typename CONFIG> class EventStateInfoSgprArray
{
public:
    using Id = typename CONFIG::Id;
    using SOA = typename CONFIG::SOA;
    using Ref = typename CONFIG::Ref;
    using SgprArray = typename CONFIG::SgprArray;
    using SgprConstArray = typename CONFIG::SgprConstArray;
    EventStateInfoSgprArray() = default;
    EventStateInfoSgprArray(const EventStateInfoSgprArray& other) :
        m_obj_ptr(other.m_obj_ptr),
        m_id(other.m_id)
    {
    }
    EventStateInfoSgprArray(SOA* obj_ptr, Id id) :
        m_obj_ptr(obj_ptr),
        m_id(id)
    {
    }
    Id   id() const { return m_id; }
    SOA& obj() const { return *m_obj_ptr; }
    bool IsValid() const { return m_obj_ptr != nullptr && m_obj_ptr->IsValidId(m_id); }

    //-----------------------------------------------
    // REF FIELD Sgpr: Per shader stage scalar general purpose register count. Always rounded up to
    // nearest multiple of 16

    // `Get(Dive::ShaderStage stage)` returns the value of the Sgpr field of the referenced object
    inline uint16_t Get(Dive::ShaderStage stage) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->Sgpr(m_id, stage);
    }

    // `Set(value)` sets the Sgpr field of the referenced object
    inline const SgprArray& Set(Dive::ShaderStage stage, uint16_t value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetSgpr(m_id, stage, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsSgprSet(Dive::ShaderStage stage) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsSgprSet(m_id, stage);
    }

    inline const char* GetSgprName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetSgprName();
    }

    inline const char* GetSgprDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetSgprDescription();
    }

protected:
    SOA* m_obj_ptr = nullptr;
    Id   m_id;
};

template<typename CONFIG> class EventStateInfoSgprConstArray
{
public:
    using Id = typename CONFIG::Id;
    using SOA = typename CONFIG::SOA;
    using Ref = typename CONFIG::Ref;
    using SgprArray = typename CONFIG::SgprArray;
    using SgprConstArray = typename CONFIG::SgprConstArray;
    EventStateInfoSgprConstArray() = default;
    EventStateInfoSgprConstArray(const EventStateInfoSgprArray<CONFIG>& other) :
        m_obj_ptr(&other.obj()),
        m_id(other.id())
    {
    }
    EventStateInfoSgprConstArray(const EventStateInfoSgprConstArray& other) :
        m_obj_ptr(other.m_obj_ptr),
        m_id(other.m_id)
    {
    }
    EventStateInfoSgprConstArray(const SOA* obj_ptr, Id id) :
        m_obj_ptr(obj_ptr),
        m_id(id)
    {
    }
    Id         id() const { return m_id; }
    const SOA& obj() const { return *m_obj_ptr; }
    bool       IsValid() const { return m_obj_ptr != nullptr && m_obj_ptr->IsValidId(m_id); }

    //-----------------------------------------------
    // REF FIELD Sgpr: Per shader stage scalar general purpose register count. Always rounded up to
    // nearest multiple of 16

    // `Get(Dive::ShaderStage stage)` returns the value of the Sgpr field of the referenced object
    inline uint16_t Get(Dive::ShaderStage stage) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->Sgpr(m_id, stage);
    }

    inline bool IsSgprSet(Dive::ShaderStage stage) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsSgprSet(m_id, stage);
    }

    inline const char* GetSgprName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetSgprName();
    }

    inline const char* GetSgprDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetSgprDescription();
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
    using AttachmentArray = typename CONFIG::AttachmentArray;
    using AttachmentConstArray = typename CONFIG::AttachmentConstArray;
    using BlendConstantArray = typename CONFIG::BlendConstantArray;
    using BlendConstantConstArray = typename CONFIG::BlendConstantConstArray;
    using DccEnabledArray = typename CONFIG::DccEnabledArray;
    using DccEnabledConstArray = typename CONFIG::DccEnabledConstArray;
    using ColorFormatArray = typename CONFIG::ColorFormatArray;
    using ColorFormatConstArray = typename CONFIG::ColorFormatConstArray;
    using Mip0HeightArray = typename CONFIG::Mip0HeightArray;
    using Mip0HeightConstArray = typename CONFIG::Mip0HeightConstArray;
    using Mip0WidthArray = typename CONFIG::Mip0WidthArray;
    using Mip0WidthConstArray = typename CONFIG::Mip0WidthConstArray;
    using VgprArray = typename CONFIG::VgprArray;
    using VgprConstArray = typename CONFIG::VgprConstArray;
    using SgprArray = typename CONFIG::SgprArray;
    using SgprConstArray = typename CONFIG::SgprConstArray;
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
    // REF FIELD LogicOpEnabled: Whether to apply Logical Operations

    // `LogicOpEnabled()` returns the value of the LogicOpEnabled field of the referenced object
    inline bool LogicOpEnabled() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->LogicOpEnabled(m_id);
    }

    // `SetLogicOpEnabled(value)` sets the LogicOpEnabled field of the referenced object
    inline const Ref& SetLogicOpEnabled(bool value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetLogicOpEnabled(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsLogicOpEnabledSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsLogicOpEnabledSet(m_id);
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

    // `LogicOp()` returns the value of the LogicOp field of the referenced object
    inline VkLogicOp LogicOp() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->LogicOp(m_id);
    }

    // `SetLogicOp(value)` sets the LogicOp field of the referenced object
    inline const Ref& SetLogicOp(VkLogicOp value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetLogicOp(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsLogicOpSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsLogicOpSet(m_id);
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
    // REF FIELD ZAddr: The read and write VA of depth buffer (assumed to be same)

    // `ZAddr()` returns the value of the ZAddr field of the referenced object
    inline uint64_t ZAddr() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->ZAddr(m_id);
    }

    // `SetZAddr(value)` sets the ZAddr field of the referenced object
    inline const Ref& SetZAddr(uint64_t value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetZAddr(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsZAddrSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsZAddrSet(m_id);
    }

    inline const char* GetZAddrName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetZAddrName();
    }

    inline const char* GetZAddrDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetZAddrDescription();
    }

    //-----------------------------------------------
    // REF FIELD HTileAddr: VA of DB's HTile buffer

    // `HTileAddr()` returns the value of the HTileAddr field of the referenced object
    inline uint64_t HTileAddr() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->HTileAddr(m_id);
    }

    // `SetHTileAddr(value)` sets the HTileAddr field of the referenced object
    inline const Ref& SetHTileAddr(uint64_t value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetHTileAddr(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsHTileAddrSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsHTileAddrSet(m_id);
    }

    inline const char* GetHTileAddrName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetHTileAddrName();
    }

    inline const char* GetHTileAddrDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetHTileAddrDescription();
    }

    //-----------------------------------------------
    // REF FIELD HiZEnabled: Whether Hi-Z is enabled for depth

    // `HiZEnabled()` returns the value of the HiZEnabled field of the referenced object
    inline bool HiZEnabled() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->HiZEnabled(m_id);
    }

    // `SetHiZEnabled(value)` sets the HiZEnabled field of the referenced object
    inline const Ref& SetHiZEnabled(bool value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetHiZEnabled(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsHiZEnabledSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsHiZEnabledSet(m_id);
    }

    inline const char* GetHiZEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetHiZEnabledName();
    }

    inline const char* GetHiZEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetHiZEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD HiSEnabled: Whether Hi-S is enabled for stencil

    // `HiSEnabled()` returns the value of the HiSEnabled field of the referenced object
    inline bool HiSEnabled() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->HiSEnabled(m_id);
    }

    // `SetHiSEnabled(value)` sets the HiSEnabled field of the referenced object
    inline const Ref& SetHiSEnabled(bool value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetHiSEnabled(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsHiSEnabledSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsHiSEnabledSet(m_id);
    }

    inline const char* GetHiSEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetHiSEnabledName();
    }

    inline const char* GetHiSEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetHiSEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD ZCompressEnabled: Whether plane compression is enabled for depth buffers. To reduce
    // bandwidth impact

    // `ZCompressEnabled()` returns the value of the ZCompressEnabled field of the referenced object
    inline bool ZCompressEnabled() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->ZCompressEnabled(m_id);
    }

    // `SetZCompressEnabled(value)` sets the ZCompressEnabled field of the referenced object
    inline const Ref& SetZCompressEnabled(bool value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetZCompressEnabled(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsZCompressEnabledSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsZCompressEnabledSet(m_id);
    }

    inline const char* GetZCompressEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetZCompressEnabledName();
    }

    inline const char* GetZCompressEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetZCompressEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD StencilCompressEnabled: Whether compression is enabled for stencil buffers. To
    // reduce bandwidth impact

    // `StencilCompressEnabled()` returns the value of the StencilCompressEnabled field of the
    // referenced object
    inline bool StencilCompressEnabled() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->StencilCompressEnabled(m_id);
    }

    // `SetStencilCompressEnabled(value)` sets the StencilCompressEnabled field of the referenced
    // object
    inline const Ref& SetStencilCompressEnabled(bool value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetStencilCompressEnabled(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsStencilCompressEnabledSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsStencilCompressEnabledSet(m_id);
    }

    inline const char* GetStencilCompressEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetStencilCompressEnabledName();
    }

    inline const char* GetStencilCompressEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetStencilCompressEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD CompressedZFetchEnabled: Whether shader fetch of compressed depth buffers is
    // enabled

    // `CompressedZFetchEnabled()` returns the value of the CompressedZFetchEnabled field of the
    // referenced object
    inline bool CompressedZFetchEnabled() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->CompressedZFetchEnabled(m_id);
    }

    // `SetCompressedZFetchEnabled(value)` sets the CompressedZFetchEnabled field of the referenced
    // object
    inline const Ref& SetCompressedZFetchEnabled(bool value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetCompressedZFetchEnabled(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsCompressedZFetchEnabledSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsCompressedZFetchEnabledSet(m_id);
    }

    inline const char* GetCompressedZFetchEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetCompressedZFetchEnabledName();
    }

    inline const char* GetCompressedZFetchEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetCompressedZFetchEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD ZFormat: Internal GPU format of the depth buffer

    // `ZFormat()` returns the value of the ZFormat field of the referenced object
    inline Dive::Legacy::ZFormat ZFormat() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->ZFormat(m_id);
    }

    // `SetZFormat(value)` sets the ZFormat field of the referenced object
    inline const Ref& SetZFormat(Dive::Legacy::ZFormat value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetZFormat(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsZFormatSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsZFormatSet(m_id);
    }

    inline const char* GetZFormatName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetZFormatName();
    }

    inline const char* GetZFormatDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetZFormatDescription();
    }

    //-----------------------------------------------
    // REF FIELD ZOrder: Indicates application preference for LateZ, EarlyZ, or ReZ

    // `ZOrder()` returns the value of the ZOrder field of the referenced object
    inline Dive::Legacy::ZOrder ZOrder() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->ZOrder(m_id);
    }

    // `SetZOrder(value)` sets the ZOrder field of the referenced object
    inline const Ref& SetZOrder(Dive::Legacy::ZOrder value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetZOrder(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsZOrderSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsZOrderSet(m_id);
    }

    inline const char* GetZOrderName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetZOrderName();
    }

    inline const char* GetZOrderDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetZOrderDescription();
    }

    //-----------------------------------------------
    // REF FIELD VSLateAlloc: Late VS wavefront allocation count. Value is the number of wavefronts
    // minus one, since at least one VS wave can always launch with late alloc enabled

    // `VSLateAlloc()` returns the value of the VSLateAlloc field of the referenced object
    inline uint16_t VSLateAlloc() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->VSLateAlloc(m_id);
    }

    // `SetVSLateAlloc(value)` sets the VSLateAlloc field of the referenced object
    inline const Ref& SetVSLateAlloc(uint16_t value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetVSLateAlloc(m_id, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsVSLateAllocSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsVSLateAllocSet(m_id);
    }

    inline const char* GetVSLateAllocName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetVSLateAllocName();
    }

    inline const char* GetVSLateAllocDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetVSLateAllocDescription();
    }

    //-----------------------------------------------
    // REF FIELD DccEnabled: Whether DCC-based bandwidth-saving color compression is enabled

    // `DccEnabled(uint32_t attachment)` returns the value of the DccEnabled field of the referenced
    // object
    inline bool DccEnabled(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->DccEnabled(m_id, attachment);
    }

    // `DccEnabled()` returns the array of values of the DccEnabled field of the referenced object
    inline DccEnabledArray DccEnabled() const { return DccEnabledArray(m_obj_ptr, m_id); }

    // `SetDccEnabled(value)` sets the DccEnabled field of the referenced object
    inline const Ref& SetDccEnabled(uint32_t attachment, bool value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetDccEnabled(m_id, attachment, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsDccEnabledSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsDccEnabledSet(m_id, attachment);
    }

    inline const char* GetDccEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDccEnabledName();
    }

    inline const char* GetDccEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDccEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD ColorFormat: Per target attachment hardware color format

    // `ColorFormat(uint32_t attachment)` returns the value of the ColorFormat field of the
    // referenced object
    inline Dive::Legacy::ColorFormat ColorFormat(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->ColorFormat(m_id, attachment);
    }

    // `ColorFormat()` returns the array of values of the ColorFormat field of the referenced object
    inline ColorFormatArray ColorFormat() const { return ColorFormatArray(m_obj_ptr, m_id); }

    // `SetColorFormat(value)` sets the ColorFormat field of the referenced object
    inline const Ref& SetColorFormat(uint32_t attachment, Dive::Legacy::ColorFormat value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetColorFormat(m_id, attachment, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsColorFormatSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsColorFormatSet(m_id, attachment);
    }

    inline const char* GetColorFormatName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetColorFormatName();
    }

    inline const char* GetColorFormatDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetColorFormatDescription();
    }

    //-----------------------------------------------
    // REF FIELD Mip0Height: Per target attachment mip0 height

    // `Mip0Height(uint32_t attachment)` returns the value of the Mip0Height field of the referenced
    // object
    inline uint32_t Mip0Height(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->Mip0Height(m_id, attachment);
    }

    // `Mip0Height()` returns the array of values of the Mip0Height field of the referenced object
    inline Mip0HeightArray Mip0Height() const { return Mip0HeightArray(m_obj_ptr, m_id); }

    // `SetMip0Height(value)` sets the Mip0Height field of the referenced object
    inline const Ref& SetMip0Height(uint32_t attachment, uint32_t value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetMip0Height(m_id, attachment, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsMip0HeightSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsMip0HeightSet(m_id, attachment);
    }

    inline const char* GetMip0HeightName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetMip0HeightName();
    }

    inline const char* GetMip0HeightDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetMip0HeightDescription();
    }

    //-----------------------------------------------
    // REF FIELD Mip0Width: Per target attachment mip0 width

    // `Mip0Width(uint32_t attachment)` returns the value of the Mip0Width field of the referenced
    // object
    inline uint32_t Mip0Width(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->Mip0Width(m_id, attachment);
    }

    // `Mip0Width()` returns the array of values of the Mip0Width field of the referenced object
    inline Mip0WidthArray Mip0Width() const { return Mip0WidthArray(m_obj_ptr, m_id); }

    // `SetMip0Width(value)` sets the Mip0Width field of the referenced object
    inline const Ref& SetMip0Width(uint32_t attachment, uint32_t value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetMip0Width(m_id, attachment, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsMip0WidthSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsMip0WidthSet(m_id, attachment);
    }

    inline const char* GetMip0WidthName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetMip0WidthName();
    }

    inline const char* GetMip0WidthDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetMip0WidthDescription();
    }

    //-----------------------------------------------
    // REF FIELD Vgpr: Per shader stage vector general purpose register count. Always rounded up to
    // nearest multiple of 4.

    // `Vgpr(Dive::ShaderStage stage)` returns the value of the Vgpr field of the referenced object
    inline uint16_t Vgpr(Dive::ShaderStage stage) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->Vgpr(m_id, stage);
    }

    // `Vgpr()` returns the array of values of the Vgpr field of the referenced object
    inline VgprArray Vgpr() const { return VgprArray(m_obj_ptr, m_id); }

    // `SetVgpr(value)` sets the Vgpr field of the referenced object
    inline const Ref& SetVgpr(Dive::ShaderStage stage, uint16_t value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetVgpr(m_id, stage, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsVgprSet(Dive::ShaderStage stage) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsVgprSet(m_id, stage);
    }

    inline const char* GetVgprName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetVgprName();
    }

    inline const char* GetVgprDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetVgprDescription();
    }

    //-----------------------------------------------
    // REF FIELD Sgpr: Per shader stage scalar general purpose register count. Always rounded up to
    // nearest multiple of 16

    // `Sgpr(Dive::ShaderStage stage)` returns the value of the Sgpr field of the referenced object
    inline uint16_t Sgpr(Dive::ShaderStage stage) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->Sgpr(m_id, stage);
    }

    // `Sgpr()` returns the array of values of the Sgpr field of the referenced object
    inline SgprArray Sgpr() const { return SgprArray(m_obj_ptr, m_id); }

    // `SetSgpr(value)` sets the Sgpr field of the referenced object
    inline const Ref& SetSgpr(Dive::ShaderStage stage, uint16_t value) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        m_obj_ptr->SetSgpr(m_id, stage, value);
        return static_cast<const Ref&>(*this);
    }

    inline bool IsSgprSet(Dive::ShaderStage stage) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsSgprSet(m_id, stage);
    }

    inline const char* GetSgprName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetSgprName();
    }

    inline const char* GetSgprDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetSgprDescription();
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
    using AttachmentArray = typename CONFIG::AttachmentArray;
    using AttachmentConstArray = typename CONFIG::AttachmentConstArray;
    using BlendConstantArray = typename CONFIG::BlendConstantArray;
    using BlendConstantConstArray = typename CONFIG::BlendConstantConstArray;
    using DccEnabledArray = typename CONFIG::DccEnabledArray;
    using DccEnabledConstArray = typename CONFIG::DccEnabledConstArray;
    using ColorFormatArray = typename CONFIG::ColorFormatArray;
    using ColorFormatConstArray = typename CONFIG::ColorFormatConstArray;
    using Mip0HeightArray = typename CONFIG::Mip0HeightArray;
    using Mip0HeightConstArray = typename CONFIG::Mip0HeightConstArray;
    using Mip0WidthArray = typename CONFIG::Mip0WidthArray;
    using Mip0WidthConstArray = typename CONFIG::Mip0WidthConstArray;
    using VgprArray = typename CONFIG::VgprArray;
    using VgprConstArray = typename CONFIG::VgprConstArray;
    using SgprArray = typename CONFIG::SgprArray;
    using SgprConstArray = typename CONFIG::SgprConstArray;
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
    // REF FIELD LogicOpEnabled: Whether to apply Logical Operations

    // `LogicOpEnabled()` returns the value of the LogicOpEnabled field of the referenced object
    inline bool LogicOpEnabled() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->LogicOpEnabled(m_id);
    }

    inline bool IsLogicOpEnabledSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsLogicOpEnabledSet(m_id);
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

    // `LogicOp()` returns the value of the LogicOp field of the referenced object
    inline VkLogicOp LogicOp() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->LogicOp(m_id);
    }

    inline bool IsLogicOpSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsLogicOpSet(m_id);
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
    // REF FIELD ZAddr: The read and write VA of depth buffer (assumed to be same)

    // `ZAddr()` returns the value of the ZAddr field of the referenced object
    inline uint64_t ZAddr() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->ZAddr(m_id);
    }

    inline bool IsZAddrSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsZAddrSet(m_id);
    }

    inline const char* GetZAddrName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetZAddrName();
    }

    inline const char* GetZAddrDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetZAddrDescription();
    }

    //-----------------------------------------------
    // REF FIELD HTileAddr: VA of DB's HTile buffer

    // `HTileAddr()` returns the value of the HTileAddr field of the referenced object
    inline uint64_t HTileAddr() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->HTileAddr(m_id);
    }

    inline bool IsHTileAddrSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsHTileAddrSet(m_id);
    }

    inline const char* GetHTileAddrName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetHTileAddrName();
    }

    inline const char* GetHTileAddrDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetHTileAddrDescription();
    }

    //-----------------------------------------------
    // REF FIELD HiZEnabled: Whether Hi-Z is enabled for depth

    // `HiZEnabled()` returns the value of the HiZEnabled field of the referenced object
    inline bool HiZEnabled() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->HiZEnabled(m_id);
    }

    inline bool IsHiZEnabledSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsHiZEnabledSet(m_id);
    }

    inline const char* GetHiZEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetHiZEnabledName();
    }

    inline const char* GetHiZEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetHiZEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD HiSEnabled: Whether Hi-S is enabled for stencil

    // `HiSEnabled()` returns the value of the HiSEnabled field of the referenced object
    inline bool HiSEnabled() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->HiSEnabled(m_id);
    }

    inline bool IsHiSEnabledSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsHiSEnabledSet(m_id);
    }

    inline const char* GetHiSEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetHiSEnabledName();
    }

    inline const char* GetHiSEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetHiSEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD ZCompressEnabled: Whether plane compression is enabled for depth buffers. To reduce
    // bandwidth impact

    // `ZCompressEnabled()` returns the value of the ZCompressEnabled field of the referenced object
    inline bool ZCompressEnabled() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->ZCompressEnabled(m_id);
    }

    inline bool IsZCompressEnabledSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsZCompressEnabledSet(m_id);
    }

    inline const char* GetZCompressEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetZCompressEnabledName();
    }

    inline const char* GetZCompressEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetZCompressEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD StencilCompressEnabled: Whether compression is enabled for stencil buffers. To
    // reduce bandwidth impact

    // `StencilCompressEnabled()` returns the value of the StencilCompressEnabled field of the
    // referenced object
    inline bool StencilCompressEnabled() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->StencilCompressEnabled(m_id);
    }

    inline bool IsStencilCompressEnabledSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsStencilCompressEnabledSet(m_id);
    }

    inline const char* GetStencilCompressEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetStencilCompressEnabledName();
    }

    inline const char* GetStencilCompressEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetStencilCompressEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD CompressedZFetchEnabled: Whether shader fetch of compressed depth buffers is
    // enabled

    // `CompressedZFetchEnabled()` returns the value of the CompressedZFetchEnabled field of the
    // referenced object
    inline bool CompressedZFetchEnabled() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->CompressedZFetchEnabled(m_id);
    }

    inline bool IsCompressedZFetchEnabledSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsCompressedZFetchEnabledSet(m_id);
    }

    inline const char* GetCompressedZFetchEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetCompressedZFetchEnabledName();
    }

    inline const char* GetCompressedZFetchEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetCompressedZFetchEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD ZFormat: Internal GPU format of the depth buffer

    // `ZFormat()` returns the value of the ZFormat field of the referenced object
    inline Dive::Legacy::ZFormat ZFormat() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->ZFormat(m_id);
    }

    inline bool IsZFormatSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsZFormatSet(m_id);
    }

    inline const char* GetZFormatName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetZFormatName();
    }

    inline const char* GetZFormatDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetZFormatDescription();
    }

    //-----------------------------------------------
    // REF FIELD ZOrder: Indicates application preference for LateZ, EarlyZ, or ReZ

    // `ZOrder()` returns the value of the ZOrder field of the referenced object
    inline Dive::Legacy::ZOrder ZOrder() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->ZOrder(m_id);
    }

    inline bool IsZOrderSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsZOrderSet(m_id);
    }

    inline const char* GetZOrderName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetZOrderName();
    }

    inline const char* GetZOrderDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetZOrderDescription();
    }

    //-----------------------------------------------
    // REF FIELD VSLateAlloc: Late VS wavefront allocation count. Value is the number of wavefronts
    // minus one, since at least one VS wave can always launch with late alloc enabled

    // `VSLateAlloc()` returns the value of the VSLateAlloc field of the referenced object
    inline uint16_t VSLateAlloc() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->VSLateAlloc(m_id);
    }

    inline bool IsVSLateAllocSet() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsVSLateAllocSet(m_id);
    }

    inline const char* GetVSLateAllocName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetVSLateAllocName();
    }

    inline const char* GetVSLateAllocDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetVSLateAllocDescription();
    }

    //-----------------------------------------------
    // REF FIELD DccEnabled: Whether DCC-based bandwidth-saving color compression is enabled

    // `DccEnabled(uint32_t attachment)` returns the value of the DccEnabled field of the referenced
    // object
    inline bool DccEnabled(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->DccEnabled(m_id, attachment);
    }

    // `DccEnabled()` returns the array of values of the DccEnabled field of the referenced object
    inline DccEnabledConstArray DccEnabled() const { return DccEnabledConstArray(m_obj_ptr, m_id); }

    inline bool IsDccEnabledSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsDccEnabledSet(m_id, attachment);
    }

    inline const char* GetDccEnabledName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDccEnabledName();
    }

    inline const char* GetDccEnabledDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetDccEnabledDescription();
    }

    //-----------------------------------------------
    // REF FIELD ColorFormat: Per target attachment hardware color format

    // `ColorFormat(uint32_t attachment)` returns the value of the ColorFormat field of the
    // referenced object
    inline Dive::Legacy::ColorFormat ColorFormat(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->ColorFormat(m_id, attachment);
    }

    // `ColorFormat()` returns the array of values of the ColorFormat field of the referenced object
    inline ColorFormatConstArray ColorFormat() const
    {
        return ColorFormatConstArray(m_obj_ptr, m_id);
    }

    inline bool IsColorFormatSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsColorFormatSet(m_id, attachment);
    }

    inline const char* GetColorFormatName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetColorFormatName();
    }

    inline const char* GetColorFormatDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetColorFormatDescription();
    }

    //-----------------------------------------------
    // REF FIELD Mip0Height: Per target attachment mip0 height

    // `Mip0Height(uint32_t attachment)` returns the value of the Mip0Height field of the referenced
    // object
    inline uint32_t Mip0Height(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->Mip0Height(m_id, attachment);
    }

    // `Mip0Height()` returns the array of values of the Mip0Height field of the referenced object
    inline Mip0HeightConstArray Mip0Height() const { return Mip0HeightConstArray(m_obj_ptr, m_id); }

    inline bool IsMip0HeightSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsMip0HeightSet(m_id, attachment);
    }

    inline const char* GetMip0HeightName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetMip0HeightName();
    }

    inline const char* GetMip0HeightDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetMip0HeightDescription();
    }

    //-----------------------------------------------
    // REF FIELD Mip0Width: Per target attachment mip0 width

    // `Mip0Width(uint32_t attachment)` returns the value of the Mip0Width field of the referenced
    // object
    inline uint32_t Mip0Width(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->Mip0Width(m_id, attachment);
    }

    // `Mip0Width()` returns the array of values of the Mip0Width field of the referenced object
    inline Mip0WidthConstArray Mip0Width() const { return Mip0WidthConstArray(m_obj_ptr, m_id); }

    inline bool IsMip0WidthSet(uint32_t attachment) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsMip0WidthSet(m_id, attachment);
    }

    inline const char* GetMip0WidthName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetMip0WidthName();
    }

    inline const char* GetMip0WidthDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetMip0WidthDescription();
    }

    //-----------------------------------------------
    // REF FIELD Vgpr: Per shader stage vector general purpose register count. Always rounded up to
    // nearest multiple of 4.

    // `Vgpr(Dive::ShaderStage stage)` returns the value of the Vgpr field of the referenced object
    inline uint16_t Vgpr(Dive::ShaderStage stage) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->Vgpr(m_id, stage);
    }

    // `Vgpr()` returns the array of values of the Vgpr field of the referenced object
    inline VgprConstArray Vgpr() const { return VgprConstArray(m_obj_ptr, m_id); }

    inline bool IsVgprSet(Dive::ShaderStage stage) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsVgprSet(m_id, stage);
    }

    inline const char* GetVgprName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetVgprName();
    }

    inline const char* GetVgprDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetVgprDescription();
    }

    //-----------------------------------------------
    // REF FIELD Sgpr: Per shader stage scalar general purpose register count. Always rounded up to
    // nearest multiple of 16

    // `Sgpr(Dive::ShaderStage stage)` returns the value of the Sgpr field of the referenced object
    inline uint16_t Sgpr(Dive::ShaderStage stage) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->Sgpr(m_id, stage);
    }

    // `Sgpr()` returns the array of values of the Sgpr field of the referenced object
    inline SgprConstArray Sgpr() const { return SgprConstArray(m_obj_ptr, m_id); }

    inline bool IsSgprSet(Dive::ShaderStage stage) const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->IsSgprSet(m_id, stage);
    }

    inline const char* GetSgprName() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetSgprName();
    }

    inline const char* GetSgprDescription() const
    {
        DIVE_ASSERT(m_obj_ptr != nullptr);
        return m_obj_ptr->GetSgprDescription();
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
    using AttachmentArray = typename CONFIG::AttachmentArray;
    using AttachmentConstArray = typename CONFIG::AttachmentConstArray;
    using BlendConstantArray = typename CONFIG::BlendConstantArray;
    using BlendConstantConstArray = typename CONFIG::BlendConstantConstArray;
    using DccEnabledArray = typename CONFIG::DccEnabledArray;
    using DccEnabledConstArray = typename CONFIG::DccEnabledConstArray;
    using ColorFormatArray = typename CONFIG::ColorFormatArray;
    using ColorFormatConstArray = typename CONFIG::ColorFormatConstArray;
    using Mip0HeightArray = typename CONFIG::Mip0HeightArray;
    using Mip0HeightConstArray = typename CONFIG::Mip0HeightConstArray;
    using Mip0WidthArray = typename CONFIG::Mip0WidthArray;
    using Mip0WidthConstArray = typename CONFIG::Mip0WidthConstArray;
    using VgprArray = typename CONFIG::VgprArray;
    using VgprConstArray = typename CONFIG::VgprConstArray;
    using SgprArray = typename CONFIG::SgprArray;
    using SgprConstArray = typename CONFIG::SgprConstArray;

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
    inline const bool* LogicOpEnabledPtr(Id id) const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kLogicOpEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline bool* LogicOpEnabledPtr(Id id)
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kLogicOpEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `LogicOpEnabled(id)` retuns the `LogicOpEnabled` element of the object identified by `id`
    inline bool LogicOpEnabled(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *LogicOpEnabledPtr(id);
    }

    // `SetLogicOpEnabled(id,value)` sets the `LogicOpEnabled` element of the object identified by
    // `id`
    inline SOA& SetLogicOpEnabled(Id id, bool value)
    {
        DIVE_ASSERT(IsValidId(id));
        *LogicOpEnabledPtr(id) = value;
        MarkFieldSet(id, kLogicOpEnabledIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsLogicOpEnabledSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kLogicOpEnabledIndex);
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
    inline const VkLogicOp* LogicOpPtr(Id id) const
    {
        return reinterpret_cast<VkLogicOp*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                            kLogicOpOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline VkLogicOp* LogicOpPtr(Id id)
    {
        return reinterpret_cast<VkLogicOp*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                            kLogicOpOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `LogicOp(id)` retuns the `LogicOp` element of the object identified by `id`
    inline VkLogicOp LogicOp(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *LogicOpPtr(id);
    }

    // `SetLogicOp(id,value)` sets the `LogicOp` element of the object identified by `id`
    inline SOA& SetLogicOp(Id id, VkLogicOp value)
    {
        DIVE_ASSERT(IsValidId(id));
        *LogicOpPtr(id) = value;
        MarkFieldSet(id, kLogicOpIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsLogicOpSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kLogicOpIndex);
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
    // FIELD ZAddr: The read and write VA of depth buffer (assumed to be same)

    // `ZAddrPtr()` returns a shared pointer to an array of `size()` elements
    inline const uint64_t* ZAddrPtr() const
    {
        return reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kZAddrOffset * m_cap);
    }
    inline uint64_t* ZAddrPtr()
    {
        return reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kZAddrOffset * m_cap);
    }
    // `ZAddrPtr()` returns a shared pointer to an array of `size()` elements
    inline const uint64_t* ZAddrPtr(Id id) const
    {
        return reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kZAddrOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline uint64_t* ZAddrPtr(Id id)
    {
        return reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kZAddrOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `ZAddr(id)` retuns the `ZAddr` element of the object identified by `id`
    inline uint64_t ZAddr(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *ZAddrPtr(id);
    }

    // `SetZAddr(id,value)` sets the `ZAddr` element of the object identified by `id`
    inline SOA& SetZAddr(Id id, uint64_t value)
    {
        DIVE_ASSERT(IsValidId(id));
        *ZAddrPtr(id) = value;
        MarkFieldSet(id, kZAddrIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsZAddrSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kZAddrIndex);
    }

    inline const char* GetZAddrName() const { return "ZAddr"; }

    inline const char* GetZAddrDescription() const
    {
        return "The read and write VA of depth buffer (assumed to be same)";
    }

    //-----------------------------------------------
    // FIELD HTileAddr: VA of DB's HTile buffer

    // `HTileAddrPtr()` returns a shared pointer to an array of `size()` elements
    inline const uint64_t* HTileAddrPtr() const
    {
        return reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kHTileAddrOffset * m_cap);
    }
    inline uint64_t* HTileAddrPtr()
    {
        return reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kHTileAddrOffset * m_cap);
    }
    // `HTileAddrPtr()` returns a shared pointer to an array of `size()` elements
    inline const uint64_t* HTileAddrPtr(Id id) const
    {
        return reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kHTileAddrOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline uint64_t* HTileAddrPtr(Id id)
    {
        return reinterpret_cast<uint64_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kHTileAddrOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `HTileAddr(id)` retuns the `HTileAddr` element of the object identified by `id`
    inline uint64_t HTileAddr(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *HTileAddrPtr(id);
    }

    // `SetHTileAddr(id,value)` sets the `HTileAddr` element of the object identified by `id`
    inline SOA& SetHTileAddr(Id id, uint64_t value)
    {
        DIVE_ASSERT(IsValidId(id));
        *HTileAddrPtr(id) = value;
        MarkFieldSet(id, kHTileAddrIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsHTileAddrSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kHTileAddrIndex);
    }

    inline const char* GetHTileAddrName() const { return "HTileAddr"; }

    inline const char* GetHTileAddrDescription() const { return "VA of DB's HTile buffer"; }

    //-----------------------------------------------
    // FIELD HiZEnabled: Whether Hi-Z is enabled for depth

    // `HiZEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* HiZEnabledPtr() const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kHiZEnabledOffset * m_cap);
    }
    inline bool* HiZEnabledPtr()
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kHiZEnabledOffset * m_cap);
    }
    // `HiZEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* HiZEnabledPtr(Id id) const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kHiZEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline bool* HiZEnabledPtr(Id id)
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kHiZEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `HiZEnabled(id)` retuns the `HiZEnabled` element of the object identified by `id`
    inline bool HiZEnabled(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *HiZEnabledPtr(id);
    }

    // `SetHiZEnabled(id,value)` sets the `HiZEnabled` element of the object identified by `id`
    inline SOA& SetHiZEnabled(Id id, bool value)
    {
        DIVE_ASSERT(IsValidId(id));
        *HiZEnabledPtr(id) = value;
        MarkFieldSet(id, kHiZEnabledIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsHiZEnabledSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kHiZEnabledIndex);
    }

    inline const char* GetHiZEnabledName() const { return "HiZEnabled"; }

    inline const char* GetHiZEnabledDescription() const
    {
        return "Whether Hi-Z is enabled for depth";
    }

    //-----------------------------------------------
    // FIELD HiSEnabled: Whether Hi-S is enabled for stencil

    // `HiSEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* HiSEnabledPtr() const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kHiSEnabledOffset * m_cap);
    }
    inline bool* HiSEnabledPtr()
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kHiSEnabledOffset * m_cap);
    }
    // `HiSEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* HiSEnabledPtr(Id id) const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kHiSEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline bool* HiSEnabledPtr(Id id)
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kHiSEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `HiSEnabled(id)` retuns the `HiSEnabled` element of the object identified by `id`
    inline bool HiSEnabled(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *HiSEnabledPtr(id);
    }

    // `SetHiSEnabled(id,value)` sets the `HiSEnabled` element of the object identified by `id`
    inline SOA& SetHiSEnabled(Id id, bool value)
    {
        DIVE_ASSERT(IsValidId(id));
        *HiSEnabledPtr(id) = value;
        MarkFieldSet(id, kHiSEnabledIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsHiSEnabledSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kHiSEnabledIndex);
    }

    inline const char* GetHiSEnabledName() const { return "HiSEnabled"; }

    inline const char* GetHiSEnabledDescription() const
    {
        return "Whether Hi-S is enabled for stencil";
    }

    //-----------------------------------------------
    // FIELD ZCompressEnabled: Whether plane compression is enabled for depth buffers. To reduce
    // bandwidth impact

    // `ZCompressEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* ZCompressEnabledPtr() const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kZCompressEnabledOffset * m_cap);
    }
    inline bool* ZCompressEnabledPtr()
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kZCompressEnabledOffset * m_cap);
    }
    // `ZCompressEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* ZCompressEnabledPtr(Id id) const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kZCompressEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline bool* ZCompressEnabledPtr(Id id)
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kZCompressEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `ZCompressEnabled(id)` retuns the `ZCompressEnabled` element of the object identified by `id`
    inline bool ZCompressEnabled(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *ZCompressEnabledPtr(id);
    }

    // `SetZCompressEnabled(id,value)` sets the `ZCompressEnabled` element of the object identified
    // by `id`
    inline SOA& SetZCompressEnabled(Id id, bool value)
    {
        DIVE_ASSERT(IsValidId(id));
        *ZCompressEnabledPtr(id) = value;
        MarkFieldSet(id, kZCompressEnabledIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsZCompressEnabledSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kZCompressEnabledIndex);
    }

    inline const char* GetZCompressEnabledName() const { return "ZCompressEnabled"; }

    inline const char* GetZCompressEnabledDescription() const
    {
        return "Whether plane compression is enabled for depth buffers. To reduce bandwidth impact";
    }

    //-----------------------------------------------
    // FIELD StencilCompressEnabled: Whether compression is enabled for stencil buffers. To reduce
    // bandwidth impact

    // `StencilCompressEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* StencilCompressEnabledPtr() const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kStencilCompressEnabledOffset * m_cap);
    }
    inline bool* StencilCompressEnabledPtr()
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kStencilCompressEnabledOffset * m_cap);
    }
    // `StencilCompressEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* StencilCompressEnabledPtr(Id id) const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kStencilCompressEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline bool* StencilCompressEnabledPtr(Id id)
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kStencilCompressEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `StencilCompressEnabled(id)` retuns the `StencilCompressEnabled` element of the object
    // identified by `id`
    inline bool StencilCompressEnabled(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *StencilCompressEnabledPtr(id);
    }

    // `SetStencilCompressEnabled(id,value)` sets the `StencilCompressEnabled` element of the object
    // identified by `id`
    inline SOA& SetStencilCompressEnabled(Id id, bool value)
    {
        DIVE_ASSERT(IsValidId(id));
        *StencilCompressEnabledPtr(id) = value;
        MarkFieldSet(id, kStencilCompressEnabledIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsStencilCompressEnabledSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kStencilCompressEnabledIndex);
    }

    inline const char* GetStencilCompressEnabledName() const { return "StencilCompressEnabled"; }

    inline const char* GetStencilCompressEnabledDescription() const
    {
        return "Whether compression is enabled for stencil buffers. To reduce bandwidth impact";
    }

    //-----------------------------------------------
    // FIELD CompressedZFetchEnabled: Whether shader fetch of compressed depth buffers is enabled

    // `CompressedZFetchEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* CompressedZFetchEnabledPtr() const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kCompressedZFetchEnabledOffset * m_cap);
    }
    inline bool* CompressedZFetchEnabledPtr()
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kCompressedZFetchEnabledOffset * m_cap);
    }
    // `CompressedZFetchEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* CompressedZFetchEnabledPtr(Id id) const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kCompressedZFetchEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline bool* CompressedZFetchEnabledPtr(Id id)
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kCompressedZFetchEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `CompressedZFetchEnabled(id)` retuns the `CompressedZFetchEnabled` element of the object
    // identified by `id`
    inline bool CompressedZFetchEnabled(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *CompressedZFetchEnabledPtr(id);
    }

    // `SetCompressedZFetchEnabled(id,value)` sets the `CompressedZFetchEnabled` element of the
    // object identified by `id`
    inline SOA& SetCompressedZFetchEnabled(Id id, bool value)
    {
        DIVE_ASSERT(IsValidId(id));
        *CompressedZFetchEnabledPtr(id) = value;
        MarkFieldSet(id, kCompressedZFetchEnabledIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsCompressedZFetchEnabledSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kCompressedZFetchEnabledIndex);
    }

    inline const char* GetCompressedZFetchEnabledName() const { return "CompressedZFetchEnabled"; }

    inline const char* GetCompressedZFetchEnabledDescription() const
    {
        return "Whether shader fetch of compressed depth buffers is enabled";
    }

    //-----------------------------------------------
    // FIELD ZFormat: Internal GPU format of the depth buffer

    // `ZFormatPtr()` returns a shared pointer to an array of `size()` elements
    inline const Dive::Legacy::ZFormat* ZFormatPtr() const
    {
        return reinterpret_cast<Dive::Legacy::ZFormat*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                        kZFormatOffset * m_cap);
    }
    inline Dive::Legacy::ZFormat* ZFormatPtr()
    {
        return reinterpret_cast<Dive::Legacy::ZFormat*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                        kZFormatOffset * m_cap);
    }
    // `ZFormatPtr()` returns a shared pointer to an array of `size()` elements
    inline const Dive::Legacy::ZFormat* ZFormatPtr(Id id) const
    {
        return reinterpret_cast<Dive::Legacy::ZFormat*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                        kZFormatOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline Dive::Legacy::ZFormat* ZFormatPtr(Id id)
    {
        return reinterpret_cast<Dive::Legacy::ZFormat*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                        kZFormatOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `ZFormat(id)` retuns the `ZFormat` element of the object identified by `id`
    inline Dive::Legacy::ZFormat ZFormat(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *ZFormatPtr(id);
    }

    // `SetZFormat(id,value)` sets the `ZFormat` element of the object identified by `id`
    inline SOA& SetZFormat(Id id, Dive::Legacy::ZFormat value)
    {
        DIVE_ASSERT(IsValidId(id));
        *ZFormatPtr(id) = value;
        MarkFieldSet(id, kZFormatIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsZFormatSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kZFormatIndex);
    }

    inline const char* GetZFormatName() const { return "ZFormat"; }

    inline const char* GetZFormatDescription() const
    {
        return "Internal GPU format of the depth buffer";
    }

    //-----------------------------------------------
    // FIELD ZOrder: Indicates application preference for LateZ, EarlyZ, or ReZ

    // `ZOrderPtr()` returns a shared pointer to an array of `size()` elements
    inline const Dive::Legacy::ZOrder* ZOrderPtr() const
    {
        return reinterpret_cast<Dive::Legacy::ZOrder*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                       kZOrderOffset * m_cap);
    }
    inline Dive::Legacy::ZOrder* ZOrderPtr()
    {
        return reinterpret_cast<Dive::Legacy::ZOrder*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                       kZOrderOffset * m_cap);
    }
    // `ZOrderPtr()` returns a shared pointer to an array of `size()` elements
    inline const Dive::Legacy::ZOrder* ZOrderPtr(Id id) const
    {
        return reinterpret_cast<Dive::Legacy::ZOrder*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                       kZOrderOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline Dive::Legacy::ZOrder* ZOrderPtr(Id id)
    {
        return reinterpret_cast<Dive::Legacy::ZOrder*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                                       kZOrderOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `ZOrder(id)` retuns the `ZOrder` element of the object identified by `id`
    inline Dive::Legacy::ZOrder ZOrder(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *ZOrderPtr(id);
    }

    // `SetZOrder(id,value)` sets the `ZOrder` element of the object identified by `id`
    inline SOA& SetZOrder(Id id, Dive::Legacy::ZOrder value)
    {
        DIVE_ASSERT(IsValidId(id));
        *ZOrderPtr(id) = value;
        MarkFieldSet(id, kZOrderIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsZOrderSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kZOrderIndex);
    }

    inline const char* GetZOrderName() const { return "ZOrder"; }

    inline const char* GetZOrderDescription() const
    {
        return "Indicates application preference for LateZ, EarlyZ, or ReZ";
    }

    //-----------------------------------------------
    // FIELD VSLateAlloc: Late VS wavefront allocation count. Value is the number of wavefronts
    // minus one, since at least one VS wave can always launch with late alloc enabled

    // `VSLateAllocPtr()` returns a shared pointer to an array of `size()` elements
    inline const uint16_t* VSLateAllocPtr() const
    {
        return reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kVSLateAllocOffset * m_cap);
    }
    inline uint16_t* VSLateAllocPtr()
    {
        return reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kVSLateAllocOffset * m_cap);
    }
    // `VSLateAllocPtr()` returns a shared pointer to an array of `size()` elements
    inline const uint16_t* VSLateAllocPtr(Id id) const
    {
        return reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kVSLateAllocOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    inline uint16_t* VSLateAllocPtr(Id id)
    {
        return reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kVSLateAllocOffset * m_cap) +
               static_cast<typename Id::basic_type>(id)

        ;
    }
    // `VSLateAlloc(id)` retuns the `VSLateAlloc` element of the object identified by `id`
    inline uint16_t VSLateAlloc(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *VSLateAllocPtr(id);
    }

    // `SetVSLateAlloc(id,value)` sets the `VSLateAlloc` element of the object identified by `id`
    inline SOA& SetVSLateAlloc(Id id, uint16_t value)
    {
        DIVE_ASSERT(IsValidId(id));
        *VSLateAllocPtr(id) = value;
        MarkFieldSet(id, kVSLateAllocIndex);
        return static_cast<SOA&>(*this);
    }

    inline bool IsVSLateAllocSet(Id id) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kVSLateAllocIndex);
    }

    inline const char* GetVSLateAllocName() const { return "VSLateAlloc"; }

    inline const char* GetVSLateAllocDescription() const
    {
        return "Late VS wavefront allocation count. Value is the number of wavefronts minus one, "
               "since at least one VS wave can always launch with late alloc enabled";
    }

    //-----------------------------------------------
    // FIELD DccEnabled: Whether DCC-based bandwidth-saving color compression is enabled

    // `DccEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* DccEnabledPtr() const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kDccEnabledOffset * m_cap);
    }
    inline bool* DccEnabledPtr()
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kDccEnabledOffset * m_cap);
    }
    // `DccEnabledPtr()` returns a shared pointer to an array of `size()` elements
    inline const bool* DccEnabledPtr(Id id, uint32_t attachment = 0) const
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kDccEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id) * 8 + attachment

        ;
    }
    inline bool* DccEnabledPtr(Id id, uint32_t attachment = 0)
    {
        return reinterpret_cast<bool*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                       kDccEnabledOffset * m_cap) +
               static_cast<typename Id::basic_type>(id) * 8 + attachment

        ;
    }
    // `DccEnabled(id)` retuns the `DccEnabled` element of the object identified by `id`
    inline bool DccEnabled(Id id, uint32_t attachment) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *DccEnabledPtr(id, attachment);
    }

    // `DccEnabled(id)` returns the array of values of the DccEnabled field of the object identified
    // by `id`
    inline DccEnabledArray DccEnabled(Id id)
    {
        return DccEnabledArray(static_cast<SOA*>(this), id);
    }
    inline DccEnabledConstArray DccEnabled(Id id) const
    {
        return DccEnabledConstArray(static_cast<const SOA*>(this), id);
    }

    // `SetDccEnabled(id,value)` sets the `DccEnabled` element of the object identified by `id`
    inline SOA& SetDccEnabled(Id id, uint32_t attachment, bool value)
    {
        DIVE_ASSERT(IsValidId(id));
        *DccEnabledPtr(id, attachment) = value;
        MarkFieldSet(id, kDccEnabledIndex + attachment);
        return static_cast<SOA&>(*this);
    }

    inline bool IsDccEnabledSet(Id id, uint32_t attachment) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kDccEnabledIndex + attachment);
    }

    inline const char* GetDccEnabledName() const { return "DccEnabled"; }

    inline const char* GetDccEnabledDescription() const
    {
        return "Whether DCC-based bandwidth-saving color compression is enabled";
    }

    //-----------------------------------------------
    // FIELD ColorFormat: Per target attachment hardware color format

    // `ColorFormatPtr()` returns a shared pointer to an array of `size()` elements
    inline const Dive::Legacy::ColorFormat* ColorFormatPtr() const
    {
        return reinterpret_cast<Dive::Legacy::ColorFormat*>(
        reinterpret_cast<uint8_t*>(m_buffer.get()) + kColorFormatOffset * m_cap);
    }
    inline Dive::Legacy::ColorFormat* ColorFormatPtr()
    {
        return reinterpret_cast<Dive::Legacy::ColorFormat*>(
        reinterpret_cast<uint8_t*>(m_buffer.get()) + kColorFormatOffset * m_cap);
    }
    // `ColorFormatPtr()` returns a shared pointer to an array of `size()` elements
    inline const Dive::Legacy::ColorFormat* ColorFormatPtr(Id id, uint32_t attachment = 0) const
    {
        return reinterpret_cast<Dive::Legacy::ColorFormat*>(
               reinterpret_cast<uint8_t*>(m_buffer.get()) + kColorFormatOffset * m_cap) +
               static_cast<typename Id::basic_type>(id) * 8 + attachment

        ;
    }
    inline Dive::Legacy::ColorFormat* ColorFormatPtr(Id id, uint32_t attachment = 0)
    {
        return reinterpret_cast<Dive::Legacy::ColorFormat*>(
               reinterpret_cast<uint8_t*>(m_buffer.get()) + kColorFormatOffset * m_cap) +
               static_cast<typename Id::basic_type>(id) * 8 + attachment

        ;
    }
    // `ColorFormat(id)` retuns the `ColorFormat` element of the object identified by `id`
    inline Dive::Legacy::ColorFormat ColorFormat(Id id, uint32_t attachment) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *ColorFormatPtr(id, attachment);
    }

    // `ColorFormat(id)` returns the array of values of the ColorFormat field of the object
    // identified by `id`
    inline ColorFormatArray ColorFormat(Id id)
    {
        return ColorFormatArray(static_cast<SOA*>(this), id);
    }
    inline ColorFormatConstArray ColorFormat(Id id) const
    {
        return ColorFormatConstArray(static_cast<const SOA*>(this), id);
    }

    // `SetColorFormat(id,value)` sets the `ColorFormat` element of the object identified by `id`
    inline SOA& SetColorFormat(Id id, uint32_t attachment, Dive::Legacy::ColorFormat value)
    {
        DIVE_ASSERT(IsValidId(id));
        *ColorFormatPtr(id, attachment) = value;
        MarkFieldSet(id, kColorFormatIndex + attachment);
        return static_cast<SOA&>(*this);
    }

    inline bool IsColorFormatSet(Id id, uint32_t attachment) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kColorFormatIndex + attachment);
    }

    inline const char* GetColorFormatName() const { return "ColorFormat"; }

    inline const char* GetColorFormatDescription() const
    {
        return "Per target attachment hardware color format";
    }

    //-----------------------------------------------
    // FIELD Mip0Height: Per target attachment mip0 height

    // `Mip0HeightPtr()` returns a shared pointer to an array of `size()` elements
    inline const uint32_t* Mip0HeightPtr() const
    {
        return reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kMip0HeightOffset * m_cap);
    }
    inline uint32_t* Mip0HeightPtr()
    {
        return reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kMip0HeightOffset * m_cap);
    }
    // `Mip0HeightPtr()` returns a shared pointer to an array of `size()` elements
    inline const uint32_t* Mip0HeightPtr(Id id, uint32_t attachment = 0) const
    {
        return reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kMip0HeightOffset * m_cap) +
               static_cast<typename Id::basic_type>(id) * 8 + attachment

        ;
    }
    inline uint32_t* Mip0HeightPtr(Id id, uint32_t attachment = 0)
    {
        return reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kMip0HeightOffset * m_cap) +
               static_cast<typename Id::basic_type>(id) * 8 + attachment

        ;
    }
    // `Mip0Height(id)` retuns the `Mip0Height` element of the object identified by `id`
    inline uint32_t Mip0Height(Id id, uint32_t attachment) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *Mip0HeightPtr(id, attachment);
    }

    // `Mip0Height(id)` returns the array of values of the Mip0Height field of the object identified
    // by `id`
    inline Mip0HeightArray Mip0Height(Id id)
    {
        return Mip0HeightArray(static_cast<SOA*>(this), id);
    }
    inline Mip0HeightConstArray Mip0Height(Id id) const
    {
        return Mip0HeightConstArray(static_cast<const SOA*>(this), id);
    }

    // `SetMip0Height(id,value)` sets the `Mip0Height` element of the object identified by `id`
    inline SOA& SetMip0Height(Id id, uint32_t attachment, uint32_t value)
    {
        DIVE_ASSERT(IsValidId(id));
        *Mip0HeightPtr(id, attachment) = value;
        MarkFieldSet(id, kMip0HeightIndex + attachment);
        return static_cast<SOA&>(*this);
    }

    inline bool IsMip0HeightSet(Id id, uint32_t attachment) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kMip0HeightIndex + attachment);
    }

    inline const char* GetMip0HeightName() const { return "Mip0Height"; }

    inline const char* GetMip0HeightDescription() const
    {
        return "Per target attachment mip0 height";
    }

    //-----------------------------------------------
    // FIELD Mip0Width: Per target attachment mip0 width

    // `Mip0WidthPtr()` returns a shared pointer to an array of `size()` elements
    inline const uint32_t* Mip0WidthPtr() const
    {
        return reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kMip0WidthOffset * m_cap);
    }
    inline uint32_t* Mip0WidthPtr()
    {
        return reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kMip0WidthOffset * m_cap);
    }
    // `Mip0WidthPtr()` returns a shared pointer to an array of `size()` elements
    inline const uint32_t* Mip0WidthPtr(Id id, uint32_t attachment = 0) const
    {
        return reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kMip0WidthOffset * m_cap) +
               static_cast<typename Id::basic_type>(id) * 8 + attachment

        ;
    }
    inline uint32_t* Mip0WidthPtr(Id id, uint32_t attachment = 0)
    {
        return reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kMip0WidthOffset * m_cap) +
               static_cast<typename Id::basic_type>(id) * 8 + attachment

        ;
    }
    // `Mip0Width(id)` retuns the `Mip0Width` element of the object identified by `id`
    inline uint32_t Mip0Width(Id id, uint32_t attachment) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *Mip0WidthPtr(id, attachment);
    }

    // `Mip0Width(id)` returns the array of values of the Mip0Width field of the object identified
    // by `id`
    inline Mip0WidthArray Mip0Width(Id id) { return Mip0WidthArray(static_cast<SOA*>(this), id); }
    inline Mip0WidthConstArray Mip0Width(Id id) const
    {
        return Mip0WidthConstArray(static_cast<const SOA*>(this), id);
    }

    // `SetMip0Width(id,value)` sets the `Mip0Width` element of the object identified by `id`
    inline SOA& SetMip0Width(Id id, uint32_t attachment, uint32_t value)
    {
        DIVE_ASSERT(IsValidId(id));
        *Mip0WidthPtr(id, attachment) = value;
        MarkFieldSet(id, kMip0WidthIndex + attachment);
        return static_cast<SOA&>(*this);
    }

    inline bool IsMip0WidthSet(Id id, uint32_t attachment) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kMip0WidthIndex + attachment);
    }

    inline const char* GetMip0WidthName() const { return "Mip0Width"; }

    inline const char* GetMip0WidthDescription() const
    {
        return "Per target attachment mip0 width";
    }

    //-----------------------------------------------
    // FIELD Vgpr: Per shader stage vector general purpose register count. Always rounded up to
    // nearest multiple of 4.

    // `VgprPtr()` returns a shared pointer to an array of `size()` elements
    inline const uint16_t* VgprPtr() const
    {
        return reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kVgprOffset * m_cap);
    }
    inline uint16_t* VgprPtr()
    {
        return reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kVgprOffset * m_cap);
    }
    // `VgprPtr()` returns a shared pointer to an array of `size()` elements
    inline const uint16_t* VgprPtr(
    Id                id,
    Dive::ShaderStage stage = static_cast<Dive::ShaderStage>(0)) const
    {
        return reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kVgprOffset * m_cap) +
               static_cast<typename Id::basic_type>(id) * Dive::kShaderStageCount +
               static_cast<uint32_t>(stage)

        ;
    }
    inline uint16_t* VgprPtr(Id id, Dive::ShaderStage stage = static_cast<Dive::ShaderStage>(0))
    {
        return reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kVgprOffset * m_cap) +
               static_cast<typename Id::basic_type>(id) * Dive::kShaderStageCount +
               static_cast<uint32_t>(stage)

        ;
    }
    // `Vgpr(id)` retuns the `Vgpr` element of the object identified by `id`
    inline uint16_t Vgpr(Id id, Dive::ShaderStage stage) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *VgprPtr(id, stage);
    }

    // `Vgpr(id)` returns the array of values of the Vgpr field of the object identified by `id`
    inline VgprArray      Vgpr(Id id) { return VgprArray(static_cast<SOA*>(this), id); }
    inline VgprConstArray Vgpr(Id id) const
    {
        return VgprConstArray(static_cast<const SOA*>(this), id);
    }

    // `SetVgpr(id,value)` sets the `Vgpr` element of the object identified by `id`
    inline SOA& SetVgpr(Id id, Dive::ShaderStage stage, uint16_t value)
    {
        DIVE_ASSERT(IsValidId(id));
        *VgprPtr(id, stage) = value;
        MarkFieldSet(id, kVgprIndex + static_cast<uint32_t>(stage));
        return static_cast<SOA&>(*this);
    }

    inline bool IsVgprSet(Id id, Dive::ShaderStage stage) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kVgprIndex + static_cast<uint32_t>(stage));
    }

    inline const char* GetVgprName() const { return "Vgpr"; }

    inline const char* GetVgprDescription() const
    {
        return "Per shader stage vector general purpose register count. Always rounded up to "
               "nearest multiple of 4.";
    }

    //-----------------------------------------------
    // FIELD Sgpr: Per shader stage scalar general purpose register count. Always rounded up to
    // nearest multiple of 16

    // `SgprPtr()` returns a shared pointer to an array of `size()` elements
    inline const uint16_t* SgprPtr() const
    {
        return reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kSgprOffset * m_cap);
    }
    inline uint16_t* SgprPtr()
    {
        return reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kSgprOffset * m_cap);
    }
    // `SgprPtr()` returns a shared pointer to an array of `size()` elements
    inline const uint16_t* SgprPtr(
    Id                id,
    Dive::ShaderStage stage = static_cast<Dive::ShaderStage>(0)) const
    {
        return reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kSgprOffset * m_cap) +
               static_cast<typename Id::basic_type>(id) * Dive::kShaderStageCount +
               static_cast<uint32_t>(stage)

        ;
    }
    inline uint16_t* SgprPtr(Id id, Dive::ShaderStage stage = static_cast<Dive::ShaderStage>(0))
    {
        return reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(m_buffer.get()) +
                                           kSgprOffset * m_cap) +
               static_cast<typename Id::basic_type>(id) * Dive::kShaderStageCount +
               static_cast<uint32_t>(stage)

        ;
    }
    // `Sgpr(id)` retuns the `Sgpr` element of the object identified by `id`
    inline uint16_t Sgpr(Id id, Dive::ShaderStage stage) const
    {
        DIVE_ASSERT(IsValidId(id));
        return *SgprPtr(id, stage);
    }

    // `Sgpr(id)` returns the array of values of the Sgpr field of the object identified by `id`
    inline SgprArray      Sgpr(Id id) { return SgprArray(static_cast<SOA*>(this), id); }
    inline SgprConstArray Sgpr(Id id) const
    {
        return SgprConstArray(static_cast<const SOA*>(this), id);
    }

    // `SetSgpr(id,value)` sets the `Sgpr` element of the object identified by `id`
    inline SOA& SetSgpr(Id id, Dive::ShaderStage stage, uint16_t value)
    {
        DIVE_ASSERT(IsValidId(id));
        *SgprPtr(id, stage) = value;
        MarkFieldSet(id, kSgprIndex + static_cast<uint32_t>(stage));
        return static_cast<SOA&>(*this);
    }

    inline bool IsSgprSet(Id id, Dive::ShaderStage stage) const
    {
        DIVE_ASSERT(IsValidId(id));
        return IsFieldSet(id, kSgprIndex + static_cast<uint32_t>(stage));
    }

    inline const char* GetSgprName() const { return "Sgpr"; }

    inline const char* GetSgprDescription() const
    {
        return "Per shader stage scalar general purpose register count. Always rounded up to "
               "nearest multiple of 16";
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
    static constexpr uint32_t kLogicOpEnabledIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kLogicOpEnabledOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kLogicOpEnabledSize = sizeof(bool);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kLogicOpEnabledOffset + kLogicOpEnabledSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kLogicOpEnabledIndex + 1
    static_assert(alignof(VkLogicOp) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kLogicOpIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kLogicOpOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kLogicOpSize = sizeof(VkLogicOp);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kLogicOpOffset + kLogicOpSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kLogicOpIndex + 1
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
    static_assert(alignof(uint64_t) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kZAddrIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kZAddrOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kZAddrSize = sizeof(uint64_t);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kZAddrOffset + kZAddrSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kZAddrIndex + 1
    static_assert(alignof(uint64_t) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kHTileAddrIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kHTileAddrOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kHTileAddrSize = sizeof(uint64_t);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kHTileAddrOffset + kHTileAddrSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kHTileAddrIndex + 1
    static_assert(alignof(bool) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kHiZEnabledIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kHiZEnabledOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kHiZEnabledSize = sizeof(bool);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kHiZEnabledOffset + kHiZEnabledSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kHiZEnabledIndex + 1
    static_assert(alignof(bool) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kHiSEnabledIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kHiSEnabledOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kHiSEnabledSize = sizeof(bool);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kHiSEnabledOffset + kHiSEnabledSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kHiSEnabledIndex + 1
    static_assert(alignof(bool) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kZCompressEnabledIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kZCompressEnabledOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kZCompressEnabledSize = sizeof(bool);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kZCompressEnabledOffset + kZCompressEnabledSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kZCompressEnabledIndex + 1
    static_assert(alignof(bool) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kStencilCompressEnabledIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kStencilCompressEnabledOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kStencilCompressEnabledSize = sizeof(bool);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kStencilCompressEnabledOffset + kStencilCompressEnabledSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kStencilCompressEnabledIndex + 1
    static_assert(alignof(bool) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kCompressedZFetchEnabledIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kCompressedZFetchEnabledOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kCompressedZFetchEnabledSize = sizeof(bool);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kCompressedZFetchEnabledOffset + kCompressedZFetchEnabledSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kCompressedZFetchEnabledIndex + 1
    static_assert(alignof(Dive::Legacy::ZFormat) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kZFormatIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kZFormatOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kZFormatSize = sizeof(Dive::Legacy::ZFormat);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kZFormatOffset + kZFormatSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kZFormatIndex + 1
    static_assert(alignof(Dive::Legacy::ZOrder) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kZOrderIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kZOrderOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kZOrderSize = sizeof(Dive::Legacy::ZOrder);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kZOrderOffset + kZOrderSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kZOrderIndex + 1
    static_assert(alignof(uint16_t) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kVSLateAllocIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kVSLateAllocOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kVSLateAllocSize = sizeof(uint16_t);
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kVSLateAllocOffset + kVSLateAllocSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kVSLateAllocIndex + 1
    static_assert(alignof(bool) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kDccEnabledIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kDccEnabledOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kDccEnabledArrayCount = 8;
    static constexpr size_t   kDccEnabledSize = sizeof(bool) * kDccEnabledArrayCount;
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kDccEnabledOffset + kDccEnabledSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kDccEnabledIndex + kDccEnabledArrayCount
    static_assert(alignof(Dive::Legacy::ColorFormat) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kColorFormatIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kColorFormatOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kColorFormatArrayCount = 8;
    static constexpr size_t   kColorFormatSize = sizeof(Dive::Legacy::ColorFormat) *
                                               kColorFormatArrayCount;
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kColorFormatOffset + kColorFormatSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kColorFormatIndex + kColorFormatArrayCount
    static_assert(alignof(uint32_t) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kMip0HeightIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kMip0HeightOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kMip0HeightArrayCount = 8;
    static constexpr size_t   kMip0HeightSize = sizeof(uint32_t) * kMip0HeightArrayCount;
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kMip0HeightOffset + kMip0HeightSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kMip0HeightIndex + kMip0HeightArrayCount
    static_assert(alignof(uint32_t) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kMip0WidthIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kMip0WidthOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kMip0WidthArrayCount = 8;
    static constexpr size_t   kMip0WidthSize = sizeof(uint32_t) * kMip0WidthArrayCount;
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kMip0WidthOffset + kMip0WidthSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kMip0WidthIndex + kMip0WidthArrayCount
    static_assert(alignof(uint16_t) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kVgprIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kVgprOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kVgprArrayCount = Dive::kShaderStageCount;
    static constexpr size_t   kVgprSize = sizeof(uint16_t) * kVgprArrayCount;
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kVgprOffset + kVgprSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kVgprIndex + kVgprArrayCount
    static_assert(alignof(uint16_t) <= kAlignment,
                  "Field type aligment requirement cannot exceed kAlignment");
    static constexpr uint32_t kSgprIndex = PARTIAL_INDEX_EventStateInfo;
    static constexpr size_t   kSgprOffset = PARTIAL_SIZE_EventStateInfo;
    static constexpr size_t   kSgprArrayCount = Dive::kShaderStageCount;
    static constexpr size_t   kSgprSize = sizeof(uint16_t) * kSgprArrayCount;
#undef PARTIAL_SIZE_EventStateInfo
#define PARTIAL_SIZE_EventStateInfo kSgprOffset + kSgprSize
#undef PARTIAL_INDEX_EventStateInfo
#define PARTIAL_INDEX_EventStateInfo kSgprIndex + kSgprArrayCount

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
    bool*                                DBG_stencil_test_enabled;
    VkStencilOpState*                    DBG_stencil_op_state_front;
    VkStencilOpState*                    DBG_stencil_op_state_back;
    float*                               DBG_min_depth_bounds;
    float*                               DBG_max_depth_bounds;
    bool*                                DBG_logic_op_enabled;
    VkLogicOp*                           DBG_logic_op;
    VkPipelineColorBlendAttachmentState* DBG_attachment;
    float*                               DBG_blend_constant;
    uint64_t*                            DBG_z_addr;
    uint64_t*                            DBG_h_tile_addr;
    bool*                                DBG_hi_z_enabled;
    bool*                                DBG_hi_s_enabled;
    bool*                                DBG_z_compress_enabled;
    bool*                                DBG_stencil_compress_enabled;
    bool*                                DBG_compressed_z_fetch_enabled;
    Dive::Legacy::ZFormat*               DBG_z_format;
    Dive::Legacy::ZOrder*                DBG_z_order;
    uint16_t*                            DBG_vs_late_alloc;
    bool*                                DBG_dcc_enabled;
    Dive::Legacy::ColorFormat*           DBG_color_format;
    uint32_t*                            DBG_mip0_height;
    uint32_t*                            DBG_mip0_width;
    uint16_t*                            DBG_vgpr;
    uint16_t*                            DBG_sgpr;
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
    using AttachmentArray = EventStateInfoAttachmentArray<EventStateInfo_CONFIG>;
    using AttachmentConstArray = EventStateInfoAttachmentConstArray<EventStateInfo_CONFIG>;
    using BlendConstantArray = EventStateInfoBlendConstantArray<EventStateInfo_CONFIG>;
    using BlendConstantConstArray = EventStateInfoBlendConstantConstArray<EventStateInfo_CONFIG>;
    using DccEnabledArray = EventStateInfoDccEnabledArray<EventStateInfo_CONFIG>;
    using DccEnabledConstArray = EventStateInfoDccEnabledConstArray<EventStateInfo_CONFIG>;
    using ColorFormatArray = EventStateInfoColorFormatArray<EventStateInfo_CONFIG>;
    using ColorFormatConstArray = EventStateInfoColorFormatConstArray<EventStateInfo_CONFIG>;
    using Mip0HeightArray = EventStateInfoMip0HeightArray<EventStateInfo_CONFIG>;
    using Mip0HeightConstArray = EventStateInfoMip0HeightConstArray<EventStateInfo_CONFIG>;
    using Mip0WidthArray = EventStateInfoMip0WidthArray<EventStateInfo_CONFIG>;
    using Mip0WidthConstArray = EventStateInfoMip0WidthConstArray<EventStateInfo_CONFIG>;
    using VgprArray = EventStateInfoVgprArray<EventStateInfo_CONFIG>;
    using VgprConstArray = EventStateInfoVgprConstArray<EventStateInfo_CONFIG>;
    using SgprArray = EventStateInfoSgprArray<EventStateInfo_CONFIG>;
    using SgprConstArray = EventStateInfoSgprConstArray<EventStateInfo_CONFIG>;
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
