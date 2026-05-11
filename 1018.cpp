/*
LeetCode 1018. Binary Prefix Divisible By 5

Problem:
Given a binary array nums, for each index i, take the prefix nums[0..i]
as a binary number and determine whether it is divisible by 5.
Return the result as a boolean array.

Idea:
Let current be the value of the current prefix modulo 5.
When a new bit is appended, newValue = oldValue * 2 + bit.
So we only maintain:
current = (current * 2 + bit) % 5.
If current == 0, that prefix is divisible by 5.
*/

#include <iostream>
#include <vector>

using namespace std;

class Solution {
public:
    vector<bool> prefixesDivBy5(vector<int>& nums) {
        vector<bool> res;
        int current = 0;

        for (int num : nums) {
            current = (current * 2 + num) % 5;
            res.push_back(current == 0);
        }

        return res;
    }
};

int main() {
    vector<int> nums = {0, 1, 1, 1, 1, 1};

    Solution s;
    vector<bool> ans = s.prefixesDivBy5(nums);

    cout << "Input: [";
    for (size_t i = 0; i < nums.size(); ++i) {
        cout << nums[i] << (i + 1 == nums.size() ? "" : ",");
    }
    cout << "]" << '\n';

    cout << "Output: [";
    for (size_t i = 0; i < ans.size(); ++i) {
        cout << (ans[i] ? "true" : "false") << (i + 1 == ans.size() ? "" : ",");
    }
    cout << "]" << '\n';

    return 0;
}