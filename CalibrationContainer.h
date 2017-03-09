/**
 * @file Go-CalibrationContainer.h
 * @ingroup Go-CamRecorder
 * @author Dominique Vaufreydaz, personnal project
 * @copyright All right reserved.
 */


#ifndef __CALIBRATION_CONTAINER_H__
#define __CALIBRATION_CONTAINER_H__

#include <opencv2/video/tracking.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/background_segm.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/imgproc/imgproc.hpp>

// 17 points 
#define NumOfPointsToCalibrate 17		// 17 points are used to calibrate the goban within the camera view

/**
 * @class CalibrationContainer 
 * @brief Class to compute and hold calibration
 */
class CalibrationContainer : public std::vector<cv::Point>
{
public:
	// Camera parameters
	cv::Mat intrinsic;			// intrinsic camera parameters
	cv::Mat distCoeffs;			// distorsion camera parameters
	cv::Mat rvec;				// rvec camera parameters
	cv::Mat tvec;				// tvec camera parameters

	// Save parameters to recompute calibration at other scale (i.e. another SizeOfCells) if mandatory
	float SizeOfCells = 1.0f;	// Fake size of cells
	int NumCells;				// Number of cells, i.e. goban size

	/**
    * @brief Constructor
    * @param _NumCells [in] Size of the goban
	* @param _SizeOfCells [in] Size of cells, considered as square even if it not actually the case
	*/
	CalibrationContainer(int _NumCells, float _SizeOfCells) : NumCells(_NumCells), SizeOfCells(_SizeOfCells)
	{
	}

	/**
    * @brief Vitural destructor
	*/
	virtual ~CalibrationContainer()
	{
	}

	/**
    * @brief Move a calibration point within the image (using the keyboard)
    * @param index [in] Index of the point
	* @param deltax [in] Delta on x axis
	* @param deltay [in] Delta on y axis
	*/
	void MovePoint(size_t index, int deltax, int deltay);

	/**
    * @brief Draw calibration point in the image. Current selected point is red.
    * @param WhereToDraw [in,out] A opencv image to draw in
	* @param Scale [in] Scaling factor
	* @param SelectedPoint [in] Current selected point
	*/
	void Draw( cv::Mat& WhereToDraw, double Scale = 1.0, int SelectedPoint = -1 );

	/**
    * @brief Do we have enough points to calibrate the camera?
	* @return True is the number of point is correct
	*/
	inline bool HasEnoughPoints()
	{
		return (size() == NumOfPointsToCalibrate);
	}

	/**
    * @brief Compute calibration using 17 points.
 	* 17 points calibration : good one, generate it from NumCells
	* 
	* On a theorical 7x7 goban, point to calibrate must plus place in this order:
	* 			  1   2  .   3  .   4  5 
	* 			 16   .  .   .  .   .  6 
	* 			  .   .  .   .  .   .  . 
	* 			 15   .  .  17  .   .  7 
	* 			  .   .  .   .  .   .  . 
	* 			 14   .  .   .  .   .  8 
	* 			 13  12  .  11  .  10  9
	* 
	* Same pattern apply to bigger goban. Point 1 is aa, point 2 is ab, and so one
	* @param ImageSize [in] Size of the image
	* @return True if calibration process worked.
	*/
	bool ComputeCalibrationParameters(cv::Size& ImageSize);

	/**
    * @brief Inline function to project 3D points to the image plane
	* @param _3DPoints [in] Input 3D points
	* @param _2DPointsInImage [out] Projected points
	*/
	inline void ProjectPoints( std::vector<cv::Vec3f>& _3DPoints, std::vector<cv::Vec2f>& _2DPointsInImage )
	{
		// Project them
		cv::projectPoints( _3DPoints, rvec, tvec, intrinsic, distCoeffs, _2DPointsInImage );
	}

};

/**
* @brief Operator>> to read calibration in a file using cv::FileStorage
* @param fs [in] Opencv file storage
* @param CalibValue [int] Calibration to read
*/
void operator>>(const cv::FileStorage& fs, CalibrationContainer& CalibValue);

/**
* @brief Operator<< to store calibration in a file using cv::FileStorage
* @param fs [in] Opencv file storage
* @param CalibValue [int] Calibration to store
*/
cv::FileStorage& operator<<(cv::FileStorage& fs, CalibrationContainer& CalibValue);

#endif // __CALIBRATION_CONTAINER_H__
