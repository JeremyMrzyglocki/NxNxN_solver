#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <stdexcept>
#include <map>
#include <cctype>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <sstream>

using namespace std;
static string g_table_path = "table_with_formula_v2.txt";

// ---------- Triples, hashing ----------
struct Triple {
    uint16_t i, j, k;
    bool operator==(const Triple& o) const noexcept {
        return i==o.i && j==o.j && k==o.k;
    }
};
struct TripleHash {
    size_t operator()(const Triple& t) const noexcept {
        return (size_t(t.i) << 32) ^ (size_t(t.j) << 16) ^ size_t(t.k);
    }
};

// 0 = direct, 1 = inverted
struct PerTripletOut {
    std::vector<std::string> lines[2];
};

// ---------- Utility types ----------
using HitRowsPair = pair<vector<int>, vector<int>>;

// ---------- Grid helpers ----------
void fillGrid(int N, const vector<pair<int,int>>& pts, vector<vector<int>>& grid) {
    for (auto [x,y] : pts) {
        if (x >= 1 && x < N && y >= 1 && y < N) {
            grid[x][y] = 1;
        }
    }
}

static inline void normalize_cycle_rotate(int& a, int& b, int& c) {
    if (a <= b && a <= c) return;
    if (b <= a && b <= c) { int ta=a; a=b; b=c; c=ta; return; }
    { int ta=a; a=c; c=b; b=ta; }
}

// Build the results across all sectors (deterministic order)
vector<HitRowsPair> collect_pattern_results(int N, int num_sectors, const vector<vector<int> >& grid) {
    vector<HitRowsPair> out;
    if (num_sectors < 1) throw invalid_argument("num_sectors must be >= 1");

    int C = N - 1;
    if (C < 1) return out;
    if (num_sectors > C) num_sectors = C;

    int base_width = C / num_sectors;
    int remainder  = C % num_sectors;

    int index_in_sector = 1;
    for (int sector = 0; sector < num_sectors; ++sector) {
        int width  = (sector == num_sectors - 1) ? (base_width + remainder) : base_width;
        int left_c = index_in_sector;
        int right_c = left_c + width - 1;

        map<string, vector<int>> pattern_rows; // lexicographic order

        for (int row = 1; row <= C; ++row) {
            string bits;
            bits.reserve(width);
            for (int col = left_c; col <= right_c; ++col)
                bits.push_back(grid[row][col] ? '1' : '0');
            pattern_rows[bits].push_back(row);
        }

        for (const auto& kv : pattern_rows) {
            const string& pat = kv.first;
            const vector<int>& rows = kv.second;

            vector<int> hit_cols;
            for (int k = 0; k < width; ++k) {
                if (pat[k] == '1') {
                    int global_col = left_c + k;
                    hit_cols.push_back(global_col);
                }
            }

            if (!hit_cols.empty()) {
                vector<int> rows_sorted = rows;
                sort(hit_cols.begin(), hit_cols.end());
                sort(rows_sorted.begin(), rows_sorted.end());
                out.emplace_back(hit_cols, rows_sorted);
            }
        }

        index_in_sector += width;
    }

    return out;
}

// --- Swap top-level commutator contents: [A,B] -> [B,A]
static std::string swap_commutator_contents(const std::string& s) {
    std::string out; out.reserve(s.size());
    for (std::size_t p = 0; p < s.size(); ) {
        if (s[p] != '[') { out.push_back(s[p++]); continue; }
        std::size_t q = s.find(']', p+1);
        if (q == std::string::npos) { out.push_back(s[p++]); continue; }
        std::string inner = s.substr(p+1, q-(p+1));
        std::size_t comma = inner.find(',');
        if (comma == std::string::npos) {
            out.append(s, p, q-p+1);
            p = q+1; continue;
        }
        auto trim = [](std::string& t) {
            std::size_t a=0,b=t.size();
            while (a<b && std::isspace((unsigned char)t[a])) a++;
            while (b>a && std::isspace((unsigned char)t[b-1])) b--;
            t = t.substr(a,b-a);
        };
        std::string A = inner.substr(0, comma);
        std::string B = inner.substr(comma+1);
        trim(A); trim(B);
        out.push_back('[');
        out += B; out += ", "; out += A;
        out.push_back(']');
        p = q+1;
    }
    return out;
}

// ---------- Core 2D alg expansion result ----------
struct GenResult {
    bool ok = false;
    std::string expanded;
    bool inverted = false;  // true => matched inverse rotation
    Triple chosen{0,0,0};   // exact (i,j,k) triplet from the table
    std::string error;
};

// ---------- Helpers for filenames & input ----------
static bool parse_filename_triplet(const std::string& fname, int& a, int& b, int& c) {
    auto base = fname.substr(fname.find_last_of("/\\") + 1);
    auto dot  = base.find_last_of('.');
    if (dot != std::string::npos) base = base.substr(0, dot);

    std::replace(base.begin(), base.end(), '-', '_');
    std::stringstream ss(base);
    std::string s1,s2,s3;
    if (!std::getline(ss, s1, '_')) return false;
    if (!std::getline(ss, s2, '_')) return false;
    if (!std::getline(ss, s3, '_')) return false;
    try {
        a = std::stoi(s1);
        b = std::stoi(s2);
        c = std::stoi(s3);
    } catch (...) { return false; }
    return true;
}

static bool read_points_file(const std::string& path, std::vector<std::pair<int,int>>& pts) {
    std::ifstream fin(path);
    if (!fin) return false;

    pts.clear();
    std::string line;
    while (std::getline(fin, line)) {
        if (line.empty()) continue;

        std::istringstream iss(line);
        int x, y;
        char comma;
        if ((iss >> x >> std::ws >> comma >> std::ws >> y) && comma == ',') {
            pts.emplace_back(x, y);
        }
    }
    return true;
}

// ---------- Tokenize & expand for one pair set; returns orientation + chosen triple ----------
static GenResult algs_2D_gen_full(int a, int b, int c, int n, const HitRowsPair& result_pair) {
    GenResult gr;

    const std::vector<int>& cols_in = result_pair.second; // p-group
    const std::vector<int>& rows_in = result_pair.first;  // q-group

    struct Tok { char var; char face; std::string suf; };
    struct FormulaInfo {
        std::string general;
        std::vector<std::pair<bool,std::string>> parts; // (isToken, literal)
        std::vector<Tok> toks;
        int pCount = 0, qCount = 0;
        char type = 0;
    };

    auto is_space = [](char ch){ return std::isspace(static_cast<unsigned char>(ch))!=0; };
    auto is_digit = [](char ch){ return std::isdigit(static_cast<unsigned char>(ch))!=0; };

    auto trim = [&](std::string& s){
        size_t a = 0, b = s.size();
        while (a < b && is_space(s[a])) ++a;
        while (b > a && is_space(s[b-1])) --b;
        s.assign(s.begin()+a, s.begin()+b);
    };

    auto tokenize_general = [&](const std::string& s)->FormulaInfo {
        FormulaInfo info; info.general = s;
        size_t i = 0, N = s.size();

        auto is_pq = [](char ch){
            ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            return ch=='p' || ch=='q';
        };
        auto is_face  = [](char ch){
            ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            return ch=='R'||ch=='L'||ch=='F'||ch=='B'||ch=='U'||ch=='D';
        };

        while (i < N) {
            size_t open = s.find('(', i);
            if (open == std::string::npos) {
                info.parts.emplace_back(false, s.substr(i));
                break;
            }
            if (open > i) info.parts.emplace_back(false, s.substr(i, open - i));

            size_t j = open + 1;
            while (j < N && is_space(s[j])) ++j;

            size_t dash = std::string::npos, close_paren = std::string::npos;
            for (size_t k = j; k < N; ++k) {
                if (s[k] == '-' && dash == std::string::npos) dash = k;
                if (s[k] == ')') { close_paren = k; break; }
            }

            bool ok = false;
            char var = 0, face = 0;
            std::string suf;

            if (dash != std::string::npos && close_paren != std::string::npos && dash < close_paren) {
                size_t vpos = dash + 1;
                while (vpos < close_paren && is_space(s[vpos])) ++vpos;
                if (vpos < close_paren && is_pq(s[vpos])) {
                    var = static_cast<char>(std::tolower(static_cast<unsigned char>(s[vpos++])));
                    while (vpos < close_paren && is_space(s[vpos])) ++vpos;
                    if (vpos == close_paren) {
                        size_t t = close_paren + 1;
                        while (t < N && is_space(s[t])) ++t;
                        if (t < N && is_face(s[t])) {
                            face = static_cast<char>(std::toupper(static_cast<unsigned char>(s[t++])));
                            if (t < N && (s[t]=='2' || s[t]=='\'')) { suf.push_back(s[t]); ++t; }
                            info.parts.emplace_back(true, std::string());
                            info.toks.push_back(Tok{var, face, suf});
                            if (var=='p') info.pCount++; else info.qCount++;
                            i = t; ok = true;
                        }
                    }
                }
            }

            if (!ok) {
                info.parts.emplace_back(false, std::string(1, s[open]));
                i = open + 1;
            }
        }

        if (info.pCount==1 && info.qCount!=1) info.type='p';
        else if (info.qCount==1 && info.pCount!=1) info.type='q';
        else if (info.pCount==1 && info.qCount==1) info.type='p';
        else info.type = 0;

        return info;
    };

    auto fix_commas = [](std::string& s){
        for (size_t pos=0; pos+1<s.size(); ++pos)
            if (s[pos]==',' && s[pos+1] != ' ') s.insert(pos+1, " ");
    };

    // ---- Read table and find matching (i,j,k) line (direct or inverse rotation) ----
    ifstream in(g_table_path);

    if (!in) {
        gr.ok = false;
        gr.error = "ERROR: cannot open table_with_formula_v2.txt";
        return gr;
    }

    std::string line, general;
    auto rd_int = [&](const std::string& L, size_t& p, int& out)->bool{
        while (p<L.size() && is_space(L[p])) ++p;
        size_t st=p; while (p<L.size() && is_digit(L[p])) ++p;
        if (st==p) return false; out = std::stoi(L.substr(st, p-st)); return true;
    };

    bool found = false;
    bool inverted = false;
    Triple chosen{0,0,0};

    while (std::getline(in, line)) {
        size_t p = 0; int i_, j_, k_;
        if (!rd_int(line, p, i_)) continue;
        while (p<line.size() && (isspace((unsigned char)line[p]) || line[p]==',')) ++p;
        if (!rd_int(line, p, j_)) continue;
        while (p<line.size() && (isspace((unsigned char)line[p]) || line[p]==',')) ++p;
        if (!rd_int(line, p, k_)) continue;

        size_t pos = line.find("general:", p);
        if (pos == std::string::npos) continue;
        pos += 8;
        while (pos<line.size() && isspace((unsigned char)line[pos])) ++pos;
        std::string g = line.substr(pos);
        trim(g);

        // direct rotations of (a,b,c)
        if ((i_==a && j_==b && k_==c) ||
            (i_==b && j_==c && k_==a) ||
            (i_==c && j_==a && k_==b)) {
            general = std::move(g);
            found = true;
            inverted = false;
            chosen = Triple{(uint16_t)i_, (uint16_t)j_, (uint16_t)k_};
            break;
        }
        // inverse rotations of (a,b,c)
        if ((i_==a && j_==c && k_==b) ||
            (i_==c && j_==b && k_==a) ||
            (i_==b && j_==a && k_==c)) {
            general = g;
            found = true;
            inverted = true;
            chosen = Triple{(uint16_t)i_, (uint16_t)j_, (uint16_t)k_};
            break;
        }
    }

    if (!found) {
        std::ostringstream oss;
        oss << "ERROR: no table entry for ("<<a<<","<<b<<","<<c<<") or its rotations/inverse";
        gr.ok = false; gr.error = oss.str();
        return gr;
    }

    // ---- Tokenize & expand (vectorized p/q) ----
    FormulaInfo fi = tokenize_general(general);

    const int M = n; // mirroring uses this size
    std::vector<int> cols, rows;
    cols.reserve(cols_in.size());
    rows.reserve(rows_in.size());
    for (int v : cols_in) cols.push_back((M + 1) - v);
    for (int v : rows_in) rows.push_back((M + 1) - v);
    std::sort(cols.begin(), cols.end());
    std::sort(rows.begin(), rows.end());

    auto append_expansion = [&](std::string& out, char var, char face, const std::string& suf) {
        const std::vector<int>& bag = (var=='p') ? cols : rows;
        for (size_t i = 0; i < bag.size(); ++i) {
            out += std::to_string(bag[i]);
            out.push_back(face);
            out += suf;
            if (i + 1 < bag.size()) out.push_back(' ');
        }
    };

    std::string expanded; size_t tok_idx = 0;
    for (const auto& part : fi.parts) {
        if (!part.first) expanded += part.second;
        else {
            const Tok& t = fi.toks[tok_idx++];
            append_expansion(expanded, t.var, t.face, t.suf);
        }
    }
    fix_commas(expanded);

    if (expanded.find('[') == std::string::npos && expanded.find(']') == std::string::npos) {
        expanded.insert(expanded.begin(), '[');
        expanded.push_back(']');
    }
    // TABLE IS INVERSE: flip commutator for direct; keep original for inverted
    if (!inverted) expanded = swap_commutator_contents(expanded);

    gr.ok = true;
    gr.expanded = std::move(expanded);
    gr.inverted = inverted;
    gr.chosen = chosen;
    return gr;
}

// ---------- Process one wave folder: accumulate & emit like rearrange.cpp ----------
void run_points_to_2D_algs(const std::string& folder, int N, int num_sectors, int wavenum,
                           const std::string& outdir) {
    namespace fs = std::filesystem;
    // Per-wave accumulator (triplet -> [direct, inverted] lines)
    std::unordered_map<Triple, PerTripletOut, TripleHash> acc_wave;

    for (const auto& entry : fs::directory_iterator(folder)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".txt") continue;

        const std::string path = entry.path().string();
        int a=0,b=0,c=0;
        if (!parse_filename_triplet(path, a, b, c)) {
            std::cerr << "WARN: skip '" << path << "' (cannot parse a_b_c)\n";
            continue;
        }

        int A=a, B=b, C=c;
        normalize_cycle_rotate(A, B, C);

        std::vector<std::pair<int,int>> pts;
        if (!read_points_file(path, pts)) {
            std::cerr << "WARN: skip '" << path << "' (cannot read points)\n";
            continue;
        }

        std::vector<std::vector<int>> grid(N, std::vector<int>(N, 0));
        fillGrid(N, pts, grid);
        std::vector<HitRowsPair> results = collect_pattern_results(N, num_sectors, grid);


        for (const auto& pr : results) {
            GenResult gr = algs_2D_gen_full(A, B, C, /*n=*/N, pr);
            if (!gr.ok) {
                std::cerr << "WARN: " << gr.error << "\n";
                continue;
            }
            int ori = gr.inverted ? 1 : 0;
            acc_wave[gr.chosen].lines[ori].push_back(std::move(gr.expanded));
        }
    }

    // Now emit this wave in rearrange.cpp order:
    std::vector<Triple> order;
    order.reserve(acc_wave.size());
    for (const auto& kv : acc_wave) order.push_back(kv.first);
    std::sort(order.begin(), order.end(), [](const Triple& a, const Triple& b){
        if (a.i!=b.i) return a.i<b.i;
        if (a.j!=b.j) return a.j<b.j;
        return a.k<b.k;
    });

    std::filesystem::create_directories(outdir);
    std::ofstream sol(outdir + "/sol_wave" + to_string(wavenum) + ".txt", ios::out | ios::trunc);    
    if (!sol) {
        std::cerr << "ERROR: cannot create sol_wave" << wavenum << ".txt\n";
        return;
    }
    for (const Triple& t : order) {
        const auto& per = acc_wave.at(t);
        for (const std::string& ln : per.lines[0]) sol << ln << '\n'; // direct first (swapped)
        for (const std::string& ln : per.lines[1]) sol << ln << '\n'; // inverted (original)
    }

    std::cout << "Wrote algorithms to sol_wave" << wavenum << ".txt\n";
}

struct Cli {
    std::string project;   // path to timestamped run dir
    int waves = 1;         // number of waves to process
    int N = 10;            // grid size
    int sectors = 1;       // vertical sector count
    std::string table;     // table path (optional)
};

static bool parse_cli(int argc, char** argv, Cli& cli) {
    // expected flags: --project <dir> --waves <W> --N <n> --sectors <s> [--table <path>]
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto need = [&](bool ok, const char* flag){ if(!ok){ std::cerr<<"Missing value for "<<flag<<"\n"; std::exit(2);} };
        if (a == "--project")  { need(i+1<argc, "--project");  cli.project = argv[++i]; }
        else if (a == "--waves"){ need(i+1<argc, "--waves");   cli.waves   = std::stoi(argv[++i]); }
        else if (a == "--N")    { need(i+1<argc, "--N");       cli.N       = std::stoi(argv[++i]); }
        else if (a == "--sectors"){need(i+1<argc, "--sectors");cli.sectors = std::stoi(argv[++i]); }
        else if (a == "--table"){ need(i+1<argc, "--table");   cli.table   = argv[++i]; }
        else if (a == "-h" || a == "--help") {
            std::cout <<
"Usage: orbit_points_to_2D_algs --project <run_dir> --waves <W> --N <n> --sectors <s> [--table <path>]\n";
            std::exit(0);
        }
    }
    if (cli.project.empty()) {
        std::cerr << "Error: --project <run_dir> is required\n";
        return false;
    }
    if (cli.waves < 1 || cli.N < 2 || cli.sectors < 1) {
        std::cerr << "Error: --waves>=1, --N>=2, --sectors>=1\n";
        return false;
    }
    return true;
}

int main(int argc, char** argv) {
    Cli cli;
    if (!parse_cli(argc, argv, cli)) {
        std::cerr <<
"Usage: orbit_points_to_2D_algs --project <run_dir> --waves <W> --N <n> --sectors <s> [--table <path>]\n";
        return 1;
    }

    // set table path (prefer flag; else look inside project; else default)
    if (!cli.table.empty()) {
        g_table_path = cli.table;
    } else {
        std::filesystem::path candidate = std::filesystem::path(cli.project) / "table_with_formula_v2.txt";
        if (std::filesystem::exists(candidate)) g_table_path = candidate.string();
        // else keep default "table_with_formula_v2.txt" in CWD
    }

    // per-wave processing (reads orbit_points_wave_<w>/, writes sol_wave<w>.txt in project)
    for (int w = 1; w <= cli.waves; ++w) {
        std::string folder = (std::filesystem::path(cli.project) /
                              ("orbit_points_wave_" + std::to_string(w))).string();
        run_points_to_2D_algs(folder, cli.N, cli.sectors, w, cli.project);
    }
}