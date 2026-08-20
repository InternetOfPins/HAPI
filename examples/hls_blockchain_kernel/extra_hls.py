"""
PlatformIO custom targets for HLS synthesis via PandA-Bambu.

    pio run -e hls -t synthesize-hash-murmur
    pio run -e hls -t synthesize-hash-xorfold
    pio run -e hls -t synthesize-hash-murmur-gcc8
    pio run -e hls -t synthesize-hash-xorfold-gcc8

Requires the BAMBU_APPIMAGE environment variable pointing at a bambu
AppImage (see README.md -- no bundled/auto-installed toolchain here,
flagged explicitly rather than silently downloading one).

Same device/clock-period pinning as hls_fir/hls_can_disabler/hls_smoke,
for direct comparability: xc7a100t-1csg324-VVD (Xilinx Artix-7, Digilent
Arty A7/Nexys A7), 10ns period (100MHz target). See those examples'
README.md for why an unconfirmed/default device isn't citable.

-gcc8 variants exist for the same reason hls_fir/hls_smoke/
hls_can_disabler carry one: Bambu's I386_GCC8 frontend has a known,
already-documented rejection of the AST node Chain<>'s recursive
composition produces (HANDOFF.md finding referenced in this example's own
README). Expected outcome here is the same rejection, not a pass -- run
it anyway; if it ever starts passing, that's the interesting result, not
the other way around.
"""
import os
Import("env")

BAMBU = os.environ.get("BAMBU_APPIMAGE")
DEVICE = "xc7a100t-1csg324-VVD"
CLOCK_PERIOD = "10"

HERE = env.subst("$PROJECT_DIR")
HAPI_INC = os.path.join(HERE, "..", "..", "include")


def _bambu_cmd(top_fname, src_file, outdir, frontend="I386_CLANG16"):
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
        f'--device-name={DEVICE} --clock-period={CLOCK_PERIOD} '
        f'--top-fname={top_fname} -v2 "{src_file}"'
    )


env.AddCustomTarget(
    name="synthesize-hash-murmur",
    dependencies=None,
    actions=[_bambu_cmd(
        "hashMurmurTop",
        os.path.join(HERE, "hls", "hash_murmur_top.cpp"),
        os.path.join(HERE, ".hls_out_hash_murmur"),
    )],
    title="HLS: synthesize MurmurHash-style mixing",
    description="Chain<MurmurHash,Transaction> -- has a real constant "
                 "multiply (h *= 0x5bd1e995u). Prediction to check: DSP "
                 "block or strength-reduced to shifts/adds? See README.md.",
    always_build=True,
)

env.AddCustomTarget(
    name="synthesize-hash-xorfold",
    dependencies=None,
    actions=[_bambu_cmd(
        "hashXorFoldTop",
        os.path.join(HERE, "hls", "hash_xorfold_top.cpp"),
        os.path.join(HERE, ".hls_out_hash_xorfold"),
    )],
    title="HLS: synthesize shift/xor-fold mixing",
    description="Chain<XorFoldHash,Transaction> -- same composition shape "
                 "as synthesize-hash-murmur, no multiply at all (shifts + "
                 "xors only). Prediction: zero DSPs, pure logic fabric. "
                 "See README.md.",
    always_build=True,
)

env.AddCustomTarget(
    name="synthesize-hash-murmur-gcc8",
    dependencies=None,
    actions=[_bambu_cmd(
        "hashMurmurTop",
        os.path.join(HERE, "hls", "hash_murmur_top.cpp"),
        os.path.join(HERE, ".hls_out_hash_murmur_gcc8"),
        frontend="I386_GCC8",
    )],
    title="HLS: synthesize MurmurHash-style mixing (GCC8 frontend)",
    description="Same design as synthesize-hash-murmur, through Bambu's "
                 "I386_GCC8 frontend. Expected to fail the same way it "
                 "does on hls_fir/hls_smoke/hls_can_disabler -- see "
                 "README.md.",
    always_build=True,
)

env.AddCustomTarget(
    name="synthesize-hash-xorfold-gcc8",
    dependencies=None,
    actions=[_bambu_cmd(
        "hashXorFoldTop",
        os.path.join(HERE, "hls", "hash_xorfold_top.cpp"),
        os.path.join(HERE, ".hls_out_hash_xorfold_gcc8"),
        frontend="I386_GCC8",
    )],
    title="HLS: synthesize shift/xor-fold mixing (GCC8 frontend)",
    description="Same design as synthesize-hash-xorfold, through Bambu's "
                 "I386_GCC8 frontend. Expected to fail the same way. "
                 "See README.md.",
    always_build=True,
)
