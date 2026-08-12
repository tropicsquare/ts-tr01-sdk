#!/usr/bin/env python3
# PYTHON_ARGCOMPLETE_OK
# -*- coding: utf-8 -*-

####################################################################################################
# Patches HEX file with 32 bits to 39 bits HEX file with ECC.
#
# The script uses CPUSS specific ECC code (ECC encode/decode blocks from Open Titan).
# The script shall be used for following purposes:
#   - Patch 32-bit ROM HEX file (For manufacturing HEX generation or for simulation preload)
#   - Patch 32-bit IRAM/DRAM HEX file (for simulation preload). When writing data to IRAM/DRAM via
#     TPDI/LSU, CPUSS recomputes the ECC on the-fly.
#
# The script shall NOT be used to e.g. patch TROPIC01 Flash Memory HEX file with ECC.
# Tropic01 Flash Memory uses a different ECC encoding (despite using the same widths: 39/32)
# than CPUSS!
#
# ECC implementation in CPUSS on RAMs treats bit 39 as "valid" bit. Valid bit is set upon 32-bit
# access, and it is used to indicate that a memory word contains valid ECC in bits 38:32.
# CPUSS WOM
#
# Usage:
#   ./patch_cpuss_hex_with_ecc.py [--set-valid-bit] --input-hex <hex_file> --output-hex <hex_file>
#
# Options:
#   --set-valid-bit     If set,the output hex file will contain highest bit (39) set to 1.
#   --input-hex         Input hex32 file
#   --output-hex        Output hex32 file extended with ECC bits
#
# TODO: License
####################################################################################################

__author__ = "Filip Kliment"
__copyright__ = "Tropic Square"
__license___ = "TODO"
__maintainer__ = "Filip Kliment"


import re
import argparse

def getParity(n):
    parity = 0
    while n:
        parity = ~parity
        n = n & (n - 1)
    return parity

def secded_39_32_enc(data, set_valid_bit):
    data = re.sub(" ","0x", data)
    data_o = int(data, 0)
    if (False != getParity(data_o & 0x002606BD25)):
        data_o += 1<<32
    if (False != getParity(data_o & 0x00DEBA8050)):
        data_o += 1<<33
    if (False != getParity(data_o & 0x00413D89AA)):
        data_o += 1<<34
    if (False != getParity(data_o & 0x0031234ED1)):
        data_o += 1<<35
    if (False != getParity(data_o & 0x00C2C1323B)):
        data_o += 1<<36
    if (False != getParity(data_o & 0x002DCC624C)):
        data_o += 1<<37
    if (False != getParity(data_o & 0x0098505586)):
        data_o += 1<<38
    data_o = data_o ^ 0x2A00000000
    if (set_valid_bit):
        data_o += 1<<39
    hexa = f" {data_o:010x}"

    return hexa

def get_arguments():
  parser = argparse.ArgumentParser(description="Script to extednd hex 32 file with SECDED ECC bits")
  parser.add_argument("--set-valid-bit",    action="store_true",    help="Set bit 39")
  parser.add_argument("--input-hex",        required=True,          help="Input hex32 file")
  parser.add_argument("--output-hex",       required=True,          help="Output hex32 file extended with ECC bits")
  return parser.parse_args()

if __name__ == "__main__":
  args = get_arguments()
  hex = open(args.input_hex, "r")
  ecc = open(args.output_hex, "w")

  correct_file = False
  for x in hex:
    data = re.findall(" [0-9a-fA-F]{8}",x)
    address = re.findall("@[0-9a-fA-F]{8}",x)
    if (len(address) == 1 and len(data) > 0):
      correct_file = True
      data_o = []
      for i in data:
          data_o.append(secded_39_32_enc(i, args.set_valid_bit))
      line = ''.join(data_o)
      line_addr = ''.join(address)
      line = line_addr + line
      ecc.write(line + "\n")
    if ((len(address) > 1) or (len(address) == 1 and len(data) == 0) or (len(address) == 0 and len(data) > 0)):
      raise TypeError("File: " + args.input_hex + " is not in hex32 format")

  if (not correct_file):
    raise TypeError("File: " + args.input_hex + " is not in hex32 format")