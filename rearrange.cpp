#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using std::string;
using std::vector;

// ------- safe wrappers for cctype on Clang -------
static inline bool is_space(char c) { return std::isspace(static_cast<unsigned char>(c)) != 0; }
static inline bool is_digit(char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; }


// -------- packed key (short, short, short) for (i,j,k) ----------
struct Triple {
    uint16_t i, j, k;
    bool operator==(const Triple& o) const noexcept {
        return i == o.i && j == o.j && k == o.k;
    }
};
struct TripleHash {
    std::size_t operator()(const Triple& t) const noexcept {
        return (std::size_t(t.i) << 32) ^ (std::size_t(t.j) << 16) ^ std::size_t(t.k);
    }
};

struct Target { Triple tr; bool inverted; };
std::unordered_map<Triple, Target, TripleHash> cycle_to_target;



// ---------- formula tokenization ----------
struct Tok {
    char var;       // 'p' or 'q'
    char face;      // R L F B U D
    string suf;     // "", "2", or "'"
};
struct FormulaInfo {
    string general;
    // parts: sequence of (is_token, literal_text). If is_token==true, literal is ignored.
    vector<std::pair<bool, string>> parts;
    vector<Tok> toks;
    int pCount = 0;
    int qCount = 0;
    char type = 0;  // 'p' or 'q' (unique) or 0 if ambiguous
};

static inline bool is_face(char c) {
    return c=='R'||c=='L'||c=='F'||c=='B'||c=='U'||c=='D';
}

static inline void fix_commas(std::string& s) {
    for (size_t pos = 0; pos + 1 < s.size(); ++pos) {
        if (s[pos] == ',' && s[pos+1] != ' ')
            s.insert(pos+1, " ");
    }
}

static FormulaInfo tokenize_general(const string& s) {
    FormulaInfo info;
    info.general = s;

    const std::size_t n = s.size();
    std::size_t i = 0;
    while (i < n) {
        // find next '('
        std::size_t open = s.find('(', i);
        if (open == string::npos) {
            info.parts.emplace_back(false, s.substr(i));
            break;
        }
        if (open > i) info.parts.emplace_back(false, s.substr(i, open - i));

        // expect: "(" <digits> "-" (p|q) ")" <face> [2|']
        std::size_t j = open + 1;
        std::size_t j0 = j;
        while (j < n && is_digit(s[j])) j++;
        if (j == j0 || j >= n || s[j] != '-') { // not a token
            info.parts.emplace_back(false, string(1, s[open]));
            i = open + 1;
            continue;
        }
        j++; // past '-'
        if (j >= n || (s[j] != 'p' && s[j] != 'q')) {
            info.parts.emplace_back(false, string(1, s[open]));
            i = open + 1;
            continue;
        }
        char var = s[j++];
        if (j >= n || s[j] != ')') {
            info.parts.emplace_back(false, string(1, s[open]));
            i = open + 1;
            continue;
        }
        j++; // past ')'
        if (j >= n || !is_face(s[j])) {
            info.parts.emplace_back(false, string(1, s[open]));
            i = open + 1;
            continue;
        }
        char face = s[j++];
        string suf;
        if (j < n && (s[j] == '2' || s[j] == '\'')) suf.push_back(s[j++]);

        // success: token
        info.parts.emplace_back(true, string());
        info.toks.push_back(Tok{var, face, suf});
        if (var=='p') info.pCount++; else info.qCount++;
        i = j;
    }

    if (info.pCount == 1 && info.qCount != 1) info.type = 'p';
    else if (info.qCount == 1 && info.pCount != 1) info.type = 'q';
    else if (info.pCount == 1 && info.qCount == 1) info.type = 'p'; // tie-breaker like Python
    else info.type = 0;

    return info;
}

static inline string render_token_once(const Tok& t, int M, int a, int b) {
    int v = (t.var=='p') ? (M + 1 - a) : (M + 1 - b);
    string out = std::to_string(v);
    out.push_back(t.face);
    out += t.suf;
    return out;
}

static inline string render_token_collect(const Tok& t, int M, char unique, int fixed, const vector<int>& many) {
    // If token depends on the fixed variable, emit single; else emit vectorized
    if ((unique=='p' && t.var=='q') || (unique=='q' && t.var=='p')) {
        int v = M + 1 - fixed;
        string out = std::to_string(v);
        out.push_back(t.face);
        out += t.suf;
        return out;
    }
    string out;
    bool first = true;
    for (int val : many) {
        if (!first) out.push_back(' ');
        first = false;
        int v = M + 1 - val;
        out += std::to_string(v);
        out.push_back(t.face);
        out += t.suf;
    }
    return out;
}

static string render_once(const FormulaInfo& fi, int M, int a, int b) {
    string out;
    std::size_t tok_idx = 0;
    for (const auto& pr : fi.parts) {
        if (!pr.first) out += pr.second;
        else           out += render_token_once(fi.toks[tok_idx++], M, a, b);
    }
    return out;
}

static string render_collect(const FormulaInfo& fi, int M, char unique,
                             int fixed, const vector<int>& many) {
    string out;
    std::size_t tok_idx = 0;
    for (const auto& pr : fi.parts) {
        if (!pr.first) out += pr.second;
        else           out += render_token_collect(fi.toks[tok_idx++], M, unique, fixed, many);
    }
    return out;
}

// Parse table line: i , j , k  algorithm: ... general: <GENERAL>
static bool parse_table_line(const string& line, int& i, int& j, int& k, string& general) {
    i = j = k = -1;
    std::size_t p = 0, n = line.size();

    auto skip_ws_and_commas = [&]() {
        while (p < n && (is_space(line[p]) || line[p]==',')) ++p;
    };
    auto read_int = [&](int& out)->bool{
        while (p < n && is_space(line[p])) ++p;
        std::size_t start = p;
        while (p < n && is_digit(line[p])) ++p;
        if (start == p) return false;
        out = std::stoi(line.substr(start, p - start));
        return true;
    };

    if (!read_int(i)) return false;
    skip_ws_and_commas();
    if (!read_int(j)) return false;
    skip_ws_and_commas();
    if (!read_int(k)) return false;

    auto pos = line.find("general:", p);
    if (pos == string::npos) return false;
    pos += 8;
    std::size_t q = pos;
    while (q < n && is_space(line[q])) ++q;
    general = line.substr(q);
    return true;
}

static inline bool find_formula_with_permutations(
    const std::unordered_map<Triple, FormulaInfo, TripleHash>& table,
    uint16_t i, uint16_t j, uint16_t k,
    Triple& chosen, bool& inverted
) {
    // direct (cyclic rotations)
    Triple direct[3] = { {i,j,k}, {j,k,i}, {k,i,j} };
    for (auto &t : direct) {
        auto it = table.find(t);
        if (it != table.end()) { chosen = t; inverted = false; return true; }
    }
    // inverted orientation (reverse cycle)
    Triple inv[3] = { {i,k,j}, {k,j,i}, {j,i,k} };
    for (auto &t : inv) {
        auto it = table.find(t);
        if (it != table.end()) { chosen = t; inverted = true; return true; }
    }
    return false;
}
static string swap_commutator_contents(const string& s) {
    string out; out.reserve(s.size());
    for (std::size_t p = 0; p < s.size(); ) {
        if (s[p] != '[') { out.push_back(s[p++]); continue; }
        std::size_t q = s.find(']', p+1);
        if (q == string::npos) { out.push_back(s[p++]); continue; }
        // inside is s.substr(p+1, q-(p+1)) -> split once on ','
        string inner = s.substr(p+1, q-(p+1));
        std::size_t comma = inner.find(',');
        if (comma == string::npos) {
            out.append(s, p, q-p+1); // keep as-is
            p = q+1; continue;
        }
        string A = inner.substr(0, comma);
        string B = inner.substr(comma+1);
        // trim simple spaces
        auto trim = [](string& t){ 
            std::size_t a=0,b=t.size();
            while (a<b && std::isspace((unsigned char)t[a])) a++;
            while (b>a && std::isspace((unsigned char)t[b-1])) b--;
            t = t.substr(a,b-a);
        };
        trim(A); trim(B);
        out.push_back('[');
        out += B; out += ", "; out += A;
        out.push_back(']');
        p = q+1;
    }
    return out;
}


// Buckets per (i,j,k)
struct BucketGroup {
    uint16_t i,j,k;
    char type; // 'p' or 'q' (skip 0/ambiguous to match Python)
    // direct orientation (Python swaps)
    std::unordered_map<int, std::vector<int>> buckets_direct;
    // inverted orientation (Python does NOT swap)
    std::unordered_map<int, std::vector<int>> buckets_inverted;
};

// Parse cycles line: "orbit(a,b) - rot(i,j,k)"
static bool parse_cycles_line(const string& s, int& a, int& b, int& i, int& j, int& k) {
    std::size_t p = s.find("orbit(");
    if (p == string::npos) return false;
    p += 6;

    auto rd = [&](int& out)->bool{
        while (p < s.size() && is_space(s[p])) ++p;
        std::size_t st = p;
        while (p < s.size() && is_digit(s[p])) ++p;
        if (st==p) return false;
        out = std::stoi(s.substr(st, p-st));
        return true;
    };

    if (!rd(a)) return false;
    if (p >= s.size() || s[p] != ',') return false; ++p;
    if (!rd(b)) return false;

    std::size_t close = s.find(')', p);
    if (close == string::npos) return false;

    p = s.find("rot(", close);
    if (p == string::npos) return false;
    p += 4;

    if (!rd(i)) return false;
    if (p >= s.size() || s[p] != ',') return false; ++p;
    if (!rd(j)) return false;
    if (p >= s.size() || s[p] != ',') return false; ++p;
    if (!rd(k)) return false;

    return true;
}
int main(int argc, char** argv) {
    // Defaults (kept for convenience)
    std::string cycles_path = "cycles_wave1.txt";
    std::string table_path  = "table_with_formula.txt";
    std::string out_path    = "wave_1_parallel.txt";
    int M = 800;

    // Very small flag parser
    auto need = [&](bool ok, const char* what){
        if (!ok) {
            std::cerr << "Missing value for " << what << "\n";
            std::exit(2);
        }
    };
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--cycles")      { need(i+1<argc, "--cycles"); cycles_path = argv[++i]; }
        else if (a == "--table")  { need(i+1<argc, "--table");  table_path  = argv[++i]; }
        else if (a == "--out")    { need(i+1<argc, "--out");    out_path    = argv[++i]; }
        else if (a == "--M")      { need(i+1<argc, "--M");      M = std::stoi(argv[++i]); }
        else if (a == "-h" || a == "--help") {
            std::cout <<
R"(Usage: rearrange --cycles <cycles_waveX.txt> --table <table_with_formula.txt> --out <wave_X_parallel.txt> --M <size>
Defaults:
  --cycles cycles_wave1.txt
  --table  table_with_formula.txt
  --out    wave_1_parallel.txt
  --M      800
)";
            return 0;
        } else {
            std::cerr << "Unknown flag: " << a << "\n";
            return 2;
        }
    }

    // ---------- (unchanged code below this line) ----------
    // 1) Load table and tokenize
    std::unordered_map<Triple, FormulaInfo, TripleHash> table;
    {
        std::ifstream in(table_path);
        if (!in) {
            std::cerr << "Failed to open " << table_path << "\n";
            return 1;
        }
        string line; int i,j,k; string gen;
        std::size_t count = 0;
        while (std::getline(in, line)) {
            if (!parse_table_line(line, i, j, k, gen)) continue;
            FormulaInfo fi = tokenize_general(gen);
            Triple t{ static_cast<uint16_t>(i), static_cast<uint16_t>(j), static_cast<uint16_t>(k) };
            table.emplace(t, std::move(fi));
            ++count;
        }
        std::cerr << "Loaded " << count << " general formulas from " << table_path << "\n";
    }

    // 2) Prepare bucket groups for cycles (skip ambiguous)
    std::unordered_map<Triple, BucketGroup, TripleHash> groups;
    groups.reserve(table.size()*2);
    for (const auto& kv : table) {
        const Triple& t = kv.first;
        const FormulaInfo& fi = kv.second;
        if (fi.type == 0) continue; // skip ambiguous
        BucketGroup g; g.i=t.i; g.j=t.j; g.k=t.k; g.type=fi.type;
        groups.emplace(t, std::move(g));
    }

    // 3) Read cycles file and bucket into direct/inverted
    {
        std::ifstream in(cycles_path);
        if (!in) {
            std::cerr << "Failed to open " << cycles_path << "\n";
            return 1;
        }
        string line; int a,b,i,j,k;
        std::size_t fed = 0, skipped = 0;
        while (std::getline(in, line)) {
            if (!parse_cycles_line(line, a, b, i, j, k)) { skipped++; continue; }

            // filter out the diag centers, edges and corners
            if (a == b || a == M || b == M) {
                //skipped++;
                continue;
            }

            Triple req{ (uint16_t)i,(uint16_t)j,(uint16_t)k };
            Triple chosen; bool inverted=false;
            if (!find_formula_with_permutations(table, req.i, req.j, req.k, chosen, inverted)) {
                skipped++;
                continue;
            }

            auto gi = groups.find(chosen);
            if (gi == groups.end()) {
                const FormulaInfo& tfi = table.at(chosen);
                if (tfi.type == 0) { skipped++; continue; } // ambiguous → skip
                BucketGroup g; g.i=chosen.i; g.j=chosen.j; g.k=chosen.k; g.type=tfi.type;
                gi = groups.emplace(chosen, std::move(g)).first;
            }
            BucketGroup& g = gi->second;

            auto& buckets = inverted ? g.buckets_inverted : g.buckets_direct;

            if (g.type == 'p') buckets[b].push_back(a);  // group by q=b, collect p=a
            else               buckets[a].push_back(b);  // group by p=a, collect q=b

            fed++;
        }
        std::cerr << "Bucketed " << fed << " pairs (" << skipped << " skipped)\n";
    }

    // 4) Emit parallelized lines
    std::ofstream out(out_path);
    if (!out) {
        std::cerr << "Failed to create " << out_path << "\n";
        return 1;
    }

    vector<Triple> order;
    order.reserve(groups.size());
    for (const auto& kv : groups) order.push_back(kv.first);
    std::sort(order.begin(), order.end(), [](const Triple& a, const Triple& b){
        if (a.i!=b.i) return a.i<b.i;
        if (a.j!=b.j) return a.j<b.j;
        return a.k<b.k;
    });

    for (const Triple& t : order) {
        const auto itT = table.find(t);
        if (itT == table.end()) continue;
        const FormulaInfo& fi = itT->second;

        const auto itG = groups.find(t);
        const BucketGroup& g = itG->second;

        std::string swapped = swap_commutator_contents(fi.general);
        FormulaInfo fi_swapped = tokenize_general(swapped);

        auto emit_bucket_map = [&](const std::unordered_map<int, std::vector<int>>& map,
                                   const FormulaInfo& use_fi, char unique_tag) {
            std::vector<int> keys;
            keys.reserve(map.size());
            for (const auto& kv : map) keys.push_back(kv.first);
            std::sort(keys.begin(), keys.end());

            for (int key : keys) {
                auto vs = map.at(key);
                std::sort(vs.begin(), vs.end());
                vs.erase(std::unique(vs.begin(), vs.end()), vs.end());

                std::string expanded =
                    (unique_tag == 'p')
                        ? render_collect(use_fi, M, 'p', key, vs)
                        : render_collect(use_fi, M, 'q', key, vs);

                fix_commas(expanded);
                out << expanded << "\n";
            }
        };

        if (g.type == 'p') {
            emit_bucket_map(g.buckets_direct,   fi_swapped, 'p'); // direct → swapped
            emit_bucket_map(g.buckets_inverted, fi,         'p'); // inverted → original
        } else { // 'q'
            emit_bucket_map(g.buckets_direct,   fi_swapped, 'q');
            emit_bucket_map(g.buckets_inverted, fi,         'q');
        }
    }

    std::cerr << "Done. Wrote " << out_path << "\n";
    return 0;
}

