/**
 * @file SGFGenerator.h
 * @ingroup Go-CamRecorder
 * @author Dominique Vaufreydaz, personnal project
 * @copyright All right reserved.
 */

#ifndef __SGF_GENERATOR_H__
#define __SGF_GENERATOR_H__

#include <System/Portage.h>
#include <System/SimpleString.h>

#include <opencv2/video/tracking.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/background_segm.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2//imgproc/imgproc.hpp>

#include "Go-CamRecorder.h"
#include "StoneDetector.h"
#include "UpdateOnline.h"

/**
* @brief Utility function. Generate an SGF header from data.
* @return SgfHeader as SimpleString
*/
inline Omiscid::SimpleString GenerateSGFHeader( const int GobanSize, Omiscid::SimpleString& FileName, Omiscid::SimpleString& EventName, Omiscid::SimpleString& RoundName,
	Omiscid::SimpleString& Rule, Omiscid::SimpleString& Komi, Omiscid::SimpleString& Date, Omiscid::SimpleString& BlackPlayerName, Omiscid::SimpleString& WhitePlayerName )
{
	Omiscid::SimpleString Result;

	Omiscid::SimpleString Version = "Go-CamRecorder:1.0a";

	Result = "(;GM[1]FF[4]CA[latin1]AP[" + Version + "]ST[2]SZ[" + Omiscid::SimpleString(GobanSize) + "]PB[" + BlackPlayerName + "]PW[" + WhitePlayerName + "]DT[" + Date + "]RE[?]";
	if ( EventName.IsEmpty() == false )
	{
		Result += "EV[" + EventName + "]";
	}

	if ( RoundName.IsEmpty() == false )
	{
		Result += "RO[" + RoundName + "]";
	}

	if ( Rule.IsEmpty() == false )
	{
		Result += "RU[" + Rule + "]";
	}

	if ( Komi.IsEmpty() == false )
	{
		Result += "KM[" + Komi + "]";
	}

	return Result;
}

/**
 * @class SGFGenerator 
 * @brief Generate SGF from information and moves. The file is generated in a simple forward maner. Could be improved.
		Each time a action is done on SGF, a copy of it is upload on the web server (if configured).
 */
class SGFGenerator
{
protected:
	FILE * fout;						// Local file pointer
	UploadOnline OnlineUploader;		// To upload online
	int NumCells;						// Number of cells, i.e. size of goban
	Omiscid::SimpleString SGFFileName;	// SGF File name, generate from time and date
	Omiscid::SimpleString FileContent;	// Will contain the file

public:
	/**
    * @brief Constructor
	*/
	SGFGenerator(int _NumCells) : NumCells(_NumCells), OnlineUploader( SingleConfig.UploadWebSite, SingleConfig.UploadURL )
	{
		fout = nullptr;
	}

	/**
	* @brief Virtual destructor
	*/
	virtual ~SGFGenerator() {}

	/**
	* @brief Open a new SGF file in a Folder. File name is generated with date, time and a random number to autorized multiple instance to run at the same time
			 on the same computer.
	* @param Folder [in] Folder name to store SGF file
	* @param Event [int] Event name (i.e. competition name)
	* @param Event [int] Event name (i.e. competition name)
	* @param Event [int] Event name (i.e. competition name)
	* @param Round [int] Round turn
	* @param Rule [int] Used rules (japanese, ...)
	* @param Komi [int] Komi set.
	* @param Date [int] Date as string
	* @param Time [int] Time as string
	* @param BlackPlayerName [int] Black player name
	* @param WhitePlayerName [int] White player name
	* @return true if SGF could be opened.
	*/
	bool Open( Omiscid::SimpleString Folder, Omiscid::SimpleString& Event, Omiscid::SimpleString& Round, Omiscid::SimpleString& Rule, Omiscid::SimpleString& Komi, 
		Omiscid::SimpleString& Date, Omiscid::SimpleString& Time, Omiscid::SimpleString& BlackPlayerName, Omiscid::SimpleString& WhitePlayerName );

	/**
	* @brief Open a new SGF file in a Folder. File name is generated with date, time and a random number to autorized multiple instance to run at the same time
			 on the same computer.
	* @param Color [in] Color of move (Black/White)
	* @param Col [int] Col number
	* @param Row [int] Row number
	* @param Comment [int] Comment to as to current move.
	* @return true if move has been added.
	*/
	bool AddMove( int Color, int Col, int Row, Omiscid::SimpleString Comment = "" );

	/**
	* @brief Close SGF file. Add Result if any.
	* @param Result [in] Result of the game.
	*/
	void Close(Omiscid::SimpleString& Result);
};

#endif // __SGF_GENERATOR_H__
