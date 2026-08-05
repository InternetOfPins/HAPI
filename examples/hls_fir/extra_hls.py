"""
PlatformIO custom targets for HLS synthesis via PandA-Bambu.

Bambu isn't a PlatformIO `platform` (it synthesizes RTL, not a linked
executable) -- this hooks it in as custom targets instead, so the two
tap counts are runnable as:

    pio run -e hls -t synthesize-fir4
    pio run -e hls -t synthesize-fir8
    pio run -e hls -t synthesize-fir4-rtcoeff

Requires the BAMBU_APPIMAGE environment variable pointing at a bambu
AppImage (see README.md -- no bundled/auto-installed toolchain here,
flagged explicitly rather than silently downloading one).
"""
import os
Import("env")

BAMBU = os.environ.get("BAMBU_APPIMAGE")

HERE = env.subst("$PROJECT_DIR")
HAPI_INC = os.path.join(HERE, "..", "..", "include")

# Each target points at its own isolated hls/*.cpp, NOT src/main.cpp --
# same isolation rationale as OneParse's examples/hls_smoke: one top-level
# global per translation unit keeps the synthesized footprint honest.


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
        f'--top-fname={top_fname} -v2 "{src_file}"'
    )


env.AddCustomTarget(
    name="synthesize-fir4",
    dependencies=None,
    actions=[_bambu_cmd(
        "firTop",
        os.path.join(HERE, "hls", "fir4_top.cpp"),
        os.path.join(HERE, ".hls_out_fir4"),
    )],
    title="HLS: synthesize 4-tap FIR",
    description="Chain<Tap<1>,Tap<3>,Tap<3>,Tap<1>> -- the small end of "
                 "the resource-scaling comparison in README.md",
    always_build=True,
)

env.AddCustomTarget(
    name="synthesize-fir8",
    dependencies=None,
    actions=[_bambu_cmd(
        "firTop",
        os.path.join(HERE, "hls", "fir8_top.cpp"),
        os.path.join(HERE, ".hls_out_fir8"),
    )],
    title="HLS: synthesize 8-tap FIR",
    description="Chain<Tap<1>,Tap<7>,Tap<21>,Tap<35>,Tap<35>,Tap<21>,"
                 "Tap<7>,Tap<1>> -- the large end of the resource-scaling "
                 "comparison in README.md",
    always_build=True,
)

env.AddCustomTarget(
    name="synthesize-fir4-rtcoeff",
    dependencies=None,
    actions=[_bambu_cmd(
        "firRtTop",
        os.path.join(HERE, "hls", "fir4_rtcoeff_top.cpp"),
        os.path.join(HERE, ".hls_out_fir4_rtcoeff"),
    )],
    title="HLS: synthesize 4-tap FIR, runtime coefficients",
    description="Same 4-tap shape as synthesize-fir4, but coefficients "
                 "come from a volatile runtime table instead of a "
                 "template NTTP -- checks whether a real DSP-mappable "
                 "multiplier gets inferred when Bambu can't constant-fold "
                 "the coefficient. See README.md.",
    always_build=True,
)
