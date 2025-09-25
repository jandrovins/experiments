#include <pthread.h>
#include <random>
#include <sched.h>
#include <chrono>

using clk_type = std::chrono::high_resolution_clock;
using timep_type = std::chrono::time_point<clk_type>;

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
