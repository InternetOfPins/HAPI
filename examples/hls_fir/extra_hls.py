"""
PlatformIO custom targets for HLS synthesis via PandA-Bambu.

Bambu isn't a PlatformIO `platform` (it synthesizes RTL, not a linked
executable) -- this hooks it in as custom targets instead, so the two
tap counts are runnable as:

    pio run -e hls -t synthesize-fir4
    pio run -e hls -t synthesize-fir8
    pio run -e hls -t synthesize-fir4-rtcoeff
    pio run -e hls -t synthesize-fir-lpf4
    pio run -e hls -t synthesize-fir-lpf8
    pio run -e hls -t synthesize-fir-lpf-cascade2

Requires the BAMBU_APPIMAGE environment variable pointing at a bambu
AppImage (see README.md -- no bundled/auto-installed toolchain here,
flagged explicitly rather than silently downloading one).

All targets synthesize against an explicit device (--device-name) and
clock period (--clock-period) -- Bambu is target-aware, not
target-independent: functional-unit selection (e.g. whether a multiply
maps to a real DSP block or LUT fabric) is characterized against a
specific device technology library, so an unconfirmed/default device
produces numbers that aren't citable against any real, ownable board.
xc7a100t-1csg324-VVD is the Xilinx Artix-7 on the Digilent Arty A7/
Nexys A7 -- widely owned, not a special-order part. 10ns targets 100MHz,
matching the ballpark of OneParse's jsonCharTop/jsonBufTop results for
rough comparability. See README.md's Results sections for the full
device-confirmed numbers.
"""
import os
Import("env")

BAMBU = os.environ.get("BAMBU_APPIMAGE")
DEVICE = "xc7a100t-1csg324-VVD"
CLOCK_PERIOD = "10"

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
        f'--device-name={DEVICE} --clock-period={CLOCK_PERIOD} '
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

env.AddCustomTarget(
    name="synthesize-fir-lpf4",
    dependencies=None,
    actions=[_bambu_cmd(
        "firLpf4Top",
        os.path.join(HERE, "hls", "fir_lpf4_top.cpp"),
        os.path.join(HERE, ".hls_out_fir_lpf4"),
    )],
    title="HLS: synthesize 4-tap Hamming-LPF FIR",
    description="Chain<Tap<10>,Tap<118>,Tap<118>,Tap<10>> -- real "
                 "Hamming-windowed-sinc low-pass coefficients (fc=1000Hz/"
                 "fs=8000Hz, Q8) instead of the fir4/fir8 binomial "
                 "placeholders. See README.md.",
    always_build=True,
)

env.AddCustomTarget(
    name="synthesize-fir-lpf8",
    dependencies=None,
    actions=[_bambu_cmd(
        "firLpf8Top",
        os.path.join(HERE, "hls", "fir_lpf8_top.cpp"),
        os.path.join(HERE, ".hls_out_fir_lpf8"),
    )],
    title="HLS: synthesize 8-tap Hamming-LPF FIR",
    description="8-tap version of synthesize-fir-lpf4's same Hamming-LPF "
                 "filter design. See README.md.",
    always_build=True,
)

env.AddCustomTarget(
    name="synthesize-fir-lpf-cascade2",
    dependencies=None,
    actions=[_bambu_cmd(
        "firLpfCascade2Top",
        os.path.join(HERE, "hls", "fir_lpf_cascade2_top.cpp"),
        os.path.join(HERE, ".hls_out_fir_lpf_cascade2"),
    )],
    title="HLS: synthesize two cascaded 4-tap Hamming-LPF FIR stages",
    description="Two independent synthesize-fir-lpf4 stages in series, "
                 "stage 1's output feeding stage 2's input -- tests "
                 "whether Bambu shares/duplicates resources across "
                 "cascaded stages the way it does across taps within one "
                 "stage. See README.md.",
    always_build=True,
)
