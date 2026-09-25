#pragma once
// od_fold.h -- the pack fold for translate.py --lower=nested.
// FoldT<T,P1,...,Pn> is P1::Part<...Pn::Part<T>...> itself (an alias, no wrapper struct), so a fold names the SAME type as
// the written-out chain: FoldT<C,A,B> == A::Part<B::Part<C>> (rule 3). hapi::Chain<A,B>::Part<C> is a struct deriving from
// that type, so it is a different type; that is what --lower=chain keeps (HAPI's current form) and --lower=nested fixes.
namespace od {
  template<typename T,typename... PP> struct Fold {using type=T;};
  template<typename T,typename P,typename... PP> struct Fold<T,P,PP...> {using type=typename P::template Part<typename Fold<T,PP...>::type>;};
  template<typename T,typename... PP> using FoldT=typename Fold<T,PP...>::type;
}
