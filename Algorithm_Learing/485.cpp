#include<algorithm>
#include<vector>
#include<iostream>
using namespace std;
class Solution {
public:
    int findMaxConsecutiveOnes(vector<int>& nums) {
        vector<int>tem;
        tem.push_back(-1);
        for(int i=0;i<nums.size();i++)
        {if(nums[i]==0)
        tem.push_back(i);}
        int max_len=0;
        if(!tem.empty())max_len=tem[0];
        else max_len=nums.size();
        for(int j=1;j<tem.size();j++)
        {
            if(max_len<tem[j]-1-tem[j-1])max_len=tem[j]-1-tem[j-1];
        }
        if(!tem.empty())max_len=max(max_len,(int)nums.size()-tem.back()-1);
        return max_len;
    }

};

int main()
{
    Solution s;
    vector<int>nums={1,1,0,1,1,1};
    int res=s.findMaxConsecutiveOnes(nums);
    cout<<res;
    return 0;
}
/*
LeetCode 485: Max Consecutive Ones

Given a binary array nums, return the maximum number of consecutive 1s.

Example 1:
nums = [1,1,0,1,1,1]
output = 3

Example 2:
nums = [1,0,1,1,0,1]
output = 2

Constraints:
1 <= nums.length <= 10^5
nums[i] is either 0 or 1.
*/