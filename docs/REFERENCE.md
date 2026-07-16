# HAPI API Reference

Hardware Abstraction Pattern Interface. Zero-cost template metaprogramming for composable hardware layers.

## Core Concept

HAPI composes hardware functionality into compile-time chains. Each layer wraps and extends the layer below it, forming a heterogeneous linked list at compile-time that optimizes to direct hardware access.

## Building Blocks

### Chains

| Type | Purpose |
|------|---------|
| `Chain<Components...>` | Compose multiple components in order |
| `APIOf<Terminal, Components...>` | Attach components to a terminal API |
| `Part<O>` | CRTP wrapper for a component within a chain |

### Traversal & Querying

| Return | Function | Params | Description |
|--------|----------|--------|-------------|
| `T*` | `FindFirst<Predicate>(chain)` | predicate | Find first component matching condition |
| `void` | `Traverse<fn>(chain)` | function | Visit every component in chain |
| `bool` | `Any<Predicate>(chain)` | predicate | Does any component match? |

### Functional Transforms

| Type | Purpose |
|------|---------|
| `Map<Fn, Chain>` | Transform each component |
| `Filter<Predicate, Chain>` | Keep components matching condition |
| `At<Index, Default>` | Get indexed component or default |

## Tag-Based Filtering

Mark components with compile-time tags for later lookup.

**Common Tags**:
| Tag | Meaning |
|-----|---------|
| `asCursor` | Tracks selection position |
| `asFormat` | Applies text formatting |
| `asPrinter` | Renders output |
| `asParser` | Parses input |

## Integration with IOP Libraries

- **OnePin**: Composes port and mask layers via APIOf
- **OneMenu**: Chains menu printers and item definitions
- **OneOutput**: Formats and buffers output via component layers

## Rationale

HAPI provides compile-time composition that looks like inheritance but generates zero-overhead machine code. Drivers are modular and reusable while remaining as fast as hand-written assembly.

---

## See Also

- [OneChip](../../../OneChip/docs/REFERENCE.md) — Hardware platform layers
- [OneMenu](../../../OneMenu/docs/REFERENCE.md) — Menu framework
- [OneData](../../../OneData/docs/REFERENCE.md) — Data observation layer
