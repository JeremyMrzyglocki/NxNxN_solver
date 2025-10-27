#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <queue>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
using namespace std;

// ---- Bring this in from your codebase ----

struct Move { int a, b, c; };  // no direction now

static void apply_cycle(array<char,8>& both, int a, int b, int c) {
    --a; --b; --c;
    char A = both[a], B = both[b], C = both[c];
    // fixed orientation: a -> b -> c
    both[b] = A; both[c] = B; both[a] = C;
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
    ///cout << "marker2\n " << both[0] << both[1] << both[2] << both[3] << both[4] << both[5] << both[6] << both[7] << "---\n";
    ///cout << "marker2\n " << both_sorted[0] << both_sorted[1] << both_sorted[2] << both_sorted[3] << both_sorted[4] << both_sorted[5] << both_sorted[6] << both_sorted[7] << "---\n";
    ///cout << "threshold value: " << threshold_value << "\n";

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
static const string ZERO8 = "00000000";

// Build a 24-char "state" whose face 0 and face 1 hold the 8 values in `both`.
// Other faces are dummy; they don't matter because your function only reads the two faces.
static string state_from_both(const array<char,8>& both) {
    string s(24, '0'); // '0' everywhere (W) is fine
    // face 0: indices 0..3; face 1: indices 4..7
    for (int i = 0; i < 4; ++i) s[i]   = char('0' + both[i]);     // lower half
    for (int i = 0; i < 4; ++i) s[4+i] = char('0' + both[4+i]);   // upper half
    return s;
}

// Compute pattern for our 8-vector by delegating to your function.
static string pattern_from_both(const array<char,8>& both) {
    string s = state_from_both(both);
    return compute_face_swap_full_orbit(s, /*face0*/0, /*face1*/1);
}

struct Node {
    array<char,8> both;
    vector<Move> path;
};

static vector<Move> all_moves() {
    vector<Move> m;

    // face id: 1..4 -> 0, 5..8 -> 1
    auto face_of = [](int p) { return (p - 1) / 4; };

    for (int a = 1; a <= 6; ++a)
        for (int b = a + 1; b <= 7; ++b)
            for (int c = b + 1; c <= 8; ++c) {
                // skip cycles whose three indices live on the same face
                if (face_of(a) == face_of(b) && face_of(b) == face_of(c)) continue;

                // keep both orientations (do NOT prune a-c-b)
                m.push_back({a, b, c}); // a -> b -> c
                m.push_back({a, c, b}); // a -> c -> b (inverse)
            }

    return m;
}



// BFS up to depth 3 to reach ZERO8. Returns empty path if none found.
static vector<Move> solve_depth3(array<char,8> start) {
    const auto MOVES = all_moves();
    // To dedupe states quickly: key is 8 chars packed into uint64
    auto key = [](const array<char,8>& v)->uint64_t {
        uint64_t k=0; for (int i=0;i<8;++i) { k = k*7 + v[i]; } return k;
    };

    unordered_set<uint64_t> vis;
    queue<Node> q;
    q.push({start,{}});
    vis.insert(key(start));

    int depth = 0;
    size_t layer_sz = 1;
    size_t next_layer = 0;

    while (!q.empty() && depth <= 3) {
        Node cur = q.front(); q.pop();
        if (pattern_from_both(cur.both) == ZERO8) return cur.path;

        if (cur.path.size()==3) continue;

        for (const auto& mv : MOVES) {
            array<char,8> nxt = cur.both;
            apply_cycle(nxt, mv.a, mv.b, mv.c);

            uint64_t k = key(nxt);
            if (vis.insert(k).second) {
                auto p = cur.path; p.push_back(mv);
                q.push({nxt, std::move(p)});
                ++next_layer;
            }
        }
        if (--layer_sz == 0) { layer_sz = next_layer; next_layer = 0; ++depth; }
    }
    return {}; // not found
}

// Enumerate all realizable patterns by scanning values in {0,1,2}^8.
// Group every 8-vector by the pattern it produces.
static void enumerate_patterns(
    unordered_map<string, vector<array<char,8>>>& buckets,
    unordered_map<string, array<char,8>>& canonical
) {
    array<char,8> v {};
    // 3^8 loops (compact mixed-radix increment)
    for (int n=0;n<6561;++n) {
        int x = n;
        for (int i=0;i<8;++i) { v[i] = x % 3; x/=3; } // values 0,1,2
        string pat = pattern_from_both(v);
        buckets[pat].push_back(v);
        if (!canonical.count(pat)) canonical[pat] = v;
    }
}

// apply cycle:



// Verify a candidate path solves *all* examples of that pattern.
static bool verify_universal(
    const vector<Move>& path,
    const vector<array<char,8>>& examples
) {
    for (const auto& ex : examples) {
        array<char,8> cur = ex;
        for (const auto& mv : path) apply_cycle(cur, mv.a, mv.b, mv.c);

        if (pattern_from_both(cur) != ZERO8) return false;
    }
    return true;
}

static string path_to_string(const vector<Move>& p) {
    stringstream ss;
    for (size_t i=0;i<p.size();++i) {
        ss << "(" << p[i].a << "," << p[i].b << "," << p[i].c << ")";
        if (i+1<p.size()) ss << " ";
    }
    return ss.str();
}

// === REPLACE your current 3-cycle catalog + solution check with this ===

// Return all unique 3-cycles on positions 1..8 with a single fixed orientation
// (a -> b -> c). This removes the "clockwise/ccw" duplication.
static vector<array<int,3>> all_three_cycles_pos() {
    vector<array<int,3>> res;

    auto all_on_same_face = [](int i, int j, int k) {
        // face 0 => positions 1..4, face 1 => positions 5..8
        auto face = [](int p) { return (p - 1) / 4; };
        int fi = face(i), fj = face(j), fk = face(k);
        return (fi == fj) && (fj == fk);
    };

    for (int a = 1; a <= 6; ++a)
        for (int b = a + 1; b <= 7; ++b)
            for (int c = b + 1; c <= 8; ++c) {
                if (all_on_same_face(a, b, c)) continue;   // skip 1..4-only or 5..8-only

                // keep both orientations so we don't prune a->c->b away
                res.push_back({a, b, c});  // a -> b -> c
                res.push_back({a, c, b});  // a -> c -> b (inverse)
            }

    return res;
}
static bool all_zero(const string& s) {
    for (char ch : s) if (ch != '0') return false;
    return true;
}

// Map pos 1..8 -> absolute index in 24-char state for (face1, face2)
static inline int face_idx4(int face, int off4) { return face * 4 + off4; }

static void apply_3cycle_on_state(std::string& s, int face1, int face2,
                                  int a, int b, int c) {
    auto idx = [&](int p)->int {
        --p; // 0-based in the 8-way "both" view
        return (p < 4) ? face_idx4(face1, p) : face_idx4(face2, p - 4);
    };
    int ia = idx(a), ib = idx(b), ic = idx(c);
    char A = s[ia], B = s[ib], C = s[ic];
    // fixed orientation: a -> b -> c
    s[ib] = A; s[ic] = B; s[ia] = C;
}


static string pattern_after_cycles(const string& input,
                                   int face1, int face2,
                                   const vector<array<int,3>>& cycles)
{
    string state = input;
    for (auto t : cycles) {
        apply_3cycle_on_state(state, face1, face2, t[0], t[1], t[2]);
    }
    return compute_face_swap_full_orbit(state, face1, face2);
}



// A set of cycles is valid iff:
// 1) It solves the pattern when applied in the given order, and
// 2) If there are 2+ cycles, EVERY permutation (i.e., any swap of two cycles)
//    also solves the SAME input pattern.
static bool solves_and_is_order_invariant(const string& input,
                                          int face1, int face2,
                                          vector<array<int,3>> cycles)
{
    if (cycles.empty()) return false;

    // Check the provided order first.
    if (!all_zero(pattern_after_cycles(input, face1, face2, cycles))) return false;

    if (cycles.size() == 1) return true;

    // Check all permutations (covers all pairwise swaps).
    sort(cycles.begin(), cycles.end()); // deterministic start for next_permutation
    do {
        if (!all_zero(pattern_after_cycles(input, face1, face2, cycles))) {
            return false;
        }
    } while (next_permutation(cycles.begin(), cycles.end()));

    return true;
}
// ----- ALL-SEQUENCES BRUTE FORCE (length 1..3), with order-invariance -----

static string move_key(const Move& m) {
    string s;
    s.reserve(8);
    s.push_back('(');
    s += to_string(m.a); s.push_back(',');
    s += to_string(m.b); s.push_back(',');
    s += to_string(m.c); s.push_back(')');
    return s;
}

static string seq_to_string(const vector<Move>& p) {
    string out;
    for (size_t i=0;i<p.size();++i) {
        if (i) out.push_back(' ');
        out += move_key(p[i]);
    }
    return out;
}

// Apply a sequence to an example and check if it solves to ZERO8
static inline bool sequence_solves_example(const vector<Move>& seq, array<char,8> ex) {
    for (const auto& mv : seq) apply_cycle(ex, mv.a, mv.b, mv.c);
    return pattern_from_both(ex) == ZERO8;
}

// True iff seq solves **all** examples
static bool sequence_solves_all(const vector<Move>& seq,
                                const vector<array<char,8>>& examples) {
    for (const auto& ex : examples) if (!sequence_solves_example(seq, ex)) return false;
    return true;
}

// Check the “swap order still solves the SAME pattern” rule.
// For len>=2 we require that **every permutation** of the cycles solves all examples.
static bool order_invariant_solution(const vector<Move>& seq,
                                     const vector<array<char,8>>& examples) {
    if (!sequence_solves_all(seq, examples)) return false;
    if (seq.size() < 2) return true;

    // Enumerate permutations by permuting indices (handles duplicate moves too).
    vector<int> idx(seq.size());
    iota(idx.begin(), idx.end(), 0);
    do {
        vector<Move> perm;
        perm.reserve(seq.size());
        for (int id : idx) perm.push_back(seq[id]);
        if (!sequence_solves_all(perm, examples)) return false;
    } while (next_permutation(idx.begin(), idx.end()));
    return true;
}

// Canonical (orderless) key for a set of cycles
static string canonical_set_key(const vector<Move>& seq) {
    vector<string> ks; ks.reserve(seq.size());
    for (auto& m : seq) ks.push_back(move_key(m));
    sort(ks.begin(), ks.end());
    string out;
    for (size_t i=0;i<ks.size();++i) {
        if (i) out.push_back(';');
        out += ks[i];
    }
    return out;
}

// Count of unique permutations for a multiset (len <= 3)
static int unique_permutation_count(const vector<Move>& seq) {
    auto eq = [](const Move& x, const Move& y){
        return x.a==y.a && x.b==y.b && x.c==y.c;
    };
    int n = (int)seq.size();
    if (n <= 1) return 1;
    // multiplicities
    vector<Move> seen;
    vector<int> cnt;
    for (const auto& m : seq) {
        bool found=false;
        for (size_t i=0;i<seen.size();++i) if (eq(seen[i], m)) {
            ++cnt[i]; found=true; break;
        }
        if (!found) { seen.push_back(m); cnt.push_back(1); }
    }
    auto fact = [](int k){ return (k<=1)?1:(k==2?2:6); }; // only up to 3
    int denom = 1;
    for (int c : cnt) denom *= fact(c);
    return fact(n) / denom;
}

// Generate all length-1..3 sequences that satisfy the rule, for a pattern bucket
static vector<vector<Move>> find_all_sequences_for_pattern(
        const vector<array<char,8>>& examples) {
    const auto MOVES = all_moves();
    vector<vector<Move>> solutions;

    vector<Move> cur;
    cur.reserve(3);

    // DFS over lengths 1..3 (no pruning; we test at each non-empty depth)
    function<void(int)> dfs = [&](int depth){
        if (depth > 0) {
            if (order_invariant_solution(cur, examples))
                solutions.push_back(cur);
        }
        if (depth == 4) return;

        for (const auto& mv : MOVES) {
            cur.push_back(mv);
            dfs(depth + 1);
            cur.pop_back();
        }
    };
    dfs(0);
    return solutions;
}


int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    unordered_map<string, vector<array<char,8>>> buckets;
    unordered_map<string, array<char,8>> canonical;
    enumerate_patterns(buckets, canonical);

    ofstream okcsv("bf_patterns_solutions.csv");
    okcsv << "pattern,example_count,solution_len,cycles\n";
    ofstream badcsv("bf_patterns_unsolved.csv");
    badcsv << "pattern,example_count,counterexample_both\n";

    int solved = 0, unsolved = 0;

    // Sort patterns for stable output// Sort patterns for stable output
    // Sort patterns for stable output
    vector<string> pats; pats.reserve(canonical.size());
    for (auto& kv : canonical) pats.push_back(kv.first);
    sort(pats.begin(), pats.end());

    // New single-file, one-line-per-pattern format
    ofstream out("bf_patterns_sequences_by_pattern.csv");
    out << "pattern,example_count,min_len,sequences\n";

    int patterns_with_solutions = 0;
    int patterns_without = 0;

    for (const string& pat : pats) {
        const auto& exs = buckets[pat];

        // Brute-force every sequence of 1..3 cycles that solves the SAME pattern
        auto sequences = find_all_sequences_for_pattern(exs);

        // Bucket by length and stringify, so we can order by depth
        vector<string> by_len[4]; // use indices 1..3
        for (const auto& s : sequences) {
            by_len[s.size()].push_back(seq_to_string(s));
        }
        // Sort lexicographically inside each depth
        for (int L = 1; L <= 3; ++L) sort(by_len[L].begin(), by_len[L].end());

        // Compute minimal length (blank if none)
        int min_len = 0;
        for (int L = 1; L <= 3; ++L) if (!by_len[L].empty()) { min_len = L; break; }

        // Join sequences in the requested order: len=1, then len=2, then len=3
        std::ostringstream seqs;
        bool first = true;
        for (int L = 1; L <= 3; ++L) {
            for (const auto& s : by_len[L]) {
                if (!first) seqs << " | ";
                seqs << s;
                first = false;
            }
        }

        if (min_len) ++patterns_with_solutions; else ++patterns_without;

        out << pat << "," << exs.size() << ",";
        if (min_len) out << min_len;         // print number
        // if none, leave the cell empty
        out << ",\"" << seqs.str() << "\"\n";
    }

    cout << "Total unique patterns: " << canonical.size() << "\n";
    cout << "Patterns with >=1 solving sequence (len<=3): " << patterns_with_solutions << "\n";
    cout << "Patterns with no solving sequence (len<=3): " << patterns_without << "\n";
    cout << "Wrote bf_patterns_sequences_by_pattern.csv\n";

    return 0;
}
