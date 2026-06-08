# DESIGN.md

## HAPI & OneData Architecture Blueprint

### Zero-Overhead, Static Composition for High-Reliability Embedded Systems

---

## 1. Core Principles

Two principles govern all modules in the HAPI/OneData ecosystem:

**No runtime allocation.** Everything lives on the stack or in static storage. No `new`, no `malloc`, no heap. On AVR with 2KB of SRAM, heap fragmentation is a silent failure mode — it is eliminated by construction.

**No runtime recursion.** Function call recursion consumes stack deterministically but unboundedly. All recursion in HAPI is *template recursion* — resolved entirely at compile time, consuming zero stack at runtime. Runtime loops and branching belong only in high-level orchestration layers, not in data pipeline components.

> **Distinction:** template recursion (`Chain::Part`, `BuildRules`) is compile-time and costs nothing at runtime. Function call recursion (`f()` calling `f()`) consumes stack and is banned in core components.

---

## 2. Composition Topology

OneData uses HAPI's open chain derivation to compose data components into a single flat object with no runtime indirection.

```
+--------------------------------------------------+
|          DataDef<Watch<Data<int>>, NumRange<int>>|  ← user declaration
+--------------------------------------------------+
                        |
                        v  [compile-time fold]
+--------------------------------------------------+
|  Watch::Part< NumRange::Part< Data::Part<Nil> > >|  ← single flat class
+--------------------------------------------------+
```

Each layer adds behaviour without adding indirection. The compiler sees the complete structure and inlines everything. The result is equivalent to hand-written code with no framework overhead.

Compare with runtime polymorphism (`OneList` / vtable models):
- Virtual dispatch: pointer dereference + indirect call — cache miss risk on every call
- Static composition: direct inlined call — no indirection, no overhead

---

## 3. Data Storage Strategies

Three storage strategies, each a composable layer:

| Component | Storage | Ownership | AVR cost |
|---|---|---|---|
| `StaticData<T, value>` | compile-time constant | none | 0 bytes RAM |
| `Data<T>` | owned member | owns value | `sizeof(T)` bytes |
| `DataRef<T, Ref>` | reference to external | borrows | 0 bytes (no pointer stored) |

`DataRef` is zero-cost because the reference is a compile-time template parameter — the address is embedded in the generated code, not stored as a runtime pointer.

---

## 4. Type Passing Rules

### Scalars: pass by value

For scalar types (`int`, `bool`, `char`, `uint8_t`) on AVR, pass by value rather than `const T&`:

```cpp
// prefer this for scalars:
void set(int v) { data = v; }

// avoid this for scalars on AVR:
void set(const int& v) { data = v; }
```

**Why:** passing a `const T&` to a scalar literal forces the compiler to construct a temporary on the stack. Pass-by-value allows the compiler to bind the parameter directly into a working register (`R24/R25` on AVR), bypassing the stack frame.

### Aggregates: pass by const reference

For structs and arrays, `const T&` remains correct — copying a large aggregate onto the stack costs more than the reference overhead.

### Volatile registers

For hardware register targets, the referenced variable must be `volatile`:

```cpp
volatile uint8_t PORTB_REG = 0;
using PinOut = DataRef<volatile uint8_t, PORTB_REG>;
```

Without `volatile`, the compiler may cache the value in a register and skip the actual hardware write. A non-volatile reference to a hardware register is undefined behaviour on most architectures.

### Interrupt safety

If a `DataRef` target can be modified by an ISR, atomic access is required:

```cpp
// AVR atomic access
ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
  ref.set(value);
}
```

OneData does not provide atomic wrappers — this is the caller's responsibility when the target is ISR-shared.

---

## 5. String and Text Handling

`const char*` in C++ is four different things depending on context:

```cpp
const char*        // pointer to const char — pointer mutable, chars immutable
char* const        // const pointer to char — pointer immutable, chars mutable
const char* const  // const pointer to const chars — nothing mutable
const char[]       // array — decays to const char* on pass, size lost
const char (&)[N]  // reference to sized array — size N preserved
```

OneData uses `CText = const char*` for runtime text — the pointer is owned, the string it points to is not. This is correct for string literals (which live in rodata/flash) but unsafe for runtime-constructed strings (which may be freed while the pointer is held).

### AVR PROGMEM

String literals on AVR are copied to RAM by default unless explicitly placed in flash:

```cpp
// RAM — costs SRAM
const char label[] = "Temperature";

// Flash — costs only flash, needs pgm_read_byte to access
const char label[] PROGMEM = "Temperature";
```

`StaticText` and `StaticData` for string types on AVR should use `PROGMEM` placement and `pgm_read_byte` access. This is platform-specific and not handled by OneData core — it is the responsibility of the AVR output sink layer.

---

## 6. Inlining and Stack Safety

The compiler inlines aggressively when:
- The function body is small
- The call site is within a `constexpr` or template context
- LTO (Link Time Optimisation) is enabled

Inlining breaks down when:
- The function contains runtime loops over variable-length data
- The function is called through a function pointer or virtual dispatch
- The call graph is too deep for the compiler's inlining budget

For OneData components, keep `Part` implementations small and loop-free. Loops over runtime-determined sizes belong in the terminal layer (the sink), not in intermediate pipeline components.

`__attribute__((always_inline))` can force inlining when the compiler's heuristic is wrong — use sparingly and verify with binary inspection.

---

## 7. Binary Validation Protocol

Do not trust source-level reasoning about overhead. Verify the generated binary.

### RAM footprint

```bash
avr-size --format=avr --mcu=atmega328p firmware.elf
```

`StaticData`, `StaticNumRange`, and `DataRef` components should contribute **0 bytes** to `.data` and `.bss`. If they appear in the map, something was not resolved at compile time.

### Inlining verification

```bash
avr-objdump -S firmware.elf | grep -E "call|rcall"
```

Data pipeline components should produce no `CALL` or `RCALL` instructions at the call site. Direct `STS`, `OUT`, or `ST` instructions confirm zero-overhead composition.

### Stack depth analysis

```makefile
CXXFLAGS += -fstack-usage -flto
```

Analyse the generated `.su` files to verify that the deepest call chain fits within the available stack under worst-case interrupt nesting. On AVR with 2KB SRAM, a typical safe stack budget is 256–512 bytes depending on static allocation.

### Map file inspection

```makefile
LDFLAGS += -Wl,-Map=firmware.map
```

Inspect the map file to verify that `StaticData` constants land in `.text` (flash) rather than `.data` (RAM-initialised) or `.bss` (zero-initialised RAM).

---

*Part of the [InternetOfPins](https://github.com/InternetOfPins) project family.*  
*Author: Rui Azevedo (neu-rah) · Azores, Portugal · MIT License*