#include <pthread.h>
#include <random>
#include <sched.h>
#include <chrono>
#include <atomic>

using clk_type = std::chrono::high_resolution_clock;
using timep_type = std::chrono::time_point<clk_type>;

extern std::atomic<bool> stop;

timep_type now();

// Includes max
size_t get_rand(const size_t max);

class RandGenerator {
public:
	std::mt19937 gen;
	std::uniform_int_distribution<size_t> dist;

    RandGenerator(size_t max=0) : gen(1111), dist(0, max) {};

    size_t generate(void)
    {
        return dist(gen);
    }
};

void set_thread_affinity(const int cpu, const pthread_t tid);

double measure_copy_bandwidth(int cpu, volatile char *buf1, volatile char *buf2, size_t bufsize, int duration_seconds);

void print_cpu_ranges(const cpu_set_t& proc_cpus);