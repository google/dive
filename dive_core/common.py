# Common functions

# ---------------------------------------------------------------------------------------
def isBuiltInType(type):
  builtin_types = [ None, "a3xx_regid", "boolean", "uint", "hex", "int", "fixed", "ufixed", "float", "address", "waddress" ]
  if not type in builtin_types:
    return False
  return True


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