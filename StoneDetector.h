/**
 * @file StoneDetector.h
 * @ingroup Go-CamRecorder
 * @author Dominique Vaufreydaz, personnal project
 * @copyright All right reserved.
 */


#ifndef __STONE_DETECTOR_H__
#define __STONE_DETECTOR_H__

#include "Go-CamRecorder.h"

#include <System/SimpleList.h>

#include "HistoryValue.h"
#include "StoneState.h"

using namespace cv;

#include <algorithm>

/**
* @brief Utility function for max (not template as std::max is).
*/
inline int Max(int a, int b)
{
	if ( b > a )
	{
		return b;
	}
	return a;
}

/**
* @brief Utility function for min (not template as std::min is).
*/
inline int Min(int a, int b)
{
	if ( b < a )
	{
		return b;
	}
	return a;
}

/**
 * @class StoneDetector 
 * @brief Class for detection fo stones at each cells on the goban
 */
class StoneDetector : public StoneState
{
public:
	// Detector position and state
	cv::Point Center;								// Center of detection area
	int radius;										// Radius in x for the stone (ellipse)	
	int radius2;									// Radius in y for the stone (ellipse)	
	bool Fixed;										// Fixe detection, may be removed...
	cv::Mat MaskStone;								// Mask of the projected stone on the detection
	int NbPixelsInStone;							// Number of pixel within the stone detector
	double ResultsScoreMin = 0.33;					// Area of an overlapping ellipse on the middle cross
	int State;										// Current detected state
	double Timestamp;								// Detected event timestamp


// Utility functions

	/**
	* @brief Get croped Rect area for the stone detector
	* @param Image [in] Working image (to get size)
	* @param CurCenter [int] Center of the stone detector
	* @return Rect containing detection area
	*/
	inline cv::Rect GetRect(cv::Mat& Image, cv::Point& CurCenter)
	{
		int minx = Max(CurCenter.x-radius,0);
		int miny = Max(CurCenter.y-radius2,0);

		int radiusx = radius*2;
		if ( (minx+radiusx) >= Image.cols )
		{
			radiusx = Image.cols-minx-1;
		}
		
		int radiusy = radius2*2;
		if ( (miny+radiusy) >= Image.rows )
		{
			radiusy = Image.rows-miny-1;
		}

		return cv::Rect( minx, miny, radiusx, radiusy );
	}

	/**
	* @brief Initialisation of a stone detector
	* @param p [in] Center point
	* @param size [in]size of the area (will be refine after intiialisation)
	* @param InitImage [in] Initialisation image (for croping)
	*/
	void Init(const cv::Point&p, int size, cv::Mat& InitImage);

	/**
	* @brief Default constructor
	*/
	StoneDetector(): Center(0,0), radius(0), State(Empty), InMotion(false), Timestamp(0.0)
	{
	}

	/**
	* @brief Virtual desconstructor
	*/
	virtual ~StoneDetector() {};


// Stone detection
	/**
	* @brief Set new detected state of the detector. If state is the same as the previous one, nothing is done.
	* @param NewState [in] New detected color
	* @param CurrentTimestamp [in] Frame timestamp
	* @param True if state has changed, i.e. it is not the same state as the previous one.
	*/
	bool SetState(int NewState, double CurrentTimestamp);

	#define NbSubDetectorsOnEachAxis 3		// Number of sub detector, in x, and y. #define because we do not want to use static const int...

	/**
	* @brief Compute detection on a sub detector. Subdetector will be merge to tackle refection on stones.
	* @param LocalDetector [in] LocalDetector is the complete image for the detector
	* @param startcol [in] Start col for stone detection
	* @param endcol [in] ending col
	* @param startline [in] starting line
	* @param endline [in] ending line
	* @param score [out] Score of the detection. REmain unchanged if not detection was done.
	* @param True if detection occurs.
	*/
	bool ComputeOnSubDetector(cv::Mat& LocalDetector, int startcol, int endcol, int startline, int endline, double& score );

	/**
	* @brief Compute detection on the full detector detector.
	* @param LocalDetector [in] LocalDetector is the complete image for the detector
	* @param UpperLeft [in] Upperleft point for detection
	* @param BottomRight [in] Bottom right point of the detection area
	* @param found [out] True if a ston is found.
	* @param Detection score is returned.
	*/
	double ComputeOverlappingAndScore( cv::Mat& LocalDetector, cv::Point& UpperLeft, cv::Point& BottomRight, bool& found);

	/**
	* @brief After color detection, IsDEtected will serach for a Black or White stone
	* @param Image [in] Current image
	* @param CurrentSearch [in] Stone color
	* @param score [in] score of detection
	* @return true if a stone is detected with the right column.
	*/
	bool IsDetected(cv::Mat& Image, int CurrentSearch, double score );

	/**
	* @brief Do stone detection on this cell.
	* @param Image [in] Current image
	* @param WhiteImage [in] White detection image
	* @param BlackImage [in] Black detection image
	* @param CurrentTimestamp [in] Frame timestamp
	* @param DepthMode [in] DepthMode: true if using Kinect.
	* @return true if a stone is detected with the right color.
	*/
	bool DoStoneDetection( cv::Mat& Image, cv::Mat& WhiteImage, cv::Mat& BlackImage, double CurrentTimestamp, bool DepthMode );

// Motion detection
	bool InMotion;									// Boolean set by premiary motion detection
	bool InMotionExtended;							// Boolean set by extended motion detection
	int MotionCount = 0;							// Third integration of motion, number of succesive frames
	const unsigned int PercentageThreshold = 5;		// 5% is a threshold for motion in color/depth data
	HistoryValue<unsigned int> LastMotionEvent;		// Last motion event on the detector
	const double OldValues = 0.500;					// Integration time, 1/2 seconde

	/**
	* @brief Aggragate motion detection score.
	* @param NewValue [in] New motion dection value
	* @param CurrentTimestamp [in] Frame timestamp
	* @param UsingDepth [in] DepthMode: true if using Kinect.
	* @return true if a stone is detected with the right color.
	*/
	unsigned int AndValueAndComputeMotion(unsigned int NewValue, double CurrentTimestamp, bool UsingDepth );

	/**
	* @brief Aggragate motion detection score and compute motion event.
	* @param MotionImage [in] Motion image (defference from previous image or difference from background depth image).
	* @param CurrentTimestamp [in] Frame timestamp
	* @param UsingDepth [in] UsingDepth: true if using Kinect.
	* @return true if the cell is currently in motion.
	*/
	bool DoMotionDetection( cv::Mat& MotionImage, double CurrentTimestamp, bool UsingDepth = false );


// Drawing function
	/**
	* @brief Draw current detection and/or motion.
	* @param WhereToDraw [in] Image to draw in
	* @param draw_size [in] If set to positive value, limite the size of drawing
	*/
	void Draw(cv::Mat& WhereToDraw, int draw_size = -1);
};


#endif // __STONE_DETECTOR_H__

