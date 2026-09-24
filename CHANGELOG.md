# Changelog

## 0.7.0 (not yet tagged)

### Added
- **`Expand<O>` / `Expansion<Children,Queried,Selected,Validates,Searched>` / `IsContainer<O>`**: one extension point that says what a
  container holds and which walks may open it. `Traverse`, `FindFirst`, `BuildRules` and `NoCollision` all read it, so a new
  container is taught once instead of once per walk. `Chain` (all four bits) and `APIOf` (`validates` only) are built in. See the README
  section "Teaching HAPI your own container" and `docs/REFERENCE.md`.
- `Chain<OO...>::Drop<n>`: the chain without its first `n` elements.
- Tests: `tests/expand_tests.cpp`, `tests/expand_walks.cpp` (one container per policy bit), `tests/descent_characterization.cpp` (pins how every
  walk treats every kind of container), and `tests/negative/` (expect-to-fail cases: `tests/negative/run.sh`, `CXX=clang++` to switch
  compiler; not part of CI's `tests/*.cpp` glob).

### Changed
- `Traverse`, `FindFirst`, `BuildRules` and `NoCollision` no longer carry per-container special cases (the nested-`Chain` and nested-`APIOf`
  splices in `rules.h` and `hapi.h` are gone); they ask `Expand`. Behaviour for `Chain` and `APIOf` is unchanged.
- A hand-written `Traverse<Op,X<...>>` specialization still works and wins over the default, so existing containers keep working;
  `Expand` is the preferred way. An `Expand` entry must be declared before the type is first queried (g++ enforces it).
- `BuildRules`: a container with `validates` is replaced by its children in place, and **its own `rules()` still runs** (with its later
  siblings as `After`). `HasOwnRules<O>` decides whether to probe a container for one; `Chain` and `APIOf` are never probed.
- `Expand`'s primary template is declared but not defined (a leaf), and the walks dispatch through alias templates, so asking about a
  leaf instantiates one class per element. Traverse-based walks cost about one extra template instantiation per element visited;
  `Chain::Map` and friends are unaffected.
- Generated code is unchanged in everything checked: the AVR builds' sizes and disassembly, OneParse's benchmark binaries (identical machine
  code), OneHLS's synthesis tops (identical objects), and the RTL from Bambu 2024.10 and Vitis HLS 2026.1 for the OneHLS and HAPI `hls_*` examples
  (identical, except one soft-float design that Bambu itself generates nondeterministically).

### Downstream libraries that need this version (`"HAPI": ">=0.7.0"`)
- **OneMenu, OneItem, OneData** replaced their hand-written `Traverse` and `query<>` specializations with `Expand` entries.
  - Wrapper components (`Hidden`, `Decor`, `NumField`, `EnDis`, `MenuPrinter`, `ItemPrinter`, `AsFmt`) now set `validates`: the `rules()` of the
    components they hold run as if placed directly. **This can newly reject code** that hid a violation inside a wrapper, e.g.
    `ItemDef<Hidden<ParentDraw,ItemNav>>` (already rejected without the `Hidden`). Fix by moving or removing the offending component.
  - OneMenu's `ItemDef` is `queried` but not `selected`: queries look inside an item, `Filter`/`Map`/`Partition` take it whole. This fixes
    `View<Tag,Body>::Types` returning nothing.
  - `Menu`, `StaticBody`, `JoinBody` and `ItemDef` deliberately do not set `validates`: they hold other items, and flattening those into one
    rule context gives false positives (e.g. a `PadDraw` menu around items that each carry `ParentDraw`).
- agnosticism's `snet::Net` declares its cells with an `Expand` entry (queried, selected, searched).

### Known limitations
- `NoCollision` only opens a container when it is the head of the list being checked, so an earlier sibling is not compared against a later
  container's children (`Chain<Leaf, APIOf<Colliding>>` is not flagged, `Chain<APIOf<Colliding>, Leaf>` is). Existed before 0.7.0; pinned by a test.
- `examples/godbolt/crc6_hapi.cpp` is a hand-inlined copy of the pre-0.7 core and does not follow later changes.
- The new tests under `tests/` are compiled by CI with MSVC as well; they have only been run with g++, clang++ and avr-g++.
