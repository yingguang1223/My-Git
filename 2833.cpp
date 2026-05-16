/*
LeetCode 2833. Furthest Point From Origin

Problem:
You are given a string moves consisting of 'L', 'R', and '_'.
Each 'L' means move one step left, each 'R' means move one step right,
and each '_' can be chosen as either 'L' or 'R'.
Return the maximum possible distance from the origin after all moves.

Idea:
Count how many left moves, right moves, and free moves we have.
To maximize the final distance, assign every free move to the same side
so the absolute distance becomes as large as possible.
*/

#include <algorithm>
#include <iostream>
#include <string>

using namespace std;

class Solution {
public:
    int furthestDistanceFromOrigin(string moves) {
        int left = 0, right = 0, free = 0;
        for (char c : moves) {
            if (c == 'L') {
                left++;
            } else if (c == 'R') {
                right++;
            } else { // '_'
                free++;
            }
        }
        // Two extreme cases
        int case1 = (right + free) - left; // All free moves go right
        int case2 = right - (left + free); // All free moves go left
        // Take the larger absolute distance
        return max(abs(case1), abs(case2));
    }
};

int main() {
    Solution s;

    string moves = "L_RL__R";
    int ans = s.furthestDistanceFromOrigin(moves);

    cout << "Input: " << moves << '\n';
    cout << "Output: " << ans << '\n';

    return 0;
}