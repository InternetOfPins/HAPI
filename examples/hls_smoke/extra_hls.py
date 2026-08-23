"""
PlatformIO custom target for HLS synthesis via PandA-Bambu.

Bambu isn't a PlatformIO `platform` (it synthesizes RTL, not a linked
executable) -- this hooks it in as a custom target instead:

    pio run -e hls -t synthesize-wrapsum
    pio run -e hls -t synthesize-wrapsum-gcc8
    pio run -e hls -t synthesize-wrapsum-altdevice

Requires the BAMBU_APPIMAGE environment variable pointing at a bambu
AppImage (see README.md -- no bundled/auto-installed toolchain here,
flagged explicitly rather than silently downloading one).

Synthesizes against an explicit device (--device-name) and clock period
(--clock-period), same device/period as hls_fir and hls_can_disabler for
direct comparability -- Bambu is target-aware, not target-independent:
functional-unit selection is characterized against a specific device
technology library, so an unconfirmed/default device produces numbers
that aren't citable against any real, ownable board.

The -gcc8/-altdevice targets are second, independent Bambu configs (see
README.md's Cross-tool/cross-config validation section) -- not a
different HLS tool, but a cross-check that the structural result (one
real ui_plus_expr_FU, exactly proportional to the computation) isn't an
artifact of one frontend/device pairing.
"""
import os
Import("env")

BAMBU = os.environ.get("BAMBU_APPIMAGE")
DEVICE = "xc7a100t-1csg324-VVD"
ALT_DEVICE = "LFE5U85F8BG756C"  # Lattice ECP5-85, different vendor/architecture
CLOCK_PERIOD = "10"

HERE = env.subst("$PROJECT_DIR")
HAPI_INC = os.path.join(HERE, "..", "..", "include")
SRC = os.path.join(HERE, "src", "main.cpp")

# Points at src/main.cpp directly, not an isolated hls/*.cpp -- unlike
# hls_fir/hls_can_disabler, this example's src/main.cpp already isolates
# the synthesis target correctly: wrapSum(int) never reaches main()'s
# iostream/host-side code, so Bambu's own call-graph-following via
# --top-fname already excludes it (confirmed by the existing 1-adder
# result in README.md, no dead-code carried into the RTL).


def _bambu_cmd(top_fname, src_file, outdir, frontend="I386_CLANG16",
                device=DEVICE):
    if not BAMBU:
        return (
            'echo "BAMBU_APPIMAGE is not set -- point it at a bambu AppImage '
            '(e.g. https://release.bambuhls.eu/bambu-2024.10.AppImage) and '
            're-run. Not auto-installing anything." && exit 1'
        )
    os.makedirs(outdir, exist_ok=True)
    return (
        f'cd "{outdir}" && "{BAMBU}" '
        f'-I"{HAPI_INC}" '
        f'--std=gnu++17 --compiler={frontend} '
        f'--device-name={device} --clock-period={CLOCK_PERIOD} '
        f'--top-fname={top_fname} -v2 "{src_file}"'
    )


env.AddCustomTarget(
    name="synthesize-wrapsum",
    dependencies=None,
    actions=[_bambu_cmd(
        "wrapSum",
        SRC,
        os.path.join(HERE, ".hls_out_wrapsum"),
    )],
    title="HLS: synthesize 5-layer Chain<> collapse (wrapSum)",
    description="Chain<Parens,SqBracks,Bracks,Bars,XTag> arithmetic "
                 "smoke test -- collapses to one real adder. See README.md.",
    always_build=True,
)

env.AddCustomTarget(
    name="synthesize-wrapsum-gcc8",
    dependencies=None,
    actions=[_bambu_cmd(
        "wrapSum",
        SRC,
        os.path.join(HERE, ".hls_out_wrapsum_gcc8"),
        frontend="I386_GCC8",
    )],
    title="HLS: synthesize 5-layer Chain<> collapse (GCC8 frontend)",
    description="Same design as synthesize-wrapsum, through Bambu's "
                 "I386_GCC8 frontend instead of I386_CLANG16 -- cross-check "
                 "for a frontend-specific artifact. See README.md.",
    always_build=True,
)

env.AddCustomTarget(
    name="synthesize-wrapsum-altdevice",
    dependencies=None,
    actions=[_bambu_cmd(
        "wrapSum",
        SRC,
        os.path.join(HERE, ".hls_out_wrapsum_altdevice"),
        device=ALT_DEVICE,
    )],
    title="HLS: synthesize 5-layer Chain<> collapse (Lattice ECP5)",
    description="Same design as synthesize-wrapsum, against a Lattice "
                 "ECP5-85 (LFE5U85F8BG756C) instead of the Xilinx "
                 "Artix-7 -- cross-check for a device-specific artifact. "
                 "See README.md.",
    always_build=True,
)
