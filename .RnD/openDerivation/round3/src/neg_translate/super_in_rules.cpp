// expect: `super` inside rules() of 'B'
// rules() is asked of the holder (it stays outside Part), which has no base
struct B {
  template<typename Before,typename After> static constexpr bool rules() {return super::ok();}
  int f() const {return super::f();}
};
