from __future__ import print_function
import sys
import os
import re
import xml.etree.ElementTree as ET


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
    "// This code has been generated automatically by generatePm4Packets_adreno.py. Do not hand-modify this code.\n"
    "//\n"
    "///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////\n"
  )

# ---------------------------------------------------------------------------------------
def addMissingDomains(registers_et_root):
  # CP_INDIRECT_BUFFER
  new_domain = ET.SubElement(registers_et_root, '{http://nouveau.freedesktop.org/}domain', name='CP_INDIRECT_BUFFER')
  ET.SubElement(new_domain, '{http://nouveau.freedesktop.org/}reg32', dict(offset='0', name='ADDR_LO'))
  ET.SubElement(new_domain, '{http://nouveau.freedesktop.org/}reg32', dict(offset='1', name='ADDR_HI'))
  ET.SubElement(new_domain, '{http://nouveau.freedesktop.org/}reg32', dict(offset='2', name='SIZE'))

  # CP_INDIRECT_BUFFER_PFD
  new_domain = ET.SubElement(registers_et_root, '{http://nouveau.freedesktop.org/}domain', name='CP_INDIRECT_BUFFER_PFD')
  ET.SubElement(new_domain, '{http://nouveau.freedesktop.org/}reg32', dict(offset='0', name='ADDR_LO'))
  ET.SubElement(new_domain, '{http://nouveau.freedesktop.org/}reg32', dict(offset='1', name='ADDR_HI'))
  ET.SubElement(new_domain, '{http://nouveau.freedesktop.org/}reg32', dict(offset='2', name='SIZE'))

  # CP_INDIRECT_BUFFER_PFE
  new_domain = ET.SubElement(registers_et_root, '{http://nouveau.freedesktop.org/}domain', name='CP_INDIRECT_BUFFER_PFE')
  ET.SubElement(new_domain, '{http://nouveau.freedesktop.org/}reg32', dict(offset='0', name='ADDR_LO'))
  ET.SubElement(new_domain, '{http://nouveau.freedesktop.org/}reg32', dict(offset='1', name='ADDR_HI'))
  ET.SubElement(new_domain, '{http://nouveau.freedesktop.org/}reg32', dict(offset='2', name='SIZE'))

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

    pm4_packet_file_h.write("enum %s\n" % enum_name)
    pm4_packet_file_h.write("{\n")
    for field in fields:
      enum_value_name = field.attrib['name']

      # Fields such as INDEX_SIZE_INVALID do not have a value and should be ignored here
      if 'value' in field.attrib:
        enum_value = int(field.attrib['value'], 0)
        pm4_packet_file_h.write("\t%s = 0x%x,\n" % (enum_value_name, enum_value))
    pm4_packet_file_h.write("};\n\n")

# ---------------------------------------------------------------------------------------
def outputPacketRegs(pm4_info_file, reg_list):
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

    bitfields = element.findall('{http://nouveau.freedesktop.org/}bitfield')

    # No bitfields, so use the register specification directly
    if len(bitfields) == 0:
      field_name = element.attrib['name']
      if field_name[0].isdigit():
        field_name = "_" + field_name

      if is_reg_32:
        print_type = False
        if 'type' in element.attrib:
          type = element.attrib['type']
          if type != "boolean" and type != "uint" and type != "int" and type != "address" and type != "hex":
            print_type = True

        if print_type:
          pm4_packet_file_h.write("\tuint32_t %s; // %s\n" % (field_name, element.attrib['type']))
        else:
          pm4_packet_file_h.write("\tuint32_t %s;\n" % field_name)
      elif is_reg_64:
        pm4_packet_file_h.write("\tuint64_t %s;\n" % field_name)
    else:
      pm4_packet_file_h.write("\tunion\n")
      pm4_packet_file_h.write("\t{\n")
      pm4_packet_file_h.write("\t\tstruct\n")
      pm4_packet_file_h.write("\t\t{\n")

    if is_reg_64 and len(bitfields) > 0:
      raise Exception("Found a reg64 with bitfields: " + field_name)

    cur_offset = 0
    for bitfield in bitfields:
      field_name = bitfield.attrib['name']
      if field_name[0].isdigit():
        field_name = "_" + field_name

      # Convert to mask & shift
      bitfield_start = 0
      bitfield_width = 1
      if 'low' in bitfield.attrib and 'high' in bitfield.attrib:
        low = int(bitfield.attrib['low'])
        high = int(bitfield.attrib['high'])
        bitfield_start = low
        bitfield_width = high - low + 1
        mask = (0xffffffff >> (31 - (high - low))) << low
      elif 'pos' in bitfield.attrib:
        bitfield_start = int(bitfield.attrib['pos'])
      else:
        raise Exception("Encountered a bitfield with no pos/low/high!")

      # Handle the case of overlapping bitfields. Only happens once, for CP_SET_MARKER
      # Just end the bitfield early in that case
      if cur_offset > bitfield_start:
        break;

      if cur_offset != bitfield_start:
        pm4_packet_file_h.write("\t\t\tuint32_t : %d;\n" % (bitfield_start - cur_offset))

      print_type = False
      if 'type' in bitfield.attrib:
        type = bitfield.attrib['type']
        if type != "boolean" and type != "uint" and type != "int" and type != "address" and type != "hex":
          print_type = True

      if print_type:
        pm4_packet_file_h.write("\t\t\tuint32_t %s : %d; // %s\n" % (field_name, bitfield_width, bitfield.attrib['type']))
      else:
        pm4_packet_file_h.write("\t\t\tuint32_t %s : %d;\n" % (field_name, bitfield_width))

      cur_offset = bitfield_start + bitfield_width

    # End of struct and union
    if len(bitfields) != 0:
      pm4_packet_file_h.write("\t\t} bitfields%d;\n" % index)
      pm4_packet_file_h.write("\t\tuint32_t ordinal%d;\n" % index)
      pm4_packet_file_h.write("\t};\n")

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
    # Ignore the array and make pm4_packet point to the contents of the array
    array = domain.find('./{http://nouveau.freedesktop.org/}array')
    pm4_packet = domain
    if array:
      pm4_packet = array

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

      # Print out any domain-specific enums
      # Only print out the enums for the 1st variant to avoid duplicates
      if index == 0:
        enums = domain.findall('./{http://nouveau.freedesktop.org/}enum')
        outputEnums(pm4_packet_file_h, enums)

      pm4_packet_file_h.write("struct %s\n" % packet_name)
      pm4_packet_file_h.write("{\n")
      pm4_packet_file_h.write("\tPm4Type7Header HEADER;\n")
      outputPacketRegs(pm4_packet_file_h, reg_list)
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

  pm4_packet_file_h.write("//------------------------------------------------\n")
  enums = registers_et_root.findall('./{http://nouveau.freedesktop.org/}enum')
  outputEnums(pm4_packet_file_h, enums)

  outputPackets(pm4_packet_file_h, registers_et_root, domains)

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
