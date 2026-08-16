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

...plus a -gcc8 variant of each of the six above, and a fir4-altdevice
variant (see README.md's Results table -- second, independent Bambu
configs, cross-checking that the structural counts aren't an artifact of
one frontend/device pairing).

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
AC_TYPES_INC = os.environ.get("AC_TYPES_INCLUDE")
DEVICE = "xc7a100t-1csg324-VVD"
ALT_DEVICE = "LFE5U85F8BG756C"  # Lattice ECP5-85, different vendor/architecture
CLOCK_PERIOD = "10"

HERE = env.subst("$PROJECT_DIR")
HAPI_INC = os.path.join(HERE, "..", "..", "include")

# Each target points at its own isolated hls/*.cpp, NOT src/main.cpp --
# same isolation rationale as OneParse's examples/hls_smoke: one top-level
# global per translation unit keeps the synthesized footprint honest.


def _bambu_cmd(top_fname, src_file, outdir, frontend="I386_CLANG16",
                device=DEVICE, extra_include=None):
    if not BAMBU:
        return (
            'echo "BAMBU_APPIMAGE is not set -- point it at a bambu AppImage '
            '(e.g. https://release.bambuhls.eu/bambu-2024.10.AppImage) and '
            're-run. Not auto-installing anything." && exit 1'
        )
    if extra_include == "AC_TYPES" and not AC_TYPES_INC:
        # Bambu bundles its own (Mentor Graphics 3.7.2, 2017, PandA-patched,
        # AC_VERSION 3) ac_types fork on its default include path, so
        # omitting -I here would silently synthesize against that fork
        # instead of the real upstream (AC_VERSION 4) headers this target
        # exists to check -- see HAPI/.RnD/acTypesHLS/HANDOFF.md. Ordering
        # of this flag relative to HAPI_INC does NOT matter (empirically
        # confirmed both ways) -- the only real failure mode is omitting
        # it entirely, which the target source's own AC_VERSION guard
        # catches with a hard build error regardless of this script.
        return (
            'echo "AC_TYPES_INCLUDE is not set -- point it at a clone of '
            'https://github.com/hlslibs/ac_types (its include/ dir) and '
            're-run. Not auto-cloning anything, and not silently falling '
            'back to bambu\'s bundled ac_types fork." && exit 1'
        )
    os.makedirs(outdir, exist_ok=True)
    include_flags = f'-I"{HAPI_INC}"'
    if extra_include == "AC_TYPES":
        include_flags += f' -I"{AC_TYPES_INC}"'
    return (
        f'cd "{outdir}" && "{BAMBU}" '
        f'{include_flags} '
        f'--std=gnu++17 --compiler={frontend} '
        f'--device-name={device} --clock-period={CLOCK_PERIOD} '
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
    name="synthesize-fir-lpf4-actypes",
    dependencies=None,
    actions=[_bambu_cmd(
        "firLpf4ActypesTop",
        os.path.join(HERE, "hls", "fir_lpf4_actypes_top.cpp"),
        os.path.join(HERE, ".hls_out_fir_lpf4_actypes"),
        extra_include="AC_TYPES",
    )],
    title="HLS: synthesize 4-tap Hamming-LPF FIR (HLSLibs ac_fixed)",
    description="Same design as synthesize-fir-lpf4, but the delay "
                 "register/accumulator type is HLSLibs AC Datatypes' "
                 "ac_fixed<W,W,true> instead of hand-rolled int16_t/"
                 "int32_t -- third-party bit-accurate type compatibility "
                 "check. Requires AC_TYPES_INCLUDE (a clone of "
                 "github.com/hlslibs/ac_types), NOT bambu's own older "
                 "bundled ac_types fork. See README.md.",
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

# -gcc8 variants: same six designs, through Bambu's I386_GCC8 frontend
# instead of I386_CLANG16 -- cross-check for a frontend-specific artifact.

env.AddCustomTarget(
    name="synthesize-fir4-gcc8",
    dependencies=None,
    actions=[_bambu_cmd(
        "firTop",
        os.path.join(HERE, "hls", "fir4_top.cpp"),
        os.path.join(HERE, ".hls_out_fir4_gcc8"),
        frontend="I386_GCC8",
    )],
    title="HLS: synthesize 4-tap FIR (GCC8 frontend)",
    description="Same design as synthesize-fir4, through Bambu's I386_GCC8 "
                 "frontend. See README.md.",
    always_build=True,
)

env.AddCustomTarget(
    name="synthesize-fir8-gcc8",
    dependencies=None,
    actions=[_bambu_cmd(
        "firTop",
        os.path.join(HERE, "hls", "fir8_top.cpp"),
        os.path.join(HERE, ".hls_out_fir8_gcc8"),
        frontend="I386_GCC8",
    )],
    title="HLS: synthesize 8-tap FIR (GCC8 frontend)",
    description="Same design as synthesize-fir8, through Bambu's I386_GCC8 "
                 "frontend. See README.md.",
    always_build=True,
)

env.AddCustomTarget(
    name="synthesize-fir4-rtcoeff-gcc8",
    dependencies=None,
    actions=[_bambu_cmd(
        "firRtTop",
        os.path.join(HERE, "hls", "fir4_rtcoeff_top.cpp"),
        os.path.join(HERE, ".hls_out_fir4_rtcoeff_gcc8"),
        frontend="I386_GCC8",
    )],
    title="HLS: synthesize 4-tap FIR, runtime coefficients (GCC8 frontend)",
    description="Same design as synthesize-fir4-rtcoeff, through Bambu's "
                 "I386_GCC8 frontend. See README.md.",
    always_build=True,
)

env.AddCustomTarget(
    name="synthesize-fir-lpf4-gcc8",
    dependencies=None,
    actions=[_bambu_cmd(
        "firLpf4Top",
        os.path.join(HERE, "hls", "fir_lpf4_top.cpp"),
        os.path.join(HERE, ".hls_out_fir_lpf4_gcc8"),
        frontend="I386_GCC8",
    )],
    title="HLS: synthesize 4-tap Hamming-LPF FIR (GCC8 frontend)",
    description="Same design as synthesize-fir-lpf4, through Bambu's "
                 "I386_GCC8 frontend. See README.md.",
    always_build=True,
)

env.AddCustomTarget(
    name="synthesize-fir-lpf8-gcc8",
    dependencies=None,
    actions=[_bambu_cmd(
        "firLpf8Top",
        os.path.join(HERE, "hls", "fir_lpf8_top.cpp"),
        os.path.join(HERE, ".hls_out_fir_lpf8_gcc8"),
        frontend="I386_GCC8",
    )],
    title="HLS: synthesize 8-tap Hamming-LPF FIR (GCC8 frontend)",
    description="Same design as synthesize-fir-lpf8, through Bambu's "
                 "I386_GCC8 frontend. See README.md.",
    always_build=True,
)

env.AddCustomTarget(
    name="synthesize-fir-lpf-cascade2-gcc8",
    dependencies=None,
    actions=[_bambu_cmd(
        "firLpfCascade2Top",
        os.path.join(HERE, "hls", "fir_lpf_cascade2_top.cpp"),
        os.path.join(HERE, ".hls_out_fir_lpf_cascade2_gcc8"),
        frontend="I386_GCC8",
    )],
    title="HLS: synthesize two cascaded 4-tap Hamming-LPF FIR stages "
          "(GCC8 frontend)",
    description="Same design as synthesize-fir-lpf-cascade2, through "
                 "Bambu's I386_GCC8 frontend. See README.md.",
    always_build=True,
)

# -altdevice: fir4 only, against a Lattice ECP5 instead of the Xilinx
# Artix-7 -- cross-check for a device-specific artifact.

env.AddCustomTarget(
    name="synthesize-fir4-altdevice",
    dependencies=None,
    actions=[_bambu_cmd(
        "firTop",
        os.path.join(HERE, "hls", "fir4_top.cpp"),
        os.path.join(HERE, ".hls_out_fir4_altdevice"),
        device=ALT_DEVICE,
    )],
    title="HLS: synthesize 4-tap FIR (Lattice ECP5)",
    description="Same design as synthesize-fir4, against a Lattice ECP5-85 "
                 "(LFE5U85F8BG756C) instead of the Xilinx Artix-7. "
                 "See README.md.",
    always_build=True,
)
