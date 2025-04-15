#pragma once

#include "hapi/canPrint.h"

namespace hapi {
    template<typename T,T data>
  struct StaticData {
    template<typename O>
    struct Part:O {
      template<typename... OO> constexpr Part(OO... oo):O{oo...}{}
      static constexpr const T get() {return data;}
      template<typename Out> Out& operator<<(Out& out) const {::operator<<(out,data);O::operator<<(out);return out;}
    };
  };

  template<char c> using StaticChar=StaticData<char,c>;
  template<int n> using StaticInt=StaticData<int,n>;
  template<const char*& text> using StaticText=StaticData<const char*&,text>;

  template<typename T,T val>
  struct DefaultValue {
    template<typename O>
    struct Part:O {
      using O::O;
      template<typename... OO>
      constexpr Part(T&&o,OO&&...oo):O(forward<T>(o),forward<OO>(oo)...){}
      template<typename... OO>
      constexpr Part(OO&&...oo):O(val,forward<OO>(oo)...){}
      static constexpr T defaultValue() {return val;}
    };
  };

  /// @brief keep track of value changes (by storing a copy to compare)
  struct ReflexOf {
    template<typename O>
    struct Part:O {
      using Base=O;
      using This=Part<O>;
      using Type=typename remove_reference<typename Base::DataType>::type;
      // using Base::Base;
      Type reflex{};
      Part(){sync();}
      template<typename... OO>
      Part(OO... oo):Base(oo...){sync();}
      /// @brief check watched data for changes
      /// @return bool
      bool changed() const {return reflex!=Base::get();}
      /// @brief match stored value with the current watched value
      void sync() {reflex=Base::get();}
      /// @brief get stored value
      /// @return stored value
      Type last() {return reflex;}
      /// @brief sync on set
      /// @param o new value
      void set(Type o) {sync();Base::set(o);}
    };
  };

  template<typename T>
  struct Data {
    using Watch=Parts<ReflexOf,Data<T>>;
    template<typename O>
    struct Part:O {
      using DataType=T;
      T data{};
      constexpr Part(){}
      template<typename... OO>
      constexpr Part(T t,OO... oo):O(oo...),data(t){}
      constexpr const T get() const {return data;}
      void set(const T o) {data=o;}
      template<typename Out> Out& operator<<(Out& out) const {out<<data;out<<*reinterpret_cast<const O*>(this);return out;}
    };
  };

  using Char=Data<unsigned char>;
  using Int=Data<int>;
  using Float=Data<double>;
  using Text=Data<const char*>;

  template<typename F>
  struct FieldValue {
    template<typename O> struct Part:Parts<F,O>{
      using Base=Parts<F,O>;
      using T=typename Base::DataType;
      using Base::Base;
      T getValue() const {return Base::get();}
      void setValue(T o) {Base::set(o);}
    };
  };

};