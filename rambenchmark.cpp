#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <time.h>
#include <limits>
#include <omp.h>
#include <thread>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <iomanip>
#include <map>

#define STRINGIFY(x) #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

using namespace std;

#define PRECISION (4)

const long long int BUFFER_SIZE = 1ll * 1024 * 1024 * 1024 * 3;
char *BUFFER;

std::stringstream nullbuff;

typedef struct
{
	timespec start;
	timespec stop;
	timespec sum;
} t_timemes;
#define t_init(t)      \
	t.sum.tv_nsec = 0; \
	t.sum.tv_sec = 0
#define t_start(t) clock_gettime(CLOCK_MONOTONIC, &(t.start))
#define t_stop(t)                                   \
	clock_gettime(CLOCK_MONOTONIC, &(t.stop));      \
	t.sum.tv_sec += t.stop.tv_sec - t.start.tv_sec; \
	t.sum.tv_nsec += t.stop.tv_nsec - t.start.tv_nsec
#define t_get_seconds(t) (double)t.sum.tv_sec + (double)t.sum.tv_nsec / (double)1000000000

inline double test_memset()
{
	t_timemes time = {0};
	t_init(time);
	t_start(time);

#pragma omp parallel for
	for (long long i = 0; i < BUFFER_SIZE; i += 1048576)
	{
		memset(BUFFER + i, 0, 1048576);
	}
	t_stop(time);

	return t_get_seconds(time);
}

inline double test_memchr()
{
	t_timemes time = {0};
	t_init(time);
	t_start(time);

#pragma omp parallel for
	for (long long i = 0; i < BUFFER_SIZE; i += 1048576)
	{
		char *c = NULL;
		c = (char *)memchr(BUFFER + i, 'Q', 1048576);
		if (c != NULL)
		{
			nullbuff << "Char on returned position: " << c[0] << endl;
			nullbuff << "Position                 : " << (long long)(c - BUFFER) << endl;
		}
	}
	t_stop(time);

	return t_get_seconds(time);
}

map<int, vector<double> > run_test(double (*fun)(), int nloops, int threads)
{
	map<int, vector<double> > bench_times;
	for (int th = 1; th <= threads; th++)
	{
		omp_set_num_threads(th);
		bench_times.insert(pair<int, vector<double> >(th, vector<double>()));
		for (int i = 0; i < nloops; i++)
		{
			bench_times[th].push_back(fun());
		}
	}
	return bench_times;
}

double get_worst(map<int, vector<double> > res, int th)
{
	return *max_element(res[th].begin(), res[th].end());
}

pair<int, double> get_worst(map<int, vector<double> > res)
{
	auto tmp = (*max_element(res.begin(), res.end(),
							 [](pair<const int, vector<double> > a, pair<const int, vector<double> > b)
							 {
								 double amax = *max_element(a.second.begin(), a.second.end());
								 double bmax = *max_element(b.second.begin(), b.second.end());
								 return amax < bmax;
							 }));
	return pair<int, double>(tmp.first, *max_element(tmp.second.begin(), tmp.second.end()));
}

double get_best(map<int, vector<double> > res, int th)
{
	return *min_element(res[th].begin(), res[th].end());
}

pair<int, double> get_best(map<int, vector<double> > res)
{
	auto tmp = (*min_element(res.begin(), res.end(),
							 [](pair<const int, vector<double> > a, pair<const int, vector<double> > b)
							 {
								 double amax = *min_element(a.second.begin(), a.second.end());
								 double bmax = *min_element(b.second.begin(), b.second.end());
								 return amax < bmax;
							 }));
	return pair<int, double>(tmp.first, *min_element(tmp.second.begin(), tmp.second.end()));
}

void perform_benchmark()
{
	BUFFER = new char[BUFFER_SIZE];
	unsigned int nth = std::thread::hardware_concurrency();
	{
		cout << "WRITE TEST" << endl;
		map<int, vector<double> > res = run_test(test_memset, 10, nth);
		auto max_overall = get_best(res);
		auto min_overall = get_worst(res);
		cout << fixed << setprecision(PRECISION) << max_overall.second << " (s) / " << fixed << setprecision(1) << ((double)(BUFFER_SIZE / 1000 / 1000)) / max_overall.second << " (MB/s)" << endl;
	}

	{
		cout << "READ TEST" << endl;
		nullbuff << "Last char                : " << BUFFER[BUFFER_SIZE - 1] << endl;
		BUFFER[BUFFER_SIZE - 1] = 'Q';

		map<int, vector<double> > res = run_test(test_memchr, 10, nth);
		auto max_overall = get_best(res);
		auto min_overall = get_worst(res);
		cout << fixed << setprecision(PRECISION) << max_overall.second << " (s) / " << fixed << setprecision(1) << ((double)(BUFFER_SIZE / 1000 / 1000)) / max_overall.second << " (MB/s)" << endl;
	}
}

int main()
{

	perform_benchmark();
	return 0;
}
