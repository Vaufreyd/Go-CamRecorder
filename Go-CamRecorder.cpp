/**
 * @file Go-CamRecorder.cpp
 * @ingroup Go-CamRecorder
 * @author Dominique Vaufreydaz, personnal project
 * @copyright All right reserved.
 */

#include "Go-CamRecorder.h"

// Unique instance of ConfigInfo to read/save from file
ConfigInfo SingleConfig;

/**
* @brief Load values from the ConfigFileName file.
* @return True if ok.
*/
bool ConfigInfo::Load()
{
	FILE * fin = fopen( ConfigFileName, "rb" );
	if ( fin == nullptr )
	{
		return false;
	}

	Omiscid::MemoryBuffer Tmpc(1024);
	Omiscid::SimpleString JsonData;
	while( fgets( (char*)Tmpc, (int)Tmpc.GetLength(), fin ) )
	{
		JsonData += (char*)Tmpc;
	}

	Unserialize(JsonData);

	fclose( fin );

	return true;
}

/**
* @brief Save values to the ConfigFileName file.
* @return True if ok.
*/
bool ConfigInfo::Save()
{
	FILE * fout = fopen( ConfigFileName, "wb" );
	if ( fout == nullptr )
	{
		return false;
	}
	Omiscid::StructuredMessage::Indented = true;
	Omiscid::SimpleString JsonData = Serialize();
	fprintf( fout, "%s\n", JsonData.GetStr() );
	fclose( fout );
	return true;
}

