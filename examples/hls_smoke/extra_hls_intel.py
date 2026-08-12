"""
PlatformIO custom target for HLS synthesis via Intel HLS Compiler (i++).

Independent, third HLS backend cross-check for hls_smoke -- see
README.md's Cross-tool/cross-config validation section.

    pio run -e hls-intel -t synthesize-wrapsum-intel

Requires `i++` on PATH (installed via Quartus Prime Lite + Intel HLS
Compiler -- not bundled/auto-installed here, see README.md for account/
install instructions). NOT run as part of this pass: no Intel HLS
Compiler install was available, so this is integration scaffolding only,
pending that install.

CROSS-VENDOR DEVICE CAVEAT: Intel HLS Compiler's free Lite edition is
gated to specific Altera/Intel device families (e.g. Cyclone V), NOT
Xilinx parts -- this cannot target the same xc7a100t-1csg324-VVD used by
the Bambu/Vitis runs. DEVICE below is a placeholder; confirm the actual
supported device list against the installed version before running, and
keep any resulting numbers labeled as a different device family, not
directly comparable to the Xilinx-targeted columns.

The `i++` flags below follow Intel's documented component-compiler
syntax but were NOT verified against a real install (none available in
this pass) -- re-check `i++ --help` / current Intel HLS Compiler docs
before the first real run.
"""
import os
import shutil
Import("env")

INTEL_HLS = shutil.which("i++")
DEVICE = "Cyclone10GX"  # PLACEHOLDER -- confirm against installed device list, see caveat above
CLOCK = "100MHz"

HERE = env.subst("$PROJECT_DIR")
HAPI_INC = os.path.join(HERE, "..", "..", "include")


def _intel_hls_cmd(top_fname, src_file, outdir):
    if not INTEL_HLS:
        return (
            'echo "i++ not found on PATH -- install Quartus Prime Lite + '
            'Intel HLS Compiler and ensure i++ is on PATH (see README.md '
            'for account/install instructions). Not auto-installing '
            'anything." && exit 1'
        )
    os.makedirs(outdir, exist_ok=True)
    return (
        f'cd "{outdir}" && i++ -march={DEVICE} --clock={CLOCK} '
        f'-I"{HAPI_INC}" --std=c++17 '
        f'--component={top_fname} -o "{top_fname}" "{src_file}"'
    )


env.AddCustomTarget(
    name="synthesize-wrapsum-intel",
    dependencies=None,
    actions=[_intel_hls_cmd(
        "wrapSum",
        os.path.join(HERE, "src", "main.cpp"),
        os.path.join(HERE, ".hls_out_wrapsum_intel"),
    )],
    title="HLS: synthesize 5-layer Chain<> collapse (Intel HLS Compiler)",
    description="Same design as synthesize-wrapsum, through Intel's own "
                 "HLS Compiler instead of Bambu -- independent tool "
                 "cross-check, DIFFERENT DEVICE FAMILY (Altera/Intel, not "
                 "Xilinx). See README.md.",
    always_build=True,
)
