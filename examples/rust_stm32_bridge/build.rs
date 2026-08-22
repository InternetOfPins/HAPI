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
    let hapi_include = env::var("HAPI_INCLUDE").unwrap_or_else(|_| "../../include".to_string());

    println!("cargo:rerun-if-changed=cpp/shim.cpp");
    println!("cargo:rerun-if-env-changed=ARM_GXX");
    println!("cargo:rerun-if-env-changed=ARM_AR");
    println!("cargo:rerun-if-env-changed=HAPI_INCLUDE");

    let shim_o = out.join("shim.o");
    let status = Command::new(&gxx)
        .args([
            "-std=c++17", "-Os",
            "-mcpu=cortex-m3", "-mthumb", "-mfloat-abi=soft",
            "-fno-exceptions", "-fno-rtti", "-fno-threadsafe-statics", "-fno-unwind-tables",
            "-I", &hapi_include,
            "-c", "cpp/shim.cpp",
            "-o",
        ])
        .arg(&shim_o)
        .status()
        .unwrap_or_else(|e| panic!(
            "rust_stm32_bridge: could not run {gxx} ({e}) -- set ARM_GXX to point at a real \
             arm-none-eabi-g++ (PlatformIO's toolchain-gccarmnoneeabi package, or any other)."
        ));
    if !status.success() {
        panic!("rust_stm32_bridge: {gxx} failed to compile cpp/shim.cpp (see errors above)");
    }

    let lib = out.join("libshim.a");
    let status = Command::new(&ar)
        .args(["crs"])
        .arg(&lib)
        .arg(&shim_o)
        .status()
        .unwrap_or_else(|e| panic!("rust_stm32_bridge: could not run {ar} ({e})"));
    if !status.success() {
        panic!("rust_stm32_bridge: {ar} failed to archive shim.o");
    }

    println!("cargo:rustc-link-lib=static=shim");
}
