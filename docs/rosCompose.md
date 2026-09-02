# Case study — reducing ROS 2's composition surface onto HAPI

> **Status: Demonstrated (structurally).** A local scouting exercise read
> ROS 2 (`rclcpp`, `rcl`, `rcl_action`, `tf2`) from source and mapped its
> composition surface onto HAPI's primitive set. The buildable result is
> [`examples/rosCompose/`](../examples/rosCompose/). No live DDS / socket
> round trip was verified — this is a claim about how the *types* compose
> and what the generated code contains, not about ROS interoperability.

ROS was used as a **reference oracle only** — a well-specified, widely
understood example of "typed multi-endpoint device structure with a
late-bound, no-lookahead transport" — never as an integration target.

## The result

The ROS composition surface reduces to **three primitives HAPI /
InternetOfPins already had, plus two genuinely new fixed-capacity
components.**

### Three pre-existing primitives

1. **Causal-boundary transport** — a static fan-out fold
   (`hapi::Chain`) over the locally-known endpoints, terminated by one
   virtual `Cap<Msg>` for everyone reachable only across the transport.
   One indirect hop at the boundary, nothing added. Covers
   fire-and-forget topics, correlated request/response (the cap reused
   twice, one hop per direction), and best-effort-reliable delivery.
2. **The decorator idiom** — `WithHistory`, `WithDeadline` (and the same
   shape for retry state) fold a cross-cutting concern into a subscriber
   through the *unchanged* `Subscriber<Msg,Body>` mechanism. This is the
   same pattern OneMenu uses for item decorators; the scout confirmed it
   is reusable infrastructure, not a per-feature trick.
3. **Fixed-capacity tables** — no heap, no `std::`, ring wraparound by
   conditional subtract (not `% N`, which is a soft-division call on
   AVR). Already present as `InList<N>` and used here for the request
   correlation table, the dedupe window, and QoS history.

### Two genuinely new components

Both are focused, general, no-heap fixed-capacity structures — **not**
composition machinery in the `chain.h` / `meta.h` sense. They are the
only components the scout produced that did not already exist anywhere in
InternetOfPins.

| component | what it does | first surfaced by | second, independent need |
|---|---|---|---|
| `ReorderBuffer<T,N>` | hold items that arrived out of contiguous-key order; release the front run once the gap fills | reliable **ordered** delivery (QoS) | segmented-frame / chunked-transfer reassembly |
| `TimeBuffer<T,N,Time>` | bounded time-sorted history; `lookup(t)` returns the bracketing pair (tf2 `findClosest` semantics, `0/1/2`), division-free | `tf2` time-indexed transform lookup | IMU/camera sensor-fusion timestamp alignment |

They share only the fixed-capacity / no-heap family trait — on every
structural axis (addressing, destructive vs. resident read, one element
vs. a pair, insert cost) they diverge. A single unified `ElasticBuffer`
type would be a shared name over two different data structures. Each was
built and confirmed against a real non-ROS consumer before being called
general.

**Home: OneHLS.** Both synthesize to clean RTL under Bambu (from-source
`dev/panda`, `xc7z020`, 10 ns): BRAM-backed, **0 DSPs**, `& (N-1)` ring
mask lowers without a modulo, ~100 MHz. `TimeBuffer`'s raw `(num,den)`
`lookup` overload keeps it division-free in hardware (the `double`
convenience form pulls a 64-bit divide). That places them with
[`OneHLS/examples/hls_streaming_buffers`](https://github.com/InternetOfPins/OneHLS)'s
`Fifo` / skid-buffer family — bounded reorder and time-indexed history
are streaming-datapath primitives of the same kind. Until promoted they
live, genericity- and synthesis-confirmed, in `HAPI/.RnD/rosCompose/`
(`elasticBuffers.h`, `hls/`).

## Per-surface reduction

| ROS surface | reduces to | notes |
|---|---|---|
| Topic pub/sub | `Chain` fan-out fold + `Cap` | rclcpp's own seam is asymmetric: the publish path has **no** C++ vtable (it bottoms out at the `rcl_publish` C ABI); only the receive path is virtual-dispatched (`SubscriptionBase::handle_message` + `AnySubscriptionCallback`, a `std::variant<std::function…>` + `std::visit`) |
| Service (req/resp) | `Cap` reused twice + `PendingTable` | both `ClientBase` and `ServiceBase` carry the receive-side virtual triad — a service is doubly-Subscription-shaped, not symmetric. The `pending_requests_` correlation map is component-internal (it is on `Client<ServiceT>`, not `ClientBase`) — an ordinary bounded table, not a new primitive |
| Action | 3 services + 2 topics + a bounded goal table + `transition()` | `rcl_action_client_impl_t` is literally `{goal_client, cancel_client, result_client, feedback_subscription, status_subscription}`. `transition(state,event)` is a pure function that constant-folds to a branch cascade |
| QoS — history depth | `WithHistory` decorator (KEEP_LAST ring) | rclcpp's depth is a `rmw_qos_profile_t` field handed across the C ABI to the RMW/DDS layer — there is no C++ mechanism there to reduce; this is the local equivalent a DDS-less target needs |
| QoS — deadline | `WithDeadline` decorator over `hw::Timeout<Ms>` | likewise delegated to RMW/DDS; the timer primitive already existed |
| QoS — reliability, **unordered** | ack = a service response with the payload dropped; retry resend goes through the **unmodified** `Cap` | the transport seam needs no new method for retransmission — a resend is byte-identical to a first send |
| QoS — reliability, **ordered** | `ReorderBuffer` (new) | the one thing the transport primitive cannot do — a receiver-side hold-back buffer |
| Node | `Chain` / `StaticList` of compile-time-known endpoints | `rclcpp::Node` is 11 fixed sub-managers; the open runtime collection lives in `CallbackGroup`, which the executor iterates — **desktop-only, see scope boundary** |
| Parameters (typed) | `oneData::Data<T>` + a veto-fold validator | the real `on_set` callback chain is first-reject-wins; the runtime `map<string, variant>` is component-internal desktop-ROS |
| TF (frame graph) | `TimeBuffer` (new) per frame pair; the graph traversal on top is chained lookups | not built — its own exercise |

## The same composition, as an FPGA datapath

HAPI's cost is paid at compile time, so the same composition that becomes
dispatch-free firmware also synthesizes to hardware. Under Bambu HLS
(from-source `dev/panda`, `xc7z020`, 10 ns clock):

| top | states | area | DSP | ~MHz |
|---|---|---|---|---|
| the `/cmd_vel` fan-out fold | 2 | 3850 | 0 | 161 |
| the action `transition()` state machine | 5 | 184 | 0 | 148 |
| `ReorderBuffer<int32,8>::offer` | 15 | 5538 | 0 | 100 |
| `TimeBuffer<int32,8,uint32>::lookup` (raw pair) | 27 | 2808 | 0 | 101 |

The compile-time composition collapses to near-combinational logic; the
two buffers map to BRAM with no DSPs. The one part that does **not**
synthesize is the virtual `Cap<Msg>` transport seam — and it should not:
on an FPGA that boundary is an AXI-Stream port, which is exactly what
`OneHLS/examples/hls_streaming_buffers/src/axis.h` already provides. The
causal-boundary primitive spans all three target classes — the seam is a
vtable on an MCU, an AXI port on an FPGA, a kernel-launch boundary on a
GPU — the same shape as HAPI's ML composition work
([`docs/INDUSTRY.md`](INDUSTRY.md), "MCU function / FPGA datapath / CUDA
kernel").

## Scope boundary

The reduction is for **single-loop embedded** targets — one process, one
spin loop, no DDS vendor library. One part of ROS explicitly does **not**
survive that reduction and is not modelled: **`CallbackGroup` / executor
scheduling** — mutually-exclusive vs. reentrant callback groups,
multi-threaded executors, concurrent callback dispatch. On a single spin
loop everything is already serialized; that machinery exists for a
deployment model (multi-threaded executors, plugin-loaded nodes,
config-created endpoints) that the embedded target class does not have.
Anyone reading "ROS reduces to HAPI primitives" should read it as "the
composition surface reduces," not "full ROS runtime coverage."

## Method note

Two assumptions written into the initial reduction plan were **overturned
by reading real source**, not confirmed: the QoS-depth backpressure
components named in the plan (`Ctrl<A>`, `oneOutput::Gate`) do not exist
in the codebase; and `tf2::TimeCache`'s lookup is a linear scan over a
`std::list`, not the binary search the plan assumed. Both were claims
about *existing code*. Reasoning about what the *target* needs (the node
endpoint set, the parameter model) held every time. The practical
take-away for this kind of scouting: verify claims about existing code by
reading it; a familiar name is not evidence of a mechanism.
