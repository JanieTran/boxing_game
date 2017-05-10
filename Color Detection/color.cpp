#include <cv.h>   // all the OpenCV headers, which links to all the libraries
#include <highgui.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/videoio/videoio.hpp>

using namespace cv;   // a "shortcut" for directly using OpenCV functions

class MotionTracker {
private:  // private members are accessible/modifiable only inside this class
	// only the functions inside MotionTracker can access/modify the values
	// of these variables
	Mat previousPreviousFrame;   // the previous previous frame
	Mat previousFrame;    // the previous frame
	Mat currentFrame;

	int xROI;   // define the top-left corner of our
	int yROI;   //     region-of-interest
	int widthROI;
	int heightROI;
	bool firstTime;

public:
	MotionTracker() {   // our constructor;   an "initialization"
		xROI = 150;   // define the top-left corner of our
		yROI = 150;   //     region-of-interest
		widthROI = 100;
		heightROI = 100;
		firstTime = true;
	}

	void feedNewframe(Mat frame) {
		Mat diffPrevious;     // difference between the previous and current frame
		Mat grayDiffPrevious;
		Mat colorOutput;

		int x, y;
		int weight, sumWeight, sumWeightX, sumWeightY;
		int xCenter, yCenter;

		if (firstTime) {
			frame.copyTo(currentFrame);
			frame.copyTo(previousFrame);
			frame.copyTo(previousPreviousFrame);
			firstTime = false;
		}
		previousFrame.copyTo( previousPreviousFrame );
		currentFrame.copyTo(previousFrame);
		frame.copyTo(currentFrame);

		inRange(frame, Scalar(0,0,160), Scalar(80,80,255), colorOutput);

//		// get the diff between the current frame and the second frame before it
//		absdiff( previousPreviousFrame, frame, diffPrevious);
//		// convert the color differences into gray differences
//		// now, each pixel is in the range 0..255
//		cvtColor( diffPrevious, grayDiffPrevious, CV_BGR2GRAY);

		sumWeightX = sumWeightY = sumWeight = 0;  // set them all to zero
		for (y = yROI; y < yROI + heightROI; y++) { // visit pixels row-by-row
			// inside each row, visit pixels from left to right
			for (x = xROI; x < xROI + widthROI; x++) {
				// weight of the pixel  x,y
				weight = colorOutput.at<unsigned char>( y,x );
				sumWeight = sumWeight + weight;
				sumWeightX = sumWeightX + x * weight;
				sumWeightY += y * weight;
			}
		}
		if (sumWeight != 0) {
			xCenter = sumWeightX / sumWeight;
			yCenter = sumWeightY / sumWeight;
		}
		xROI = xCenter - widthROI / 2;   // make the ROI "follow" the center-of-mass
		yROI = yCenter - heightROI / 2;
		// let's keep the ROI inside the frame at all times
		if (xROI < 0) {   // the ROI is starting to go out on the left
			xROI = 0;  // keep it inside
		}
		if (yROI < 0) {  // the ROI is starting to go up out of the frame
			yROI = 0;   // keep it inside
		}
		if (xROI + widthROI >= frame.cols) {  // ROI starts to go out on the right
			xROI = frame.cols - widthROI; // keep it inside
		}
		if (yROI + heightROI >= frame.rows) { // ROI starts to go out at bottom
			yROI = frame.rows - heightROI; // keep it inside
		}

	}
	// setROI() is a mutator or "setter" function that is used
	//     to modify the values of private attributes
	// This function is like a "guardian" of private information
	void setROI(int new_xROI, int new_yROI, int new_widthROI, int new_heightROI) {
		xROI = new_xROI;
		yROI = new_yROI;
		widthROI = new_widthROI;
		heightROI = new_heightROI;
	}
	// These functions are called accessor or "getter" functions.
	// They are used to fetch the values of private attributes
	// Again, these functions are like "guardians" that allow
	//   outside code to read private information.
	int get_xCenter() {   return xROI + widthROI / 2;   }
	int get_yCenter() {   return yROI + heightROI / 2;   }
	int get_widthROI() {  return widthROI;  }
	int get_heightROI() {  return heightROI;  }

	void drawROI(Mat frame, Scalar color) {
		rectangle( frame, Rect(xROI,yROI,widthROI,heightROI), color, 2);
	}
};


int main(  int argc, char** argv ) {

	MotionTracker mTrack1;  // our MotionTracker object is mTrack1
	mTrack1.setROI( 10,20,150,150 );   // set mTrack1's region-of-interest

	Mat drawFrame;   // where we visualize

	int frameCount;   // counts the frames that are read from the camera
	Mat frame;   // Mat is a 2-D "matrix" of numbers, containing our image data
	VideoCapture cap(0);   // live camera
	if (!cap.isOpened()) {  // check if we succeeded in opening the camera
		return -1;  // quit the program if we did not succeed
	}

	namedWindow("Raw Image",CV_WINDOW_NORMAL);  // create a window
	namedWindow("Draw Frame", CV_WINDOW_NORMAL);

	for (frameCount = 0; frameCount < 100000000; frameCount++) {
		if (frameCount % 100 == 0) {  // every 100 frames, print a message
			printf("frameCount = %d \n", frameCount);
		}

		cap >> frame;  // from the first camera
		flip( frame,frame,1 );  // flip the frame horizontally
		imshow("Raw Image", frame);  // display the frame in the window
		mTrack1.feedNewframe(frame);

		frame.copyTo( drawFrame );  // create our "drawing" frame
		mTrack1.drawROI( drawFrame,Scalar(0,0,255));  // draw mTrack1's ROI

		imshow("Draw Frame", drawFrame);

		if (waitKey(20) >= 0) {   // wait 20ms and check if a key was pressed
			break;   // if a key was pressed, exit the for loop
		}
	}

	printf("Final frameCount = %d \n", frameCount);
	return 0;
}
