#include <vector>
#include <cmath>
#include <numeric>
#include <iostream>
#include <iterator>
#include "generate_blue_noise.h"


namespace pbar {

template<class It>
class ProgressBar {
public:
  ProgressBar(It it, It it_end, size_t width, const char symbol='=')
      : count_(0),
        width_(width), 
        size_(std::distance(it, it_end)),
        symbol_(symbol), 
        it_begin_(*this, std::move(it)),
        it_end_(*this, std::move(it_end))
  {
    size_ = size_? size_ : 1; // to prevent division by zero in notify()
  }

  ~ProgressBar() {
    std::clog << std::endl;
  }

  struct iterator;

  iterator begin() {
    count_ = 0;
    return it_begin_;
  }

  iterator end() {
    return it_end_;
  }

private:
  void notify() const;

  size_t count_;
  size_t width_;
  size_t size_;

  char symbol_;
  char left_delim_{'['};
  char right_delim_{']'};
  char pointer_{'>'};

  iterator it_begin_;
  iterator it_end_;

}; // class ProgressBar


template<class It>
struct ProgressBar<It>::iterator {

  using iterator_category = std::forward_iterator_tag;
  using value_type = typename It::value_type;
  using difference_type = typename It::difference_type;
  using pointer = typename It::pointer;
  using reference = typename It::reference;

public:
  iterator(ProgressBar<It>& parent, It iter) 
    : parent_(parent), iter_(std::move(iter)) {}

  iterator& operator++() {
    ++(iter_); 
    ++parent_.count_;
    parent_.notify();
    return *this; 
  }

  iterator operator++(int) {
    auto retval = *this;
    ++(*this);
    return retval;
  }

  bool operator==(const iterator& other) const {
    return iter_ == other.iter_;
  }
  
  bool operator!=(const iterator& other) const {
    return !(*this == other);
  }

  reference operator*() const {
    return *iter_;
  }

private:
  ProgressBar<It>& parent_;
  It iter_;

};


template<class It>
inline void ProgressBar<It>::notify() const {
  size_t pos_old = width_*(count_-1)/size_;
  size_t pos = width_*count_/size_;
  if (pos_old == pos)
    return;
  std::clog << left_delim_;
  for (size_t i=0; i<width_; i++) {
    if (i < pos)
      std::clog << symbol_;
    else if (i == pos)
      std::clog << pointer_;
    else
      std::clog << " ";
  }
  std::clog << right_delim_ << count_*100/size_ << "%\r";
  std::clog.flush();
}

} // namespace pbar

// random float number in range [0,1]
float NextFloat() {
	return  static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
}

// distance between two 2D points
float DistanceSquared(float x1, float y1, float x2, float y2) {
	return (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);
}

float DistanceSamples(const std::vector<float>& p, const std::vector<float>& q) {
	const int d = p.size();
	float res = 0.0f;
	for (size_t i = 0; i < p.size(); ++i) {
		res += std::pow(std::abs(p[i] - q[i]), (d / 2.0));
	}
	return res;
}


void BlueNoiseGenerator::_init()
{
	for (size_t i = 0; i < x_resolution * y_resolution; ++i) {
		for (size_t d = 0; d < depth; ++d) {
			float _t = NextFloat();
			white_noise[i].push_back(_t);
			blue_noise[i].push_back(_t);
		}
	}
}

float BlueNoiseGenerator::E()
{
	float loss = 0;
	const float sigma_i = 2.1f;
	const float sigma_s = 1.0f;

#pragma omp parallel for num_threads(threads) reduction(+: loss) // use openmp 
	for (int px = 0; px < x_resolution; ++px) {
		for (int py = 0; py < y_resolution; ++py) {
			int i = px * x_resolution + py;
			std::vector<float> ps = blue_noise[i];

			float pix = i / x_resolution, piy = i % x_resolution;

			if (kernel_size == -1) {
				// use all pixel pairs to compute the loss
				for (int qx = 0; qx < x_resolution; ++qx) {
					for (int qy = 0; qy < y_resolution; ++qy) {
						float qix = qx, qiy = qy;

						float pixel_distance = DistanceSquared(pix, piy, qix, qiy);

						int qqx = (qx + x_resolution) % x_resolution;
						int qqy = (qy + y_resolution) % y_resolution;

						int j = qqx * x_resolution + qqy;
						if (j == i) continue;
						std::vector<float> qs = blue_noise[j];

						float sample_distance = DistanceSamples(ps, qs);
						loss += std::exp(
							-(pixel_distance) / (sigma_i * sigma_i)
							- (sample_distance) / (sigma_s * sigma_s));
					}
				}
			}
			else {
				// use all neighborhood around the pixel (px,py) to approximate the loss
				for (int qx = pix - kernel_size; qx <= pix + kernel_size; ++qx) {
					for (int qy = piy - kernel_size; qy <= piy + kernel_size; ++qy) {
						float qix = qx, qiy = qy;

						float pixel_distance = DistanceSquared(pix, piy, qix, qiy);

						int qqx = (qx + x_resolution) % x_resolution;
						int qqy = (qy + y_resolution) % y_resolution;

						int j = qqx * x_resolution + qqy;
						if (j == i) continue;
						std::vector<float> qs = blue_noise[j];

						float sample_distance = DistanceSamples(ps, qs);
						loss += std::exp(
							-(pixel_distance) / (sigma_i * sigma_i)
							- (sample_distance) / (sigma_s * sigma_s));
					}
				}
			}
		}
	}
	return loss;
}

void BlueNoiseGenerator::optimize(int max_iter, bool verbose)
{
	
	// 2. random swap two pixels to minimize the loss
	float loss = E();
	int size = x_resolution * y_resolution;

	// pbar and verbose stuff
	int progress_iters = int(float(max_iter) / 10.0f);
	std::vector<int> progress_v(max_iter);
	std::iota(progress_v.begin(), progress_v.end(), 0);
	int pbar_width = 50;
	if (!verbose) {
		pbar_width = 0;
	}
	pbar::ProgressBar<std::vector<int>::iterator> pbar(progress_v.begin(), progress_v.end(), pbar_width);


	for (auto _i = pbar.begin(); _i != pbar.end(); _i++) {
		int iter = *_i;
		int i = NextFloat() * (size - 1);
		int j = NextFloat() * (size - 1);
		if (i == j) continue;

		std::swap(blue_noise[i], blue_noise[j]);

		float new_loss = E();

		// swap back.
		if (new_loss > loss) {
			std::swap(blue_noise[i], blue_noise[j]);
		}
		else {
			loss = new_loss;
		}

		if (verbose && iter % progress_iters == 0) {
			printf("loss: %0.4f, iter:%d\n", loss, iter);
		}
	}
}
