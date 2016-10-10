/*
 * This file is part of `et engine`
 * Copyright 2009-2016 by Sergey Reznik
 * Please, modify content only if you know what are you doing.
 *
 */

#pragma once

#include <et/geometry/vector3.h>

namespace et
{
	template <typename T, int size, typename GenType = T>
	class Polynome
	{
	public: 
		Polynome();
		Polynome(const T* coefficients);

		T calculate(GenType time) const;
		T derivative(int order, GenType time) const;
		T numericDerivative(int order, GenType time, GenType dt);
		T curvature(int derivativeOrder, GenType time) const;

	protected:
		T _coefficients[size];
	};

	template <typename T, typename GenType = T>
	class MinimallyQualifiedSpline : public Polynome<T, 5, GenType>
	{
	public:
		MinimallyQualifiedSpline(const T& p0, const T& dp0, const T& pT, const T& dpT, GenType time);

		const GenType duration() const
			{ return _time; }

		const T startPoint() const
			{ return _p0; }
		
		const T startPointVelocity() const
			{ return _dp0; }

		const T endPoint() const
			{ return _pT; }
		
		const T endPointVelocity() const
			{ return _dpT; }

		const T startPointAcceleration() const;
		const T midPointAcceleration() const;
		const T endPointAcceleration() const;

	private:
		void genPolynomial();
		T _get_c2(const T& p0, const T& dp0, const T& pT, const T& dpT, GenType time) const;
		T _get_c3(const T& p0, const T& dp0, const T& pT, const T& dpT, GenType time) const;

	private:
		T _p0;
		T _dp0;
		T _pT;
		T _dpT;
		GenType _time;
	};

	template <typename T, typename GenType = T>
	class PartialyQualifiedSpline : public Polynome<T, 5, GenType>
	{
	public:
		PartialyQualifiedSpline(const T& p0, const T& dp0, const T& ddp0, const T& pT, const T& dpT, GenType time);
		PartialyQualifiedSpline(const PartialyQualifiedSpline& prev, const T& pT, const T& dpT, GenType time);

		const GenType duration() const
			{ return _time; }
		
		const T startPoint() const
			{ return _p0; }
		
		const T startPointVelocity() const
			{ return _dp0; }
		
		const T startPointAcceleration() const
			{ return _ddp0; }
		
		const T endPoint() const
			{ return _pT; }
		
		const T endPointVelocity() const
			{ return _dpT; }

		const T endPointAcceleration() const;

	private: 
		void genPolynomial();
		T _get_c3(const T& q0, const T& qT, const T& dq0, const T& dqT, const T& ddq0, GenType time) const;
		T _get_c4(const T& q0, const T& qT, const T& dq0, const T& dqT, const T& ddq0, GenType time) const;

	private:
		T _p0;
		T _dp0;
		T _ddp0;
		T _pT;
		T _dpT;
		GenType _time;
	};

	template <typename T, typename GenType = T>
	class FullyQualifiedSpline : public Polynome<T, 6, GenType>
	{
	public:
		FullyQualifiedSpline(const T& p0, const T& dp0, const T& ddp0, const T& pT, const T& dpT, const T& ddpT, GenType time);

		FullyQualifiedSpline();
		FullyQualifiedSpline(const MinimallyQualifiedSpline<T, GenType>& minimal);
		FullyQualifiedSpline(const PartialyQualifiedSpline<T, GenType>& partial);
		FullyQualifiedSpline(const PartialyQualifiedSpline<T, GenType>& prev, const T& pT, const T& dpT, const T& ddpT, GenType time);
		FullyQualifiedSpline(const FullyQualifiedSpline& prev, const T& pT, const T& dpT, const T& ddpT, GenType time);

		const GenType duration() const
			{ return _time; }
		
		const T startPoint() const
			{ return _p0; }
		
		const T startPointVelocity() const
			{ return _dp0; }
		
		const T startPointAcceleration() const
			{ return _ddp0; }

		const T midPoint() const
			{ return derivative(0, _time / GenType(2)); }
		
		const T midPointVelocity() const
			{ return derivative(1, _time / GenType(2)); }
		
		const T midPointAcceleration() const
			{ return derivative(2, _time / GenType(2)); }

		const T endPoint() const
			{ return _pT; }
		
		const T endPointVelocity() const
			{ return _dpT; }
		
		const T endPointAcceleration() const
			{ return _ddpT; }

	private:
		void genPolynomial();
		
		T _get_c3(const T& q0, const T& qT, const T& dq0, const T& dqT, const T& ddq0, const T& ddqT, GenType time) const;
		T _get_c4(const T& q0, const T& qT, const T& dq0, const T& dqT, const T& ddq0, const T& ddqT, GenType time) const;
		T _get_c5(const T& q0, const T& qT, const T& dq0, const T& dqT, const T& ddq0, const T& ddqT, GenType time) const;

	private:
		T _p0;
		T _dp0;
		T _ddp0;
		T _pT;
		T _dpT;
		T _ddpT;
		GenType _time;
	};

	template <typename T, typename GenType = T>
	class SplineSequence
	{
	public:
		struct SplinePoint
		{
			T position;
			T velocity;
			T acceleration;
		};
		
		struct SplineBasePoint
		{
			T position;
			GenType velocityModule;

			SplineBasePoint() :
				position(0), velocityModule(0) { }
			
			SplineBasePoint(T pos, GenType vel) :
				position(pos), velocityModule(vel) { }
		};
		
		typedef std::vector<SplinePoint> SplinePointList;
		typedef std::vector<SplineBasePoint> SplineBasePointList;
		typedef std::vector<FullyQualifiedSpline<T, GenType> > FullyQualifiedSplineList;
		
	public:
		SplineSequence();
		SplineSequence(const SplinePointList& points);
		SplineSequence(const SplineBasePointList& points);

		const GenType& duration() const
			{ return _duration; }
		
		SplinePoint sample(GenType time);

	private:
		void buildSequence();
		
	private:
		GenType _duration;
		SplinePointList _points;
		FullyQualifiedSplineList _splines;
		bool _closedTrajectory;
	};

#	include <et/geometry/splines.inl.h>
}
