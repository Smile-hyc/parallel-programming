#include <iostream>
#include <vector>
#include <chrono>
#include <cstdint>
#include <algorithm>

using namespace std;
using Clock = std::chrono::high_resolution_clock;

// 结构体，存每一个规模的测试结果
struct Row {
    int n;
    int repeat;
    double avg_us;
    int64_t checksum;
};

// 防止编译器把重复计算过度优化掉
volatile int64_t g_sink = 0;

// 规模列表，可修改
vector<int> sizes() {
    return {
        1024
    };
}

// 根据规模大小决定跑几次
int repeat_of(int n) {
    if (n <= 16) return 10000;
    if (n <= 64) return 5000;
    if (n <= 256) return 2000;
    if (n <= 1024) return 500;
    return 100;
}

// 初始化矩阵和向量
void init_data(vector<int>& A, vector<int>& x, int n) {
    for (int i = 0; i < n; i++) {
        x[i] = i % 10 + 1;
        for (int j = 0; j < n; j++) {
            A[i * n + j] = (i + j) % 17 + 1;
        }
    }
}

// ordinary 版本：按列访问
__attribute__((noinline)) int64_t ordinary(const vector<int>& A, const vector<int>& x, int n) {
    vector<int64_t> y(n, 0);

    for (int j = 0; j < n; j++) {
        int64_t s = 0;
        for (int i = 0; i < n; i++) {
            s += 1LL * A[i * n + j] * x[i];
        }
        y[j] = s;
    }

    int64_t checksum = 0;
    for (int j = 0; j < n; j++) checksum += y[j];
    return checksum;
}

// 跑一次测试，返回 Row 结构体
Row run_one(int n) {
    vector<int> A(n * n), x(n);

    init_data(A, x, n);
    int64_t warm = ordinary(A, x, n); // warm-up

    int repeat = repeat_of(n);

    int64_t checksum = 0;
    const int rounds = 5;
    vector<double> times;
    times.reserve(rounds);

    for (int r = 0; r < rounds; r++) {
        auto start = Clock::now();

        for (int t = 0; t < repeat; t++) {
            int64_t cur = ordinary(A, x, n);
            checksum += cur;
            g_sink ^= (cur + t);
        }

        auto end = Clock::now();
        double total_us = std::chrono::duration<double, std::micro>(end - start).count();
        times.push_back(total_us / repeat);
    }

    sort(times.begin(), times.end());
    double avg_us = times[times.size() / 2];

    return {n, repeat, avg_us, checksum + warm - warm};
}

int main() {
    cout << "n,repeat,avg_time_us,checksum" << endl;
    for (int n : sizes()) {
        Row r = run_one(n);
        cout << r.n << "," << r.repeat << "," << r.avg_us << "," << r.checksum << endl;
    }
    return 0;
}

// WSL/Linux 下编译：g++ -O2 matrix_ordinary_linux.cpp -o matrix_ordinary_linux