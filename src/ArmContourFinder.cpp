// argenteuil_final
#include "ArmContourFinder.h"

ArmContourFinder::ArmContourFinder() {

	// Find from the bounding boxes
	bounds.push_back(22);
	bounds.push_back(42);
	bounds.push_back(601);
	bounds.push_back(438);

	setMinArea(50);

	MIN_HAND_SIZE = 56;
	MAX_HAND_SIZE = 75;
	MAX_WRIST_WIDTH = 33;
	MIN_WRIST_WIDTH = 15;

	smoothingRate = 0.5;

}

void ArmContourFinder::update() {

	//To run every frame
	for (int i = 0; i < polylines.size(); ++i)
	{
		handFound[getLabel(i)] = findHand(i);
	}
	updateHands();
}

void ArmContourFinder::updateHands() {

	vector < Hand > newHands;
	newHands.clear();

	for (int i = 0; i < size(); ++i)
	{
		unsigned int l = getLabel(i);
		if(handFound[l]) {
			Hand blob;
			blob.label = l;
			blob.side = side[l];
			blob.line = getHand(i);
			blob.centroid = blob.line.getCentroid2D();
			blob.tip = tips[l];
			blob.wrists = wrists[l];
			blob.end = ends[l];
			blob.boxCenter = ofxCv::toOf(getCenter(i));
			blob.index = i;
			newHands.push_back(blob);
		}
	}

	sort(newHands.begin(), newHands.end());

	// Remove dead ones
	for (int i = 0; i < hands.size(); ++i)
	{
		bool found = false;
		unsigned int label = hands[i].label;
		for (int j = 0; j < newHands.size(); ++j)
		{
			if(newHands[j].label == label) {
				hands[i].index = newHands[j].index;
				found = true;
				break;
			}
		}
		if(!found) {
			hands.erase( hands.begin() + i );
			i--; //So that it doesn't skip the next one
		}
	}

	//Add new ones
	for (int i = 0; i < newHands.size(); ++i)
	{
		bool found = false;
		unsigned int label = newHands[i].label;
		for (int j = 0; j < hands.size(); ++j)
		{
			if(hands[j].label == label) {
				hands[j].index = newHands[i].index;
				found = true;
				break;
			}
		}
		if(!found) {
			hands.push_back(newHands[i]);
		}
	}
	sort(hands.begin(), hands.end());

	//Finally, the magic
	for (int i = 0; i < hands.size(); ++i)
	{
		Hand handCopy = newHands[i];
		// Doesn't copy vectors, do it by 'hand' for now
		handCopy.wrists = newHands[i].wrists;
		handCopy.velocity = newHands[i].velocity;

		ofPoint oldKeypoints[] = {hands[i].centroid, hands[i].end, hands[i].tip, hands[i].wrists[0], hands[i].wrists[1]};
		ofPoint * keypoints[] = {&handCopy.centroid, &handCopy.end, &handCopy.tip, &handCopy.wrists[0], &handCopy.wrists[1]};


		if( !(newHands[i].centroid.x == 0 && newHands[i].centroid.y == 0) ) 
		{
			for (int j = 0; j < 5; ++j)
			{
				float smoothedX = ofLerp(keypoints[j]->x, oldKeypoints[j].x, smoothingRate);
				float smoothedY = ofLerp(keypoints[j]->y, oldKeypoints[j].y, smoothingRate);
				*keypoints[j] = ofPoint(smoothedX, smoothedY);
			}
			// handCopy.velocity = ofVec2f((keypoints[0]->x - oldKeypoints[0].x)/2, (keypoints[0]->y - oldKeypoints[0].y)/2);	
		}

		// Lerped  veloctiy
		vector< ofPoint > prevs = hands[i].previousPositions;
		prevs.insert(prevs.begin(),handCopy.centroid);

		ofPoint oldAverage = ofPoint(0,0);
		ofPoint presentAverage = ofPoint(0,0);
		for (int j = 0; j < prevs.size()/2; ++j)
		{
			oldAverage.x += prevs[j + prevs.size()/2].x;
			presentAverage.x += prevs[j].x;
			oldAverage.y += prevs[j + prevs.size()/2].y;
			presentAverage.y += prevs[j].y;
		}
        
        oldAverage.x /= prevs.size()/2;
		presentAverage.x /= prevs.size()/2;
		oldAverage.y /= prevs.size()/2;
		presentAverage.y /= prevs.size()/2;

		handCopy.velocity = ofVec2f((presentAverage.x - oldAverage.x), (presentAverage.y - oldAverage.y));
        if(prevs.size() > 12) prevs.resize(12);
        handCopy.previousPositions = prevs;

		hands[i] = handCopy;

	}
}

ofPolyline ArmContourFinder::getHand(int n) {

	ofPolyline hand;

	if(!handFound[getLabel(n)]) return hand;

	unsigned int start, end;
	polylines[n].getClosestPoint(wrists[getLabel(n)][1], &start);
	polylines[n].getClosestPoint(wrists[getLabel(n)][0], &end);


	int i = start;
	while( i != end ) {
		hand.addVertex( polylines[n][i] );
		i++;
		if( i == polylines[n].size() )
			i = 0;
	}
	hand.addVertex( polylines[n][end] );
	
	// So that it closes up;
	hand.setClosed(true);

	return hand;

}

bool ArmContourFinder::findHand(int n) {

	unsigned int l = getLabel(n);
	//First, find ends
	ends[l] = findEnd(n);
	if( ends[l].x == -1 && ends[l].y == -1)
		return false;

	//Now, get the tip
	tips[l] = findTip(n);
	//See if it's far enough away
	float d1 = ofDistSquared(tips[l].x, tips[l].y, ends[l].x, ends[l].y);
	float d2 = ofDistSquared(tips[l].x, tips[l].y, ends[l].x, ends[l].y);
	if( d1 < MIN_HAND_SIZE * MIN_HAND_SIZE && d2 < MIN_HAND_SIZE * MIN_HAND_SIZE )
		return false; // Too small!
	// if( d1 > MAX_HAND_SIZE * MAX_HAND_SIZE && d2 > MAX_HAND_SIZE * MAX_HAND_SIZE )
	// 	return false; // Too big!

	//Now find the wrists
	wrists[l] = findWrists(n);
	if( wrists[l].size() != 2 ) return false;

	tips[l] = refitTip(n);

	d1 = ofDistSquared(wrists[l][0].x, wrists[l][0].y, tips[l].x, tips[l].y);
	d2 = ofDistSquared(wrists[l][1].x, wrists[l][1].y, tips[l].x, tips[l].y);
	if(d1 <= MIN_HAND_SIZE * MIN_HAND_SIZE || d1 >= MAX_HAND_SIZE * MAX_HAND_SIZE 
		 || d2 <= MIN_HAND_SIZE * MIN_HAND_SIZE || d2 >= MAX_HAND_SIZE * MAX_HAND_SIZE) {
		wrists[l] = findWrists(n);
		if(wrists[l].size() != 2)
			return false;
	}

	return true;

}

ofPoint ArmContourFinder::findEnd(int n) {

	vector< ofPoint > pts = polylines[n].getVertices();
	vector< ofPoint > endPoints;
	ofPoint center = ofxCv::toOf(getCenter(n));
	unsigned int l = getLabel(n);

	int sides [4] = {0,0,0,0};
	for (int i = 0; i < pts.size(); ++i)
	{
		if(pts[i].x <= bounds[0] + 2 || pts[i].y <= bounds[1] + 2
 			|| pts[i].x >= bounds[2] - 2 || pts[i].y >=  bounds[3] - 2) {
			endPoints.push_back(pts[i]);
		}
		if(pts[i].x <= bounds[0] + 2)
			sides[0]++;
		if(pts[i].y <= bounds[1] + 2)
			sides[1]++;
		if(pts[i].x >= bounds[2] - 2)
			sides[2]++;
		if(pts[i].y >= bounds[3] - 2)
			sides[3]++;
	}
	int maxSide = 0;
	for (int i = 0; i < 4; ++i)
	{
		if(sides[i] > maxSide) {
			maxSide = sides[i];
			side[l] = i;
		}
	}
	if(endPoints.size() > 0) {
		// Just take the one that's the farthest from the center of the box
		float maxDist = 0;
		for (int i = 0; i < endPoints.size(); ++i)
		{
			float dist = ofDistSquared(center.x, center.y, endPoints[i].x, endPoints[i].y);
			if(dist > maxDist) {
				maxDist = dist;
				endPoints[0] = endPoints[i];
			}
		}

		endPoints.resize(1);

		// if(endPoints[0].x <= bounds[0] + 2) side[l] = 0;
		// else if(endPoints[0].y <= bounds[1] + 2) side[l] = 1;
		// else if(endPoints[0].x >= bounds[2] - 2) side[l] = 2;
		// else if(endPoints[0].y >= bounds[3] - 2) side[l] = 3;
	}


	if(endPoints.size() == 0) {
		if(handFound[l]) {
			ofPoint centroid = polylines[n].getCentroid2D();
			int thisSide = side[l];
			// assume they're still on the same side
			ofPoint mark;
			if(thisSide == 0 || thisSide == 2)
				mark = ofPoint(bounds[thisSide], centroid.y);
			else
				mark = ofPoint(centroid.x, bounds[thisSide]);
			endPoints.push_back(polylines[n].getClosestPoint(mark));
		}
		else return ofPoint(-1, -1);
	}



	// New tactic!
	ofPolyline rotatedRect = ofxCv::toOf(getMinAreaRect(n));
	vector< ofPoint > verts = rotatedRect.getVertices();

	// Remove two farthest from endpoint
	for (int i = 0; i < 2; ++i)
	{
		float maxDist = 0;			
		int indexToRemove;
		for (int i = 0; i < verts.size(); ++i)
		{
			float dist = ofDistSquared(endPoints[0].x, endPoints[0].y, verts[i].x, verts[i].y);
			if(dist > maxDist) {
				maxDist = dist;
				indexToRemove = i;
			}
		}
		verts.erase(verts.begin() + indexToRemove);
	}


	ofPoint end;

	float maxDist = 0;
	vector< float > distances;
	for (int i = 0; i < verts.size(); ++i)
	{
		float dist = ofDistSquared(verts[i].x, verts[i].y, center.x, center.y);
		distances.push_back(dist);
		if(dist > maxDist) {
			maxDist = dist;
			end = verts[i];
		}
	}

	if(distances.size() == 2) {
		if( abs(distances[0] - distances[1]) < 400 ) {
			float d1 = ofDistSquared(ends[l].x, ends[l].y, verts[0].x, verts[0].y);
			float d2 = ofDistSquared(ends[l].x, ends[l].y, verts[1].x, verts[1].y);
			if(d1 < d2)
				end = verts[0];
			else
				end = verts[1];
		}
	}


	return end;

}

ofPoint ArmContourFinder::findTip(int n) {

	//Create a line connecting the center of the base of the arm to the center of the bounding box
	ofPoint boxCenter = ofxCv::toOf(getCenter(n));
	unsigned int l = getLabel(n);

	float xn = 3*boxCenter.x - 2*ends[l].x;
	float yn = 3*boxCenter.y - 2*ends[l].y;
	
	ofPoint mark = ofPoint(xn, yn);

	ofPoint newTip = polylines[n].getClosestPoint(mark);

	//If our old tip is still good, keep it
	ofPoint closestTip = polylines[n].getClosestPoint(tips[getLabel(n)]);
	float dist = ofDistSquared(closestTip.x, closestTip.y, newTip.x, newTip.y);
	if(dist < 100) { //TODO change magic number
		return closestTip;
	}

	return newTip;

}

vector < ofPoint > ArmContourFinder::findWrists(int n) {

	unsigned int l = getLabel(n);
	//Square our distances now, because premature optimization
	int minSquared = MIN_HAND_SIZE * MIN_HAND_SIZE;
	int maxSquared = MAX_HAND_SIZE * MAX_HAND_SIZE;
	int maxWrist = MAX_WRIST_WIDTH * MAX_WRIST_WIDTH;
	int minWrist = MIN_WRIST_WIDTH * MIN_WRIST_WIDTH;
	float distSquared;
	if(wrists[l].size() == 2) {
		//If the old wrists still work, keep em
		vector< ofPoint > closestWrists;
		closestWrists.push_back(polylines[n].getClosestPoint(wrists[l][0]));
		closestWrists.push_back(polylines[n].getClosestPoint(wrists[l][1]));
		distSquared = ofDistSquared(closestWrists[0].x, closestWrists[0].y, closestWrists[1].x, closestWrists[1].y);
		if(distSquared <= maxWrist && distSquared >= minWrist) {
			float d1 = ofDistSquared(closestWrists[0].x, closestWrists[0].y, tips[l].x, tips[l].y);
			float d2 = ofDistSquared(closestWrists[1].x, closestWrists[1].y, tips[l].x, tips[l].y);
			if(d1 >= minSquared && d1 <= maxSquared && d2 >= minSquared && d2 <= maxSquared)
				return closestWrists;
		}
	}

	//One polyline for each side of the hand
	ofPolyline sideOne, sideTwo;

	//The indeces at which the split will occur
	unsigned int start, end;

	//Find those indeces, not sure if this is computationally expensive (premature optimization, anyone?)
	polylines[n].getClosestPoint(tips[l], &start);
	polylines[n].getClosestPoint(ends[l], &end);


	//Put all verteces within the right distance in one set
	int i = start;
	while(i != end) {
		distSquared = ofDistSquared(tips[l].x, tips[l].y, polylines[n][i].x, polylines[n][i].y);
		if(distSquared <= maxSquared && distSquared >= minSquared)
			sideOne.addVertex( polylines[n][i] );
		i++;
		if( i == polylines[n].size() )
			i = 0;
	}

	//Now grab all the suitable ones on the other side
	i = start;
	while(i != end) {
		distSquared = ofDistSquared(tips[l].x, tips[l].y, polylines[n][i].x, polylines[n][i].y);
		if(distSquared <= maxSquared && distSquared >= minSquared)
			sideTwo.addVertex( polylines[n][i] );
		i--;
		if( i < 0 )
			i = polylines[n].size() - 1;
	}

	// Now find the closest two points on these lines
	float shortestDist = maxWrist + 1;
	vector< ofPoint > possibleWrists;
	for (int i = 0; i < sideOne.size(); ++i)
	{
		for (int j = 0; j < sideTwo.size(); ++j)
		{
			distSquared = ofDistSquared(sideOne[i].x, sideOne[i].y, sideTwo[j].x, sideTwo[j].y);
			if(distSquared < shortestDist && distSquared <= maxWrist && distSquared >= minWrist) {
				possibleWrists.resize(2);
				possibleWrists[0] = sideOne[i];
				possibleWrists[1] = sideTwo[j];
				shortestDist = distSquared;
			}
		}
	}

	return possibleWrists;

}

ofPoint ArmContourFinder::refitTip(int n)
{	
	unsigned int l = getLabel(n);
	ofPoint midWrist = ofPoint( (wrists[l][0].x + wrists[l][1].x)/2, (wrists[l][0].y + wrists[l][1].y)/2 );

	unsigned int start, end;
	polylines[n].getClosestPoint(wrists[l][1], &start);
	polylines[n].getClosestPoint(wrists[l][0], &end);

	ofPoint newTip;
	float maxDist = 0;
	int i = start;
	while( i != end ) {
		float dist = ofDistSquared(midWrist.x, midWrist.y, polylines[n][i].x, polylines[n][i].y);
		if(dist > maxDist) {
			maxDist = dist;
			newTip = polylines[n][i];
		}
		i++;
		if( i == polylines[n].size() )
			i = 0;
	}

	ofPoint closestTip = polylines[n].getClosestPoint(tips[getLabel(n)]);
	float dist = ofDistSquared(closestTip.x, closestTip.y, newTip.x, newTip.y);
	if(dist < 100) { //TODO change magic number
		return closestTip;
	}
	
	return newTip;
}

