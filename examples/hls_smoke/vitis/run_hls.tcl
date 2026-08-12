# Parameterized Vitis HLS synthesis script, invoked by extra_hls_vitis.py.
#
# Args (via -tclargs): top_fname src_file include_dir part clock_period_ns
#
# Mirrors extra_hls.py's Bambu invocation shape -- see README.md's
# Cross-tool/cross-config validation section.

set top_fname   [lindex $argv 0]
set src_file    [lindex $argv 1]
set include_dir [lindex $argv 2]
set part        [lindex $argv 3]
set period      [lindex $argv 4]

open_project -reset proj
set_top $top_fname
add_files $src_file -cflags "-I$include_dir -std=c++17"
open_solution -reset "solution1" -flow_target vivado
set_part $part
create_clock -period $period -name default
csynth_design
exit
