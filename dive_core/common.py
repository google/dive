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
    return bitfields.asbyte

# ---------------------------------------------------------------------------------------
def isBuiltInType(type):
  builtin_types = [ None, "a3xx_regid", "boolean", "uint", "hex", "int", "fixed", "ufixed", "float", "address", "waddress" ]
  return type in builtin_types

# ---------------------------------------------------------------------------------------
def gatherAllEnums(registers_et_root):
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
  return enums