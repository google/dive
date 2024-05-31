/*
 Copyright 2024 Google LLC

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/
#include <iostream>
#include <vector>

#include "dive_core/data_core.h"
#include "pm4_info.h"

//--------------------------------------------------------------------------------------------------
// validate LRZ from the capture
class LRZValidator : public Dive::IEmulateCallbacks
{
public:
    LRZValidator(Dive::EmulateStateTracker &state_tracker, std::ofstream &output_file);
    ~LRZValidator();

    virtual void OnSubmitStart(uint32_t submit_index, const Dive::SubmitInfo &submit_info) override;
    virtual void OnSubmitEnd(uint32_t submit_index, const Dive::SubmitInfo &submit_info) override;

    const Dive::EmulateStateTracker &GetStateTracker() const { return m_state_tracker; }

    // Callbacks
    virtual bool OnIbStart(uint32_t                        submit_index,
                           uint32_t                        ib_index,
                           const Dive::IndirectBufferInfo &ib_info,
                           Dive::IbType                    type) override;

    virtual bool OnIbEnd(uint32_t                        submit_index,
                         uint32_t                        ib_index,
                         const Dive::IndirectBufferInfo &ib_info) override;

    virtual bool OnPacket(const Dive::IMemoryManager &mem_manager,
                          uint32_t                    submit_index,
                          uint32_t                    ib_index,
                          uint64_t                    va_addr,
                          Dive::Pm4Type               type,
                          uint32_t                    header) override;

private:
    Dive::EmulateStateTracker &m_state_tracker;
    std::ofstream             &m_output_file;
    // In the tiled mode, we skip all drawcalls in the tiled rendering passes, only keep the ones in
    // the binning pass
    bool m_skip_drawcalls = false;
};

// =================================================================================================
// LRZValidator
// =================================================================================================
LRZValidator::LRZValidator(Dive::EmulateStateTracker &state_tracker, std::ofstream &output_file) :
    m_state_tracker(state_tracker),
    m_output_file(output_file),
    m_skip_drawcalls(false)
{
    m_state_tracker.Reset();
}

//--------------------------------------------------------------------------------------------------
LRZValidator::~LRZValidator() {}

//--------------------------------------------------------------------------------------------------
void LRZValidator::OnSubmitStart(uint32_t submit_index, const Dive::SubmitInfo &submit_info)
{
    m_state_tracker.Reset();
    m_skip_drawcalls = false;
}

//--------------------------------------------------------------------------------------------------
void LRZValidator::OnSubmitEnd(uint32_t submit_index, const Dive::SubmitInfo &submit_info) {}

//--------------------------------------------------------------------------------------------------
bool LRZValidator::OnIbStart(uint32_t                        submit_index,
                             uint32_t                        ib_index,
                             const Dive::IndirectBufferInfo &ib_info,
                             Dive::IbType                    type)
{
    m_state_tracker.PushEnableMask(ib_info.m_enable_mask);
    return true;
}

//--------------------------------------------------------------------------------------------------
bool LRZValidator::OnIbEnd(uint32_t                        submit_index,
                           uint32_t                        ib_index,
                           const Dive::IndirectBufferInfo &ib_info)
{
    m_state_tracker.PopEnableMask();
    return true;
}

//--------------------------------------------------------------------------------------------------
bool LRZValidator::OnPacket(const Dive::IMemoryManager &mem_manager,
                            uint32_t                    submit_index,
                            uint32_t                    ib_index,
                            uint64_t                    va_addr,
                            Dive::Pm4Type               type,
                            uint32_t                    header)
{
    if (!m_state_tracker.OnPacket(mem_manager, submit_index, ib_index, va_addr, type, header))
        return false;

    if (type != Dive::Pm4Type::kType7)
        return true;

    Pm4Type7Header *type7_header = (Pm4Type7Header *)&header;
    const uint32_t  opcode = type7_header->opcode;

    if (opcode == CP_SET_MARKER)
    {
        PM4_CP_SET_MARKER packet;
        DIVE_VERIFY(mem_manager.CopyMemory(&packet, submit_index, va_addr, sizeof(packet)));
        // as mentioned in adreno_pm4.xml, only b0-b3 are considered when b8 is not set
        DIVE_ASSERT((packet.u32All0 & 0x100) == 0);
        a6xx_marker marker = static_cast<a6xx_marker>(packet.u32All0 & 0xf);
        // we skip all drawcalls in the tile rendering passes
        if (marker == RM6_GMEM)
        {
            m_skip_drawcalls = true;
        }
        else
        {
            m_skip_drawcalls = false;
        }
    }

    if (m_skip_drawcalls)
        return true;

    // This is just to align the strings so that they are easier to read
    auto AppendSpace = [](std::string &str, uint32_t desired_len) {
        if (str.length() < desired_len)
        {
            str.append(desired_len - str.length(), ' ');
        }
    };

    if (Dive::IsDrawEventOpcode(opcode))
    {
        std::string    draw_string = Dive::Util::GetEventString(mem_manager,
                                                             submit_index,
                                                             va_addr,
                                                             opcode);
        const uint32_t desired_draw_string_len = 64;
        AppendSpace(draw_string, desired_draw_string_len);
        m_output_file << draw_string + "\t";

        uint32_t rb_depth_cntl_reg_offset = GetRegOffsetByName("RB_DEPTH_CNTL");
        if (m_state_tracker.IsRegSet(rb_depth_cntl_reg_offset))
        {
            RB_DEPTH_CNTL rb_depth_cntl;
            rb_depth_cntl.u32All = m_state_tracker.GetRegValue(rb_depth_cntl_reg_offset);
            const bool is_depth_test_enabled = rb_depth_cntl.bitfields.Z_TEST_ENABLE;
            const bool is_depth_write_enabled = rb_depth_cntl.bitfields.Z_WRITE_ENABLE;
            // Be careful!!! Here we assume the enum `VkCompareOp` matches exactly
            // `adreno_compare_func`
            VkCompareOp zfunc = static_cast<VkCompareOp>(rb_depth_cntl.bitfields.ZFUNC);

            if (is_depth_test_enabled)
            {
                m_output_file << "DepthTest:Enabled\t";
            }
            else
            {
                m_output_file << "DepthTest:Disabled\t";
            }

            if (is_depth_write_enabled)
            {
                m_output_file << "DepthWrite:Enabled\t";
            }
            else
            {
                m_output_file << "DepthWrite:Disabled\t";
            }

            std::string zfunc_str = "Invalid";
            switch (zfunc)
            {
            case VK_COMPARE_OP_NEVER: zfunc_str = "Never"; break;
            case VK_COMPARE_OP_LESS: zfunc_str = "Less"; break;
            case VK_COMPARE_OP_EQUAL: zfunc_str = "Equal"; break;
            case VK_COMPARE_OP_LESS_OR_EQUAL: zfunc_str = "Less or Equal"; break;
            case VK_COMPARE_OP_GREATER: zfunc_str = "Greater"; break;
            case VK_COMPARE_OP_NOT_EQUAL: zfunc_str = "Not Equal"; break;
            case VK_COMPARE_OP_GREATER_OR_EQUAL: zfunc_str = "Greater or Equal"; break;
            case VK_COMPARE_OP_ALWAYS: zfunc_str = "Always"; break;
            default: DIVE_ASSERT(false); break;
            }
            const uint32_t desired_zfunc_str_len = 16;
            AppendSpace(zfunc_str, desired_zfunc_str_len);
            m_output_file << "DepthFunc:" + zfunc_str + "\t";

            if (is_depth_test_enabled)
            {
                uint32_t gras_lrz_cntl_reg_offset = GetRegOffsetByName("GRAS_LRZ_CNTL");
                if (m_state_tracker.IsRegSet(gras_lrz_cntl_reg_offset))
                {
                    GRAS_LRZ_CNTL gras_lrz_cntl;
                    gras_lrz_cntl.u32All = m_state_tracker.GetRegValue(gras_lrz_cntl_reg_offset);
                    const auto bitfields = gras_lrz_cntl.bitfields;
                    const bool lrz_enabled = bitfields.ENABLE;

                    if (!lrz_enabled)
                    {
                        m_output_file << "LRZ:Disabled\t";
                        // if depth func is Always or Never, we don't really care about LRZ
                        if (is_depth_test_enabled &&
                            ((zfunc != VK_COMPARE_OP_NEVER) && (zfunc != VK_COMPARE_OP_ALWAYS)))
                        {
                            m_output_file
                            << "[WARNING!!!] LRZ is disabled for this drawcall but depth test is "
                               "enabled, and depth func is not Never or Always!";
                        }
                    }
                    else
                    {
                        m_output_file << "LRZ:Enabled\t";
                    }
                }
            }
            else
            {
                m_output_file << "LRZ:Disabled\t";
            }

            m_output_file << std::endl;
        }
    }
    return true;
}

//--------------------------------------------------------------------------------------------------
int main(int argc, char **argv)
{
    Pm4InfoInit();
    if ((argc != 2) && (argc != 3))
    {
        std::cout
        << "You need to call: lrz_validator <input_file_name.rd> <output_file_name.txt>(optional)";
        return 0;
    }
    char             *input_file_name = argv[1];
    Dive::LogCompound log_compound;
    {
        std::unique_ptr<Dive::DataCore> data_core = std::make_unique<Dive::DataCore>(&log_compound);
        Dive::CaptureData::LoadResult   load_res = data_core->LoadCaptureData(input_file_name);
        if (load_res != Dive::CaptureData::LoadResult::kSuccess)
        {
            std::cout << "Loading capture " << argv[1] << " failed!";
            return 0;
        }
        std::string output_file_name = "lrz_validation_result.txt";
        if (argc == 3)
        {
            output_file_name = argv[2];
        }

        std::unique_ptr<Dive::EmulateStateTracker> state_tracker(new Dive::EmulateStateTracker);

        std::ofstream outputFile(output_file_name);

        LRZValidator lrz_validator(*state_tracker, outputFile);
        if (!lrz_validator.ProcessSubmits(data_core->GetCaptureData().GetSubmits(),
                                          data_core->GetCaptureData().GetMemoryManager()))
        {
            std::cout << "Validate " << argv[1] << " failed!";
            return 0;
        }
    }

    return 1;
}
