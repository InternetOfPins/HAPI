// A ROS-shaped node — a topic (publisher + two local subscribers), a
// service (client + server, one round trip), and an action (goal
// lifecycle with feedback + status topics) — composed entirely from HAPI
// primitives in include/rosCompose/.
//
// Everything structural is fixed at compile time: the subscriber set is
// the type, the pending-request table and goal table are fixed arrays,
// the goal lifecycle is a constexpr switch. The ONLY indirect calls in
// the exercised path are transport `Cap` seam hops — one per message
// crossing the boundary. On this single-process build the caps are wired
// loopback; a real deployment swaps in rcl_publish / a socket / DDS.
//
// Build:  pio run -e node_avr        (atmega328p)   — see README.md
#include <stdint.h>
#include <hapi/hapi.h>
#include "rosCompose/action.h"

using namespace rosCompose;

// ---- messages ----------------------------------------------------------
struct Twist    { int16_t lin, ang; };
struct Goal     { int16_t target; };
struct Result   { int16_t value; };
struct Feedback { int16_t progress; };

volatile int16_t g_odom = 0, g_watch = 0, g_svc = 0, g_status = 0, g_fb = 0;

// ---- topic: two local subscribers, additive fold ---------------------
struct OdomBody  { template<typename T> struct Part : T { using T::T;
  void on(const Twist& t) { g_odom  += t.lin; } }; };
struct WatchBody { template<typename T> struct Part : T { using T::T;
  void on(const Twist& t) { g_watch += (t.ang > 90); } }; };

using CmdVelSubs = LocalFanout<Twist, Subscriber<Twist, OdomBody>,
                                      Subscriber<Twist, WatchBody>>;

// ---- the node -------------------------------------------------------
struct Node {
  CmdVelSubs                              cmd_vel;    // /cmd_vel subscriber side
  PubLink<Twist>                          odom_pub;   // /odom publisher side
  Client<Goal, int, 2>                    doubler_cli;
  Service<Goal, int>                      doubler_srv;
  ActionServer<Goal, Result, Feedback, 4> drive;
};
static Node node;

static_assert(!__is_polymorphic(CmdVelSubs), "topic fan-out has no vtable");
static_assert(sizeof(CmdVelSubs) == 1, "two stateless subscribers EBO-fold");

// ---- loopback transport (a real transport replaces every line here) --
struct OdomSink : Cap<Twist>    { void deliver(const Twist& t) override { g_odom -= t.lin; } };
struct FbSink   : Cap<Feedback> { void deliver(const Feedback& f) override { g_fb = f.progress; } };
struct StSink   : Cap<int>      { void deliver(const int& s) override { g_status = s; } };
static OdomSink odomSink;
static FbSink   fbSink;
static StSink   stSink;

static void on_doubled(const int& v) { g_svc = v; }

static void wire() {
  node.odom_pub.link          = &odomSink;
  node.drive.feedbackPub.link  = &fbSink;
  node.drive.statusPub.link    = &stSink;
  node.doubler_cli.linkOut     = &node.doubler_srv;  // client -> service
  node.doubler_srv.linkOut     = &node.doubler_cli;  // service reply -> client
  node.doubler_srv.handler     = [](const Goal& g) { return (int)(g.target * 2); };
}

// ---- one spin of the node ------------------------------------------
void spin_once(const Twist& cmd) {
  node.cmd_vel.deliver(cmd);                    // fan out /cmd_vel (fold, no icall)
  node.odom_pub.deliver(Twist{g_odom, 0});      // publish /odom            [cap hop]

  node.doubler_cli.call(Goal{21}, &on_doubled); // service round trip  [2 cap hops + handler]

  int id = node.drive.accept(Goal{100});        // action: accept (fixed table, no icall)
  node.drive.advance(id, GoalEvent::Execute);   // status topic             [cap hop]
  node.drive.feedback(Feedback{50});            // feedback topic           [cap hop]
  node.drive.advance(id, GoalEvent::Succeed);   // terminal, slot freed     [cap hop]
}

int main() {
  wire();
  spin_once(Twist{3, 120});
  spin_once(Twist{4, 30});
  return g_odom + g_watch + g_svc + g_status + g_fb;
}
