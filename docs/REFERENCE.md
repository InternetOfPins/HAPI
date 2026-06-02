# HAPI — Happy API

> **Zero-overhead static composition for embedded systems and modern C++**  
> Stack features like layers. Pay only for what you use. Catch mistakes at compile time.

```cpp
// Declare a full-featured TUI output device in one line
OutDef<
  ScrollPrinter, ANSIFmt, ClearFreeFmt,
  DataParser<>, CtrlChars, UTF8, TextWrap, Clip,
  ColorTrack<int>, Cursor, Gate, ANSIOut,
  ConsoleOut, StaticPos<20,10>, StaticArea<30,8>
> out;
```

No virtual dispatch. No heap allocation. Wrong layer order → **named compile error**.  
The same pipeline compiles to AVR UART or POSIX stdout by swapping one layer.

---

## The Win-Win Architecture

Embedded development traditionally forces a choice: clean, modular code *or* hardware performance. HAPI eliminates that tradeoff by shifting all structural complexity onto the compiler.

- **The developer wins** — write highly expressive, modular, reusable code without coupling or architectural overhead. Composition is declared, not wired.
- **The hardware wins** — the silicon receives a perfectly flat, optimised instruction block. No vtables, no dynamic allocation, no runtime indirection. Code runs at the physical limit of the target, whether an 8-bit AVR or a 32-bit ARM.
- **The compiler pays the price** — all abstraction cost is paid in build-time seconds. The final binary contains none of it.

Validation follows the same principle. If a composition violates a declared constraint — wrong layer order, missing dependency, duplicate component — the compiler refuses to produce a binary and reports a named error. There is no such thing as a structurally broken HAPI program that compiles.

---

## Contents

- [What is HAPI?](#what-is-hapi)
- [Core Concepts](#core-concepts)
- [Quick Start](#quick-start)
- [Examples](#examples)
  - [Hello World — minimal output](#hello-world--minimal-output)
  - [Composing features step by step](#composing-features-step-by-step)
  - [Cross-platform output (AVR + desktop)](#cross-platform-output-avr--desktop)
  - [Compile-time constraint enforcement](#compile-time-constraint-enforcement)
  - [Runtime polymorphism bridge](#runtime-polymorphism-bridge)
  - [Query the composition](#query-the-composition)
- [Architecture](#architecture)
- [Layer Reference](#layer-reference)
- [Building Your Own Layer](#building-your-own-layer)
- [Platform Support](#platform-support)
- [Related Projects](#related-projects)

---

## What is HAPI?

HAPI is a **static composition engine**. You declare a stack of feature types; HAPI folds them into a single class via recursive CRTP mixin inheritance. The result:

- Every method call resolves at compile time — no vtable, no indirection
- Unused features cost zero bytes of flash or RAM
- Layer ordering is validated at compile time with human-readable error messages
- The same feature stack compiles for AVR (2KB RAM), ESP32, ARM, or desktop

HAPI is the architectural foundation for [ArduinoMenu v5 (AM5)](https://github.com/neu-rah/AM5), a ground-up rewrite of the [ArduinoMenu](https://github.com/neu-rah/ArduinoMenu) library.

---

## Core Concepts

### `Chain<A, B, C, ...>`

A compile-time type list. Knows its size, head, tail, and how to map or append types.

```cpp
using MyChain = Chain<Alpha, Beta, Gamma>;
// MyChain::size  == 3
// MyChain::Head  == Alpha
// MyChain::Tail  == Chain<Beta, Gamma>
```

### `Chain<...>::Part<Base>`

The composition operator. Folds the type list into a single class by recursive inheritance:

```cpp
Chain<A, B, C>::Part<Base>
// expands to:
// A::Part< B::Part< C::Part<Base> > >
```

Each type `A`, `B`, `C` contributes a `Part<O>` template that wraps the layer below it. This is standard CRTP mixin — HAPI automates the fold.

### `APIOf<API, A, B, C, ...>`

The public-facing composition entry point. Wraps `Chain<A,B,C>::Part<API>` with a clean name:

```cpp
struct MyAPI { /* public interface */ };

using MyDevice = APIOf<MyAPI, FeatureA, FeatureB, FeatureC>;
MyDevice dev;
dev.someMethod(); // resolves through the full feature stack
```

### `query<Q, T>`

Compile-time introspection. Ask whether a type or chain satisfies a predicate:

```cpp
// Does the stack contain a cursor tracker?
static_assert(query<IsCursor, MyDevice>, "need a cursor");

// Sugar form:
static_assert(has<IsCursor, FeatureA, FeatureB>, "...");
```

---

## Quick Start

### 1. Include

```cpp
#include "hapi/hapi.h" //single include file, namespace hapi::
```

### 2. Define feature layers

Each layer is a struct with an inner `Part<O>` template that extends `O`:

```cpp
struct Logger {
  template<typename O>
  struct Part : O {
    void put(char c) {
      log_char(c);       // add behavior
      O::put(c);         // pass through
    }
  };
};
```

### 3. Compose and use

```cpp
using MyOut = APIOf<OutputAPI, Logger, Formatter, PhysicalSink>;
MyOut out;
out.put('!');
```

---

## Examples

### Hello World — minimal output

```cpp
#include "hapi.h"
using namespace hapi;

// A sink: the bottom layer, actually writes characters
struct StdoutSink {
  template<typename O>
  struct Part : O {
    void put(char c) { putchar(c); }
    void nl()        { putchar('\n'); }
  };
};

// An empty API base
struct BasicAPI {};

// Compose: just a sink, nothing else
using Out = APIOf<BasicAPI, StdoutSink>;
Out out;

int main() {
  out.put('H'); out.put('i'); out.nl();
  // prints: Hi
}
```

---

### Composing features step by step

Each layer you add wraps the one below. Layers higher in the list see output first.

```cpp
// Layer 1: count characters written
struct CharCounter {
  template<typename O>
  struct Part : O {
    void put(char c) { m_count++; O::put(c); }
    size_t count() const { return m_count; }
  private:
    size_t m_count{0};
  };
};

// Layer 2: convert lowercase to uppercase
struct UpperCase {
  template<typename O>
  struct Part : O {
    void put(char c) {
      O::put(c >= 'a' && c <= 'z' ? c - 32 : c);
    }
  };
};

// Stack: UpperCase sees input first, then CharCounter, then StdoutSink
using Out = APIOf<BasicAPI, UpperCase, CharCounter, StdoutSink>;
Out out;

out.put('h'); out.put('i'); out.nl();
// prints: HI
// out.count() == 3  (H, I, \n)
```

---

### Cross-platform output (AVR + desktop)

Swap only the sink layer. Everything above is identical.

```cpp
struct AVRSerialSink {
  template<typename O>
  struct Part : O {
    void put(char c) { Serial.write(c); }
    void nl()        { Serial.write('\n'); }
  };
};

struct POSIXConsoleSink {
  template<typename O>
  struct Part : O {
    void put(char c) { putchar(c); }
    void nl()        { putchar('\n'); }
  };
};

// Common feature stack
template<typename Sink>
using MyOut = APIOf<BasicAPI, UpperCase, CharCounter, Sink>;

#ifdef __AVR__
  MyOut<AVRSerialSink> out;
#else
  MyOut<POSIXConsoleSink> out;
#endif

// identical usage on both platforms:
out.put('h'); out.put('i'); out.nl();
```

Or with the `#ifdef` inside the declaration, as in AM5:

```cpp
OutDef<
  ScrollPrinter, DataParser<>, TextWrap, Clip, Cursor, Gate,
  #ifdef __AVR__
    SerialOut,
  #else
    ConsoleOut,
  #endif
  StaticPos<0,0>, StaticArea<80,24>
> out;
```

---

### Compile-time constraint enforcement

Layers can declare what must (or must not) come above or below them using `query` and `static_assert`. Wrong stacking order produces a named error, not a runtime mystery.

```cpp
// Tag type: marks that DataParser is present in the stack
struct IsDataParser { 
  template<typename O> struct Check {
    static constexpr bool value = std::is_same_v<typename O::IsDataParser, std::true_type>;
  };
};

struct DataParser {
  template<typename O>
  struct Part : O {
    using IsDataParser = std::true_type;  // publish tag
    // ... implementation
  };
};

struct Clip {
  template<typename O>
  struct Part : O {
    // Clip needs DataParser somewhere below it in the stack
    static_assert(
      query<IsDataParser, O>,
      "Clip requires DataParser<> to be placed below it in the stack"
    );
    void put(char c) {
      if (freeX() > 0 && freeY() > 0) O::put(c);
    }
  };
};

// ✓ Correct order — DataParser is below Clip
using Good = APIOf<MyAPI, Clip, DataParser, Cursor, Gate, ConsoleSink>;

// ✗ Wrong order — static_assert fires with the message above
using Bad  = APIOf<MyAPI, DataParser, Clip, Cursor, Gate, ConsoleSink>;
//  error: "Clip requires DataParser<> to be placed below it in the stack"
```

The full AM5 output pipeline enforces:

| Layer | Requires below | Requires above |
|---|---|---|
| `Clip` | `DataParser`, `Cursor`, `Gate` | — |
| `TextWrap` | `DataParser` | — |
| `UTF8` | `DataParser` | — |
| `Cursor` | `DataParser` | — |
| `Gate` | — | `DataParser`, all formatters |
| `Buffer` | `Gate`, `Cursor` | `Clip`, `TextWrap` |

---

### Runtime polymorphism bridge

Static composition is zero-overhead but can't cross a virtual boundary. `IOutDef` solves this: it inherits from both `IOut` (abstract interface) and the composed stack, bridging the two worlds.

```cpp
// Abstract interface — store and pass by pointer, no template needed
struct IOut {
  virtual void put(char c) = 0;
  virtual void nl() = 0;
  virtual ~IOut() = default;
};

// Concrete composed type that also satisfies IOut
template<typename... OO>
struct IOutDef : IOut, APIOf<BasicAPI, OO...> {
  using Base = APIOf<BasicAPI, OO...>;
  void put(char c) override { Base::put(c); }
  void nl()        override { Base::nl(); }
};

// Concrete instance (zero-overhead on the hot path)
IOutDef<UpperCase, CharCounter, ConsoleSink> concrete;

// Store as abstract pointer (e.g. pass to a menu renderer)
IOut* iout = &concrete;
iout->put('x'); // virtual dispatch here only
```

This means a menu system can hold `IOut*` and work with any output device, while the device itself has no overhead in its own execution path.

---

### Query the composition

Ask compile-time questions about what a stack contains:

```cpp
using MyStack = Chain<UpperCase, CharCounter, ConsoleSink>;

// Does this stack contain CharCounter?
constexpr bool has_counter = query<SameAs<CharCounter>, MyStack>;  // true

// Sugar: has<T, types...>
constexpr bool b = has<CharCounter, UpperCase, CharCounter, ConsoleSink>;  // true

// Use in static_assert inside a layer:
template<typename O>
struct Part : O {
  static_assert(query<SameAs<Gate>, O>, "Gate must be present below this layer");
};
```

---

## Architecture

```
┌─────────────────────────────────────────────────────┐
│  User code                                          │
│  out.put("hello");  out << value << endl;           │
└────────────────────┬────────────────────────────────┘
                     │
┌────────────────────▼────────────────────────────────┐
│  APIOf<API, L1, L2, L3, ..., Sink>                  │
│                                                     │
│  expands to:                                        │
│  L1::Part< L2::Part< L3::Part< ... Sink ... > > >  │
│                                                     │
│  Each Part<O> :                                     │
│    • inherits O (the layer below)                   │
│    • overrides/extends methods                      │
│    • passes through via O::method()                 │
│    • may assert on O's type tags                    │
└────────────────────┬────────────────────────────────┘
                     │  all resolved at compile time
┌────────────────────▼────────────────────────────────┐
│  Physical sink  (SerialOut / ConsoleOut / Buffer)   │
└─────────────────────────────────────────────────────┘
```

The fold is right-to-left: `L1` is outermost (first to receive a call), `Sink` is innermost (last). Inserting a layer anywhere in the list changes behaviour at that point in the chain without touching anything else.

---

## Layer Reference

Layers used in the AM5 output pipeline, in typical stack order (top = first to receive):

| Layer | Purpose |
|---|---|
| `ScrollPrinter` | High-level scrolling menu/list API |
| `ANSIFmt` | Semantic formatting intent (bold, colour names) |
| `ClearFreeFmt` | Handles clear/erase so user code doesn't have to |
| `DataParser<N>` | Serialises typed values (`int`, `double`, `char*`) → char stream. Buffer size `N` (default 16) |
| `CtrlChars` | Interprets `\n` → `nl()`, `\t` → padding, etc. |
| `UTF8` | Passes UTF-8 surrogate bytes directly to sink so cursor counts one character per glyph |
| `TextWrap` | Inserts `nl()` when `free().x <= 0` |
| `Clip` | Suppresses characters outside the declared area |
| `ColorTrack<T>` | Remembers current fg/bg colours for resume/redraw |
| `Cursor` | Tracks `(x, y)` position; provides `freeX()`, `freeY()`, `nl()` |
| `Gate` | Conditional pass-through; supports `LockMode::None / Measure / Update` |
| `ANSIOut` | Translates abstract format intent → ANSI escape codes |
| `SerialOut` | AVR/Arduino UART sink |
| `ConsoleOut` | POSIX stdout sink |
| `Buffer<Scroll,Fill>` | Character panel buffer with optional scroll and change tracking |
| `StaticPos<x,y>` | Compile-time origin — zero runtime cost |
| `StaticArea<w,h>` | Compile-time bounds — zero runtime cost |
| `DeviceCursor` | Records edit-cursor position for format-driven restoration |
| `ColorTrack<T>` | Tracks colour state across resume cycles |
| `PartialDraw` | Signals that the device supports partial update |
| `Raw` | Direct pass-through to sink, bypasses all layers above |

---

## Building Your Own Layer

A layer is any struct with an inner `Part<O>` template. `O` is the layer below.

```cpp
struct MyLayer {
  template<typename O>
  struct Part : O {
    using Base = O;

    // Optional: publish a tag so other layers can query your presence
    using IsMyLayer = std::true_type;

    // Optional: assert on what must be below you
    static_assert(
      !query<SomePredicate, O> || query<SomeOtherPredicate, O>,
      "MyLayer requires SomeOtherLayer to be placed below it"
    );

    // Override methods you care about; forward everything else via O::
    void put(char c) {
      // do something
      Base::put(c);  // pass through
    }

    // Add new methods freely; they become part of the composed API
    void myMethod() { /* ... */ }
  };
};
```

Rules of thumb:
- Always forward to `O::method()` unless you intentionally want to suppress output (like `Gate` or `Clip`).
- Keep `Part` stateless if possible — state costs RAM on every instance.
- Use `static_assert` + `query` for any ordering constraint you know about.
- Publish a tag `using IsXxx = std::true_type` if other layers might need to detect you.

---

## Platform Support

| Platform | Sink layer | Notes |
|---|---|---|
| AVR (Uno, Mega, …) | `SerialOut` | `using size_t = unsigned int`; no `<type_traits>` — uses `platform/avr/avr_std.h` |
| ESP8266 / ESP32 | `SerialOut` | Full C++17 available |
| STM32 / ARM | `SerialOut` | Standard C++17 |
| Teensy | `SerialOut` | Standard C++17 |
| POSIX / Linux / macOS | `ConsoleOut` | Full C++17; used for desktop development and testing |
| Windows | `ConsoleOut` | C++17 with MSVC or clang |

The `#ifdef __AVR__` block in `hapi.h` provides a minimal `<type_traits>` / `<utility>` substitute for AVR targets where the standard library is unavailable.

---

## Related Projects

| Project | Description |
|---|---|
| [ArduinoMenu v5 (AM5)](https://github.com/neu-rah/AM5) | Full TUI menu framework built on HAPI |
| [ArduinoMenu v4](https://github.com/neu-rah/ArduinoMenu) | Previous generation — 1k★, 197 forks, GitHub Arctic Code Vault |
| [OneList](https://github.com/InternetOfPins/OneList) | Heterogeneous runtime list with compile-time type mirror |
| [streamFlow](https://github.com/neu-rah/streamFlow) | Lightweight `<<` stream operator for Arduino |

---

## Author

**Rui Azevedo** (neu-rah) — ruihfazevedo@gmail.com  
Azores, Portugal · [github.com/neu-rah](https://github.com/neu-rah) · [github.com/InternetOfPins](https://github.com/InternetOfPins)

---

## License

See [LICENSE](LICENSE) for details.