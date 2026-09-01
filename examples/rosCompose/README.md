# rosCompose

Can a ROS-shaped node — topic pub/sub, a service, an action, with QoS —
be assembled from HAPI primitives, with the composition adding **no
vtable, no heap, and no indirect dispatch of its own**? The one place an
indirect call is irreducible is the transport boundary itself (the
network hop with no lookahead), and it stays a single hop per message.

This is the promoted, buildable result of a local scouting exercise that
mapped ROS 2's composition surface (`rclcpp` / `rcl` / `rcl_action` /
`tf2`, read from source) onto HAPI's primitive set. The full round-by-
round record and the reduction argument are in
[`docs/rosCompose.md`](../../docs/rosCompose.md); this directory is the
clean demonstration.

## The primitives (`include/rosCompose/`)

| header | what it is | ROS analog |
|---|---|---|
| `transport.h` | additive fan-out fold + the `Cap<Msg>` virtual boundary seam + `PubLink` | topic pub/sub |
| `service.h` | `Cap` reused twice (one hop per direction) + a fixed-capacity `PendingTable` correlating requests by token | service (req/resp) |
| `action.h` | 3 services + 2 topics + a fixed goal array + a `constexpr` `transition(state,event)` | action |
| `qos.h` | `WithHistory` (KEEP_LAST depth) / `WithDeadline` (missed-publish watchdog) as decorators over a subscriber Body | QoS |

Nothing here is a new composition mechanism — `transport.h`'s fold is a
`hapi::Chain`, `qos.h`'s decorators wrap the *unchanged*
`Subscriber<Msg,Body>`, and the tables are the same no-heap fixed-
capacity idiom used across the ecosystem (`InList<N>`, `PendingTable`).

## `node_avr` — one composed node

```cpp
struct Node {
  CmdVelSubs                              cmd_vel;   // /cmd_vel  (two local subscribers)
  PubLink<Twist>                          odom_pub;  // /odom     (publisher)
  Client<Goal, int, 2>                    doubler_cli;
  Service<Goal, int>                      doubler_srv;
  ActionServer<Goal, Result, Feedback, 4> drive;     // an action
};
```

`spin_once()` fans `/cmd_vel` out to both subscribers, publishes `/odom`,
runs one `doubler` service round trip, and drives one goal through
`accept → Execute → feedback → Succeed` (status + feedback on their
topics). Transport `Cap`s are wired loopback here.

```
Flash: 1412 bytes
RAM:   69 bytes .data + 10 bytes .bss
```

`avr-g++ -Os`, `atmega328p`. **0 `malloc`, 0 soft-division.** Every
indirect `icall` is either a `Cap::deliver` at the transport boundary
(one `/odom` publish, one service request + one reply, one feedback, one
status update per `advance()`) or a user-supplied callback (the service
handler, the response continuation) — the same indirection a ROS
callback carries. None is composition machinery: the fan-out fold, the
correlation-table slot lookup, the goal table and `transition()` are all
straight-line. `sizeof(CmdVelSubs) == 1` (two stateless subscribers
EBO-fold); `!__is_polymorphic` on the composed types.

## `qos_decorators` — QoS without touching the composition

```cpp
using OdomWithQos = WithDeadline<Twist, 20, WithHistory<Twist, 8, OdomBody>>;
using CmdVelSub   = LocalFanout<Twist, Subscriber<Twist, OdomWithQos>>;
```

The same `/cmd_vel` subscriber now retains a depth-8 history and fires a
watchdog if no message arrives within 20 ms — folded in by wrapping the
Body, with **zero change** to `transport.h` or the fan-out.

```
Flash: 550 bytes    (0 malloc, 0 soft-division)
```

The ring wraps with a conditional subtract, not `% N` (which lowers to a
soft-division call on AVR).

## Build

```sh
pio run -e node_avr          # atmega328p — the composed node
pio run -e qos_decorators    # atmega328p — QoS decorators
./verify.sh                  # plain g++ + avr-g++, no PlatformIO
```

## Scope boundary

This targets **single-loop embedded** — one process, one spin loop, no
DDS. Under that reduction:

- The transport `Cap` is a plain pointer swapped for `rcl_publish` / a
  socket / a CAN mailbox at deployment. DDS discovery is out of scope
  (it happens on the far side of the seam).
- **`CallbackGroup` / executor scheduling is out of scope** —
  mutually-exclusive vs. reentrant callback groups, multi-threaded
  executors. On a single spin loop everything is already serialized;
  that machinery is a desktop-ROS concern and does not survive the
  DDS-less reduction. If you need it, this is not the layer for it.
- QoS reliability's *ordered* delivery and `tf2`'s time-interpolated
  lookup each need a stateful buffer beyond what's shown here
  (`ReorderBuffer` / `TimeBuffer`) — see `docs/rosCompose.md`.

## What's not verified

No real transport is wired (loopback only — this proves the types
compose and the generated code is dispatch-free, not that a real DDS /
socket / CAN round trip behaves correctly). No hardware attached.
