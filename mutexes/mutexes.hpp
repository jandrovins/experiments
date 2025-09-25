#include <atomic>
#include <memory>

#define CACHE_ALIGNED alignas(64)

inline void architecturalYield() {
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
  __builtin_ia32_pause();
#elif defined(__aarch64__) || defined(_M_ARM64)
  __asm__ volatile("yield" ::: "memory");
#elif defined(__arm__) || defined(_M_ARM)
  __asm__ volatile("yield" ::: "memory");
#else
  std::this_thread::yield();
#endif
}

// A base class so that we can have a simple interface to
// our timing operations and pass in a specific lock type
// to use. This does mean that we're doing an indirect call
// for each operation, but that is the same for all
// cases, and should be well predicted and cached.
class CACHE_ALIGNED abstractLock {
public:
  abstractLock() {}
  virtual ~abstractLock() {}
  virtual void lock() = 0;
  virtual void unlock() = 0;
  virtual char const *name() const = 0;
};

// A Test - and - Set lock
class TASLock : public abstractLock {
  CACHE_ALIGNED std::atomic<bool> locked;

public:
  TASLock() : locked(false) {}
  ~TASLock() {}

  void lock() override {
    bool expected = false;
    while (!locked.compare_exchange_weak(expected, true, std::memory_order_acquire)) {
      expected = false;
      // architecturalYield();
    }
  }

  void unlock() override {
    locked.store(false, std::memory_order_release);
  }

  char const *name() const override { return "TAS Lock"; }
  
  static std::unique_ptr<abstractLock> factory() {
    return std::make_unique<TASLock>();
  }
};