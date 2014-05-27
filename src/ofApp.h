// argentueil_final#pragma once#ifdef __APPLE__	#define PLATFORM "MAC"#else	#define PLATFORM "WINDOWS"#endif#include "ofMain.h"// Computer vision#include "ofxOpenCv.h"#include "ofxCv.h"#include "ofxKinect.h"// Ripples effect#include "ofxRipples.h"#include "ofxBounce.h" // Special font characters#include "ofxTrueTypeFontUC.h"// Saving data#include "ofxXmlSettings.h"// My headers#include "utility.h"#include "ArmContourFinder.h"#include "Critter.h"// Standards#include <cmath>// Video player#include "ofxGstStandaloneVideoPlayer.h"#define START_PHASE 0#define REGISTRATION false// Don't ask#define UULONG_MAX 18446744073709551615class ofApp : public ofBaseApp{public:	void setup();	void loadSettings();	void update();	void adjustPhase();	void updateRegions();	void updateRipples();	void updateBeavers();	void draw();	void drawHandMask(ofColor color = ofColor(0,0,0), bool bDrawArms = true, bool scale = false);	void drawBeavers();	void drawHandText();	void drawHandImages();	void drawFeedback();	void startScreensaver();	void keyPressed(int key);	void mouseMoved(int mx, int my);	// Input processing	ofxKinect 			kinect;		ofxCvGrayscaleImage	kinectImg;	ofxCvGrayscaleImage kinectBackground;	int 				nearThreshold;	int 				farThreshold;	bool 				bLearnBackground;	bool				bBackgroundLearned; 	// For cropping out un-desired regions	cv::Mat 			input;	cv::Mat 			croppedInput;	// Computer vision	ArmContourFinder 	ContourFinder;	// Videos	ofxGstStandaloneVideoPlayer firstVideo;	ofxGstStandaloneVideoPlayer 		secondVideo;	ofxGstStandaloneVideoPlayer *		video;	float 				speed;	// Video blending	ofShader 			shader;	ofFbo 				fbo;	ofFbo 				maskFbo;	ofImage 			brushImg;	// Ripples effect	ofxRipples			ripples;	ofxBounce 			bounce;	ofImage 			riverMask;	map<int, ofImage> 	animatedMask;	ofVideoPlayer 		maskVid;	ofImage 			currentMask;	bool 				bRipple;	// Beaver game	vector< Critter > 	Beavers;	vector< ofImage > 	gifFrames;	int 				numBeaverFrames;	// Labeling	map<string, ofPolyline> 	regions;	// For saving data	ofxXmlSettings 		XML;	// For determining what's going on	int currentPhase;	int nextPhaseFrame;	// For transitions between phases	bool bTransition;	// Font display	ofxTrueTypeFontUC font;	bool 				bHandText;	map< unsigned int, vector<ofPoint> > stableHands;	float 				minVelocity;	float TEXT_SCALE;	// Calibration settings	float video_x, video_y, video_w, video_h;	float kinect_x, kinect_y, kinect_w, kinect_h;	int crop_left, crop_top, crop_right, crop_bottom;	// Feedback	bool bDisplayFeedback;	ofSoundPlayer beaverSound;	ofSoundPlayer forestSound;	ofSoundPlayer handSound;		// For screensaver	unsigned long long  lastTime;	unsigned long long  screensaverTime;	unsigned long long  screensaverEndTime;	int screensaverDx;	int screensaverDy;	bool 				bScreensaver;	ofxGstStandaloneVideoPlayer 		screensaver;	ofFbo	gstFbo;	//============ Non Permanent Variables ======//	// Calibration	int minArea;	ofPoint testPoint;	bool bCalibrateKinect;	bool bCalibrateVideo;	bool bCalibrateText;};