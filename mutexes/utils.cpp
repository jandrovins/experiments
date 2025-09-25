#include "utils.hpp"
#include <cstring>
#include <iostream>

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

double measure_copy_bandwidth(int cpu, volatile char *buf1, volatile char *buf2, size_t bufsize, int duration_seconds)
{
	set_thread_affinity(cpu, pthread_self());

	double total_bytes = 0;
	auto start_time = now();
	auto end_time = start_time + std::chrono::seconds(duration_seconds);
	int copy_count = 0;
	
	while (now() < end_time && !stop.load()) {
		// Actual memory copy, not pointer assignment
		std::memcpy((void*)buf1, (void*)buf2, bufsize);
		
		total_bytes += bufsize;
		copy_count++;
		
		// Swap buffers
		volatile char* tmp = buf1;
		buf1 = buf2;
		buf2 = tmp;
	}
	stop.store(0);
	
	auto actual_duration = now() - start_time;
	double duration_sec = std::chrono::duration<double>(actual_duration).count();
	
	if (copy_count > 0 && duration_sec > 0) {
		double bandwidth_gbps = (total_bytes / 1e9) / duration_sec;
		double avg_memcpy_duration = duration_sec / copy_count;
		std::cout << "Bandwidth: " << bandwidth_gbps << " GB/s (copies: " << copy_count 
				  << ", duration: " << duration_sec << "s, avg memcpy: " << avg_memcpy_duration << "s)" << std::endl;
		return bandwidth_gbps;
	}
	return 0.0;
}

void print_cpu_ranges(const cpu_set_t& proc_cpus) {
	int start_range = -1;
	int end_range = -1;
	bool first_range = true;
	
	for (int cpu = 0; cpu < CPU_SETSIZE; cpu++) {
		if (CPU_ISSET(cpu, &proc_cpus)) {
			if (start_range == -1) {
				start_range = cpu;
				end_range = cpu;
			} else if (cpu == end_range + 1) {
				end_range = cpu;
			} else {
				// Print previous range
				if (!first_range) std::cout << ", ";
				if (start_range == end_range) {
					std::cout << start_range;
				} else {
					std::cout << start_range << "-" << end_range;
				}
				first_range = false;
				start_range = cpu;
				end_range = cpu;
			}
		}
	}
	
	// Print final range
	if (start_range != -1) {
		if (!first_range) std::cout << ", ";
		if (start_range == end_range) {
			std::cout << start_range;
		} else {
			std::cout << start_range << "-" << end_range;
		}
	}
}