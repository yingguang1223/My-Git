// 题目：3512. 使数组元素和能被 k 整除的最少操作数
// 思路：数组总和为 sum，最少需要执行 sum % k 次操作。

#include <bits/stdc++.h>
using namespace std;

class Solution {
public:
    int minOperations(vector<int>& nums, int k) {
        int sum = accumulate(nums.begin(), nums.end(), 0);
        return sum % k;
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n, k;
    cin >> n >> k;

    vector<int> nums(n);
    for (int i = 0; i < n; ++i) {
        cin >> nums[i];
    }

    Solution sol;
    cout << sol.minOperations(nums, k) << '\n';
    return 0;
}