/**
 * @file Go-CalibrationContainer.cpp
 * @ingroup Go-CamRecorder
 * @author Dominique Vaufreydaz, personnal project
 * @copyright All right reserved.
 */

#include "CalibrationContainer.h"

/**
* @brief Operator>> to read calibration in a file using cv::FileStorage
* @param fs [in] Opencv file storage
* @param CalibValue [int] Calibration to read
*/
void operator>>(const cv::FileStorage& fs, CalibrationContainer& CalibValue)
{
	fs["intrinsic"] >> CalibValue.intrinsic;
	fs["distCoeffs"] >> CalibValue.distCoeffs;
	fs["rvec"] >> CalibValue.rvec;
	fs["tvecs"] >> CalibValue.tvec;
	fs["points"] >> (std::vector<cv::Point>&)(CalibValue);
	fs["sizeofcells"] >> CalibValue.SizeOfCells;
}

/**
* @brief Operator<< to store calibration in a file using cv::FileStorage
* @param fs [in] Opencv file storage
* @param CalibValue [int] Calibration to store
*/
cv::FileStorage& operator<<(cv::FileStorage& fs, CalibrationContainer& CalibValue)
{
	fs << "intrinsic" << CalibValue.intrinsic;
	fs << "distCoeffs" << CalibValue.distCoeffs;
	fs << "rvec" << CalibValue.rvec;
	fs << "tvecs" << CalibValue.tvec;
	fs << "points" << (std::vector<cv::Point>&)(CalibValue);
	fs << "sizeofcells" << CalibValue.SizeOfCells;

	return fs;
}

/**
* @brief Move a calibration point within the image (using the keyboard)
* @param index [in] Index of the point
* @param deltax [in] Delta on x axis
* @param deltay [in] Delta on y axis
*/
void CalibrationContainer::MovePoint( size_t index, int deltax, int deltay )
{
	size_t NbElements = size();
	if ( index < NbElements )
	{
		// Get element
		cv::Point& Tab = this->operator[]( index );
		Tab.x += deltax;
		Tab.y += deltay;
	}
}

/**
* @brief Draw calibration point in the image. Current selected point is red.
* @param WhereToDraw [in,out] A opencv image to draw in
* @param Scale [in] Scaling factor
* @param SelectedPoint [in] Current selected point
*/
void CalibrationContainer::Draw( cv::Mat& WhereToDraw, double scale /*= 1.0*/, int SelectedPoint /* = -1 */)
{
	for ( int i = 0; i < size(); i++ )
	{
		// Get element
		cv::Point Tab = this->operator[]( i );
		Tab.x = (int)((double)(Tab.x)*scale);
		Tab.y = (int)((double)(Tab.y)*scale);
		cv::circle( WhereToDraw, Tab, (int)(2.0*scale), i == SelectedPoint ? CV_RGB( 255, 0, 0 ) : CV_RGB( 255, 255, 255 ), -1 );
	}
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
bool CalibrationContainer::ComputeCalibrationParameters( cv::Size& ImageSize )
{
	// Let the compiler optimize it as the implicite constructor initialisation is tricky
	float PosX[NumOfPointsToCalibrate] ={-((float)(NumCells/2)), -((float)(NumCells/2-1)), 0.0f, ((float)(NumCells/2-1)), ((float)(NumCells/2)), ((float)(NumCells/2)), ((float)(NumCells/2)), ((float)(NumCells/2)), ((float)(NumCells/2)), ((float)(NumCells/2-1)), 0.0f, -((float)(NumCells/2-1)), -((float)(NumCells/2)), -((float)(NumCells/2)), -((float)(NumCells/2)), -((float)(NumCells/2)), 0.0f};
	float PosY[NumOfPointsToCalibrate] ={((float)(NumCells/2)), ((float)(NumCells/2)), ((float)(NumCells/2)), ((float)(NumCells/2)), ((float)(NumCells/2)), ((float)(NumCells/2-1)), 0.0f, -((float)(NumCells/2-1)), -((float)(NumCells/2)), -((float)(NumCells/2)), -((float)(NumCells/2)), -((float)(NumCells/2)), -((float)(NumCells/2)), -((float)(NumCells/2-1)), 0.0f, ((float)(NumCells/2-1)), 0.0f};

	if ( size() != NumOfPointsToCalibrate )
	{
		fprintf( stderr, "Not enough points to calibrate: need %ld, has %ld\n", NumOfPointsToCalibrate, size() );
		return false;
	}

	std::vector<cv::Vec3f> _3DPoints;
	std::vector<cv::Vec2f> _2DPoints;

	// Create 3D points and associated 2D points for camera calibration from grid
	// Create false 3D stones and project them to the image to compute parameters
	for ( int i = 0; i < size(); i++ )
	{
		// Get element
		cv::Point& Tab = this->operator[]( i );
		_3DPoints.push_back( cv::Vec3f( PosX[i]*SizeOfCells, PosY[i]*SizeOfCells, 0.0f ) );
		_2DPoints.push_back( cv::Vec2f( (float)Tab.x, (float)Tab.y ) );
	}

	// Vector of vectors, usual in opencv
	// Duplicate previous points vectors 10 times, better calibration?
	std::vector<std::vector<cv::Vec3f>> objectPoints;
	std::vector<std::vector<cv::Vec2f>> imagePoints;
	for ( int j = 0; j < 10; j++ )
	{
		objectPoints.push_back( _3DPoints );
		imagePoints.push_back( _2DPoints );
	}

	// Vecotr of vectors to get result
	std::vector<cv::Mat> rvecs;
	std::vector<cv::Mat> tvecs;

	// Empty intinsic paramters
	intrinsic = cv::Mat::zeros( 3, 3, CV_32FC1 );

	// Compute calibration
	double rms = cv::calibrateCamera( objectPoints, imagePoints, ImageSize, intrinsic, distCoeffs, rvecs, tvecs );

	// Coypy result to object member
	rvecs[0].copyTo( rvec );
	tvecs[0].copyTo( tvec );

	return true;
}

