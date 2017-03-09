/**
 * @file MultiSourceVideo.cpp
 * @ingroup Go-CamRecorder
 * @author Dominique Vaufreydaz, personnal project
 * @copyright All right reserved.
 */

#include "MultiSourceVideo.h"


#ifdef GO_CAM_KINECT_VERSION

using namespace MobileRGBD;

/**
* @brief Overload callback to get Color (RGBA) frame from Kinect
* @param Buffer [in] Raw data buffer
* @param BufferSize [in] Size of Buffer
* @param Width [in] Width of the image
* @param Height [in] Height of the image
* @param ImgType [in] Type of data (here RGBA)
* @param NumFrame [in] Frame number
* @param FrameTimestamp [in] Frame timestamp (received time by this application)
* @param InternalFrameTime [in] Internal frame timestamp from the kinect
*/
/* virtual */ void MultiVideoSource::ProcessColorFrame(void * Buffer, unsigned int BufferSize, int Width, int Height, ImgType FrameType, int NumFrame, const struct timeb& FrameTimestamp, TIMESPAN InternalFrameTime)
{
	Omiscid::SmartLocker SD_Lock(*this);
	cv::cvtColor( cv::Mat( Height, Width, CV_8UC4, Buffer ), RGBImage, CV_BGRA2BGR );
	// fprintf( stderr, "%s\n", type2str(RGBImage.type()).GetStr());
}

/**
* @brief Overload callback to get Depth frame from Kinect
* @param Buffer [in] Raw data buffer
* @param BufferSize [in] Size of Buffer
* @param Width [in] Width of the image
* @param Height [in] Height of the image
* @param ImgType [in] Type of data (here UINT16)
* @param NumFrame [in] Frame number
* @param FrameTimestamp [in] Frame timestamp (received time by this application)
* @param InternalFrameTime [in] Internal frame timestamp from the kinect
*/
/* virtual */ void MultiVideoSource::ProcessDepthFrame(void * Buffer, unsigned int BufferSize, int Width, int Height, ImgType FrameType, int NumFrame, const struct timeb& FrameTimestamp, TIMESPAN InternalFrameTime)
{
	Omiscid::SmartLocker SD_Lock(*this);
	cv::Mat( Height, Width, CV_16UC1, Buffer ).copyTo(DepthImage);
}

/**
* @brief Function to wait for the first ~synchronous RGB1/Depth images
* @param MaxTries [in] Number of tries
* @param SleepTimeInLoop [in] Sleep time for each time (in ms)
* @return True if frames are available
*/
bool MultiVideoSource::WaitForFrames(int MaxTries, int SleepTimeInLoop)
{
	// Multiple tries if we reach this point before received the frames
	for( int NbTries = 0; NbTries < MaxTries; NbTries++ )
	{
		Omiscid::SmartLocker SL_this(*this);
		if ( RGBImage.empty() == false && DepthImage.empty() == false )
		{
			return true;
		}

		Omiscid::Thread::Sleep(SleepTimeInLoop);	// 100 ms
	}
	return false;
}

#endif

/**
* @brief Constructor
*/
MultiVideoSource::MultiVideoSource()
{
	NumberOfFrame = 0;
	Mode = Unk_Mode;
	IsOpened = false;
	LastFrameTimestamp = 0.0;
}

/**
* @brief Virtual destructor
*/
/* virtual */ MultiVideoSource::~MultiVideoSource()
{
#ifdef GO_CAM_KINECT_VERSION
	StopThread(5000);
	Release();

#endif
};

/**
* @brief Open an input source
* @param [in] Input name for the source. Is if is a number, it will be used as Opencv Number. If it equlas to "kinect1:",
		the kinect device will be used (if compiled with kinect mode active). If a file name is provided, it will be open.
* @return true if the device was opened.
*/
bool MultiVideoSource::Open( const char * InputName )
{
	// Check if it is a device number
	int DeviceNum = -1;
	int NumberOfCharRead = 0;
	if ( sscanf( InputName, "%d%n", &DeviceNum, &NumberOfCharRead ) == 1 )
	{
		// ok, a number start
		if ( DeviceNum >= 0 && DeviceNum <= MaxDeviceNum && NumberOfCharRead == strlen(InputName) )
		{
			// Ok, found a probable device number, try to open it
			if ( OpenCV_VideoCapture.open(DeviceNum) == true )
			{
				// Check if we can ask for RGB frames only
				// if ( OpenCV_VideoCapture.set(CV_CAP_PROP_CONVERT_RGB, 1.0) == true )
				{
					fprintf( stderr, "Device %d openned as input source\n", DeviceNum );
					IsOpened = true;
					Mode = Live_Mode;
					TotalTime.Reset();
					NumberOfFrame = 0;
					return true;
				}
				fprintf( stderr, "Unable to ask for RGB frames from device %d.\n", DeviceNum );
			}
			else
			{
				fprintf( stderr, "Could not open device %d as input source\n", DeviceNum );
			}
			// Bad case
			IsOpened = false;
			Mode = Unk_Mode;
			NumberOfFrame = 0;
			LastFrameTimestamp = 0.0;
			return false;
		}
	}

	if ( strcasecmp("kinect1:", InputName) == 0 )
	{
#ifndef GO_CAM_KINECT_VERSION
		fprintf( stderr, "Kinect1 not supported. Recompile with '-DUSE_KINECT:BOOLEAN=TRUE' cmake option.\n" );
		return false;
#else
		fprintf( stderr, "Try to open Kinect1 in near mode... " );
		SetNearMode(true);
		if ( Init( FrameSourceTypes_Color | FrameSourceTypes_Depth ) == false )
		{
			fprintf( stderr, "Error when Init Kinect1 device.\n" );
			// Bad case
			IsOpened = false;
			Mode = Unk_Mode;
			NumberOfFrame = 0;
			LastFrameTimestamp = 0.0;
			return false;
		}

		IsOpened = true;
		Mode = Kinect1_Mode;
		TotalTime.Reset();
		NumberOfFrame = 0;
		fprintf( stderr, "Device opened. Wait for first frames... " );
		if ( WaitForFrames(100, 100) == true )
		{
			fprintf( stderr, "Device ready.\n" );
			return true;
		}
		fprintf( stderr, "Device not ready, abording.\n" );
		return false;
#endif
	}

	// Try to open the name a a usual file
	if ( VideoReader.Open(InputName, "" ) == true ) // if we want to start at 3:55n change to '"-ss 00:03:55" ) == true )'
	{
			fprintf( stderr, "File '%s' openned as input source\n", InputName );
			IsOpened = true;
			Mode = File_Mode;
			TotalTime.Reset();
			NumberOfFrame = 0;
			LastFrameTimestamp = 0.0;
			return true;
	}
		
	fprintf( stderr, "Could not open file '%s' openned as input source\n", InputName );
	IsOpened = false;
	Mode = Unk_Mode;
	NumberOfFrame = 0;
	LastFrameTimestamp = 0.0;
	return false;
}

/**
* @brief Close source (if opened).
*/
void MultiVideoSource::Close()
{
	switch(Mode)
	{
		case Live_Mode:
			OpenCV_VideoCapture.release();
			break;

		case File_Mode:
			VideoReader.Close();
			break;

	}
	IsOpened = false;
	Mode = Unk_Mode;
}

/**
* @brief Get Frame per second count. Either file FPS or actual FPS for live source.
*/
double MultiVideoSource::GetFPS()
{
	switch(Mode)
	{
		case Live_Mode:
		case Kinect1_Mode:
			return (double)NumberOfFrame/TotalTime.GetInSeconds();

		case File_Mode:
			return VideoReader.Fps;
	}
	return 0.0;
}

/**
* @brief Read frames. VideoImg is read from RGB source. In case of kinect usage, DepthImg will be filled also (remain untouch when not using kinect).
* @param VideoImg [out] BGR Image for Opencv processing
* @param DepthImg [out] Depth Image for Opencv processing
* @return true if frame(s) has been retrieved
*/
bool MultiVideoSource::ReadFrame( cv::Mat& VideoImg, cv::Mat& DepthImg )
{
	switch(Mode)
	{
		case Live_Mode:
			if ( OpenCV_VideoCapture.retrieve( VideoImg ) == true )
			{
				LastFrameTimestamp = TotalTime.GetInSeconds();
				return true;
			}
			return false;

		case File_Mode:
			if ( VideoReader.ReadFrame( VideoImg ) == true )
			{
				LastFrameTimestamp += 1.0/VideoReader.Fps;
				return true;
			}
			return false;

		case Kinect1_Mode:
			{
#ifdef GO_CAM_KINECT_VERSION
				// Multiple tries (max 33 tries of 30 ms) if we reach this point before received the frames
				if ( WaitForFrames(33, 30) == true )
				{
					Omiscid::SmartLocker SL_this(*this);
					if ( RGBImage.empty() == false && DepthImage.empty() == false )
					{
						RGBImage.copyTo(VideoImg);
						cv::flip(VideoImg, VideoImg, 1);
						DepthImage.copyTo(DepthImg);
						cv::flip(DepthImg, DepthImg, 1);
						LastFrameTimestamp = TotalTime.GetInSeconds();
						return true;
					}
				}
#endif
				return false;
			}

		default:
			throw "No file opened at this time!!";
	}

	// to make compiler happy
	return false;
}