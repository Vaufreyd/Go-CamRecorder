/**
 * @file MainGo.cpp
 * @ingroup Go-CamRecorder
 * @author Dominique Vaufreydaz, personnal project
 * @copyright All right reserved.
 */

#include <System/MutexedSimpleList.h>
#include <System/ElapsedTime.h>
#include <System/TypedMemoryBuffer.h>

#include "Go-CamRecorder.h"
#include "GobanDetector.h"
#include "MultiSourceVideo.h"	// will include VideoIO also


#include <sys/stat.h>
#include <algorithm>

using namespace std;
using namespace cv;

#ifndef OMISCID_ON_WINDOWS
inline bool CreateDirectory( const char * Filename, void * Dummy )
{
	mode_t mode = 0755;
	return ( mkdir(Filename, mode) == 0 );
}
#endif 

#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>


/**
 * @class ProcessingStatistics  S
 * @brief Class to compute statistics (Fps, processing time...)
 */
class ProcessingStatistics
{
public:
	double StatsReportTime;				// report fsp every ~10 s
	Omiscid::PerfElapsedTime FpsTime;	// To get time
	int FpsStartFrame;					// 1, as NumFrame == 1
	
	double MinProcessingTime;			// Min processing time
	double MaxProcessingTime;			// Max processing time

    /**
    * @brief Constructor
    * @param ReportTime [in] Time between statistics reports
	*/
	ProcessingStatistics(double ReportTime) : StatsReportTime(ReportTime)
	{
		Init(0);
	}

    /**
    * @brief Virtual destructor
	*/
	virtual ~ProcessingStatistics() {}

	/**
    * @brief Virtual destructor
	* @param NumFrame [in] Current frame number
	*/
	void Init(int NumFrame)
	{
		FpsStartFrame = NumFrame;					// Current frame number 
		MinProcessingTime = 24.0*60.0*60.0*1000;	// 1 day in millisecond as min
		MaxProcessingTime = 0.0;					// 0 ms, as max
		FpsTime.Reset();
	}

	/**
    * @brief Virtual destructor
	* @param NumFrame [in] Current frame number
	* @param ProcessingTime [in] Current processing time
	* @param fout [in] Current file to output statictics (default=stderr)
	*/
	void UpdateAndReportStats(int NumFrame, double ProcessingTime, FILE * fout = stderr)
	{
		// UpdateStats about processing time
		if ( ProcessingTime < MinProcessingTime )
		{
			MinProcessingTime = ProcessingTime;
		}

		if ( ProcessingTime > MaxProcessingTime )
		{
			MaxProcessingTime = ProcessingTime;
		}

		double CurrentFpsTime = FpsTime.GetInSeconds();
		if ( CurrentFpsTime < StatsReportTime )
		{
			// Nothing to do
			return;
		}

		fprintf( fout, "\rfps=%3.3lf (min=%.6lf, max=%.6lf)", (double)(NumFrame-FpsStartFrame)/CurrentFpsTime, MinProcessingTime, MaxProcessingTime );
		Init(NumFrame);
	}
};

/**
* @brief Utility function to get current time as an Omiscid::SimpleString
*/
inline Omiscid::SimpleString GetTimeAsString()
{
	// Atomatically folder name from local time
	time_t		now = time( nullptr );
	struct tm * tstruct = localtime( &now );
	Omiscid::TypedMemoryBuffer<char> TmpDateAndTime(100);

	// Create folder
	strftime( TmpDateAndTime, TmpDateAndTime.GetLength(), "%Y-%m-%d.%H-%M", tstruct );

	return Omiscid::SimpleString((char*)TmpDateAndTime);
}

/**
* @brief Utility function to get current time and date as Omiscid::SimpleString
* @param Date [out] Retrieve Date
* @param Date [out] Retrieve Time
*/
inline void GetDateAndTimeAsStrings( Omiscid::SimpleString& Date, Omiscid::SimpleString& Time )
{
	// Atomatically folder name from local time
	time_t		now = time( nullptr );
	struct tm * tstruct = localtime( &now );
	Omiscid::TypedMemoryBuffer<char> TmpDateAndTime(100);

	strftime( TmpDateAndTime, TmpDateAndTime.GetLength(), "%Y-%m-%d", tstruct );
	Date = (char*)TmpDateAndTime;
	strftime( TmpDateAndTime, TmpDateAndTime.GetLength(), "%H-%M", tstruct );
	Time = (char*)TmpDateAndTime;
}

/**
* @brief Utility function to get current time and date as Omiscid::SimpleString
* @param Variable [in,out] Variable to set if empty. If not, the function does nothing.
* @param InputMessage [in] Input message to show in the console
* @param DefaultValue [in] DefaultValue if no user input
*/
void CheckAndSetVariable(Omiscid::SimpleString& Variable, Omiscid::SimpleString InputMessage, Omiscid::SimpleString DefaultValue )
{
	char Tmpc[512];

	if ( Variable.IsEmpty() == false )
	{
		return;
	}

	printf( "%s (default:%s):", InputMessage.GetStr(), DefaultValue.GetLength() != 0 ? DefaultValue.GetStr() : "none" );
	Tmpc[0] = '\0';
	fgets(Tmpc, 512, stdin);
	for( int i = 0; Tmpc[i] != '\0'; i++ )
	{
		if ( Tmpc[i] == '\n' || Tmpc[i] == '\r' )
		{
			Tmpc[i] = '\0';
			break;
		}
	}

	Variable = Tmpc;

	if ( Variable.IsEmpty() )
	{
		Variable = DefaultValue;
	}
}

#define OutputFolderName "Results/"		// Default output folder for sgf file

/**
* @brief Main function. It will produce an sgf file. Name of the file is generated using date and time.
         A export video could be done using ffmpeg. 
*/
int main( int argc, char *argv[] )
{
	// Param from commande line
	Omiscid::SimpleString RecordingDeviceOrFile = "0";	// Default recording device

	// SGF option
	Omiscid::SimpleString BlackPlayerName;	// Default black player name
	Omiscid::SimpleString WhitePlayerName;	// Default white player name
	Omiscid::SimpleString EventName;		// Name of the event, if any
	Omiscid::SimpleString RoundName;		// Current round
	Omiscid::SimpleString Rule;				// Rule used during the game
	Omiscid::SimpleString Komi;				// Komi

	int GobanSize = 19;						// Default goban size

	bool ExportResultVideo = false;			// Flag to know if we want to produce and mp4 file.

	bool AutomaticRescaleOutput = true;		// Flag to indicate if we want to resclae video and feedback

	// First load config file, if exists
	SingleConfig.Load();
	EventName = SingleConfig.EventName;

	// Work on arguments
	for ( int PosArg = 1; PosArg < argc; PosArg++ )
	{
		if ( strcasecmp("-source", argv[PosArg]) == 0 )
		{
			PosArg++;
			if ( PosArg >= argc )
			{
				fprintf( stderr, "Missing parameter after '-source' option\n" );
				return -1;
			}
			RecordingDeviceOrFile = argv[PosArg];
			continue;
		}
		if ( strcasecmp("-h", argv[PosArg]) == 0 || strcasecmp("-help", argv[PosArg]) == 0 || strcasecmp("--help", argv[PosArg]) == 0 )
		{
			fprintf( stderr, "Usage: %s [-source <source_name>] [-export] [-noauto] [-sz <goban size>] [-ev <event_name>] [-ro <round>] [-pb <black player name>] [-pw <white player name>] ", argv[0] );
			fprintf( stderr, "[-km <Komi>] [-ru <rules>]\n" );
			fprintf( stderr, "-source: Defaul source is '0' (default camera). Source must be a device number, 'kinect1:' or a video file.\n" );
			fprintf( stderr, "-export: Export result also as an mp4 file using ffmpeg.\n-noauto: do not auto resize too small image." );
			fprintf( stderr, "-sz: Size of goban (Default=19)\n//// SGF content ///" );
			fprintf( stderr, "-ev: Event name.\n-ro: Round.\n-pb: Black player name.\n-pw: White player name.\n-km: Komi (Default=7.5)\n-ru: Rules (Default none)\n\n" );
			return 0;
		}
		if ( strcasecmp("-export", argv[PosArg]) == 0 )
		{
			ExportResultVideo = true;
			continue;
		}

		// Sgf options
		if ( strcasecmp("-EV", argv[PosArg]) == 0 )
		{
			PosArg++;
			if ( PosArg >= argc )
			{
				fprintf( stderr, "Missing parameter after '-ev' option\n" );
				return -1;
			}
			EventName = argv[PosArg];
			continue;
		}

		if ( strcasecmp("-RO", argv[PosArg]) == 0 )
		{
			PosArg++;
			if ( PosArg >= argc )
			{
				fprintf( stderr, "Missing parameter after '-ro' option\n" );
				return -1;
			}
			RoundName = argv[PosArg];
			continue;
		}
		if ( strcasecmp("-PB", argv[PosArg]) == 0 )
		{
			PosArg++;
			if ( PosArg >= argc )
			{
				fprintf( stderr, "Missing parameter after '-pb' option\n" );
				return -1;
			}
			BlackPlayerName = argv[PosArg];
			continue;
		}

		if ( strcasecmp("-PW", argv[PosArg]) == 0 )
		{
			PosArg++;
			if ( PosArg >= argc )
			{
				fprintf( stderr, "Missing parameter after '-PW' option\n" );
				return -1;
			}
			WhitePlayerName = argv[PosArg];
			continue;
		}

		if ( strcasecmp("-KM", argv[PosArg]) == 0 )
		{
			PosArg++;
			if ( PosArg >= argc )
			{
				fprintf( stderr, "Missing parameter after '-KM' option\n" );
				return -1;
			}
			Komi = argv[PosArg];
			continue;
		}

		if ( strcasecmp("-RU", argv[PosArg]) == 0 )
		{
			PosArg++;
			if ( PosArg >= argc )
			{
				fprintf( stderr, "Missing parameter after '-RU' option\n" );
				return -1;
			}
			Rule = argv[PosArg];
			continue;
		}

		if ( strcasecmp("-SZ", argv[PosArg]) == 0 )
		{
			PosArg++;
			if ( PosArg >= argc )
			{
				fprintf( stderr, "Missing parameter after '-SZ' option\n" );
				return -1;
			}
			int tmpi = atoi(argv[PosArg]);
			if ( tmpi != 9 && tmpi != 13 && tmpi != 19 )
			{
				fprintf( stderr, "Bad goban size after '-SZ' option (should be 7, 13 or 19)\n" );
				return -1;
			}
			GobanSize = tmpi;
			continue;
		}

		if ( strcasecmp("-noauto", argv[PosArg]) == 0 )
		{
			AutomaticRescaleOutput = false;
			continue;
		}

		fprintf( stderr, "Bad parameter '%s'\n", argv[PosArg] );
		return -1;
	}

	// Check variable
	CheckAndSetVariable( EventName, "Enter event name", "" );
	CheckAndSetVariable( RoundName, "Enter round name", "" );
	// Later support
	// CheckAndSetVariable( Rules, "Enter rules", "" );
	CheckAndSetVariable( Komi, "Enter komi", DefaultKomi );
	CheckAndSetVariable( BlackPlayerName, "Enter black player name", "BlackPlayer" );
	CheckAndSetVariable( WhitePlayerName, "Enter white player name", "WhitePlayer" );

	Omiscid::SimpleString Date, Time;
	GetDateAndTimeAsStrings( Date, Time );

	// Set angles
	MultiVideoSource Vid;
	Vid.Open(RecordingDeviceOrFile.GetStr());

	if ( Vid.IsOpen() == false )
	{
		fprintf( stderr, "Could not open device or file '%s'\n", RecordingDeviceOrFile.GetStr() );
		return -1;
	}

	// Create output folder
	CreateDirectory( OutputFolderName, NULL );

	// Create Goban detector
	GobanDetector Goban(GobanSize);

	// Remove trailing ":" or '/' here from RecordingDeviceOrFile => store calibration file
	char TrailingChar = RecordingDeviceOrFile[RecordingDeviceOrFile.GetLength()-1];
	if ( TrailingChar == ':' || TrailingChar == '/' )
	{
		// C++11 view as an std::string
		RecordingDeviceOrFile.pop_back();
	}
	
	// Retrieve or compute calibration
	if ( Goban.GetCalibration( RecordingDeviceOrFile, Vid ) == false )
	{
		// We did not get calibration, exit!
		return -1;
	}

	// SubImage for image processing and video
	cv::Rect SubImageRect = Goban.GetSubImageRect();

	cv::Rect VideoRect = SubImageRect;
	int ScaleOutputImage = 1;

	// Do we rescale output?
	if ( AutomaticRescaleOutput == true )
	{
		if ( VideoRect.height < 200 || VideoRect.width < 200 )
		{
			ScaleOutputImage = 2;
			VideoRect.width *= 2;
			VideoRect.height *= 2;
		}
	}
	
	// To create output video
	VideoIO WriteVideo;
	cv::Mat VideoImage( VideoRect.height, VideoRect.width*2, CV_8UC3 );
	cv::Mat PlaceImage( VideoImage, cv::Rect( VideoRect.width, 0, VideoRect.width, VideoRect.height ) );
	cv::Mat GobanImage( VideoImage, cv::Rect( 0, 0, VideoRect.width, VideoRect.height )  );
	Goban.GameState.DrawGoban( GobanImage );

	if ( ExportResultVideo == true )
	{
		// create Video Name according to input name
		Omiscid::SimpleString VideoFilename =  OutputFolderName + Date + "." + Time + ".mp4";

		if ( WriteVideo.Create( VideoFilename.GetStr(), VideoImage.cols, VideoImage.rows, 25, "-y -codec:v libx264 -profile:v high -preset slow -b:v 500k -maxrate 500k -bufsize 1000k -threads 0 -pix_fmt yuv420p" ) == false )
		{
			fprintf( stderr, "Could not create ouput video, abording..." );
			return -2;
		}
	}
	
	// As calibration could take a long time, use new image as init
	// Goban *must* be empty
	cv::Mat LoadImage;
	cv::Mat DepthImage;

	if ( Vid.ReadFrame(LoadImage, DepthImage) == false )
	{
		fprintf( stderr, "Could not read init frame, abording..." );
		return -3;
	}

	// Init undelying detection algorithm
	if ( DepthImage.empty() == false )
	{
		// Depth is present, use it as motion detector
		Goban.InitDetection(DepthImage);
	}
	else
	{
		Goban.InitDetection(LoadImage);
	}

	// Set loop parameters and statistics
	int NumFrame = 1;						// Frame number
	double CurTime = 0.000;					// Current video/stream time

	bool ShowFeedback = true;				// Do we show feedback?
	bool Paused = false;					// Pause?

	bool ShowStoneDetection	 = false;		// Show underlying stone detection?
	bool ShowMotion = false;				// Show detection?

	ProcessingStatistics ProcStats(10.0f);	// Report Stats every 10s

	// GenerateSGF into OutputFolderName folder
	if ( Goban.GameState.SGFWriter.Open( OutputFolderName, EventName, RoundName, Rule, Komi, Date, Time, BlackPlayerName, WhitePlayerName ) == false )
	{
		fprintf( stderr, "Could not open output SGF file, abording...\n" );
		return -4;
	}

	#define MainWindoName "Processing... ESC to terminate game recording."

	fprintf( stderr, "Start processing!\n" );
	while( Vid.ReadFrame(LoadImage, DepthImage) ) 
	{
		// Get frame timestamp
		CurTime = Vid.GetTimestamp();

		// cv::imshow( "LoadImage", LoadImage );
#ifdef GO_CAM_KINECT_VERSION
		if ( DepthImage.empty() == false ) { cv::imshow( "Depth", DepthImage ); }
#endif

		// Deal with closing window: when a user close the window, when using imshow, only the
		// image is present, there is no more the trackbar. One way in c++ to deal with that is
		// to ask for track bar. If the track bar could not be retrieve (the window or the trackbar
		// is not present), you know that the Window has never been opened or has been closed.
		// Thus, create it again and add trackbar.
		if ( getTrackbarPos("Black Threshold", MainWindoName) == -1 )
		{
			cv::namedWindow( MainWindoName );
			cv::createTrackbar( "Black Threshold", MainWindoName, &SingleConfig.BlackThreshold, MaxThresholdForColorDetection );
			cv::createTrackbar( "White Threshold", MainWindoName, &SingleConfig.WhiteThreshold, MaxThresholdForColorDetection );
#ifdef GO_CAM_KINECT_VERSION
			if ( DepthImage.empty() == false ) { cv::createTrackbar( "Motion", MainWindoName, &SingleConfig.MotionThreshold, 150 ); }
#endif
		}

		// Process current frame, draw feedback on video if feddback is active or video is exported
		double FrameProcessingTime = Goban.ProcessCurrentFrame(LoadImage, DepthImage, CurTime, SingleConfig.BlackThreshold, SingleConfig.WhiteThreshold,
				ShowFeedback == true || ExportResultVideo == true, ShowStoneDetection, ShowMotion );

		if ( ShowFeedback == true || ExportResultVideo == true )
		{
			// copy it only if we need to draw feedback
			if ( ScaleOutputImage == 1 )
			{
				cv::Mat( LoadImage, SubImageRect ).copyTo( PlaceImage );
			}
			else
			{
				cv::resize( cv::Mat( LoadImage, SubImageRect ), PlaceImage, cv::Size( VideoRect.width, VideoRect.height ) );
			}
		}

		// Always draw game state
		Goban.GameState.DrawCurrentState( GobanImage, CurTime );

		// Write current frame to video
		if ( ExportResultVideo == true )
		{	
			WriteVideo.WriteFrame( VideoImage );
		}

		// Shall we show feedback?
		if ( ShowFeedback == false )
		{
			// Show only goban
			cv::imshow( MainWindoName, GobanImage );
			char KeyPressed = 0;

			KeyPressed = cv::waitKey( 1 );		// 1 means 1ms, fatest way to process

			// Show feedback?
			if ( KeyPressed == 's' || KeyPressed == 'S' )
			{
				// Invert feedback
				ShowFeedback = true;

				// Destroy window to resize trackbar
				cv::destroyWindow(MainWindoName);

				cv::waitKey(1);
			}

			// Show detection
			if ( KeyPressed == 'd' || KeyPressed == 'D' )
			{
				if ( ShowStoneDetection == true )
				{
					ShowStoneDetection = false;

					// destroy unused windows
					cv::destroyWindow(WhiteDetectionWindowName);
					cv::destroyWindow(BlackDetectionWindowName);
				}
				else
				{
					ShowStoneDetection = true;
				}
			}

			// Show motion detection
			if ( KeyPressed == 'm' || KeyPressed == 'M' )
			{
				if ( ShowMotion == true )
				{
					ShowMotion = false;
					cv::destroyWindow(MotionDetectionWindowsName);
				}
				else
				{
					ShowMotion = true;
				}
			}

			// SPACE will toggle pause mode
			if ( KeyPressed == ' ' )
			{
				Paused = !Paused;
			}
		}

		if ( ShowFeedback == true )
		{
			cv::imshow( MainWindoName, VideoImage );
			char KeyPressed = 0;

			for(;;)
			{
				KeyPressed = cv::waitKey( 1 );		// 1 means 1ms, fatest way to process without blocking mode

				// ESC stop processing
				if ( KeyPressed == 27 )
				{
					break;
				}

				// Show feeback?
				if ( KeyPressed == 's' || KeyPressed == 'S' )
				{
					// Invert feedback
					ShowFeedback = false;

					// Destroy window to resize trackbar
					cv::destroyWindow(MainWindoName);

					// destroy unused windows
					cv::destroyWindow( WhiteDetectionWindowName );
					cv::destroyWindow( BlackDetectionWindowName );

					cv::waitKey(1);
				}

				// Show detection
				if ( KeyPressed == 'd' || KeyPressed == 'D' )
				{
					if ( ShowStoneDetection == true )
					{
						ShowStoneDetection = false;

						// destroy unused windows
						cv::destroyWindow( WhiteDetectionWindowName );
						cv::destroyWindow( BlackDetectionWindowName );
					}
					else
					{
						ShowStoneDetection = true;
					}
				}

				// Show motion detection?
				if ( KeyPressed == 'm' || KeyPressed == 'M' )
				{
					if ( ShowMotion == true )
					{
						ShowMotion = false;
						cv::destroyWindow( MotionDetectionWindowsName );
					}
					else
					{
						ShowMotion = true;
					}
				}

				// SPACE will toggle pause mode
				if ( KeyPressed == ' ' )
				{
					Paused = !Paused;
				}

				// When not paused : go back to processing
				// In pause mode, 'x' or 'X' will process the next frame
				if ( Paused == false || KeyPressed == 'x' || KeyPressed == 'X' )
				{
					break;
				}
			}

			// ESC break the global loop
			if ( KeyPressed == 27 )
			{
				break;
			}

			// A new frame was processed
			NumFrame += 1;

			ProcStats.UpdateAndReportStats( NumFrame, FrameProcessingTime, stderr );
		}
	}

	// Close every window left
	cv::destroyAllWindows();

	// Ask score
	Omiscid::SimpleString Result;
	CheckAndSetVariable( Result, "\n\nEnter game result", "" );

	// Close file
	Goban.GameState.SGFWriter.Close(Result);

	if ( ExportResultVideo == true )
	{
		WriteVideo.Close();
	}

	// Save config
	SingleConfig.Save();

	// Everything went fine
	return 0;
}

