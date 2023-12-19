#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <set>

using namespace std;

typedef int T_ASSOC;
typedef int T_SIZE;
typedef int T_LINE_LEN;

constexpr int MAX_MEM = 1024 * 1024 * 1024;
constexpr int MAX_ASSOC = 32;
constexpr int PROB_TIMES = 1000 * 1000;

char *measure_data;

void init_measure_data() {
    measure_data = (char *) malloc(MAX_MEM);
    if (measure_data == nullptr) {
        cout << "Could not to allocate memory.\n";
        exit(1);
    }
}

void free_measure_data() {
    if (measure_data != nullptr) {
        free(measure_data);
    }
}

long long measure_pattern(const int patt_pow, const int assoc_patt) {
    char **el;
    for (int i = (assoc_patt - 1) * patt_pow; i >= 0; i -= patt_pow) {
        el = (char **) &measure_data[i];
        if (i >= patt_pow) {
            *el = &measure_data[i - patt_pow];
        } else {
            *el = &measure_data[(assoc_patt - 1) * patt_pow];
        }
    }

    const long long rounds = 20;
    vector<long long> measured_times;
    for (long long k = 0L; k < rounds; k++) {
        auto t_start = chrono::high_resolution_clock::now();
        for (long long prob = 0L; prob < PROB_TIMES; prob++) {
            el = (char **) *el;
        }
        auto t_end = chrono::high_resolution_clock::now();
        auto t_delta = t_end - t_start;
        measured_times.push_back((t_delta).count());
    }
    int time_sum = 0;
    for_each(measured_times.begin(), measured_times.end(), [&] (int t) {
        time_sum += t;
    });
    return time_sum / rounds;
}

pair<vector<set<int>>, int> get_calc_jmps() {
    vector<set<int>> calculated_jmps;
    int patt_pow = MAX_ASSOC;

    for (; patt_pow < MAX_MEM / MAX_ASSOC; patt_pow *= 2) {
        long long previous_measure = measure_pattern(patt_pow, 1);
        set<int> next_jmp;

        for (int assoc_iter = 1; assoc_iter <= MAX_ASSOC; assoc_iter++) {
            long long current_measure = measure_pattern(patt_pow, assoc_iter);
            long long delta_measure = current_measure - previous_measure;
            if (delta_measure * 10 > current_measure) {
                next_jmp.insert(assoc_iter - 1);
            }
            previous_measure = current_measure;
        }

        bool same = true;
        if (!calculated_jmps.empty()) {
            for (int new_jmp: next_jmp) {
                same &= calculated_jmps.back().count(new_jmp);
            }
            for (int jmp: calculated_jmps.back()) {
                same &= next_jmp.count(jmp);
            }
        }

        if (same && patt_pow >= 256 * 1024) {
            break;
        }
        calculated_jmps.push_back(next_jmp);
    }
    return {calculated_jmps, patt_pow};
}

vector<pair<T_SIZE, T_ASSOC>> calculate_cache() {
    auto [j_patterns, patt_pow] = get_calc_jmps();
    vector<pair<int, int>> caches;
    set<int> j_pattern = j_patterns[j_patterns.size() - 1];
    reverse(j_patterns.begin(), j_patterns.end());
    for (auto &patt: j_patterns) {
        set<int> j_exclude;
        for (int assoc: j_pattern) {
            if (!patt.count(assoc)) {
                caches.push_back(make_pair(patt_pow * assoc, assoc));
                j_exclude.insert(assoc);
            }
        }
        for (int patt_to_erase: j_exclude) {
            j_pattern.erase(patt_to_erase);
        }
        patt_pow /= 2;
    }
    if (caches.empty()) {
        cout << "Failed calculating cache characteristics.\n";
        exit(1);
    }
    return caches;
}

pair<T_SIZE, T_ASSOC> get_l1_cache_characteristics(vector<pair<T_SIZE, T_ASSOC>> cache_chars) {
    sort(cache_chars.begin(), cache_chars.end());
    return cache_chars[0];
}

T_LINE_LEN get_cache_line_len(T_SIZE cache_size, T_ASSOC cache_assoc) {
    int result = -1;
    int previous_jmp = 1025;

    for (int L = 1; L <= cache_size; L *= 2) {
        long long previous_measure = measure_pattern(cache_size / cache_assoc + L, 2);
        int jmp = -1;
        for (int accoc_patt = 1; accoc_patt <= 1024; accoc_patt *= 2) {
            long long current_measure = measure_pattern(cache_size / cache_assoc + L, accoc_patt + 1);
            long long delta_measure = current_measure - previous_measure;
            if (delta_measure * 10 > current_measure) {
                if (jmp <= 0) {
                    jmp = accoc_patt;
                }
            }
            previous_measure = current_measure;
        }
        if (jmp > previous_jmp) {
            result = L * cache_assoc;
            break;
        }
        previous_jmp = jmp;
    }

    return result;
}

tuple<T_SIZE, T_ASSOC, T_LINE_LEN> get_cache_characteristics() {
    auto [cache_size, cache_assoc] = get_l1_cache_characteristics(calculate_cache());
    T_LINE_LEN cache_line_len = get_cache_line_len(cache_size, cache_assoc);
    return {cache_size, cache_assoc, cache_line_len};
}

int main() {
    cout << "Start measuring L1 cache characteristics...\n";
    init_measure_data();
    auto [cache_size, cache_assoc, cache_line_len] = get_cache_characteristics();
    cout << "Measurement results:\n" << "cache_size = " << cache_size <<
              "\nassociativity = " << cache_assoc <<
              "\ncache_line_length = " << cache_line_len << "\n";
    free_measure_data();
    return 0;
}