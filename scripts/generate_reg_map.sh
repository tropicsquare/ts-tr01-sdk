#!/bin/bash

# ordt_run.sh is a wrapped script which runs ORDT generation tool in behind.
# This example does following:
#      1. Takes parameter input file "ordt_param_file".
#      2. Takes register map file "reg_map_example.rdl"
#      3. Generateds LaTex documentation, RTL design, UVM RAL and C header file.
ordt_run.sh -parms $TS_REPO_ROOT/scripts/ordt_param_file \
            -cheader $TS_REPO_ROOT/drv/ipc_regs.h \
            $TS_REPO_ROOT/ipc/ipc.rdl
