/**
 * @file UploadOnline.h
 * @ingroup Go-CamRecorder
 * @author Dominique Vaufreydaz, personnal project
 * @copyright All right reserved.
 */

#ifndef __UPDATE_ONLINE_H__
#define __UPDATE_ONLINE_H__

#include <System/Socket.h>
#include <System/Mutex.h>
#include <System/Thread.h>

// API Key to prevent upload from unknown clients
#define APIKey "TO_BE_DEFINED"					// This key must be defined at compile time to permits upload on the server

/**
 * @class UploadOnline 
 * @brief Thread to upload when mandatory (if a modification appears) on the Web site using UploadSGF.php file.
 */
class UploadOnline : public Omiscid::Thread
{
protected:
	Omiscid::SimpleString Host;					// HTTP host
	Omiscid::SimpleString URL;					// URL on the Web server
	int HTTPPort;								// HTTP port

	Omiscid::Mutex ProtectCurrentFileContent;	// Mutex for multithreading access
	Omiscid::SimpleString CurrentFileContent;	// Current file content
	int ValueUpdate;							// ValueUpate, aka version number of the SGF file.
	Omiscid::SimpleString PrecomputedJson;		// Precomputed Json Header
	Omiscid::SimpleString PrecomputedHeader;	// Precomputed HTTP header


	/**
	* @brief Upload file on server using HTTP1.1
	* @param EndGame [in] Is it the end of the file? (i.e. last call)
	*/
	bool UploadOnServeur(bool EndGame = false);

	/**
	* @brief Send data for uploading. If LastUpdateValue does not equal ValueUpdate, upload is done.
	* @param LastUpdateValue [in] Last update value, i.e. last version of the SGF file.
	*/
	void SendIt(int& LastUpdateValue);

public:
	/**
	* @brief Constructor
	* @param RemoteHost [in] HTTP1.1 host name
	* @param RemoteURL [in] HTTP1.1 URL, in this case, path on the HTTP server
	* @param RemoteHTTPPort [in] HTTP server port (default = 80)
	*/
	UploadOnline( const Omiscid::SimpleString& RemoteHost, const Omiscid::SimpleString& RemoteURL, int RemoteHTTPPort = 80 )
	{
		Host = RemoteHost;
		URL = RemoteURL;
		HTTPPort = RemoteHTTPPort;
		ValueUpdate = 0;
	}

	/**
	* @brief Virtual destructor
	*/
	virtual ~UploadOnline() {}

	/**
	* @brief Utility function to check if the remote HTTP server is well configured.
	*/
	inline bool IsConfigured()
	{
		return (Host.IsEmpty() == false && URL.IsEmpty() == false);
	}

	/** @brief Method executed in a thread to send peridically (if needed) the new SGF file.
	 */
	virtual void FUNCTION_CALL_TYPE Run();

	/**
	* @brief Init SGF file for upload.
	* @param GobanSize [in] Size of goban
	* @param FileName [in] SGF file name
	* @param FileName [in] SGF Header (event, player names, komi, rules ...)
	*/
	void InitDistantSGF( int GobanSize, const Omiscid::SimpleString& FileName, const Omiscid::SimpleString& SGFHeader );

	/**
	* @brief Add a new move at the end of the SGF. 
	* @param NewMove [in] Move information.
	*/
	void AddMove(const Omiscid::SimpleString NewMove);

	/**
	* @brief End the SGF file, add result if provided by the user and make a last upload.
	* @param Result [in] Game result computed by a human.
	*/
	void EndDistantSGF(Omiscid::SimpleString Result);
};

#endif

