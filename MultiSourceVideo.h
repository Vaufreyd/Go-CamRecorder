/**
 * @file MultiSourceVideo.cpp
 * @ingroup Go-CamRecorder
 * @author Dominique Vaufreydaz, personnal project
 * @copyright All right reserved.
 */

#ifndef __MULTI_SOURCE_VIDEO_H__
#define __MULTI_SOURCE_VIDEO_H__

#include "Go-CamRecorder.h"

#include <System/ElapsedTime.h>

#include "DataManagement/VideoIO.h"
#include "DataManagement/TimestampTools.h"

#ifdef GO_CAM_KINECT_VERSION
	#include "Kinect/KinectSensor.h"
#endif

/**
 * @class MultiVideoSource 
 * @brief Class to get images from device (using Opencv), file (using ffmpeg) or Kinect
 */
class MultiVideoSource 
#ifdef GO_CAM_KINECT_VERSION
	: public MobileRGBD::Kinect1::KinectSensor, public Omiscid::ReentrantMutex
#endif
{
protected:
// Open cv live capture
	cv::VideoCapture OpenCV_VideoCapture;					// for live recording using a webcam

// File input
	VideoIO VideoReader;									// Data management video reader, use ffmpeg executable as input tool for video reading, better support that opencv more over under Windows

	// Fps computation
	unsigned int NumberOfFrame;								// Number of frame read on the source
	Omiscid::PerfElapsedTime TotalTime;						// Total recording or reading time

	enum { Unk_Mode, Live_Mode, File_Mode, Kinect1_Mode };	// Mode enum for selecting source
	int Mode;												// Current source mode
	bool IsOpened;											// Flag when device is opened
	double LastFrameTimestamp;								// Timestamp of last frame

#ifdef GO_CAM_KINECT_VERSION
public:
	cv::Mat DepthImage;										// Last depth image sent by Kinect device
	cv::Mat RGBImage;										// LAst RGB image sent by Kinect device

protected:
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
	virtual void ProcessColorFrame(void * Buffer, unsigned int BufferSize, int Width, int Height, ImgType FrameType, int NumFrame, const struct timeb& FrameTimestamp, TIMESPAN InternalFrameTime);

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
	virtual void ProcessDepthFrame(void * Buffer, unsigned int BufferSize, int Width, int Height, ImgType FrameType, int NumFrame, const struct timeb& FrameTimestamp, TIMESPAN InternalFrameTime);

	/**
	* @brief Function to wait for the first ~synchronous RGB1/Depth images
	* @param MaxTries [in] Number of tries
	* @param SleepTimeInLoop [in] Sleep time for each time (in ms)
	* @return True if frames are available
	*/
	bool WaitForFrames(int MaxTries, int SleepTimeInLoop);

#endif

public:
	
	/**
    * @brief Constructor
	*/
	MultiVideoSource();

	/**
	* @brief Virtual destructor
	*/
	virtual ~MultiVideoSource();

	const int MaxDeviceNum = 20;			// Max device num for Opencv devices

	/**
	* @brief Open an input source
	* @param [in] Input name for the source. Is if is a number, it will be used as Opencv Number. If it equlas to "kinect1:",
			the kinect device will be used (if compiled with kinect mode active). If a file name is provided, it will be open.
	* @return true if the device was opened.
	*/
	bool Open( const char * InputName );

	/**
	* @brief Is the source a live source?
	* @return true if the source is a live one.
	*/
	inline bool IsInLiveMode()
	{
		return ( Mode == Live_Mode || Mode == Kinect1_Mode );
	}

	/**
	* @brief Is the source opened.
	* @return true if the source opened.
	*/
	inline bool IsOpen()
	{
		return IsOpened;
	}

	/**
	* @brief Close source (if opened).
	*/
	void Close();

	/**
	* @brief Get Frame per second count. Either file FPS or actual FPS for live source.
	*/
	double GetFPS();

	/**
	* @brief Get timestamp of the last frame
	*/
	inline double GetTimestamp()
	{
		return LastFrameTimestamp;
	}
	
	/**
	* @brief Read frames. VideoImg is read from RGB source. In case of kinect usage, DepthImg will be filled also (remain untouch when not using kinect).
	* @param VideoImg [out] BGR Image for Opencv processing
	* @param DepthImg [out] Depth Image for Opencv processing
	* @return true if frame(s) has been retrieved
	*/
	bool ReadFrame( cv::Mat& VideoImg, cv::Mat& DepthImg );
};

#endif