#include <iostream>  // 입출력 관련 표준 라이브러리
#include <thread>  // 스레드 관련 기능을 사용하기 위한 헤더
#include <vector>  // 벡터 컨테이너 사용을 위한 헤더
#include <fstream>  // 파일 입출력 관련 기능을 사용하기 위한 헤더
#include <sstream>  // 문자열 스트림 관련 기능을 사용하기 위한 헤더
#include <string>  // 문자열 관련 기능을 사용하기 위한 헤더
#include <mutex>  // 뮤텍스 사용을 위한 헤더
#include <chrono>  // 시간 관련 기능을 사용하기 위한 헤더
#include <atomic>  // 원자적 연산을 위한 헤더
#include <functional>  // 함수 객체, 람다 등을 사용하기 위한 헤더

using namespace std;

mutex cout_mutex;  // 콘솔 출력을 동기화하는 뮤텍스
mutex vector_mutex;  // 벡터 접근을 동기화하는 뮤텍스

string prompt = "prompt> "; // 명령어 프롬프트 문자열

// 최대공약수를 계산하는 함수
int gcd(int a, int b) {
    while (b != 0) {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

// n 이하의 소수 개수를 세는 함수
int count_primes(int n) {
    vector<bool> prime(n + 1, true);
    int count = 0;
    for (int p = 2; p * p <= n; p++) {
        if (prime[p]) {
            for (int i = p * p; i <= n; i += p) {
                prime[i] = false; // 소수가 아닌 것으로 표시
            }
        }
    }
    for (int p = 2; p <= n; p++) {
        if (prime[p]) count++;
    }
    return count;
}

// 지정된 구간의 정수 합을 계산하는 함수
long long sum_partial(int start, int end) {
    long long sum = 0;
    for (int i = start; i <= end; i++) {
        sum += i;
        sum %= 1000000; // 결과를 1000000으로 나눈 나머지를 계산
    }
    return sum;
}

// n까지의 정수를 m 개의 스레드로 나누어 합을 계산하는 함수
void execute_sum(int n, int m) {
    vector<thread> workers;  // 스레드를 저장할 벡터
    vector<long long> partial_sums(m, 0);  // 각 스레드의 계산 결과를 저장할 벡터
    int chunk_size = n / m;  // 각 스레드가 처리할 구간의 크기
    int remaining = n % m;  // 나머지 처리할 수

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

// 명령어에 따라 스레드를 생성하여 지정된 작업을 수행하는 함수
void execute_command(string cmd, int repeat_period, int duration, int instance_count, int m, bool isBackground) {
    vector<thread> threads;  // 작업을 수행할 스레드들
    auto start_time = chrono::steady_clock::now();  // 시작 시간 기록

    for (int i = 0; i < instance_count; ++i) {
        threads.emplace_back([=]() {
            try {
                while (chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - start_time).count() < duration) {
                    {
                        lock_guard<mutex> lock(cout_mutex); // 콘솔 출력을 동기화
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
            // 예외 발생 시 콘솔에 출력
            catch (const std::exception& e) {
                lock_guard<mutex> lock(cout_mutex);
                cout << "Exception caught in thread: " << e.what() << endl;
            }
            });
    }

    if (!isBackground) {
        for (auto& thread : threads) {
            if (thread.joinable()) {
                thread.join(); // 백그라운드 모드가 아닐 경우, 모든 스레드가 종료될 때까지 대기
            }
        }
    }
    else {
        for (auto& thread : threads) {
            thread.detach(); // 백그라운드 모드일 경우, 스레드를 분리하여 독립적으로 실행
        }
    }
}

// 파일에서 명령을 읽어와서 처리하는 함수
void process_command(const string& line) {
    bool isBackground = line[0] == '&'; // 명령어가 백그라운드 실행 여부 체크
    string cleanLine = isBackground ? line.substr(1) : line; // 백그라운드 식별자 제거

    istringstream iss(cleanLine);
    string token;
    vector<string> tokens;

    while (iss >> token) {
        tokens.push_back(token); // 공백을 기준으로 명령어를 분리하여 벡터에 저장
    }

    if (tokens.empty()) return; // 명령어가 비어있으면 종료

    string cmd = tokens[0];
    int n = 1, d = 300, p = 0, m = 0; // 명령어 관련 변수 초기화

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

// 파일에서 명령을 읽어 실행하는 메인 스레드 함수
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

// 메인 함수, commands.txt 파일로부터 명령을 읽어 스레드를 생성하여 실행
int main() {
    thread shell(shell_thread_function, "commands.txt");
    shell.join();
    return 0;
}
