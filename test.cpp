#include <thread>
#include <unistd.h>
#include <iostream>
#include <atomic>
#include <unordered_set>
#include <unordered_map>
#include <chrono>
#include <bytell_hash_map.hpp>
//#include <crnlib.h>
#include <string.h>
#include <crn_assert.h>
#include <crn_types.h>
#include <crn_helpers.h>
#include <crn_traits.h>
#include <crn_mem.h>
#include <crn_math.h>
#include <crn_platform.h>
#include <crn_core.h>
#include <crn_utils.h>
#include <crn_vector.h>
#include <crn_hash_map.h>
#include <map>
#include <tpie/hash_map.h>
#include <tpie/tpie.h>

std::size_t allocated = 0;
std::size_t allocations = 0;

std::size_t max_allocations = 0;
std::size_t max_allocated = 0;
std::atomic_size_t max_rss;
std::atomic_bool stop;

template <typename T>
struct test_allocator {
  typedef T value_type;
  typedef size_t size_type;
  typedef std::ptrdiff_t difference_type;
  typedef std::true_type is_always_equal;

  test_allocator() = default;
  template <typename U>
  test_allocator(const test_allocator<U> & o) {}
  
  T * allocate(size_t n) {
    allocated += sizeof(T) * n;
    allocations += 1;
    max_allocations = std::max(allocations, max_allocations);
    max_allocated = std::max(allocated, max_allocated);
    return new T[n];
  }

  void deallocate(T * p, size_t n) {
    allocated -= sizeof(T) * n;
    allocations -= 1;
    delete[] p;
  }
};

void stat() {
  size_t ps = sysconf( _SC_PAGESIZE);
  size_t _max_rss = 0;
  while (!stop) {
    usleep(10000);
    long rss = 0L;
    FILE* fp = NULL;
    if ( (fp = fopen( "/proc/self/statm", "r" )) == NULL ) continue;
    if ( fscanf( fp, "%*s%ld", &rss ) != 1 ) continue;
    fclose(fp);
    _max_rss = std::max(_max_rss, rss * ps);
    max_rss = _max_rss;
  }
}



struct Hash {
  size_t operator()(int v) const noexcept {
    return v;
  }
};


    
typedef std::chrono::high_resolution_clock hpet;


struct Time {
  double v;
  Time(double v): v(v) {}

  friend std::ostream & operator << (std::ostream & o, const Time & t) {
    if (t.v > 5) return o << t.v << " s";
    return o << t.v * 1000 << " ms";
  }
};

struct Size {
  size_t v;
  Size(size_t v): v(v) {}

  friend std::ostream & operator << (std::ostream & o, const Size & t) {
    if (t.v < 15*1024) return o << t.v << " b";
    if (t.v < 15*1024*1024) return o << (t.v / 1024) << " kb";
    if (t.v < 15ll*1024ll*1024ll*1024) return o << (t.v / 1024/1024) << " mb";
    return o << (t.v / 1024/1024 / 1024) << " gb";
  }
};


template <typename T>
void dump(T start, T end, const char * message) {
  std::chrono::duration<double> elapsed_seconds = end-start;
  
  std::cout << message << "; "
	    << "time: " << Time(elapsed_seconds.count()) << "; "
	    << "max_rss: " << Size(max_rss) << "; "
	    << "max_allocated: " << Size(max_allocated) << "; "
    	    << "max_allocations: " << max_allocations << "; " << std::endl;
}



struct GenLinear {
  uint32_t operator()(uint32_t i) const noexcept {
    return i;
  }
};

struct GenPerm {
  uint32_t operator()(uint32_t i) const noexcept {
    return i * 4294963093;
  }
};



template <typename S>
void test_linear_set(int count) {
  S s;
  s.reserve(count);
  auto t1 = hpet::now();
  for (int i=0; i < count; i += 4)
    s.insert(i);
  auto t2 = hpet::now();

  dump(t1, t2, "insert");

  size_t cnt = 0;
  
  for (int i=1; i < count; i += 4)
    cnt += s.count(i);

  auto t3 = hpet::now();
  dump(t2, t3, "find_missing");
  
  for (int i=0; i < count; i += 4)
    cnt += s.count(i);

  auto t4 = hpet::now();
  dump(t3, t4, "find_existing");

  for (int i=0; i < count; i += 4)
    s.erase(i);

  auto t5 = hpet::now();
  dump(t4, t5, "erase");
  std::cout << cnt << std::endl;
}

template <typename S, typename Gen, typename V, typename R>
void test_map4(uint32_t count, Gen gen, V v, R r) {
  S s;
  r(s, count/4);
  auto t1 = hpet::now();
  for (uint32_t i=0; i < count; i += 4)
    s.insert(std::make_pair(gen(i), v));
  auto t2 = hpet::now();

  dump(t1, t2, "insert");
  std::cout << s.size() << std::endl;
  size_t cnt = 0;
  
   for (uint32_t i=1; i < count; i += 4)
     cnt += (s.find(gen(i)) == s.end()?1:0);

  auto t3 = hpet::now();
  dump(t2, t3, "find_missing");
  
  for (uint32_t i=0; i < count; i += 4)
    cnt += (s.find(gen(i)) == s.end()?1:0);

  auto t4 = hpet::now();
  dump(t3, t4, "find_existing");

  for (uint32_t i=0; i < count; i += 4)
    s.erase(gen(i));

  auto t5 = hpet::now();
  dump(t4, t5, "erase");
  std::cout << cnt << std::endl;
}

template <typename S, typename Gen, typename V>
void test_map4_tpie(uint32_t count, Gen gen, V v) {
  S s;
  s.resize(count);
  auto t1 = hpet::now();
  for (uint32_t i=0; i < count; i += 4)
    s.insert(gen(i), v);
  auto t2 = hpet::now();

  dump(t1, t2, "insert");

  size_t cnt = 0;
  
   for (uint32_t i=1; i < count; i += 4)
     cnt += (s.find(gen(i)) == s.end()?1:0);

  auto t3 = hpet::now();
  dump(t2, t3, "find_missing");
  
  for (uint32_t i=0; i < count; i += 4)
    cnt += (s.find(gen(i)) == s.end()?1:0);

  auto t4 = hpet::now();
  dump(t3, t4, "find_existing");

  for (uint32_t i=0; i < count; i += 4)
    s.erase(gen(i));

  auto t5 = hpet::now();
  dump(t4, t5, "erase");
  std::cout << cnt << std::endl;
}


template <typename Gen, typename V>
void test_map3(uint32_t cnt, std::string map) {
  typedef std::unordered_map<uint32_t, V, std::hash<uint32_t>, std::equal_to<uint32_t>, test_allocator<uint32_t>> std_map;
  typedef ska::bytell_hash_map<uint32_t, V, Hash, std::equal_to<uint32_t>, test_allocator<uint32_t> > ska_map;
  typedef ska::flat_hash_map<uint32_t, V, Hash, std::equal_to<uint32_t>, test_allocator<uint32_t> > flat_map;
  typedef crnlib::hash_map<uint32_t, V, Hash, std::equal_to<uint32_t>> crn_map;
  typedef std::map<uint32_t, V, std::less<uint32_t>, test_allocator<uint32_t> > ordered_map;
  typedef tpie::hash_map<uint32_t, V> tpie_map;
  
  if (map == "std")
    test_map4<std_map>(cnt, Gen(), V(), [](auto m, auto cnt) {m.reserve(cnt/4);});
  else if (map == "ska")
    test_map4<ska_map>(cnt, Gen(), V(), [](auto m, auto cnt) {m.reserve(cnt/4);});
  else if (map == "flat")
    test_map4<flat_map>(cnt, Gen(), V(), [](auto m, auto cnt) {m.reserve(cnt/4);});
  else if (map == "crn")
    test_map4<crn_map>(cnt, Gen(), V(), [](auto m, auto cnt) {m.reserve(cnt/4);});
  else if (map == "ordered")
    test_map4<ordered_map>(cnt, Gen(), V(), [](auto m, auto cnt) {});
  else if (map == "tpie")
    test_map4_tpie<tpie_map>(cnt, Gen(), V());
}

template <typename Gen>
void test_map2(uint32_t cnt, std::string map, std::string size) {
  if (size == "large")
    test_map3<Gen, std::array<uint32_t, 16>>(cnt, map);
  else
    test_map3<Gen, uint32_t>(cnt, map);  
}

void test_map(uint32_t cnt, std::string map, std::string size, std::string order) {
  if (order == "linear")
    test_map2<GenLinear>(cnt, map, size);
  else
    test_map2<GenPerm>(cnt, map, size);
}

int main(int arc, char ** argv) {
  tpie::tpie_init(tpie::ALL);
  stop = false;
  std::thread t(stat);

  std::string test = argv[1];
  std::string map = argv[2];
  std::string order = argv[3];
  std::string size = argv[4];
  size_t cnt = atoi(argv[5]);

  usleep(100000);
  std::cout << "start; " << test << " " << map << " " << cnt << "; "
	    << "max_rss: " << Size(max_rss) << "; "
	    << "max_allocated: " << Size(max_allocated) << "; "
    	    << "max_allocations: " << max_allocations << "; " << std::endl;
  
  if (test == "linear_set") {
    if (map == "std")
      test_linear_set< std::unordered_set<int, std::hash<int>, std::equal_to<int>, test_allocator<int> > > (cnt);
    else if (map == "ska")
      test_linear_set< ska::bytell_hash_set<int, Hash, std::equal_to<int>, test_allocator<int> > > (cnt);
  } else if (test == "linear_map") {
    test_map(cnt, map, size, order);
  }
  
  
  stop = true;
  t.join();
  
  std::cout << "end; "
	    << "max_rss: " << Size(max_rss) << "bytes; "
	    << "max_allocated: " << Size(max_allocated) << "bytes; "
    	    << "max_allocations: " << max_allocations << "; " << std::endl;
  
}
