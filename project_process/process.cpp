#include <iostream>
#include <thread>
#include <vector>
#include <fstream>
#include <sstream>
#include <mutex>
#include <map>
#include <functional>
#include <algorithm>

using namespace std;

mutex cout_mutex;

void echo(const string& text) {
    lock_guard<mutex> lock(cout_mutex);
    cout << text << endl;
}

int gcd(int a, int b) {
    while (b != 0) {
        int t = b;
        b = a % b;
        a = t;
    }
    return a;
}

int count_primes(int limit) {
    vector<bool> is_prime(limit + 1, true);
    int count = 0;

    for (int i = 2; i <= limit; ++i) {
        if (is_prime[i]) {
            ++count;
            for (int j = 2 * i; j <= limit; j += i) {
                is_prime[j] = false;
            }
        }
    }
    return count;
}

long long sum_up_to(int limit) {
    long long sum = 0;
    for (int i = 1; i <= limit; ++i) {
        sum += i;
    }
    return sum % 1000000;
}

void execute_command(const string& command) {
    istringstream iss(command);
    string cmd;
    iss >> cmd;

    if (cmd == "echo") {
        string text;
        getline(iss, text);
        echo(text.substr(1)); // remove leading space
    }
    else if (cmd == "gcd") {
        int x, y;
        iss >> x >> y;
        lock_guard<mutex> lock(cout_mutex);
        cout << "GCD of " << x << " and " << y << " is " << gcd(x, y) << endl;
    }
    else if (cmd == "prime") {
        int x;
        iss >> x;
        lock_guard<mutex> lock(cout_mutex);
        cout << "Count of primes up to " << x << " is " << count_primes(x) << endl;
    }
    else if (cmd == "sum") {
        int x;
        iss >> x;
        lock_guard<mutex> lock(cout_mutex);
        cout << "Sum of numbers up to " << x << " mod 1000000 is " << sum_up_to(x) << endl;
    }
}

void process_commands(const string& filename) {
    ifstream file(filename);
    string line;
    vector<thread> threads;

    while (getline(file, line)) {
        threads.push_back(thread(execute_command, line));
        this_thread::sleep_for(chrono::seconds(5)); // simulate the time interval
    }

    // Wait for all threads to complete
    for (auto& th : threads) {
        th.join();
    }
}

int main() {
    string filename = "commands.txt";
    process_commands(filename);
    return 0;
}
