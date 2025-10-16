import xml.etree.ElementTree as ET
import copy

# Common functions
import ctypes
import re
c_uint8 = ctypes.c_uint8

class BitfieldsBits(ctypes.LittleEndianStructure):
    _fields_ = [
            ("A2XX", c_uint8, 1),
            ("A3XX", c_uint8, 1),
            ("A4XX", c_uint8, 1),
            ("A5XX", c_uint8, 1),
            ("A6XX", c_uint8, 1),
            ("A7XX", c_uint8, 1),
            ("A8XX", c_uint8, 1),
        ]

class Bitfields(ctypes.Union):
    _fields_ = [("bits", BitfieldsBits),
                ("asbyte", c_uint8)]
    
# convert "variants" string to bitfield 
def GetGPUVariantsBitField(str):
    pattern = r"A\d{1}XX"
    gpu_list = re.findall(pattern, str)
    bitfields = Bitfields()
    bitfields.asbyte = 0
    a_count = len(gpu_list)
    if a_count == 0:
        return 0
    dash_count = len(re.findall(r"-", str))

    # if there is a "-""
    # 1. if there are 2 A?xx, we need to add the ones between the 2 
    # 2. if there is only 1 A?XX, we need to add all following ones
    if dash_count != 0:
        begin = end = gpu_list[0]
        skip = False
        if a_count == 2:
            end = gpu_list[1]
        else:
            end = bitfields._fields_[0][1]._fields_[-1][0]
            if end != begin:
                gpu_list.append(end)
            else:
                skip = True

        # iterate all elements in BitfieldsBits to add the missing ones into "gpu_list"
        # note that the begin and end have already added to "gpu_list"  
        if not skip:  
            need_add = False
            for e in bitfields._fields_[0][1]._fields_:
                if e[0] == end:
                    need_add = False
                    break
                
                if need_add:
                    gpu_list.append(e[0])

                if e[0] == begin:
                    need_add = True     

    for var in gpu_list:
        match var:
            case "A2XX": bitfields.bits.A2XX = 1
            case "A3XX": bitfields.bits.A3XX = 1
            case "A4XX": bitfields.bits.A4XX = 1
            case "A5XX": bitfields.bits.A5XX = 1
            case "A6XX": bitfields.bits.A6XX = 1
            case "A7XX": bitfields.bits.A7XX = 1
            case "A8XX": bitfields.bits.A8XX = 1
    return bitfields.asbyte

# ---------------------------------------------------------------------------------------
def isBuiltInType(type):
  builtin_types = [ None, "a3xx_regid", "boolean", "uint", "hex", "int", "fixed", "ufixed", "float", "address", "waddress" ]
  return type in builtin_types

# ---------------------------------------------------------------------------------------
def gatherAllEnums(registers_et_root):
  # Find all enums anywhere in the tree
  enums = registers_et_root.findall('.//{http://nouveau.freedesktop.org/}enum')
  return enums

# ---------------------------------------------------------------------------------------
def addMissingDomains(registers_et_root):
  # CP_INDIRECT_BUFFER_PFD
  new_domain = ET.SubElement(registers_et_root, '{http://nouveau.freedesktop.org/}domain', name='CP_INDIRECT_BUFFER_PFD')
  ET.SubElement(new_domain, '{http://nouveau.freedesktop.org/}reg64', dict(offset='0', name='IB_BASE', type='address'))
  new_element = ET.SubElement(new_domain, '{http://nouveau.freedesktop.org/}reg32', dict(offset='2', name='2'))
  ET.SubElement(new_element, '{http://nouveau.freedesktop.org/}bitfield', dict(name='IB_SIZE', low='0', high='19'))

  # CP_INDIRECT_BUFFER_PFE
  new_domain = ET.SubElement(registers_et_root, '{http://nouveau.freedesktop.org/}domain', name='CP_INDIRECT_BUFFER_PFE')
  ET.SubElement(new_domain, '{http://nouveau.freedesktop.org/}reg64', dict(offset='0', name='IB_BASE', type='address'))
  new_element = ET.SubElement(new_domain, '{http://nouveau.freedesktop.org/}reg32', dict(offset='2', name='2'))
  ET.SubElement(new_element, '{http://nouveau.freedesktop.org/}bitfield', dict(name='IB_SIZE', low='0', high='19'))

  # Grab and make a copy of CP_LOAD_STATE. Get rid of the enums.
  cp_load_state_domain = registers_et_root.find('./{http://nouveau.freedesktop.org/}domain[@name="CP_LOAD_STATE6"]')
  cp_load_state_geom = copy.deepcopy(cp_load_state_domain)
  for child in cp_load_state_geom.findall('{http://nouveau.freedesktop.org/}enum'):
    cp_load_state_geom.remove(child)

  # CP_LOAD_STATE6_GEOM, which is same structurally as CP_LOAD_STATE except for an extra DWORD
  cp_load_state_geom.attrib["name"] = "CP_LOAD_STATE6_GEOM"
  registers_et_root.append(cp_load_state_geom)

  # CP_LOAD_STATE6_FRAG, which is same structurally as CP_LOAD_STATE except for an extra DWORD
  cp_load_state_frag = copy.deepcopy(cp_load_state_geom)
  cp_load_state_frag.attrib["name"] = "CP_LOAD_STATE6_FRAG"
  registers_et_root.append(cp_load_state_frag)
