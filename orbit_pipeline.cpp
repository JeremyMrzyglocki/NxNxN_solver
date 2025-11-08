#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;
namespace fs = std::filesystem;

// ────────────────────────────────────────────────────────────
// Utilities
// ────────────────────────────────────────────────────────────
static inline string trim(const string& s){
    size_t i=0,j=s.size();
    while(i<j && isspace((unsigned char)s[i])) ++i;
    while(j>i && isspace((unsigned char)s[j-1])) --j;
    return s.substr(i,j-i);
}

static vector<string> split_ws(const string& s){
    vector<string> t; string cur;
    for(char ch: s){
        if(isspace((unsigned char)ch)){
            if(!cur.empty()){ t.push_back(cur); cur.clear(); }
        } else cur.push_back(ch);
    }
    if(!cur.empty()) t.push_back(cur);
    return t;
}

static string join_csv(const vector<int>& v){
    ostringstream o;
    for(size_t i=0;i<v.size();++i){ if(i) o<<","; o<<v[i]; }
    return o.str();
}
static string brace_list(const vector<int>& v){
    ostringstream o; o<<"{"<<join_csv(v)<<"}";
    return o.str();
}

// ────────────────────────────────────────────────────────────
// Read orbit_points (x,y) pairs (already produced earlier)
// ────────────────────────────────────────────────────────────
static vector<pair<int,int>> read_tuples_file(const fs::path& file, int M){
    const int usable_M = M - 1;
    vector<pair<int,int>> pts;
    ifstream f(file);
    if(!f){ cerr<<"Error: cannot open "<<file<<"\n"; return pts; }
    string line;
    while(getline(f,line)){
        if(line.empty()) continue;
        replace(line.begin(), line.end(), ',', ' ');
        istringstream iss(line);
        int x,y; if(!(iss>>x>>y)) continue;
        // basic guard rails (and skip diagonal)
        if(x<0 || y<0 || x>=usable_M || y>=usable_M || x==y) continue;
        pts.emplace_back(x,y);
    }
    return pts;
}

static string make_hit_vector(const vector<int>& hits, int K){
    string s(K,'0');
    for(int h: hits) if(0<=h && h<K) s[h] = '1';
    return s;
}
static vector<int> pattern_to_layers(const string& patt, int x0){
    vector<int> out; out.reserve(patt.size());
    for(int i=0;i<(int)patt.size();++i) if(patt[i]=='1') out.push_back(x0+i);
    return out;
}

struct SectorOutput{
    unordered_map<string, vector<int>> table; // pattern -> rows (q-layers)
    int disp_L = 0, disp_R = -1;             // display range for columns in this sector
};

static SectorOutput build_sector_output(const vector<pair<int,int>>& points,
                                        int sector, int K_base, int K_local, int M_usable){
    const int x0 = sector * K_base;
    const int x1 = min(M_usable, x0 + K_local);
    unordered_map<int, vector<int>> by_row;      // y -> local x offsets with hits
    for(auto& [x,y] : points)
        if(x>=x0 && x<x1) by_row[y].push_back(x - x0);

    unordered_map<string, vector<int>> table;
    const int W = max(0, x1 - x0);
    for(auto& [y, h] : by_row){
        sort(h.begin(), h.end());
        h.erase(unique(h.begin(), h.end()), h.end());
        table[make_hit_vector(h, W)].push_back(y);
    }
    for(auto& kv : table){
        auto& v = kv.second;
        sort(v.begin(), v.end());
        v.erase(unique(v.begin(), v.end()), v.end());
    }
    return { std::move(table), x0, x1-1 };
}

// ────────────────────────────────────────────────────────────
// CLI
// ────────────────────────────────────────────────────────────
static bool parse_triple_from_filename(const fs::path& file, int& a, int& b, int& c){
    static const regex rx(R"(^\s*(\d+)\s*_\s*(\d+)\s*_\s*(\d+)\s*\.txt\s*$)");
    smatch m; string name = file.filename().string();
    if(!regex_match(name, m, rx)) return false;
    a = stoi(m[1]); b = stoi(m[2]); c = stoi(m[3]);
    return true;
}

struct Cli{
    fs::path orbit_path, inter_dir, sol_dir; // sol_dir kept for CLI compatibility (not used)
    string table_path;                       // kept for CLI compatibility (not used)
    int M = 0, s = 0;
};

static bool parse_cli(int argc, char* argv[], Cli& cli){
    if(argc < 7) return false;
    cli.orbit_path = argv[1];
    cli.M          = stoi(argv[2]);
    cli.s          = stoi(argv[3]);
    cli.inter_dir  = argv[4];
    cli.table_path = argv[5]; // ignored now
    cli.sol_dir    = argv[6]; // ignored now
    return true;
}

// ────────────────────────────────────────────────────────────
// MAIN
// ────────────────────────────────────────────────────────────
int main(int argc, char* argv[]){
    Cli cli;
    if(!parse_cli(argc, argv, cli)){
        cerr << "Usage:\n  " << argv[0]
             << " <orbit_points_dir_or_file> <M> <s> <intermediary_out_dir> <table_with_formula> <solutions_out_dir>\n";
        cerr << "(note: as of this build, no solution files are produced)\n";
        return 1;
    }
    if(cli.M <= 1 || cli.s <= 0){
        cerr << "Error: M>=2 and s>0.\n"; return 1;
    }

    // We still emit per-file intermediary diagnostics (optional); no solutions.
    fs::create_directories(cli.inter_dir);

    const int usable_M = cli.M - 1;
    const int K_base   = usable_M / cli.s;
    const int R        = usable_M - cli.s * K_base;

    vector<fs::path> files;
    if(fs::is_regular_file(cli.orbit_path)){
        files.push_back(cli.orbit_path);
    } else if(fs::is_directory(cli.orbit_path)){
        for(auto& de : fs::directory_iterator(cli.orbit_path))
            if(de.is_regular_file() && de.path().extension()==".txt")
                files.push_back(de.path());
        sort(files.begin(), files.end(),
            [](const fs::path& a, const fs::path& b){
                return a.filename().string() < b.filename().string();
            });
    } else {
        cerr << "Error: orbit_points path not found.\n"; return 1;
    }

    size_t processed = 0;
    for(const auto& path : files){
        int a=0,b=0,c=0;
        if(!parse_triple_from_filename(path, a, b, c)) continue;

        auto points = read_tuples_file(path, cli.M);
        if(points.empty()) continue;

        // optional: write intermediary sector summaries (no algorithms)
        for(int sector=0; sector<cli.s && K_base>0; ++sector){
            SectorOutput S = build_sector_output(points, sector, K_base, K_base, usable_M);
            fs::create_directories(cli.inter_dir);
            ofstream out(cli.inter_dir / (path.stem().string()+"__sector_"+to_string(sector)+".txt"));
            for(const auto& kv : S.table){
                const string& pattern = kv.first;
                const vector<int>& rows = kv.second;
                vector<int> p_layers = pattern_to_layers(pattern, S.disp_L); // columns in this sector
                out << pattern << " - " << join_csv(rows) << " (" << S.disp_L << "-" << S.disp_R << ")\n";
                out << "p_layers=" << brace_list(p_layers) << "; q_layers=" << brace_list(rows) << "\n";
                out << "---\n";
            }
        }
        if(R > 0){
            SectorOutput S = build_sector_output(points, cli.s, K_base, R, usable_M);
            fs::create_directories(cli.inter_dir);
            ofstream out(cli.inter_dir / (path.stem().string()+"__sector_"+to_string(cli.s)+".txt"));
            for(const auto& kv : S.table){
                const string& pattern = kv.first;
                const vector<int>& rows = kv.second;
                vector<int> p_layers = pattern_to_layers(pattern, S.disp_L);
                out << pattern << " - " << join_csv(rows) << " (" << S.disp_L << "-" << S.disp_R << ")\n";
                out << "p_layers=" << brace_list(p_layers) << "; q_layers=" << brace_list(rows) << "\n";
                out << "---\n";
            }
        }

        cout << "✅ processed " << path.filename().string() << "\n";
        ++processed;
    }

    if(processed==0){
        cerr << "No orbit_points files found.\n"; return 2;
    }
    cout << "All done. Files: " << processed << " (no solution files produced in this mode)\n";
    return 0;
}
