#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main( ){
	ofSetupOpenGL(1920,1080,OF_FULLSCREEN); // <-------- setup the GL context
	//ofSetupOpenGL(10,10,OF_WINDOW);			
	// this kicks off the running of my app
	// can be OF_WINDOW || OF_FULLSCREEN
	// pass in width && height too:
	ofRunApp(new ofApp());

}