// Argenteuil final
#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){

	//ofSetFrameRate(60);
	ofBackground(0,0,0);  
	XML.loadFile("settings.xml");

	firstVideo.setPixelFormat(OF_PIXELS_RGB);
	string file = ofFilePath::getAbsolutePath("videos/p1.mov", true);
	std::replace(file.begin(), file.end(), '\\', '/');
	file = "file:///" + file;
	if(!firstVideo.loadMovie(file)) {
		ofLogError("setup", "movie loading failed: \n" + file);
	}
	secondVideo.setPixelFormat(OF_PIXELS_RGB);
	file = ofFilePath::getAbsolutePath("videos/p2.mov", true);
	std::replace(file.begin(), file.end(), '\\', '/');
	file = "file:///" + file;
	if(!secondVideo.loadMovie(file)) {
		ofLogError("setup", "movie loading failed: \n" + file);
	}
	firstVideo.setLoopState(OF_LOOP_NONE);
	secondVideo.setLoopState(OF_LOOP_NONE);

	loadSettings();
	currentPhase = START_PHASE - 1;
	cout << XML.getPushLevel() << endl;
	XML.pushTag("PHASEINFORMATION");
		nextPhaseFrame = XML.getValue("PHASE:STARTFRAME", nextPhaseFrame + 1000, currentPhase + 1);
		int v = XML.getValue("PHASE:VIDEO", 1, currentPhase + 1);
		if(v == 1)
			video = &firstVideo;
		if(v == 2)
			video = &secondVideo;
		video->setFrame(nextPhaseFrame - 1);
		video->play();
		video->setPaused(true);
	XML.popTag();
	speed = 1;

	//kinect instructions
	kinect.init();
	kinect.open();
	kinectImg.allocate(kinect.width, kinect.height);
	kinectBackground.allocate(kinect.width, kinect.height);
	kinectBackground.set(0);
	bBackgroundLearned = false;
	bLearnBackground = false;

	// Hand display
	font.loadFont("fonts/AltoPro-Normal.ttf", 12);
	bHandText = false;
	minVelocity = 1;
	TEXT_SCALE = 1;

	// Set up beavers for phase 4
	XML.pushTag("BEAVERS");
	numBeaverFrames = XML.getValue("NUMFRAMES", 24);
	float beaverScale = XML.getValue("IMGSCALE", 0.2);
	for (int i = 0; i < numBeaverFrames; ++i)
	{	
		ofImage img;
		img.loadImage("beaver/beaver-"+ofToString(i)+".png");
		img.resize(img.getWidth()*beaverScale, img.getHeight()*beaverScale);
		gifFrames.push_back(img);
	}
	XML.popTag();

	//for water ripples
	ofEnableAlphaBlending();
	ripples.allocate(video->getWidth(), video->getHeight());
	bounce.allocate(video->getWidth(), video->getHeight());
	// Animated mask
	ofDirectory dir("masks/animated_mask");
	dir.allowExt("png");
	dir.listDir();
	for (int i = 0; i < dir.numFiles(); ++i)
	{
		string name = dir.getName(i);
		ofImage img;
		img.loadImage("masks/animated_mask/"+name);
		name = name.substr(0,4);
		int frame = std::atoi(name.c_str());
		animatedMask[frame] = img;
	}
	riverMask.loadImage("masks/river_mask_v3.png");
	bRipple = false;

	// Setting up shaders for phase 5
	shader.setupShaderFromFile(GL_FRAGMENT_SHADER, "shadersGL2/shader.frag");
	shader.linkProgram();
	fbo.allocate(video->getWidth(), video->getHeight());
	maskFbo.allocate(video->getWidth(), video->getHeight());
	maskFbo.begin();
		ofClear(0,0,0,255);
	maskFbo.end();
	fbo.begin();
		ofClear(0,0,0,0);
	fbo.end();

	screensaver.setPixelFormat(OF_PIXELS_RGB);
	file = ofFilePath::getAbsolutePath("videos/screensaver.mov", true);
	std::replace(file.begin(), file.end(), '\\', '/');
	file = "file:///" + file;
	if(!screensaver.loadMovie(file)) {
		ofLogError("setup", "movie loading failed: \n" + file);
	}
    screensaver.setFrame(0);
    screensaver.update();
	lastTime = ofGetSystemTimeMicros();
	bScreensaver = false;
	bTransition = false;
	screensaverEndTime = UULONG_MAX; // HACK

	gstFbo.allocate(video->getWidth(), video->getHeight());

	beaverSound.loadSound("sounds/Castor.aif");
	forestSound.loadSound("sounds/MainForet.aif");
	handSound.loadSound("sounds/Texte_Mains.aif");


	bCalibrateKinect = false;
	bCalibrateVideo = false;
	bCalibrateText = false;
	bDisplayFeedback = false;
}	

//--------------------------------------------------------------
void ofApp::update(){

	kinect.update();
	adjustPhase();

	if(kinect.isFrameNew()) {

		kinectImg.setFromPixels(kinect.getDepthPixels(), kinect.width, kinect.height);
		if(bLearnBackground || (firstVideo.getCurrentFrame() >= 600 && !bBackgroundLearned)) {
			cout << "background learned!" << endl;
			bBackgroundLearned = true;
			kinectBackground = kinectImg;
			bLearnBackground = false;
		}
		// Background subtraction
		kinectImg -= kinectBackground;
		// Remove out of bounds
		unsigned char *pix = kinectImg.getPixels();
		for (int i = 0; i < kinectImg.getHeight() * kinectImg.getWidth(); ++i)
		{
			if(pix[i] > nearThreshold || pix[i] < farThreshold)
				pix[i] = 0;
			else
				pix[i] = 255;
		}

		input = ofxCv::toCv(kinectImg);
		cv::Rect crop_roi = cv::Rect(crop_left, crop_top, 
			kinect.width - crop_left - crop_right,
			kinect.height - crop_top - crop_bottom);
		croppedInput = input(crop_roi).clone();

		ContourFinder.findContours(croppedInput);
		if(bHandText)
			ContourFinder.update();
		else
			ContourFinder.hands.clear();
        

		if(ContourFinder.size() > 0) {
			if(bScreensaver && !bTransition) {
				screensaverEndTime = ofGetSystemTimeMicros();
				maskFbo.begin();
					ofClear(0,0,0,255);
				maskFbo.end();
			}
			if(!bTransition) {
				bScreensaver = false;
				lastTime = ofGetSystemTimeMicros();
			}
		}
	}

	unsigned long long idleTime = (ofGetSystemTimeMicros() - lastTime)/1000;
	if(idleTime >= (screensaverTime - 2000) && !bScreensaver) { // Screensaver fade
		bTransition = true;
		float volume = ofMap(screensaverTime - idleTime, 2000, 0, 0.5, 0, true);
		video->setVolume(volume - 0.1);
		maskFbo.begin();
			// For a fade
			float alpha = ofMap(screensaverTime - idleTime, 2000, 0, 0, 20);
			ofPushStyle();
			ofSetColor(255,255,255, floorf(alpha + 0.5));
			ofRect(0,0,video->getWidth(), video->getHeight());
			ofPopStyle();
		maskFbo.end();

		fbo.begin();
			ofClear(0,0,0,0);
			shader.begin();
				shader.setUniformTexture("maskTex", maskFbo.getTextureReference(), 1);
				screensaver.update();
				screensaver.draw(0,0);
			shader.end();
		fbo.end();
	} 
	// Done fade to screensaver
	if(idleTime >= screensaverTime && !bScreensaver) {
		bTransition = false;
		bScreensaver = true;
		startScreensaver();
	}
	// Fade from screensaver
	if(ofGetSystemTimeMicros() - screensaverEndTime < 2000000) {
		bTransition = true;
		int diff = ofGetSystemTimeMicros() - screensaverEndTime;
		maskFbo.begin();
			// For a fade
			ofClear(0,0,0,0);
			float alpha = ofMap(diff, 0, 2000000, 255, 0);
			ofPushStyle();
			ofSetColor(255,255,255,floorf(alpha + 0.5));
			ofRect(0,0,video->getWidth(), video->getHeight());
			ofPopStyle();
		maskFbo.end();

		fbo.begin();
			ofClear(0,0,0,0);
			shader.begin();
				shader.setUniformTexture("maskTex", maskFbo.getTextureReference(), 1);
				screensaver.update();
				screensaver.draw(0,0);
			shader.end();
		fbo.end();
	}
	// Restart after screensaver
	if(ofGetSystemTimeMicros() - screensaverEndTime > 2000000 && !bScreensaver && ofGetSystemTimeMicros() > screensaverEndTime) {
		bTransition = false;
		screensaver.stop();
		screensaver.setFrame(0);
		screensaverEndTime = UULONG_MAX;
	}

	if(!bScreensaver) {
		// Water ripples on 1,2,3,4,7,8
		if(bRipple) 
			updateRipples();

		// Beaver phase
		if(currentPhase == 4) {
			if(ofGetFrameNum() % 30 == 0 && Beavers.size() < 120) {
				Critter newBeaver = Critter(numBeaverFrames); 
				Beavers.push_back(newBeaver);
			}
		}
		if(Beavers.size() > 0)
			updateBeavers();

		// Transition phase
		if(currentPhase == 5) {
			for (int i = 0; i < ContourFinder.size(); i++)
			{
				ofVec2f vel = ofxCv::toOf(ContourFinder.getVelocity(i));
				if(vel.length() > 8 && !forestSound.getIsPlaying())
					forestSound.play();
			}
			maskFbo.begin();
				ofPushMatrix();
				ofTranslate(-video_x, -video_y);
				ofScale(video->getWidth() / video_w, video->getHeight() / video_h);
					drawHandMask(ofColor(255,255,255,127), true); 
				ofPopMatrix();
				// For a fade
				float alpha = ofMap((nextPhaseFrame - video->getCurrentFrame()), 100, 0, 0, 10);
				ofPushStyle();
				ofSetColor(255,255,255,floorf(alpha + 0.5));
				ofRect(0,0,video->getWidth(), video->getHeight());
				ofPopStyle();
			maskFbo.end();

			fbo.begin();
				ofClear(0,0,0,0);
				shader.begin();
					shader.setUniformTexture("maskTex", maskFbo.getTextureReference(), 1);
					secondVideo.draw(0,0);
				shader.end();
			fbo.end();
		}
	}

}

//--------------------------------------------------------------
void ofApp::draw(){

	if(bRipple)
		bounce.draw(video_x, video_y, video_w, video_h);
	else if (bScreensaver){ 
		video->draw(video_x + screensaverDx, video_y + screensaverDy, video_w, video_h);
	}
	else
		video->draw(video_x, video_y, video_w, video_h);
	fbo.draw(video_x, video_y, video_w, video_h);


	if(bHandText) {
		drawHandMask(ofColor(0,0,0));
		drawHandText();
	}

	drawBeavers();

	if(bDisplayFeedback)
		drawFeedback();

}

//--------------------------------------------------------------
// Custom functions
//--------------------------------------------------------------
void ofApp::startScreensaver() {
	maskFbo.begin();
		ofClear(0,0,0,255);
	maskFbo.end();
	fbo.begin();
		ofClear(0,0,0,0);
	fbo.end();
	screensaverDx = ofRandom(0,20);
	screensaverDy = ofRandom(0,20);
	video->stop();
	video->setFrame(0);
	video = &screensaver;
	bRipple = false;
	currentPhase = -1;
	nextPhaseFrame = -1;
	video->play();
	video->update();
	video->setLoopState(OF_LOOP_NORMAL);
}

void ofApp::adjustPhase() {

	video->update();

	XML.pushTag("PHASEINFORMATION");

	int frame = video->getCurrentFrame();
	if( frame >= nextPhaseFrame && !bScreensaver) { // Change phase
		maskFbo.begin();
			ofClear(0,0,0,255);
		maskFbo.end();
		fbo.begin();
			ofClear(0,0,0,0);
		fbo.end();
		currentPhase++;
		if(currentPhase >= 11) {
			if(firstVideo.isPaused()) 
				firstVideo.setPaused(false);
			currentPhase = 0;
		}
		if(currentPhase == 0) {
			ripples.damping = 0.995;
			secondVideo.stop();
			secondVideo.setFrame(0);
			video = &firstVideo;
			video->play();
			video->setVolume(1);
		}
		if(currentPhase == 5) {
			secondVideo.setPosition(0.19);
			secondVideo.update();
			secondVideo.setPaused(true);
		}
		if(currentPhase == 6) {
			secondVideo.setPaused(false);
			secondVideo.update();
			video = &secondVideo;
			video->setVolume(1);
			firstVideo.setPaused(true);
			firstVideo.setFrame(0);
			firstVideo.update();
		}

		if(XML.getValue("PHASE:RIPPLE", 0, currentPhase))
			bRipple = true;
		else
			bRipple = false;

		if(XML.getValue("PHASE:HANDTEXT", 0, currentPhase))
			bHandText = true;
		else
			bHandText = false;

		nextPhaseFrame = XML.getValue("PHASE:STARTFRAME", nextPhaseFrame + 1000, currentPhase + 1);
		farThreshold = XML.getValue("PHASE:FARTHRESHOLD", 2, currentPhase);
		nearThreshold = XML.getValue("PHASE:NEARTHRESHOLD", 42, currentPhase);
		updateRegions();
	}
	XML.popTag();
}

void ofApp::updateRegions() {

	regions.clear();
	XML.pushTag("PHASE", currentPhase);
	if(XML.getNumTags("REGIONS") > 0) {
		XML.pushTag("REGIONS");
		for (int i = 0; i < XML.getNumTags("REGION"); ++i)
		{
			XML.pushTag("REGION", i);
			string name = XML.getValue("NAME", "unknown");
			regions.insert(pair<string, ofPolyline>(name, ofPolyline()));
			for (int j = 0; j < XML.getNumTags("PT"); ++j)
			{
				int x = XML.getValue("PT:X", 0, j);
				int y = XML.getValue("PT:Y", 0, j);
				regions[name].addVertex(ofPoint(x,y));
			}
			XML.popTag();
		}
	    XML.popTag();
	}
    XML.popTag();

}

void ofApp::updateRipples() {

	gstFbo.begin();
		video->draw(0,0);
	gstFbo.end();
	bounce.setTexture(gstFbo.getTextureReference(), 1);
	int frameDiff = nextPhaseFrame - video->getCurrentFrame();
	if(currentPhase != 0)
		ripples.damping = ofMap(frameDiff, 100, 0, 0.995, 0, true);
	// Water ripples
	ripples.begin();
		ofPushStyle();
		ofPushMatrix();
			ofFill();
			// Put the data reference frame into the untransformed video reference
			ofTranslate(-video_x, -video_y);
			ofScale(video->getWidth() / video_w,  video->getHeight() / video_h);
			drawHandMask(ofColor(255,255,255), !bHandText, true);
			drawBeavers();

		ofPopMatrix();

		if(currentPhase == 0) {
            int frame = video->getCurrentFrame();
			while(true) {
				if( animatedMask.find(frame) != animatedMask.end() ) {
					animatedMask[frame].draw(0,0, video->getWidth(), video->getHeight());
					break;
				}
				frame--;
				if(frame < 0) {
					ofPushStyle();
					ofSetColor(0,0,0);
					ofFill();
					ofRect(0,0, video->getWidth(), video->getHeight());
					ofPopStyle();
                    break;
				}
			}
		}
		else
			riverMask.draw(0,0);
		ofPopStyle();
	ripples.end();
	
	ripples.update();
	bounce << ripples;
}

void ofApp::updateBeavers() {

	float Iw = gifFrames[0].getWidth();
	float Ih = gifFrames[0].getHeight();
	float beaverScaleUp = 1.4; // makes them easier to catch

	for (int i = 0; i < Beavers.size(); ++i)
	{
		// Find vector to closest hand
		ofVec2f nearestHand = ofVec2f(0,0);
		float minDist = 99999999999999; 

		for (int j = 0; j < ContourFinder.size(); ++j)
		{
			ofPoint center = utility::transform(ContourFinder.getPolyline(j).getCentroid2D(), kinect_x, kinect_y, kinect_w, kinect_h);
			float dist = ofDistSquared(Beavers[i].p.x, Beavers[i].p.y, center.x, center.y);
			if(dist < minDist) {
				minDist = dist;
				nearestHand = ofVec2f(center.x - Beavers[i].p.x, center.y - Beavers[i].p.y);
			}
		}
		Beavers[i].update(nearestHand);
		Critter * B = &Beavers[i];
		ofPolyline corners;
		for (int j = 0; j < 4; ++j)
		{
			int xd = (j & 1)*2 - 1;
			int yd = (j & 2) - 1;
			float x = B->p.x + beaverScaleUp*cos(B->d*PI/180)*Iw*xd/2 + beaverScaleUp*sin(B->d*PI/180)*Ih*yd/2;
			float y = B->p.y - beaverScaleUp*sin(B->d*PI/180)*Iw*xd/2 + beaverScaleUp*cos(B->d*PI/180)*Ih*yd/2;
			corners.addVertex(ofPoint(x,y));
		}
		// Beaver has left the building
		if(B->p.x > ofGetWindowWidth() + Iw || B->p.x < 0 - Iw || B->p.y > ofGetWindowHeight() + Iw || B->p.y < 0 - Iw)
				Beavers.erase(Beavers.begin() + i);
		// Collision
		B->previousFrames.resize(5);
		B->previousFrames.insert(B->previousFrames.begin(), B->hidden);
		B->hidden = false; 
		for (int j = 0; j < ContourFinder.size(); ++j)
		{
			ofPolyline line = utility::transform(ContourFinder.getPolyline(j), kinect_x, kinect_y, kinect_w, kinect_h);
			for (int k = 0; k < line.size(); ++k)
			{
				if(corners.inside(line[k].x, line[k].y)) {
					if(!beaverSound.getIsPlaying())
						beaverSound.play();
					B->v = 10;
					B->hidden = true;
					break;
				}
			}
		}

	}
}

void ofApp::drawBeavers() {

	for (int i = 0; i < Beavers.size(); ++i)
	{		
		Critter CB = Beavers[i];
		bool draw = true;		
		for (int j = 0; j < CB.previousFrames.size(); ++j)
		{
			if(CB.previousFrames[j]) // Hidden was true
				draw = false;
		}
		if(!CB.hidden && draw) {
			ofPushMatrix();
	        ofTranslate(CB.p.x, CB.p.y); // Translate to the center of the beaver
			ofRotateZ(CB.d);
				gifFrames[CB.currentFrame].setAnchorPercent(0.5,0.5); // So that we draw from the middle
				gifFrames[CB.currentFrame].draw(0,0);
			ofPopMatrix();
		}
	}

}
// third argument scales the alpha based on hand velocity
void ofApp::drawHandMask(ofColor color, bool bDrawArms, bool scale) {

	ofPushStyle();
	ofSetColor(color);
	ofFill();
	ofPushMatrix();
	ofTranslate(kinect_x, kinect_y);
	ofScale(kinect_w, kinect_h);

	if(bDrawArms) {
		for (int i = 0; i < ContourFinder.size(); ++i)
		{
			ofPolyline blob = ContourFinder.getPolyline(i);
			blob = blob.getSmoothed(4);

			ofBeginShape();
				for (int j = 0; j < blob.size(); ++j) {
					ofVertex(blob[j]);
				}
			ofEndShape();
		}
	}
	else{
		for (int i = 0; i < ContourFinder.hands.size(); ++i)
		{
			if(scale) {
				float vel = ContourFinder.hands[i].velocity.length();
				float alpha = ofMap(vel, 1, 8, 0, 255, true);
				color = ofColor(color.r,alpha);
			}
			ofSetColor(color);
			ofPolyline blob = ContourFinder.hands[i].line;
			blob = blob.getSmoothed(4);

			ofBeginShape();
				for (int j = 0; j < blob.size(); ++j) {
					ofVertex(blob[j]);
				}
			ofEndShape();
		}
	}

	ofPopMatrix();
	ofPopStyle();

}

void ofApp::drawHandText() {

	for (int i = 0; i < ContourFinder.hands.size(); ++i)
	{
		string palmText;
		unsigned int label = ContourFinder.hands[i].label;
		stableHands[label].resize(2);

		ofPoint center 	= ContourFinder.hands[i].centroid;
		ofPoint tip 	= ContourFinder.hands[i].tip;
		int side 		= ContourFinder.hands[i].side;
		if( ContourFinder.hands[i].velocity.length() < minVelocity ) {
			center = stableHands[label][0];
			tip = stableHands[label][1];
		}
		else {
			stableHands[label][0] = center;
			stableHands[label][1] = tip;
		}
		// Transform to proper reference frame
		center.x = center.x * kinect_w + kinect_x;
		center.y = center.y * kinect_h + kinect_y;
		tip.x = tip.x * kinect_w + kinect_x;
		tip.y = tip.y * kinect_h + kinect_y;

		for (auto iter=regions.begin(); iter!=regions.end(); ++iter)
		{
			ofPolyline trans = utility::transform(iter->second, video_x, video_y, video_w/video->getWidth(), video_h/video->getHeight());
			if( trans.inside(center) ) {
				palmText = iter->first;
			}
		}

		ofPushMatrix();
			ofTranslate(center.x, center.y);
			// Proper rotation
			float h = sqrt( pow(center.x - tip.x, 2) + pow(center.y - tip.y, 2) );
			float angle =  ofRadToDeg( asin( (tip.y - center.y) / h ));
			if(tip.x < center.x) angle *= -1;
			if (side == 1) angle += 180;
			if ( (side == 0 || side == 2 ) && tip.y < center.y ) 
				angle += 180;
			ofRotateZ(angle);
			ofRectangle textBox = font.getStringBoundingBox(palmText, 0, 0);
			ofPoint textCenter = textBox.getCenter();
			float tWidth = textBox.getWidth();
			float tHeight = textBox.getHeight();
			float tSize = max(tWidth, 2*tHeight);
			float size = ofDist(tip.x, tip.y, center.x, center.y);
			ofScale(size/tSize*TEXT_SCALE, size/tSize*TEXT_SCALE);
			ofTranslate(-textCenter.x, -textCenter.y);
			font.drawString(palmText, 0, 0);
		ofPopMatrix();
	}

	string palmText;

	for (auto iter=regions.begin(); iter!=regions.end(); ++iter)
	{
		ofPolyline trans = utility::transform(iter->second, video_x, video_y, video_w/video->getWidth(), video_h/video->getHeight());
		if( trans.inside(testPoint) ) {
			palmText = iter->first;
		}
	}

	ofPushStyle();
	ofSetColor(0,0,0);
	ofRect(testPoint.x-60,testPoint.y-30, 120, 60);
	

	ofPoint center = testPoint;

	ofPushMatrix();
		ofTranslate(center.x, center.y);
		// Proper rotation
		ofRectangle textBox = font.getStringBoundingBox(palmText, 0, 0);
		ofNoFill();
		ofSetColor(255,255,255);
		ofPoint textCenter = textBox.getCenter();
		float tWidth = textBox.getWidth();
		float tHeight = textBox.getHeight();
		float tSize = max(tWidth, 2*tHeight);
		float size = 60;
		ofScale(size/tSize*TEXT_SCALE, size/tSize*TEXT_SCALE);
		ofTranslate(-textCenter.x, -textCenter.y);
		font.drawString(palmText, 0, 0);
	ofPopMatrix();
	ofPopStyle();

}

void ofApp::drawFeedback() {

	ofPushStyle();
	if(bCalibrateKinect) {
		kinectImg.draw(0,0);
		ofPushStyle();
		ofSetColor(0,255,0);
		ofTranslate(crop_left, crop_top);
			ContourFinder.draw();

		for (int i = 0; i < ContourFinder.hands.size(); ++i)
		{
			ofSetColor(255,0,255);
			ofFill();
			ofCircle(ContourFinder.hands[i].end, 3);
			ofCircle(ContourFinder.hands[i].tip, 3);
			ofCircle(ContourFinder.hands[i].wrists[0], 3);
			ofCircle(ContourFinder.hands[i].wrists[1], 3);
			ofCircle(ContourFinder.hands[i].centroid, 3);
			ofSetColor(255,255,255);
			ofNoFill();
			ofCircle(ContourFinder.hands[i].tip, ContourFinder.MAX_HAND_SIZE);
			ofCircle(ContourFinder.hands[i].tip, ContourFinder.MIN_HAND_SIZE);
			ofCircle(ContourFinder.hands[i].wrists[0], ContourFinder.MAX_WRIST_WIDTH);
			ofCircle(ContourFinder.hands[i].wrists[0], ContourFinder.MIN_WRIST_WIDTH);
		}
		ofTranslate(-crop_left, -crop_top);
		ofPopStyle();
	}

	if(bCalibrateText) {
		ofPushStyle();
		ofPushMatrix();
		ofTranslate(video_x, video_y);
		ofScale(video_w/video->getWidth(), video_h/video->getHeight());
		int i = 0;
		for(auto iter=regions.begin(); iter!=regions.end(); ++iter) {
			ofSetColor( (i&1) * 255, (i&2) * 128, (i&4) * 64); // A color varying tool I'm quite proud of
			iter->second.draw();
			i++;
		}
		ofSetColor(255,255,255);
		ofPopStyle();
		ofPopMatrix();
	}

	stringstream reportStream;

	if(bCalibrateKinect) {
		reportStream << "Calibrating Kinect" << endl
		<< "kinect_x (left, right): " << kinect_x << endl
		<< "kinect_y (up, down): " << kinect_y << endl
		<< "kinect_w (W, w): " << kinect_w << endl
		<< "kinect_h (H, h): " << kinect_h << endl
		<< "Press ENTER to capture background." << endl
		<< "Press 'S' to save." << endl;
		if(ContourFinder.size() == 1) {
			ofRectangle rect = ofxCv::toOf(ContourFinder.getBoundingRect(0));
			reportStream 
			<< "left: " << rect.getLeft() << endl
			<< "right: " << rect.getRight() << endl
			<< "top: " << rect.getTop() << endl
			<< "bottom: " << rect.getBottom() << endl;
		}
	}
	if(bCalibrateVideo) {
		reportStream << "Calibrating Video" << endl
		<< "video_x (left, right): " << video_x << endl
		<< "video_y (up, down): " << video_y << endl
		<< "video_w (W, w): " << video_w << endl
		<< "video_h (H, h): " << video_h << endl
		<< "Press 'S' to save." << endl;
	}
	if(bCalibrateText) {
		reportStream << "Calibrating Hand Text" << endl
		<< "MAX_HAND_SIZE (H, h): " << ContourFinder.MAX_HAND_SIZE << endl
		<< "MIN_HAND_SIZE (J, j): " << ContourFinder.MIN_HAND_SIZE << endl
		<< "MAX_WRIST_WIDTH (W, w): " << ContourFinder.MAX_WRIST_WIDTH << endl
		<< "MIN_WRIST_WIDTH (E, e): " << ContourFinder.MIN_WRIST_WIDTH << endl
		<< "TEXT_SCALE (T, t): " << TEXT_SCALE << endl
		<< "Press 'S' to save." << endl;
	}
	if(!bCalibrateText && !bCalibrateVideo && !bCalibrateKinect) {
		reportStream
		<< "Frame: " << video->getCurrentFrame() << endl
		<< "CurrentPhase: " << currentPhase << endl
		<< "NextPhaseFrame: " << nextPhaseFrame << endl
		<< "Idle time: " << ofToString((ofGetSystemTimeMicros() - lastTime)/1000000) << endl
		<< "Framerate: " << ofToString(ofGetFrameRate()) << endl
		<< "Press 'C' to cycle through calibration modes." << endl
		<< "Press SPACE to start/stop playback." << endl;
	}
	
	ofDrawBitmapString(reportStream.str(), 100, 600);

	ofPopStyle();

}

void ofApp::loadSettings() {

	XML.pushTag(ofToString(PLATFORM));
		video_x = XML.getValue("VIDEO:X", 0);
		video_y = XML.getValue("VIDEO:Y", 0);
		video_w = XML.getValue("VIDEO:W", firstVideo.getWidth());
		video_h = XML.getValue("VIDEO:H", firstVideo.getHeight());

		XML.pushTag("KINECT");
				XML.pushTag("CROP");
					crop_left = XML.getValue("LEFT", 0);
					crop_right = XML.getValue("RIGHT", 0);
					crop_top = XML.getValue("TOP", 0);
					crop_bottom = XML.getValue("BOTTOM", 0);
				XML.popTag();
			if(REGISTRATION) 
				XML.pushTag("REGISTRATION");
			else
				XML.pushTag("NOREGISTRATION");
					kinect_w = XML.getValue("W", 2.77);
					kinect_h = XML.getValue("H", 2.77);
					kinect_x = XML.getValue("X", 0);
					kinect_y = XML.getValue("Y", 0);
				XML.popTag();
		XML.popTag();
	XML.popTag();

	// ContourFinder, you need to find these by moving one contour around the frame and seein the max/mins
	ContourFinder.bounds[0] = 1;
	ContourFinder.bounds[1] = 27; // TODO WHY???
	ContourFinder.bounds[2] = 600;
	ContourFinder.bounds[3] = 437;

	XML.pushTag("CV");
		ContourFinder.setMinArea(XML.getValue("MINAREA", 1000));
		ContourFinder.MAX_HAND_SIZE = XML.getValue("HAND:MAXSIZE", 99);
		ContourFinder.MIN_HAND_SIZE = XML.getValue("HAND:MINSIZE", 56);
		ContourFinder.MAX_WRIST_WIDTH = XML.getValue("WRIST:MAXSIZE", 33);
		ContourFinder.MIN_WRIST_WIDTH = XML.getValue("WRIST:MINSIZE", 15);

		TEXT_SCALE = XML.getValue("TEXTSCALE", 1);
	XML.popTag();

	farThreshold = XML.getValue("KINECT:FARTHRESHOLD", 2);
	nearThreshold = XML.getValue("KINECT:NEARTHRESHOLD", farThreshold + 40);
	screensaverTime = XML.getValue("SCREENSAVERTIME", 120000);

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

	switch(key) {

		case 'C': {
			if(bCalibrateKinect) {
				bCalibrateKinect = false;
				bCalibrateVideo = true;
			}
			else if (bCalibrateVideo) {
				bCalibrateVideo = false;
				bCalibrateText = true;
			}
			else if (bCalibrateText)
				bCalibrateText = false;
			else
				bCalibrateKinect = true; 
			break;
		}

		case OF_KEY_BACKSPACE:{
			bDisplayFeedback = !bDisplayFeedback;
			break;
		}

		case ' ': {
  			if(video->isPaused())
				video->play();
			else
				video->setPaused(true);
			break;
		}

		case OF_KEY_RETURN:
			bLearnBackground = true;
			break;

		case 'S': {
			// Saving calibration
			XML.pushTag(ofToString(PLATFORM));
				XML.pushTag("KINECT");
				if(REGISTRATION)
					XML.pushTag("REGISTRATION");
				else
					XML.pushTag("NOREGISTRATION");

						XML.setValue("X", kinect_x);
						XML.setValue("Y", kinect_y);
						XML.setValue("W", kinect_w);
						XML.setValue("H", kinect_h);
					XML.popTag();
				XML.popTag();
				XML.pushTag("VIDEO");
					XML.setValue("X", video_x);
					XML.setValue("Y", video_y);
					XML.setValue("W", video_w);
					XML.setValue("H", video_h);
				XML.popTag();
			XML.popTag();
			XML.pushTag("CV");
				XML.setValue("HAND:MAXSIZE", ContourFinder.MAX_HAND_SIZE);
				XML.setValue("HAND:MINSIZE", ContourFinder.MIN_HAND_SIZE);
				XML.setValue("WRIST:MAXSIZE", ContourFinder.MAX_WRIST_WIDTH);
				XML.setValue("WRIST:MINSIZE", ContourFinder.MIN_WRIST_WIDTH);
				XML.setValue("TEXTSCALE", TEXT_SCALE);
			XML.popTag();
			XML.saveFile("settings.xml");
			cout << "Settings saved!";
			break;
		}

		// Calibration stuff
		case OF_KEY_UP:
			if(bCalibrateKinect)
				kinect_y--;
			if(bCalibrateVideo)
				video_y--;
			break;

		case OF_KEY_DOWN:
			if(bCalibrateKinect)
				kinect_y++;
			if(bCalibrateVideo)
				video_y++;
			break;

		case OF_KEY_LEFT:
			if(bCalibrateKinect)
				kinect_x--;
			if(bCalibrateVideo)
				video_x--;
			break;

		case OF_KEY_RIGHT:
			if(bCalibrateKinect)
				kinect_x++;
			if(bCalibrateVideo)
				video_x++;
			break;

		case 'w':
			if(bCalibrateKinect)
				kinect_w -= 0.01;
			if(bCalibrateVideo)
				video_w--;
			if(bCalibrateText)
				ContourFinder.MAX_WRIST_WIDTH--;
			break;

		case 'W':
			if(bCalibrateKinect)
				kinect_w += 0.01;
			if(bCalibrateVideo)
				video_w++;
			if(bCalibrateText)
				ContourFinder.MAX_WRIST_WIDTH++;
			break;

		case 'h':
			if(bCalibrateKinect)
				kinect_h -= 0.01;
			if(bCalibrateVideo)
				video_h--;
			if(bCalibrateText)
				ContourFinder.MAX_HAND_SIZE--;
			break;

		case 'H':
			if(bCalibrateKinect)
				kinect_h += 0.01;
			if(bCalibrateVideo)
				video_h++;
			if(bCalibrateText)
				ContourFinder.MAX_HAND_SIZE++;
			break;

		case 'j':
			if(bCalibrateText)
				ContourFinder.MIN_HAND_SIZE--;
			break;

		case 'J':
			if(bCalibrateText)
				ContourFinder.MIN_HAND_SIZE++;
			break;

		case 'e':
			if(bCalibrateText)
				ContourFinder.MIN_WRIST_WIDTH--;
			break;

		case 'E':
			if(bCalibrateText)
				ContourFinder.MIN_WRIST_WIDTH++;
			break;

		case 't':
			if(bCalibrateText)
				TEXT_SCALE -= 0.01;
			break;

		case 'T':
			if(bCalibrateText)
				TEXT_SCALE += 0.01;
			break;

		case 'P':
			handSound.play();
			break;


	}

}

// For simulating hands
//--------------------------------------------------------------
void ofApp::mouseMoved(int mx, int my) {

	testPoint = ofPoint(mx, my);
}
