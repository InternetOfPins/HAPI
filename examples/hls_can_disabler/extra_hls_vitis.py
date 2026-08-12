"""
PlatformIO custom target for HLS synthesis via AMD/Xilinx Vitis HLS.

Independent, second HLS backend cross-check for hls_can_disabler -- same
source, same top function, same Xilinx part as the Bambu runs, but a
different vendor's own tool instead of Bambu. See README.md's
Cross-tool/cross-config validation section.

    pio run -e hls-vitis -t synthesize-can-disabler-vitis

Requires the VITIS_HLS environment variable pointing at the `vitis_hls`
binary from an installed Vitis HLS toolchain (not bundled/auto-installed
here -- see README.md for the Xilinx account + Vitis Unified Installer
steps). NOT run as part of this pass: no Vitis HLS install was available,
so this is integration scaffolding only, pending that install.
"""
import os
Import("env")

VITIS_HLS = os.environ.get("VITIS_HLS")
DEVICE = "xc7a100t-1csg324-VVD"  # same Artix-7 part as the Bambu runs -- directly comparable
CLOCK_PERIOD = "10"

HERE = env.subst("$PROJECT_DIR")
HAPI_INC = os.path.join(HERE, "..", "..", "include")
TCL = os.path.join(HERE, "vitis", "run_hls.tcl")


def _vitis_cmd(top_fname, src_file, outdir):
    if not VITIS_HLS:
        return (
            'echo "VITIS_HLS is not set -- point it at an installed '
            'vitis_hls binary (see README.md for the Xilinx account + '
            'Vitis Unified Installer steps). Not auto-installing '
            'anything." && exit 1'
        )
    os.makedirs(outdir, exist_ok=True)
    return (
        f'cd "{outdir}" && "{VITIS_HLS}" -f "{TCL}" -tclargs '
        f'"{top_fname}" "{src_file}" "{HAPI_INC}" "{DEVICE}" "{CLOCK_PERIOD}"'
    )


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
