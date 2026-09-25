// expect: [od-dup] duplicate layer: 'Self' occurs twice in the composition
// amendment: the same layer twice in one chain (exact type match)
struct Term {};
struct Self {int get() const {return super::get();}};
struct Y : Self:Self:Term {};
