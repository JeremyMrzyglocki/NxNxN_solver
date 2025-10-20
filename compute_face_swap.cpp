#include <stdlib.h>
#include <string.h>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>
#include <random>

using namespace std;

// "both" refers to the concatenated arrays of the two faces being swapped, maybe I should rename it to "concatenated"

vector<vector<int>> layer_1;
vector<vector<int>> layer_2;
vector<vector<int>> layer_3;
vector<vector<int>> layer_4;
vector<vector<int>> layer_5;

inline void add_cycle(int a, int b, int c, int d, int e, int layer) { // d and e are the face-indices
    if (layer == 1){
        layer_1.push_back({a, b, c, d, e});
    } 
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

string random_state(){
    string state = "000011112222333344445555";
    shuffle(state.begin(), state.end(), std::mt19937(std::random_device{}()));
    return state;
}



void mapping_into_3cycles(const string& pattern, int face1, int face2, int layer){ {

    if (pattern == "00000000"){}
    else if (pattern == "10001000"){add_cycle(1,5,2, face1, face2, layer);}
    else if (pattern == "10000100"){add_cycle(1,6,2, face1, face2, layer);}
    else if (pattern == "10000010"){add_cycle(1,7,2, face1, face2, layer);}
    else if (pattern == "10000001"){add_cycle(1,8,2, face1, face2, layer);}
    else if (pattern == "01001000"){add_cycle(2,5,1, face1, face2, layer);}  // I am here chosing to use the inverse of the first case, because I have free choice anyways and to be able to abuse some modulo-3 tricks later on
    else if (pattern == "01000100"){add_cycle(2,6,1, face1, face2, layer);}
    else if (pattern == "01000010"){add_cycle(2,7,1, face1, face2, layer);}
    else if (pattern == "01000001"){add_cycle(2,8,1, face1, face2, layer);}
    else if (pattern == "00101000"){add_cycle(3,5,1, face1, face2, layer);}
    else if (pattern == "00100100"){add_cycle(3,6,1, face1, face2, layer);}
    else if (pattern == "00100010"){add_cycle(3,7,1, face1, face2, layer);}
    else if (pattern == "00100001"){add_cycle(3,8,1, face1, face2, layer);}
    else if (pattern == "00011000"){add_cycle(4,5,1, face1, face2, layer);}
    else if (pattern == "00010100"){add_cycle(4,6,1, face1, face2, layer);}
    else if (pattern == "00010010"){add_cycle(4,7,1, face1, face2, layer);}
    else if (pattern == "00010001"){add_cycle(4,8,1, face1, face2, layer);}

    else if (pattern == "11001100"){add_cycle(1,5,3, face1, face2, layer), add_cycle(2,6,4, face1, face2, layer);} // we take 3 and 4 as cycle-independant buffers
    else if (pattern == "11001010"){add_cycle(1,5,3, face1, face2, layer), add_cycle(2,7,4, face1, face2, layer);}
    else if (pattern == "11001001"){add_cycle(1,5,3, face1, face2, layer), add_cycle(2,8,4, face1, face2, layer);}
    else if (pattern == "11000110"){add_cycle(1,6,3, face1, face2, layer), add_cycle(2,7,4, face1, face2, layer);}
    else if (pattern == "11000101"){add_cycle(1,6,3, face1, face2, layer), add_cycle(2,8,4, face1, face2, layer);}
    else if (pattern == "11000011"){add_cycle(1,7,3, face1, face2, layer), add_cycle(2,8,4, face1, face2, layer);}
    else if (pattern == "10101100"){add_cycle(1,5,2, face1, face2, layer), add_cycle(3,6,4, face1, face2, layer);}
    else if (pattern == "10101010"){add_cycle(1,5,2, face1, face2, layer), add_cycle(3,7,4, face1, face2, layer);}
    else if (pattern == "10101001"){add_cycle(1,5,2, face1, face2, layer), add_cycle(3,8,4, face1, face2, layer);}
    else if (pattern == "10100110"){add_cycle(1,6,2, face1, face2, layer), add_cycle(3,7,4, face1, face2, layer);}
    else if (pattern == "10100101"){add_cycle(1,6,2, face1, face2, layer), add_cycle(3,8,4, face1, face2, layer);}
    else if (pattern == "10100011"){add_cycle(1,7,2, face1, face2, layer), add_cycle(3,8,4, face1, face2, layer);}
    else if (pattern == "10011100"){add_cycle(1,5,2, face1, face2, layer), add_cycle(4,6,3, face1, face2, layer);}
    else if (pattern == "10011010"){add_cycle(1,5,2, face1, face2, layer), add_cycle(4,7,3, face1, face2, layer);}
    else if (pattern == "10011001"){add_cycle(1,5,2, face1, face2, layer), add_cycle(4,8,3, face1, face2, layer);}
    else if (pattern == "10010110"){add_cycle(1,6,2, face1, face2, layer), add_cycle(4,7,3, face1, face2, layer);}
    else if (pattern == "10010101"){add_cycle(1,6,2, face1, face2, layer), add_cycle(4,8,3, face1, face2, layer);}
    else if (pattern == "10010011"){add_cycle(1,7,2, face1, face2, layer), add_cycle(4,8,3, face1, face2, layer);}
    else if (pattern == "01101100"){add_cycle(2,5,1, face1, face2, layer), add_cycle(3,6,4, face1, face2, layer);}
    else if (pattern == "01101010"){add_cycle(2,5,1, face1, face2, layer), add_cycle(3,7,4, face1, face2, layer);}
    else if (pattern == "01101001"){add_cycle(2,5,1, face1, face2, layer), add_cycle(3,8,4, face1, face2, layer);}
    else if (pattern == "01100110"){add_cycle(2,6,1, face1, face2, layer), add_cycle(3,7,4, face1, face2, layer);}
    else if (pattern == "01100101"){add_cycle(2,6,1, face1, face2, layer), add_cycle(3,8,4, face1, face2, layer);}
    else if (pattern == "01100011"){add_cycle(2,7,1, face1, face2, layer), add_cycle(3,8,4, face1, face2, layer);}
    else if (pattern == "01011100"){add_cycle(2,5,1, face1, face2, layer), add_cycle(4,6,3, face1, face2, layer);}
    else if (pattern == "01011010"){add_cycle(2,5,1, face1, face2, layer), add_cycle(4,7,3, face1, face2, layer);}
    else if (pattern == "01011001"){add_cycle(2,5,1, face1, face2, layer), add_cycle(4,8,3, face1, face2, layer);}
    else if (pattern == "01010110"){add_cycle(2,6,1, face1, face2, layer), add_cycle(4,7,3, face1, face2, layer);}
    else if (pattern == "01010101"){add_cycle(2,6,1, face1, face2, layer), add_cycle(4,8,3, face1, face2, layer);}
    else if (pattern == "01010011"){add_cycle(2,7,1, face1, face2, layer), add_cycle(4,8,3, face1, face2, layer);}
    else if (pattern == "00111100"){add_cycle(3,5,1, face1, face2, layer), add_cycle(4,6,2, face1, face2, layer);}
    else if (pattern == "00111010"){add_cycle(3,5,1, face1, face2, layer), add_cycle(4,7,2, face1, face2, layer);}
    else if (pattern == "00111001"){add_cycle(3,5,1, face1, face2, layer), add_cycle(4,8,2, face1, face2, layer);}
    else if (pattern == "00110110"){add_cycle(3,6,1, face1, face2, layer), add_cycle(4,7,2, face1, face2, layer);}
    else if (pattern == "00110101"){add_cycle(3,6,1, face1, face2, layer), add_cycle(4,8,2, face1, face2, layer);}
    else if (pattern == "00110011"){add_cycle(3,7,1, face1, face2, layer), add_cycle(4,8,2, face1, face2, layer);}

    else if (pattern == "10002200"){add_cycle(1,5,2, face1, face2, layer);}
    else if (pattern == "10002020"){add_cycle(1,5,2, face1, face2, layer);}
    else if (pattern == "10002002"){add_cycle(1,5,2, face1, face2, layer);}
    else if (pattern == "01002200"){add_cycle(2,5,1, face1, face2, layer);}
    else if (pattern == "01002020"){add_cycle(2,5,1, face1, face2, layer);}
    else if (pattern == "01002002"){add_cycle(2,5,1, face1, face2, layer);}
    else if (pattern == "00102200"){add_cycle(3,5,1, face1, face2, layer);}
    else if (pattern == "00102020"){add_cycle(3,5,1, face1, face2, layer);}
    else if (pattern == "00102002"){add_cycle(3,5,1, face1, face2, layer);}
    else if (pattern == "00012200"){add_cycle(4,5,1, face1, face2, layer);}
    else if (pattern == "00012020"){add_cycle(4,5,1, face1, face2, layer);}
    else if (pattern == "00012002"){add_cycle(4,5,1, face1, face2, layer);}
    else if (pattern == "10000220"){add_cycle(1,6,2, face1, face2, layer);}
    else if (pattern == "10000202"){add_cycle(1,6,2, face1, face2, layer);}
    else if (pattern == "10000022"){add_cycle(1,7,2, face1, face2, layer);}
    else if (pattern == "01000220"){add_cycle(2,6,1, face1, face2, layer);}
    else if (pattern == "01000202"){add_cycle(2,6,1, face1, face2, layer);}
    else if (pattern == "01000022"){add_cycle(2,7,1, face1, face2, layer);}
    else if (pattern == "00100220"){add_cycle(3,6,1, face1, face2, layer);}
    else if (pattern == "00100202"){add_cycle(3,6,1, face1, face2, layer);}
    else if (pattern == "00100022"){add_cycle(3,7,1, face1, face2, layer);}
    else if (pattern == "00010220"){add_cycle(4,6,1, face1, face2, layer);}
    else if (pattern == "00010202"){add_cycle(4,6,1, face1, face2, layer);}
    else if (pattern == "00010022"){add_cycle(4,7,1, face1, face2, layer);}
    else if (pattern == "22001000"){add_cycle(1,5,2, face1, face2, layer);}
    else if (pattern == "20201000"){add_cycle(1,5,2, face1, face2, layer);}
    else if (pattern == "20021000"){add_cycle(1,5,2, face1, face2, layer);}
    else if (pattern == "22000100"){add_cycle(1,6,2, face1, face2, layer);}
    else if (pattern == "20200100"){add_cycle(1,6,2, face1, face2, layer);}
    else if (pattern == "20020100"){add_cycle(1,6,2, face1, face2, layer);}
    else if (pattern == "22000010"){add_cycle(1,7,2, face1, face2, layer);}
    else if (pattern == "20200010"){add_cycle(1,7,2, face1, face2, layer);}
    else if (pattern == "20020010"){add_cycle(1,7,2, face1, face2, layer);}
    else if (pattern == "22000001"){add_cycle(1,8,2, face1, face2, layer);}
    else if (pattern == "20200001"){add_cycle(1,8,2, face1, face2, layer);}
    else if (pattern == "20020001"){add_cycle(1,8,2, face1, face2, layer);}
    else if (pattern == "02201000"){add_cycle(2,5,1, face1, face2, layer);}
    else if (pattern == "02021000"){add_cycle(2,5,1, face1, face2, layer);}
    else if (pattern == "00221000"){add_cycle(3,5,1, face1, face2, layer);}
    else if (pattern == "02200100"){add_cycle(2,6,1, face1, face2, layer);}
    else if (pattern == "02020100"){add_cycle(2,6,1, face1, face2, layer);}
    else if (pattern == "00220100"){add_cycle(3,6,1, face1, face2, layer);}
    else if (pattern == "02200010"){add_cycle(2,7,1, face1, face2, layer);}
    else if (pattern == "02020010"){add_cycle(2,7,1, face1, face2, layer);}
    else if (pattern == "00220010"){add_cycle(3,7,1, face1, face2, layer);}
    else if (pattern == "02200001"){add_cycle(2,8,1, face1, face2, layer);}
    else if (pattern == "02020001"){add_cycle(2,8,1, face1, face2, layer);}
    else if (pattern == "00220001"){add_cycle(3,8,1, face1, face2, layer);}

    else {
        cout << "\nPattern not recognized: " << pattern << endl;
    }
    

    //if (pattern == "11101110"){return string("arot(1,5,4) + arot(2,6,1) arot(3,7,4)");}

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
    cout << "State:";
    for (size_t i = 0; i < state.size(); ++i) {
        cout << state[i];
        if ((i + 1) % 4 == 0 && i + 1 != state.size()) {
            cout << " ";
        }
    }
}

void sorting_via_sorting_network(string state){
    cout << endl << endl << "Layer 1:" << endl;
    layer_1.clear();   
    mapping_into_3cycles(compute_face_swap_full_orbit(state, 0, 5), 0, 5, 1);
    mapping_into_3cycles(compute_face_swap_full_orbit(state, 1, 3), 1, 3, 1);
    mapping_into_3cycles(compute_face_swap_full_orbit(state, 2, 4), 2, 4, 1);

    for (const auto& cyc : layer_1)
        cout << "cycle(" << cyc[0] << "," << cyc[1] << ","  << cyc[2] << ") in blocks " << cyc[3] << " and " << cyc[4] << "" << endl;
    cout << endl;
    apply_cycles(state, layer_1);
    print_state(state);


    cout << endl << endl << "Layer 2:" << endl;
    layer_2.clear();   
    mapping_into_3cycles(compute_face_swap_full_orbit(state, 1, 2), 1, 2, 2);
    mapping_into_3cycles(compute_face_swap_full_orbit(state, 3, 4), 3, 4, 2);

    for (const auto& cyc : layer_2)
        cout << "cycle(" << cyc[0] << "," << cyc[1] << ","  << cyc[2] << ") in blocks " << cyc[3] << " and " << cyc[4] << "" << endl;
    cout << endl;
    apply_cycles(state, layer_2);
    print_state(state);


    cout << endl <<  endl << "Layer 3:" << endl;
    layer_3.clear();   
    mapping_into_3cycles(compute_face_swap_full_orbit(state, 0, 3), 0, 3, 3);
    mapping_into_3cycles(compute_face_swap_full_orbit(state, 2, 5), 2, 5, 3);

    for (const auto& cyc : layer_3)
        cout << "cycle(" << cyc[0] << "," << cyc[1] << ","  << cyc[2] << ") in blocks " << cyc[3] << " and " << cyc[4] << "" << endl;
    cout << endl;
    apply_cycles(state, layer_3);
    print_state(state);



    cout << endl <<  endl << "Layer 4:" << endl;
    layer_4.clear();   
    mapping_into_3cycles(compute_face_swap_full_orbit(state, 0, 1), 0, 1, 4);
    mapping_into_3cycles(compute_face_swap_full_orbit(state, 2, 3), 2, 3, 4);
    mapping_into_3cycles(compute_face_swap_full_orbit(state, 4, 5), 4, 5, 4);

    for (const auto& cyc : layer_4)
        cout << "cycle(" << cyc[0] << "," << cyc[1] << ","  << cyc[2] << ") in blocks " << cyc[3] << " and " << cyc[4] << "" << endl;
    cout << endl;
    apply_cycles(state, layer_4);
    print_state(state);

    
    cout << endl <<  endl << "Layer 5:" << endl;
    layer_5.clear();   
    mapping_into_3cycles(compute_face_swap_full_orbit(state, 1, 2), 1, 2, 5);
    mapping_into_3cycles(compute_face_swap_full_orbit(state, 3, 4), 3, 4, 5);

    for (const auto& cyc : layer_5)
        cout << "cycle(" << cyc[0] << "," << cyc[1] << ","  << cyc[2] << ") in blocks " << cyc[3] << " and " << cyc[4] << "" << endl;
    cout << endl;
    apply_cycles(state, layer_5);
    print_state(state);
    cout << endl;

}

int main() {
    //string state = "314532423403512051412005";
    //string state = random_state();
    string state = "012353040242541131435205";


    cout << endl << "Find 3-cycles to solve via two-face-sorting-network-method" << endl << endl;
    print_state(state);
    cout << endl;
    sorting_via_sorting_network(state);
    cout << endl;
}

// g++ -std=c++17 compute_face_swap.cpp

// #include <iomanip>

// int main() {
//     const int NUM_SAMPLES = 100000;  // number of random states to test
//     const pair<int,int> faces = {0, 5}; // which face pair to analyze

//     unordered_map<string, int> histogram;
//     mt19937 rng(random_device{}());

//     for (int i = 0; i < NUM_SAMPLES; ++i) {
//         string state = random_state();
//         string pattern = compute_face_swap_full_orbit(state, faces.first, faces.second);
//         histogram[pattern]++;
//     }

//     // Convert map to vector for sorting
//     vector<pair<string, int>> freq(histogram.begin(), histogram.end());
//     sort(freq.begin(), freq.end(),
//          [](auto &a, auto &b){ return b.second < a.second; });

//     cout << "\nHistogram of 8-digit patterns for faces ("
//          << faces.first << "," << faces.second << ") over "
//          << NUM_SAMPLES << " random states:\n\n";

//     // Determine scaling for bars
//     int maxCount = freq.empty() ? 1 : freq.front().second;
//     const int MAX_BAR = 60;

//     for (auto &[pattern, count] : freq) {
//         int barLen = static_cast<int>(round((double)count / maxCount * MAX_BAR));
//         cout << setw(10) << pattern << " | "
//              << setw(8) << count << " "
//              << string(barLen, '#') << '\n';
//     }

//     cout << "\nTotal unique patterns: " << freq.size() << endl;
//     return 0;
// }




