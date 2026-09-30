/**
 * @file slots.h
 * @brief Slots: state composed along a chain and addressed by tag.
 *
 * hapi::Slot<Tag, S> is a component that adds one slot, a value of type S, to the state the components after it built.
 * The state is one object of static size with no heap; a slot whose S is empty adds nothing on GCC and Clang (on MSVC it cost 2 bytes in CI:
 * its layout of several empty bases differs; `__declspec(empty_bases)` was not tried).
 *
 *   using State = hapi::APIOf<hapi::SlotApi, hapi::Slot<A, SlotA>, hapi::Slot<B, SlotB>>::Res;
 *   hapi::slot<A>(state).x = 1;       // by tag; a tag that is not in the state is a compile error
 *   state.each(visitor);              // visitor.layer(Tag::name()), then S::each(slot, visitor), for every slot, the last-listed first
 *
 * The same tag twice in one state is a compile error, also across nested chains and nested APIOf.
 *
 * What the slots mean, what their fields are called, how the state evolves or travels is not decided here. Slot's third parameter,
 * a Contract, is how a user adds members to the state's type; R is the state's type and R::me() is the slot of this Part:
 *
 *   template<class Below, class R> struct Counted : Below { unsigned steps = 0; void step() { ++steps; } };
 *   hapi::Slot<A, SlotA, Counted>
 *
 * each() calls Tag::name() and S::each() only when it is used, and does not look at what Tag::name() returns.
 */
#pragma once
#include "hapi/base.h"

#ifndef HAPI_EMPTY_BASES
  #if defined(_MSC_VER) && !defined(__clang__)
    #define HAPI_EMPTY_BASES __declspec(empty_bases)   // MSVC removes an empty base from one base only, unless asked
  #else
    #define HAPI_EMPTY_BASES
  #endif
#endif
#ifndef HAPI_SLOT_EACH_INLINE
  #define HAPI_SLOT_EACH_INLINE   // e.g. [[gnu::always_inline]]: a size policy for walks over the state
#endif

namespace hapi {
  /// @brief the slot as a base class, addressed by its tag
  template<class Tag, class S> struct SlotOf : S {};
  template<class Tag, class S> constexpr S& slot_(SlotOf<Tag,S>& f) { return f; }
  template<class Tag, class S> constexpr const S& slot_(const SlotOf<Tag,S>& f) { return f; }

  template<class T> T& slot_decl();
  template<class Tag, class R, class = void> struct HasSlot : std::false_type {};
  template<class Tag, class R> struct HasSlot<Tag, R, std::void_t<decltype(slot_<Tag>(slot_decl<R>()))>> : std::true_type {};

  /// @brief the slot of Tag in a state
  template<class Tag, class R> constexpr decltype(auto) slot(R& r) {
    static_assert(HasSlot<Tag,R>::value, "hapi::slot<Tag>: no slot with that tag in this state (a Part sees its own slot and the slots of the Parts after it)");
    return slot_<Tag>(r);
  }

  /// @brief the end of the chain: no slot, nothing to walk. SlotApi is the terminal API of a state that needs nothing more.
  struct SlotRoot { template<class V> constexpr void each(V&) const {} };
  struct SlotApi { using Res = SlotRoot; };

  /// @brief what a state adds when no Contract asks for more: nothing
  template<class Below, class R> struct SlotBase : Below {};

  /// @brief a component that adds a slot. Contract<Below, R> is the base the state derives from, carrying the state below (Below) and the state's
  /// own type (R): a Contract adds members to the state without adding an inheritance level, so the state keeps the same aggregate shape.
  template<class Tag, class S, template<class, class> class Contract = SlotBase>
  struct Slot {
    template<class O> struct Part : O {
      static_assert(!HasSlot<Tag, typename O::Res>::value, "hapi::Slot: two Parts claim the same tag");
      struct HAPI_EMPTY_BASES Res : Contract<typename O::Res, Res>, SlotOf<Tag,S> {
        constexpr S& me() { return slot_<Tag>(*this); }
        constexpr const S& me() const { return slot_<Tag>(*this); }
        template<class V> HAPI_SLOT_EACH_INLINE constexpr void each(V& v) { O::Res::each(v); v.layer(Tag::name()); S::each(me(), v); }
        template<class V> HAPI_SLOT_EACH_INLINE constexpr void each(V& v) const { O::Res::each(v); v.layer(Tag::name()); S::each(me(), v); }
      };
    };
  };
}
