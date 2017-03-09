/**
 * @file GobanState.cpp
 * @ingroup Go-CamRecorder
 * @author Dominique Vaufreydaz, personnal project
 * @copyright All right reserved.
 */

#include "GobanState.h"


// Validdate capture, i.e do not find any liberty
void GobanState::CheckLibertiesRecursive( int a, int b, int StoneColor, Omiscid::SimpleList<cv::Point>& CurrentChain )
{
	if ( Goban[a][b].CaptureChecked == true )
	{
		// Already check, current validation is still questionned, go ahead
		return;
	}

	// Check if Current is Empty
	if ( Goban[a][b].State == Empty )
	{
		// Found liberty, go out!
		throw Omiscid::SimpleException( "Liberty found" );
	}

	// if Stone is from another color, no liberty
	if ( Goban[a][b].State != StoneColor )
	{
		return;
	}

	// Ok, we have check this stone, add it to the current chain
	Goban[a][b].CaptureChecked = true;
	CurrentChain.AddTail( cv::Point( a, b ) );

	// Check points in a cross manner, TO CHECK : do we need to check diagonals?
	if ( a > 0 )
	{
		CheckLibertiesRecursive( a-1, b, StoneColor, CurrentChain );
	}
	if ( a < (NumCells-1) )
	{
		CheckLibertiesRecursive( a+1, b, StoneColor, CurrentChain );
	}
	if ( b > 0 )
	{
		CheckLibertiesRecursive( a, b-1, StoneColor, CurrentChain );
	}
	if ( b < (NumCells-1) )
	{
		CheckLibertiesRecursive( a, b+1, StoneColor, CurrentChain );
	}
}


bool GobanState::CheckLiberties( int a, int b, int StoneColor, StoneDetector( &AllDetectors )[MaxNumCells][MaxNumCells], double CurrentTimestamp )
{
	if ( Goban[a][b].State != StoneColor )
	{
		// Not a stone in the opposite color, do not check
		return false;
	}

	Omiscid::SimpleList<cv::Point> CurrentChain;

	try
	{
		CheckLibertiesRecursive( a, b, StoneColor, CurrentChain );

		// Here, capture is validated (not exception thrown), remove stone
		for ( CurrentChain.First(); CurrentChain.NotAtEnd(); CurrentChain.Next() )
		{
			cv::Point& p = CurrentChain.GetCurrent();
			Goban[p.x][p.y].State		   = Empty;
			Goban[p.x][p.y].CaptureChecked = false;

			AllDetectors[p.x][p.y].State     = Empty;
			AllDetectors[p.x][p.y].Timestamp = CurrentTimestamp;
		}

		return true;
	}
	catch ( Omiscid::SimpleException& /* Exception */ )
	{
		// Here Liberty found, remove CaptureChecked flag
		for ( CurrentChain.First(); CurrentChain.NotAtEnd(); CurrentChain.Next() )
		{
			cv::Point& p = CurrentChain.GetCurrent();
			Goban[p.x][p.y].CaptureChecked = false;
		}

		// liberty found
		return false;
	}
}

bool GobanState::ValidateCapture( int a, int b, int StoneColor, StoneDetector( &AllDetectors )[MaxNumCells][MaxNumCells], double CurrentTimestamp )
{
	// Check points in a crossed manner
	if ( a > 0 )
	{
		CheckLiberties( a-1, b, StoneColor, AllDetectors, CurrentTimestamp );
	}
	if ( a < (NumCells-1) )
	{
		CheckLiberties( a+1, b, StoneColor, AllDetectors, CurrentTimestamp );
	}
	if ( b > 0 )
	{
		CheckLiberties( a, b-1, StoneColor, AllDetectors, CurrentTimestamp );
	}
	if ( b < (NumCells-1) )
	{
		CheckLiberties( a, b+1, StoneColor, AllDetectors, CurrentTimestamp );
	}

	return true;
}

// Could be done in one pass by making an ordered list of new event... 
bool GobanState::LookupForOlderEvent( StoneDetector( &AllDetectors )[MaxNumCells][MaxNumCells], double CurrentTimestamp, Omiscid::SimpleString Comment /* = "" */, bool EndKifu /* = false */ )
{
	int posa = -1, posb = -1;
	// +1.0 will include current event
	double lTimestamp = CurrentTimestamp + 1.0;
	int NbNewFoundOlder = 0;
	for ( int a = 0; a < NumCells; a++ )
	{
		for ( int b = 0; b < NumCells; b++ )
		{
			// Do not consider moving cells
			if ( AllDetectors[a][b].InMotionExtended == true )
			{
				continue;
			}
			int CurrentStoneState = AllDetectors[a][b].State;				// ComputeAndRetrieveState(CurrentTimestamp);

			if ( CurrentStoneState != Goban[a][b].State &&					// Is it a different event, aka new event?
				(CurrentTimestamp - AllDetectors[a][b].Timestamp) >= 5.0 )	// Older enough?
			{
				if ( CurrentStoneState == Empty )
				{
					// Ok, back to empty state
					if ( (CurrentTimestamp - AllDetectors[a][b].Timestamp) >= 10.0 )
					{
						Goban[a][b].State = Empty;
					}

					// Do not count it as event
					continue;
				}

				// New event, not empty
				NbNewFoundOlder++;
				if ( CurrentStoneState == SearchFor && AllDetectors[a][b].Timestamp < lTimestamp )
				{
					// Do we already found a search event?
					if ( posa == -1 )
					{
						//	no, first one !
						posa = a;
						posb = b;
					}
					else
					{
						if ( AllDetectors[a][b].Timestamp > AllDetectors[a][b].Timestamp )
						{
							// New oldest event of the good color
							posa = a;
							posb = b;
						}
						// Not better, we already count it, and we keep previous posa, posb event
					}
				}
			}
		}
	}

	// did we found an event ?
	if ( posa >= 0 )
	{
		// yes, fixe it !
		Goban[posa][posb].State = SearchFor;
		AllDetectors[posa][posb].Fixed = true;		// Still interesting?

		// Add move to the Kifu sgf file
		SGFWriter.AddMove( SearchFor, posa, posb, Comment );

		// Switch for next search
		SwitchState();

		// Check if we need to remove stones!
		// Use new value for SearchFor (that has been switched) as color for possibly removal stones
		ValidateCapture( posa, posb, SearchFor, AllDetectors, CurrentTimestamp );

		return true;
	}

	// Not found an event but with the wrong color, add it but set a comment on this stone
	if ( NbNewFoundOlder >= 1 )
	{
		// here, we have 2 consecutive bad event, do not wait longer to solve it...

		// call recursively, we already know that the state will match
		SwitchState();
		return LookupForOlderEvent( AllDetectors, CurrentTimestamp, "Check this move" );
	}

	// Here not found, or only 1 event but with the wrong color, wait for next event to try to solve it

	return false;
}

void GobanState::UpdateCurrentState( StoneDetector( &AllDetectors )[MaxNumCells][MaxNumCells], double CurrentTimestamp )
{
	// Search iteratively alternatively for stones: black, white, black, white...
	while ( LookupForOlderEvent( AllDetectors, CurrentTimestamp ) );
}

// Draw a empty goban centered within the drawing area
void GobanState::DrawGoban( cv::Mat& WhereToDraw )
{
	// Compute goban size
	int minSize = Min( WhereToDraw.cols, WhereToDraw.rows );

	// NumCells for the grid +1 for the space arround the goban
	int DrawingCellSize = minSize/(NumCells+1);

	// Compute center
	int CenterX = WhereToDraw.cols/2;
	int CenterY = WhereToDraw.rows/2;

	// Comput starting point from center of image
	int StartCol = CenterX - DrawingCellSize*(NumCells/2);
	int StartRow = CenterY - DrawingCellSize*(NumCells/2);

	// Fond blanc
	WhereToDraw = cv::Scalar( 255, 255, 255 );

	// Draw goban
	cv::rectangle( WhereToDraw, cv::Rect( StartCol-DrawingCellSize/2, StartRow-DrawingCellSize/2, DrawingCellSize*NumCells, DrawingCellSize*NumCells ), cv::Scalar( 110, 190, 235 ), -1 );

	// Draw lines
	int LineSize = (NumCells-1)*DrawingCellSize;
	for ( int a = 0; a < NumCells; a++ )
	{
		cv::line( WhereToDraw, cv::Point( StartCol+a*DrawingCellSize, StartRow ), cv::Point( StartCol+a*DrawingCellSize, StartRow+LineSize ), cv::Scalar( 0, 0, 0 ), 2 );
		cv::line( WhereToDraw, cv::Point( StartCol, StartRow+a*DrawingCellSize ), cv::Point( StartCol+LineSize, StartRow+a*DrawingCellSize ), cv::Scalar( 0, 0, 0 ), 2 );
	}

	// TO CHECK, point name
	// Draw points on goban
	if ( NumCells == 7 )
	{
		for ( int a = 3; a < NumCells; a += NumCells/2 )
		{
			for ( int b = 3; b < NumCells; b += 6 )
			{
				cv::circle( WhereToDraw, cv::Point( StartCol+(a)*DrawingCellSize, StartRow+(b)*DrawingCellSize ), DrawingCellSize*15/100, cv::Scalar( 0, 0, 0 ), -1 );
			}
		}
	}
	else
	{
		for ( int a = 3; a < NumCells; a += 6 )
		{
			for ( int b = 3; b < NumCells; b += 6 )
			{
				cv::circle( WhereToDraw, cv::Point( StartCol+(a)*DrawingCellSize, StartRow+(b)*DrawingCellSize ), DrawingCellSize*15/100, cv::Scalar( 0, 0, 0 ), -1 );
			}
		}
	}
	// TO CHECK: other non standard size?
}

void GobanState::DrawCurrentState( cv::Mat& WhereToDraw, double CurrentTimestamp )
{
	// Compute goban size
	int minSize = Min( WhereToDraw.cols, WhereToDraw.rows );

	// NumCells for the grid +1 for the space arround the goban
	int DrawingCellSize = minSize/(NumCells+1);
	int DrawingStoneSize = (DrawingCellSize/2)*90/100;

	// Comput starting point from center of image
	int StartCol = WhereToDraw.cols/2-DrawingCellSize*(NumCells/2);
	int StartRow = WhereToDraw.rows/2-DrawingCellSize*(NumCells/2);

	DrawGoban( WhereToDraw );

	// Draw stones if any
	int colpos = StartCol;
	for ( int a = 0; a < NumCells; a++, colpos += DrawingCellSize )
	{
		int rowpos = StartRow;
		for ( int b = 0; b < NumCells; b++, rowpos += DrawingCellSize )
		{

			if ( Goban[a][b].State == StoneDetector::Black )
			{
				// AllDetectors[a][b].Fixed = true;
				cv::circle( WhereToDraw, cv::Point( StartCol+(a)*DrawingCellSize, StartRow+(b)*DrawingCellSize ), DrawingStoneSize, cv::Scalar( 0, 0, 0 ), -1 );
			}
			else if ( Goban[a][b].State == StoneDetector::White )
			{
				// AllDetectors[a][b].Fixed = true;
				cv::circle( WhereToDraw, cv::Point( StartCol+(a)*DrawingCellSize, StartRow+(b)*DrawingCellSize ), DrawingStoneSize, cv::Scalar( 20, 20, 20 ), -1 );
				cv::circle( WhereToDraw, cv::Point( StartCol+(a)*DrawingCellSize, StartRow+(b)*DrawingCellSize ), DrawingStoneSize-2, cv::Scalar( 255, 255, 255 ), -1 );
			}
		}
	}
}