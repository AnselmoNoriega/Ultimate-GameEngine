// Reverb model tuning values
//
// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// This code is public domain
#ifndef _tuning_
#define _tuning_

const int	numcombs = 8;
const int	numallpasses = 4;
const float	muted = 0;
const float	fixedgain = 0.015f;
const float scalewet = 3;
const float scaledry = 2;
const float scaledamp = 0.4f;
const float scaleroom = 0.28f;
const float offsetroom = 0.7f;
const float initialroom = 0.5f;
const float initialdamp = 0.5f;
const float initialwet = 1 / scalewet;
const float initialdry = 0;
const float initialwidth = 1;
const float initialmode = 0;
const float freezemode = 0.5f;
const int	stereospread = 23;

// These values assume 44.1KHz sample rate
// they will probably be OK for 48KHz sample rate
// but would need scaling for 96KHz (or other) sample rates.
// The values were obtained by listening tests.
const int combtuningL1 = (float)1116;/// 0.91875f;
const int combtuningR1 = (float)1116 + stereospread;/// 0.91875f;
const int combtuningL2 = (float)1188;/// 0.91875f;
const int combtuningR2 = (float)1188 + stereospread;/// 0.91875f;
const int combtuningL3 = (float)1277;/// 0.91875f;
const int combtuningR3 = (float)1277 + stereospread;/// 0.91875f;
const int combtuningL4 = (float)1356;/// 0.91875f;
const int combtuningR4 = (float)1356 + stereospread;/// 0.91875f;
const int combtuningL5 = (float)1422;/// 0.91875f;
const int combtuningR5 = (float)1422 + stereospread;/// 0.91875f;
const int combtuningL6 = (float)1491;/// 0.91875f;
const int combtuningR6 = (float)1491 + stereospread;/// 0.91875f;
const int combtuningL7 = (float)1557;/// 0.91875f;
const int combtuningR7 = (float)1557 + stereospread;/// 0.91875f;
const int combtuningL8 = (float)1617;/// 0.91875f;
const int combtuningR8 = (float)1617 + stereospread;/// 0.91875f;

const int allpasstuningL1 = (float)556;/// 0.91875f;
const int allpasstuningR1 = (float)556 + stereospread;/// 0.91875f;
const int allpasstuningL2 = (float)441;/// 0.91875f;
const int allpasstuningR2 = (float)441 + stereospread;/// 0.91875f;
const int allpasstuningL3 = (float)341;/// 0.91875f;
const int allpasstuningR3 = (float)341 + stereospread;/// 0.91875f;
const int allpasstuningL4 = (float)225;/// 0.91875f;
const int allpasstuningR4 = (float)225 + stereospread;/// 0.91875f;
// Different character
/*
const int combtuningL1 = 1557;
const int combtuningR1 = 1557 + stereospread;
const int combtuningL2 = 1617;
const int combtuningR2 = 1617 + stereospread;
const int combtuningL3 = 1491;
const int combtuningR3 = 1491 + stereospread;
const int combtuningL4 = 1422;
const int combtuningR4 = 1422 + stereospread;
const int combtuningL5 = 1277;
const int combtuningR5 = 1277 + stereospread;
const int combtuningL6 = 1356;
const int combtuningR6 = 1356 + stereospread;
const int combtuningL7 = 1188;
const int combtuningR7 = 1188 + stereospread;
const int combtuningL8 = 1116;
const int combtuningR8 = 1116 + stereospread;
const int allpasstuningL1 = 225;
const int allpasstuningR1 = 225 + stereospread;
const int allpasstuningL2 = 556;
const int allpasstuningR2 = 556 + stereospread;
const int allpasstuningL3 = 441;
const int allpasstuningR3 = 441 + stereospread;
const int allpasstuningL4 = 341;
const int allpasstuningR4 = 341 + stereospread;
*/
#endif//_tuning_
//ends