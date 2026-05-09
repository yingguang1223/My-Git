#include<vector>
#include<iostream>
using namespace std;

class Solution{
    public:
    vector<int>getConcatenation(vector<int>&nums)
    {
        int n=nums.size();
        vector<int>ans(2*n);
        for(int i=0;i<n;i++)
        {
            ans[i]=nums[i];
            ans[i+n]=nums[i];
        }
        return ans;
    }
};

int main()
{
    Solution s;
    vector<int>nums={1,2,1};
    vector<int>res=s.getConcatenation(nums);
    for(int i=0;i<res.size();i++)
    {
        cout<<res[i]<<" ";
    }
    return 0;
}

/*
LeetCode 1929: Concatenation of Array

Given a 0-indexed integer array nums, construct a 2n-length array ans such that:
ans[i] = nums[i]
ans[i + n] = nums[i]

Example 1:
nums = [1,2,1]
output = [1,2,1,1,2,1]

Example 2:
nums = [1,3,2,1]
output = [1,3,2,1,1,3,2,1]

Constraints:
n == nums.length
1 <= n <= 1000
1 <= nums[i] <= 1000
*/