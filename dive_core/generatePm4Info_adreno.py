from __future__ import print_function
import sys
import os
import re
import xml.etree.ElementTree as ET


# ---------------------------------------------------------------------------------------
def outputHeader(pm4_info_file):
  pm4_info_file.writelines(
    " /*\n"
    " Copyright 2020 Google LLC\n"
    "\n"
    " Licensed under the Apache License, Version 2.0 (the \"License\";\n"
    " you may not use this file except in compliance with the License.\n"
    " You may obtain a copy of the License at\n"
    "\n"
    " http://www.apache.org/licenses/LICENSE-2.0\n"
    "\n"
    " Unless required by applicable law or agreed to in writing, software\n"
    " distributed under the License is distributed on an \"AS IS\" BASIS,\n"
    " WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n"
    " See the License for the specific language governing permissions and\n"
    " limitations under the License.\n"
    "*/\n"
    "\n"
    "///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////\n"
    "//\n"
    "// WARNING!  WARNING!  WARNING!  WARNING!  WARNING!  WARNING!  WARNING!  WARNING!  WARNING!  WARNING!  WARNING!\n"
    "//\n"
    "// This code has been generated automatically by generatePm4Strings_adreno.py. Do not hand-modify this code.\n"
    "//\n"
    "///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////\n"
  )

# ---------------------------------------------------------------------------------------
def outputH(pm4_info_file):
  outputHeader(pm4_info_file)
  pm4_info_file.writelines("""
#pragma once

#include <vector>
#include <stdint.h>
#include <string.h>

struct RegField {
    uint32_t m_shift;
    uint32_t m_mask;
    const char* m_name;
};

struct RegInfo {
    const char* m_name;
    std::vector<RegField> m_fields;
};

struct PacketField {
    const char* m_name;
    uint32_t m_is_variant_opcode : 1; // If 1, then is used to indicate variant
    uint32_t m_dword             : 31;
    uint32_t m_enum_handle;
    uint32_t m_shift;
    uint32_t m_mask;
};

struct PacketInfo {
    const char* m_name;
    uint32_t m_max_array_size;
    uint32_t m_stripe_variant;  // Which variant of the packet this is
    std::vector<PacketField> m_fields;
};

void Pm4InfoInit();
const char *GetOpCodeString(uint32_t op_code);
const RegInfo *GetRegInfo(uint32_t reg);
const RegInfo *GetRegByName(const char *);
const char *GetEnumString(uint32_t enum_handle, uint32_t val);
const PacketInfo *GetPacketInfo(uint32_t op_code);
const PacketInfo *GetPacketInfo(uint32_t op_code, const char *name);
""")

# ---------------------------------------------------------------------------------------
def outputHeaderCpp(pm4_info_header_file_name, pm4_info_file):
  outputHeader(pm4_info_file)
  pm4_info_file.write("#include \"%s\"\n" % (pm4_info_header_file_name))
  pm4_info_file.writelines("""
#include <cstring>
#include <map>
#include <vector>
#include <assert.h>

static std::map<uint32_t, const char*> g_sOpCodeToString;
static std::map<uint32_t, RegInfo> g_sRegInfo;
struct cmp_str
{
    bool operator()(char const *a, char const *b) const
    {
        return std::strcmp(a, b) < 0;
    }
};
static std::map<const char *, uint32_t, cmp_str> g_sRegNameToIndex;
static std::vector<std::map<uint32_t, const char*>> g_sEnumReflection;
static std::multimap<uint32_t, PacketInfo> g_sPacketInfo;
"""
  )

# ---------------------------------------------------------------------------------------
def outputPm4InfoInitFunc(pm4_info_file, registers_et_root, opcode_dict):
  pm4_info_file.writelines(
    "void Pm4InfoInit()\n"
    "{\n"
    "    assert(g_sOpCodeToString.empty());\n"
    "    assert(g_sRegInfo.empty());\n"
    "    assert(g_sEnumReflection.empty());\n"
    "    assert(g_sPacketInfo.empty());\n"
    "\n"
  )
  outputOpcodes(pm4_info_file, opcode_dict)
  pm4_info_file.write("\n")
  outputRegisterInfo(pm4_info_file, registers_et_root)
  pm4_info_file.write("\n")
  outputPacketInfo(pm4_info_file, registers_et_root)
  pm4_info_file.write("}\n")
  return

valid_opcodes = {}
# ---------------------------------------------------------------------------------------
def outputOpcodes(pm4_info_file, opcode_dict):
  for opcode in opcode_dict:
    pm4_info_file.write("    g_sOpCodeToString[%s] = \"%s\";\n" % (opcode, opcode_dict[opcode]))
  return

# ---------------------------------------------------------------------------------------
def outputSingleRegister(pm4_info_file, registers_et_root, a6xx_domain, offset, name, bitfields, type):
    pm4_info_file.write("    g_sRegInfo[0x%x] = { \"%s\", {" % (offset, name))

    # if a 'type' attrib is available, then check for a custom bitset
    if type:
      # if it isn't one of the standard types, then it must be a custom bitset or an enum
      if type != "boolean" and type != "uint" and type != "int" and type != "float" \
        and type != "fixed" and type != "waddress" and type != "hex":
        bitfields = a6xx_domain.find('./{http://nouveau.freedesktop.org/}bitset[@name="'+type+'"]')

        # Not found in the A6XX domain. Some of the bitsets are defined in the root.
        if not bitfields:
          bitfields = registers_et_root.find('./{http://nouveau.freedesktop.org/}bitset[@name="'+type+'"]')

        # It's not a bitset. Let's sanity check that it's a top-level enum!
        if not bitfields:
          bitfields = []  # Create an empty list instead of a "None" object
          enum = registers_et_root.find('./{http://nouveau.freedesktop.org/}enum[@name="'+type+'"]')
          if not enum:
            raise Exception("Not able to find bitset/enum " + type + " for register " + name)

    # Iterate through optional bitfields
    for bitfield in bitfields:
      if bitfield.tag == '{http://nouveau.freedesktop.org/}doc':
        continue

      name = bitfield.attrib['name']

      # TODO: Store the shr bits so we know how much to shift left to extract the "original" value
      # TODO: Store the type, align, and variants bits. The 'type' field in particular should reference
      #  an enum value when appropriate

      # Convert to mask & shift
      if 'low' in bitfield.attrib and 'high' in bitfield.attrib:
        shift = low = int(bitfield.attrib['low'])
        high = int(bitfield.attrib['high'])
        mask = (0xffffffff >> (31 - (high - low))) << low
      elif 'pos' in bitfield.attrib:
        shift = pos = int(bitfield.attrib['pos'])
        mask = (0x1 << pos)
      else:
        raise Exception("Encountered a bitfield with no pos/low/high!")

      pm4_info_file.write("    { %d, 0x%x, \"%s\" }, "  % (
          shift,
          mask,
          name
        ))
    pm4_info_file.write("} };\n")

# ---------------------------------------------------------------------------------------
def outputRegisterInfo(pm4_info_file, registers_et_root):
  a6xx_domain = registers_et_root.find('./{http://nouveau.freedesktop.org/}domain[@name="A6XX"]')

  # TODO: Store the register type information somewhere

  # Parse through reg32 registers
  reg32s = a6xx_domain.findall('{http://nouveau.freedesktop.org/}reg32')
  bitsets = a6xx_domain.findall('{http://nouveau.freedesktop.org/}bitset')
  for reg32 in reg32s:
    offset = int(reg32.attrib['offset'],0)
    name = reg32.attrib['name']
    bitfields = reg32.findall('{http://nouveau.freedesktop.org/}bitfield')
    type = None
    if 'type' in reg32.attrib:
      type = reg32.attrib['type']
    outputSingleRegister(pm4_info_file, registers_et_root, a6xx_domain, offset, name, bitfields, type)

  # Iterate and output the reg64s as 2 32-bit registers
  # TODO: Mark these are 64-bit registers so they can be merged back into 1 value later in the UI
  reg64s = a6xx_domain.findall('{http://nouveau.freedesktop.org/}reg64')
  for reg64 in reg64s:
    # Assumption: reg64s should not have any children
    if len(list(reg64)) > 0:
      raise Exception("Reg64s should have no children!")

    offset = int(reg64.attrib['offset'],0)
    name = reg64.attrib['name']
    outputSingleRegister(pm4_info_file, registers_et_root, a6xx_domain, \
                         offset, name+"_LO", [], None)
    outputSingleRegister(pm4_info_file, registers_et_root, a6xx_domain, \
                         offset+1, name+"_HI", [], None)

  # Iterate and output the arrays as a sequence of reg32s with an index as a suffix
  arrays = a6xx_domain.findall('{http://nouveau.freedesktop.org/}array')
  for array in arrays:
    offset = int(array.attrib['offset'],0)
    array_name = array.attrib['name']
    stride = int(array.attrib['stride'])
    length = int(array.attrib['length'])

    reg32s = array.findall('{http://nouveau.freedesktop.org/}reg32')

    # Arrays with stride==1 just generate a series of registers with index suffixes
    # Arrays with stride==2 and no reg32s are going to generate 2 registers with _LO/_HI suffixes
    # Arrays with stride>2 must have reg32s and will generate registers as follows:
    #   Ex: Array name: "BASE", Length: 2, Reg32 names: "ZIG/ZAG"
    #       -> 4 registers with names: BASE0_ZIG, BASE0_ZAG, BASE1_ZIG, BASE1_ZAG
    for i in range(0,length):
      if stride == 1:
        outputSingleRegister(pm4_info_file, registers_et_root, a6xx_domain, \
                             offset+i, array_name+str(i), [], None)
      elif stride == 2 and not reg32s:
        outputSingleRegister(pm4_info_file, registers_et_root, a6xx_domain, \
                             offset+i*stride, array_name+"_LO", [], None)
        outputSingleRegister(pm4_info_file, registers_et_root, a6xx_domain, \
                             offset+i*stride+1, array_name+"_HI", [], None)
      else:
        for reg_idx, reg32 in enumerate(reg32s):
          reg_name = reg32.attrib['name']
          bitfields = reg32.findall('{http://nouveau.freedesktop.org/}bitfield')
          type = None
          if 'type' in reg32.attrib:
            type = reg32.attrib['type']
          outputSingleRegister(pm4_info_file, registers_et_root, a6xx_domain, \
                               offset+i*stride+reg_idx, array_name+str(i)+"_"+reg_name, \
                               bitfields, type)
  return

# ---------------------------------------------------------------------------------------
def parseEnumInfo(enum_index_dict, enum_list, registers_et_root):
  enums = registers_et_root.findall('{http://nouveau.freedesktop.org/}enum')

  # Find all enums within domain blocks
  # Those are reserved for use within the domain, but we need to keep track of all
  # enums in our list, irrespective of scope
  domains = registers_et_root.findall('{http://nouveau.freedesktop.org/}domain')
  for domain in domains:
    domain_enums = domain.findall('{http://nouveau.freedesktop.org/}enum')
    if domain_enums:
      for enum in domain_enums:
        enums.append(enum)

  for enum in enums:
    fields = enum.findall('{http://nouveau.freedesktop.org/}value')
    enum_name = enum.attrib['name']

    # There shouldn't be any repeat entries
    if enum_name in enum_index_dict:
      raise Exception("Encountered multiple enums with same name: " + enum_name)

    enum_index_dict[enum_name] = len(enum_list)
    enum_list.append((enum_name, dict()))

    for field in fields:
      enum_value_name = field.attrib['name']

      # Fields such as INDEX_SIZE_INVALID do not have a value and should be ignored here
      if 'value' in field.attrib:
        enum_value = int(field.attrib['value'], 0)

        # enum_list is an array of {string, dict()}, where the key of the dict() is
        # the integer enum_value
        # Note: It's possible to have multiple enum fields with the same value, for different
        # variants. These fields are listed in order of hw variants, with later entries covering
        # more recent hw. Since we are only interested in the latest hardware, we can just
        # override the earlier value name and use the newer name for the field
        index = enum_index_dict[enum_name]
        enum_list[index][1][enum_value] = enum_value_name

# ---------------------------------------------------------------------------------------
def outputField(pm4_info_file, field_name, is_variant_opcode, dword, enum_handle, shift, mask):
  pm4_info_file.write("{ \"%s\", %d, %d, %s, %d, 0x%x }," %
                       (field_name, is_variant_opcode, dword, enum_handle, shift, mask))

# ---------------------------------------------------------------------------------------
def outputStructRegs(pm4_info_file, enum_index_dict, reg_list):
  dword_count = 0
  for element in reg_list:
    is_reg_32 = (element.tag == '{http://nouveau.freedesktop.org/}reg32')
    is_reg_64 = (element.tag == '{http://nouveau.freedesktop.org/}reg64')
    offset = int(element.attrib['offset'],0)

    # Sanity check
    # Note: Allowed to skip an offset (see CP_EVENT_WRITE7)
    if dword_count > offset:
        raise Exception("Unexpected reverse offset found in packet")

    dword_count = dword_count + 1
    if is_reg_64:
      dword_count = dword_count + 1

    bitfields = element.findall('{http://nouveau.freedesktop.org/}bitfield')

    # No bitfields, so use the register specification directly
    if len(bitfields) == 0:
      field_name = element.attrib['name']
      shift = 0
      mask = int('0xffffffff', 16)
      enum_handle = "UINT32_MAX"

      # if 'addvariant' is an attribute, then this field is used to determine the packet variant
      is_variant_opcode = 0
      if 'addvariant' in element.attrib:
        is_variant_opcode = 1
      if is_reg_32:
        outputField(pm4_info_file, field_name, is_variant_opcode, dword_count, enum_handle, shift, mask)
      elif is_reg_64:
        outputField(pm4_info_file, field_name+"_LO", is_variant_opcode, dword_count-1, enum_handle, shift, mask)
        outputField(pm4_info_file, field_name+"_HI", is_variant_opcode, dword_count, enum_handle, shift, mask)

    if is_reg_64 and len(bitfields) > 0:
      raise Exception("Found a reg64 with bitfields: " + field_name)

    for bitfield in bitfields:
      field_name = bitfield.attrib['name']
      type = None
      if 'type' in bitfield.attrib:
        type = bitfield.attrib['type']

      enum_handle = "UINT32_MAX"
      if type and type != "boolean" and type != "uint" and type != "int" and type != "float" \
        and type != "fixed" and type != "address" and type != "hex":
        if type not in enum_index_dict:
          raise Exception("Enumeration %s not found!" % type)
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
        raise Exception("Encountered a bitfield with no pos/low/high!")

      # if 'addvariant'  is an attribute, then this field is used to determine the packet variant
      is_variant_opcode = 0
      if 'addvariant' in bitfield.attrib:
        is_variant_opcode = 1
      outputField(pm4_info_file, field_name, is_variant_opcode, dword_count, enum_handle, shift, mask)

# ---------------------------------------------------------------------------------------
def outputStructInfo(pm4_info_file, enum_index_dict, op_code_set, registers_et_root):
  domains = registers_et_root.findall('{http://nouveau.freedesktop.org/}domain')

  # Find all CP packet types so we can find out which domains are relevant
  pm4_type_packets = registers_et_root.find('./{http://nouveau.freedesktop.org/}enum[@name="adreno_pm4_type3_packets"]')

  for domain in domains:
    domain_name = domain.attrib['name']

    # Check if it is a domain describing a PM4 packet
    pm4_type_packet = pm4_type_packets.find('./{http://nouveau.freedesktop.org/}value[@name="'+domain_name+'"]')
    if pm4_type_packet is None:
      continue

    # There are only 2 PM4 packets with array tags: CP_SET_DRAW_STATE and CP_SET_PSEUDO_REG
    # Both of these have arrays that encapsulate the whole packet
    array = domain.find('./{http://nouveau.freedesktop.org/}array')
    array_size = 1
    pm4_packet = domain
    if array:
      # Double check that there are no registers in this domain (i.e. they're all within the array block)
      if domain.find('./{http://nouveau.freedesktop.org/}reg32') or \
         domain.find('./{http://nouveau.freedesktop.org/}reg64'):
        raise Exception("Unexpected top-level registers found in a PM4 packet with arrays: " + domain_name)
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
      if varset == "chip":  # For chip-based variants, add only the last one
        if idx == len(stripes) - 1:
          variant_list.append((variants, stripe))
      else:
        variant_list.append((variants, stripe))

    # If there are no variants, add a "default" variant
    if len(variant_list) == 0:
      variant_list.append(("default", None))

    for variant in variant_list:
      # Filter out everything but the reg32 and reg64 elements from the packet definition
      # First let's add it to a dict so we can ignore duplicates (some stripes redefine root registers)
      reg_dict = {}
      for element in pm4_packet:
        is_reg_32 = (element.tag == '{http://nouveau.freedesktop.org/}reg32')
        is_reg_64 = (element.tag == '{http://nouveau.freedesktop.org/}reg64')
        if is_reg_32 or is_reg_64:
          offset = int(element.attrib['offset'])
          if not (offset in reg_dict):
            reg_dict[offset] = element

      # Add the registers from the variant-specific section (i.e. stripe)
      stripe = variant[1]
      packet_name = domain_name
      if stripe:
        for element in variant[1]:
          is_reg_32 = (element.tag == '{http://nouveau.freedesktop.org/}reg32')
          is_reg_64 = (element.tag == '{http://nouveau.freedesktop.org/}reg64')
          if is_reg_32 or is_reg_64:
            offset = int(element.attrib['offset'])
            if not (offset in reg_dict):
              reg_dict[offset] = element
        # Add the prefix to the packet_name
        if 'prefix' in stripe.attrib:
          prefix = stripe.attrib['prefix']
          packet_name = domain_name + "_" + prefix

      # Determine stripe opcode for this variant/stripe
      stripe_variant = "UINT32_MAX"
      if stripe:
        varset = stripe.attrib['varset']
        if varset != "chip":
          enum = domain.find('./{http://nouveau.freedesktop.org/}enum[@name="'+varset+'"]')
          enum_value = enum.find('./{http://nouveau.freedesktop.org/}value[@name="'+variant[0]+'"]')
          stripe_variant = enum_value.attrib['value']

      # Convert dict to list
      reg_list = list(reg_dict.values())

      # Sort based on offset
      reg_list = sorted(reg_list, key=lambda x: int(x.attrib['offset']))

      opcode = int(pm4_type_packet.attrib['value'],0)
      pm4_info_file.write("    g_sPacketInfo.insert(std::pair<uint32_t, PacketInfo>(")
      pm4_info_file.write("0x%x, { \"%s\", %d, %s, {" % (opcode, packet_name, array_size, stripe_variant))
      outputStructRegs(pm4_info_file, enum_index_dict, reg_list)
      pm4_info_file.write(" } }));\n")

  # Not all pm4 packets are described via a "domain". These are usually packets (such as CP_WAIT_FOR_IDLE) which
  # have no fields. In that case, add a corresponding g_sPacketInfo entry with no fields
  pm4_type_packets_values = pm4_type_packets.findall('./{http://nouveau.freedesktop.org/}value')
  for pm4_type_packet_value in pm4_type_packets_values:
    # See if it shows up in the domains list
    packet_name = pm4_type_packet_value.attrib['name']

    domain = registers_et_root.find('{http://nouveau.freedesktop.org/}domain[@name="'+packet_name+'"]')
    if domain is None:
      opcode = int(pm4_type_packet_value.attrib['value'],0)
      pm4_info_file.write("    g_sPacketInfo.insert(std::pair<uint32_t, PacketInfo>(")
      pm4_info_file.write("0x%x, { \"%s\", 0, UINT32_MAX, {" % (opcode, packet_name))
      pm4_info_file.write(" } }));\n")


# ---------------------------------------------------------------------------------------
def outputPacketInfo(pm4_info_file, registers_et_root):
  enum_index_dict = {}

  # Get enum values from the XML element tree, being careful not to have duplicates
  enum_list = []
  parseEnumInfo(enum_index_dict, enum_list, registers_et_root)

  # Output enum_list to file
  for idx, enum_info in enumerate(enum_list):
    # enum_list is an array of {string, dict()}, where the key of the dict() is
    # the integer enum_value
    pm4_info_file.write("    g_sEnumReflection.push_back({")
    for enum_value, enum_value_string in sorted(enum_info[1].items()):
      pm4_info_file.write(" { %d, \"%s\" }," % (enum_value, enum_value_string))
    pm4_info_file.write(" }); // %s (%d)\n" % (enum_info[0], idx))
  pm4_info_file.write("\n")

  # Parse packet values from each of the 4 different files, being careful not to have duplicates
  op_code_set = set()
  outputStructInfo(pm4_info_file, enum_index_dict, op_code_set, registers_et_root)
  pm4_info_file.write("\n")

# ---------------------------------------------------------------------------------------

def outputFunctionsCpp(pm4_info_file):
  pm4_info_file.writelines("""
const char *GetOpCodeString(uint32_t op_code)
{
    if (g_sOpCodeToString.find(op_code) == g_sOpCodeToString.end())
        return nullptr;
    return g_sOpCodeToString.find(op_code)->second;
}

const RegInfo *GetRegInfo(uint32_t reg)
{
    if (g_sRegInfo.find(reg) == g_sRegInfo.end())
        return nullptr;
    return &g_sRegInfo.find(reg)->second;
}

const RegInfo *GetRegByName(const char *name)
{
    auto i = g_sRegNameToIndex.find(name);
    if (i == g_sRegNameToIndex.end())
        return nullptr;
    return GetRegInfo(i->second);
}

const char *GetEnumString(uint32_t enum_handle, uint32_t val)
{
    if (g_sEnumReflection.size() <= enum_handle)
        return nullptr;
    const std::map<uint32_t, const char*> &val_to_str_map = g_sEnumReflection[enum_handle];
    if (val_to_str_map.find(val) == val_to_str_map.end())
        return nullptr;
    return val_to_str_map.find(val)->second;
}

const PacketInfo *GetPacketInfo(uint32_t op_code)
{
    if (g_sPacketInfo.find(op_code) == g_sPacketInfo.end())
        return nullptr;
    return &g_sPacketInfo.find(op_code)->second;
}

const PacketInfo *GetPacketInfo(uint32_t op_code, const char *name)
{
    if (g_sPacketInfo.find(op_code) == g_sPacketInfo.end())
        return nullptr;
    auto ret_pair = g_sPacketInfo.equal_range(op_code);
    for (auto it  = ret_pair.first; it != ret_pair.second; ++it) {
        if (strcmp(it->second.m_name, name) == 0)
            return &it->second;
    }
    return nullptr;
}
"""
  )

# ---------------------------------------------------------------------------------------
if len(sys.argv) != 4:
  print(sys.argv[0] + " <Path to adreno register file> <Path to output file base-name>")
  sys.exit()

try:

  register_root = sys.argv[1]
  register_file = sys.argv[2]

  # create element tree object
  tree = ET.parse(sys.argv[1]+"/"+sys.argv[2])

  # get root element
  registers_et_root = tree.getroot()

  # import any additional xml files referenced in the base xml file via "import" tags
  sub_xml_file_set = set()
  for child in registers_et_root:
    if child.tag == '{http://nouveau.freedesktop.org/}import':
      if 'file' in child.attrib:
        sub_xml_file = sys.argv[1]+"/"+child.attrib['file']

        # Check if this file has already been parsed or not
        if sub_xml_file not in sub_xml_file_set:
          sub_xml_file_set.add(sub_xml_file)
          sub_tree = ET.parse(sys.argv[1]+"/"+child.attrib['file'])
          for sub_tree_child in sub_tree.getroot():
            registers_et_root.append(sub_tree_child)

  # Find child with enum = "vgt_event_type"

  elements = registers_et_root.findall('{http://nouveau.freedesktop.org/}enum')

  pm4_info_file_h = open(sys.argv[3] + ".h", "w")
  pm4_info_file_cpp = open(sys.argv[3] + ".cpp", "w")

  head, tail = os.path.split(sys.argv[3] + ".h")
  pm4_info_filename_h = tail


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
      # Only add ones that start with "CP_*"
      if pm4_name.startswith("CP_"):
        opcode_dict[pm4_value] = pm4_name


  #pm4_info_file_h.write("enum Type7Opcodes\n")
  #pm4_info_file_h.write("{\n")
  #for opcode in opcode_dict:
  #  pm4_info_file_h.write("    %s = %s,\n" % (opcode_dict[opcode], opcode))
  #pm4_info_file_h.write("};\n")

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
  print("formatting " + "clang-format-7 -i -style=file " + sys.argv[2] + ".h")
  os.system("clang-format-7 -i -style=file " + sys.argv[2] + ".h")
  print("formatting " + "clang-format-7 -i -style=file " + sys.argv[2] + ".cpp")
  os.system("clang-format-7 -i -style=file  " + sys.argv[2] + ".cpp")

except IOError as e:
    errno, strerror = e.args
    print("I/O error({0}): {1}".format(errno,strerror))
    # e can be printed directly without using .args:
    # print(e)
except:
  print("Unexpected error:", sys.exc_info()[0])
  raise

print("%s: %s file generated" % (sys.argv[0], sys.argv[2]))

