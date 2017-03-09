/**
 * @file GobanState.h
 * @ingroup Go-CamRecorder
 * @author Dominique Vaufreydaz, personnal project
 * @copyright All right reserved.
 */


#ifndef __GOBAN_STATE_H__
#define __GOBAN_STATE_H__


#include <System/SimpleString.h>
#include <System/SimpleList.h>
#include <Messaging/Serializable.h>

#include "Go-CamRecorder.h"
#include "StoneState.h"
#include "StoneDetector.h"
#include "SGFGenerator.h"

/**
 * @class GobanState 
 * @brief Class to manage current state of the game
 */
class GobanState : public StoneState
{
public:
	/**
	 * @class Intersection 
	 * @brief Container class to handle state of a goban position
	 */
	class Intersection 
	{
	public:
		int  State = Empty;								// Current state of detection
		bool CaptureChecked = false;					// Flag used to limite recursive work when searching for capture
	};

	Intersection Goban[MaxNumCells][MaxNumCells];		// State of all goban positions (max goban size = 19)
	
	int NumCells;										// Number of Cells, i.e. goban size
	int SearchFor = StoneState::Black;					// First event, it is black

	/**
    * @brief Constructor
    * @param _NumCells [in] Size of the goban
	*/
	GobanState(int _NumCells) : NumCells(_NumCells), SGFWriter(_NumCells)
	{
	}

	/**
	* @brief Virtual destructor
	*/
	virtual ~GobanState()
	{}

	SGFGenerator SGFWriter;								// Writer of the SGF file

	/**
	* @brief Inline function to switc search stone color. Black/White/Black/white...
	*/
	inline void SwitchState()
	{
		SearchFor = (SearchFor+1)%StateModulo;
	}

	/**
	* @brief Check if the new move do a capture by searching recursively for liberties
	* @param a [in] current column of the goban
	* @param b [in] current line of the goban
	* @param StoneColor [in] Color of the stone on which we are searching for liberties
	* @param CurrentChain [in,out] List of all goban positions winthin the current territory
	* @throw This function will throw an exception if a liberty is found
	*/
	void CheckLibertiesRecursive( int a, int b, int StoneColor, Omiscid::SimpleList<cv::Point>& CurrentChain );

	/**
	* @brief Check if the new move do a capture by searching recursively for liberties
	* @param a [in] current column of the goban
	* @param b [in] current line of the goban
	* @param StoneDetector [in] Actual detection state on the goban
	* @param CurrentTimestamp [in] Current timestamp of the working frame
	* @return false if no liberty have been found
	*/
	bool CheckLiberties( int a, int b, int StoneColor, StoneDetector (&AllDetectors)[MaxNumCells][MaxNumCells], double CurrentTimestamp );

	/**
	* @brief Check if the new move do a capture by searching recursively for liberties. If there
			 is at least a liberty, no capture.
	* @param a [in] current column of the goban
	* @param b [in] current line of the goban
	* @param StoneDetector [in] Actual detection state on the goban
	* @param CurrentTimestamp [in] Current timestamp of the working frame
	* @return false if there is no cpature
	*/
	bool ValidateCapture( int a, int b, int StoneColor, StoneDetector (&AllDetectors)[MaxNumCells][MaxNumCells], double CurrentTimestamp );

	/**
	* @brief Search for the older event of the current searched stone color
	* @param StoneDetector [in] Actual detection state on the goban
	* @param CurrentTimestamp [in] Current timestamp of the working frame
	* @param Comment [in] Comment to add to the current move in SGF (default is no comment)
	* @param EndKifu [in] Is it the end of search? (default=false)
	* @return true if an event was found
	*/
	bool LookupForOlderEvent( StoneDetector (&AllDetectors)[MaxNumCells][MaxNumCells], double CurrentTimestamp, Omiscid::SimpleString Comment = "", bool EndKifu = false );

	/**
	* @brief Search alternatively for the older event of the current searched stone color, siwtch color and restart until it failed.
			 There is an integration time to prevent false detection to be include too easily
	* @param StoneDetector [in] Actual detection state on the goban
	* @param CurrentTimestamp [in] Current timestamp of the working frame
	*/
	void UpdateCurrentState( StoneDetector (&AllDetectors)[MaxNumCells][MaxNumCells], double CurrentTimestamp );

	/**
	* @brief Draw an empty square goban centered in the image
	* @param WhereToDraw [in,out] Actual detection state on the goban
	*/
	void DrawGoban( cv::Mat& WhereToDraw );

	/**
	* @brief Draw an goban centered in the image. Draw stone at their current detected place.
	* @param WhereToDraw [in,out] Actual detection state on the goban
	*/
	void DrawCurrentState( cv::Mat& WhereToDraw, double CurrentTimestamp );

};

#endif // __GOBAN_STATE_H__
