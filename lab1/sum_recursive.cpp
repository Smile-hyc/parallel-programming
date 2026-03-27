#include <iostream>
#include <vector>
#include <windows.h>
#include <cstdint>
#include <algorithm>

using namespace std;

struct Row {
    int n;
    int repeat;
    double avg_us;
    int64_t checksum;
};

volatile int64_t g_sink = 0;

// 递归规约：把当前数组前 n 个元素两两相加，结果写回前半部分
void recursion(vector<int64_t>& a, int n) {
    if (n == 1) return;

    for (int i = 0; i < n / 2; i++) {
        a[i] = a[i * 2] + a[i * 2 + 1];
    }

    // 如果 n 是奇数，把最后一个元素保留下来
    if (n % 2 == 1) {
        a[n / 2] = a[n - 1];
        recursion(a, n / 2 + 1);
    } else {
        recursion(a, n / 2);
    }
}

__declspec(noinline) int64_t recursive_pairwise(const vector<int>& src, int n) {
    vector<int64_t> a(n);
    for (int i = 0; i < n; i++) a[i] = src[i];

    recursion(a, n);
    return a[0];
}

vector<int> sizes() {
    return {
        1024, 2048, 4096, 8192, 16384, 32768,
        65536, 131072, 262144, 524288,
        1048576, 2097152, 4194304, 8388608
    };
}

int repeat_of(int n) {
    if (n <= 16384) return 10000;
    if (n <= 131072) return 5000;
    if (n <= 1048576) return 1000;
    return 200;
}

void init_data(vector<int>& a, int n) {
    for (int i = 0; i < n; i++) {
        a[i] = i % 10 + 1;
    }
}

Row run_one(int n, const LARGE_INTEGER& freq) {
    vector<int> a(n);
    init_data(a, n);

    int64_t warm = recursive_pairwise(a, n);

    int repeat = repeat_of(n);
    int64_t checksum = 0;

    const int rounds = 5;
    vector<double> times;
    times.reserve(rounds);

    for (int r = 0; r < rounds; r++) {
        LARGE_INTEGER start, end;
        QueryPerformanceCounter(&start);

        for (int t = 0; t < repeat; t++) {
            int64_t cur = recursive_pairwise(a, n);
            checksum += cur;
            g_sink ^= (cur + t);
        }

        QueryPerformanceCounter(&end);
        double total_us = (end.QuadPart - start.QuadPart) * 1e6 / freq.QuadPart;
        times.push_back(total_us / repeat);
    }

    sort(times.begin(), times.end());
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

// g++ -O2 sum_recursive.cpp -o sum_recursive.exe