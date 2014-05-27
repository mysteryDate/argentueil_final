// Argentueil_final
#include "ContourFinder.h"
#include "ofMain.h"

class ArmContourFinder : public ofxCv::ContourFinder
{
public:

	ArmContourFinder();

	// The keys are the labels
	map< unsigned int, ofPoint > ends;
	map< unsigned int, ofPoint > tips;
	map< unsigned int, vector< ofPoint > > wrists;
	map< unsigned int, bool > handFound;
	map< unsigned int, int > side;

	void update();
	struct Hand
	{
		ofPolyline line;
		ofPoint centroid;
		ofPoint tip;
		ofPoint end;
		ofPoint boxCenter;
		vector< ofPoint > wrists;
		int index;
		unsigned int label;
		ofVec2f velocity;
		int side;
		// Used for finding velocity
		vector<ofPoint> previousPositions;

		//For sorting by label
		bool operator < (const Hand& str) const
		{
			return (label < str.label);
		}
	};
	vector< Hand >		hands;
	ofPolyline getHand(int n);
	
	int MIN_HAND_SIZE;
	int MAX_HAND_SIZE;
	int MAX_WRIST_WIDTH;
	int MIN_WRIST_WIDTH;
	int MAX_MOVEMENT_DISTANCE;
	int smoothingRate;

	vector< int > bounds;

private:

	bool findHand(int n);
	void 				updateHands();
	ofPoint			 	findEnd(int n);
	ofPoint				findTip(int n);
	vector< ofPoint > 	findWrists(int n);
	ofPoint 			refitTip(int n);

	void addHand(int n);

};