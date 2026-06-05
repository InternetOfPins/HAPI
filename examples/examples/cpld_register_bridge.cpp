#pragma once
#include <hapi/hapi.h>

// user state --
struct PipelineState {
  uint8_t val;
  uint8_t dir;
};

// modules --
struct RotateLeft {
  template<typename O>
  struct Part : O {
    using Base = O;
    void tick() {
      if (Base::state.dir == 1)
        Base::state.val
          = (((Base::state.val & 0x07) << 1) 
          | ((Base::state.val & 0x08) >> 3)) & 0x0F;
      Base::tick();
    }
  };
};

struct RotateRight {
  template<typename O>
  struct Part : O {
    using Base = O;
    void tick() {
      if (Base::state.dir == 0) 
        Base::state.val 
          = (((Base::state.val & 0x01) << 3) 
          | ((Base::state.val & 0x0E) >> 1)) & 0x0F;
      Base::tick();
    }
  };
};

// hardware target --
struct CoolRunnerII_Family {uint8_t led_reg;};

template<typename Family, Family& silicon, typename State = PipelineState>
struct HardwareTarget {
  State state;
  void tick() {silicon.led_reg = state.val;}
};

// hardware instance --
extern CoolRunnerII_Family board;

// pipeline --
using MyPipeline = OutDef<
  RotateLeft,
  RotateRight,
  HardwareTarget<CoolRunnerII_Family, board>
>;