"""
PlatformIO custom target for HLS synthesis via AMD/Xilinx Vitis HLS.

Independent, second HLS backend cross-check for hls_can_disabler -- same
source, same top function, same Xilinx part as the Bambu runs, but a
different vendor's own tool instead of Bambu. See README.md's
Cross-tool/cross-config validation section.

    pio run -e hls-vitis -t synthesize-can-disabler-vitis

Requires the VITIS_HLS environment variable pointing at the `vitis-run`
binary from an installed Vitis toolchain (2026.1 replaced the old
standalone `vitis_hls` command with `vitis-run --mode hls`; not bundled/
auto-installed here -- see README.md for the Xilinx account + Vitis
Unified Installer steps). Verified working 2026-08-18 against a real
2026.1 install.
"""
import os
Import("env")

VITIS_HLS = os.environ.get("VITIS_HLS")
DEVICE = "xc7a100tcsg324-1"  # same Artix-7 part as the Bambu runs -- directly comparable.
# Real Vivado part-string format, NOT Bambu's own "-1csg324-VVD" --device-name
# surface syntax -- Bambu translates that internally before calling actual
# Vivado (see its own generated vivado.tcl: "-part xc7a100tcsg324-1").
CLOCK_PERIOD = "10"

HERE = env.subst("$PROJECT_DIR")
HAPI_INC = os.path.join(HERE, "..", "..", "include")


def _vitis_cmd(top_fname, src_file, outdir):
    if not VITIS_HLS:
        return (
            'echo "VITIS_HLS is not set -- point it at the `vitis-run` '
            'binary from an installed Vitis toolchain (see README.md for '
            'the Xilinx account + Vitis Unified Installer steps). Not '
            'auto-installing anything." && exit 1'
        )
    os.makedirs(outdir, exist_ok=True)
    # vitis-run (2026.1's HLS entry point) has no -tclargs/argv passthrough,
    # unlike the old standalone vitis_hls -- so each target's Tcl content is
    # generated here instead of parameterizing a shared script via $argv.
    # See vitis/run_hls.tcl for the same content as a readable template.
    tcl_path = os.path.join(outdir, "run_hls_generated.tcl")
    with open(tcl_path, "w") as f:
        f.write(
            f'open_project -reset proj\n'
            f'set_top {top_fname}\n'
            f'add_files {src_file} -cflags "-I{HAPI_INC} -std=c++17"\n'
            f'open_solution -reset "solution1"\n'
            f'set_part {DEVICE}\n'
            f'create_clock -period {CLOCK_PERIOD} -name default\n'
            f'csynth_design\n'
            f'exit\n'
        )
    return f'cd "{outdir}" && "{VITIS_HLS}" --mode hls --tcl "{tcl_path}"'


env.AddCustomTarget(
    name="synthesize-can-disabler-vitis",
    dependencies=None,
    actions=[_vitis_cmd(
        "canDisablerTop",
        os.path.join(HERE, "hls", "can_disabler_top.cpp"),
        os.path.join(HERE, ".hls_out_can_disabler_vitis"),
    )],
    title="HLS: synthesize CAN transmit-disabler gate (Vitis HLS)",
    description="Same design as synthesize-can-disabler, through AMD/"
                 "Xilinx's own Vitis HLS instead of Bambu -- independent "
                 "tool cross-check. See README.md.",
    always_build=True,
)
