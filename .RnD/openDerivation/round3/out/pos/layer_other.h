#pragma once
// a closed class defined in another file: layer_other.h and closed_layer.cpp are translated in one batch, so the class gets
// its Part here because closed_layer.cpp uses it as a layer
struct Mix {int m=11; template<typename O> struct Part:O {using Base=O; using Base::Base; int m=11;}; };
