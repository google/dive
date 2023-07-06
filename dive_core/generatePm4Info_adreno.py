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
def readFileLines(gfx9_header_directory, header_file_name):
  file_path = gfx9_header_directory + header_file_name
  if not os.path.exists(file_path):
    print(file_path + " does not exist!")
    sys.exit()

  cur_file = open(file_path, "r")

  lines = cur_file.readlines()
  cur_file.close()
  return lines

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
    uint32_t m_dword;
    uint32_t m_enum_handle;
    uint32_t m_shift;
    uint32_t m_mask;
};

struct PacketInfo {
    const char* m_name;
    std::vector<PacketField> m_fields;
};

void Pm4InfoInit();
const char *GetOpCodeString(uint32_t op_code);
const RegInfo *GetRegInfo(uint32_t reg);
const RegInfo *GetRegByName(const char *);
const char *GetEnumString(uint32_t enum_handle, uint32_t val);
const PacketInfo *GetPacketInfo(uint32_t op_code);
const PacketInfo *GetPacketInfo(uint32_t op_code, const char *name);
"""
  )

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
def outputPm4InfoInitFunc(gfx9_header_directory, pm4_info_file, registers_et_root):
  pm4_info_file.writelines(
    "void Pm4InfoInit()\n"
    "{\n"
    "    assert(g_sOpCodeToString.empty());\n"
    "    assert(g_sRegInfo.empty());\n"
    "    assert(g_sEnumReflection.empty());\n"
    "    assert(g_sPacketInfo.empty());\n"
    "\n"
  )
  outputOpcodes(gfx9_header_directory, pm4_info_file, registers_et_root)
  pm4_info_file.write("\n")
  outputRegisterInfo(gfx9_header_directory, pm4_info_file, registers_et_root)
  pm4_info_file.write("\n")
  outputPacketInfo(gfx9_header_directory, pm4_info_file, registers_et_root)
  pm4_info_file.write("}\n")
  return

valid_opcodes = {}
# ---------------------------------------------------------------------------------------
def outputOpcodes(gfx9_header_directory, pm4_info_file, registers_et_root):

  # Find the "adreno_pm4_type3_packets" enum (defined in the andreno_pm4.xml file)
  pm4_type_packets = registers_et_root.find('./{http://nouveau.freedesktop.org/}enum[@name="adreno_pm4_type3_packets"]')

  # Iterate through each enum

  opcode_dict = {}

  # There can be multiple opcodes with the same value, to support different adreno versions
  # The opcodes are listed in hw revision order, with later entries of the same opcode used for later revisions
  # We only want the opcode names for the latest revision, so overwrite the dictionary if the same value encountered
  for pm4_type_packet in pm4_type_packets:
    if 'name' in pm4_type_packet.attrib and 'value' in pm4_type_packet.attrib:
      pm4_name = pm4_type_packet.attrib['name']
      pm4_value = pm4_type_packet.attrib['value']
      opcode_dict[pm4_value] = pm4_name
      valid_opcodes[pm4_value]=True

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
def outputRegisterInfo(gfx9_header_directory, pm4_info_file, registers_et_root):
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

def outputUnionLine(pm4_info_file, field_name, dword_count, enum_handle, shift, mask):
  pm4_info_file.write("{ \"%s\", %d, %s, %d, 0x%x }," %
                       (field_name, dword_count, enum_handle, shift, mask))

# ---------------------------------------------------------------------------------------
def outputStructInfo(lines, prefix, pm4_info_file, enum_index_dict, op_code_set):








  is_in_new_struct = False
  is_in_struct_union = False
  bits_so_far = 0
  op_code = ""
  field_name = ""
  dword_count = 0
  ds_stack = []
  for line in lines:
    if not ds_stack:
      # Match "typedef struct PM4_{prefix}_###__GFX09" (__GFX09 optional)
      match_obj = re.search(r'^typedef struct PM4_%s_(\w+?)(__GFX09)?$' % prefix.upper(), line)
      if match_obj:
        op_code = "IT_" + match_obj.group(1)

        # Ignore all packets that end in __GFX*
        match_obj = re.search(r'(\w+?)__GFX(\w+)$', op_code)
        if match_obj:
            continue

        # For some reason, some op-code names end suffixes
        if re.search(r'DISPATCH_DRAW', op_code):
          op_code += "__GFX101"
        if re.search(r'GET_LOD_STATS', op_code):
          op_code += "__GFX09"
        if re.search(r'DISPATCH_MESH_INDIRECT_MULTI', op_code):
          op_code += "__NV10"
        if re.search(r'DISPATCH_TASKMESH_GFX', op_code):
          op_code += "__NV10"
        if re.search(r'LOAD_UCONFIG_REG_INDEX', op_code):
          op_code += "__NV10"
        if re.search(r'DISPATCH_TASK_STATE_INIT', op_code):
          op_code += "__NV10"
        if re.search(r'DISPATCH_TASKMESH_DIRECT_ACE', op_code):
          op_code += "__NV10"
        if re.search(r'DISPATCH_TASKMESH_INDIRECT_MULTI_ACE', op_code):
          op_code += "__NV10"

        # Accounting for inconsistent spelling in files
        op_code = re.sub(r'_CONST$', '_CNST', op_code)

        if not op_code in valid_opcodes:
          continue

        if op_code not in op_code_set:
          is_in_new_struct = True
          pm4_info_file.write("    g_sPacketInfo.insert(std::pair<uint32_t, PacketInfo>(")
          pm4_info_file.write("%s, { \"%s\", {" % (op_code, op_code))
          op_code_set.add(op_code)
          dword_count = 0
          ds_stack = ["struct"]
      continue

    if "union" in line:
      ds_stack.append("union")
      bits_so_far = 0
      enum_handle = "UINT32_MAX"
      mask = int('0xffffffff', 16)
      shift = 0
      continue
    elif "struct" in line:
      ds_stack.append("struct")
      continue

    # parse dword_count
    match_obj = re.search(r'ordinal(\d+);', line)
    if match_obj:
      dword_count = int(match_obj.group(1))

    # Parse union
    if ds_stack and ds_stack[-1] == "union":
      if "}" in line:
        if field_name:    # If it is concatanating non-bitfields, then need to output at end
          outputUnionLine(pm4_info_file, field_name, dword_count - 1 , enum_handle, shift, mask)
          field_name = ""
        is_in_struct_union = False
        ds_stack.pop()
        shift = 0
        mask = int('0xffffffff', 16)
        continue

      # Match non-bitfields
      match_obj = re.search(r'^\s*(\w+)\s*(\w+);', line)
      if match_obj:
        type_string = match_obj.group(1)

        # Sometimes a union is used to alias different non-bitfield fields with different
        # names. In those cases, concatanate the field names together, and output the line
        # when the end curly bracket of the union is parsed
        # (eg: PM4_PFP_LOAD_SH_REG_INDEX's mem_addr_hi vs addr_offset)

        # Ignore ordinal & header & u32All fields
        cur_field_name = match_obj.group(2)
        if not re.search(r'ordinal\d+', cur_field_name) and (cur_field_name != "header") and (cur_field_name != "u32All") and not re.search(r'reserved\d+', cur_field_name):
          if not field_name:
            field_name = cur_field_name
          else:
            field_name += " | " + cur_field_name

    # Parse struct
    if ds_stack and ds_stack[-1] == "struct":
      if "}" in line:
        ds_stack.pop()
        shift = 0
        mask = int('0xffffffff', 16)
        if not ds_stack:
          pm4_info_file.write(" } }));\n")
          is_in_new_struct = False
        continue

      # Match a non-union struct field
      match_obj = re.search(r'uint32_t\s*(\w+);', line)
      if match_obj:
        if len(ds_stack) == 1:
          dword_count += 1
        # Ignore reserved fields
        if not re.search(r'reserved\d', match_obj.group(1)):
          pm4_info_file.write("{ \"%s\", %d, UINT32_MAX, 0, 0xFFFFFFFF }," %
                              (match_obj.group(1), dword_count))
          continue

      # Match non-bitfields
      match_obj = re.search(r'^\s*(\w+)\s*(\w+);', line)
      if match_obj:
        type_string = match_obj.group(1)

        # Ignore ordinal & header fields
        cur_field_name = match_obj.group(2)
        if not re.search(r'ordinal\d+', cur_field_name) and (cur_field_name != "header"):
          if not field_name:
            field_name = cur_field_name
          else:
            field_name += " | " + cur_field_name

      # Match bitfields
      match_obj = re.search(r'^\s*(\w+)\s+(\w+)\s*:\s*(\w+);', line)
      if match_obj:
        type_string = match_obj.group(1)
        field_name = match_obj.group(2)
        bits = int(match_obj.group(3))
        shift = bits_so_far
        mask = int('0xffffffff', 16)
        mask = mask >> (32 - bits)
        mask = mask << shift
        enum_handle = "UINT32_MAX"

        # If not uint32_t type, then must be a enum type. Look up index in enum dictionary.
        if type_string != "uint32_t":
          match_obj = re.search(r'%s_(\w+)' % prefix, type_string)  # Remove prefix
          type_string_minus_prefix = match_obj.group(1)
          if type_string_minus_prefix not in enum_index_dict:
            print("Error: Enumeration %s not found!" % type_string_minus_prefix)
            sys.exit()
          enum_handle = str(enum_index_dict[type_string_minus_prefix])

        # Ignore reserved# bit-fields
        if not re.search(r'reserved\d+', field_name):
          outputUnionLine(pm4_info_file, field_name, dword_count, enum_handle, shift, mask)
        field_name = ""

        bits_so_far += bits
        if bits_so_far == 32:
          bits_so_far = 0

# ---------------------------------------------------------------------------------------
def outputPacketInfo(gfx9_header_directory, pm4_info_file, registers_et_root):
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
  outputStructInfo(lines_pfp, "PFP", pm4_info_file, enum_index_dict, op_code_set)
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
  for element in elements: print(element.tag,element.attrib)

  for child in registers_et_root:
    print(child.tag, child.attrib)

  #ET.tostring(registers_et_root)

  pm4_info_file_h = open(sys.argv[3] + ".h", "w")
  pm4_info_file_cpp = open(sys.argv[3] + ".cpp", "w")

  head, tail = os.path.split(sys.argv[3] + ".h")
  pm4_info_filename_h = tail

  # .H file
  outputH(pm4_info_file_h)

  # .CPP file
  outputHeaderCpp(pm4_info_filename_h, pm4_info_file_cpp)
  outputPm4InfoInitFunc(sys.argv[1], pm4_info_file_cpp, registers_et_root)
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

