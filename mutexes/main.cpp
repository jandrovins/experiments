#include <chrono>
#include <complex.h>
#include <cstddef>
#include <cstring>
#include <memory>
#include <random>
#include <thread>
#include "mutexes.hpp"
#include <sched.h>
#include <iostream>
#include <vector>
#include <chrono>

#define CLK_TYPE std::chrono::high_resolution_clock

#define NLOCKS 1
#define BENCH_NSECS 3
#define ITERATIONS 1e5
int nthreads;

std::atomic<bool> should_end;
std::vector<std::unique_ptr<abstractLock>> vlocks;

constexpr size_t buf_sz = 1<<27 / 8; // 128 MiB on doubles
CACHE_ALIGNED double *mem_empty[2];

size_t val = 0;

void thread_run(std::chrono::time_point<CLK_TYPE> start)
{
    std::random_device rd;  // a seed source for the random number engine
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, NLOCKS-1);

    std::this_thread::sleep_until(start);
    for (int i = 0; i < ITERATIONS; i++) {
        size_t rand_idx = dist(gen);
        abstractLock& mtx = *vlocks[rand_idx];
        mtx.lock();
        val++;
        mtx.unlock();
    }
}

void thread_copier()
{
    int src = 0;
    int dest = 0;
    double total_bytes = 0;
    auto total_time = std::chrono::nanoseconds(0);
    int copy_count = 0;
    
    while (val < ITERATIONS * nthreads) {
        auto t1 = CLK_TYPE::now();
        std::memcpy(mem_empty[dest], mem_empty[src], buf_sz * 8);
        auto t2 = CLK_TYPE::now();
        
        auto copy_time = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);
        total_time += copy_time;
        total_bytes += buf_sz * 8;
        copy_count++;
        
        int tmp = src;
        src = dest;
        dest = tmp;
    }
    
    // Final bandwidth calculation
    if (copy_count > 0) {
        double avg_bandwidth_gbps = (total_bytes / 1e9) / (total_time.count() / 1e9);
        std::cout << "Average memcpy bandwidth: " << avg_bandwidth_gbps << " GB/s" << std::endl;
    }
}

int main ()
{
    cpu_set_t proc_cpus;
    CPU_ZERO(&proc_cpus);

    int ret = sched_getaffinity(0, sizeof(cpu_set_t), &proc_cpus);
    if (ret == -1)
        perror("Error in sched_getaffinity()");

    nthreads = CPU_COUNT(&proc_cpus);
    std::cout << "Allowed CPUs: ";
    for (int cpu = 0; cpu < CPU_SETSIZE; cpu++) {
        if (CPU_ISSET(cpu, &proc_cpus))
            std::cout << cpu << " ";
    }

    // Atomic to signal when to finish
    should_end.store(false);

    mem_empty[0] = new double[buf_sz];
    mem_empty[1] = new double[buf_sz];
    for (size_t i = 0; i < buf_sz; i++) {
        mem_empty[0][i] = static_cast<double>(i);
        mem_empty[1][i] = static_cast<double>(i);
    }

    // First, allocate mutexes
    vlocks.reserve(NLOCKS);
    for (int i = 0; i < NLOCKS; i++)
        vlocks.push_back(TASLock::factory());

    auto start = CLK_TYPE::now() + std::chrono::seconds(1);

    std::vector<std::thread> vthr;
    vthr.reserve(nthreads);
    for (int i = 0; i < nthreads; i++)
        vthr.emplace_back(thread_run, start);

    std::thread copier(thread_copier);

    for (int i = 0; i < nthreads; i++)
        vthr[i].join();

    auto end = CLK_TYPE::now();
    copier.join();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "\nTime measured: " << duration.count() << " ms" << std::endl;
}