#include <algorithm>
#include <cassert>
#include <iostream>
#include <omp.h>
#include <random>
#include <string>
#include <vector>

constexpr auto DISTANCE_THRESHOLD = 100;
constexpr auto SEED = 41;

auto get_random_numbers(std::size_t n) {
  std::vector<double> array(n);
#pragma omp parallel default(none) shared(array, n)
  {
    std::mt19937 e{SEED};
    std::uniform_real_distribution<double> uniform_dist(0.0, 1.0);

#pragma omp for
    for (std::size_t idx = 0; idx < n; ++idx)
      array[idx] = uniform_dist(e);
  }

  return array;
}

template <typename It> auto partition(It const first, It const last) {
  auto pivot = *first;
  auto mid = first;
  for (auto it = last - 1; it > mid;) {
    if (*it < pivot) {
      std::iter_swap(it, ++mid);
    } else {
      --it;
    }
  }
  std::iter_swap(first, mid);
  return mid;
}

template <typename It> void quicksort_serial(It const first, It const last) {
  auto distance = std::distance(first, last);
  if (distance < 2)
    return;
  auto mid = partition(first, last);

  quicksort_serial(first, mid);
  quicksort_serial(mid + 1, last);
}

template <typename It>
void quicksort_par_sections(It const first, It const last) {
  auto distance = std::distance(first, last);
  if (distance < DISTANCE_THRESHOLD)
    return quicksort_serial(first, last);
  auto mid = partition(first, last);

#pragma omp parallel sections default(none) shared(first, mid, last)
  {
#pragma omp section
    quicksort_par_sections(first, mid);
#pragma omp section
    quicksort_par_sections(mid + 1, last);
  }
}

template <typename It>
void quicksort_par_tasks_helper(It const first, It const last) {
  auto distance = std::distance(first, last);
  if (distance < DISTANCE_THRESHOLD)
    return quicksort_serial(first, last);
  auto mid = partition(first, last);

#pragma omp task default(none) shared(first, mid)
  quicksort_par_tasks_helper(first, mid);
#pragma omp task default(none) shared(mid, last)
  quicksort_par_tasks_helper(mid + 1, last);
#pragma omp taskwait
}

template <typename It> void quicksort_par_tasks(It first, It last) {
#pragma omp parallel default(none) shared(first, last)
  {
#pragma omp single
    quicksort_par_tasks_helper(first, last);
  }
}

int main(int argc, const char **argv) {
  using namespace std::string_literals;

  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " N {ser,sec,task}\n";
    return EXIT_FAILURE;
  }

  auto n = std::stoull(argv[1]);
  auto quicksort = [argv](auto first, auto last) {
    return "ser"s == argv[2]   ? quicksort_serial(first, last)
           : "sec"s == argv[2] ? quicksort_par_sections(first, last)
           : "task"s == argv[2]
               ? quicksort_par_tasks(first, last)
               : throw std::runtime_error("must be one of '{ser, sec, task}");
  };

  auto array = get_random_numbers(n);

  auto t_start = omp_get_wtime();
  quicksort(std::begin(array), std::end(array));
  auto t_end = omp_get_wtime();
  std::cout << "elapsed time: " << t_end - t_start << "\n";
  assert(std::is_sorted(std::begin(array), std::end(array)));
  return 0;
}