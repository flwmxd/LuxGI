#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <random>
#include <type_traits>

namespace maple
{
	class Randomizer
	{
	  public:
		Randomizer()
		{
		}

		auto nextBool() const
		{
			return nextInt(2) == 1;
		}

		auto below(float percent) const
		{
			return nextReal(1.0f) < percent;
		}

		auto above(float percent) const
		{
			return nextReal(1.0f) > percent;
		}

		template <class T>
		auto nextReal(T to) const -> T
		{
			return nextReal<T>(0, to);
		}

		template <class T>
		auto nextReal(T from, T to) const -> T
		{
			if (from >= to)
				return from;
			std::uniform_real_distribution<T> range(from, to);
			std::random_device                rd;
			std::default_random_engine        engine{rd()};
			return range(engine);
		}

		template <class T>
		auto nextInt(T to) const -> T
		{
			return nextInt<T>(0, to);
		}

		template <class T>
		auto nextInt(T from, T to) const -> T
		{
			if (from >= to)
				return from;

			std::uniform_int_distribution<T> range(from, to - 1);
			std::random_device               rd;
			std::default_random_engine       engine{rd()};
			return range(engine);
		}

		template <class E>
		auto nextEnum(E to = E::LENGTH) const -> E
		{
			return nextEnum(E(), to);
		}

		static auto random() -> double
		{
			static Randomizer r;
			return r.nextReal(0.f, 1.f);
		}

		template <class E>
		auto nextEnum(E from, E to) const -> E
		{
#ifdef _WIN32
			auto next_underlying = nextInt<std::underlying_type<E>::type>(from, to);
			return static_cast<E>(next_underlying);
#else
			auto next_underlying = nextInt<int32_t>(from, to);
			return static_cast<E>(next_underlying);
#endif        // _WIN32
		}
	};

}        // namespace maple
