/**
 * @file StoneDetector.cpp
 * @ingroup Go-CamRecorder
 * @author Dominique Vaufreydaz, personnal project
 * @copyright All right reserved.
 */

#include "StoneDetector.h"

/**
* @brief Initialisation of a stone detector
* @param p [in] Center point
* @param size [in]size of the area (will be refine after intiialisation)
* @param InitImage [in] Initialisation image (for croping)
*/
void StoneDetector::Init( const cv::Point&p, int size, cv::Mat& InitImage )
{
	Center = p;
	radius = size;

	State = Empty;
	Timestamp = 0.0;


	InMotion = false;
	InMotionExtended = false;

	Fixed = false;

	cv::Rect DetectionRect =  GetRect( InitImage, Center );

	// #define SQRT2_1 14142/10000

	MaskStone = cv::Mat( DetectionRect.height, DetectionRect.width, CV_8UC1, cv::Scalar( 0 ) );
	cv::ellipse( MaskStone, cv::Point( DetectionRect.width/2, DetectionRect.height/2 ), cv::Size( radius, radius2 ), 0.0, 0.0, 360.0, cv::Scalar( 255 ), -1 );

	NbPixelsInStone = cv::countNonZero( MaskStone );
}

/**
* @brief Set new detected state of the detector. If state is the same as the previous one, nothing is done.
* @param NewState [in] New detected color
* @param CurrentTimestamp [in] Frame timestamp
* @param True if state has changed, i.e. it is not the same state as the previous one.
*/
bool StoneDetector::SetState( int NewState, double CurrentTimestamp )
{
	// Same state???
	if ( State == NewState )
	{
		return false;
	}

	// Timestamp of this detection
	State = NewState;
	Timestamp = CurrentTimestamp;

	return true;
}

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
bool StoneDetector::ComputeOnSubDetector( cv::Mat& LocalDetector, int startcol, int endcol, int startline, int endline, double& score )
{
	cv::Point UpperLeft( endcol+1, endline+1 );
	cv::Point BottomRight( -1, -1 );
	bool found = false;
	for ( int line = startline; line < endline; line++ )
	{
		unsigned char * line_ptr = LocalDetector.ptr<unsigned char>( line );
		unsigned char * mask_ptr = MaskStone.ptr<unsigned char>( line );
		for ( int col = startcol; col < endcol; col++ )
		{
			if ( mask_ptr[col] == 0 )
			{
				continue;
			}

			// NbOfPoints++;

			if ( line_ptr[col] == 255 )
			{
				found = true;

				if ( line < UpperLeft.y )
				{
					UpperLeft.y = line;
				}
				if ( col < UpperLeft.x )
				{
					UpperLeft.x = col;
				}
				if ( line > BottomRight.y )
				{
					BottomRight.y = line;
				}
				if ( col > BottomRight.x )
				{
					BottomRight.x = col;
				}
			}
		}
	}

	if ( found == false )
	{
		return false;
	}

	// Recompute score of box as there are full but inside stone detection are
	int NbOfPoints = 0;
	for ( int line = UpperLeft.y; line <= BottomRight.y; line++ )
	{
#ifdef DEBUG
		unsigned char * line_ptr = LocalDetector.ptr<unsigned char>( line );
#endif
		unsigned char * mask_ptr = MaskStone.ptr<unsigned char>( line );
		for ( int col = UpperLeft.x; col <= BottomRight.x; col++ )
		{
			if ( mask_ptr[col] == 0 )
			{
				continue;
			}

#ifdef DEBUG
			line_ptr[col] = 255;
#endif
			NbOfPoints++;
		}
	}

	score += (double)NbOfPoints;
	return true;
}

/**
* @brief Compute detection on the full detector detector.
* @param LocalDetector [in] LocalDetector is the complete image for the detector
* @param UpperLeft [in] Upperleft point for detection
* @param BottomRight [in] Bottom right point of the detection area
* @param found [out] True if a ston is found.
* @param Detection score is returned.
*/
double StoneDetector::ComputeOverlappingAndScore( cv::Mat& LocalDetector, cv::Point& UpperLeft, cv::Point& BottomRight, bool& found )
{
	UpperLeft = cv::Point( LocalDetector.cols, LocalDetector.rows );
	BottomRight = cv::Point( -1, -1 );
	found = false;

	float SubDetectorsSizeInX = (float)LocalDetector.cols/(float)NbSubDetectorsOnEachAxis;
	float SubDetectorsSizeInY = (float)LocalDetector.rows/(float)NbSubDetectorsOnEachAxis;

	double score = 0.0;
	float startcol = 0.0f;
	for ( int i = 0; i < NbSubDetectorsOnEachAxis; i++ )
	{
		float startline = 0.0f;
		for ( int j = 0; j < NbSubDetectorsOnEachAxis; j++ )
		{
			found |= ComputeOnSubDetector( LocalDetector, (int)startcol, (int)(startcol+SubDetectorsSizeInX), (int)startline, (int)(startline+SubDetectorsSizeInY), score );
			startline += SubDetectorsSizeInY;
		}
		startcol += SubDetectorsSizeInX;
	}

	if ( found == false )
	{
		return 0.0;
	}

	score /= (double)NbPixelsInStone;

	return score;
}

/**
* @brief After color detection, IsDEtected will serach for a Black or White stone
* @param Image [in] Current image
* @param CurrentSearch [in] Stone color
* @param score [in] score of detection
* @return true if a stone is detected with the right column.
*/
bool StoneDetector::IsDetected( cv::Mat& Image, int CurrentSearch, double score )
{
	cv::Mat LocalDetector( Image, GetRect( Image, Center ) );

	// Bounding box outside the image
	cv::Point UpperLeft;
	cv::Point BottomRight;
	bool found = false;

	score = ComputeOverlappingAndScore( LocalDetector, UpperLeft, BottomRight, found );

	if ( found == false )	// no point is present
	{
		return false;
	}

	if ( score >= ResultsScoreMin )
	{
		return true;
	}

	return false;
}

/**
* @brief Do stone detection on this cell.
* @param Image [in] Current image
* @param WhiteImage [in] White detection image
* @param BlackImage [in] Black detection image
* @param CurrentTimestamp [in] Frame timestamp
* @param DepthMode [in] DepthMode: true if using Kinect.
* @return true if a stone is detected with the right color.
*/
bool StoneDetector::DoStoneDetection( cv::Mat& Image, cv::Mat& WhiteImage, cv::Mat& BlackImage, double CurrentTimestamp, bool DepthMode )
{
	// Process only empty cells when moving to detect stone earlier
	if ( InMotionExtended == true )
	{
		// In depth mode, enough, we know that is it not a stone
		if ( DepthMode == true )
		{
			return false;
		}

		// In camera mode, if we have already a state, do not do detection while motion
		if ( State != Empty )
		{
			return false;
		}
	}

	// here, try to do detection (even while motion)

	// Ok, we process it this time

	double score = 0.0;
	switch ( State )
	{
		case White:
		// Check if still white
		if ( IsDetected( WhiteImage, White, score ) == true )
		{
			return true;
		}
		// Very bad score for white, no more white
		if ( score <= 0.1 )
		{
			return SetState( Empty, CurrentTimestamp );
		}
		// Check later, still White
		return true;

		case Black:
		// Check if still white
		if ( IsDetected( BlackImage, Black, score ) == true )
		{
			return true;
		}
		// Very bad score for Black, no more black
		if ( score <= 0.1 )
		{
			return SetState( Empty, CurrentTimestamp );
		}
		// Check later, still Black for the moment...
		return true;
	}

	// Chekc black or White
	if ( IsDetected( WhiteImage, White, score ) == true )
	{
		// fprintf( stderr, "White %.3f\n", CurrentTimestamp );
		return SetState( White, CurrentTimestamp );
	}

	if ( IsDetected( BlackImage, Black, score ) == true )
	{
		// fprintf( stderr, "Black %.3f\n", CurrentTimestamp );
		return SetState( Black, CurrentTimestamp );
	}

	return SetState( Empty, CurrentTimestamp );
}


/**
* @brief Aggragate motion detection score.
* @param NewValue [in] New motion dection value
* @param CurrentTimestamp [in] Frame timestamp
* @param UsingDepth [in] DepthMode: true if using Kinect.
* @return true if a stone is detected with the right color.
*/
unsigned int StoneDetector::AndValueAndComputeMotion( unsigned int NewValue, double CurrentTimestamp, bool UsingDepth )
{
	unsigned int CurrentMotionValue = NewValue;

	if ( NewValue >= 255*PercentageThreshold/100 )
	{
		LastMotionEvent.HValue = 255;
		LastMotionEvent.Timestamp = CurrentTimestamp;
		return 255;
	}

	if ( (CurrentTimestamp-LastMotionEvent.Timestamp) <= OldValues )
	{
		return LastMotionEvent.HValue;
	}

	LastMotionEvent.HValue = 0;
	return 0;
}					


/**
* @brief Aggragate motion detection score and compute motion event.
* @param MotionImage [in] Motion image (defference from previous image or difference from background depth image).
* @param CurrentTimestamp [in] Frame timestamp
* @param UsingDepth [in] UsingDepth: true if using Kinect.
* @return true if the cell is currently in motion.
*/
bool StoneDetector::DoMotionDetection( cv::Mat& MotionImage, double CurrentTimestamp, bool UsingDepth /* = false */ )
{
	cv::Mat srcMotion( MotionImage, GetRect( MotionImage, Center ) );

	// Compute motion state
	unsigned int NewMotion = AndValueAndComputeMotion( (unsigned int)cv::mean( srcMotion )[0], CurrentTimestamp, UsingDepth );

	// Compute is this a motion
	bool NewInMotionEvent = (NewMotion >= (255*PercentageThreshold/100));


	// Do we currently move ?
	if ( NewInMotionEvent == true )
	{
		// In motion
		InMotion = true;

		return true;
	}

	InMotion = false;

	return false;
}

/**
* @brief Draw current detection and/or motion.
* @param WhereToDraw [in] Image to draw in
* @param draw_size [in] If set to positive value, limite the size of drawing
*/
void StoneDetector::Draw( cv::Mat& WhereToDraw, int draw_size /* = -1 */ )
{
	// return;

	cv::Scalar CurColor = CV_RGB( 0, 255, 0 );
	cv::Scalar OutsideColor = CV_RGB( 0, 255, 0 );

	int rad_draw;
	if ( draw_size > 0 )
	{
		rad_draw = draw_size;
	}
	else
	{
		rad_draw = Max( radius/2, 3 );
	}

	if ( State == White )
	{								
		CurColor = CV_RGB( 255, 255, 255 );
		OutsideColor = CV_RGB( 0, 0, 0 );
	}
	else if ( State == Black )
	{
		CurColor = CV_RGB( 0, 0, 0 );
		OutsideColor = CV_RGB( 0, 0, 255 );
	}

	if ( InMotionExtended == true )
	{
		cv::rectangle( WhereToDraw, GetRect( WhereToDraw, Center ), CV_RGB( 255, 0, 0 ), 2 );
	}

	if ( State == Empty )
	{
		return;
	}

	cv::circle( WhereToDraw, Center, rad_draw, OutsideColor, -1 );
	if ( rad_draw-3 > 0 )
	{
		cv::circle( WhereToDraw, Center, rad_draw-3, CurColor, -1 );
	}
}

