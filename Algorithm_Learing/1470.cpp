#include <vector>
#include <iostream>
using namespace std;
class Solution {
public:
    vector<int> shuffle(vector<int>& nums, int n) {
        vector<int>ans(2*n);
        for(int i=0;i<n;i++)
        {
            ans[2*i]=nums[i];
            ans[2*i+1]=nums[i+n];
        }
        return ans;
    }
};

int main()
{
    Solution s;
    vector<int>nums={2,5,1,3,4,7};
    int n=3;
    vector<int>res=s.shuffle(nums,n);
    for(int i=0;i<res.size();i++)
    {
        cout<<res[i]<<" ";
    }
}
/*
LeetCode 1470: Shuffle the Array

Given nums in the form [x1,x2,...,xn,y1,y2,...,yn],
rearrange it to [x1,y1,x2,y2,...,xn,yn].

Example:
nums = [2,5,1,3,4,7], n = 3
output = [2,3,5,4,1,7]

Constraints:
1 <= n <= 500
nums.length == 2 * n
1 <= nums[i] <= 1000
*/