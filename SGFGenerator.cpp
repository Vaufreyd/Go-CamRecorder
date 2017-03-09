/**
 * @file SGFGenerator.cpp
 * @ingroup Go-CamRecorder
 * @author Dominique Vaufreydaz, personnal project
 * @copyright All right reserved.
 */

#include "SGFGenerator.h"

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
bool SGFGenerator::Open( Omiscid::SimpleString Folder, Omiscid::SimpleString& Event, Omiscid::SimpleString& Round, Omiscid::SimpleString& Rule, Omiscid::SimpleString& Komi,
	Omiscid::SimpleString& Date, Omiscid::SimpleString& Time, Omiscid::SimpleString& BlackPlayerName, Omiscid::SimpleString& WhitePlayerName )
{
	Omiscid::SimpleString FileName;

	// Generate filename from date, time and random numbers in order to prevent
	// games with same file name
	FileName += Date + "." + Time + "_";

	// Generate 8 ramdom chararacters to prevent same name from different computer
	for ( int i = 0; i < 10; i++ )
	{
		FileName += (char)('0' + (char)(Omiscid::random()%10));
	}
	FileName += ".sgf";

	// Generate local file name, into Folder
	// First, add '/' if mandatory
	SGFFileName = Folder;
	if ( SGFFileName[SGFFileName.GetLength()-1] != '/' )
	{
		SGFFileName += '/';
	}
	SGFFileName += FileName;

	// Open localfile name
	fout = fopen( SGFFileName.GetStr(), "wb" );
	if ( fout == nullptr )
	{
		return false;
	}

	// Set no buffer to file, security in case of crash, state is in the file...
	// At the end, we will need to rewrite the file with the result, if any...
	setbuf( fout, NULL );

	// Generate SGF Header
	FileContent = GenerateSGFHeader( NumCells, FileName, Event, Round, Rule, Komi, Date, BlackPlayerName, WhitePlayerName );

	// Write header of the file, need to check variation code for ST
	fprintf( fout, "%s\n", FileContent.GetStr() );

	// Init upload (if upload server and URL are not define, this call is effectless)
	OnlineUploader.InitDistantSGF( NumCells, FileName, FileContent );

	// FileContent must now have a '\n', maybe be done in a better way
	FileContent += '\n';

	return true;
}

/**
* @brief Open a new SGF file in a Folder. File name is generated with date, time and a random number to autorized multiple instance to run at the same time
			on the same computer.
* @param Color [in] Color of move (Black/White)
* @param Col [int] Col number
* @param Row [int] Row number
* @param Comment [int] Comment to as to current move.
* @return true if move has been added.
*/
bool SGFGenerator::AddMove( int Color, int Col, int Row, Omiscid::SimpleString Comment /* = "" */ )
{
	if ( fout == nullptr || Col >= NumCells || Row >= NumCells )
	{
		return false;
	}

	Omiscid::SimpleString CurrentMove = ";";
	CurrentMove += (Color == StoneDetector::Black ? 'B' : 'W');
	CurrentMove += '[';
	CurrentMove += (char)('a'+Col);
	CurrentMove += (char)('a'+Row);
	CurrentMove += ']';

	// Add comment if any
	if ( Comment.GetLength() != 0 )
	{
		// Escape en tag in Comment, Comment is a local copy
		CurrentMove += "C[" + Comment + "]";
	}

	// Add current move (and comment) to the file and later to the file content


	// Print current move in file
	fprintf( fout, "%s\n", CurrentMove.GetStr() );

	// Add move to online SGF (if configured)
	OnlineUploader.AddMove( CurrentMove );

	// Add current move to the FileContent
	FileContent += CurrentMove + "\n";

	return true;
}

/**
* @brief Close SGF file. Add Result if any.
* @param Result [in] Result of the game.
*/
void SGFGenerator::Close( Omiscid::SimpleString& Result )
{
	if ( fout != nullptr )
	{
		fprintf( fout, ")\n" );
		fclose( fout );
		fout = nullptr;
	}

	if ( Result.IsEmpty() == false )
	{
		// Reopen localfile name
		fout = fopen( SGFFileName.GetStr(), "wb" );
		if ( fout == nullptr )
		{
			fprintf( stderr, "Unable to store final game SGF with result\n" );
			return;
		}

		FileContent.ReplaceFirst( "RE[?]", "RE[" + Result + "]" );
		fprintf( fout, "%s)\n", FileContent.GetStr() );
		fclose( fout );
	}

	// End distant SGF (if configured)
	OnlineUploader.EndDistantSGF( Result );
}