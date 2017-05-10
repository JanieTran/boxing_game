// Name        : boxing_game.cpp
// Author      : Pan
// Version     : 24 April 2017
// Description : This is a simple motion tracker that is packaged in a class

#include <cv.h>   // all the OpenCV headers, which links to all the libraries
#include <highgui.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/videoio/videoio.hpp>
#include <iostream>
#include <cmath>

using namespace std;
using namespace cv;   // a "shortcut" for directly using OpenCV functions

class ScreenObs {// This is the parent class of all objects on screen
public:
	// Attribute
	Scalar obColour;

	// Behaviours
	ScreenObs () {
		obColour = Scalar (0,0,255);
	}
	void setColour (Scalar set_obColour) {
		obColour = set_obColour;
	}
};

class moTracker: public ScreenObs {
public:
	//attributes
	Mat prePrev, prevFrame, curFrame; //saved frames for finding differences
	//includes 2nd preivous frame, previous frame, and current frame
	Mat colorOutput;

	int xROI, yROI, wROI, hROI; // set ROI's dimensions
	int xCOM, yCOM; //coordinates of Centre of Mass
	bool firstRun; //check whether it is the first time the programme runs

	int topLim, leftLim, botLim, rLim; // ROI can only move within this region
	bool limSet; // are specific limits set for ROI?

	// Attributes of a player
	int xHead, yHead, headRad; //head set
	int xlHand, ylHand, handRad, xrHand, yrHand; //hand set
	int p2Factor; // applied in calculation depends on which player is using function
	bool player2; // check if the ROI belongs to player 2
	bool safe;

	moTracker () {
		//default setROI
		xROI = 100;
		yROI = 200;
		wROI = 150;
		hROI = 150;

		//loop check
		firstRun = true; //by default loop has to go through first run
		limSet = false;  //by default no limits are set
		p2Factor = 1;
		player2 = false;
		safe = true;
	}
	
	void feedNewframe (Mat frame) {
		Mat diffPrev;

		int x, y; //coordinates of pixel
		//brightness of each pixel - on x,y-axis and the sum
		int xMass, yMass, sMass, m;

		if (firstRun) { //if this is the 1st Run!
			frame.copyTo (curFrame);
			frame.copyTo (prevFrame);
			frame.copyTo (prePrev);
			xCOM = xROI + wROI/2;
			yCOM = yROI + hROI/2;
			firstRun = false; // 1st run is over
		}
		prevFrame.copyTo (prePrev);
		curFrame.copyTo (prevFrame);
		frame.copyTo (curFrame);

		inRange(frame, Scalar(0,0,160), Scalar(80,80,255), colorOutput);

//		absdiff (prePrev, frame, diffPrev);
//		cvtColor (diffPrev, diffPrev, CV_BGR2GRAY, 1);

	// Compute COM
		sMass = xMass = yMass = 0;
		for (y = yROI; y < yROI + hROI; y++) {
			for (x = xROI; x < xROI + wROI; x++) {
				m = colorOutput.at<unsigned char>(y,x);
				sMass += m;
				xMass += m*x;
				yMass += m*y;
			}
		}
		if (sMass != 0) {
			xCOM = xMass/sMass;
			yCOM = yMass/sMass;
		}
	}

	void updateROI (Mat frame) {
		xROI = xCOM - wROI/2;
		yROI = yCOM - hROI/2;

		// Keeping the tracker inside boundaries
			if (limSet == false) {
				//default setLim
				topLim = 0;
				leftLim = 0;
				botLim = frame.rows;
				rLim = frame.cols;
			}
			if (xROI < leftLim) {
				xROI = leftLim;
			}
			if (xROI + wROI > rLim) {
				xROI = rLim - wROI;
			}
			if (yROI < topLim) {
				yROI = topLim;
			}
			if (yROI + hROI > botLim) {
				yROI = botLim - hROI;
			}
	}
	void drawROI (Mat frame) {
		rectangle (frame, Rect(xROI, yROI, wROI, hROI), obColour, 2);
	}

	// set ROI accordingly
	void setROI (int set_xROI, int set_yROI, int set_wROI,
			int set_hROI) {
		xROI = set_xROI;
		yROI = set_yROI;
		wROI = set_wROI;
		hROI = set_hROI;
	}

	//set limits accordingly
	void setLim (int set_topLim, int set_leftLim, int set_botLim, int set_rLim) {
		topLim = set_topLim;
		leftLim = set_leftLim;
		botLim =  set_botLim;
		rLim = set_rLim;
		limSet = true;
	}
	void separateROI (moTracker t1, int r1, int r2) {
		// keeps 2 ROIs on screen from overlapping one another
		// t1 has the coordinates and radius of reference ROI
		//the ROI using this function will have its position changed
		double delX = 0.0, delY = 0.0, rDist = 0.0;
		double alpha = 0.0; // This is the angle between the second object and
				// the horizon of the reference object (range from 0 to pi)
		delX = xCOM - t1.xCOM;
		delY = yCOM - t1.yCOM;
		rDist = r1 + r2; // Magnitude of total radius distance between objects
			// Calculate the distance between two object's centres
		int dist = round(sqrt(pow(delX, 2.0) + pow (delY, 2.0)));

		if (dist < rDist) {
			alpha = acos ((double) (delX/dist));
			if (delY < 0) {
				alpha = -alpha;
			}
			xCOM = (int) (t1.xCOM + cos(alpha)*rDist);
			yCOM = (int) (t1.yCOM + sin(alpha)*rDist);
		}
	}
	// Behaviours of a player
	void drawPlayer (Mat frame, moTracker leftFist, moTracker rFist) {
		if (player2) {
			xHead = frame.cols - xCOM;
			yHead = frame.rows - yCOM;
			xlHand = frame.cols - leftFist.xCOM;
			ylHand = frame.rows - leftFist.yCOM;
			xrHand = frame.cols - rFist.xCOM;
			yrHand = frame.rows - rFist.yCOM;
		} else {
			xHead = xCOM;
			yHead = yCOM;
			xlHand = leftFist.xCOM;
			ylHand = leftFist.yCOM;
			xrHand = rFist.xCOM;
			yrHand = rFist.yCOM;
		}
		headRad = (int)(wROI/4 * sqrt(2.0));
		handRad = (int)(rFist.wROI/4);

				// A black background to erase previous image
		rectangle (frame, Rect(0,0, frame.cols, frame.rows), Scalar (0,0,0), -1);
				// Head
		circle (frame, Point(xHead, yHead), headRad, obColour, -1);
				// Arms
		line (frame, Point (xHead, yHead + p2Factor*headRad), Point (xlHand, ylHand),
				obColour, 10);
		line (frame, Point (xHead, yHead + p2Factor*headRad), Point (xrHand, yrHand),
				obColour, 10);
				// Fists
		circle (frame, Point(xlHand, ylHand), handRad, obColour, -1);
		circle (frame, Point(xrHand, yrHand), handRad, obColour, -1);
	}
	void p2 () {
		p2Factor = -1;
		player2 = true;
	}
	void separatePlayers (Mat frame, moTracker t1, int r1, int r2, bool hit) { // Hit
		// keeps 2 players from overlapping one another
		// t1 has the coordinates and radius of reference player
		//the ROI using this function will have its position changed
		double delX = 0.0, delY = 0.0, rDist = 0.0;
		double alpha = 0.0; // This is the angle between the second object and
				// the horizon of the reference object (range from 0 to pi)
		delX = p2Factor*((xCOM + t1.xCOM) - frame.cols);
		delY = p2Factor*((yCOM + t1.yCOM) - frame.rows);
		rDist = r1 + r2; // Magnitude of total radius distance between objects
			// Calculate the distance between two object's centres
		int dist = round(sqrt(pow(delX, 2.0) + pow (delY, 2.0)));

		if (dist < rDist) {
			alpha = acos ((double) (delX/dist));
			if (delY < 0) {
				alpha = -alpha;
			}
			xCOM = (int) (frame.cols - t1.xCOM + p2Factor*cos(alpha)*rDist);
			yCOM = (int) (frame.rows - t1.yCOM + p2Factor*sin(alpha)*rDist);
//			cout<<"Updated: "<< xCOM <<", "<< yCOM <<endl;
			if (hit) {
				t1.safe = false; //???????????????????????????????????????????????????????
			}
		} else if (dist > rDist + 10 && hit == true) {
			t1.safe = true;
		}
		//			cout<<"Updated safe zone: "<< t1.safe <<endl;
	}
	void safezone () {

	}
};
// Finish motion tracker & player setup

class Stamina: public moTracker { //(hit, defend, recovery)
public:
	// Attributes
	int xBar, yBar, wBar, hBar, maxStat; //Stamina bar attributes
	// Behaviours
	Stamina () {
		hBar = 30;
		maxStat = 160;
	}
	void bar (Mat frame, moTracker player) {
		int xBox, yBox, wBox, hBox;
		bool firstRun = true;
		if (player.player2) {
			xBar = (int)(frame.cols*1/16);
			yBar = (int)(frame.rows*1/16);
		} else {
			xBar = (int)(frame.cols*11/16);
			yBar = (int)(frame.rows*14/16);
		}
		if (firstRun) {
			wBar = maxStat;
			firstRun = false;
			xBox = xBar - 5;
			yBox = yBar - 5;
			wBox = wBar + 10;
			hBox = hBar + 10;
		}

		rectangle (frame, Rect(xBar, yBar, wBar, hBar), Scalar (100,255,100), -1);
		rectangle (frame, Rect(xBox, yBox, wBox, hBox), Scalar (255,255,255), 2);
	}
};

int main(  int argc, char** argv ) {

	int frameCount;
	Mat frame;
	Mat frame2;

	VideoCapture cap(0); // open the default camera
	if (!cap.isOpened()) {  // check if we succeeded
		return -1;
	}
//	VideoCapture cap2(1); // open the default camera
//	if (!cap2.isOpened()) {  // check if we succeeded
//		return -1;
//	}
	cap  >> frame;
//	cap2 >> frame2;

	Mat game = Mat (frame.rows, frame.cols, CV_8UC3);
	moTracker p1, p1LHand, p1RHand;
//	p1.p2(); p1LHand.p2(); p1RHand.p2();					//delete/////////////////////
//	moTracker p2, p2LHand, p2RHand;
//	p2.p2(); p2LHand.p2(); p2RHand.p2();
	Stamina p1Stat;

	p1.setROI (frame.cols/2 - 100, 200, 200, 200);
	p1.setLim (frame.rows/2, (int)(frame.cols*1/7), frame.rows,
			(int)(frame.cols*6/7));
	p1LHand.setROI (10, 300, 150, 150);
	p1LHand.setColour (Scalar (255,0,0));					//delete/////////////////////
	p1RHand.setROI (frame.cols-10, 300, 150, 150);

	int headROIradius = (int)(p1.wROI/3);
	int handROIradius = (int)(p1LHand.wROI/3);

	namedWindow("Player 1 ROI", CV_WINDOW_AUTOSIZE);
	namedWindow("Player 2 ROI", CV_WINDOW_NORMAL);
	namedWindow("Boxing Game 1", CV_WINDOW_AUTOSIZE);
	namedWindow("Boxing Game 2", CV_WINDOW_AUTOSIZE);

	for(frameCount = 0; frameCount < 100000000; frameCount++) {

		if (frameCount % 100 == 0) {
			printf("frameCount = %d \n", frameCount);
		}

		cap >> frame; // get a new frame from camera
//		cap2 >> frame2;

		flip (frame, frame, 1); // flip frame horizontally
		flip (frame2, frame2, 1);
				//Calculate COM and feed each frame captured
		p1.feedNewframe(frame);
		p1LHand.feedNewframe(frame);
		p1RHand.feedNewframe(frame);

				// Separate the ROIs of one player
		p1LHand.separateROI (p1, headROIradius, handROIradius);
		p1RHand.separateROI (p1, headROIradius, handROIradius);
		if (p1.xlHand + p1.handRad < p1.xHead) {
			p1RHand.separateROI (p1LHand, p1.handRad, p1.handRad);
		}
		if (p1.xrHand - p1.handRad >= p1.xHead) {
			p1LHand.separateROI (p1RHand, p1.handRad, p1.handRad);
		}

		p1.updateROI(frame);
		p1LHand.updateROI(frame);
//delete		cout<<"New COM: "<< p1LHand.xCOM<<", "<< p1LHand.yCOM <<endl;
		p1RHand.updateROI(frame);

				//Visualise each ROI on frame
		p1.drawROI (frame);
		p1LHand.drawROI (frame);
		p1RHand.drawROI (frame);

		imshow("Player 1 ROI", frame);
//		imshow("Player 2 ROI", frame2);

		p1.drawPlayer (game, p1LHand, p1RHand);
		p1Stat.bar (game, p1);

		imshow("Boxing Game 1", game);
//		flip (game, game, -1); // flip image on both axes
//		imshow("Boxing Game 2", game);
//		flip (game, game, -1);

		if (waitKey(20) >= 0) {
			break;
		}
	}

	printf("Final frameCount = %d \n", frameCount);
	return 0;
}

