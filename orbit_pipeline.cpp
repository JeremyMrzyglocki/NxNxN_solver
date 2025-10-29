// orbit_pipeline.cpp
// Combines "boxer" (sector pattern builder) + "double_parallelizer" (formula expander)
// to process all run_dir/orbit_points/*.txt and write:
//   - run_dir/intermediary/<a>_<b>_<c>__sector_<id>.txt  (intermediary files)
//   - run_dir/solutions/<a>_<b>_<c>_solution.txt         (final algorithms)
//
// Usage:
//   ./orbit_pipeline <orbit_points_dir> <M> <s> <intermediary_out_dir> <table_with_formula> <solutions_out_dir>
//
// Example:
//   ./orbit_pipeline runs/2025-10-29_12-34-56/orbit_points 50 5 \
//                    runs/2025-10-29_12-34-56/intermediary table_with_formula_v2.txt \
//                    runs/2025-10-29_12-34-56/solutions

#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <array>
#include <string>
#include <regex>
#include <algorithm>
#include <filesystem>
#include <cctype>

using namespace std;
namespace fs = std::filesystem;

// ------------------------ shared helpers ------------------------

static inline string trim(const string& s) {
    size_t i = 0, j = s.size();
    while (i < j && isspace((unsigned char)s[i])) ++i;
    while (j > i && isspace((unsigned char)s[j-1])) --j;
    return s.substr(i, j - i);
}

static vector<string> split_ws(const string& s) {
    vector<string> toks;
    string cur;
    for (char ch : s) {
        if (isspace((unsigned char)ch)) {
            if (!cur.empty()) { toks.push_back(cur); cur.clear(); }
        } else {
            cur.push_back(ch);
        }
    }
    if (!cur.empty()) toks.push_back(cur);
    return toks;
}

static string join_csv(const vector<int>& v) {
    ostringstream o;
    for (size_t i = 0; i < v.size(); ++i) { if (i) o << ","; o << v[i]; }
    return o.str();
}
static string brace_list(const vector<int>& v) {
    ostringstream o; o << "{";
    for (size_t i = 0; i < v.size(); ++i) { if (i) o << ","; o << v[i]; }
    o << "}"; return o.str();
}

// ------------------------ read orbit_points (x,y) ------------------------

static vector<pair<int,int>> read_tuples_file(const fs::path& file) {
    vector<pair<int,int>> pts;
    ifstream f(file);
    if (!f) return pts;
    string line;
    while (getline(f, line)) {
        if (line.empty()) continue;
        // lines are "x,y"
        replace(line.begin(), line.end(), ',', ' ');
        istringstream iss(line);
        int x, y;
        if (iss >> x >> y) { 
            if (x == 1 || y == 1 || x==y) continue;
        pts.emplace_back(x,y);
        }
    }
    return pts;
}

// ------------------------ sector pattern builder ("boxer") ------------------------
// (based on your boxer.cpp; adapted to take points vector and to write into given paths)
// Ref: :contentReference[oaicite:0]{index=0}

static string make_hit_vector(const vector<int>& hits, int K_local) {
    string s(K_local, '0');
    for (int h : hits) if (0 <= h && h < K_local) s[h] = '1';
    return s;
}

static vector<int> pattern_to_layers(const string& pattern, int x_start_one_based) {
    vector<int> out; out.reserve(pattern.size());
    for (int i = 0; i < (int)pattern.size(); ++i)
        if (pattern[i] == '1') out.push_back(x_start_one_based + i);
    return out;
}

struct SectorOutput {
    // map pattern -> columns
    unordered_map<string, vector<int>> table;
    // 1-based display range for this sector
    int disp_L = 1, disp_R = 0;
};

static SectorOutput build_sector_output(const vector<pair<int,int>>& points,
                                        int sector_id, int K_base, int K_local, int M)
{
    const int x_start = sector_id * K_base;
    const int x_end   = min(M, x_start + K_local);  // exclusive
    unordered_map<int, vector<int>> by_col;
    for (auto& [x,y] : points) {
        if (x >= x_start && x < x_end) by_col[y].push_back(x - x_start);
    }
    unordered_map<string, vector<int>> table;
    for (auto& [y, hits] : by_col) {
        sort(hits.begin(), hits.end());
        hits.erase(unique(hits.begin(), hits.end()), hits.end());
        string vec = make_hit_vector(hits, K_local);
        table[vec].push_back(y);
    }
    // Sort cols per pattern
    for (auto& kv : table) {
        auto& cols = kv.second;
        sort(cols.begin(), cols.end());
        cols.erase(unique(cols.begin(), cols.end()), cols.end());
    }
    SectorOutput out;
    out.table = std::move(table);
    out.disp_L = x_start + 1;
    out.disp_R = x_end; // since x_end is exclusive
    return out;
}

// ------------------------ double_parallelizer (from your code) ------------------------
// Ref: :contentReference[oaicite:1]{index=1}

struct FormulaEntry {
    array<int,3> triple{};
    string general_formula;
};

static vector<FormulaEntry> load_formula_table(const string& path) {
    ifstream f(path);
    if (!f) { cerr << "Error: cannot open " << path << "\n"; exit(1); }
    vector<FormulaEntry> res;
    string line;
    const regex rx(R"(^\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s+.*?\bgeneral\s*:\s*(.+?)\s*$)");
    while (getline(f, line)) {
        smatch m; if (!regex_search(line, m, rx)) continue;
        FormulaEntry e;
        e.triple = { stoi(m[1]), stoi(m[2]), stoi(m[3]) };
        e.general_formula = trim(m[4]);
        if (!e.general_formula.empty()) res.push_back(std::move(e));
    }
    return res;
}

struct BracketParts { vector<string> prefix, left, right, suffix; };

static BracketParts parse_general(const string& general) {
    BracketParts out;
    size_t lb = general.find('[');
    if (lb == string::npos) { out.prefix = split_ws(general); return out; }
    int depth = 0; size_t comma_pos = string::npos, rb = string::npos;
    for (size_t i = lb; i < general.size(); ++i) {
        char ch = general[i];
        if (ch == '[') ++depth;
        else if (ch == ']') { --depth; if (depth == 0) { rb = i; break; } }
        else if (ch == ',' && depth == 1 && comma_pos == string::npos) comma_pos = i;
    }
    string pref = general.substr(0, lb);
    string suff = (rb != string::npos && rb + 1 < general.size()) ? general.substr(rb + 1) : string();
    out.prefix = split_ws(pref);
    out.suffix = split_ws(suff);
    if (rb == string::npos || comma_pos == string::npos) return out;
    string leftStr  = general.substr(lb + 1, comma_pos - (lb + 1));
    string rightStr = general.substr(comma_pos + 1, rb - (comma_pos + 1));
    out.left  = split_ws(leftStr);
    out.right = split_ws(rightStr);
    return out;
}

struct Placeholder {
    char var = 0;     // 'p' or 'q'
    char axis = 0;    // R L U D F B
    string suffix;    // "" or "'" or "2"
};
static bool parse_placeholder(const string& tok, Placeholder& out) {
    static const regex rx(R"(^\((?:11|M\+1)-(p|q)\)\s*([RLUDFB])((?:'|2)?)$)");
    smatch m; if (!regex_match(tok, m, rx)) return false;
    out.var = m[1].str()[0];
    out.axis = m[2].str()[0];
    out.suffix = m[3].str();
    return true;
}

static vector<string> expand_placeholder_block(const Placeholder& ph,
                                               const vector<int>& p_orbits,
                                               const vector<int>& q_orbits)
{
    const vector<int>& lst = (ph.var == 'p') ? p_orbits : q_orbits;
    vector<string> out; if (lst.empty()) return out;
    bool reverseOrder = (ph.var == 'p' && !ph.suffix.empty() && ph.suffix.find('\'') != string::npos);
    auto emit = [&](int v){
        string mv; mv.reserve(8);
        mv += to_string(v);
        mv += ph.axis;
        if (ph.suffix == "'") mv += "'";
        else if (ph.suffix == "2") mv += "2";
        out.push_back(std::move(mv));
    };
    if (reverseOrder) for (int i = (int)lst.size() - 1; i >= 0; --i) emit(lst[i]);
    else for (int v : lst) emit(v);
    return out;
}

static string join_tokens(const vector<string>& v) {
    ostringstream o; for (size_t i = 0; i < v.size(); ++i) { if (i) o << ' '; o << v[i]; }
    return o.str();
}

static string render_double_parallelized_inline(const FormulaEntry& row,
                                                const vector<int>& p_orbits,
                                                const vector<int>& q_orbits)
{
    BracketParts bp = parse_general(row.general_formula);
    auto expand_half = [&](const vector<string>& half) {
        vector<string> out;
        for (const auto& t : half) {
            Placeholder ph;
            if (parse_placeholder(t, ph)) {
                auto blk = expand_placeholder_block(ph, p_orbits, q_orbits);
                out.insert(out.end(), blk.begin(), blk.end());
            } else {
                out.push_back(t);
            }
        }
        return out;
    };
    vector<string> L = expand_half(bp.left);
    vector<string> R = expand_half(bp.right);

    ostringstream oss;
    if (!bp.prefix.empty()) oss << join_tokens(bp.prefix) << ' ';
    oss << '[' << join_tokens(L) << ", " << join_tokens(R) << ']';
    if (!bp.suffix.empty()) oss << ' ' << join_tokens(bp.suffix);
    return oss.str();
}

static string generate_double_parallelized(
    int a, int b, int c,
    const vector<int>& p_orbits,
    const vector<int>& q_orbits,
    const string& table_path
) {
    auto table = load_formula_table(table_path);
    array<int,3> want = {a,b,c};
    sort(want.begin(), want.end());
    for (const auto& row : table) {
        auto t = row.triple;
        sort(t.begin(), t.end());
        if (t == want) return render_double_parallelized_inline(row, p_orbits, q_orbits);
    }
    return "# no matching triple found";
}

// ------------------------ glue: iterate orbit_points and write outputs ------------------------

static void write_intermediary(const fs::path& file_path,
                               int a, int b, int c,
                               int sector_id,
                               const SectorOutput& S,
                               const fs::path& intermediary_dir,
                               const string& table_path,
                               const fs::path& solutions_file)
{
    // 1) intermediary file for this sector
    fs::create_directories(intermediary_dir);
    ostringstream fname;
    fname << file_path.stem().string() << "__sector_" << sector_id << ".txt";
    fs::path out_path = intermediary_dir / fname.str();
    ofstream out(out_path);
    if (!out) {
        cerr << "Error: cannot write intermediary " << out_path << "\n";
        return;
    }

    // 2) solutions file (append per sector)
    ofstream sol(solutions_file, ios::app);
    if (!sol) {
        cerr << "Error: cannot write solutions " << solutions_file << "\n";
        return;
    }

    // emit
    for (auto& kv : S.table) {
        const string& pattern = kv.first;
        const vector<int>& cols = kv.second;
        vector<int> p_layers = pattern_to_layers(pattern, S.disp_L);
        const vector<int>& q_layers = cols;

        // Intermediary
        out << pattern << " - " << join_csv(cols)
            << " (" << S.disp_L << "-" << S.disp_R << ")\n";
        out << "p_layers=" << brace_list(p_layers)
            << "; q_layers=" << brace_list(q_layers) << "\n";

        // Algorithm
        string alg = generate_double_parallelized(a,b,c, p_layers, q_layers, table_path);
        out << "ALGO: " << alg << "\n";
        out << "---\n";

        // Save algorithm only in the solutions file
        sol << alg << "\n";
    }
}

static bool parse_triple_from_filename(const fs::path& file, int& a, int& b, int& c) {
    // Expect "<a>_<b>_<c>.txt"
    static const regex rx(R"(^(\d+)_\s*(\d+)_\s*(\d+)\.txt$)");
    smatch m;
    string name = file.filename().string();
    if (!regex_match(name, m, rx)) return false;
    a = stoi(m[1]); b = stoi(m[2]); c = stoi(m[3]);
    return true;
}

int main(int argc, char* argv[]) {
    if (argc < 7) {
        cerr << "Usage: " << argv[0]
             << " <orbit_points_dir> <M> <s> <intermediary_out_dir> <table_with_formula> <solutions_out_dir>\n";
        return 1;
    }

    fs::path orbit_dir   = argv[1];
    const int M          = stoi(argv[2]);
    const int s          = stoi(argv[3]);
    fs::path inter_dir   = argv[4];
    string   table_path  = argv[5];
    fs::path solutions_dir = argv[6];

    if (!fs::exists(orbit_dir) || !fs::is_directory(orbit_dir)) {
        cerr << "Error: orbit_points_dir not found: " << orbit_dir << "\n";
        return 1;
    }
    fs::create_directories(inter_dir);
    fs::create_directories(solutions_dir);
    if (M <= 0 || s <= 0) { cerr << "Error: M and s must be positive.\n"; return 1; }

    // Sector widths
    const int K_base = M / s;
    const int R      = M - s * K_base;

    // Iterate over all "<a>_<b>_<c>.txt" in orbit_points_dir
    int files = 0;
    for (auto& de : fs::directory_iterator(orbit_dir)) {
        if (!de.is_regular_file()) continue;
        fs::path path = de.path();
        if (path.extension() != ".txt") continue;

        int a=0,b=0,c=0;
        if (!parse_triple_from_filename(path, a,b,c)) continue;

        // read points and skip if empty
        auto points = read_tuples_file(path);
        if (points.empty()) continue;
        ++files;

        // prepare solutions file (truncate per triple to have clean re-run)
        fs::path sol_file = solutions_dir / (path.stem().string() + "_solution.txt");
        ofstream clear(sol_file); // truncate
        if (!clear) { cerr << "Error: cannot create " << sol_file << "\n"; continue; }

        // process base sectors
        for (int sector=0; sector<s; ++sector) {
            if (K_base <= 0) break;
            SectorOutput S = build_sector_output(points, sector, K_base, K_base, M);
            write_intermediary(path, a,b,c, sector, S, inter_dir, table_path, sol_file);
        }
        // remainder sector, if any
        if (R > 0) {
            SectorOutput S = build_sector_output(points, s, K_base, R, M);
            write_intermediary(path, a,b,c, s, S, inter_dir, table_path, sol_file);
        }
        cout << "✅ processed " << path.filename().string()
             << "  -> solutions: " << (sol_file) << "\n";
    }

    if (files == 0) {
        cerr << "No orbit_points files found in: " << orbit_dir << "\n";
        return 2;
    }
    cout << "All done.\n";
    return 0;
}
