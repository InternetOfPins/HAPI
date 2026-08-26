// HYPOTHETICAL third generated artifact -- what AppCMakeGenerator.cpp's
// existing loop (register_app_modules, app_generated.cpp) could emit for
// free: same manifest.appModules walk, same module classes, same order,
// one more line. Hand-written here for the PoC -- vix's real generator
// is untouched; this file demonstrates what it would produce, not what
// it does today.
#pragma once

#include <hapi/hapi.h>

#include <auth/AuthModule.hpp>
#include <db/DbModule.hpp>

namespace vix::app_generated
{
  using AppModuleChain = hapi::Chain<
      myapp::auth::AuthModule,
      myapp::db::DbModule>;
}
