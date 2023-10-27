from __future__ import print_function
import sys
import os
import re
import xml.etree.ElementTree as ET

from common import isBuiltInType
from common import gatherAllEnums

# ---------------------------------------------------------------------------------------
def outputHeader(pm4_info_file):
  pm4_info_file.writelines(
    " /*\n"
    " Copyright 2023 Google LLC\n"
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
    "// This code has been generated automatically by generateAdrenoHeader.py. Do not hand-modify this code.\n"
    "//\n"
    "///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////\n"
  )

# ---------------------------------------------------------------------------------------
def addMissingDomains(registers_et_root):
  # CP_INDIRECT_BUFFER
  new_domain = ET.SubElement(registers_et_root, '{http://nouveau.freedesktop.org/}domain', name='CP_INDIRECT_BUFFER')
  ET.SubElement(new_domain, '{http://nouveau.freedesktop.org/}reg32', dict(offset='0', name='ADDR_LO', type='hex'))
  ET.SubElement(new_domain, '{http://nouveau.freedesktop.org/}reg32', dict(offset='1', name='ADDR_HI', type='hex'))
  ET.SubElement(new_domain, '{http://nouveau.freedesktop.org/}reg32', dict(offset='2', name='SIZE', type='uint'))

  # CP_INDIRECT_BUFFER_PFD
  new_domain = ET.SubElement(registers_et_root, '{http://nouveau.freedesktop.org/}domain', name='CP_INDIRECT_BUFFER_PFD')
  ET.SubElement(new_domain, '{http://nouveau.freedesktop.org/}reg32', dict(offset='0', name='ADDR_LO', type='hex'))
  ET.SubElement(new_domain, '{http://nouveau.freedesktop.org/}reg32', dict(offset='1', name='ADDR_HI', type='hex'))
  ET.SubElement(new_domain, '{http://nouveau.freedesktop.org/}reg32', dict(offset='2', name='SIZE', type='uint'))

  # CP_INDIRECT_BUFFER_PFE
  new_domain = ET.SubElement(registers_et_root, '{http://nouveau.freedesktop.org/}domain', name='CP_INDIRECT_BUFFER_PFE')
  ET.SubElement(new_domain, '{http://nouveau.freedesktop.org/}reg32', dict(offset='0', name='ADDR_LO', type='hex'))
  ET.SubElement(new_domain, '{http://nouveau.freedesktop.org/}reg32', dict(offset='1', name='ADDR_HI', type='hex'))
  ET.SubElement(new_domain, '{http://nouveau.freedesktop.org/}reg32', dict(offset='2', name='SIZE', type='uint'))

# ---------------------------------------------------------------------------------------
def outputPrefix(pm4_info_file):
  outputHeader(pm4_info_file)
  pm4_info_file.writelines("""
#pragma once
#include <vector>
#include <stdint.h>
#include <string.h>

union Pm4Type4Header
{
    struct
    {
        uint32_t count          : 7;
        uint32_t count_parity   : 1;
        uint32_t offset         : 19;
        uint32_t offset_parity  : 1;
        uint32_t type           : 4;
    };
    uint32_t u32All;
};

union Pm4Type7Header
{
    struct
    {
        uint32_t count          : 15;
        uint32_t count_parity   : 1;
        uint32_t opcode         : 7;
        uint32_t opcode_parity  : 1;
        uint32_t zeroes         : 4;
        uint32_t type           : 4;
    };
    uint32_t u32All;
};

""")

# ---------------------------------------------------------------------------------------
def outputEnums(pm4_packet_file_h, enums):
  if enums is None:
    return

  for enum in enums:
    fields = enum.findall('{http://nouveau.freedesktop.org/}value')
    enum_name = enum.attrib['name']

    # Get rid of duplicates. Always prefer the ones that appear later, since the enums
    # are listed in ascending hardware revision order (i.e. A3xx followed by A4xx, etc)
    enum_dict = {}
    enum_variant_dict = {}
    for field in fields:
      enum_value_name = field.attrib['name']

      # Fields such as INDEX_SIZE_INVALID do not have a value and should be ignored here
      if 'value' in field.attrib:
        enum_value = int(field.attrib['value'], 0)

        add_replace_in_dict = True
        if 'variants' in field.attrib:
          # Check if this is a more recent variant
          if enum_value in enum_variant_dict:
            prev_variant = enum_variant_dict[enum_value]
            prev_ver = int(prev_variant[1],0)
            cur_variant = field.attrib['variants']
            cur_ver = int(cur_variant[1], 0)
            if prev_ver > cur_ver:
              add_replace_in_dict = False
            else:
              enum_variant_dict[enum_value] = field.attrib['variants']
          else:
            enum_variant_dict[enum_value] = field.attrib['variants']
        if add_replace_in_dict:
          enum_dict[enum_value] = enum_value_name

    pm4_packet_file_h.write("enum %s: uint32_t\n" % enum_name)
    pm4_packet_file_h.write("{\n")
    for enum_value in enum_dict:
      pm4_packet_file_h.write("\t%s = 0x%x,\n" % (enum_dict[enum_value], enum_value))
    pm4_packet_file_h.write("};\n\n")

# ---------------------------------------------------------------------------------------
def outputPacketRegs(pm4_info_file, reg_list, prefix, domain):
  dword_count = 0
  for index, element in enumerate(reg_list):
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

    field_name = element.attrib['name']
    if field_name[0].isdigit():
      field_name = "_" + field_name

    type = None
    if 'type' in element.attrib:
      type = element.attrib['type']

    use_buildin_type = False
    if is_reg_32:
      if type and isBuiltInType(type):
          pm4_packet_file_h.write("\tuint32_t %s; // %s\n" % (field_name, type))
          use_buildin_type = True
    elif is_reg_64:
      pm4_packet_file_h.write("\tuint64_t %s;\n" % field_name)
      use_buildin_type = True

    if not use_buildin_type:
      outputRegUnions(pm4_packet_file_h, domain, field_name, element, True, str(index)) 

    bitfields = element.findall('{http://nouveau.freedesktop.org/}bitfield')
    bitset = None
    if type:
      bitset = domain.find('./{http://nouveau.freedesktop.org/}bitset[@name="'+type+'"]')
    use_bitfield = len(bitfields) > 0 or bitset

    if is_reg_64 and use_bitfield:
      raise Exception("Found a reg64 with bitfields: " + field_name)


# ---------------------------------------------------------------------------------------
def outputBitfields(pm4_info_file, bitfields, extra_front_tab_str, cur_offset):
  for bitfield in bitfields:
    if bitfield.tag == '{http://nouveau.freedesktop.org/}doc':
      continue
    bitfield_name = bitfield.attrib['name']
    if bitfield_name[0].isdigit():
        bitfield_name = "_" + bitfield_name
    bitfield_type = "uint32_t"
    if 'type' in bitfield.attrib:
      bitfield_type = bitfield.attrib['type']    
      if isBuiltInType(bitfield_type):
        bitfield_type = "uint32_t"
    bitfield_start = 0
    bitfield_width = 1
    if 'low' in bitfield.attrib and 'high' in bitfield.attrib:
      low = int(bitfield.attrib['low'])
      high = int(bitfield.attrib['high'])
      bitfield_start = low
      bitfield_width = high - low + 1
    elif 'pos' in bitfield.attrib:
      bitfield_start = int(bitfield.attrib['pos'])
      bitfield_width = 1
    else:
      raise Exception("Encountered a bitfield with no pos/low/high!") 
    
    # Handle the case of overlapping bitfields. Only happens once, for CP_SET_MARKER
    # Just end the bitfield early in that case
    if cur_offset > bitfield_start:
      break;

    # Handle the case where some bits are skipped like CP_EVENT_WRITE7
    if cur_offset != bitfield_start:
      pm4_packet_file_h.write(extra_front_tab_str + "\t\tuint32_t : %d;\n" % (bitfield_start - cur_offset))

    pm4_info_file.write(extra_front_tab_str + "\t\t%s %s : %d;\n" % (bitfield_type, bitfield_name, bitfield_width)) 

    cur_offset = bitfield_start + bitfield_width
  return cur_offset

# ---------------------------------------------------------------------------------------
def outputRegUnions(pm4_info_file, a6xx_domain, name, reg, for_pm4, postfix):
  extra_front_tab_str = "";
  union_name = name
  if for_pm4:
    extra_front_tab_str = "\t";
    union_name = ""

  pm4_info_file.write(extra_front_tab_str + "union %s \n" % (union_name) )
  pm4_info_file.write(extra_front_tab_str + "{\n")

  type = None
  use_bitset_as_type = False
  cur_bitfield_offset = 0
  if 'type' in reg.attrib:
    type = reg.attrib['type']
    if type:
      # if it isn't one of the standard types, then it must be a custom bitset or an enum
      if not isBuiltInType(type):
        enum = registers_et_root.find('./{http://nouveau.freedesktop.org/}enum[@name="'+type+'"]')
        
        if enum:
          enum_name = enum.attrib['name']
          pm4_info_file.write(extra_front_tab_str + "\t%s bitfields;\n" % (enum_name)) 
        else:
          use_bitset_as_type = True
          bitset = a6xx_domain.find('./{http://nouveau.freedesktop.org/}bitset[@name="'+type+'"]')
          # Not found in the A6XX domain. Some of the bitsets are defined in the root.
          if not bitset:
            bitset = registers_et_root.find('./{http://nouveau.freedesktop.org/}bitset[@name="'+type+'"]')
            if not bitset:
              raise Exception("Not able to find bitset/enum " + type + " for register " + name)
          
          pm4_info_file.write(extra_front_tab_str + "\tstruct\n") 
          pm4_info_file.write(extra_front_tab_str + "\t{\n") 
          cur_bitfield_offset = outputBitfields(pm4_info_file, bitset, extra_front_tab_str, cur_bitfield_offset)
                  
  bitfields = reg.findall('{http://nouveau.freedesktop.org/}bitfield')
  # This handles the case where bitfields is used to extend bitset type, see SP_VS_CTRL_REG0
  if bitfields:
    if not use_bitset_as_type:
      pm4_info_file.write(extra_front_tab_str + "\tstruct\n") 
      pm4_info_file.write(extra_front_tab_str + "\t{\n")
    cur_bitfield_offset = outputBitfields(pm4_info_file, bitfields, extra_front_tab_str, cur_bitfield_offset)

  if use_bitset_as_type or bitfields:
    pm4_info_file.write(extra_front_tab_str + "\t}bitfields" + postfix + ";\n\n")

  pm4_info_file.write(extra_front_tab_str + "\tuint32_t u32All" + postfix + ";\n")      
  pm4_info_file.write(extra_front_tab_str + "\tint i32All" + postfix + ";\n")  
  pm4_info_file.write(extra_front_tab_str + "\tfloat f32All" + postfix + ";\n")
  pm4_info_file.write(extra_front_tab_str + "};\n\n")

# ---------------------------------------------------------------------------------------
def addToRegHash(name, reg, regs):
  if name in regs:
    bitfield_count_new = 0
    bitfield_count_exist = 0;
    bitfields_new = reg.findall('{http://nouveau.freedesktop.org/}bitfield')
    if (bitfields_new):
      bitfield_count_new = len(bitfields_new)
    bitfields_exist = regs[name].findall('{http://nouveau.freedesktop.org/}bitfield')
    if (bitfields_exist):
      bitfield_count_exist = len(bitfields_exist)
    # Always keep the one with more bitfield defined
    # TODO(wangra): verify if it make sense for different variants
    if (bitfield_count_new > bitfield_count_exist):
      regs[name] = reg
  else:
    regs[name] = reg


# ---------------------------------------------------------------------------------------
def outputPackets(pm4_packet_file_h, registers_et_root, domains):

  # Find all CP packet types so we can find out which domains are relevant
  pm4_type_packets = registers_et_root.find('./{http://nouveau.freedesktop.org/}enum[@name="adreno_pm4_type3_packets"]')

  for domain in domains:
    domain_name = domain.attrib['name']

    # Check if it is a domain describing a PM4 packet
    pm4_type_packet = pm4_type_packets.find('./{http://nouveau.freedesktop.org/}value[@name="'+domain_name+'"]')
    if pm4_type_packet is None:
      continue

    # There are only 2 PM4 packets with array tags: CP_SET_DRAW_STATE and CP_SET_PSEUDO_REG
    array = domain.find('./{http://nouveau.freedesktop.org/}array')
    array_size = 1
    pm4_packet = domain
    if array:
      pm4_packet = array
      array_size = int(array.attrib['length'])

    # There are 2 PM4 packet types with stripe tags, see CP_DRAW_INDIRECT_MULTI and CP_DRAW_INDX_INDIRECT for example
    # For CP_DRAW_INDX_INDIRECT type, the stripe is based on HW version, so just use the latest stripe only
    # For CP_DRAW_INDIRECT_MULTI type, we will have to first determine what field determines the variant
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

    for index, variant in enumerate(variant_list):
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
      packet_name = "PM4_" + domain_name
      if stripe:
        for element in variant[1]:
          is_reg_32 = (element.tag == '{http://nouveau.freedesktop.org/}reg32')
          is_reg_64 = (element.tag == '{http://nouveau.freedesktop.org/}reg64')
          if is_reg_32 or is_reg_64:
            offset = int(element.attrib['offset'])
            if not (offset in reg_dict):
              reg_dict[offset] = element
        # Add a postfix to the packet_name if it is a non-chip stripe
        if 'varset' in stripe.attrib:
          varset = stripe.attrib['varset']
          if varset != "chip":
            if 'prefix' in stripe.attrib:
              prefix = stripe.attrib['prefix']
              packet_name = "PM4_" + domain_name + "_" + prefix
            elif 'variants' in stripe.attrib:
              variant_string = stripe.attrib['variants']
              packet_name = "PM4_" + domain_name + "_" + variant_string

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

      pm4_packet_file_h.write("//------------------------------------------------\n")

      pm4_packet_file_h.write("struct %s\n" % packet_name)
      pm4_packet_file_h.write("{\n")
      pm4_packet_file_h.write("\tPm4Type7Header HEADER;\n")

      # For arrays packets such as CP_SET_DRAW_STATE, make an actual fixed-sized array containing the registers
      prefix = ""
      if array_size > 1:
        pm4_packet_file_h.write("\tstruct ARRAY_ELEMENT {\n")
        prefix = "\t"
      outputPacketRegs(pm4_packet_file_h, reg_list, prefix, domain)
      if array_size > 1:
        pm4_packet_file_h.write("\t} ARRAY[%d];\n" % array_size)
      pm4_packet_file_h.write("};\n\n")

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

  pm4_packet_file_h = open(sys.argv[3], "w")

  head, tail = os.path.split(sys.argv[3])
  pm4_packet_filename_h = tail


  # Determine the "domains" (i.e. packets) to output
  addMissingDomains(registers_et_root)
  domains = registers_et_root.findall('{http://nouveau.freedesktop.org/}domain')


  outputPrefix(pm4_packet_file_h)

  enums  = gatherAllEnums(registers_et_root)
  outputEnums(pm4_packet_file_h, enums)

  outputPackets(pm4_packet_file_h, registers_et_root, domains)

  # Define all unions
  # Use hashmap to avoid duplications
  regs = dict()
  a6xx_domain = registers_et_root.find('./{http://nouveau.freedesktop.org/}domain[@name="A6XX"]')
  for element in a6xx_domain:
    is_reg_32 = (element.tag == '{http://nouveau.freedesktop.org/}reg32')
    if is_reg_32:
      name = element.attrib['name']
      addToRegHash(name, element, regs)

  arrays = a6xx_domain.findall('{http://nouveau.freedesktop.org/}array')
  for array in arrays:
    array_name = array.attrib['name']
    for element in array:
      is_reg_32 = (element.tag == '{http://nouveau.freedesktop.org/}reg32')
      if is_reg_32:
        name = element.attrib['name']
        addToRegHash(array_name + "_" + name, element, regs)

  for name, reg in regs.items():
    outputRegUnions(pm4_packet_file_h, a6xx_domain, name, reg, False, "")
  pm4_packet_file_h.write("\n\n")

  # close to flush
  pm4_packet_file_h.close();

  # lint
  print("formatting " + "clang-format-7 -i -style=file " + sys.argv[3])
  os.system("clang-format-7 -i -style=file " + sys.argv[3])

except IOError as e:
    errno, strerror = e.args
    print("I/O error({0}): {1}".format(errno,strerror))
    # e can be printed directly without using .args:
    # print(e)
except:
  print("Unexpected error:", sys.exc_info()[0])
  raise

print("%s: %s file generated" % (sys.argv[0], sys.argv[3]))
