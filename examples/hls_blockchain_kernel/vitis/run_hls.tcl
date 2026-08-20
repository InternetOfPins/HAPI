# Reference template of the Tcl content extra_hls_vitis.py generates per
# target -- NOT invoked directly. See hls_fir/vitis/run_hls.tcl for the
# full rationale (vitis-run has no -tclargs/argv passthrough). Fill in the
# placeholders below and run with `vitis-run --mode hls --tcl run_hls.tcl`.

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
