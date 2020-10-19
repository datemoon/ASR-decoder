#ifndef __WEIGTH_H__
#define __WEIGTH_H__

#include <iostream>
#include <cmath>
#include <limits>
#include <stdlib.h>

#include "src/util/namespace-start.h"

typedef long long int LLint;
typedef unsigned long long int ULLint;

constexpr float kDelta = 1.0F / 1024.0F;
using namespace std;
// Numeric limits class.
template <class T>
class FloatLimits 
{
public:
	static constexpr T PosInfinity() 
	{
		return std::numeric_limits<T>::infinity();
	}

	static constexpr T NegInfinity() { return -PosInfinity(); }

	static constexpr T NumberBad() { return std::numeric_limits<T>::quiet_NaN(); }
};

template<class T = float>
class FloatWeightTpl
{
public:
	using ValueType = T;

	inline T Value() const { return _value;}
	inline void SetValue(T f) { _value = f;}
	FloatWeightTpl(T value) : _value(value){}
	FloatWeightTpl():_value(0){}

	static const FloatWeightTpl Zero()
	{
		return FloatWeightTpl(numeric_limits<T>::infinity());
	}

	static const FloatWeightTpl One()
	{
		return FloatWeightTpl(0);
	}

	FloatWeightTpl &operator=(const FloatWeightTpl &w)
	{
		_value = w._value;
		return *this;
	}
	bool Read(FILE *fp)
	{
		if(fread((void *)&_value, sizeof(_value),1, fp) != 1)
		{
			std::cerr << "Read value error." << std::endl;
			return false;
		}
	}

	bool Write(FILE *fp)
	{
		if(fwrite((const void *)&_value, sizeof(_value), 1, fp) != 1)
		{
			std::cerr << "Write value error." << std::endl;
			return false;
		}
		return true;
	}
private:
	T _value;
};
// Single-precision float weight.
using FloatWeight = FloatWeightTpl<float>;

template <class T>
inline bool operator==(const FloatWeightTpl<T> &w1,
		const FloatWeightTpl<T> &w2) 
{
	volatile T v1 = w1.Value();
	volatile T v2 = w2.Value();
	return v1 == v2;
}

// These seemingly unnecessary overloads are actually needed to make
// comparisons like FloatWeightTpl<float> == float compile.  If only the
// templated version exists, the FloatWeightTpl<float>(float) conversion
// won't be found.
inline bool operator==(const FloatWeightTpl<float> &w1,
		const FloatWeightTpl<float> &w2) 
{
	return operator==<float>(w1, w2);
}

inline bool operator==(const FloatWeightTpl<double> &w1,
		const FloatWeightTpl<double> &w2) 
{
	return operator==<double>(w1, w2);
}

template <class T>
inline bool operator!=(const FloatWeightTpl<T> &w1,
		const FloatWeightTpl<T> &w2) 
{
	return !(w1 == w2);
}

inline bool operator!=(const FloatWeightTpl<float> &w1,
		const FloatWeightTpl<float> &w2) 
{
	return operator!=<float>(w1, w2);
}

inline bool operator!=(const FloatWeightTpl<double> &w1,
		const FloatWeightTpl<double> &w2) 
{
	return operator!=<double>(w1, w2);
}

template <class T>
inline bool ApproxEqual(const FloatWeightTpl<T> &w1,
		const FloatWeightTpl<T> &w2, float delta = kDelta) 
{
	return w1.Value() <= w2.Value() + delta && w2.Value() <= w1.Value() + delta;
}



template <class T>
inline ostream &operator<<(ostream &strm,const FloatWeightTpl<T> &w)
{
	if (w.Value() == FloatLimits<T>::PosInfinity())
	{
		return strm << "Infinity";
	}
	else if (w.Value() == FloatLimits<T>::NegInfinity())
	{
		return strm << "-Infinity";
	}
	else if (w.Value() != w.Value()) 
	{  // Fails for IEEE NaN.
		return strm << "BadNumber";
	}
	else
	{
		return strm << w.Value();
	}
}

template <class T>
inline std::istream &operator>>(std::istream &strm, FloatWeightTpl<T> &w)
{
	string s;
	strm >> s;
	if (s == "Infinity")
	{
		w = FloatWeightTpl<T>(FloatLimits<T>::PosInfinity());
	}
	else if (s == "-Infinity") 
	{
		w = FloatWeightTpl<T>(FloatLimits<T>::NegInfinity());
	}
	else
	{
		char *p;
		T f = strtod(s.c_str(), &p);
		if (p < s.c_str() + s.size())
		{
			strm.clear(std::ios::badbit);
		}
		else
		{
			w = FloatWeightTpl<T>(f);
		}
	}
	return strm;
}


// lattice weight
template<class FloatType>
class LatticeWeightTpl;

template <class FloatType>
inline ostream &operator <<(ostream &strm, const LatticeWeightTpl<FloatType> &w);

template<class FloatType>
class LatticeWeightTpl
{
public:
	typedef FloatType T; // normally float.

	inline T Value1() const { return _value1;}
	inline T Value2() const { return _value2;}
	inline T Value() const { return _value1 + _value2; }

	inline void SetValue1(T f) { _value1 = f; }
	inline void SetValue2(T f) { _value2 = f; }

	LatticeWeightTpl(): _value1(0.0), _value2(0.0) { }

	LatticeWeightTpl(T a, T b): _value1(a), _value2(b) {}

	LatticeWeightTpl(const LatticeWeightTpl &other): _value1(other._value1), _value2(other._value2) { }

	static const LatticeWeightTpl Zero() 
	{
		return LatticeWeightTpl(numeric_limits<T>::infinity(),
				numeric_limits<T>::infinity());
	}

	static const LatticeWeightTpl One() 
	{
		return LatticeWeightTpl(0.0, 0.0);
	}

	LatticeWeightTpl &operator=(const LatticeWeightTpl &w) 
	{
		_value1 = w._value1;
		_value2 = w._value2;
		return *this;
	}

	bool Read(FILE *fp)
	{
		if(fread((void *)&_value1, sizeof(_value1), 1, fp) != 1)
		{
			std::cerr << "Read value1 error." << std::endl;
			return false;
		}
		if(fread((void *)&_value2, sizeof(_value2), 1, fp) != 1)
		{
			std::cerr << "Read value2 error." << std::endl;
			return false;
		}
		return true;
	}

	bool Write(FILE *fp)
	{
		if(fwrite((const void *)&_value1, sizeof(_value1), 1, fp) != 1)
		{
			std::cerr << "Write value1 error." << std::endl;
			return false;
		}
		if(fwrite((const void *)&_value2, sizeof(_value2), 1, fp) != 1)
		{
			std::cerr << "Write value2 error." << std::endl;
			return false;
		}
		return true;
	}
	friend ostream &operator<< <FloatType>(ostream&, const LatticeWeightTpl<FloatType>&);
private:
	T _value1;
	T _value2;
};

template <class FloatType>
inline ostream &operator <<(ostream &strm, const LatticeWeightTpl<FloatType> &w)
{
	strm << w.Value1() << "," << w.Value2();
	return strm;
}

template<class FloatType>
inline bool operator!=(const LatticeWeightTpl<FloatType> &wa,
		const LatticeWeightTpl<FloatType> &wb)
{
	volatile FloatType va1 = wa.Value1(), va2 = wa.Value2(),
			 vb1 = wb.Value1(), vb2 = wb.Value2();
	return (va1 != vb1 || va2 != vb2);
}

template<class FloatType>
inline bool operator==(const LatticeWeightTpl<FloatType> &wa,
		const LatticeWeightTpl<FloatType> &wb) 
{
	volatile FloatType va1 = wa.Value1(), va2 = wa.Value2(),
			 vb1 = wb.Value1(), vb2 = wb.Value2();
	return (va1 == vb1 && va2 == vb2);
}

// We define a Compare function LatticeWeightTpl even though it's
// not required by the semiring standard-- it's just more efficient
// to do it this way rather than using the NaturalLess template.

/// Compare returns -1 if w1 < w2, +1 if w1 > w2, and 0 if w1 == w2.

template<class FloatType>
inline int LatticeWeightCompare (const LatticeWeightTpl<FloatType> &w1,
		const LatticeWeightTpl<FloatType> &w2) 
{
	FloatType f1 = w1.Value1() + w1.Value2(),
			  f2 = w2.Value1() + w2.Value2();
	if (f1 < f2) { return 1; } // having smaller cost means you're larger
	// in the semiring [higher probability]
	else if (f1 > f2) { return -1; }
	// mathematically we should be comparing (w1.value1_-w1.value2_ < w2.value1_-w2.value2_)
	// in the next line, but add w1.value1_+w1.value2_ = w2.value1_+w2.value2_ to both sides and
	// divide by two, and we get the simpler equivalent form w1.value1_ < w2.value1_.
	else if (w1.Value1() < w2.Value1()) { return 1; }
	else if (w1.Value1() > w2.Value1()) { return -1; }
	else { return 0; }
}

template<class FloatType>
inline LatticeWeightTpl<FloatType> Plus(const LatticeWeightTpl<FloatType> &w1,
		const LatticeWeightTpl<FloatType> &w2) 
{
	return (LatticeWeightCompare(w1, w2) >= 0 ? w1 : w2);
}

template<class FloatType>
inline LatticeWeightTpl<FloatType> Times(const LatticeWeightTpl<FloatType> &w1,
		const LatticeWeightTpl<FloatType> &w2) 
{
	return LatticeWeightTpl<FloatType>(w1.Value1()+w2.Value1(), w1.Value2()+w2.Value2());
}

// divide w1 by w2 (on left/right/any doesn't matter as
// commutative).
template<class FloatType>
inline LatticeWeightTpl<FloatType> Divide(const LatticeWeightTpl<FloatType> &w1,
		const LatticeWeightTpl<FloatType> &w2)
{
	typedef FloatType T;
	T a = w1.Value1() - w2.Value1(), b = w1.Value2() - w2.Value2();
	if (a != a || b != b || a == -numeric_limits<T>::infinity()
			|| b == -numeric_limits<T>::infinity()) {
		std::cout << "LatticeWeightTpl::Divide, NaN or invalid number produced. "
			<< "[dividing by zero?]  Returning zero" << std::endl;
		return LatticeWeightTpl<T>::Zero();
	}
	if (a == numeric_limits<T>::infinity() ||
			b == numeric_limits<T>::infinity())
		return LatticeWeightTpl<T>::Zero(); // not a valid number if only one is infinite.
	return LatticeWeightTpl<T>(a, b);
}



template<class FloatType>
inline bool ApproxEqual(const LatticeWeightTpl<FloatType> &w1,
		const LatticeWeightTpl<FloatType> &w2,
		float delta = kDelta) 
{
	if (w1.Value1() == w2.Value1() && w1.Value2() == w2.Value2()) 
		return true;  // handles Zero().
	return (fabs((w1.Value1() + w1.Value2()) - (w2.Value1() + w2.Value2())) <= delta);
}
#include "src/util/namespace-end.h"
#endif
