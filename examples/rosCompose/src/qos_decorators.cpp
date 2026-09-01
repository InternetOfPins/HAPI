// QoS as decorators: the same /cmd_vel subscriber gains a KEEP_LAST
// history and a deadline watchdog WITHOUT any change to the topic
// composition — WithHistory / WithDeadline wrap the subscriber Body
// through the unchanged Subscriber<Msg,Body> mechanism.
//
// Build:  pio run -e qos_decorators   (atmega328p)
#include <stdint.h>
#include <hapi/hapi.h>
#include "rosCompose/qos.h"

using namespace rosCompose;

struct Twist { int16_t lin, ang; };
volatile int16_t g_odom = 0;
volatile int     g_missed = 0;
static void on_missed() { ++g_missed; }

// the plain subscriber body
struct OdomBody { template<typename T> struct Part : T { using T::T;
  void on(const Twist& t) { g_odom += t.lin; } }; };

// depth-8 history + a 20 ms deadline, folded onto OdomBody
using OdomWithQos = WithDeadline<Twist, 20,
                      WithHistory<Twist, 8, OdomBody>>;
using CmdVelSub   = LocalFanout<Twist, Subscriber<Twist, OdomWithQos>>;
static CmdVelSub cmd_vel;

static_assert(!__is_polymorphic(CmdVelSub), "decorated subscriber has no vtable");

void spin(const Twist& t) { cmd_vel.deliver(t); }
void tick()                { cmd_vel.checkDeadline(); }

int main() {
  cmd_vel.onMissed = &on_missed;
  for (int i = 0; i < 12; ++i) { spin(Twist{1, 0}); tick(); }   // 12 into a depth-8 ring
  // history now holds the last 8 messages; g_odom saw all 12
  int16_t held = 0;
  for (unsigned k = 0; k < 8; ++k) held += cmd_vel.history.at(k).lin;
  return g_odom + held + g_missed;   // 12 + 8 + 0
}
