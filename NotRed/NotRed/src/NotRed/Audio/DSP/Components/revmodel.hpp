// Reverb model declaration
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// This code is public domain
#include "NotRed/Audio/SourceManager.h"

#ifndef _revmodel_
#define _revmodel_

#include "comb.hpp"
#include "allpass.hpp"
#include "tuning.h"

using FloatAllocatorVector = std::vector<float, NR::Audio::SourceManager::Allocator<float>>;

class revmodel
{
public:
	revmodel(double sampleRate);

	void	mute();

	void	processmix(const float* inputL, const float* inputR, float* outputL, float* outputR, long numsamples, int skip);
	void	processreplace(const float* inputL, const float* inputR, float* outputL, float* outputR, long numsamples, int skip);

	void	setroomsize(float value);
	float	getroomsize();

	void	setdamp(float value);
	float	getdamp();

	void	setwet(float value);
	float	getwet();

	void	setdry(float value);
	float	getdry();

	void	setwidth(float value);
	float	getwidth();

	void	setmode(float value);
	float	getmode();

private:
	void	update();

private:
	float	gain;
	float	roomsize, roomsize1;
	float	damp, damp1;
	float	wet, wet1, wet2;
	float	dry;
	float	width;
	float	mode;

	// The following are all declared inline 
	// to remove the need for dynamic allocation
	// with its subsequent error-checking messiness
	// Comb filters
	comb	combL[numcombs];
	comb	combR[numcombs];

	// Allpass filters
	allpass	allpassL[numallpasses];
	allpass	allpassR[numallpasses];

	// Buffers for the combs
	FloatAllocatorVector bufcombL1;
	FloatAllocatorVector bufcombR1;
	FloatAllocatorVector bufcombL2;
	FloatAllocatorVector bufcombR2;
	FloatAllocatorVector bufcombL3;
	FloatAllocatorVector bufcombR3;
	FloatAllocatorVector bufcombL4;
	FloatAllocatorVector bufcombR4;
	FloatAllocatorVector bufcombL5;
	FloatAllocatorVector bufcombR5;
	FloatAllocatorVector bufcombL6;
	FloatAllocatorVector bufcombR6;
	FloatAllocatorVector bufcombL7;
	FloatAllocatorVector bufcombR7;
	FloatAllocatorVector bufcombL8;
	FloatAllocatorVector bufcombR8;

	// Buffers for the allpasses
	FloatAllocatorVector bufallpassL1;
	FloatAllocatorVector bufallpassR1;
	FloatAllocatorVector bufallpassL2;
	FloatAllocatorVector bufallpassR2;
	FloatAllocatorVector bufallpassL3;
	FloatAllocatorVector bufallpassR3;
	FloatAllocatorVector bufallpassL4;
	FloatAllocatorVector bufallpassR4;
};
#endif//_revmodel_
//ends