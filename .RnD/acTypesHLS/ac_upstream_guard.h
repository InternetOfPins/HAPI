// Compile-time guard confirming the real upstream HLSLibs AC Datatypes
// (github.com/hlslibs/ac_types, AC_VERSION 4) resolved, not Bambu's own
// bundled fork (Mentor Graphics 3.7.2, 2017, AC_VERSION 3) that sits on
// Bambu's default include search path. Include this AFTER <ac_int.h> or
// <ac_fixed.h> in any file whose whole point is testing the real header --
// a wrong -I silently resolving to Bambu's fork now fails the build
// instead of passing quietly with different resource numbers.
#ifndef AC_VERSION
#error "ac_upstream_guard.h included before <ac_int.h>/<ac_fixed.h>"
#endif
#if AC_VERSION < 4
#error "resolved to bambu's bundled ac_types fork (AC_VERSION < 4) -- not the real upstream github.com/hlslibs/ac_types header. Check your -I search path."
#endif
