#pragma once

template<bool chk,typename T=void> using When=typename std::enable_if<chk,T>::type;

