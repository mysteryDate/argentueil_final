#pragma once

#include "ofMain.h"
#include <cmath>

// #define PI 3.14159265;
#define TIME_SCALE 0.5 // Smoothness of noisy motion
#define VELOCITY_DISPLACEMENT_SCALE 0.1 // How much velocity can change in a frame
#define DIRECTION_DISPLACEMENT_SCALE 1 // How much direction can change in a frame

#define MIN_VELOCITY 2
#define MAX_VELOCITY 10

class Critter
{
public:
	Critter(int numFrames);

	float 	v;
	float 	d; //Direction, in degrees, because what was I thinking?
	ofPoint p; // From the center of the critter
	int 	currentFrame;
	float 	nextFrame; // for changing framerate
	bool 	hidden;
	int 	numFrames;
	vector<bool> previousFrames; // if it was hidden in the past

	vector< float > 	offsets;

	void update(ofVec2f nearestHand);

};