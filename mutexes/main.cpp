#include <chrono>
#include <complex.h>
#include <cstddef>
#include <cstring>
#include <memory>
#include <ostream>
#include <pthread.h>
#include <random>
#include <string>
#include <thread>
#include "mutexes.hpp"
#include <sched.h>
#include <iostream>
#include <vector>
#include <chrono>
#include "utils.hpp"

#define NLOCKS 1
#define BENCH_NSECS 3
#define ITERATIONS 1e6
int nthreads;

std::vector<std::unique_ptr<abstractLock>> vlocks;

size_t val = 0;

void thread_run(int cpu, timep_type start)
{
    set_thread_affinity(cpu, pthread_self());

    RandGenerator rand(NLOCKS-1);

    std::this_thread::sleep_until(start);
    for (int i = 0; i < ITERATIONS; i++) {
        size_t rand_idx = rand.generate();

        abstractLock& mtx = *vlocks[rand_idx];
        mtx.lock();
        val++;
        mtx.unlock();
    }
}

void thread_copier(int cpu, volatile char *buf1, volatile char *buf2, size_t bufsize)
{
    set_thread_affinity(cpu, pthread_self());

    double total_bytes = 0;
    auto total_time = std::chrono::nanoseconds(0);
    int copy_count = 0;
    
    while (val < ITERATIONS * nthreads) {
        auto t1 = now();
        //std::memcpy(mem_empty[dest], mem_empty[src], buf_sz);
        for (size_t i = 0; i < bufsize; i++) {
            buf1[i] = buf2[i];
        }
        auto t2 = now();
        
        auto copy_time = std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1);
        total_time += copy_time;
        total_bytes += bufsize;
        copy_count++;
        
        volatile char* tmp = buf1;
        buf2 = buf1;
        buf1 = tmp;
    }
    
    // Final bandwidth calculation
    if (copy_count > 0) {
        double avg_bandwidth_gbps = (total_bytes / 1e9) / (total_time.count() / 1e9);
        std::cout << "Average memcpy bandwidth: " << avg_bandwidth_gbps << " GB/s - avg time per copy: " << total_time.count()/1e9/copy_count << " sec" << std::endl;
    }
}

int main (int argc, char** argv)
{
    // 4GiB by default
    size_t bufsize = 1ULL<<32;
    if (argc > 1)
        bufsize = std::stoll(argv[1]);

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
    std::cout << std::endl << std::flush;

    // Allocating and generating arrays 
    std::cout << "Allocating and generating arrays (bytes="<< bufsize/(1ULL<<20) << "MiB each)..." << std::flush;
    RandGenerator r(255);
    char *buf1 = new char[bufsize];
    char *buf2 = new char[bufsize];
    for (size_t i = 0; i < bufsize; i++) {
        buf1[i] = r.generate();
        buf2[i] = r.generate();
    }
    std::cout << "finished." << std::endl << std::flush;

    // First, allocate mutexes
    vlocks.reserve(NLOCKS);
    for (int i = 0; i < NLOCKS; i++)
        vlocks.push_back(TASLock::factory());

    auto start = now() + std::chrono::seconds(1);

    std::vector<std::thread> vthr;
    vthr.reserve(nthreads);
    int last_cpu = -1;
    for (int cpu = 0; cpu < CPU_SETSIZE; cpu++) {
        if (CPU_ISSET(cpu, &proc_cpus)) {
            last_cpu = cpu;
            vthr.emplace_back(thread_run, cpu, start);
        }
    }

    std::thread copier(thread_copier, last_cpu+1, buf1, buf2, bufsize);
    set_thread_affinity(last_cpu+2, pthread_self());

    for (int i = 0; i < nthreads; i++)
        vthr[i].join();

    auto end = now();
    copier.join();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "cpus " << nthreads << " time " << duration.count() << " ms" << std::endl << std::endl;
}