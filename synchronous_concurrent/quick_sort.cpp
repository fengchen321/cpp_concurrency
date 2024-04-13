#include <iostream>
#include <list>
#include <algorithm>
#include <future>

template<typename T>
void quick_sort_recursive(T q[], int l, int r){
    if (l >= r) return;
    T x = q[(l + r + 1) >> 1];
    int i = l - 1, j = r + 1;
    while(i < j){
        do i++; while(q[i] < x);
        do j--; while(q[j] > x);
        if (i < j) std::swap (q[i], q[j]);
    }
    quick_sort_recursive(q, l, i - 1);
    quick_sort_recursive(q, i, r);
}

template<typename T>
void quick_sort(T q[], int len) {
    quick_sort_recursive(q, 0, len - 1);
}

void test_quick_sort() {
    int num_arr[] = {5, 3, 7, 6, 4, 1, 0, 2, 9, 10, 8};
    int length = sizeof(num_arr) / sizeof(int);
    quick_sort(num_arr, length);
    std::cout << "sorted result is ";
    for (int i = 0; i < length; i++) {
        std::cout << " " << num_arr[i];
    }
    std::cout << std::endl;    
}

template<typename T>
std::list<T> sequential_quick_sort(std::list<T> input) {
    if (input.empty()) {
        return input;
    }
    std::list<T> result;
    result.splice(result.begin(), input, input.begin()); // 将 input 列表中的第一个元素移动到 result 列表的起始位置，并且在 input 列表中删除该元素
    T const& pivot = *result.begin(); // 取首元素作为 x
    // partition 分区函数，使得满足条件的元素排在不满足条件元素之前。divide_point指向的是input中第一个大于等于pivot的地址
    auto divide_point = std::partition(input.begin(), input.end(),
        [&](T const& t){return t < pivot;});
    std::list<T> lower_part;
    lower_part.splice(lower_part.end(), input, input.begin(), divide_point); // 小于pivot的元素放在lower_part里
    auto new_lower(sequential_quick_sort(std::move(lower_part)));
    auto new_higher(sequential_quick_sort(std::move(input)));
    result.splice(result.end(), new_higher);
    result.splice(result.begin(), new_lower);
    return result;
}

void test_sequential_quick_sort() {
    std::list<int> num_lists = {5, 3, 7, 6, 4, 1, 0, 2, 9, 10, 8};
    auto sort_result = sequential_quick_sort(num_lists);
    std::cout << "sorted result is ";
    for (auto &iter : sort_result) {
        std::cout << " " << iter;
    }
    std::cout << std::endl;    
}

template<typename T>
std::list<T> parallel_quick_sort(std::list<T> input) {
    if (input.empty()) {
        return input;
    }
    std::list<T> result;
    result.splice(result.begin(), input, input.begin());
    T const& pivot = *result.begin(); 

    auto divide_point = std::partition(input.begin(), input.end(),
        [&](T const& t){return t < pivot;});
    std::list<T> lower_part;
    lower_part.splice(lower_part.end(), input, input.begin(), divide_point);
    std::future<std::list<T>> new_lower(std::async(parallel_quick_sort<T>, std::move(lower_part)));
    auto new_higher(parallel_quick_sort(std::move(input)));
    result.splice(result.end(), new_higher);
    result.splice(result.begin(), new_lower.get());
    return result;
}

void test_parallel_quick_sort() {
    std::list<int> num_lists = {5, 3, 7, 6, 4, 1, 0, 2, 9, 10, 8};
    auto sort_result = parallel_quick_sort(num_lists);
    std::cout << "sorted result is ";
    for (auto &iter : sort_result) {
        std::cout << " " << iter;
    }
    std::cout << std::endl;    
}

#include "thread_pool.h"
template<typename T>
std::list<T> thread_pool_quick_sort(std::list<T> input) {
    if (input.empty()) {
        return input;
    }
    std::list<T> result;
    result.splice(result.begin(), input, input.begin());
    T const& pivot = *result.begin(); 

    auto divide_point = std::partition(input.begin(), input.end(),
        [&](T const& t){return t < pivot;});
    std::list<T> lower_part;
    lower_part.splice(lower_part.end(), input, input.begin(), divide_point);
    auto new_lower = ThreadPool::instance().commit(&parallel_quick_sort<T>, std::move(lower_part)); // 投递给线程池处理
    auto new_higher(parallel_quick_sort(std::move(input)));
    result.splice(result.end(), new_higher);
    result.splice(result.begin(), new_lower.get());
    return result;
}

void test_thread_pool_quick_sort() {
    std::list<int> num_lists = {5, 3, 7, 6, 4, 1, 0, 2, 9, 10, 8};
    auto sort_result = thread_pool_quick_sort(num_lists);
    std::cout << "sorted result is ";
    for (auto &iter : sort_result) {
        std::cout << " " << iter;
    }
    std::cout << std::endl;    
}

#include <cstdlib>
#include <chrono>
#include <random>
template<typename T>
std::list<T> generate_random_list(int N) {
    std::list<T> num_list;
    std::srand(std::time(0));
    for (int i = 0; i < N; ++i) {
        num_list.push_back(std::rand() % N + 1); // 生成 1 到 N 之间的随机数
    }
    return num_list;
}

// std::list<T> generate_random_list(int N) {
//     std::list<T> num_list;
//     std::mt19937 gen(std::random_device{}()); // 使用硬件随机数生成器作为种子
//     std::uniform_int_distribution<T> dist(1, N); // 生成 1 到 N 之间的随机数
//     for (int i = 0; i < N; ++i) {
//         num_list.push_back(dist(gen));
//     }
//     return num_list;
// }

template<typename T>
std::list<T> test_sort_func(int N, std::list<T> num_list, std::list<T>(*sort_func)(std::list<T>), std::string sort_name) {
    auto start_time = std::chrono::steady_clock::now();
    std::list<T> sorted_list = sort_func(num_list);
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    std::cout << sort_name << " " << duration.count() << "(ms)" << std::endl;
    return sorted_list;
}
template<typename T>
bool is_sorted(const std::list<T>& lst) {
    auto it = lst.begin();
    if (it == lst.end()) {
        return true; 
    }
    auto prev = it;
    ++it;
    while (it != lst.end()) {
        if (*prev > *it) {
            return false; 
        }
        ++prev;
        ++it;
    }
    return true; 
}

void test_time(int N) {
    std::list<int> num_list = generate_random_list<int>(N);
    auto num_list_copy_sequential(num_list);
    auto num_list_copy_parallel(num_list);
    auto num_list_copy_thread_pool(num_list);

    auto sort_result1 = test_sort_func(N, num_list_copy_sequential, &sequential_quick_sort<int>, "sequential_quick_sort");
    auto sort_result2 = test_sort_func(N, num_list_copy_parallel, &parallel_quick_sort<int>, "parallel_quick_sort");
    auto sort_result3 = test_sort_func(N, num_list_copy_thread_pool, &thread_pool_quick_sort<int>, "thread_pool_quick_sort");
    
    bool is_sorted1 = is_sorted(sort_result1);
    bool is_sorted2 = is_sorted(sort_result2);
    bool is_sorted3 = is_sorted(sort_result3);
    bool same_result = is_sorted1 && is_sorted2 && is_sorted3;
    if (!same_result){
        std::cout << "Sorted results are different." << std::endl;
    }
}

int main(int argc, char** argv) {
    // test_quick_sort();
    // test_sequential_quick_sort();
    // test_parallel_quick_sort();
    // test_thread_pool_quick_sort();
     if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <N>" << std::endl;
        return 1;
    }
    int N = std::stoi(argv[1]);
    test_time(N);
    return 0;
}