/**
 * @file GobanDetector.cpp
 * @ingroup Go-CamRecorder
 * @author Dominique Vaufreydaz, personnal project
 * @copyright All right reserved.
 */

#include "GobanDetector.h"


/**
* @brief Compute goban area deformation using the camera calibration
* @param NumCells [in] Goban size
* @param SizeOfCells [in] Theoretical size of cells
* @param _2DPointsInImage [out] Points projected in the image
* @param CurCalibration [out] Camera calibration
*/
void ProjectGobanBoard( int NumCells, float SizeOfCells, CalibrationContainer& CurCalibration, std::vector<cv::Vec2f>& _2DPointsInImage )
{
	std::vector<cv::Vec3f> _3DPoints;

	// Top left angle
	_3DPoints.push_back( cv::Vec3f( (-(NumCells/2))*SizeOfCells-SizeOfCells/2, (NumCells/2)*SizeOfCells+SizeOfCells/2, 0.0f ) );
	// Bottom left angle
	_3DPoints.push_back( cv::Vec3f( (-(NumCells/2))*SizeOfCells-SizeOfCells/2, (-NumCells/2)*SizeOfCells-SizeOfCells/2, 0.0f ) );
	// Bottom right angle
	_3DPoints.push_back( cv::Vec3f( ((NumCells/2))*SizeOfCells+SizeOfCells/2, (-NumCells/2)*SizeOfCells-SizeOfCells/2, 0.0f ) );
	// top right angle
	_3DPoints.push_back( cv::Vec3f( ((NumCells/2))*SizeOfCells+SizeOfCells/2, (NumCells/2)*SizeOfCells+SizeOfCells/2, 0.0f ) );

	CurCalibration.ProjectPoints( _3DPoints, _2DPointsInImage );
}

/**
* @brief Create a mask to exclude ourside part of thescene from processing
* @param NumCells [in] Goban size
* @param SizeOfCells [in] Theoretical size of cells
* @param CurCalibration [out] Camera calibration
* @param ImageMask [out] Binary mask to exclude outside area
* @return Contour points.
*/
std::vector<cv::Point> DoGobanMask( int NumCells, float SizeOfCells, CalibrationContainer& CurCalibration, cv::Mat& ImageMask )
{
	std::vector<cv::Point> PolygoneToFill;
	std::vector<cv::Vec2f> _2DPointsInImage;

	// Get projection og goban board
	ProjectGobanBoard( NumCells, SizeOfCells, CurCalibration, _2DPointsInImage );

	// cv::Point PolygoneAngle;

	// add all points of the contour to the vector
	// PolygoneAngle = cv::Point( (int)_2DPointsInImage[0][0], (int)_2DPointsInImage[0][1] );
	PolygoneToFill.push_back( cv::Point( (int)_2DPointsInImage[0][0], (int)_2DPointsInImage[0][1] ) );
	// PolygoneAngle = cv::Point( (int)_2DPointsInImage[1][0], (int)_2DPointsInImage[1][1] );
	PolygoneToFill.push_back( cv::Point( (int)_2DPointsInImage[1][0], (int)_2DPointsInImage[1][1] ) );
	// PolygoneAngle = cv::Point( (int)_2DPointsInImage[2][0], (int)_2DPointsInImage[2][1] );
	PolygoneToFill.push_back( cv::Point( (int)_2DPointsInImage[2][0], (int)_2DPointsInImage[2][1] ) );
	// PolygoneAngle = cv::Point( (int)_2DPointsInImage[3][0], (int)_2DPointsInImage[3][1] );
	PolygoneToFill.push_back( cv::Point( (int)_2DPointsInImage[3][0], (int)_2DPointsInImage[3][1] ) );

	// Add polygon to a opolygon container and fill it
	std::vector<std::vector<cv::Point> > PolygonContainer;
	PolygonContainer.push_back( PolygoneToFill );

	ImageMask = cv::Scalar( 0 );
	cv::fillPoly( ImageMask, PolygonContainer, cv::Scalar( 255 ) );

	return PolygoneToFill;
}

/**
* @brief Draw result of all stone detectors in the Img opencv image
* @param Img [in, out] Img
*/
void GobanDetector::DrawAll( cv::Mat& Img )
{
	for ( int a = 0; a < NumCells; a++ )
	{
		for ( int b = 0; b < NumCells; b++ )
		{
			AllDetectors[a][b].Draw( Img );
		}
	}
}

/**
* @brief Extend motion detection to border of the Goban. Use convex hull to fill moving objects.
* @param CurrentTimestamp [in] Current timestamp of the frame
*/
void GobanDetector::ComputeExtendedMotion( double CurrentTimestamp )
{
#ifdef GO_CAM_KINECT_VERSION

	// First copy "simple" motion detection
	for ( int a = 0; a < NumCells; a++ )
	{
		for ( int b = 0; b < NumCells; b++ )
		{
			if ( AllDetectors[a][b].InMotion == true )
			{
				AllDetectors[a][b].InMotionExtended = true;
				if ( a != 0 ) { AllDetectors[a-1][b].InMotionExtended = true; }
				if ( b != 0 ) { AllDetectors[a][b-1].InMotionExtended = true; }
				if ( a != NumCells-1 ) { AllDetectors[a+1][b].InMotionExtended = true; }
				if ( b != NumCells-1 ) { AllDetectors[a][b+1].InMotionExtended = true; }
			}
			else
			{
				AllDetectors[a][b].InMotionExtended = false;
			}
		}
	}

#else
	// First copy "simple" motion detection
	for ( int a = 0; a < NumCells; a++ )
	{
		for ( int b = 0; b < NumCells; b++ )
		{
			AllDetectors[a][b].InMotionExtended = AllDetectors[a][b].InMotion;
		}
	}
#endif

	// extend points to border of the goban
	for ( int a = 1; a < NumCells-1; a++ )
	{
		if ( AllDetectors[a][2].InMotionExtended == true )
		{
			AllDetectors[a][1].InMotionExtended = true;
			AllDetectors[a][0].InMotionExtended = true;
		}
	}

	for ( int a = 1; a < NumCells-1; a++ )
	{
		if ( AllDetectors[a][NumCells-3].InMotionExtended == true )
		{
			AllDetectors[a][NumCells-2].InMotionExtended = true;
			AllDetectors[a][NumCells-1].InMotionExtended = true;
		}
	}

	// extend points to border of the goban
	for ( int b = 1; b < NumCells-1; b++ )
	{
		if ( AllDetectors[2][b].InMotionExtended == true )
		{
			AllDetectors[1][b].InMotionExtended = true;
			AllDetectors[0][b].InMotionExtended = true;
		}
	}

	// extend points to border of the goban
	for ( int b = 1; b < NumCells-1; b++ )
	{
		if ( AllDetectors[NumCells-3][b].InMotionExtended == true )
		{
			AllDetectors[NumCells-2][b].InMotionExtended = true;
			AllDetectors[NumCells-1][b].InMotionExtended = true;
		}
	}

	// Fill holes using convex hull algorithm

	// ok, from extended motion, now, let compute conveh hull
	Mat threshold_output( NumCells+4, NumCells+4, CV_8UC1, cv::Scalar( 0 ) );

	for ( int a = 0; a < NumCells; a++ )
	{
		for ( int b = 0; b < NumCells; b++ )
		{
			if ( AllDetectors[a][b].InMotionExtended == true )
			{
				threshold_output.at<unsigned char>( a+2, b+2 ) = 255;
			}
		}
	}

	using namespace std;

	/// Find contours
	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	findContours( threshold_output, contours, hierarchy, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE, Point( 0, 0 ) );

	/// Find the convex hull object for each contour
	vector<vector<Point> >hull( contours.size() );
	for ( int i = 0; i < contours.size(); i++ )
	{
		convexHull( Mat( contours[i] ), hull[i], false );
	}

	/// Draw contours + hull results
	Mat drawing = Mat::zeros( threshold_output.size(), CV_8UC1 );
	for ( int i = 0; i< contours.size(); i++ )
	{
		Scalar color = Scalar( 255, 255, 255 );
		drawContours( threshold_output, hull, i, color, -1 );
	}

	// #define DRAW_EXTENDED_MOTION
#ifdef DRAW_EXTENDED_MOTION
	bool modif = false;

	cv::Mat ExMotionResult( NumCells*10, NumCells * 10, CV_8UC1, cv::Scalar( 0 ) );
#endif

	// Result back to moving objects
	for ( int a = 0; a < NumCells; a++ )
	{
		for ( int b = 0; b < NumCells; b++ )
		{
			if ( threshold_output.at<unsigned char>( a+2, b+2 ) == 255 && AllDetectors[a][b].InMotionExtended == false )
			{
				AllDetectors[a][b].InMotionExtended = true;
				AllDetectors[a][b].LastMotionEvent.HValue = 255;
				// AllDetectors[a][b].LastMotionEvent.Timestamp = CurrentTimestamp;
#ifdef DRAW_EXTENDED_MOTION
				modif = true;
				cv::rectangle( ExMotionResult, cv::Rect( a*10, b*10, 10, 10 ), cv::Scalar( 255 ), -1 );
#endif
			}
		}
	}

#ifdef DRAW_EXTENDED_MOTION

	if ( modif == true )
	{
		// DrawAll( ModifImg );
		cv::imshow( "Extended motion", ExMotionResult );
		// paused = true;
	}
#endif

	for ( int a = 0; a < NumCells; a++ )
	{
		for ( int b = 0; b < NumCells; b++ )
		{
			if ( AllDetectors[a][b].InMotionExtended == true )
			{
				// Reset counter
				AllDetectors[a][b].MotionCount = 10;
			}
			else
			{
				if ( AllDetectors[a][b].MotionCount > 0 )
				{
					AllDetectors[a][b].MotionCount--;
				}
				if ( AllDetectors[a][b].MotionCount > 0 )
				{
					AllDetectors[a][b].InMotionExtended = true;
				}
			}
		}
	}

}

/**
* @brief Compute and retrieve calibration
* @param InputName [in] Current input name
* @param CurSource [in] Source of images (video or device)
* @param RestartCalibration [in] Flag to indicateif we call the function recursively.
* @return True if calibration went fine.
*/
bool GobanDetector::GetCalibration( Omiscid::SimpleString& InputName, MultiVideoSource& CurSource, bool RestartCalibration /* = false*/ )
{
	// Get local copye of image
	cv::Mat FirstImage;
	cv::Mat CurImage, CurDepth;			// To get data from Source, Depth won't be used

	if ( CurSource.ReadFrame( CurImage, CurDepth ) == false )
	{
		fprintf( stderr, "Could not read calibration frame, abording..." );
		return false;
	}

	double scale = 1.0;
	cv::resize( CurImage, FirstImage, cv::Size( 0, 0 ), (double)scale, (double)scale );

	Omiscid::SimpleString CalibFileName = InputName + ".calib";

	bool LoadedCalibration = true;
	cv::FileStorage fs;
	if ( RestartCalibration == false && fs.open( CalibFileName, cv::FileStorage::READ ) == true )
	{
		fs >> GobanViewCalibration;
		fs.release();
	}
	else
	{
		// New calibration, if validated, need to be saved
		LoadedCalibration = false;

		// Last click point
		ClickPoint CurPoint;
		int SelPoint = -1;		// Selected point

		for ( ;; )
		{
			// Here we need to redefine the mouse callback. Indeed, if the user close the window,
			// it will reopen but without the mouse callback
#define CalibrationWindowName "Place goban in the image. Set 17 points for calibration."

			// Create window
			cv::namedWindow( CalibrationWindowName );
			// Add mouse callback
			cv::setMouseCallback( CalibrationWindowName, onMouse, (void*)&CurPoint );

			cv::imshow( CalibrationWindowName, FirstImage );
			int KeyPressed = cv::waitKey( 10 );

			// Lock point with a smart locker
			Omiscid::SmartLocker SL_CurPoint( CurPoint );

			bool DoCalibration = false;

			// First as char
			switch ( (char)KeyPressed )
			{
				case ' ':
				{
					// Remove all calibration points
					GobanViewCalibration.clear();
					continue;
				}

				case 's':
				{
					// Remove last point
					GobanViewCalibration.pop_back();
					continue;
				}

				case '+':
				{
					// Increase scale until 3.0
					if ( scale < 3 )
					{
						scale += 0.25;
					}

					cv::resize( CurImage, FirstImage, cv::Size( 0, 0 ), (double)scale, (double)scale );
					continue;
				}

				case '-':
				{
					// Decrease scale
					if ( scale > 1 )
					{
						scale -= 0.25;
					}

					cv::resize( CurImage, FirstImage, cv::Size( 0, 0 ), (double)scale, (double)scale );
					continue;
				}

				case 27:
				// Exit !
				return false;
			}

			// As int for arrow
			switch ( KeyPressed )
			{
				case 2490368:	// up key, on windows
				case 1113938:	// on linux
				{
					GobanViewCalibration.MovePoint( SelPoint, 0, -1 );
					break;
				}

				case 2621440:	// down key on Windows
				case 1113940:	// On linux
				{
					GobanViewCalibration.MovePoint( SelPoint, 0, +1 );
					break;
				}

				case 2424832:  // left key, on windows
				case 1113937:	// on linux
				{
					GobanViewCalibration.MovePoint( SelPoint, -1, 0 );
					break;
				}

				case 2555904:	// right key, on windows
				case 1113939:	// on linux
				{
					GobanViewCalibration.MovePoint( SelPoint, +1, 0 );
					break;
				}

				case -1:
				// no key has been pressed 
				break;

				default:
				// To check specific code of key 
				// fprintf( stderr, "%d\n", KeyPressed );
				break;
			}

			DoCalibration = ( (char)KeyPressed == 'c' ||  (char)KeyPressed == 'C');
			if ( DoCalibration == true )
			{
				// Ask for calibration, ok go out of the loop
				break;
			}

			if ( CurPoint.x != -1 && CurPoint.y != -1 )
			{
				// Check if click is near a point
				bool ClickOnPoint = false;
				cv::Point CurPointRescaled = CurPoint;
				CurPointRescaled.x = (int)(double( CurPointRescaled.x )/scale);
				CurPointRescaled.y = (int)(double( CurPointRescaled.y )/scale);
				for ( int k = 0; k < GobanViewCalibration.size(); k++ )
				{
					cv::Point& p = GobanViewCalibration[k];
					int dist_square = (p.x - CurPointRescaled.x)*(p.x - CurPointRescaled.x) + (p.y - CurPointRescaled.y)*(p.y - CurPointRescaled.y);

					if ( dist_square < 100 )	// < 10 pixels
					{
						SelPoint = k;
						ClickOnPoint = true;
						break;
					}
				}

				// Add last point if mandatory
				if ( ClickOnPoint == false && GobanViewCalibration.HasEnoughPoints() == false )
				{
#ifdef AUTOMATIC_POINT_PLACEMENT
					// Attempt to help calibration process by selecting a good point to track,
					// hopefully the cross on the goban... Not so helpful on some test...

					// Add new point
					int init_x = CurPointRescaled.x-5;
					int init_y = CurPointRescaled.y-5;
					cv::Mat SubImage( CurImage, cv::Rect( init_x, init_y, 10, 10 ) );
					cv::Mat gray;
					cv::cvtColor( SubImage, gray, cv::COLOR_RGB2GRAY );

					Mat dst;
					dst = Mat::zeros( gray.size(), CV_32FC1 );

					// Trying to find the nearest point to track
					std::vector< cv::Point2f > corners;
					cv::goodFeaturesToTrack( gray, corners, 1, 0.01, 20. );

					// Project it back
					cv::Point NewPoint;
					if ( corners.size() != 0 )
					{
						NewPoint.x = (int)(corners[0].x) + init_x;
						NewPoint.y = (int)(corners[0].y) + init_y;
					}
					else
					{
						NewPoint = CurPointRescaled;
					}

					// Add this point
					GobanViewCalibration.push_back( NewPoint );
#else
					GobanViewCalibration.push_back( CurPointRescaled );
#endif
					SelPoint = (int)GobanViewCalibration.size()-1;
				}

				CurPoint.Reset();
			}

			// If in live mode, get current frame
			if ( CurSource.IsInLiveMode() == true )
			{
				if ( CurSource.ReadFrame( CurImage, CurDepth ) == false )
				{
					fprintf( stderr, "Could not read calibration frame, abording..." );
					return false;
				}
			}

			// Draw points
			cv::resize( CurImage, FirstImage, cv::Size( 0, 0 ), (double)scale, (double)scale );

			// Draw points
			GobanViewCalibration.Draw( FirstImage, scale, SelPoint );

			// Print calibration command on screen is we have enough points
			if ( GobanViewCalibration.HasEnoughPoints() )
			{
				cv::putText( FirstImage, "Press c to compute calibration", cv::Point( 5, 30 ), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar( 255, 255, 255 ) );
			}
		}

		// Compute calibration
		cv::Size CurImageSize = CurImage.size();
		GobanViewCalibration.ComputeCalibrationParameters( CurImageSize );

		cv::destroyWindow( CalibrationWindowName );
	}

	// Make calibration check by user, clear vectors if we used them for calibration
	std::vector<cv::Vec3f> _3DPoints;
	std::vector<cv::Vec2f> _2DPoints;

	// Compute min/max boudary
	ProjectGobanBoard( NumCells, SizeOfCells, GobanViewCalibration, _2DPoints );

	// Compute subimage rect
	SubImageRect = cv::boundingRect( _2DPoints );

	// To be usable for mp4 exporting, wize must be multiple of 2.
	if ( SubImageRect.height % 2 == 1 )
	{
		SubImageRect.height -= 1;
	}

	if ( SubImageRect.width % 2 == 1 )
	{
		SubImageRect.width -= 1;
	}

	int init_x = SubImageRect.x;
	int init_y = SubImageRect.y;

	_2DPoints.clear();

	// Percentage of considered stones
	const float PercentageSizeOfStones = 0.90f;

	// Construct position of all goban points in 3D
	for ( int a = -(NumCells/2); a <= (NumCells/2); a++ )
	{
		for ( int b = (NumCells/2); b >= -(NumCells/2); b-- )
		{
			// Compute center of Cell
			_3DPoints.push_back( cv::Vec3f( (a)*SizeOfCells, (b)*SizeOfCells, 0.0f ) );
			// Compute radius of detection area
			_3DPoints.push_back( cv::Vec3f( (a)*SizeOfCells+PercentageSizeOfStones*SizeOfCells/2.0f, (b)*SizeOfCells, 0.0f ) );
			_3DPoints.push_back( cv::Vec3f( (a)*SizeOfCells, (b)*SizeOfCells+PercentageSizeOfStones*SizeOfCells/2.0f, 0.0f ) );
		}
	}

	// Project them
	GobanViewCalibration.ProjectPoints( _3DPoints, _2DPoints );

	// Copie original image
	CurImage.copyTo( FirstImage );

	// Project detector back in new subframe
	int Curp = 0;
	for ( int a = 0; a < NumCells; a++ )
	{
		for ( int b = 0; b < NumCells; b++ )
		{
			cv::Point p( (int)_2DPoints[Curp][0], (int)_2DPoints[Curp][1] );
			Curp += 3;

			// Draw center position of calibration !
			cv::circle( FirstImage, p, 3, cv::Scalar( 0, 255, 0 ), -1 );
			// cv::ellipse( FirstImage, p, cv::Size(abs(borderx.x - p.x), abs(bordery.y - p.y)), 0.0, 0.0, 360.0,  cv::Scalar(0,255,0), -1 );
		}
	}

	// Full image gray scale mask
	FullImageMask = cv::Mat( FirstImage.size(), CV_8UC1 );
	std::vector<cv::Point> ContourPoints = DoGobanMask( NumCells, SizeOfCells, GobanViewCalibration, FullImageMask );

	// Create masked version
	cv::Mat Masked;
	FirstImage.copyTo( Masked );

	// Add 1st point as 5th points also
	ContourPoints.push_back( ContourPoints[0] );
	// DrawContour of 4 lines
	for ( int Point = 0; Point < 4; Point++ )
	{
		cv::line( Masked, ContourPoints[Point], ContourPoints[Point+1], CV_RGB( 255, 0, 0 ), 2 );
	}

#define CalibrationTitle "Calibration and working area. Type y to validate, n to abord."

	cv::imshow( CalibrationTitle, Masked );
	for ( ;; )
	{
		char KeyPressed = cv::waitKey( 0 );

		switch ( KeyPressed )
		{
			// Validated
			case 'y':
			case 'Y':
			{
				// Project detector back in new subframe
				int Curp = 0;
				for ( int a = 0; a < NumCells; a++ )
				{
					for ( int b = 0; b < NumCells; b++ )
					{
						cv::Point p( (int)_2DPoints[Curp][0], (int)_2DPoints[Curp][1] );
						Curp++;

						// Get border of the stone
						cv::Point borderx( (int)_2DPoints[Curp][0], (int)_2DPoints[Curp][1] );
						Curp++;

						cv::Point bordery( (int)_2DPoints[Curp][0], (int)_2DPoints[Curp][1] );
						Curp++;

						// Min 2 pixels, radius of the globing circle, rect area will be double
						// int radius = Max( Min( abs(borderx.x - p.x), abs(bordery.y - p.y) ), 2 );
						int radius = Max( abs( borderx.x - p.x ), 2 );

						AllDetectors[a][b].radius2 = Max( abs( bordery.y - p.y ), 2 );
						AllDetectors[a][b].Init( cv::Point( p.x-init_x, p.y-init_y ), radius, FirstImage );
					}
				}

				// Shall we save the calibration?
				if ( LoadedCalibration == false )
				{
					Omiscid::SimpleString CalibImage= InputName + ".calib.png";
					cv::imwrite( CalibImage, Masked );

					// Save calibration
					fs.open( CalibFileName, cv::FileStorage::WRITE );
					fs << GobanViewCalibration;
					fs.release();
				}

				cv::destroyWindow( CalibrationTitle );
				return true;
			}

			// not validated
			case 'n':
			case 'N':
			{
				fprintf( stderr, "Calibration not validated, back to calibration process..." );
				cv::destroyWindow( CalibrationTitle );
				/*
				// Finally, do not remove here, will be remove whan a new calibration will be validated
				if ( LoadedCalibration == true )
				{
				remove( CalibFileName.GetStr() );
				}*/

				// Recursive call for next round
				return GetCalibration( InputName, CurSource, true );
			}

			case 27:
			{
				fprintf( stderr, "Calibration abording, exiting..." );
				cv::destroyWindow( CalibrationTitle );
				return false;
			}
		}
	}

	return false;
}


/**
* @brief Init detection process (motion detection, stone detectors)
* @param InputImage [in] initialisation image
*/
void GobanDetector::InitDetection( cv::Mat& InputImage )
{
	SubImageMask = cv::Mat( FullImageMask, SubImageRect );
	cv::Mat CropedImage( InputImage, SubImageRect );

	// Create history frames
	HistoryFrames[0] = new cv::Mat( SubImageMask.size(), SubImageMask.type() );
	HistoryFrames[1] = new cv::Mat( SubImageMask.size(), SubImageMask.type() );
#ifdef GO_CAM_KINECT_VERSION
	CropedImage.copyTo( *HistoryFrames[0] );
#else
	cv::cvtColor( CropedImage, *HistoryFrames[0], cv::COLOR_BGR2GRAY );
	HistoryFrames[0]->copyTo( *HistoryFrames[1] );
#endif

}

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
double GobanDetector::ProcessCurrentFrame( cv::Mat& LoadImage, cv::Mat& DepthImage, double CurrentTimestamp, int BlackThreshold, int WhiteThreshold,
											bool DrawResult /* = false */ , bool ShowBWDetection /* = false */, bool ShowMotion /* = false */ )
{
	Omiscid::PerfElapsedTime ET;

	// crop image to necessary zone
	cv::Mat CurImage( LoadImage, SubImageRect );

	bool DepthMode = (DepthImage.empty() == false);

	if ( DepthMode == true )
	{
		if ( Motion.empty() == true )
		{
			// Create motion image once
			Motion = cv::Mat( CurImage.size(), CV_8UC1 );
		}

		cv::Mat( DepthImage, SubImageRect ).copyTo( *HistoryFrames[1] );

		// Remove ouside of the Goban
		// bitwise_and( *HistoryFrames[0], SubImageMask, *HistoryFrames[0] );
		for ( int line = 0; line < Motion.rows; line++ )
		{
			unsigned char * MotionLine = Motion.ptr<unsigned char>( line );
			unsigned short * GobanLine = HistoryFrames[0]->ptr<unsigned short>( line );
			unsigned short * CurrentViewLine = HistoryFrames[1]->ptr<unsigned short>( line );
			for ( int pixel = 0; pixel < Motion.cols; pixel++ )
			{
				unsigned short GobanDepth = GobanLine[pixel] >> 3;
				unsigned short CurrentViewDepth = CurrentViewLine[pixel] >> 3;
				if ( GobanDepth == 0 || CurrentViewDepth == 0 || GobanDepth >= 2047 || CurrentViewDepth >= 2047 )
				{
					// No depth data, no motion...
					MotionLine[pixel] = SingleConfig.MotionThreshold;
					continue;
				}

				if ( abs( GobanDepth-CurrentViewDepth ) > 15 )	// ~3cm
				{
					MotionLine[pixel] = 255;
				}
				else
				{
					MotionLine[pixel] = 0;
				}
			}
		}
	}
	else
	{
		// standard motion detection

		// Convert to gray and apply mask
		cv::cvtColor( CurImage, *HistoryFrames[0], cv::COLOR_BGR2GRAY );

		// Remove ouside of the Goban
		bitwise_and( *HistoryFrames[0], SubImageMask, *HistoryFrames[0] );

		// Do motion detection
		cv::absdiff( *HistoryFrames[1], *HistoryFrames[0], Motion );
		cv::threshold( Motion, Motion, 40, 255, cv::THRESH_BINARY );
	}

	if ( ShowMotion == true )
	{
		// Show motion if asked
		cv::imshow( MotionDetectionWindowsName, Motion );
	}

	// Compute motion for all Detectors
	for ( int a = 0; a < NumCells; a++ )
	{
		for ( int b = 0; b < NumCells; b++ )
		{
			AllDetectors[a][b].DoMotionDetection( Motion, CurrentTimestamp, DepthMode );
		}
	}

	// Extend motion to neighborhood
	ComputeExtendedMotion( CurrentTimestamp );

	// Compute White and Black masks
	int HighBlackValue = (CentralValueForBlackDetection - MaxThresholdForColorDetection/2) + BlackThreshold;
	int LowWhiteValue = (CentralValueForWhiteDetection + MaxThresholdForColorDetection/2) - WhiteThreshold;
	cv::inRange( CurImage, cv::Scalar( 0, 0, 0 ), cv::Scalar( HighBlackValue, HighBlackValue, HighBlackValue ), BlackDetection );
	cv::inRange( CurImage, cv::Scalar( LowWhiteValue, LowWhiteValue, LowWhiteValue ), cv::Scalar( 255, 255, 255 ), WhiteDetection );

	// Do actual detection of stones
	for ( int a = 0; a < NumCells; a++ )
	{
		for ( int b = 0; b < NumCells; b++ )
		{
			AllDetectors[a][b].DoStoneDetection( Motion, WhiteDetection, BlackDetection, CurrentTimestamp, DepthMode );
		}
	}

	// If not using Kinect, swap frame
	if ( DepthMode == false )
	{
		// Swap frames (pointers) to remove useless copy
		cv::Mat * pTmp = HistoryFrames[1];
		HistoryFrames[1] = HistoryFrames[0];
		HistoryFrames[0] = pTmp;
	}

	// Draw detection/motion if mandatory
	if ( DrawResult == true )
	{
		for ( int a = 0; a < NumCells; a++ )
		{
			for ( int b = 0; b < NumCells; b++ )
			{
				AllDetectors[a][b].Draw( CurImage );
			}
		}

		// Drawing empty ellipses are *very* expensive in Opencv... Detection should be used only for debugging
		if ( ShowBWDetection == true )
		{
			for ( int a = 0; a < NumCells; a++ )
			{
				for ( int b = 0; b < NumCells; b++ )
				{
					cv::ellipse( BlackDetection, AllDetectors[a][b].Center, cv::Size( AllDetectors[a][b].radius, AllDetectors[a][b].radius2 ), 0.0, 0.0, 360.0, cv::Scalar( 127 ), 1 );
					cv::ellipse( WhiteDetection, AllDetectors[a][b].Center, cv::Size( AllDetectors[a][b].radius, AllDetectors[a][b].radius2 ), 0.0, 0.0, 360.0, cv::Scalar( 127 ), 1 );
					cv::ellipse( CurImage, AllDetectors[a][b].Center, cv::Size( AllDetectors[a][b].radius, AllDetectors[a][b].radius2 ), 0.0, 0.0, 360.0, cv::Scalar( 127 ), 1 );
				}
			}

			cv::imshow( WhiteDetectionWindowName, WhiteDetection );
			cv::imshow( BlackDetectionWindowName, BlackDetection );
		}
	}

	if ( IsChanging() == false )
	{
		// Update game state
		GameState.UpdateCurrentState( AllDetectors, CurrentTimestamp );
	}

	// Processing time including drawing and updating
	double FrameProcessingTime = ET.GetInSeconds();

	// return processing time in seconds
	return FrameProcessingTime;
}

/**
* @brief To retrive if there is motion over the goban
* @return True is motion is ongoing.
*/
bool GobanDetector::IsChanging()
{
	// Compute motion
	for ( int a = 0; a < NumCells; a++ )
	{
		for ( int b = 0; b < NumCells; b++ )
		{
			// If motion cells have detected something
			if ( AllDetectors[a][b].InMotionExtended == true && AllDetectors[a][b].State != StoneDetector::Empty )
			{
				return true;
			}
		}
	}
	return false;
}