#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <vector>

using namespace std;

/*
LeetCode 1722. Minimize Hamming Distance After Swap Operations

给你两个长度相等的整数数组 source 和 target，以及一个二维数组 allowedSwaps。
在允许交换的下标对之间，可以任意多次交换 source 中的元素，返回通过这些交换后，
source 和 target 的最小汉明距离。

示例：
source = [1,2,3,4], target = [2,1,4,5], allowedSwaps = [[0,1],[2,3]]
输出：1

思路：
使用并查集将可互相交换的下标分到同一个连通分量中，分别统计每个连通分量中
source 和 target 的元素频率，求出最多能匹配上的元素个数。
*/

class Solution {
public:
    int minimumHammingDistance(vector<int>& source, vector<int>& target, vector<vector<int>>& allowedSwaps) {
        int n = source.size();
        vector<int> parent(n);
        for (int i = 0; i < n; i++) parent[i] = i;

        auto find = [&](auto&& self, int x) -> int {
            if (parent[x] != x) parent[x] = self(self, parent[x]);
            return parent[x];
        };

        for (auto& p : allowedSwaps) {
            int a = find(find, p[0]);
            int b = find(find, p[1]);
            if (a != b) parent[b] = a;
        }

        unordered_map<int, unordered_map<int, int>> src, tgt;
        for (int i = 0; i < n; i++) {
            int root = find(find, i);
            src[root][source[i]]++;
            tgt[root][target[i]]++;
        }

        int same = 0;
        for (auto& [root, mp] : src) {
            for (auto& [val, cnt] : mp) {
                same += min(cnt, tgt[root][val]);
            }
        }

        return n - same;
    }
};

int main() {
    Solution s;

    vector<int> source = {1, 2, 3, 4};
    vector<int> target = {2, 1, 4, 5};
    vector<vector<int>> allowedSwaps = {{0, 1}, {2, 3}};

    int res = s.minimumHammingDistance(source, target, allowedSwaps);
    cout << res << '\n';

    return 0;
}
