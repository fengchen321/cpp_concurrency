#include <iostream>
#include <thread>
#include <vector>
#include <numeric> 

void some_function(){
    std::cout << "some_function " << std::endl;
}
void some_other_function(){
    std::cout << "some_other_function " << std::endl;
}
void dangerous_use() {
	//t1 绑定some_function
	std::thread t1(some_function);
	//2 转移t1管理的线程给t2，转移后t1无效
	std::thread t2 = std::move(t1);
	//3 t1 可继续绑定其他线程,执行some_other_function
	t1 = std::thread(some_other_function);
	//4  创建一个线程变量t3
	std::thread t3;
	//5  转移t2管理的线程给t3
	t3 = std::move(t2);
	//6  转移t3管理的线程给t1
    // t1 = std::move(t3);  // 将一个线程的管理权交给一个已经绑定线程的变量，会触发线程的terminate函数引发崩溃
    t3.join(); 
    t1.join();
}
void param_function(int a) {
    std::cout << "param is " << a << std::endl;
	std::this_thread::sleep_for(std::chrono::seconds(1));
}
void use_vector() {
    unsigned int N = std::thread::hardware_concurrency(); 
    std::vector<std::thread> threads;
    for (unsigned int i = 0; i < N; ++i) {
        threads.emplace_back(param_function, i);
    }
    for (auto& entry : threads) {
        if (entry.joinable()) {
            entry.join();
        } 
    }
    threads.clear();
}

template<typename Iterator, typename T>
struct accumulate_block{
    void operator()(Iterator first, Iterator last, T& result){
        result = std::accumulate(first, last, result);
    }
};

template<typename Iterator, typename T>
T parallel_accumulate(Iterator first, Iterator last, T init){
    unsigned long const length = std::distance(first, last);
    if (!length) {  // 1.输入为空，返回初始值init
        return init;
    }
    unsigned long const min_per_thread = 25;
    unsigned long const max_threads = (length + min_per_thread - 1) / min_per_thread;  // 2.需要的线程最大数（向上取整）
    unsigned long const hardware_threads = std::thread::hardware_concurrency();
    unsigned long const num_threads = std::min(hardware_threads!=0 ? hardware_threads : 2, max_threads); // 3.实际的线程选择数量
    unsigned long const block_size = length / num_threads; // 4.每个线程待处理的条目数量，步长

    std::vector<T> results(num_threads);
    std::vector<std::thread> threads(num_threads - 1);  // 5.初始化了(num_threads - 1)个大小的vector，因为主线程也参与计算

    Iterator block_start = first;
    for (unsigned long i = 0; i < num_threads - 1; ++i){
        Iterator block_end = block_start;
        std::advance(block_end, block_size);  // 6. 递进block_size迭代器到当前块的结尾
        threads[i] = std::thread(accumulate_block<Iterator, T>(), block_start, block_end, std::ref(results[i])); // 7.启动新的线程计算结果
        block_start = block_end;  // 8.更新起始位置
    }
    accumulate_block<Iterator, T>()(
        block_start, last, results[num_threads - 1]);     // 9. 主线程计算，处理最后的块
        for (auto& entry : threads){
            if (entry.joinable()){
                entry.join();
            }
        }
    return std::accumulate(results.begin(), results.end(), init);  // 10. 累加
}
void use_parallel_acc(int N) {
    auto start = std::chrono::high_resolution_clock::now();
    std::vector <int> vec(N);
    for (int i = 0; i < N; i++) {
        vec.push_back(i);
    }
    int sum = 0;
    sum = parallel_accumulate<std::vector<int>::iterator, int>(vec.begin(), vec.end(), sum);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> timeDuration = end - start;
    double duration = timeDuration.count();
    std::cout << "use_parallel_acc sum is " << sum << " duration: " << duration << std::endl;
}
void normal_acc(int N) {
    auto start = std::chrono::high_resolution_clock::now();
    int sum = 0;
    for (int i = 0; i < N; i++) {
        sum += i;
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> timeDuration = end - start;
    double duration = timeDuration.count();
    std::cout << "normal_acc sum is " << sum << " duration: " << duration << std::endl;
}

void do_subthread(){
    std::cout << "do sub thread work " << std::this_thread::get_id() << std::endl;
}

void thread_id(){
    std::thread::id master_thread = std::this_thread::get_id();
    std::thread t(do_subthread);
    std::cout << "do_subthread id: " << t.get_id() << std::endl; // 线程可能没运行，可能会返回一个空的 std::thread::id
    t.join();
    if (std::this_thread::get_id() == master_thread){
        std::cout << "do master thread work: "<<  std::this_thread::get_id() << std::endl;
    }
    std::cout << "do common thread work: " <<  std::this_thread::get_id() << std::endl;
}

int main()
{
    // 1. 线程归属
    dangerous_use();

    // 2. 容器存储
    use_vector();

    // 3. 选择运行数量
    use_parallel_acc(10000);
    normal_acc(10000);

    // 4. 识别线程
    thread_id();
    return 0;
}
