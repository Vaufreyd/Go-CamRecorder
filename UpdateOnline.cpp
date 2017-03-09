/**
* @file UploadOnline.h
* @ingroup Go-CamRecorder
* @author Dominique Vaufreydaz, personnal project
* @copyright All right reserved.
*/

#include "UpdateOnline.h"

/**
* @brief Upload file on server using HTTP1.1
* @param EndGame [in] Is it the end of the file? (i.e. last call)
*/
bool UploadOnline::UploadOnServeur( bool EndGame /* = false */ )
{
	try
	{
		// Lock was done at upper level

		// Generate data to send
		Omiscid::SimpleString JsonMsg = PrecomputedJson + CurrentFileContent;

		if ( EndGame == false )
		{
			JsonMsg += ")\",\"End\":\"0\"}";
		}
		else
		{
			JsonMsg += "C[End of game])\",\"End\":\"1\"}";
		}

		// Generate Header
		Omiscid::SimpleString DataToSend = PrecomputedHeader;
		DataToSend += JsonMsg.GetLength();
		DataToSend += "\r\n\r\n";

		// Add POST data and finish JsonData
		DataToSend += JsonMsg;

		Omiscid::Socket ClientSock( Omiscid::Socket::TCP );
		if ( ClientSock.Connect( Host, HTTPPort ) == false )
		{
			return false;
		}

		// fprintf( stderr, DataToSend.GetStr() );

		// Send data to server
		ClientSock.Send( DataToSend.GetLength(), DataToSend.GetStr() );

		char TmpC[20];
		TmpC[0] = '\0';
		if ( ClientSock.Recv( 14, (unsigned char*)TmpC ) )
		{
			// Check for HTTP return code here
			// baseline is Unrecoverable Error (even if we receive nothing in return)
			int HTTPReturnedCode = 456;
			// If scanf failed returned code will stay 456, do not check sscanf return value
			sscanf( (char*)TmpC, "HTTP/%*c.%*c %d", &HTTPReturnedCode );
			// HTTP returned code should be 200 but could be anothing else like 202 Created
			// Accept only in range [200,299]
			if ( HTTPReturnedCode < 200 || HTTPReturnedCode >= 300 )
			{
				fprintf( stderr, "HTTP uploading request failed with code %d\n", HTTPReturnedCode );
			}
		}

		ClientSock.Close();
	}
	catch ( const Omiscid::SimpleException& e )
	{
		fprintf( stderr, "SocketError: %s (%d)\n", e.msg.GetStr(), e.err );
	}
	return false;
}

/**
* @brief Send data for uploading. If LastUpdateValue does not equal ValueUpdate, upload is done.
* @param LastUpdateValue [in] Last update value, i.e. last version of the SGF file.
*/
void UploadOnline::SendIt( int& LastUpdateValue )
{
	if ( LastUpdateValue != ValueUpdate )
	{
		// Lock file content
		Omiscid::SmartLocker SL_ProtectCurrentFileContent( ProtectCurrentFileContent );
		UploadOnServeur( ValueUpdate == -2 );
		LastUpdateValue = ValueUpdate;
	}
}



/** @brief Method executed in a thread to send peridically (if needed) the new SGF file.
*/
void FUNCTION_CALL_TYPE UploadOnline::Run()
{
	if ( IsConfigured() == false )
	{
		fprintf( stderr, "No upload server nor URL defined, upload disabled.\n" );
		return;
	}

	int LastUpdateValue = 0;
	for ( ; StopPending() == false; Thread::Sleep( 1000 ) )	// Wait for 1s after each loop
	{
		SendIt( LastUpdateValue );
	}

	// before Leaving, try to send last version
	SendIt( LastUpdateValue );
}

/**
* @brief Init SGF file for upload.
* @param GobanSize [in] Size of goban
* @param FileName [in] SGF file name
* @param FileName [in] SGF Header (event, player names, komi, rules ...)
*/
void UploadOnline::InitDistantSGF( int GobanSize, const Omiscid::SimpleString& FileName, const Omiscid::SimpleString& SGFHeader )
{
	if ( IsConfigured() == false )
	{
		fprintf( stderr, "No upload server nor URL defined, upload disabled.\n" );
		return;
	}

	// Lock file content
	Omiscid::SmartLocker SL_ProtectCurrentFileContent( ProtectCurrentFileContent );

	if ( URL[0] != '/' )
	{
		URL = "/" + URL;
	}

	// Precomputed header
	PrecomputedHeader = "POST " + URL + " HTTP/1.1\r\nUser-Agent: Go-CamRecorder 1.0a\r\nAccept: */*\r\nContent-Type: application/application/json\r\n";	// [WAS] Content-Type: x-www-form-urlencoded
	PrecomputedHeader += "Host: " + Host + "\r\n";
	PrecomputedHeader += "Content-Length: ";

	// Init begining of file
	CurrentFileContent = SGFHeader + "\\n";

	// Generate Precomputed header, could use JSon serialization facility
	PrecomputedJson = "{\"APIKey\":\"" + Omiscid::SimpleString( APIKey ) + "\",\"FileName\":\"" + FileName + "\",\"FileContent\":\"";

	// Sure to restart sending in thread
	ValueUpdate = -1;

	// Start my thread part
	StartThread();
}

/**
* @brief Add a new move at the end of the SGF. 
* @param NewMove [in] Move information.
*/
void UploadOnline::AddMove( const Omiscid::SimpleString NewMove )
{
	if ( IsConfigured() == false || IsRunning() == false )
	{
		return;
	}

	// Lock file content
	Omiscid::SmartLocker SL_ProtectCurrentFileContent( ProtectCurrentFileContent );

	// Add move followed by line return, no check here about content!
	CurrentFileContent += NewMove + "\\n";

	ValueUpdate++;
}

/**
* @brief End the SGF file, add result if provided by the user and make a last upload.
* @param Result [in] Game result computed by a human.
*/
void UploadOnline::EndDistantSGF( Omiscid::SimpleString Result )
{
	if ( IsConfigured() == false || IsRunning() == false )
	{
		return;
	}

	// Lock file content
	Omiscid::SmartLocker SL_ProtectCurrentFileContent( ProtectCurrentFileContent );

	if ( Result.IsEmpty() == false )
	{
		CurrentFileContent.ReplaceFirst( "RE[?]", "RE[" + Result + "]" );
	}

	ValueUpdate = -2;	// Code to end game

	SL_ProtectCurrentFileContent.Unlock();

	// End my thread part, wait for sending last data for 5s max
	StopThread( 5000 );
}