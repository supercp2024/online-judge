#include <iostream>
#include <vector>
#include <string>

int main(int argc, char* argv[])
{
    int num = 0;
    std::string arg = argv[1];
    unsigned int size = std::stoull(arg);
    std::vector<int> nums;
    for (unsigned int i = 0; i < size; ++i)
    {
        std::cin >> num;
        nums.push_back(num);
    }
    std::cin >> num;
    Solution sol;
    auto ret = sol.twoSum(nums, num);
    for (unsigned int i = 0; i < ret.size() - 1; ++i)
    {
        std::cout << ret[i] << ' ';
    }
    std::cout << ret[ret.size() - 1];
}