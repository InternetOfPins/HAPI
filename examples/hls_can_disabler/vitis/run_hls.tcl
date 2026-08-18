# Reference template of the Tcl content extra_hls_vitis.py generates per
# target -- NOT invoked directly. vitis-run (2026.1's HLS entry point,
# replacing standalone vitis_hls) has no -tclargs/argv passthrough, so
# extra_hls_vitis.py writes this same sequence with values substituted
# inline instead of parameterizing this file via $argv. Kept here as a
# readable, manually-runnable copy -- fill in the placeholders below and
# run with `vitis-run --mode hls --tcl run_hls.tcl`.
#
# Mirrors extra_hls.py's Bambu invocation shape: same source file, same
# top-function name, same device/period where the device is Xilinx --
# see README.md's Cross-tool/cross-config validation section.

set top_fname   TOP_FUNCTION_NAME
set src_file    SOURCE_FILE_PATH
set include_dir INCLUDE_DIR_PATH
set part        xc7a100tcsg324-1
set period      10

open_project -reset proj
set_top $top_fname
add_files $src_file -cflags "-I$include_dir -std=c++17"
open_solution -reset "solution1"
set_part $part
create_clock -period $period -name default
csynth_design
exit
