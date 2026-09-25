// expect: [od-rule9] only the right fold
struct T {};
template<typename... PP> using F = (T : ... : PP);
