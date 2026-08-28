// Negative test: this file is EXPECTED TO FAIL TO COMPILE.
//
// The naive way to "compose FK and IK-velocity with HAPI" is a flat
// hapi::Chain<KDL::ChainFkSolverPos, KDL::ChainIkSolverVel>, which layers both
// KDL abstract bases as base classes. Two problems, neither diagnosed on its
// own by plain C++:
//   1. diamond on KDL::SolverI — ambiguous getError()/strError(), doubled
//      `protected int error`;
//   2. both bases declare `virtual void updateInternalDataStructures() = 0`,
//      so one silently hides the other under ordinary name lookup.
//
// kdlAPI.h's working shape avoids this by holding each solver as stage-local
// member state instead (kdlCompose::FkStage / IkVelStage). This file shows
// HAPI core's NoCollision diagnostic (rules.h) catches problem 2 as a compile
// error at the composition site. Kept as its own file because kdlCompose.cpp
// asserts a CLEAN build.
//
// Expected: static_assert failure naming HapiMember_updateInternalDataStructures
// and the two colliding types (see the MemberCollision<Detector,A,B>
// instantiation in the diagnostic).

#include <kdl/chainfksolver.hpp>
#include <kdl/chainiksolver.hpp>

#include <hapi/chain.h>
#include <hapi/rules.h>

HAPI_DETECT_MEMBER(updateInternalDataStructures);

static_assert(
    hapi::NoCollision<HapiMember_updateInternalDataStructures,
                      hapi::Chain<KDL::ChainFkSolverPos, KDL::ChainIkSolverVel>>,
    "expected to fail: flat Chain<ChainFkSolverPos, ChainIkSolverVel> collides "
    "on updateInternalDataStructures() — compose the solvers as stage members "
    "(kdlCompose::FkStage / IkVelStage) instead");

int main() {}
