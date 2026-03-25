#include <iostream>
#include <vector>
#include <windows.h>
#include <cstdint>
#include <algorithm>

using namespace std;

// 结构体，存每一个规模的测试结果
struct Row {
    int n;
    int repeat;
    double avg_us;
    int64_t checksum;
};

// 防止编译器把重复计算过度优化掉
volatile int64_t g_sink = 0;

// 普通链式累加
__declspec(noinline) int64_t ordinary(const vector<int>& a, int n) {
    int64_t s = 0;
    for (int i = 0; i < n; i++) {
        s += a[i];
    }
    return s;
}

// 规模列表，可修改
vector<int> sizes() {
    return {
        1024, 2048, 4096, 8192, 16384, 32768,
        65536, 131072, 262144, 524288,
        1048576, 2097152, 4194304, 8388608
    };
}

// 根据规模大小决定跑几次
int repeat_of(int n) {
    if (n <= 16384) return 10000;
    if (n <= 131072) return 5000;
    if (n <= 1048576) return 1000;
    return 200;
}

// 初始化向量，值在 1~10 循环，保证每次测试数据一致
void init_data(vector<int>& a, int n) {
    for (int i = 0; i < n; i++) {
        a[i] = i % 10 + 1;
    }
}

// 跑一次测试，返回 Row 结构体
Row run_one(int n, const LARGE_INTEGER& freq) {
    vector<int> a(n);

    init_data(a, n);
    int64_t warm = ordinary(a, n); // warm-up，预热后再正式计时

    int repeat = repeat_of(n);

    int64_t checksum = 0;
    const int rounds = 5; // 每个规模跑5轮，取中位数结果，降低偶然因素影响
    vector<double> times; // 存每轮的平均时间，单位微秒
    times.reserve(rounds); //分配内存

    for (int r = 0; r < rounds; r++) {
        LARGE_INTEGER start, end;
        QueryPerformanceCounter(&start); // 开始计时
        for (int t = 0; t < repeat; t++) {
            int64_t cur = ordinary(a, n); // 执行计算
            checksum += cur;              // 统一口径：累加所有轮次与重复的结果
            g_sink ^= (cur + t);         // 写入volatile，避免循环被消除
        }
        QueryPerformanceCounter(&end); // 结束计时

        double total_us = (end.QuadPart - start.QuadPart) * 1e6 / freq.QuadPart;
        times.push_back(total_us / repeat);
    }

    sort(times.begin(), times.end()); // 排序后取中位数，降低抖动影响
    double avg_us = times[times.size() / 2];

    return {n, repeat, avg_us, checksum + warm - warm};
}

int main() {
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);

    cout << "n,repeat,avg_time_us,checksum" << endl;
    for (int n : sizes()) {
        Row r = run_one(n, freq);
        cout << r.n << "," << r.repeat << "," << r.avg_us << "," << r.checksum << endl;
    }

    return 0;
}

// g++ -O2 sum_ordinary.cpp -o sum_ordinary
