"""
PlatformIO custom target for HLS synthesis via PandA-Bambu.

Bambu isn't a PlatformIO `platform` (it synthesizes RTL, not a linked
executable) -- this hooks it in as a custom target instead:

    pio run -e hls -t synthesize-can-disabler

Requires the BAMBU_APPIMAGE environment variable pointing at a bambu
AppImage (see README.md -- no bundled/auto-installed toolchain here,
flagged explicitly rather than silently downloading one).

Synthesizes against an explicit device (--device-name) and clock period
(--clock-period), same device/period as hls_fir and hls_smoke for direct
comparability -- Bambu is target-aware, not target-independent:
functional-unit selection is characterized against a specific device
technology library, so an unconfirmed/default device produces numbers
that aren't citable against any real, ownable board.
"""
import os
Import("env")

BAMBU = os.environ.get("BAMBU_APPIMAGE")
DEVICE = "xc7a100t-1csg324-VVD"
CLOCK_PERIOD = "10"

HERE = env.subst("$PROJECT_DIR")
HAPI_INC = os.path.join(HERE, "..", "..", "include")

# Points at its own isolated hls/can_disabler_top.cpp, NOT src/main.cpp --
# same isolation rationale as hls_fir/hls_smoke: one top-level global per
# translation unit keeps the synthesized footprint honest.


def _bambu_cmd(top_fname, src_file, outdir):
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
        f'--std=gnu++17 --compiler=I386_CLANG16 '
        f'--device-name={DEVICE} --clock-period={CLOCK_PERIOD} '
        f'--top-fname={top_fname} -v2 "{src_file}"'
    )


env.AddCustomTarget(
    name="synthesize-can-disabler",
    dependencies=None,
    actions=[_bambu_cmd(
        "canDisablerTop",
        os.path.join(HERE, "hls", "can_disabler_top.cpp"),
        os.path.join(HERE, ".hls_out_can_disabler"),
    )],
    title="HLS: synthesize CAN transmit-disabler gate",
    description="Chain<MinCycle<100>,Allow<0,0x100>,Allow<1,0x101>,"
                 "Allow<2,0x102>> -- whitelist + minimum-retransmit-"
                 "interval guard. See README.md.",
    always_build=True,
)
