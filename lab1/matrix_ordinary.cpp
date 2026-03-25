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

// ordinary版本的矩阵乘向量（按列内积）
void ordinary(const vector<int>& A, const vector<int>& x, vector<int>& y, int n) {
    for (int j = 0; j < n; j++) {
        y[j] = 0;
        for (int i = 0; i < n; i++) {
            y[j] += A[i * n + j] * x[i];
        }
    }
}

// 规模列表，可修改
vector<int> sizes() {
    return {
        16, 32, 48, 64, 80, 96, 112, 128,
        192, 256, 320, 384, 448, 512,
        768, 1024, 1536, 2048
    };
}

// 根据规模大小决定跑几次
int repeat_of(int n) {
    if (n <= 128) return 5000;
    if (n <= 512) return 1000;
    if (n <= 1024) return 200;
    return 50;
}

// 初始化矩阵和向量，0-9循环，保证每次测试数据一致
void init_data(vector<int>& A, vector<int>& x, int n) {
    for (int i = 0; i < n; i++) {
        x[i] = i % 10 + 1;
        for (int j = 0; j < n; j++) {
            A[i * n + j] = (i + j) % 10;
        }
    }
}

// 计算结果向量y的元素和，作为checksum
int64_t sum_y(const vector<int>& y) {
    int64_t s = 0;
    for (int v : y) s += v;
    return s;
}

// 跑一次测试，返回Row结构体
Row run_one(int n, const LARGE_INTEGER& freq) {
    vector<int> A(n * n);
    vector<int> x(n), y(n);

    init_data(A, x, n);
    ordinary(A, x, y, n); // warm-up，预热后再正式计时

    int repeat = repeat_of(n); 
    const int rounds = 5; // 每个规模跑5轮，取中位数结果，降低偶然因素影响
    vector<double> times; // 存每轮的平均时间，单位微秒
    times.reserve(rounds); //分配内存

    for (int r = 0; r < rounds; r++) {
        LARGE_INTEGER start, end;
        QueryPerformanceCounter(&start); // 开始计时
        for (int t = 0; t < repeat; t++) {
            ordinary(A, x, y, n); // 执行计算
        }
        QueryPerformanceCounter(&end); // 结束计时

        double total_us = (end.QuadPart - start.QuadPart) * 1e6 / freq.QuadPart;
        times.push_back(total_us / repeat);
    }

    sort(times.begin(), times.end()); // 排序后取中位数，降低抖动影响
    double avg_us = times[times.size() / 2];

    return {n, repeat, avg_us, sum_y(y)};
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

// g++ -O2 matrix_ordinary.cpp -o matrix_ordinary
