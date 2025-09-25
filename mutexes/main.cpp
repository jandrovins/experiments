#include <atomic>
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

#define NLOCKS 8
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
	//for (int i = 0; i < ITERATIONS; i++) {
	while (stop.load(std::memory_order_relaxed)) {
		size_t rand_idx = rand.generate();

		abstractLock& mtx = *vlocks[rand_idx];
		mtx.lock();
		val++;
		//volatile int dummy = 0;
		//for (int j = 0; j < 10; j++) dummy++;
		mtx.unlock();
	}
}

void thread_copier(timep_type start, int cpu, volatile char *buf1, volatile char *buf2, size_t bufsize, const double baseline_bandwidth)
{
	std::this_thread::sleep_until(start);
	double contended_bandwidth = measure_copy_bandwidth(cpu, buf1, buf2, bufsize, BENCH_NSECS);
	std::cout << "Contended bandwidth: " << contended_bandwidth << " GB/s" << std::endl;
	std::cout << "Impact: " << ((baseline_bandwidth - contended_bandwidth) / baseline_bandwidth * 100) 
				<< "% reduction" << std::endl;
}

std::atomic<bool> stop;
int main (int argc, char** argv)
{
	stop = false;

	// 4GiB by default
	size_t bufsize = 1ULL<<26;
	if (argc > 1)
		bufsize = std::stoll(argv[1]);

	cpu_set_t proc_cpus;
	CPU_ZERO(&proc_cpus);
	int ret = sched_getaffinity(0, sizeof(cpu_set_t), &proc_cpus);
	if (ret == -1)
		perror("Error in sched_getaffinity()");

	// Reserve 1 cpu for copier
	nthreads = CPU_COUNT(&proc_cpus) - 1;
	std::cout << "Allowed CPUs: ";
	print_cpu_ranges(proc_cpus);
	std::cout << std::endl << std::flush;

	std::vector<int> available_cpus;
	int waiter_cpu = -1;
	for (int cpu = 0; cpu < CPU_SETSIZE; cpu++) {
		if (CPU_ISSET(cpu, &proc_cpus))
			available_cpus.push_back(cpu);
		else if (waiter_cpu == -1)
			waiter_cpu = cpu;
	}

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

	// BASELINE MEASUREMENT (no contention)
	std::cout << "\n=== BASELINE (no mutex contention) ===" << std::endl;
	double baseline_bandwidth = measure_copy_bandwidth(available_cpus.back(), buf1, buf2, bufsize, BENCH_NSECS);

	std::cout << "\n=== WITH MUTEX CONTENTION ===" << std::endl;
	stop.store(false);
	val = 0;

	auto start = now() + std::chrono::milliseconds(1000);

	std::vector<std::thread> vthr;
	vthr.reserve(nthreads);
	for (size_t i = 0; i < available_cpus.size() - 1; i++) {
		vthr.emplace_back(thread_run, available_cpus[i], start);
	}

	std::thread copier(thread_copier, start, available_cpus.back(), buf1, buf2, bufsize, baseline_bandwidth);

	set_thread_affinity(waiter_cpu, pthread_self());
	std::this_thread::sleep_for(std::chrono::seconds(BENCH_NSECS+1));
	stop.store(true);

    for (auto& t : vthr) {
        if (t.joinable()) t.join();
    }
    if (copier.joinable()) copier.join();

    std::cout << "\nTotal mutex operations: " << val << std::endl;

    delete[] buf1;
    delete[] buf2;
}