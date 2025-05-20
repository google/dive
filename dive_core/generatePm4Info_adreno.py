from __future__ import print_function
import sys
import os
import re
import xml.etree.ElementTree as ET

from common import isBuiltInType
from common import gatherAllEnums
from common import GetGPUVariantsBitField
from common import addMissingDomains

# ---------------------------------------------------------------------------------------
def outputHeader(pm4_info_file):
  pm4_info_file.writelines('''
/*
Copyright 2020 Google LLC

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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// WARNING!  WARNING!  WARNING!  WARNING!  WARNING!  WARNING!  WARNING!  WARNING!  WARNING!  WARNING!  WARNING!
//
// This code has been generated automatically by generatePm4Strings_adreno.py. Do not hand-modify this code.
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
''')

# ---------------------------------------------------------------------------------------
def outputH(pm4_info_file):
  outputHeader(pm4_info_file)
  pm4_info_file.writelines('''
#pragma once

#include <stdint.h>
#include <string.h>
#include "dive_core/stl_replacement.h"

enum ValueType
{
    kBoolean,
    kUint,
    kInt,
    kFloat,     // floating-point numbers, including 16bit, 32bit, 64bit
    kFixed,     // signed fixed-point numbers
    kUFixed,    // unsigned fixed-point numbers
    kAddress,   // 64bit address (bo_write = false)
    kWaddress,  // 64bit address (bo_write = true)
    kHex,       // hex representation of numbers
    kRegID,     // register id
    kOther
};

enum GPUVariantType
{
    kGPUVariantNone = 0x0,
    kA2XX = 0x1,
    kA3XX = 0x2,
    kA4XX = 0x4,
    kA5XX = 0x8,
    kA6XX = 0x10,
    kA7XX = 0x20,
};

constexpr uint32_t kInvalidRegOffset = UINT32_MAX;
// be careful when increase this value
// this is used in
// - RegField::m_gpu_variants, so the unused bits needs to be adjusted
// - key of g_sRegInfo, the register offset needs at least 16bits, so kGPUVariantsBits cannot be
// larger than 16
constexpr uint32_t kGPUVariantsBits = 6;

struct RegField
{
    uint32_t m_type : 4;  // ValueType enum, range [0, 15]
    uint32_t m_enum_handle : 8;
    uint32_t m_shift : 6;
    uint32_t m_shr : 5;  // used to shift left to extract the "original" value, range: [0, 31]
    uint32_t m_gpu_variants : kGPUVariantsBits;  // only 6 bits are used for now, see GPUVariantType
    uint32_t : 3;
    uint32_t m_bit_width : 6; // high - low, range [0, 63]
    uint32_t m_radix : 5; // only used when the type is ufixed/fixed, range [0, 31]
    uint32_t : 21;
    uint64_t    m_mask;
    const char *m_name;
};

struct RegInfo
{
    const char *m_name;
    uint32_t    m_is_64_bit : 1;  // Either 32 or 64 bit
    uint32_t    m_type : 4;       // ValueType enum, range [0, 15]
    uint32_t    m_enum_handle : 8;
    uint32_t    m_shr : 5;  // used to shift left to extract the "original" value, range: [0, 31]
    uint32_t    m_bit_width : 6; // high - low, range [0, 63]
    uint32_t    m_radix : 5; // only used when the type is ufixed/fixed, range [0, 31]
    uint32_t : 3;
    DiveVector<RegField> m_fields;
};

struct PacketField
{
    const char *m_name;
    uint32_t    m_is_variant_opcode : 1;  // If 1, then is used to indicate variant
    uint32_t    m_dword : 8;
    uint32_t    m_type : 4;  // ValueType enum, range [0, 15]
    uint32_t    m_enum_handle : 8;
    uint32_t    m_shift : 6;
    uint32_t    m_shr : 5;  // used to shift left to extract the "original" value, range: [0, 31]
    uint32_t    m_mask;
};

struct PacketInfo
{
    const char *m_name;
    uint32_t    m_max_array_size : 8;
    uint32_t    m_stripe_variant : 8;  // Which variant of the packet this is
    uint32_t : 16;
    DiveVector<PacketField> m_fields;
};

void              Pm4InfoInit();
const char       *GetOpCodeString(uint32_t op_code);
const RegInfo    *GetRegInfo(uint32_t reg);
const RegInfo    *GetRegByName(const char *);
const RegField   *GetRegFieldByName(const char *name, const RegInfo *info);
uint32_t          GetRegOffsetByName(const char *name);
const char       *GetEnumString(uint32_t enum_handle, uint32_t val);
const PacketInfo *GetPacketInfo(uint32_t op_code);
const PacketInfo *GetPacketInfo(uint32_t op_code, const char *name);
void              SetGPUID(uint32_t gpu_id);
uint32_t          GetGPUID();
GPUVariantType    GetGPUVariantType();
bool              IsFieldEnabled(const RegField *field);
''')

# ---------------------------------------------------------------------------------------
def outputHeaderCpp(pm4_info_header_file_name, pm4_info_file):
  outputHeader(pm4_info_file)
  pm4_info_file.write('#include "%s"\n' % (pm4_info_header_file_name))
  pm4_info_file.writelines('''
#include <assert.h>
#include <algorithm>
#include <cstring>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include "dive_core/common/common.h"

static DiveVector<const char*> g_sOpCodeToString;
static std::unordered_map<uint32_t, RegInfo> g_sRegInfoVariant;
static DiveVector<RegInfo> g_sRegInfo;
static std::unordered_map<std::string, uint32_t> g_sRegNameToIndex;
static DiveVector<DiveVector<const char*>> g_sEnumReflection;
static DiveVector<PacketInfo> g_sPacketInfo;
static std::unordered_map<uint32_t, PacketInfo> g_sPacketInfoVariant;
static std::multimap<uint32_t, PacketInfo> g_sPacketInfoMultiple;
static GPUVariantType g_sGPU_variant = kGPUVariantNone;
static uint32_t g_sGPU_id = 0;

std::string GetGPUStr(GPUVariantType variant)
{
    std::string s;
    switch(variant)
    {
    case kA2XX: s = "A2XX"; break;
    case kA3XX: s = "A3XX"; break;
    case kA4XX: s = "A4XX"; break;
    case kA5XX: s = "A5XX"; break;
    case kA6XX: s = "A6XX"; break;
    case kA7XX: s = "A7XX"; break;
    case kGPUVariantNone:
    default:
        DIVE_ASSERT(false);
        break;
    }
    return s;
}

'''
  )

# ---------------------------------------------------------------------------------------
def outputPm4InfoInitFunc(pm4_info_file, registers_et_root, opcode_dict):

  # Get enum values from the XML element tree, being careful not to have duplicates
  enum_index_dict = {}
  enum_list = []
  parseEnumInfo(enum_index_dict, enum_list, registers_et_root)

  pm4_info_file.writelines('''
void Pm4InfoInit()
{
    assert(g_sOpCodeToString.empty());
    assert(g_sRegInfo.empty());
    assert(g_sEnumReflection.empty());
    assert(g_sPacketInfo.empty());
    assert(g_sPacketInfoVariant.empty());
''')
  outputOpcodes(pm4_info_file, opcode_dict)
  pm4_info_file.write('\n')
  outputRegisterInfo(pm4_info_file, registers_et_root, enum_index_dict)
  pm4_info_file.write('\n')
  outputEnums(pm4_info_file, enum_list)
  pm4_info_file.write('\n')
  outputPacketInfo(pm4_info_file, registers_et_root, enum_index_dict, opcode_dict)
  pm4_info_file.write('}\n')
  return

valid_opcodes = {}
# ---------------------------------------------------------------------------------------
def outputOpcodes(pm4_info_file, opcode_dict):
  # Find max opcode first
  max_opcode = max(opcode_dict)

  pm4_info_file.write('    g_sOpCodeToString.resize(0x%x);\n' % (max_opcode+1))
  for opcode in opcode_dict:
    pm4_info_file.write('    g_sOpCodeToString[%s] = "%s";\n' % (hex(opcode), opcode_dict[opcode]))
  return

# ---------------------------------------------------------------------------------------
def getTypeEnumString(type):
  type_string = 'ValueType::kOther'
  if type == 'boolean':
    type_string = 'ValueType::kBoolean'
  elif type == 'uint':
    type_string = 'ValueType::kUint'
  elif type == 'int':
    type_string = 'ValueType::kInt'
  elif type == 'float':
    type_string = 'ValueType::kFloat'
  elif type == 'fixed':
    type_string = 'ValueType::kFixed'
  elif type == 'ufixed':
    type_string = 'ValueType::kUFixed'
  elif type == 'address':
    type_string = 'ValueType::kAddress'
  elif type == 'waddress':
    type_string = 'ValueType::kWaddress'
  elif type == 'hex':
    type_string = 'ValueType::kHex'
  elif type == 'a3xx_regid':
    type_string = 'ValueType::kRegID'
  return type_string

class RegAttributes():
  name = ''
  offset = 0
  bitfields = []
  type = None
  is_64 = False
  variants = ''
  shr = 0
  bit_width = 0
  radix = 0

# ---------------------------------------------------------------------------------------
def AppendBitfield(pm4_info_file, enum_index_dict, attributes):
    # Iterate through optional bitfields
    for bitfield in attributes.bitfields:
      if bitfield.tag == '{http://nouveau.freedesktop.org/}doc':
        continue

      name = bitfield.attrib['name']

      enum_handle = 'UINT8_MAX'
      bitfield_type = 'other'
      if 'type' in bitfield.attrib:
        bitfield_type = bitfield.attrib['type']
        if not isBuiltInType(bitfield_type):
          if bitfield_type not in enum_index_dict:
            raise Exception('Enumeration %s not found!' % bitfield_type)
          if enum_index_dict[bitfield_type] > 256:
            raise Exception('Enumeration handle %d is too big! The bitfield storing this is only 8-bits!' % enum_index_dict[bitfield_type])
          enum_handle = str(enum_index_dict[bitfield_type])

      shr = 0
      if 'shr' in bitfield.attrib:
        shr = int(bitfield.attrib['shr'])
      # TODO: Store the align, and variants bits. The 'type' field in particular should reference
      #  an enum value when appropriate

      # Convert to mask & shift
      width = 0
      if 'low' in bitfield.attrib and 'high' in bitfield.attrib:
        shift = low = int(bitfield.attrib['low'])
        high = int(bitfield.attrib['high'])
        width = high - low
        mask = (0xffffffffffffffff >> (63 - (high - low))) << low
        if not attributes.is_64:
          mask = (0x00000000ffffffff >> (31 - (high - low))) << low
      elif 'pos' in bitfield.attrib:
        shift = pos = int(bitfield.attrib['pos'])
        mask = (0x1 << pos)
      else:
        raise Exception('Encountered a bitfield with no pos/low/high!')

      variants_bitfield = 0
      if 'variants' in bitfield.attrib:
        variants_bitfield = GetGPUVariantsBitField(bitfield.attrib['variants'])

      radix = getIntAttributeValue(bitfield, 'radix')

      pm4_info_file.write('    { %s, %s, %d, %d, %d, %d, %d, 0x%x, "%s" }, '  % (
          getTypeEnumString(bitfield_type),
          enum_handle,
          shift,
          shr,
          variants_bitfield,
          width,
          radix,
          mask,
          name
        ))

# ---------------------------------------------------------------------------------------
def outputSingleRegister(pm4_info_file, registers_et_root, a6xx_domain, enum_index_dict, attributes: RegAttributes):
    is_64_string = '0'
    if attributes.is_64 is True:
      is_64_string = '1'

    # if a 'type' attrib is available, then check for a custom bitset
    enum_handle = 'UINT8_MAX'
    if attributes.type:
      # if it isn't one of the standard types, then it must be a custom bitset or an enum
      if not isBuiltInType(attributes.type):
        bitfields = a6xx_domain.find('./{http://nouveau.freedesktop.org/}bitset[@name="'+attributes.type+'"]')

        # Not found in the A6XX domain. Some of the bitsets are defined in the root.
        if bitfields is None:
          bitfields = registers_et_root.find('./{http://nouveau.freedesktop.org/}bitset[@name="'+attributes.type+'"]')

        # It's not a bitset. Let's sanity check that it's a top-level enum!
        if bitfields is None:
          bitfields = []  # Create an empty list instead of a "None" object
          enum = registers_et_root.find('./{http://nouveau.freedesktop.org/}enum[@name="'+attributes.type+'"]')
          if enum is None:
            raise Exception('Not able to find bitset/enum ' + attributes.type + ' for register ' + attributes.name)
          # Now that we know it's an enum, let's get the enum_handle for it!
          if attributes.type not in enum_index_dict:
            raise Exception('Enumeration %s not found!' % attributes.type)
          if enum_index_dict[attributes.type] > 256:
            raise Exception('Enumeration handle %d is too big! The bitfield storing this is only 8-bits!' % enum_index_dict[attributes.type])
          enum_handle = str(enum_index_dict[attributes.type])

    variants_bitfield = GetGPUVariantsBitField(attributes.variants)
    if (variants_bitfield != 0):
        # kGPUVariantsBits has 6 bits
        for i in range(6):
            cur_variant_bitfield = (1<<i)
            if cur_variant_bitfield & variants_bitfield:
                pm4_info_file.write('    g_sRegInfoVariant[(0x%x << kGPUVariantsBits) | 0x%x] = { "%s", %s, %s, %s, %d, %d, %d, {' % (attributes.offset, cur_variant_bitfield, attributes.name, is_64_string, getTypeEnumString(attributes.type), enum_handle, attributes.shr, attributes.bit_width, attributes.radix))
                AppendBitfield(pm4_info_file, enum_index_dict, attributes)
                pm4_info_file.write('} };\n')
    else:
        pm4_info_file.write('    g_sRegInfo[0x%x] = { "%s", %s, %s, %s, %d, %d, %d, {' % (attributes.offset, attributes.name, is_64_string, getTypeEnumString(attributes.type), enum_handle, attributes.shr, attributes.bit_width, attributes.radix))
        AppendBitfield(pm4_info_file, enum_index_dict, attributes)
        pm4_info_file.write('} };\n')


# ---------------------------------------------------------------------------------------
def getBitWidth(reg):
  bit_width = 0
  if 'low' in reg.attrib and 'high' in reg.attrib:
    low = int(reg.attrib['low'])
    high = int(reg.attrib['high'])
    bit_width = high - low
  return bit_width

# ---------------------------------------------------------------------------------------
def getIntAttributeValue(reg, attribute_name):
  value = 0
  if attribute_name in reg.attrib:
    value = int(reg.attrib[attribute_name])
  return value

# ---------------------------------------------------------------------------------------
def outputRegisterInfo(pm4_info_file, registers_et_root, enum_index_dict):
  a6xx_domain = registers_et_root.find('./{http://nouveau.freedesktop.org/}domain[@name="A6XX"]')

  # Create a list of 32-bit and 64-bit registers
  regs = []
  for element in a6xx_domain:
    is_reg_32 = (element.tag == '{http://nouveau.freedesktop.org/}reg32')
    is_reg_64 = (element.tag == '{http://nouveau.freedesktop.org/}reg64')
    if is_reg_32 or is_reg_64:
      regs.append(element)

  # Determine highest register offset
  max_offset = 0
  for reg in regs:
    offset = int(reg.attrib['offset'],0)
    max_offset = max(max_offset, offset)
  pm4_info_file.write('    g_sRegInfo.resize(0x%x);\n' % (max_offset+1))

  # Parse through registers
  for reg in regs:
    offset = int(reg.attrib['offset'],0)
    name = reg.attrib['name']
    bitfields = reg.findall('{http://nouveau.freedesktop.org/}bitfield')
    type = None
    if 'type' in reg.attrib:
      type = reg.attrib['type']
    is_64 = False
    if reg.tag == '{http://nouveau.freedesktop.org/}reg64':
      is_64 = True
    variants = ''
    if 'variants' in reg.attrib:
      variants = reg.attrib['variants']

    shr = getIntAttributeValue(reg, 'shr')
    bit_width = getBitWidth(reg);
    radix = getIntAttributeValue(reg, 'radix')

    reg_attributes = RegAttributes()
    reg_attributes.name = name
    reg_attributes.offset = offset
    reg_attributes.bitfields = bitfields
    reg_attributes.type = type
    reg_attributes.is_64 = is_64
    reg_attributes.variants = variants
    reg_attributes.shr = shr
    reg_attributes.bit_width = bit_width
    reg_attributes.radix = radix

    outputSingleRegister(pm4_info_file, registers_et_root, a6xx_domain, enum_index_dict, reg_attributes)

  # Iterate and output the arrays as a sequence of reg32s with an index as a suffix
  arrays = a6xx_domain.findall('{http://nouveau.freedesktop.org/}array')
  for array in arrays:
    offset = int(array.attrib['offset'],0)
    array_name = array.attrib['name']
    stride = int(array.attrib['stride'])
    length = int(array.attrib['length'])

    # Create a list of 32-bit and 64-bit registers
    array_regs = []
    reg_variants = []
    for element in array:
      is_reg_32 = (element.tag == '{http://nouveau.freedesktop.org/}reg32')
      is_reg_64 = (element.tag == '{http://nouveau.freedesktop.org/}reg64')
      if is_reg_32 or is_reg_64:
        array_regs.append(element)
      variants = ''
      if 'variants' in element.attrib:
        variants = element.attrib['variants']
      reg_variants.append(variants)

    reg_attributes = RegAttributes()
    # Arrays with stride==1 just generate a series of registers with index suffixes
    # Arrays with stride==2 and no reg32s/reg64s are going to generate a 64-bit register entry
    # Arrays with stride>2 must have registers and will generate registers as follows:
    #   Ex: Array name: "BASE", Length: 2, Reg32 names: "ZIG/ZAG"
    #       -> 4 registers with names: BASE0_ZIG, BASE0_ZAG, BASE1_ZIG, BASE1_ZAG
    for i in range(0,length):
      if stride == 1 and not array_regs:
        reg_attributes.name = array_name+str(i)
        reg_attributes.offset = offset+i
        reg_attributes.bitfields = []
        reg_attributes.type = None
        reg_attributes.is_64 = False
        reg_attributes.variants = ''
        reg_attributes.shr = 0
        reg_attributes.bit_width = 0
        reg_attributes.radix = 0
        outputSingleRegister(pm4_info_file, registers_et_root, a6xx_domain, enum_index_dict, reg_attributes)
      elif stride == 2 and not array_regs:
        reg_attributes.name = array_name+str(i)+'_LO'
        reg_attributes.offset = offset+i*stride
        reg_attributes.bitfields = []
        reg_attributes.type = None
        reg_attributes.is_64 = True
        reg_attributes.variants = ''
        reg_attributes.shr = 0
        reg_attributes.bit_width = 0
        reg_attributes.radix = 0
        outputSingleRegister(pm4_info_file, registers_et_root, a6xx_domain, enum_index_dict, reg_attributes)
      else:
        for reg_idx, reg in enumerate(array_regs):
          reg_name = reg.attrib['name']
          reg_offset = int(reg.attrib['offset'],0)
          bitfields = reg.findall('{http://nouveau.freedesktop.org/}bitfield')
          type = None
          if 'type' in reg.attrib:
            type = reg.attrib['type']
          is_64 = False
          if reg.tag == '{http://nouveau.freedesktop.org/}reg64':
            is_64 = True

          shr = getIntAttributeValue(reg, 'shr')
          bit_width = getBitWidth(reg);
          radix = getIntAttributeValue(reg, 'radix')

          reg_attributes.name = array_name+str(i)+'_'+reg_name
          reg_attributes.offset = offset+i*stride+reg_offset
          reg_attributes.bitfields = bitfields
          reg_attributes.type = type
          reg_attributes.is_64 = is_64
          reg_attributes.variants = reg_variants[reg_idx]
          reg_attributes.shr = shr
          reg_attributes.bit_width = bit_width
          reg_attributes.radix = radix
          outputSingleRegister(pm4_info_file, registers_et_root, a6xx_domain, enum_index_dict, reg_attributes)
  return

# ---------------------------------------------------------------------------------------
def parseEnumInfo(enum_index_dict, enum_list, registers_et_root):
  enums = gatherAllEnums(registers_et_root)

  for enum in enums:
    fields = enum.findall('{http://nouveau.freedesktop.org/}value')
    enum_name = enum.attrib['name']

    # There shouldn't be any repeat entries
    if enum_name in enum_index_dict:
      raise Exception('Encountered multiple enums with same name: ' + enum_name)

    enum_index_dict[enum_name] = len(enum_list)
    enum_list.append((enum_name, dict()))

    for field in fields:
      enum_value_name = field.attrib['name']

      # Fields such as INDEX_SIZE_INVALID do not have a value and should be ignored here
      if 'value' in field.attrib:
        enum_value = int(field.attrib['value'], 0)


        # RegField::m_enum_handle is 8-bit. So any enum that exceeds this (e.g. adreno_pm4_packet_type)
        # are not actually used
        if (enum_value < 256):
          # enum_list is an array of {string, dict()}, where the key of the dict() is
          # the integer enum_value
          # Note: It's possible to have multiple enum fields with the same value, for different
          # variants. These fields are listed in order of hw variants, with later entries covering
          # more recent hw. Since we are only interested in the latest hardware, we can just
          # override the earlier value name and use the newer name for the field
          index = enum_index_dict[enum_name]
          enum_list[index][1][enum_value] = enum_value_name

class FieldAttributes():
  name = ''
  is_variant_opcode = 0
  dword_count = 0
  type = ''
  enum_handle = 'UINT8_MAX'
  shift = 0
  shr = 0
  mask = 0

# ---------------------------------------------------------------------------------------
def outputField(pm4_info_file, field_attributes: FieldAttributes):
  pm4_info_file.write('{ "%s", %d, %d, %s, %s, %d, %d, 0x%x },' %
                       (field_attributes.name, field_attributes.is_variant_opcode, field_attributes.dword_count, getTypeEnumString(field_attributes.type), field_attributes.enum_handle, field_attributes.shift, field_attributes.shr, field_attributes.mask))

# ---------------------------------------------------------------------------------------
def outputPacketFields(pm4_info_file, enum_index_dict, reg_list):
  dword_count = 0
  for element in reg_list:
    is_reg_32 = (element.tag == '{http://nouveau.freedesktop.org/}reg32')
    is_reg_64 = (element.tag == '{http://nouveau.freedesktop.org/}reg64')
    offset = int(element.attrib['offset'],0)

    # Sanity check
    # Note: Allowed to skip an offset (see CP_EVENT_WRITE7)
    if dword_count > offset:
        raise Exception('Unexpected reverse offset found in packet')

    dword_count = dword_count + 1
    if is_reg_64:
      dword_count = dword_count + 1

    bitfields = element.findall('{http://nouveau.freedesktop.org/}bitfield')

    # No bitfields, so use the register specification directly
    if len(bitfields) == 0:
      field_name = element.attrib['name']
      shift = 0
      mask = int('0xffffffff', 16)
      enum_handle = 'UINT8_MAX'

      type = None
      if 'type' in element.attrib:
        type = element.attrib['type']

      shr = 0
      if 'shr' in element.attrib:
        shr = int(element.attrib['shr'])

      # if 'addvariant' is an attribute, then this field is used to determine the packet variant
      is_variant_opcode = 0
      if 'addvariant' in element.attrib:
        is_variant_opcode = 1

      field_attributes = FieldAttributes()
      field_attributes.is_variant_opcode = is_variant_opcode
      field_attributes.type = type
      field_attributes.enum_handle = enum_handle
      field_attributes.shift = shift
      field_attributes.shr = shr
      field_attributes.mask = mask

      if is_reg_32:
        field_attributes.name = field_name
        field_attributes.dword_count = dword_count

        outputField(pm4_info_file, field_attributes)
      elif is_reg_64:
        field_attributes.name = field_name+'_LO'
        field_attributes.dword_count = dword_count - 1
        outputField(pm4_info_file, field_attributes)

        field_attributes.name = field_name+'_HI'
        field_attributes.dword_count = dword_count
        outputField(pm4_info_file, field_attributes)

    if is_reg_64 and len(bitfields) > 0:
      raise Exception('Found a reg64 with bitfields: ' + field_name)

    for bitfield in bitfields:
      field_name = bitfield.attrib['name']
      type = None
      if 'type' in bitfield.attrib:
        type = bitfield.attrib['type']

      enum_handle = 'UINT8_MAX'
      if not isBuiltInType(type):
        if type not in enum_index_dict:
          raise Exception('Enumeration %s not found!' % type)
        if enum_index_dict[type] > 256:
          raise Exception('Enumeration handle %d is too big! The bitfield storing this is only 8-bits!' % enum_index_dict[type])
        enum_handle = str(enum_index_dict[type])

      # TODO: Store the shr bits so we know how much to shift left to extract the "original" value

      # Convert to mask & shift
      if 'low' in bitfield.attrib and 'high' in bitfield.attrib:
        shift = low = int(bitfield.attrib['low'])
        high = int(bitfield.attrib['high'])
        mask = (0xffffffff >> (31 - (high - low))) << low
      elif 'pos' in bitfield.attrib:
        shift = pos = int(bitfield.attrib['pos'])
        mask = (0x1 << pos)
      else:
        raise Exception('Encountered a bitfield with no pos/low/high!')

      shr = 0
      if 'shr' in bitfield.attrib:
        shr = int(bitfield.attrib['shr'])

      # if 'addvariant'  is an attribute, then this field is used to determine the packet variant
      is_variant_opcode = 0
      if 'addvariant' in bitfield.attrib:
        is_variant_opcode = 1

      field_attributes = FieldAttributes()
      field_attributes.name = field_name
      field_attributes.is_variant_opcode = is_variant_opcode
      field_attributes.dword_count = dword_count
      field_attributes.type = type
      field_attributes.enum_handle = enum_handle
      field_attributes.shift = shift
      field_attributes.shr = shr
      field_attributes.mask = mask
      outputField(pm4_info_file, field_attributes)

# ---------------------------------------------------------------------------------------
def outputEnums(pm4_info_file, enum_list):
  pm4_info_file.write('    g_sEnumReflection.resize(%d);\n' % (len(enum_list)+1));
  # Output enum_list to file
  for idx, enum_info in enumerate(enum_list):
    # enum_list is an array of {string, dict()}, where the key of the dict() is
    # the integer enum_value
    enum_sorted_items = sorted(enum_info[1].items())
    max_enum_value = enum_sorted_items[-1][0]
    pm4_info_file.write('    g_sEnumReflection[%d].resize(%d, nullptr); // %s\n' % (idx, max_enum_value+1, enum_info[0]));
    for enum_value, enum_value_string in enum_sorted_items:
      pm4_info_file.write('    g_sEnumReflection[%d][%d] = "%s";\n' % (idx, enum_value, enum_value_string));

# ---------------------------------------------------------------------------------------
# This function adds info for PM4 packets as well as structs that have no opcodes (e.g. V#s/T#s/S#s)
def outputPacketInfo(pm4_info_file, registers_et_root, enum_index_dict, opcode_dict):
  domains = registers_et_root.findall('{http://nouveau.freedesktop.org/}domain')

  # Find all CP packet types so we can find out which domains are relevant
  pm4_type_packets = registers_et_root.find('./{http://nouveau.freedesktop.org/}enum[@name="adreno_pm4_type3_packets"]')

  # Get highest opcode value to properly resize() the vector
  highest_opcode = 0
  for domain in domains:
    domain_name = domain.attrib['name']

    # Check if it is a domain describing a PM4 packet
    pm4_type_packet = pm4_type_packets.find('./{http://nouveau.freedesktop.org/}value[@name="'+domain_name+'"]')
    if (pm4_type_packet is None) and (not domain_name.startswith('A6XX_')):
      continue

    opcode = 0
    if pm4_type_packet is not None:
      opcode = int(pm4_type_packet.attrib['value'],0)
      if opcode_dict[opcode] != domain_name:
        continue

    if highest_opcode < opcode:
      highest_opcode = opcode

  pm4_info_file.write('    g_sPacketInfo.resize(0x%x);\n' % (highest_opcode+1))

  ############################################################################
  packet_type_instances = {}
  for domain in domains:
    domain_name = domain.attrib['name']

    # Check if it is a domain describing a PM4 packet, OR see if it a 'A6XX_' packet (e.g. V#s/T#s/S#s)
    pm4_type_packet = pm4_type_packets.find('./{http://nouveau.freedesktop.org/}value[@name="'+domain_name+'"]')
    if (pm4_type_packet is None) and (not domain_name.startswith('A6XX_')):
      continue

    # Make sure the opcode_dict is referring to the same packet name
    # This can differ if this PM4 definition is for an older GPU
    # These opcodes are repurposed across different generations
    opcode = 0
    if pm4_type_packet is not None:
      opcode = int(pm4_type_packet.attrib['value'],0)
      if opcode_dict[opcode] != domain_name:
        continue

    # There are 2 PM4 packet types with stripe tags, see CP_DRAW_INDIRECT_MULTI and CP_DRAW_INDX_INDIRECT for example
    # For CP_DRAW_INDX_INDIRECT type, the stripe is based on HW version, so just use the latest stripe only
    # For CP_DRAW_INDIRECT_MULTI type, we will have to first determine what field determines the variant
    array = domain.find('./{http://nouveau.freedesktop.org/}array')
    array_size = 1
    pm4_packet = domain
    if array is not None:
      # Double check that there are no registers in this domain (i.e. they're all within the array block)
      if domain.find('./{http://nouveau.freedesktop.org/}reg32') or \
         domain.find('./{http://nouveau.freedesktop.org/}reg64'):
        raise Exception('Unexpected top-level registers found in a PM4 packet with arrays: ' + domain_name)
      pm4_packet = array
      array_size = int(array.attrib['length'])

    # There are 3 PM4 packets with stripe tags: CP_DRAW_INDIRECT_MULTI, CP_DRAW_INDX_INDIRECT, and CP_EVENT_WRITE7
    # For CP_DRAW_INDX_INDIRECT, the stripe is based on HW version, so just use the latest stripe only
    # For CP_DRAW_INDIRECT_MULTI/CP_EVENT_WRITE7, we will have to first determine what field determines the variant
    variant_list = []
    stripes = domain.findall('./{http://nouveau.freedesktop.org/}stripe')
    for idx, stripe in enumerate(stripes):
      varset = stripe.attrib['varset']
      variants = stripe.attrib['variants']
      if varset == 'chip':  # For chip-based variants, add only the last one
        if idx == len(stripes) - 1:
          variant_list.append((variants, stripe))
      else:
        variant_list.append((variants, stripe))

    # If there are no variants, add a 'default' variant
    if len(variant_list) == 0:
      variant_list.append(('default', None))

    for variant in variant_list:
      # Filter out everything but the reg32 and reg64 elements from the packet definition
      # First let's add it to a dict so we can ignore duplicates (some stripes redefine root registers)
      reg_dict = {}
      skip_this_packet = False
      for element in pm4_packet:
        is_reg_32 = (element.tag == '{http://nouveau.freedesktop.org/}reg32')
        is_reg_64 = (element.tag == '{http://nouveau.freedesktop.org/}reg64')
        if is_reg_32 or is_reg_64:
          offset = int(element.attrib['offset'],0)
          # There are certain packets (e.g. A6XX_PDC) which have register offsets
          # instead of packet offsets, for some weird reason. Skip those packets.
          if offset > 1000:
            skip_this_packet = True
            break
          if not (offset in reg_dict):
            reg_dict[offset] = element
      if skip_this_packet:
        break

      # Add the registers from the variant-specific section (i.e. stripe)
      stripe = variant[1]
      packet_name = domain_name
      if stripe is not None:
        for element in variant[1]:
          is_reg_32 = (element.tag == '{http://nouveau.freedesktop.org/}reg32')
          is_reg_64 = (element.tag == '{http://nouveau.freedesktop.org/}reg64')
          if is_reg_32 or is_reg_64:
            offset = int(element.attrib['offset'],0)
            if not (offset in reg_dict):
              reg_dict[offset] = element

        # Add the prefix to the packet_name
        if 'prefix' in stripe.attrib:
          prefix = stripe.attrib['prefix']
          packet_name = domain_name + '_' + prefix

      # Determine stripe opcode for this variant/stripe
      stripe_variant = 'UINT8_MAX'
      if stripe is not None:
        varset = stripe.attrib['varset']
        if varset != 'chip':
          enum = domain.find('./{http://nouveau.freedesktop.org/}enum[@name="'+varset+'"]')
          enum_value = enum.find('./{http://nouveau.freedesktop.org/}value[@name="'+variant[0]+'"]')
          stripe_variant = enum_value.attrib['value']

      # Convert dict to list
      reg_list = list(reg_dict.values())

      # Sort based on offset
      reg_list = sorted(reg_list, key=lambda x: int(x.attrib['offset'],0))

      # Keep track of instance #. Only the 1st instance belongs in the vector. The rest are in the multimap.
      if opcode not in packet_type_instances:
        packet_type_instances[opcode] = 1
        if (opcode == 0):
          pm4_info_file.write('''    
    // For descriptors, we purposefully try to include them as "packets" for easier parsing.
    // They are not technically PM4 packets, hence the 0x0.
    // Example: const PacketInfo *packet_info_ptr = GetPacketInfo(0, sharp_struct_name);
''')

        pm4_info_file.write('    g_sPacketInfo[0x%x] = { "%s", %d, %s, {' % (opcode, packet_name, array_size, stripe_variant))
        outputPacketFields(pm4_info_file, enum_index_dict, reg_list)
        pm4_info_file.write(' } };\n')
      else:
        packet_type_instances[opcode] += 1
        pm4_info_file.write('    g_sPacketInfoMultiple.insert(std::pair<uint32_t, PacketInfo>(')
        pm4_info_file.write('0x%x, { "%s", %d, %s, {' % (opcode, packet_name, array_size, stripe_variant))
        outputPacketFields(pm4_info_file, enum_index_dict, reg_list)
        pm4_info_file.write(' } }));\n')

  # Not all pm4 packets are described via a 'domain'. These are usually packets (such as CP_WAIT_FOR_IDLE) which
  # have no fields. In that case, add a corresponding g_sPacketInfo entry with no fields
  pm4_type_packets_values = pm4_type_packets.findall('./{http://nouveau.freedesktop.org/}value')
  for pm4_type_packet_value in pm4_type_packets_values:
    # See if it shows up in the domains list
    packet_name = pm4_type_packet_value.attrib['name']

    domain = registers_et_root.find('{http://nouveau.freedesktop.org/}domain[@name="'+packet_name+'"]')
    if domain is None:
      opcode = int(pm4_type_packet_value.attrib['value'],0)

      # We need the g_sPacketInfoVariant because some PM4s share the same value 
      # but with different variants (CP_THREAD_CONTROL (A7XX-) and IN_IB_PREFETCH_END (A2XX) both use 0x17)
      if 'variants' in pm4_type_packet_value.attrib:
        variants = pm4_type_packet_value.attrib['variants']
        variants_bitfield = GetGPUVariantsBitField(variants)
        if (variants_bitfield != 0):
          # kGPUVariantsBits has 6 bits
          # it seems that the variant is only used for the non-domain ones
          for i in range(6):
            cur_variant_bitfield = (1<<i)
            if cur_variant_bitfield & variants_bitfield:
              pm4_info_file.write('    g_sPacketInfoVariant[(0x%x << kGPUVariantsBits) | 0x%x] = { "%s", 0, UINT8_MAX, {' % (opcode, cur_variant_bitfield, packet_name) + ' } };\n')
      else:
        pm4_info_file.write('    g_sPacketInfo[0x%x] = { "%s", 0, UINT8_MAX, {' % (opcode, packet_name) + ' } };\n')
      
  pm4_info_file.write('\n')

  # Append _A?XX to the name if there is any variant
  # This is to handle the cases where the regsiters have the same name
  # but different offset for different variants, like PC_POLYGON_MODE
  pm4_info_file.writelines('''
  for (uint64_t i = 0; i < g_sRegInfo.size(); ++i)
  {
    if (g_sRegInfo[i].m_name != nullptr)
    {
      g_sRegNameToIndex[g_sRegInfo[i].m_name] = (uint32_t)i;
    }
  }
	for (auto &reg : g_sRegInfoVariant)
	{
		const std::string& name = reg.second.m_name;
		const uint32_t shift_bits = 32 - kGPUVariantsBits;
		uint32_t gpu_variants = reg.first << shift_bits >> shift_bits;
		uint32_t reg_offset = reg.first >> kGPUVariantsBits;
		if (gpu_variants != 0)
		{
			uint32_t bit_offset = 0;
			while(gpu_variants != 0)
			{
				if ((gpu_variants & 0x1) != 0)
				{
					const std::string name_with_variant = name + "_" + GetGPUStr(static_cast<GPUVariantType>(1 << (bit_offset)));
					g_sRegNameToIndex[name_with_variant] = reg_offset;
				}
				gpu_variants = gpu_variants>>1;
				++bit_offset;
			}
		}
		else
		{
			g_sRegNameToIndex[name] = reg_offset;
		}
	}\n''')

# ---------------------------------------------------------------------------------------

def outputFunctionsCpp(pm4_info_file):
  pm4_info_file.writelines('''
const char *GetOpCodeString(uint32_t op_code)
{
    return g_sOpCodeToString[op_code];
}

const RegInfo *GetRegInfo(uint32_t reg)
{
    // check without variant as key
    if (g_sRegInfo[reg].m_name == nullptr)
    {
        // check with variant as key
        uint32_t key = (reg << kGPUVariantsBits) | g_sGPU_variant;
        auto it = g_sRegInfoVariant.find(key);
        if (it == g_sRegInfoVariant.end())
        {
            return nullptr;
        }
        return &it->second;
    }
    return &g_sRegInfo[reg];
}

const RegInfo *GetRegByName(const char *name)
{
    uint32_t offset = GetRegOffsetByName(name);
    if(offset == kInvalidRegOffset)
    {
        return nullptr;
    }
    return GetRegInfo(offset);
}

const RegField *GetRegFieldByName(const char *name, const RegInfo *info)
{
    if (info == nullptr)
        return nullptr;

    const DiveVector<RegField> &field = info->m_fields;
    auto i = std::find_if(field.begin(), field.end(), [&](const RegField& f) {
        return strcmp(name, f.m_name) == 0;
    });

    if (i == info->m_fields.end())
        return nullptr;
    return &(*i);
}

uint32_t GetRegOffsetByName(const char *name)
{
    DIVE_ASSERT(g_sGPU_variant != kGPUVariantNone);
    std::string str = std::string(name);
    auto i = g_sRegNameToIndex.find(str);
    if (i == g_sRegNameToIndex.end())
    {
        std::string name_with_variant = str + "_" + GetGPUStr(g_sGPU_variant);
        i = g_sRegNameToIndex.find(name_with_variant);
        if (i == g_sRegNameToIndex.end())
        {
            return kInvalidRegOffset;
        }
    }
    return i->second;
}

const char *GetEnumString(uint32_t enum_handle, uint32_t val)
{
    if (g_sEnumReflection.size() <= enum_handle)
        return nullptr;
    if (g_sEnumReflection[enum_handle].size() <= val)
        return nullptr;
    return g_sEnumReflection[enum_handle][val];
}

const PacketInfo *GetPacketInfo(uint32_t op_code)
{
    // check without variant as key
    if (g_sPacketInfo[op_code].m_name == nullptr)
    {
        // check with variant as key
        uint32_t key = (op_code << kGPUVariantsBits) | g_sGPU_variant;
        auto it = g_sPacketInfoVariant.find(key);
        if (it == g_sPacketInfoVariant.end())
        {
            return nullptr;
        }
        return &it->second;
    }

    return &g_sPacketInfo[op_code];
}

const PacketInfo *GetPacketInfo(uint32_t op_code, const char *name)
{
    if (g_sPacketInfo[op_code].m_name == nullptr)
        return nullptr;
    if (strcmp(g_sPacketInfo[op_code].m_name, name) == 0)
        return &g_sPacketInfo[op_code];
    auto ret_pair = g_sPacketInfoMultiple.equal_range(op_code);
    for (auto it  = ret_pair.first; it != ret_pair.second; ++it) {
        if (strcmp(it->second.m_name, name) == 0)
            return &it->second;
    }
    return nullptr;
}

void SetGPUID(uint32_t gpu_id)
{
    g_sGPU_id = gpu_id;
    uint32_t gpu_series = gpu_id / 100;
    if((gpu_series >= 2) && (gpu_series <= 7))
    {
        g_sGPU_variant = static_cast<GPUVariantType>(1 << (gpu_series - 2));
    }
    else
    {
        g_sGPU_variant = kGPUVariantNone;
    }
}

uint32_t GetGPUID()
{
    return g_sGPU_id;
}

GPUVariantType GetGPUVariantType()
{
    return g_sGPU_variant;
}

bool IsFieldEnabled(const RegField* field)
{
    DIVE_ASSERT(g_sGPU_variant != kGPUVariantNone);
    return (g_sGPU_variant & field->m_gpu_variants) != 0;
}
'''
  )

# ---------------------------------------------------------------------------------------
if len(sys.argv) != 4:
  print(sys.argv[0] + ' <Path to adreno register file> <Path to output file base-name>')
  sys.exit()

try:

  register_root = sys.argv[1]
  register_file = sys.argv[2]

  # create element tree object
  tree = ET.parse(sys.argv[1]+'/'+sys.argv[2])

  # get root element
  registers_et_root = tree.getroot()

  # import any additional xml files referenced in the base xml file via "import" tags
  sub_xml_file_set = set()
  for child in registers_et_root:
    if child.tag == '{http://nouveau.freedesktop.org/}import':
      if 'file' in child.attrib:
        sub_xml_file = sys.argv[1]+'/'+child.attrib['file']

        # Check if this file has already been parsed or not
        if sub_xml_file not in sub_xml_file_set:
          sub_xml_file_set.add(sub_xml_file)
          sub_tree = ET.parse(sys.argv[1]+'/'+child.attrib['file'])
          for sub_tree_child in sub_tree.getroot():
            registers_et_root.append(sub_tree_child)

  pm4_info_file_h = open(sys.argv[3] + '.h', 'w')
  pm4_info_file_cpp = open(sys.argv[3] + '.cpp', 'w')

  head, tail = os.path.split(sys.argv[3] + '.h')
  pm4_info_filename_h = tail

  addMissingDomains(registers_et_root)

  # Parse type3 opcodes
  # There can be multiple opcodes with the same value, to support different adreno versions
  # The opcodes are listed in hw revision order, with later entries of the same opcode used for later revisions
  # We only want the opcode names for the latest revision, so overwrite the dictionary if the same value encountered

  # Find the "adreno_pm4_type3_packets" enum (defined in the andreno_pm4.xml file)
  opcode_dict = {}
  pm4_type_packets = registers_et_root.find('./{http://nouveau.freedesktop.org/}enum[@name="adreno_pm4_type3_packets"]')
  for pm4_type_packet in pm4_type_packets:
    if 'name' in pm4_type_packet.attrib and 'value' in pm4_type_packet.attrib:
      pm4_name = pm4_type_packet.attrib['name']
      pm4_value = pm4_type_packet.attrib['value']
      # Only add ones that start with "CP_*" or "A6XX_*"
      if pm4_name.startswith('CP_') or pm4_name.startswith('A6XX_'):
        opcode_dict[int(pm4_value,0)] = pm4_name

  # .H file
  outputH(pm4_info_file_h)

  # .CPP file
  outputHeaderCpp(pm4_info_filename_h, pm4_info_file_cpp)
  outputPm4InfoInitFunc(pm4_info_file_cpp, registers_et_root, opcode_dict)
  outputFunctionsCpp(pm4_info_file_cpp)

  # close to flush
  pm4_info_file_cpp.close();
  pm4_info_file_h.close();

  # lint
  format_cmd = 'clang-format -i -style=file ' + sys.argv[2]
  format_cmd_h = format_cmd + '.h'
  format_cmd_cpp = format_cmd + '.cpp'
  print('formatting ' + format_cmd_h)
  os.system(format_cmd_h)
  print('formatting ' + format_cmd_cpp)
  os.system(format_cmd_cpp)

except IOError as e:
    errno, strerror = e.args
    print('I/O error({0}): {1}'.format(errno,strerror))
    # e can be printed directly without using .args:
    # print(e)
except:
  print('Unexpected error:', sys.exc_info()[0])
  raise

print('%s: %s file generated' % (sys.argv[0], sys.argv[2]))

