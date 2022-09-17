//测试各种topK算法
//1 实现快排
//2 实现随机化快排
//3 实现快速选择算法
//4 实现随机化快速选择算法
//5 实现简单版本media of medias算法
//6 实现复杂版本media of medias算法
//7 测试和nth_element性能区别

#include "/root/env/snippets/cpp/cpp_test_common.h"

//helper
inline void print(const vector<int> &nums, uint64_t start = 0, uint64_t end = 0) {
#ifndef LOG_TAG
    return;
#endif
    for (uint64_t i = 0;i < nums.size();i++) {
        auto &num = nums[i];
        if (start == 0 && end == 0) {
            cout << num << " ";
        }else {
            if (i >= start && i <= end) {
                cout << num << " ";
            }
        }
    }
    cout << endl;
}
vector<int> getRandomVector() {
    //100w数据，3.8MB
    int testCount = 1000000;
    vector<int> nums(testCount, 0);
    for (int i = 0;i < testCount;i++) {
        nums[i] = rand() % INT_MAX;
    }
    return nums;
}
vector<int> getOrderedVector() {
    //1w数据，0.038MB
    int testCount = 10000;
    vector<int> nums(testCount, 0);
    for (int i = 0;i < testCount;i++) {
        nums[i] = i;
    }
    return nums;
}
void testFuncOk(const string &testName, const function<void(vector<int>&)> &testFunc,
    const function<void(vector<int>&)> &stdFunc) {
    auto nums = getRandomVector();
    auto backupNums = nums;
    testFunc(nums);
    stdFunc(backupNums);
    if (nums == backupNums) {
        cout << testName << " ok" << endl;
        return;
    }
    cout << testName << " failed" << endl;
}
void randomTest(const string &testName, const function<void(vector<int>&)> &testFunc) {
    cout << __FUNCTION__ << " " << testName << endl;
    Timer t;
    for (int count = 0;count < 10;count++) {
        auto nums = getRandomVector();
        testFunc(nums);
    }
}
void orderedTest(const string &testName, const function<void(vector<int>&)> &testFunc) {
    cout << __FUNCTION__ << " " << testName << endl;
    Timer t;
    for (int count = 0;count < 10;count++) {
        auto nums = getOrderedVector();
        testFunc(nums);
    }
}
//helper end

//my quick sort
//partion有3种实现方式
//1 如下
//2 双向遍历的填坑式
//3 start,end,middle三点中值的双向遍历式
//3的实现：
//https://stackoverflow.com/questions/49764892/how-to-partition-correctly-using-the-median-of-the-first-middle-and-last-eleme
//三点中值的stl的实现分析
//https://zhuanlan.zhihu.com/p/364361964
//sort实现分析，包含了三点中值的解读
//三点中值既然是stl使用的方式，我猜是最快的
int quick_sort_partion(vector<int> &nums, int start, int end) {
    int pivot = nums[end];
    int smallerThenPivotIndex = start - 1;
    for (int cur = start;cur < end;cur++) {
        if (nums[cur] <= pivot) {
            smallerThenPivotIndex++;
            if (cur != smallerThenPivotIndex) {
                swap(nums[cur], nums[smallerThenPivotIndex]);
            }
        }
    }
    if (end != smallerThenPivotIndex + 1) {
        swap(nums[end], nums[smallerThenPivotIndex + 1]);
    }
    return smallerThenPivotIndex + 1;
}
void quick_sort_imp(vector<int> &nums, int start, int end) {
    if (start < end) {
        int q = quick_sort_partion(nums, start, end);
        quick_sort_imp(nums, start, q - 1);
        quick_sort_imp(nums, q + 1, end);
    }
}
void quick_sort(vector<int> &nums) {
    quick_sort_imp(nums, 0, nums.size() - 1);
}
//my quick sort end

//my random quick sort
//讨论即将跳出循环的边界条件
//start停在大于等于pivot的第一个值上，而end停在小于等于pivot的第一个值上
//nums[origin_start, new_start - 1] < pivot, nums[new_start] >= pivot
//nums[new_end + 1, origin_end] > pivot, nums[new_end] <= pivot
//
//1 如果以new_start作为分区算法返回值
//因为new_start > new_end，所以[new_start + 1, origin_end]是[new_end + 1, origin_end]的子集，都>pivot，而nums[new_start] >= pivot
//所以[new_start, origin_end] >= pivot
//然后[origin_start, new_start - 1] < pivot，分成了两个部分
//因此quick_sort为(start, q - 1]和[q, end], q = new_start
//
//1.1 当pivot取end的时候，无法收敛，对完全有序的序列，pivot为最大值，第一轮循环new_start = new_end = origin_end
//第二轮循环new_start越界，跑出不可控值，使得后面的q也不可控
//还有一个小问题，是当pivot取start的时候，对完全有序的序列，pivot为最小值，第一轮循环new_start = new_end = origin_start
//第二轮循环new_end越界，跑出不可控值，由于分区算法的返回值为new_start，所以这里问题比较隐蔽
//因此pivot随机取[origin_start + 1, origin_end - 1]才行
//
//1.2 想让1.1能让pivot取end，改一行变成if (start >= end) break;就行，此时new_start无法越界
//但是当pivot取start的时候，隐蔽的问题变成了大问题，会无法收敛，对完全有序的序列，pivot为最小值，第一轮循环new_start = new_end = origin_start
//然后break返回，这时quick_sort的q为new_start，[origin_start, q - 1]，也就是[origin_start, origin_start - 1]的递归不会执行
//但是[q, origin_end]，也就是[origin_start, origin_end]和本轮递归一样，导致无法收敛
//因此pivot随机取[origin_start + 1, origin_end]才行
//
//2 如果以new_end作为分区算法返回值
//因为new_start > new_end，所以[origin_start, new_end - 1]是[origin_start, new_start - 1]的子集，都<pivot，而nums[new_end] <= pivot
//所以[origin_start, new_end] <= pivot
//然后[new_end + 1, origin_end] > pivot，分成了两个部分
//因此quick_sort为[start, q]和[q + 1, end], q = new_end
//
//2.1 这里会遇到和1.1一样的问题: 当pivot取end，new_start会跑越界
//如果用end做分区的返回值，这是个小问题，越界问题就比较隐蔽
//同样的，当pivot取start时，new_end会跑溢出越下界，导致无法收敛
//因此pivot随机取[origin_start + 1, origin_end - 1]才行
//
//2.2 同1.2想让取pivot为start时收敛，改一行变成if (start >= end) break;就行
//但是当pivot取end的时候，隐蔽的问题也成了大问题，对完全有序的序列，pivot为最大值，第一轮循环new_start = new_end = origin_end
//然后break返回，这时quick_sort的q为new_end，[origin_start, q]，也就是[origin_start, origin_end]的递归和本轮递归一样，导致无法收敛
//因此pivot随机取[origin_start, origin_end - 1]才行
//
//3 这种写法好处很明显
//一是可以减少start++和end--时的越界判断，二是可以使pivotIndex取一个随机数，来进行完全顺序序列的时间复杂度优化
//但是带来的代价，则是分析边界条件非常的复杂，因为在递归时，是[start, q - 1][q, end]或者[start, q][q + 1, end]
//相对简单写法的[start, q - 1][q + 1, end]，很可能下一轮递归和本轮递归的区间完全一致，导致无法收敛
//
//4 总结
//分区算法的返回值是start或者end都是可以的
//主要是推理清楚完全顺序的序列，在pivot取start或者end值时是否会无法收敛
//
int random_quick_sort_partion(vector<int> &nums, int start, int end, int pivotIndex) {
    int pivot = nums[pivotIndex];
    LDEBUG(LOGV(start), LOGV(end), LOGV(pivotIndex), LOGV(pivot), "start");
    while(true) {
        while(nums[start] < pivot) start++;
        while(nums[end] > pivot) end--;
        if (start >= end) {
            break;
        }
        print(nums);
        LDEBUG("swap", LOGV(start), LOGV(end), LOGV(nums[start]), LOGV(nums[end]));
        swap(nums[start], nums[end]);
        print(nums);
        start++;
        end--;
    }
    print(nums);
    LDEBUG(LOGV(start), LOGV(end), LOGV(pivotIndex), LOGV(pivot), "end");
    return start;
}
void random_quick_sort_imp(vector<int> &nums, int start, int end) {
    if (start < end) {
        /*int pivotIndex = (rand() % (end - start)) + start;
        if (pivotIndex != end) {
            swap(nums[pivotIndex], nums[end]);
        }
        int q = quick_sort_partion(nums, start, end);*/
        int pivotIndex = (rand() % (end - start)) + start + 1; //rand()%(end-start),结果[0, end - start - 1]
        int q = random_quick_sort_partion(nums, start, end, pivotIndex);
        LDEBUG(LOGV(start), LOGV(q), LOGV(end), "start");
        random_quick_sort_imp(nums, start, q - 1);
        random_quick_sort_imp(nums, q, end);
        LDEBUG(LOGV(start), LOGV(q), LOGV(end), "end");
    }
}
void random_quick_sort(vector<int> &nums) {
    random_quick_sort_imp(nums, 0, nums.size() - 1);
}
//my random quick sort end

//csdn code
//https://blog.csdn.net/qq_44819750/article/details/106112299
//start == end的时候，跳出循环
//由于end先行，因此end会先停在一个小于pivot的数上，然后start也停在end上
//最后将start和temp互换，也就是小于pivot的数和pivot互换，使得这个数在nums的最前面
//从而使得[初始start, 最终start)的数都小于等于pivot,最终start的值为pivot,(最终start, 初始end]的数都大于等于pivot
//因此在quicksort中，可以排除掉pivot值的再次划分，他必然在正确的位置上了
//PS：因为必须要和start交换，来把小于pivot的数放在最前面，所以这种写法不支持随机pivot的优化
int csdn_quick_sort_partion(vector<int> &nums,int start,int end){
    int pivot = nums[start];
    int temp = start;

    LDEBUG(LOGV(start), LOGV(end), LOGV(pivot), "start");
    while(start < end)
    {
        while(start < end && nums[end] >= pivot)
            end--;
        while(start < end && nums[start] <= pivot)
            start++;
        swap(nums[start], nums[end]);
    }
    print(nums);
    swap(nums[start], nums[temp]);
    print(nums);
    LDEBUG(LOGV(start), LOGV(end), LOGV(pivot), "end");
    return start;
}
void csdn_quick_sort(vector<int> &nums,int start,int end)
{
    if(start >= end)
        return ;
    int q = csdn_quick_sort_partion(nums, start, end);
    LDEBUG(LOGV(start), LOGV(q), LOGV(end), "start");
    csdn_quick_sort(nums, start, q - 1);
    csdn_quick_sort(nums, q + 1, end);
    LDEBUG(LOGV(start), LOGV(q), LOGV(end), "end");
}
//csdn code end

//leetcode book code
//https://leetcode.cn/leetbook/read/sort-algorithms/eul7hm/
//
//讨论即将跳出循环的边界条件，此时要分两种情况：
//情况1 倒数第二轮循环，在交换过一次后，start + 2 = end，此时start++且end--，start = end
//最后一次循环从而跳出while(start<end)的循环
//跳出循环后判断此时nums[end] > pivot，所以[start,end)都小于等于pivot，nums[end]大于pivot，(end, end]都大于等于pivot
//因此将end--后，nums[end] <= pivot
//情况2 倒数第二轮循环，在交换过一次后，start + 1 = end，此时start++且end--，new_start = new_end + 1
//最后一次循环从而跳出while(start<end)的循环
//由于此时new_end = old_end - 1 = old_start，而old_start是刚和人交换过小于pivot的值，因此nums[new_end] < pivot
//
//
//合并情况1和情况2，nums[end] <= pivot，将nums[end]和pivot交换，此时的[start, end)肯定都小于等于pivot
//因此在quicksort中，可以排除掉pivot值的再次划分，他必然在正确的位置上了
//
// 将 nums 从 start 到 end 分区，左边区域比基数小，右边区域比基数大，然后返回中间值的下标
int lc_quick_sort_partition(vector<int> &nums, int start, int end) {
    // 取第一个数为基数
    int pivot = nums[start];
    LDEBUG(LOGV(start), LOGV(end), LOGV(pivot), "start");
    int temp = start;
    // 从第二个数开始分区
    start++;
    // 右边界
    
    while (start < end) {
        // 找到第一个大于基数的位置
        while (start < end && nums[start] <= pivot) start++;
        // 找到第一个小于基数的位置
        while (start < end && nums[end] >= pivot) end--;
        // 交换这两个数，使得左边分区都小于或等于基数，右边分区大于或等于基数
        if (start < end) {
            swap(nums[start], nums[end]);
            start++;
            end--;
        }
    }
    // 如果 start 和 end 相等，单独比较 nums[end] 和 pivot
    if (start == end && nums[end] > pivot) end--;
    swap(nums[temp], nums[end]);
    print(nums);
    LDEBUG(LOGV(start), LOGV(end), LOGV(pivot), "end");
    return end;
}
void lc_quick_sort(vector<int> &nums, int start, int end) {
    // 如果区域内的数字少于 2 个，退出递归
    if (start >= end) return;
    // 将数组分区，并获得中间值的下标
    int q = lc_quick_sort_partition(nums, start, end);
    // 对左边区域快速排序
    LDEBUG(LOGV(start), LOGV(q), LOGV(end), "start");
    lc_quick_sort(nums, start, q - 1);
    // 对右边区域快速排序
    lc_quick_sort(nums, q + 1, end);
    LDEBUG(LOGV(start), LOGV(q), LOGV(end), "end");
}
//leetcode book code end

//stackoverflow code start
int stackoverflow_partition(std::vector<int> &nums, int start, int end) {
    int middle = start + (end - start) / 2;
    if((nums.at(middle) >= nums.at(start) && nums.at(middle) <= nums.at(end))
        || (nums.at(middle) <= nums.at(start) && nums.at(middle) >= nums.at(end)))
        std::swap(nums.at(end), nums.at(middle));
    if((nums.at(end) >= nums.at(start) && nums.at(end) <= nums.at(middle))
        || (nums.at(end) <= nums.at(start) && nums.at(end) >= nums.at(middle)))
        std::swap(nums.at(start), nums.at(end));
    int pivot = nums.at(start);
    LDEBUG(LOGV(start), LOGV(end), LOGV(pivot), "start");
    start--;
    end++;
    while(true) {
        do {
            start++;
        } while(nums.at(start) < pivot);
        do {
            end--;
        } while(nums.at(end) > pivot);
        if(start >= end) {
            break;
        }
        std::swap(nums.at(start), nums.at(end));
    }
    print(nums);
    LDEBUG(LOGV(start), LOGV(end), LOGV(pivot), LOGV(nums[end]), "end");
    return end;
}
void stackoverflow_quick_sort(std::vector<int> &nums, int start, int end) {
    if(start < end) {
        int q = stackoverflow_partition(nums, start, end);
        LDEBUG(LOGV(start), LOGV(q), LOGV(end), "start");
        stackoverflow_quick_sort(nums, start, q);
        stackoverflow_quick_sort(nums, q + 1, end);
        LDEBUG(LOGV(start), LOGV(q), LOGV(end), "end");
    }
}
//stackoverflow code end

template <typename T>
void testFunc(const string &name, vector<int> nums, const T& f) {
    LDEBUG(LOGV(name), "------------------------------", "start");
    print(nums);
    f(nums);
    print(nums);
    LDEBUG(LOGV(name), "------------------------------", "end");
}

int main() {
    srand(time(0));

    auto my_quick_sort = [&](vector<int> &nums){quick_sort(nums);};
    auto my_random_quick_sort = [&](vector<int> &nums){random_quick_sort(nums);};
    auto other_csdn_quick_sort = [&](vector<int> &nums){csdn_quick_sort(nums, 0, nums.size() - 1);};
    auto other_lc_quick_sort = [&](vector<int> &nums){lc_quick_sort(nums, 0, nums.size() - 1);};
    auto other_stackoverflow_quick_sort = [&](vector<int> &nums){stackoverflow_quick_sort(nums, 0, nums.size() - 1);};
    auto std_quick_sort = [&](vector<int> &nums){sort(nums.begin(), nums.end());};
    //vector<int> nums{3,2,1,6,4,1,2,3};
    //vector<int> nums{3,2,1,3,2,1,1,2,3};
    vector<int> nums{1,2};
    testFunc("my normal", nums, my_quick_sort);
    testFunc("my random", nums, my_random_quick_sort);
    testFunc("csdn", nums, other_csdn_quick_sort);
    testFunc("lc", nums, other_csdn_quick_sort);
    testFunc("stackoverflow", nums, other_stackoverflow_quick_sort);
    testFuncOk("my normal", my_quick_sort, std_quick_sort);
    testFuncOk("my random", my_random_quick_sort, std_quick_sort);
    testFuncOk("csdn", other_csdn_quick_sort, std_quick_sort);
    testFuncOk("lc", other_lc_quick_sort, std_quick_sort);
    testFuncOk("stackoverflow", other_stackoverflow_quick_sort, std_quick_sort);
    randomTest("my normal", my_quick_sort);
    randomTest("my random", my_random_quick_sort);
    randomTest("csdn", other_csdn_quick_sort);
    randomTest("lc", other_lc_quick_sort);
    randomTest("stackoverflow", other_stackoverflow_quick_sort);
    randomTest("std", std_quick_sort);
    orderedTest("my normal", my_quick_sort);
    orderedTest("my random", my_random_quick_sort);
    orderedTest("csdn", other_csdn_quick_sort);
    orderedTest("lc", other_lc_quick_sort);
    orderedTest("stackoverflow", other_stackoverflow_quick_sort);
    orderedTest("std", std_quick_sort);
}
