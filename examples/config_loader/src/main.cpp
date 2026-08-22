/**
 * @file main.cpp
 * @brief Cross-library demonstrator: HAPI + OneData + OneParse, no embedded
 *        target at all — a small CLI config loader/validator.
 *
 * OneParse parses "key = value" lines structurally (no field-name
 * knowledge). A HAPI Chain<> of per-field validator layers, each owning its
 * own OneData storage, does the semantic work: required/range checks and
 * change-tracking across a reload. This is the family's first example that
 * combines all three libraries and never targets hardware.
 */

#include <fstream>
#include <iostream>
#include <sstream>
using namespace std;

#include <hapi/hapi.h>
#include <oneData/oneData.h>
#include <oneParse/oneParse.h>
using namespace oneData;
using namespace oneParse;

// ── Grammar: structural parse only, no notion of "expected" keys ──────────

static constexpr size_t kMaxFields = 16;

struct RawEntry { Arr<char,32> key{}; Arr<char,64> val{}; };
static RawEntry makeRawEntry(Pair<Arr<char,32>,Arr<char,64>> p) { return {p.fst, p.snd}; }

// key: one-or-more letters
using KeyP = ParseDef<Arr<char,32>, SomeN<ParseDef<char, Alpha>, 32>>;

// optional spaces, '=', optional spaces, then one-or-more non-space chars
using ValLineP = ParseDef<Arr<char,64>,
    Skip<Many<Space>, Char<'='>, Many<Space>>,
    SomeN<ParseDef<char, NoneOf<' ','\t','\r','\n'>>, 64>>;

// key + value line -> RawEntry
using EntryP = ParseDef<RawEntry,
    To<Pair<Arr<char,32>,Arr<char,64>>, makeRawEntry, Seq<KeyP, ValLineP>>>;

using LineSep = Some<Or<Char<'\n'>, Char<'\r'>>>;

// whole file: entries separated by one-or-more newlines
using ConfigP = ParseDef<Arr<RawEntry, kMaxFields>, SepBy1<EntryP, LineSep, kMaxFields>>;

// ── Small helpers bridging OneParse's Arr<char,N> to plain C strings/ints ──

static const RawEntry* findEntry(const Arr<RawEntry,kMaxFields>& es, const char* key) {
  size_t klen = strlen(key);
  for (size_t i = 0; i < es.len; ++i) {
    const auto& e = es.data[i];
    if (e.key.len == klen && memcmp(e.key.data, key, klen) == 0) return &e;
  }
  return nullptr;
}

static int toInt(const Arr<char,64>& v) {
  int sign = 1, n = 0, i = 0;
  if (v.len && v.data[0] == '-') { sign = -1; i = 1; }
  for (; i < (int)v.len && v.data[i] >= '0' && v.data[i] <= '9'; ++i)
    n = n * 10 + (v.data[i] - '0');
  return sign * n;
}

static void toStr(const Arr<char,64>& v, char* out, size_t cap) {
  size_t n = v.len < cap - 1 ? v.len : cap - 1;
  memcpy(out, v.data, n);
  out[n] = '\0';
}

// ── Validation chain: one HAPI Part<O> layer per field ─────────────────────

struct Report {
  ostream& out;
  int missing{0};
  int badRange{0};
};

struct NameField {
  template<typename O> struct Part : O {
    using Base = O; using Base::Base;
    oneData::DataDef<oneData::Data<char[32]>> name{};

    void validate(Report& rep, const Arr<RawEntry,kMaxFields>& es) {
      if (auto* e = findEntry(es, "name")) {
        char buf[32]; toStr(e->val, buf, sizeof buf);
        name.set(buf);
        rep.out << "  name    : ok (\"" << name.get() << "\")\n";
      } else {
        rep.out << "  name    : MISSING\n"; ++rep.missing;
      }
      Base::validate(rep, es);
    }
    // name has no Watch<> — Watch<W>::changed() compares get()!=watched,
    // and for an array type that compares pointer/decay values, not string
    // contents. A real composability limit, not papered over: no
    // sync()/reportChanges() override here, calls just fall through to
    // whatever the next layer defines.
  };
};

template<const char* Key, int Lo, int Hi>
struct RangedIntField {
  template<typename O> struct Part : O {
    using Base = O; using Base::Base;
    oneData::DataDef<oneData::Watch<oneData::Int>> field{};

    void validate(Report& rep, const Arr<RawEntry,kMaxFields>& es) {
      if (auto* e = findEntry(es, Key)) {
        field.set(toInt(e->val));
        bool ok = oneData::StaticRange<Lo,Hi>::valid(field.get());
        rep.out << "  " << Key << " : " << (ok ? "ok" : "OUT OF RANGE")
                 << " (" << field.get() << ")\n";
        if (!ok) ++rep.badRange;
      } else {
        rep.out << "  " << Key << " : MISSING\n"; ++rep.missing;
      }
      Base::validate(rep, es);
    }
    void sync() { field.sync(); Base::sync(); }
    void reportChanges(ostream& out) {
      if (field.changed()) out << "  " << Key << " changed -> " << field.get() << "\n";
      Base::reportChanges(out);
    }
  };
};

constexpr char kPort[]    = "port";
constexpr char kTimeout[] = "timeout";
constexpr char kRetries[] = "retries";

using PortField    = RangedIntField<kPort,    1, 65535>;
using TimeoutField = RangedIntField<kTimeout, 1, 3600>;
using RetriesField = RangedIntField<kRetries, 0, 10>;

struct ConfigAPI {
  static void validate(Report&, const Arr<RawEntry,kMaxFields>&) {}
  static void sync() {}
  static void reportChanges(ostream&) {}
};

using Config = hapi::APIOf<ConfigAPI, NameField, PortField, TimeoutField, RetriesField>;

// ── main: real file I/O, real bundled input, real output, real exit code ──

static Arr<RawEntry,kMaxFields> parseFile(const char* path) {
  ifstream f(path);
  if (!f) { cerr << "cannot open: " << path << "\n"; exit(1); }
  stringstream ss; ss << f.rdbuf();
  string text = ss.str();
  auto r = ConfigP::run(text.c_str());
  if (!r.ok) { cerr << "parse failed: " << path << "\n"; exit(1); }
  return r.val;
}

int main(int argc, char** argv) {
  const char* file1 = argc > 1 ? argv[1] : "data/config.txt";
  const char* file2 = argc > 2 ? argv[2] : "data/config_v2.txt";

  Config cfg;

  cout << "== loading " << file1 << " ==\n";
  Report rep1{cout};
  cfg.validate(rep1, parseFile(file1));
  cout << (rep1.missing || rep1.badRange ? "-> problems found\n" : "-> ok\n");
  cfg.sync();

  cout << "\n== reloading " << file2 << " ==\n";
  Report rep2{cout};
  cfg.validate(rep2, parseFile(file2));
  cout << (rep2.missing || rep2.badRange ? "-> problems found\n" : "-> ok\n");

  cout << "\n== changes since first load ==\n";
  cfg.reportChanges(cout);

  return (rep1.missing || rep1.badRange || rep2.missing || rep2.badRange) ? 1 : 0;
}
