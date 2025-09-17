#include <atomic>

class channel {
  std ::atomic<bool> go_atomic;
  bool go_dummy;
  void *payload;

public:
  channel() : go_atomic(false) {}
  void send_good(void *data) {
	while (go_atomic.load(std ::memory_order_acquire))
	  ;
	/* Wait for data to be consumed */
	payload = data;
	go_atomic.store(true, std ::memory_order_release);
  }
  void *recv_good() {
	while (!go_atomic.load(std ::memory_order_acquire))
	  ;
	/* Wait for data to be written */
	void *result = payload;
	go_atomic.store(false, std ::memory_order_release);
	return result;
  }
  void send_bad(void *data) {
	while (go_dummy)
	  ;
	/* Wait for data to be consumed */
	payload = data;
	go_dummy = true;
  }
  void *recv_bad() {
	while (!go_dummy)
	  ;
	/* Wait for data to be written */
	void *result = payload;
	go_dummy = false;
	return result;
  }
};

//void send_good_ext(channel &ch, void *data) {
//	ch.send_good(data); 
//}
//
//void *recv_good_ext(channel &ch) {
//  void *data = ch.recv_good();
//  return data;
//}

void send_bad_ext(channel &ch, void *data) {
	ch.send_bad(data); 
}

void *recv_bad_ext(channel &ch) {
  void *data = ch.recv_bad();
  return data;
}