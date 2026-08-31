use std::env;
use std::fs::File;
use std::io::Write;
use std::path::PathBuf;
use std::process::Command;

fn main() {
    let out = PathBuf::from(env::var_os("OUT_DIR").unwrap());
    File::create(out.join("memory.x"))
        .unwrap()
        .write_all(include_bytes!("memory.x"))
        .unwrap();
    println!("cargo:rustc-link-search={}", out.display());
    println!("cargo:rerun-if-changed=memory.x");

    // ── Phase B: compile the C++ shim with the same real
    // arm-none-eabi-g++ toolchain focCompose already uses (PlatformIO-
    // bundled, no new toolchain), and link the result into the Rust
    // binary. Not using the `cc` crate: its target-detection heuristics
    // don't know about `thumbv7m-none-eabi`, so invoking the real
    // compiler directly is more explicit and easier to reason about here.
    let gxx = env::var("ARM_GXX").unwrap_or_else(|_| {
        let home = env::var("HOME").unwrap();
        format!("{home}/.platformio/packages/toolchain-gccarmnoneeabi/bin/arm-none-eabi-g++")
    });
    let ar = env::var("ARM_AR").unwrap_or_else(|_| {
        let home = env::var("HOME").unwrap();
        format!("{home}/.platformio/packages/toolchain-gccarmnoneeabi/bin/arm-none-eabi-ar")
    });
    // Sibling-repo include dirs, same "override via env var, sane default
    // if the whole IOP tree is checked out locally" pattern as HAPI_INCLUDE.
    // lcd_shim.cpp (real I2C1 + PCF8574/HD44780 LCD) needs all of these;
    // shim.cpp (the original trivial Counter/DoubleStep test) needs none
    // of them, only HAPI_INCLUDE.
    let hapi_include     = env::var("HAPI_INCLUDE").unwrap_or_else(|_| "../../include".to_string());
    let onedata_include  = env::var("ONEDATA_INCLUDE").unwrap_or_else(|_| "../../../OneData/include".to_string());
    let onebus_include   = env::var("ONEBUS_INCLUDE").unwrap_or_else(|_| "../../../OneBus/include".to_string());
    let oneio_include    = env::var("ONEIO_INCLUDE").unwrap_or_else(|_| "../../../OneIO/include".to_string());
    let onechip_include  = env::var("ONECHIP_INCLUDE").unwrap_or_else(|_| "../../../OneChip/include".to_string());
    let onepin_include   = env::var("ONEPIN_INCLUDE").unwrap_or_else(|_| "../../../OnePin/include".to_string());
    let oneoutput_include = env::var("ONEOUTPUT_INCLUDE").unwrap_or_else(|_| "../../../OneOutput/include".to_string());
    let onebit_src        = env::var("ONEBIT_SRC").unwrap_or_else(|_| "../../../OneBit/src".to_string());
    let onebit_include     = env::var("ONEBIT_INCLUDE").unwrap_or_else(|_| "../../../OneBit/include".to_string());

    println!("cargo:rerun-if-changed=cpp/shim.cpp");
    println!("cargo:rerun-if-changed=cpp/lcd_shim.cpp");
    for v in [
        "ARM_GXX", "ARM_AR", "HAPI_INCLUDE", "ONEDATA_INCLUDE", "ONEBUS_INCLUDE",
        "ONEIO_INCLUDE", "ONECHIP_INCLUDE", "ONEPIN_INCLUDE", "ONEOUTPUT_INCLUDE",
        "ONEBIT_SRC", "ONEBIT_INCLUDE",
    ] {
        println!("cargo:rerun-if-env-changed={v}");
    }

    let common_flags = [
        "-std=c++17", "-Os",
        "-mcpu=cortex-m3", "-mthumb", "-mfloat-abi=soft",
        "-fno-exceptions", "-fno-rtti", "-fno-threadsafe-statics", "-fno-unwind-tables",
    ];

    let compile = |src: &str, obj: &PathBuf, extra_includes: &[&str]| {
        let mut cmd = Command::new(&gxx);
        cmd.args(common_flags);
        cmd.args(["-I", &hapi_include]);
        for inc in extra_includes {
            cmd.args(["-I", inc]);
        }
        cmd.args(["-c", src, "-o"]).arg(obj);
        let status = cmd.status().unwrap_or_else(|e| panic!(
            "rust_stm32_bridge: could not run {gxx} ({e}) -- set ARM_GXX to point at a real \
             arm-none-eabi-g++ (PlatformIO's toolchain-gccarmnoneeabi package, or any other)."
        ));
        if !status.success() {
            panic!("rust_stm32_bridge: {gxx} failed to compile {src} (see errors above)");
        }
    };

    let shim_o = out.join("shim.o");
    compile("cpp/shim.cpp", &shim_o, &[]);

    let lcd_shim_o = out.join("lcd_shim.o");
    compile("cpp/lcd_shim.cpp", &lcd_shim_o, &[
        &onedata_include, &onebus_include, &oneio_include, &onechip_include,
        &onepin_include, &oneoutput_include, &onebit_src, &onebit_include,
    ]);

    let lib = out.join("libshim.a");
    let status = Command::new(&ar)
        .args(["crs"])
        .arg(&lib)
        .arg(&shim_o)
        .arg(&lcd_shim_o)
        .status()
        .unwrap_or_else(|e| panic!("rust_stm32_bridge: could not run {ar} ({e})"));
    if !status.success() {
        panic!("rust_stm32_bridge: {ar} failed to archive shim objects");
    }

    println!("cargo:rustc-link-lib=static=shim");
}
