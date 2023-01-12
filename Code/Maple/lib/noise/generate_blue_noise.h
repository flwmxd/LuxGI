#pragma once
#ifndef __BLUE_NOISE_H__
#define __BLUE_NOISE_H__

#include <vector>

struct BlueNoiseGenerator {
	std::vector<std::vector<float>> white_noise;
	std::vector<std::vector<float>> blue_noise;
	int x_resolution=0, y_resolution=0, depth=1;
	int kernel_size = -1;
	int threads = 4;
	
	BlueNoiseGenerator() {}

	BlueNoiseGenerator(int x_resolution, int y_resolution, int depth = 1, int kernel_size = -1) 
		:x_resolution(x_resolution), y_resolution(y_resolution), depth(depth), kernel_size(kernel_size) {
		blue_noise.resize(x_resolution * y_resolution);
		white_noise.resize(x_resolution * y_resolution);
		_init();
	}


	void optimize(int max_iter) {
		return optimize(max_iter, false);
	}
	
	void optimize(int max_iter, bool verbose);

	float E();
private:
	void _init();
};

#endif