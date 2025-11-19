#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <random>
#include <fstream>
#include <cctype>
#include <map>
#include <array>
#include <iomanip>
#include <sstream>
#include <climits>
#include <unordered_map>
#include <set>
#include <tuple>

using namespace std;

// "both" refers to the concatenated arrays of the two faces being swapped, maybe I should rename it to "concatenated"

vector<vector<int>> layer_1;
vector<vector<int>> layer_2;
vector<vector<int>> layer_3;
vector<vector<int>> layer_4;
vector<vector<int>> layer_5;

vector<vector<int>> layer_1_b, layer_2_b, layer_3_b, layer_4_b, layer_5_b;


inline void add_cycle_deferred(int a, int b, int c, int d, int e, int layer) {
    if (layer == 1)      layer_1_b.push_back({a,b,c,d,e});
    else if (layer == 2) layer_2_b.push_back({a,b,c,d,e});
    else if (layer == 3) layer_3_b.push_back({a,b,c,d,e});
    else if (layer == 4) layer_4_b.push_back({a,b,c,d,e});
    else if (layer == 5) layer_5_b.push_back({a,b,c,d,e});
}


static inline bool should_defer(int a, int b, int c, int face1, int face2, int layer) {
    static const set<tuple<int,int,int>> LOCAL_TRIPLES_TO_DEFER = {
        {4,8,7},{4,7,8},{2,6,7},{4,5,8},{2,8,3},{2,7,1} // These are the third and forth triples in cycle_tables_v2.csv
    };
        if (LOCAL_TRIPLES_TO_DEFER.count({a,b,c})) 
            return true;
        return false;
}



inline void add_cycle(int a, int b, int c, int d, int e, int layer) { // d and e are the face-indices
    if (layer == 1){layer_1.push_back({a, b, c, d, e});} 
    else if (layer == 2){layer_2.push_back({a, b, c, d, e});}
    else if (layer == 3){layer_3.push_back({a, b, c, d, e});}
    else if (layer == 4){layer_4.push_back({a, b, c, d, e});}
    else if (layer == 5){layer_5.push_back({a, b, c, d, e});}
} 


string compute_face_swap_full_orbit(const std::string& input, int face1, int face2) {
    // like 3145 | 3242 | 3403 | 5120 | 5141 | 2005
    vector<int> group0 = {input[0] -'0', input[1]- '0', input[2] -'0', input[3] -'0'};
    vector<int> group1 = {input[4] -'0', input[5]- '0', input[6] -'0', input[7] -'0'};
    vector<int> group2 = {input[8] -'0', input[9]- '0', input[10]-'0', input[11]-'0'};
    vector<int> group3 = {input[12]-'0', input[13]-'0', input[14]-'0', input[15]-'0'};
    vector<int> group4 = {input[16]-'0', input[17]-'0', input[18]-'0', input[19]-'0'};
    vector<int> group5 = {input[20]-'0', input[21]-'0', input[22]-'0', input[23]-'0'};

    vector<int> partner1;
    vector<int> partner2;

    switch (face1) {
        case 0: partner1 = group0; break;
        case 1: partner1 = group1; break;
        case 2: partner1 = group2; break;
        case 3: partner1 = group3; break;
        case 4: partner1 = group4; break;
        case 5: partner1 = group5; break;
    }

    switch (face2) {
        case 0: partner2 = group0; break;
        case 1: partner2 = group1; break;
        case 2: partner2 = group2; break;
        case 3: partner2 = group3; break;
        case 4: partner2 = group4; break;
        case 5: partner2 = group5; break;
    }
    //group0 here will be partner1, group5 will be partner2:

    vector<int> both = partner1;
    both.insert(both.end(), partner2.begin(), partner2.end()); // concatinate the arrays
    vector<int> both_sorted = both;
    vector<char> both_pattern(8, '5'); // 8 elements, all '5' for initialization
    //print both-array:

    // cout << "Both array: ";
    // for (int val : both) {
    //     cout << val << " ";
    // }
    // cout << std::endl;

    sort(both_sorted.begin(), both_sorted.end());

    int threshold_value = both_sorted[4];
    int count_threshold_in_both_sorted = 0;
    int count_below_threshold_in_both_sorted = 0;
    int count_threshold_in_both_unsorted_lower_half = 0; // actually only lower part

    for (short i = 0; i < 8; i++) {
        if (both_sorted[i] == threshold_value) { count_threshold_in_both_sorted += 1; }
        else if (both_sorted[i] < threshold_value) { count_below_threshold_in_both_sorted += 1; }
    }
    
    for (short i = 0; i < 4; i++) {
        if (both[i] == threshold_value) { count_threshold_in_both_unsorted_lower_half += 1; }
    }


    // CASE A (ssss tbbb, ssss ttbb, ssss tttb, ssss tttt)
    if ((count_threshold_in_both_sorted == 1) || (count_below_threshold_in_both_sorted == 4)) { // NO THRESHHOLD DUPLICATE (simple case)

        for (short i = 0; i < 4; i++) {
            if (both[i] < threshold_value) { both_pattern[i] = '0'; } // solved
            else if (both[i] >= threshold_value) { both_pattern[i] = '1'; } // not solved
        }
        for (short i = 4; i < 8; i++) {
            if (both[i] >= threshold_value) { both_pattern[i] = '0'; } // solved
            else if (both[i] < threshold_value) { both_pattern[i] = '1'; } // not solved
            
        }
    }

    // CASE B (ssst tbbb) 
    if ((count_threshold_in_both_sorted == 2) && (count_below_threshold_in_both_sorted == 3)) {
        //a)
        if (count_threshold_in_both_unsorted_lower_half == 1){ // good
            for (short i = 0; i < 4; i++) {
                if (both[i] < threshold_value) { both_pattern[i] = '0'; } // solved
                else if (both[i] > threshold_value) { both_pattern[i] = '1'; } // not solved
                else if (both[i] == threshold_value) { both_pattern[i] = '0'; } // solved
            }
            for (short i = 4; i < 8; i++) {
                if (both[i] > threshold_value) { both_pattern[i] = '0'; } // solved
                else if (both[i] < threshold_value) { both_pattern[i] = '1'; } // not solved
                else if (both[i] == threshold_value) { both_pattern[i] = '0'; } // solved
            }
        }
        //b)
        if (count_threshold_in_both_unsorted_lower_half == 2){ // not solved
            for (short i = 0; i < 4; i++) {
                if (both[i] < threshold_value) { both_pattern[i] = '0'; } // solved
                else if (both[i] > threshold_value) { both_pattern[i] = '1'; } // not solved
                else if (both[i] == threshold_value) { both_pattern[i] = '2'; } // solved
            }
            for (short i = 4; i < 8; i++) {
                if (both[i] > threshold_value) { both_pattern[i] = '0'; } // solved
                else if (both[i] < threshold_value) { both_pattern[i] = '1'; } // not solved
                else if (both[i] == threshold_value) { both_pattern[i] = 'e'; } // won't come up
            }
        }
        //c)
        if (count_threshold_in_both_unsorted_lower_half == 0){ // not solved
            for (short i = 0; i < 4; i++) {
                if (both[i] < threshold_value) { both_pattern[i] = '0'; } // solved
                else if (both[i] > threshold_value) { both_pattern[i] = '1'; } // not solved
                else if (both[i] == threshold_value) { both_pattern[i] = 'e'; } // won't come up
            }
            for (short i = 4; i < 8; i++) {
                if (both[i] > threshold_value) { both_pattern[i] = '0'; } // solved
                else if (both[i] < threshold_value) { both_pattern[i] = '1'; } // not solved
                else if (both[i] == threshold_value) { both_pattern[i] = '2'; } // won't come up
            }
        }
    }
    
    // CASE C (sstt tbbb)
    if ((count_threshold_in_both_sorted == 3) && (count_below_threshold_in_both_sorted == 2)) {
        //a)
        if (count_threshold_in_both_unsorted_lower_half == 2){ // good
            for (short i = 0; i < 4; i++) {
                if (both[i] < threshold_value) { both_pattern[i] = '0'; } // solved
                else if (both[i] > threshold_value) { both_pattern[i] = '1'; } // not solved
                else if (both[i] == threshold_value) { both_pattern[i] = '0'; } // solved
            }
            for (short i = 4; i < 8; i++) {
                if (both[i] > threshold_value) { both_pattern[i] = '0'; } // solved
                else if (both[i] < threshold_value) { both_pattern[i] = '1'; } // not solved
                else if (both[i] == threshold_value) { both_pattern[i] = '0'; } // solved
            }
        }

        //b)
        if (count_threshold_in_both_unsorted_lower_half == 1){ // not solved
            for (short i = 0; i < 4; i++) {
                if (both[i] < threshold_value) { both_pattern[i] = '0'; } // solved
                else if (both[i] > threshold_value) { both_pattern[i] = '1'; } // not solved
                else if (both[i] == threshold_value) { both_pattern[i] = '0'; } // solved
            }
            for (short i = 4; i < 8; i++) {
                if (both[i] > threshold_value) { both_pattern[i] = '0'; } // solved
                else if (both[i] < threshold_value) { both_pattern[i] = '1'; } // not solved
                else if (both[i] == threshold_value) { both_pattern[i] = '2'; } // not solved !!!
            }
        }

        //c)
        if (count_threshold_in_both_unsorted_lower_half == 3){
            for (short i = 0; i < 4; i++) {
                if (both[i] < threshold_value) { both_pattern[i] = '0'; } // solved
                else if (both[i] > threshold_value) { both_pattern[i] = '1'; } // not solved
                else if (both[i] == threshold_value) { both_pattern[i] = '2'; } // not solved !!! one of them has to go up
            }
            for (short i = 4; i < 8; i++) {
                if (both[i] > threshold_value) { both_pattern[i] = '0'; } // solved
                else if (both[i] < threshold_value) { both_pattern[i] = '1'; } // not solved
                else if (both[i] == threshold_value) { both_pattern[i] = 'e'; } // won't come up
            }
        }
        //d)
        if (count_threshold_in_both_unsorted_lower_half == 0){
            for (short i = 0; i < 4; i++) {
                if (both[i] < threshold_value) { both_pattern[i] = '0'; } // solved
                else if (both[i] > threshold_value) { both_pattern[i] = '1'; } // not solved
                else if (both[i] == threshold_value) { both_pattern[i] = 'e'; } // won't come up
            }
            for (short i = 4; i < 8; i++) {
                if (both[i] > threshold_value) { both_pattern[i] = '0'; } // solved
                else if (both[i] < threshold_value) { both_pattern[i] = '1'; } // not solved
                else if (both[i] == threshold_value) { both_pattern[i] = '3'; } // not solved !!!, two of them have to go down
            }
        }
    }

    // CASE D (ssst ttbb)
        if ((count_threshold_in_both_sorted == 3) && (count_below_threshold_in_both_sorted == 3)) {
            //a)
            if (count_threshold_in_both_unsorted_lower_half == 1){ // good
                for (short i = 0; i < 4; i++) {
                    if (both[i] < threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] > threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = '0'; } // solved
                }
                for (short i = 4; i < 8; i++) {
                    if (both[i] > threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] < threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = '0'; } // solved
                }
            }
            //b)
            if (count_threshold_in_both_unsorted_lower_half == 2){ // not solved
                for (short i = 0; i < 4; i++) {
                    if (both[i] < threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] > threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = '2'; } // not solved !!!, one of them has to go up
                }
                for (short i = 4; i < 8; i++) {
                    if (both[i] > threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] < threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = '0'; } // solved
                }
            }
            //c)
            if (count_threshold_in_both_unsorted_lower_half == 3){ // not solved
                for (short i = 0; i < 4; i++) {
                    if (both[i] < threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] > threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = '3'; } // not solved !!!, two of them have to go up
                }
                for (short i = 4; i < 8; i++) {
                    if (both[i] > threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] < threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = 'e'; } // won't come up
                }
            }
            //d)
            if (count_threshold_in_both_unsorted_lower_half == 0){
                for (short i = 0; i < 4; i++) {
                    if (both[i] < threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] > threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = 'e'; } // wont come up
                }
                for (short i = 4; i < 8; i++) {
                    if (both[i] > threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] < threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = '2'; } // one has to go down
                }
            }
        }

    // CASE E (sttt tbbb)
        if ((count_threshold_in_both_sorted == 4) && (count_below_threshold_in_both_sorted == 1)) {
            //a)
            if (count_threshold_in_both_unsorted_lower_half == 3){ // good
                for (short i = 0; i < 4; i++) {
                    if (both[i] < threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] > threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = '0'; } // solved
                }
                for (short i = 4; i < 8; i++) {
                    if (both[i] > threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] < threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = '0'; } // solved
                }
            }
            //b)
            if (count_threshold_in_both_unsorted_lower_half == 4){ // not solved
                for (short i = 0; i < 4; i++) {
                    if (both[i] < threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] > threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = '2'; } // not solved !!!, one has to go up
                }
                for (short i = 4; i < 8; i++) {
                    if (both[i] > threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] < threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = 'e'; } // won't come up
                }
            }
            //c)
            if (count_threshold_in_both_unsorted_lower_half == 2){ // not solved
                for (short i = 0; i < 4; i++) {
                    if (both[i] < threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] > threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = '0'; } // solved
                }
                for (short i = 4; i < 8; i++) {
                    if (both[i] > threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] < threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = '2'; } // one has to go down
                }
            }
            //d)
            if (count_threshold_in_both_unsorted_lower_half == 1){
                for (short i = 0; i < 4; i++) {
                    if (both[i] < threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] > threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = '0'; } // solved
                }
                for (short i = 4; i < 8; i++) {
                    if (both[i] > threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] < threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = '3'; } // two have to go down
                }
            }
            //e)
            if (count_threshold_in_both_unsorted_lower_half == 0){
                for (short i = 0; i < 4; i++) {
                    if (both[i] < threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] > threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = 'e'; } // won't come up
                }
                for (short i = 4; i < 8; i++) {
                    if (both[i] > threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] < threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = '4'; } // three have to go down
                }
            }
        }
    // CASE F (sstt ttbb)
        if ((count_threshold_in_both_sorted == 4) && (count_below_threshold_in_both_sorted == 2)) {
            if (count_threshold_in_both_unsorted_lower_half == 2){ // good
                for (short i = 0; i < 4; i++) {
                    if (both[i] < threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] > threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = '0'; } // solved
                }
                for (short i = 4; i < 8; i++) {
                    if (both[i] > threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] < threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = '0'; } // solved
                }
            }
            if (count_threshold_in_both_unsorted_lower_half == 0){ // not solved
                for (short i = 0; i < 4; i++) {
                    if (both[i] < threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] > threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = 'e'; } 
                }
                for (short i = 4; i < 8; i++) {
                    if (both[i] > threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] < threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = '3'; } 
                }
            }
            if (count_threshold_in_both_unsorted_lower_half == 1){ // not solved
                for (short i = 0; i < 4; i++) {
                    if (both[i] < threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] > threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = '0'; } // solved
                }
                for (short i = 4; i < 8; i++) {
                    if (both[i] > threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] < threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = '2'; } 
                }
            }
            if (count_threshold_in_both_unsorted_lower_half == 3){ // not solved
                for (short i = 0; i < 4; i++) {
                    if (both[i] < threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] > threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = '2'; }
                }
                for (short i = 4; i < 8; i++) {
                    if (both[i] > threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] < threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = '0'; } 
                }
            }
            if (count_threshold_in_both_unsorted_lower_half == 4){ // not solved
                for (short i = 0; i < 4; i++) {
                    if (both[i] < threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] > threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = '3'; } 
                }
                for (short i = 4; i < 8; i++) {
                    if (both[i] > threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] < threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = 'e'; } 
                }
            }
        }
    // CASE F (ssst tttb)
        if ((count_threshold_in_both_sorted == 4) && (count_below_threshold_in_both_sorted == 3)) {
            //a)
            if (count_threshold_in_both_unsorted_lower_half == 1){ // good
                for (short i = 0; i < 4; i++) {
                    if (both[i] < threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] > threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = '0'; } // solved
                }
                for (short i = 4; i < 8; i++) {
                    if (both[i] > threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] < threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = '0'; } // solved
                }
            }
            //b)
            if (count_threshold_in_both_unsorted_lower_half == 2){ // not solved
                for (short i = 0; i < 4; i++) {
                    if (both[i] < threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] > threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = '2'; } 
                }
                for (short i = 4; i < 8; i++) {
                    if (both[i] > threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] < threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = '0'; }
                }
            }
            //c)
            if (count_threshold_in_both_unsorted_lower_half == 0){ // not solved
                for (short i = 0; i < 4; i++) {
                    if (both[i] < threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] > threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = 'e'; }
                }
                for (short i = 4; i < 8; i++) {
                    if (both[i] > threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] < threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = '2'; } 
                }
            }
            //d)
            if (count_threshold_in_both_unsorted_lower_half == 3){ // not solved
                for (short i = 0; i < 4; i++) {
                    if (both[i] < threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] > threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = '3'; }
                }
                for (short i = 4; i < 8; i++) {
                    if (both[i] > threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] < threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = '0'; } 
                }
            }
            //e)
            if (count_threshold_in_both_unsorted_lower_half == 4){ // not solved
                for (short i = 0; i < 4; i++) {
                    if (both[i] < threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] > threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = '4'; }
                }
                for (short i = 4; i < 8; i++) {
                    if (both[i] > threshold_value) { both_pattern[i] = '0'; } // solved
                    else if (both[i] < threshold_value) { both_pattern[i] = '1'; } // not solved
                    else if (both[i] == threshold_value) { both_pattern[i] = '0'; } 
                }
            }
        }

    string result(both_pattern.begin(), both_pattern.end());
    return result;
}

string random_state(){
    //string state = "000011112222333344445555";
    string state = "WWWWOOOOYYYYGGGGRRRRBBBB";
    shuffle(state.begin(), state.end(), mt19937(random_device{}()));
    return state;
}

// FOR READING:

// A cycle is (a,b,c) with indices in 1..8
struct CsvCycle { int a, b, c; };
using CsvSequence = vector<CsvCycle>;

// Storage for all patterns
static unordered_map<string, vector<CsvSequence>> g_sequences_by_pattern;
static unordered_map<string, int>                g_minlen_by_pattern;
static bool g_table_loaded = false;

// Trim helpers
static inline void ltrim(string& s){ size_t p=0; while (p<s.size() && isspace((unsigned char)s[p])) ++p; s.erase(0,p); }
static inline void rtrim(string& s){ size_t p=s.size(); while (p>0 && isspace((unsigned char)s[p-1])) --p; s.erase(p); }
static inline string trim_copy(string s){ ltrim(s); rtrim(s); return s; }

static vector<string> split_csv_4cols(const string& line) {
    vector<string> cols;
    cols.reserve(4);
    bool in_quotes = false;
    string cur;
    for (size_t i=0;i<line.size();++i) {
        char ch = line[i];
        if (ch == '"') { in_quotes = !in_quotes; continue; }
        if (ch == ',' && !in_quotes) {
            cols.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(ch);
        }
    }
    cols.push_back(cur);
    while (cols.size() < 4) cols.push_back("");
    return cols;
}

// Parse "(x,y,z)" triples from a chunk; returns one sequence.
static CsvSequence parse_cycle_list(const string& chunkRaw) {
    CsvSequence seq;
    string s = chunkRaw;
    // strip surrounding quotes just in case
    if (!s.empty() && s.front()=='"' && s.back()=='"') s = s.substr(1, s.size()-2);

    const char* p = s.c_str();
    while (*p) {
        // find '('
        while (*p && *p!='(') ++p;
        if (!*p) break;
        ++p; // after '('
        // skip spaces
        while (*p==' '||*p=='\t') ++p;

        char* endp=nullptr;
        int a = strtol(p,&endp,10); p=endp;
        while (*p==','||*p==' '||*p=='\t') ++p;
        int b = strtol(p,&endp,10); p=endp;
        while (*p==','||*p==' '||*p=='\t') ++p;
        int c = strtol(p,&endp,10); p=endp;

        // advance to ')'
        while (*p && *p!=')') ++p;
        if (*p==')') ++p;

        if (a>=1 && a<=8 && b>=1 && b<=8 && c>=1 && c<=8)
            seq.push_back({a,b,c});
    }
    return seq;
}

// Split the "sequences" cell by '|' to multiple sequences, trimming spaces.
static vector<CsvSequence> parse_sequences_cell(const string& cellRaw) {
    vector<CsvSequence> all;
    size_t start = 0;
    while (start < cellRaw.size()) {
        size_t bar = cellRaw.find('|', start);
        string chunk = (bar==string::npos) ? cellRaw.substr(start) : cellRaw.substr(start, bar-start);
        chunk = trim_copy(chunk);
        if (!chunk.empty()) {
            auto seq = parse_cycle_list(chunk);
            if (!seq.empty()) all.push_back(std::move(seq));
        }
        if (bar==string::npos) break;
        start = bar + 1;
    }
    return all;
}

static const char* kPatternCSV = "cycle_tables_v2.csv";

static void load_pattern_table_once() {
    if (g_table_loaded) return;
    g_table_loaded = true;

    ifstream in(kPatternCSV);
    if (!in) {
        cerr << "[mapping_into_3cycles] WARNING: cannot open " << kPatternCSV << "\n";
        return;
    }

    string header; getline(in, header); // skip header
    string line;
    while (getline(in, line)) {
        if (line.empty()) continue;
        auto cols = split_csv_4cols(line);
        string pattern = trim_copy(cols[0]);
        // cols[1] (example_count) unused here
        int min_len = 0;
        try { min_len = stoi(trim_copy(cols[2])); } catch(...) { min_len = 0; }
        string sequences_cell = cols[3];

        auto seqs = parse_sequences_cell(sequences_cell);
        if (!seqs.empty()) {
            auto& dst = g_sequences_by_pattern[pattern];
            dst.insert(dst.end(), seqs.begin(), seqs.end());
            if (!g_minlen_by_pattern.count(pattern)) g_minlen_by_pattern[pattern] = min_len;
            else g_minlen_by_pattern[pattern] = std::min(g_minlen_by_pattern[pattern], min_len);
        } else {
            // even if no sequences parsed, keep min_len if present
            if (!g_minlen_by_pattern.count(pattern)) g_minlen_by_pattern[pattern] = min_len;
        }
    }
}

// Pick one sequence to apply for a pattern.
// Current policy: prefer minimal length sequences; if multiple, take the first.

// will delete this function if, I decide to only use cycle_tables with exactly one choice as algorithm.

static const CsvSequence* pick_sequence_for_pattern(const string& pattern) { // option1
    load_pattern_table_once();
    auto it = g_sequences_by_pattern.find(pattern);
    if (it == g_sequences_by_pattern.end() || it->second.empty()) return nullptr;

    // Filter to minimal length (either from file's min_len or recompute)
    int minL = INT_MAX;
    auto itMin = g_minlen_by_pattern.find(pattern);
    if (itMin != g_minlen_by_pattern.end() && itMin->second > 0) {
        minL = itMin->second;
    } else {
        for (auto& s : it->second) minL = std::min<int>(minL, (int)s.size());
        if (minL == INT_MAX) minL = 0;
    }
    for (auto& s : it->second) {
        if ((int)s.size() == minL) return &s; // first minimal
    }
    return &it->second.front(); // fallback
}

void mapping_into_3cycles(const string& pattern, int face1, int face2, int layer) {
    const CsvSequence* seq = pick_sequence_for_pattern(pattern);
    if (!seq) return;

    for (const auto& cyc : *seq) {
        if (should_defer(cyc.a, cyc.b, cyc.c, face1, face2, layer)) {
            add_cycle_deferred(cyc.a, cyc.b, cyc.c, face1, face2, layer);
        } else {
            add_cycle        (cyc.a, cyc.b, cyc.c, face1, face2, layer);
        }
    }
}


int mapPos(int pos1to8, int face1, int face2) { // Map a position 1..8 to an index in the 24-char state string,
    if (pos1to8 <= 4) { return face1 * 4 + (pos1to8 - 1);} 
    else {return face2 * 4 + (pos1to8 - 5);      
    }
}


void apply_cycles(std::string& state, const std::vector<std::vector<int>>& cycles) {
    for (size_t k = 0; k < cycles.size(); ++k) {   // Loop over all cycles
        const vector<int>& cyc = cycles[k];
        int i1 = mapPos(cyc[0], cyc[3], cyc[4]);
        int i2 = mapPos(cyc[1], cyc[3], cyc[4]);
        int i3 = mapPos(cyc[2], cyc[3], cyc[4]);

        // Perform the 3-cycle a -> b -> c
        char va = state[i1];
        char vb = state[i2];
        char vc = state[i3];

        state[i2] = va;  // old a goes to b
        state[i3] = vb;  // old b goes to c
        state[i1] = vc;  // old c goes to a
    }
}

void print_state(string state){
    cout << "State: ";
    for (size_t i = 0; i < state.size(); ++i) {
        cout << state[i];
        if ((i + 1) % 4 == 0 && i + 1 != state.size()) {
            cout << " ";
        }
    }
}

void sorting_via_sorting_network(string state){

    //ULFRBD

    for (size_t i = 0; i < 24; ++i) { 
        switch (state[i]) {
            case 'W': state[i] = '0'; break;
            case 'O': state[i] = '1'; break;
            case 'Y': state[i] = '2'; break;
            case 'G': state[i] = '3'; break;
            case 'R': state[i] = '4'; break;
            case 'B': state[i] = '5'; break;
            default: break;
        }
    }
    cout << "Numeric representation of the state:" << endl;
    print_state(state);
    cout << endl;

    layer_1.clear(); layer_1_b.clear();
    layer_2.clear(); layer_2_b.clear();
    layer_3.clear(); layer_3_b.clear();
    layer_4.clear(); layer_4_b.clear();
    layer_5.clear(); layer_5_b.clear();

    cout << endl << "Layer 1:" << endl;
    layer_1.clear();
    mapping_into_3cycles(compute_face_swap_full_orbit(state, 0, 4), 0, 4, 1);  // (0,5) → (0,4)
    mapping_into_3cycles(compute_face_swap_full_orbit(state, 1, 2), 1, 2, 1);  // (1,3) → (1,2)
    mapping_into_3cycles(compute_face_swap_full_orbit(state, 5, 3), 5, 3, 1);  // (2,4) → (5,3)

    for (const auto& cyc : layer_1)
        cout << "cycle(" << cyc[0] << "," << cyc[1] << ","  << cyc[2] << ") in blocks " 
            << cyc[3] << " and " << cyc[4] << "" << endl;
    cout << endl;
    apply_cycles(state, layer_1);
    print_state(state);

    // Wave 1b (deferred)
    cout << "\n\nWave 1b (deferred):\n";
    for (const auto& cyc : layer_1_b)
        cout << "cycle(" << cyc[0] << "," << cyc[1] << ","  << cyc[2] << ") in blocks " 
            << cyc[3] << " and " << cyc[4] << "\n";
    cout << endl;
    apply_cycles(state, layer_1_b);
    print_state(state);

    cout << endl << endl << "Layer 2:" << endl;
    layer_2.clear();   
    mapping_into_3cycles(compute_face_swap_full_orbit(state, 1, 5), 1, 5, 2);  // (1,2) → (1,5)
    mapping_into_3cycles(compute_face_swap_full_orbit(state, 2, 3), 2, 3, 2);  // (3,4) → (2,3)


    for (const auto& cyc : layer_2)
        cout << "cycle(" << cyc[0] << "," << cyc[1] << ","  << cyc[2] << ") in blocks " 
            << cyc[3] << " and " << cyc[4] << "" << endl;
    cout << endl;
    apply_cycles(state, layer_2);
    print_state(state);

    // Wave 2b (deferred)
    cout << "\n\nWave 2b (deferred):\n";
    for (const auto& cyc : layer_2_b)
        cout << "cycle(" << cyc[0] << "," << cyc[1] << ","  << cyc[2] << ") in blocks " 
            << cyc[3] << " and " << cyc[4] << "\n";
    cout << endl;
    apply_cycles(state, layer_2_b);
    print_state(state);


    cout << endl <<  endl << "Layer 3:" << endl;
    layer_3.clear();  
    mapping_into_3cycles(compute_face_swap_full_orbit(state, 0, 2), 0, 2, 3);  // (0,3) → (0,2)
    mapping_into_3cycles(compute_face_swap_full_orbit(state, 5, 4), 5, 4, 3);  // (2,5) → (5,4)

    for (const auto& cyc : layer_3)
        cout << "cycle(" << cyc[0] << "," << cyc[1] << ","  << cyc[2] << ") in blocks " 
            << cyc[3] << " and " << cyc[4] << "" << endl;
    cout << endl;
    apply_cycles(state, layer_3);
    print_state(state);

    // Wave 3b (deferred)
    cout << "\n\nWave 3b (deferred):\n";
    for (const auto& cyc : layer_3_b)
        cout << "cycle(" << cyc[0] << "," << cyc[1] << ","  << cyc[2] << ") in blocks " 
            << cyc[3] << " and " << cyc[4] << "\n";
    cout << endl;
    apply_cycles(state, layer_3_b);
    print_state(state);

    cout << endl <<  endl << "Layer 4:" << endl;
    layer_4.clear();  
    mapping_into_3cycles(compute_face_swap_full_orbit(state, 0, 1), 0, 1, 4);  // (0,1) → (0,1)
    mapping_into_3cycles(compute_face_swap_full_orbit(state, 5, 2), 5, 2, 4);  // (2,3) → (5,2)
    mapping_into_3cycles(compute_face_swap_full_orbit(state, 3, 4), 3, 4, 4);  // (4,5) → (3,4)

    for (const auto& cyc : layer_4)
        cout << "cycle(" << cyc[0] << "," << cyc[1] << ","  << cyc[2] << ") in blocks " 
            << cyc[3] << " and " << cyc[4] << "" << endl;
    cout << endl;
    apply_cycles(state, layer_4);
    print_state(state);

    // Wave 4b (deferred)
    cout << "\n\nWave 4b (deferred):\n";
    for (const auto& cyc : layer_4_b)
        cout << "cycle(" << cyc[0] << "," << cyc[1] << ","  << cyc[2] << ") in blocks " 
            << cyc[3] << " and " << cyc[4] << "\n";
    cout << endl;
    apply_cycles(state, layer_4_b);
    print_state(state);

    cout << endl <<  endl << "Layer 5:" << endl;
    layer_5.clear(); 
    mapping_into_3cycles(compute_face_swap_full_orbit(state, 1, 5), 1, 5, 5);  // (1,2) → (1,5)
    mapping_into_3cycles(compute_face_swap_full_orbit(state, 2, 3), 2, 3, 5);  // (3,4) → (2,3)

    for (const auto& cyc : layer_5)
        cout << "cycle(" << cyc[0] << "," << cyc[1] << ","  << cyc[2] << ") in blocks " 
            << cyc[3] << " and " << cyc[4] << "" << endl;
    cout << endl;
    apply_cycles(state, layer_5);
    print_state(state);
    cout << endl;


    // Wave 5b (deferred)
    cout << "\n\nWave 5b (deferred):\n";
    for (const auto& cyc : layer_5_b)
        cout << "cycle(" << cyc[0] << "," << cyc[1] << ","  << cyc[2] << ") in blocks " 
            << cyc[3] << " and " << cyc[4] << "\n";
    cout << endl;
    apply_cycles(state, layer_5_b);
    print_state(state);
    }

    static inline int abs1_index(int p, int f1, int f2) {
        // Map local 1..8 (over faces f1,f2) -> absolute 1-based index in 24-array
        return (p <= 4) ? (p + 4*f1) : (p + (4*f2 - 4));
    }
    
void translate_and_flush_into_file(int orbit1, int orbit2, const std::string& outdir) {

    ofstream o1 (outdir + "/cycles_wave1.txt",  std::ios::app);
    ofstream o1b(outdir + "/cycles_wave2.txt", std::ios::app);
    ofstream o2 (outdir + "/cycles_wave3.txt",  std::ios::app);
    ofstream o2b(outdir + "/cycles_wave4.txt", std::ios::app);
    ofstream o3 (outdir + "/cycles_wave5.txt",  std::ios::app);
    ofstream o3b(outdir + "/cycles_wave6.txt", std::ios::app);
    ofstream o4 (outdir + "/cycles_wave7.txt",  std::ios::app);
    ofstream o4b(outdir + "/cycles_wave8.txt", std::ios::app);
    ofstream o5 (outdir + "/cycles_wave9.txt",  std::ios::app);
    ofstream o5b(outdir + "/cycles_wave10.txt", std::ios::app);

    auto emit_layer = [&](ofstream& out, const vector<std::vector<int>>& L) {
        for (const auto& cyc : L) {
            int f1 = cyc[3], f2 = cyc[4];
            int p1 = abs1_index(cyc[0], f1, f2);
            int p2 = abs1_index(cyc[1], f1, f2);
            int p3 = abs1_index(cyc[2], f1, f2);
            out << "orbit(" << orbit1 << "," << orbit2 << ") - rot("
                << p1 << "," << p2 << "," << p3 << ")\n";
        }
    };

    // Wave “a”
    emit_layer(o1, layer_1);
    emit_layer(o2, layer_2);
    emit_layer(o3, layer_3);
    emit_layer(o4, layer_4);
    emit_layer(o5, layer_5);

    // Wave “b” (deferred)
    emit_layer(o1b, layer_1_b);
    emit_layer(o2b, layer_2_b);
    emit_layer(o3b, layer_3_b);
    emit_layer(o4b, layer_4_b);
    emit_layer(o5b, layer_5_b);
}


// int main() {
//     string state = random_state();
//     cout << endl << "Find 3-cycles to solve via two-face-sorting-network-method" << endl << endl;
//     print_state(state);
//     cout << endl;
//     sorting_via_sorting_network(state);
//     cout << endl;
//     translate_and_flush_into_file(1,1, ".");
// }

int main(int argc, char** argv) {
    int M = 16;
    string state_path = "cube_state.txt"; // not ideal. Should rather use a bin-file later. Will save much storage.
    string outdir = ".";

    for (int i = 1; i < argc; ++i) {
        string a = argv[i];
        if (a == "--M" && i+1 < argc) M = std::stoi(argv[++i]);
        else if (a == "--state" && i+1 < argc) state_path = argv[++i];
        else if (a == "--outdir" && i+1 < argc) outdir = argv[++i];
    }

    std::map<std::pair<int,int>, std::string> orbit_state;
    {
        std::ifstream fin(state_path);
        std::string line;
        while (std::getline(fin, line)) {
            if (line.size() && line[0] == 'o') {
                // line: "o a,b : <24 letters>"
                size_t sp = line.find(' ');
                size_t comma = line.find(',', sp+1);
                size_t colon = line.find(':', comma+1);
                int a = std::stoi(line.substr(sp+1, comma - (sp+1)));
                int b = std::stoi(line.substr(comma+1, colon - (comma+1)));
                size_t letters_pos = line.find_first_not_of(" \t", colon+1);
                std::string letters = (letters_pos == std::string::npos) ? "" : line.substr(letters_pos);
                if (letters.size() > 24) letters.resize(24);
                orbit_state[{a,b}] = letters;
            }
        }
    }

    for (int a = 1; a <= M; ++a) {
        for (int b = 1; b <= M; ++b) {
            auto it = orbit_state.find({a,b});
            if (it == orbit_state.end() || it->second.size() != 24) continue; 
            std::string state = it->second;
            sorting_via_sorting_network(state);  
            translate_and_flush_into_file(a, b, outdir);
        }
    }

    return 0;
}



