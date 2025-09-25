#include "utils.hpp"

timep_type now()
{
    return clk_type::now();
}

void set_thread_affinity(const int cpu, const pthread_t tid)
{
	// Create a cpu_set_t object representing a set of CPUs. Clear it and mark
	// only CPU i as set.
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(cpu, &cpuset);

	// Now, set affinity of thread
	int rc = pthread_setaffinity_np(tid,
									sizeof(cpu_set_t), &cpuset);
	if (rc != 0)
		perror("Could not set affinity to thread");
}