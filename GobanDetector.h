/**
 * @file GobanDetector.h
 * @ingroup Go-CamRecorder
 * @author Dominique Vaufreydaz, personnal project
 * @copyright All right reserved.
 */


#ifndef __GOBAN_DETECOR_H__
#define __GOBAN_DETECOR_H__

#include <System/ReentrantMutex.h>
#include <System/ElapsedTime.h>

#include "CalibrationContainer.h"
#include "GobanState.h"
#include "StoneDetector.h"
#include "MultiSourceVideo.h"

#define WhiteDetectionWindowName "White detection"
#define BlackDetectionWindowName "Black detection"
#define MotionDetectionWindowsName "Motion detection"

/**
* @brief Static function to handle mouse click
* @param NumCells [in] Size fo goban
* @param SizeOfCells [in] Size of goban cells
* @param _2DPointsInImage [out] Projected points all over the goban
* @param CurCalibration [in] Input calibration
*/
void ProjectGobanBoard(int NumCells, float SizeOfCells, CalibrationContainer& CurCalibration, std::vector<cv::Vec2f>& _2DPointsInImage );

/**
* @brief Static function to handle mouse click
* @param NumCells [in] Size fo goban
* @param ImageMask [in,out] ImageMask from the goban
* @param SizeOfCells [in] Size of goban cells
* @param CurCalibration [in] Input calibration
*/
std::vector<cv::Point> DoGobanMask(int NumCells, float SizeOfCells, CalibrationContainer& CurCalibration, cv::Mat& ImageMask );

/**
 * @class GobanDetector 
 * @brief Class to hold and manage all stone detectors on the goban
 */
class GobanDetector
{
protected:
	friend class GobanState;									// Need to acces to internal data, thus GobanState is a friend class

// public:
	// Work in millimeter
	const float DefaultSizeOfCells = 22.5f;						// Default size of the cells. In this program, wells are square
	const float DefaultSizeOfStone = 22; 

	int NumCells;												// Number of Cells to work on, i.e. size of goban

	CalibrationContainer GobanViewCalibration;					// Container for camera of the Goban view

	float SizeOfCells = DefaultSizeOfCells;						// Current size of cells
	StoneDetector AllDetectors[MaxNumCells][MaxNumCells];		// Max supported size if 19x19

	// To crop image on goban and mask moves outside it
	cv::Rect SubImageRect;										// Goban rect within the image
	cv::Mat  FullImageMask;										// Processing mask on the full image
	cv::Mat  SubImageMask;										// Processing mask on the croped image

public:

	/**
	 * @class ClickPoint 
	 * @brief Simple class to allow mutexed access to a cv::Point from a click within an opencv windows
	 */
	class ClickPoint : public Omiscid::ReentrantMutex, public cv::Point
	{
		public:
			/**
			* @brief Constructor
			*/
			ClickPoint() 
			{
				Reset();
			}

			/**
			* @brief Virtual destructor
			*/
			virtual ~ClickPoint()
			{}

			/**
			* @brief reset internal values x and y to -1
			*/
			void Reset()
			{
				x = y = -1;
			}
	};

	GobanState GameState;					// Current game state

	/**
    * @brief Constructor
    * @param _NumCells [in] Size of the goban
	*/
	GobanDetector(int _NumCells) : NumCells(_NumCells), GobanViewCalibration(_NumCells, DefaultSizeOfCells), GameState(_NumCells)
	{
		if ( _NumCells <= 0 || _NumCells > 19 )
		{
			Omiscid::SimpleException Ex("Unsupported goban size (" + Omiscid::SimpleString(_NumCells) + ")" );
			Ex.msg += _NumCells;
			fprintf( stderr, "%s\n", Ex.msg.GetStr() );
			throw Ex;
		}
	}

	/**
    * @brief Vitural destructor
	*/
	virtual ~GobanDetector()
	{
	}

	/**
    * @brief Get rect of subimage
    * @return cv::Rect& to select data to process
	*/
	inline const cv::Rect& GetSubImageRect()
	{
		return (const cv::Rect&)SubImageRect;
	}

	/**
    * @brief Static function to handle mouse click
    * @param event [in] standard data from opencv, button event, ...
	* @param x [in] x pos of the current event
	* @param y [in] y pos of the current event
	* @param Param [in] Pointer to a ClickPoint object to fill
	*/
	static void onMouse( int event, int x, int y, int, void* Param )
	{
		if( event != cv::EVENT_LBUTTONUP )
			return;

		// Get referebnce to list of points
		// Omiscid::MutexedSimpleList<cv::Point>& ListOfPoints = *((Omiscid::MutexedSimpleList<cv::Point>*)Param);
		ClickPoint& CurPoint = *((ClickPoint*)Param);

		// Lock and fill it
		Omiscid::SmartLocker SL_CurPoint(CurPoint);
		CurPoint.x = x;
		CurPoint.y = y;
	}

	/**
    * @brief Draw result of all stone detectors in the Img opencv image
    * @param Img [in, out] Img
	*/
	void DrawAll(cv::Mat& Img);

	/**
    * @brief Extend motion detection to border of the Goban. Use convex hull to fill moving objects.
    * @param CurrentTimestamp [in] Current timestamp of the frame
	*/
	void ComputeExtendedMotion(double CurrentTimestamp);

	/**
	* @brief Compute and retrieve calibration
	* @param InputName [in] Current input name
	* @param CurSource [in] Source of images (video or device)
	* @param RestartCalibration [in] Flag to indicateif we call the function recursively.
	* @return True if calibration went fine.
	*/
	bool GetCalibration(Omiscid::SimpleString& InputName, MultiVideoSource& CurSource, bool RestartCalibration = false );

	cv::Mat * HistoryFrames[2];		// History frame to compute motion detection

	/**
    * @brief Init detection process (motion detection, stone detectors)
    * @param InputImage [in] initialisation image
	*/
	void InitDetection(cv::Mat& InputImage);

	// do not recreate them all the time
	cv::Mat WhiteDetection,			// Detection of White area to detect white stones
			BlackDetection,			// Detection of black area to detect black stones
			Motion;					// Motion detection

	/**
    * @brief Process current frame. Do motion detection and stone detection
    * @param LoadImage [in,out] Image from the current video source
	* @param InputImage [in] Image from the current depth source (Kinect) if any
	* @param CurrentTimestamp [in] Timestamp of the frame
	* @param BlackThreshold [in] Threasold to detect black areas. This threshold varies using main windows trackbar.
	* @param WhiteThreshold [in] Threasold to detect white areas. This threshold varies using main windows trackbar.
	* @param DrawResult [in] Shall we draw detection result in LoadImage?
	* @param DrawResult [in] Shall we show Black/White detection result? (expensive)
	* @param ShowMotion [in] Shall we show motion detection result? (expensive)
	* @param InputImage [in] initialisation image
	* @return Processing time for this frame.
	*/
	double ProcessCurrentFrame( cv::Mat& LoadImage, cv::Mat& DepthImage, double CurrentTimestamp, int BlackThreshold, int WhiteThreshold,
								bool DrawResult = false, bool ShowBWDetection = false, bool ShowMotion = false );

	/**
    * @brief To retrive if there is motion over the goban
	* @return True is motion is ongoing.
	*/
	bool IsChanging();
	
};

#endif // __GOBAN_DETECOR_H__

