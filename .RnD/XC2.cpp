#include <stdint.h>

// --- Physical Hardware Layer (FOSS Target Definitions) ---
struct XC2C64A_Silicon { uint8_t led_reg; };

// Static global instance representing the raw hardware mapping (Fixed link-time address)
extern XC2C64A_Silicon board;

// Shared bus abstraction holding register states
struct PipelineState {
  uint8_t val;
  uint8_t dir;
};

// --- Combinatorial Logic Modules (Pure Static Behavior) ---
struct RotateLeft {
  template<typename NextNode>
  struct Part : NextNode {
    void tick() {
      if (NextNode::state.dir == 1) {
        NextNode::state.val = ((NextNode::state.val & 0x07) << 1) | ((NextNode::state.val & 0x08) >> 3);
      }
      NextNode::tick();
    }
  };
};

struct RotateRight {
  template<typename NextNode>
  struct Part : NextNode {
    void tick() {
      if (NextNode::state.dir == 0) {
        NextNode::state.val = ((NextNode::state.val & 0x01) << 3) | ((NextNode::state.val & 0x0E) >> 1);
      }
      NextNode::tick();
    }
  };
};

// --- Static Architecture Target (Terminal Sink) ---
template<typename SiliconType, SiliconType* SiliconAddress>
struct CoolRunner2_Architecture_Target {
  PipelineState state; // Absolute minimum footprint: exactly 2 bytes in memory

  void tick() {
    // Zero-cost: direct register write to the absolute address injected at compile time
    SiliconAddress->led_reg = state.val; 
  }
};

// --- FOSS Metaprogramming Composition Engine ---
template<typename Module, typename Next>
struct Chain : public Module::template Part<Next> {};

// --- Strictly Static Compilation Pipeline ---
// The hardware address (&board) is now deeply embedded within the type signature itself.
using MyCoolRunnerPipeline = Chain<RotateLeft, Chain<RotateRight, CoolRunner2_Architecture_Target<XC2C64A_Silicon, &board>>>;

MyCoolRunnerPipeline pipeline;

void on_clock_tick(uint8_t direction) {
  // Read baseline state directly from the static hardware address
  pipeline.state.val = board.led_reg;
  pipeline.state.dir = direction;
  
  // Fire the pipeline execution tree
  pipeline.tick();
}