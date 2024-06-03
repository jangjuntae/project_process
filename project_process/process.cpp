#include <iostream>
#include <thread>
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <mutex>
#include <chrono>
#include <atomic>
#include <functional>

using namespace std;

mutex cout_mutex;
mutex vector_mutex;

string prompt = "prompt> ";

int gcd(int a, int b) {
    while (b != 0) {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

int count_primes(int n) {
    vector<bool> prime(n + 1, true);
    int count = 0;
    for (int p = 2; p * p <= n; p++) {
        if (prime[p]) {
            for (int i = p * p; i <= n; i += p) {
                prime[i] = false;
            }
        }
    }
    for (int p = 2; p <= n; p++) {
        if (prime[p]) count++;
    }
    return count;
}

long long sum_partial(int start, int end) {
    long long sum = 0;
    for (int i = start; i <= end; i++) {
        sum += i;
        sum %= 1000000;
    }
    return sum;
}

void execute_sum(int n, int m) {
    vector<thread> workers;
    vector<long long> partial_sums(m, 0);
    int chunk_size = n / m;
    int remaining = n % m;

    for (int i = 0; i < m; i++) {
        int start = i * chunk_size + 1;
        int end = (i + 1) * chunk_size + (i == m - 1 ? remaining : 0);

        workers.emplace_back([=, &partial_sums]() {
            long long sum = sum_partial(start, end);
            partial_sums[i] = sum;  // 스레드에서 계산한 부분합을 저장
            });
    }

    for (auto& worker : workers) {
        worker.join();  // 모든 스레드가 종료될 때까지 대기
    }

    long long final_sum = 0;
    for (auto value : partial_sums) {
        final_sum += value;
        final_sum %= 1000000;  // 최종 합산을 모듈로 연산
    }

    // 최종 결과를 한 번만 출력
    cout << "Sum of numbers up to " << n << " mod 1000000 is " << final_sum << endl;
}

void execute_command(string cmd, int repeat_period, int duration, int instance_count, int m, bool isBackground) {
    vector<thread> threads;
    auto start_time = chrono::steady_clock::now();

    for (int i = 0; i < instance_count; ++i) {
        threads.emplace_back([=]() {
            try {
                while (chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - start_time).count() < duration) {
                    {
                        lock_guard<mutex> lock(cout_mutex);
                        if (cmd.find("gcd") == 0) {
                            stringstream ss(cmd);
                            string tmp; int x, y;
                            ss >> tmp >> x >> y;
                            cout << "GCD of " << x << " and " << y << " is " << gcd(x, y) << endl;
                        }
                        else if (cmd.find("prime") == 0) {
                            stringstream ss(cmd);
                            string tmp; int x;
                            ss >> tmp >> x;
                            cout << "Count of primes up to " << x << " is " << count_primes(x) << endl;
                        }
                        else if (cmd.find("sum") == 0) {
                            stringstream ss(cmd);
                            string tmp; int x;
                            ss >> tmp >> x;
                            if (m > 0) {
                                execute_sum(x, m);
                            }
                            else {
                                cout << "Sum of numbers up to " << x << " mod 1000000 is " << sum_partial(1, x) << endl;
                            }
                        }
                        else if (cmd.find("echo") == 0) {
                            cout << cmd.substr(5) << endl;
                        }
                    }
                    this_thread::sleep_for(chrono::seconds(repeat_period));
                    if (repeat_period == 0) break;
                }
            }
            catch (const std::exception& e) {
                lock_guard<mutex> lock(cout_mutex);
                cout << "Exception caught in thread: " << e.what() << endl;
            }
            });
    }

    if (!isBackground) {
        for (auto& thread : threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }
    else {
        for (auto& thread : threads) {
            thread.detach();
        }
    }
}


void process_command(const string& line) {
    bool isBackground = line[0] == '&';
    string cleanLine = isBackground ? line.substr(1) : line;

    istringstream iss(cleanLine);
    string token;
    vector<string> tokens;

    while (iss >> token) {
        tokens.push_back(token);
    }

    if (tokens.empty()) return;

    string cmd = tokens[0];
    int n = 1, d = 300, p = 0, m = 0;

    for (size_t i = 1; i < tokens.size(); ++i) {
        if (tokens[i] == "-n" && i + 1 < tokens.size()) {
            n = stoi(tokens[i + 1]);
            i++;
        }
        else if (tokens[i] == "-d" && i + 1 < tokens.size()) {
            d = stoi(tokens[i + 1]);
            i++;
        }
        else if (tokens[i] == "-p" && i + 1 < tokens.size()) {
            p = stoi(tokens[i + 1]);
            i++;
        }
        else if (tokens[i] == "-m" && i + 1 < tokens.size()) {
            m = stoi(tokens[i + 1]);
            i++;
        }
        else {
            cmd += " " + tokens[i];
        }
    }

    execute_command(cmd, p, d, n, m, isBackground);
}

void shell_thread_function(const string& filename) {
    ifstream file(filename);
    string line;

    if (!file.is_open()) {
        lock_guard<mutex> lock(cout_mutex);
        cout << "Failed to open " << filename << endl;
        return;
    }

    while (getline(file, line)) {
        cout << prompt << line << endl;
        process_command(line);
    }
}

int main() {
    thread shell(shell_thread_function, "commands.txt");
    shell.join();
    return 0;
}
