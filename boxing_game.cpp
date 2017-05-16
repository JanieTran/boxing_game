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
#include <string>

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
	// ROI Attributes
	Mat prePrev, prevFrame, curFrame; //saved frames for finding differences
	//includes 2nd preivous frame, previous frame, and current frame
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
	bool isTouching, hit;
	int hitNo;
	int xlEye;
	int xrEye;
	int yEye;

	//Stamina bar attributes
	int xBar, yBar, wBar, hBar, maxStat;
	int xBox, yBox, wBox, hBox;
	int countFrame;

	moTracker () {
		//default setROI
		xROI = 100;
		yROI = 200;
		wROI = 50;
		hROI = 50;

		hBar = 30;
		maxStat = 160;

		//loop check
		firstRun = true; //by default loop has to go through first run
		limSet = false;  //by default no limits are set
		p2Factor = 1;
		player2 = false;
		isTouching = false;
		hit = false;
		hitNo = 0;
		countFrame = 0;
	}
	void feedNewframe (Mat frame, Scalar darker, Scalar brighter) {
		Mat diffPrev;
		Mat colorOutput;

		int x, y; //coordinates of pixel
		//brightness of each pixel - on x,y-axis and the sum
		int xMass, yMass, sMass, m;

		if (firstRun) { //if this is the 1st Run!
			xCOM = xROI + wROI/2;
			yCOM = yROI + hROI/2;
		// Set up stamina bar
			if (player2) {
				xBar = (int)(frame.cols*1/16);
				yBar = (int)(frame.rows*1/16);
			} else {
				xBar = (int)(frame.cols*11/16);
				yBar = (int)(frame.rows*14/16);
			}
			wBar = maxStat;
			xBox = xBar - 5;
			yBox = yBar - 5;
			wBox = wBar + 10;
			hBox = hBar + 10;
			firstRun = false; // 1st run is over
		}

		inRange(frame, darker, brighter, colorOutput);

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
		int eyeRad = (int) headRad/10;

		headRad = (int)(wROI/2);
		handRad = (int)(rFist.wROI/2);

		if (player2) {
			xHead = frame.cols - xCOM;
			yHead = frame.rows - yCOM;
			xlHand = frame.cols - leftFist.xCOM;
			ylHand = frame.rows - leftFist.yCOM;
			xrHand = frame.cols - rFist.xCOM;
			yrHand = frame.rows - rFist.yCOM;
			xlEye = frame.cols - (xCOM - headRad/3);
			xrEye = frame.cols - (xCOM + headRad/3);
			yEye = frame.rows - (yCOM - headRad/3);
		} else {
			xHead = xCOM;
			yHead = yCOM;
			xlHand = leftFist.xCOM;
			ylHand = leftFist.yCOM;
			xrHand = rFist.xCOM;
			yrHand = rFist.yCOM;
			xlEye = xCOM - headRad/3;
			xrEye = xCOM + headRad/3;
			yEye = yCOM - headRad/3;
		}

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
				// Eyes
		if (hit == true) {
			line(frame, Point(xlEye - 10, yEye - 10), Point(xlEye + 10, yEye + 10), Scalar(0,0,0), 4);
			line(frame, Point(xlEye + 10, yEye - 10), Point(xlEye - 10, yEye + 10), Scalar(0,0,0), 4);

			line(frame, Point(xrEye - 10, yEye - 10), Point(xrEye + 10, yEye + 10), Scalar(0,0,0), 4);
			line(frame, Point(xrEye + 10, yEye - 10), Point(xrEye - 10, yEye + 10), Scalar(0,0,0), 4);
		} else {
			circle(frame, Point(xlEye, yEye), eyeRad, Scalar(0,0,0), -1);
			circle(frame, Point(xrEye, yEye), eyeRad, Scalar(0,0,0), -1);
		}
				// Smile
		int xSmile = xHead;
		int ySmile = yHead;

		if (hit == true) {
			if (player2) {
				circle(frame, Point(xHead, yHead - 20), 20, Scalar(0,0,0), -1);
			} else {
				circle(frame, Point(xHead, yHead + 40), 20, Scalar(0,0,0), -1);
			}
		} else {
			if (player2) {
				ellipse(frame, Point(xSmile, ySmile), Size(headRad/2, headRad/2), 180, 0, 180, Scalar(0,0,0), 4, 8);
			} else {
				ellipse(frame, Point(xSmile, ySmile), Size(headRad/2, headRad/2), 180, 0, -180, Scalar(0,0,0), 4, 8);
			}
		}

	}
	void p2 () {
		p2Factor = -1;
		player2 = true;
	}
	void separatePlayers (Mat frame, moTracker t1, int r1, int r2, bool Hit) { // Hit
		// keeps 2 players from overlapping one another
		// t1 has the coordinates and radius of reference player
		//the ROI using this function will have its position changed
		int rDist = 0;
		double delX = 0.0, delY = 0.0;
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
		}
		if (dist > rDist - 10 && dist < rDist + 10 && Hit == true){
			isTouching = true;
		}
		else if (dist > (rDist + 30) && Hit == true) {
			isTouching = false;
		}
	}
	void stamina (Mat frame, moTracker LFist, moTracker rFist) {
		// This function is only used by head motion tracker of each player

		if (LFist.isTouching || rFist.isTouching) {
			if (!hit && hitNo < 5) {
				hitNo ++;
				hit = true;
			}
		} else if (!LFist.isTouching && !rFist.isTouching) { hit = false; }
		if (!hit) {
			countFrame++;
			if (countFrame % 100 == 0 && hitNo > 0) {
				hitNo--;
			}
		} else countFrame = 0;
				// Visualise stamina bar
		wBar = (5 - hitNo)*maxStat/5;

		if (!player2) {
			xBar = (int)(frame.cols*11/16) + hitNo/5*maxStat;
		}
		// representation of stamina
		rectangle (frame, Rect(xBar, yBar, wBar, hBar),
									Scalar (0,0,255), -1);

		// representation of stamina's case
		rectangle (frame, Rect(xBox, yBox, wBox, hBox),
									Scalar (255,255,255), 2);

	}
};
// Finish motion tracker & player setup

int main(  int argc, char** argv ) {

	int frameCount;
	Mat frame;
	Mat frame2;

	FILE *history;

	string winner;

	char player1[10];
	char player2[10];

	do {
		printf("Enter name for Player 1: \n");
		scanf("%s", player1);
	} while (player1 == NULL);

	do {
		printf("Enter name for Player 2: \n");
		scanf("%s", player2);
	} while (player2 == NULL);

	string player1Str = player1;
	string player2Str = player2;

	VideoCapture cap(0); // open the default camera
	if (!cap.isOpened()) {  // check if we succeeded
		return -1;
	}
	VideoCapture cap2(1); // open the default camera
	if (!cap2.isOpened()) {  // check if we succeeded
		return -1;
	}
	cap  >> frame;
	cap2 >> frame2;

			// Declare players
	Mat game = Mat (frame.rows, frame.cols, CV_8UC3);
	Mat game2 = Mat (frame.rows, frame.cols, CV_8UC3);
	moTracker p1, p1LHand, p1RHand;
	moTracker p2, p2LHand, p2RHand;
	p2.p2(); p2LHand.p2(); p2RHand.p2();

			// Setup players
	p1.setROI (frame.cols/2 - 100, 200, 150, 150);
	p1.setColour(Scalar(255,0,0));
	p1.setLim (frame.rows/2, (int)(frame.cols*1/7), frame.rows,
			(int)(frame.cols*6/7));
	p1LHand.setROI (10, 300, 100, 100);
	p1RHand.setROI (frame.cols-10, 300, 100, 100);

	p2.setROI (frame2.cols/2 - 100, 200, 150, 150);
	p2.setLim (frame2.rows/2, (int)(frame2.cols*1/7), frame2.rows,
			(int)(frame2.cols*6/7));
	p2LHand.setROI (10, 300, 100, 100);
	p2.setColour (Scalar (0,255,0));					
	p2RHand.setROI (frame2.cols-10, 300, 100, 100);

	int headROIradius = (int)(p1.wROI/3);
	int handROIradius = (int)(p1LHand.wROI/3);

	namedWindow("Player 1 ROI", CV_WINDOW_NORMAL);
	namedWindow("Player 2 ROI", CV_WINDOW_NORMAL);
	namedWindow("Boxing Game 1", CV_WINDOW_NORMAL);
	namedWindow("Boxing Game 2", CV_WINDOW_NORMAL);

	for(frameCount = 0; frameCount < 100000000; frameCount++) {

		cap >> frame; // get a new frame from camera
		cap2 >> frame2;

				//Calculate COM and feed each frame captured
		p1.feedNewframe(frame, Scalar(10,10,194), Scalar(125,125,249));
		p1LHand.feedNewframe(frame, Scalar(0,164,164), Scalar(125,255,255));
		p1RHand.feedNewframe(frame, Scalar(0,164,164), Scalar(125,255,255));

		p2.feedNewframe(frame2, Scalar(10,10,194), Scalar(125,125,249));
		p2LHand.feedNewframe(frame2, Scalar(0,164,164), Scalar(125,255,255));
		p2RHand.feedNewframe(frame2, Scalar(0,164,164), Scalar(125,255,255));

				// Separate the ROIs of one player
		p1LHand.separateROI (p1, headROIradius, handROIradius);
		p1RHand.separateROI (p1, headROIradius, handROIradius);
		if (p1.xlHand + p1.handRad < p1.xHead) {
			p1RHand.separateROI (p1LHand, p1.handRad, p1.handRad);
		} else {
			p1LHand.separateROI (p1RHand, p1.handRad, p1.handRad);
		}

		p2LHand.separateROI (p2, headROIradius, handROIradius);
		p2RHand.separateROI (p2, headROIradius, handROIradius);
		if (p2.xlHand + p2.handRad < p2.xHead) {
			p2RHand.separateROI (p2LHand, p2.handRad, p2.handRad);
		} else {
			p2LHand.separateROI (p2RHand, p2.handRad, p2.handRad);
		}

		// Separate the two players
		if (p1.ylHand - p1.handRad >= frame.rows/2) {
			p2LHand.separatePlayers(frame, p1LHand, p1.handRad, p2.handRad, false);
			p2RHand.separatePlayers(frame, p1LHand, p1.handRad, p2.handRad, false);
			p2LHand.separatePlayers(frame, p1RHand, p1.handRad, p2.handRad, false);
			p2RHand.separatePlayers(frame, p1RHand, p1.handRad, p2.handRad, false);
		} else {
			p1LHand.separatePlayers(frame, p2LHand, p1.handRad, p2.handRad, false);
			p1RHand.separatePlayers(frame, p2LHand, p1.handRad, p2.handRad, false);
			p1LHand.separatePlayers(frame, p2RHand, p1.handRad, p2.handRad, false);
			p1RHand.separatePlayers(frame, p2RHand, p1.handRad, p2.handRad, false);
		}
		p2LHand.separatePlayers(frame, p1, p1.headRad, p2.handRad, true);
		p2RHand.separatePlayers(frame, p1, p1.headRad, p2.handRad, true);
		p1LHand.separatePlayers(frame, p2, p2.headRad, p1.handRad, true);
		p1RHand.separatePlayers(frame, p2, p2.headRad, p1.handRad, true);

		// Update final position of ROI
		p1.updateROI(frame);
		p1LHand.updateROI(frame);
		p1RHand.updateROI(frame);

		p2.updateROI(frame2);
		p2LHand.updateROI(frame2);
		p2RHand.updateROI(frame2);

				// Visualise each ROI on frame
		p1.drawROI (frame);
		p1LHand.drawROI (frame);
		p1RHand.drawROI (frame);

		p2.drawROI (frame2);
		p2LHand.drawROI (frame2);
		p2RHand.drawROI (frame2);

		imshow("Player 1 ROI", frame);
		imshow("Player 2 ROI", frame2);

		if (not (p1.wBar == 0 || p2.wBar == 0)) {
					// Visualise players on game window
				// A black background to erase previous image

			rectangle (game, Rect(0,0, frame.cols, frame.rows), Scalar (0,0,0), -1);
			p1.drawPlayer (game, p1LHand, p1RHand);
			p1.stamina (game, p2LHand, p2RHand);
			p2.drawPlayer (game, p2LHand, p2RHand);
			p2.stamina (game, p1LHand, p1RHand);

			game.copyTo(game2);

			putText(game, player2Str, Point(400,70), FONT_HERSHEY_PLAIN, 3, Scalar(255,255,255), 2);
			putText(game, player1Str, Point(150,450), FONT_HERSHEY_PLAIN, 3, Scalar(255,255,255), 2);

			imshow("Boxing Game 1", game);

			flip (game2, game2, -1); // flip image on both axes
			putText(game2, player1Str, Point(400,70), FONT_HERSHEY_PLAIN, 3, Scalar(255,255,255), 2);
			putText(game2, player2Str, Point(150,450), FONT_HERSHEY_PLAIN, 3, Scalar(255,255,255), 2);
			imshow("Boxing Game 2", game2);
//			flip (game, game, -1);
		}

		if (p1.wBar == 0) {
			winner = player2Str;

			rectangle (game, Rect(0,0, frame.cols, frame.rows), Scalar (0,0,0), -1);
			putText(game, "You Win!", Point(80,300), FONT_HERSHEY_TRIPLEX, 3, Scalar(0,0,255), 2, 8);
			imshow("Boxing Game 2", game);

			rectangle (game, Rect(0,0, frame.cols, frame.rows), Scalar (0,0,0), -1);
			putText(game, "You Lose!", Point(70,250), FONT_HERSHEY_TRIPLEX, 3, Scalar(0,0,255), 2, 8);
			imshow("Boxing Game 1", game);
		}

		else if (p2.wBar == 0) {
			winner = player1Str;

			rectangle (game, Rect(0,0, frame.cols, frame.rows), Scalar (0,0,0), -1);
			putText(game, "You Win!", Point(80,300), FONT_HERSHEY_TRIPLEX, 3, Scalar(0,0,255), 2, 8);
			imshow("Boxing Game 1", game);

			rectangle (game, Rect(0,0, frame.cols, frame.rows), Scalar (0,0,0), -1);
			putText(game, "You Lose!", Point(70,250), FONT_HERSHEY_TRIPLEX, 3, Scalar(0,0,255), 2, 8);
			imshow("Boxing Game 2", game);
		}


		if (waitKey(20) >= 0) {
			break;
		}
	}

	char winnerChar[winner.size() + 1];
	strcpy(winnerChar, winner.c_str());

	history = fopen("winners.txt", "a");
	fprintf(history, "%s \n", winnerChar);
	fclose(history);

	printf("Final frameCount = %d \n", frameCount);
	return 0;
}
