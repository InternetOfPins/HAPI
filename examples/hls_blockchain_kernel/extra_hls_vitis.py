"""
PlatformIO custom targets for HLS synthesis via AMD/Xilinx Vitis HLS.

Independent, second HLS backend cross-check -- same two sources/top
functions, same Xilinx part as the Bambu runs, but a different vendor's
own tool instead of Bambu. See README.md's Cross-tool validation section.

    pio run -e hls-vitis -t synthesize-hash-murmur-vitis
    pio run -e hls-vitis -t synthesize-hash-xorfold-vitis

Requires the VITIS_HLS environment variable pointing at the `vitis-run`
binary from an installed Vitis toolchain (2026.1's HLS entry point,
replacing standalone vitis_hls). Not bundled/auto-installed here -- see
hls_fir/README.md for the Xilinx account + Vitis Unified Installer steps,
identical prerequisite.
"""
import os
Import("env")

VITIS_HLS = os.environ.get("VITIS_HLS")
DEVICE = "xc7a100tcsg324-1"  # same Artix-7 part as the Bambu runs -- directly comparable
CLOCK_PERIOD = "10"

HERE = env.subst("$PROJECT_DIR")
HAPI_INC = os.path.join(HERE, "..", "..", "include")


def _vitis_cmd(top_fname, src_file, outdir):
    if not VITIS_HLS:
        return (
            'echo "VITIS_HLS is not set -- point it at the `vitis-run` '
            'binary from an installed Vitis toolchain (see hls_fir/'
            'README.md for the Xilinx account + Vitis Unified Installer '
            'steps). Not auto-installing anything." && exit 1'
        )
    os.makedirs(outdir, exist_ok=True)
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
    name="synthesize-hash-murmur-vitis",
    dependencies=None,
    actions=[_vitis_cmd(
        "hashMurmurTop",
        os.path.join(HERE, "hls", "hash_murmur_top.cpp"),
        os.path.join(HERE, ".hls_out_hash_murmur_vitis"),
    )],
    title="HLS (Vitis): synthesize MurmurHash-style mixing",
    description="Same design as synthesize-hash-murmur, through Vitis "
                 "HLS instead of Bambu. See README.md.",
    always_build=True,
)

env.AddCustomTarget(
    name="synthesize-hash-xorfold-vitis",
    dependencies=None,
    actions=[_vitis_cmd(
        "hashXorFoldTop",
        os.path.join(HERE, "hls", "hash_xorfold_top.cpp"),
        os.path.join(HERE, ".hls_out_hash_xorfold_vitis"),
    )],
    title="HLS (Vitis): synthesize shift/xor-fold mixing",
    description="Same design as synthesize-hash-xorfold, through Vitis "
                 "HLS instead of Bambu. See README.md.",
    always_build=True,
)
