/**
 * @file Go-CamRecorder.h
 * @ingroup Go-CamRecorder
 * @author Dominique Vaufreydaz, personnal project
 * @copyright All right reserved.
 */

#ifndef __GO_CAM_RECORDER_H__
#define __GO_CAM_RECORDER_H__

#include <Messaging/Serializable.h>
#include <System/MemoryBuffer.h>

#include <opencv2/video/tracking.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/background_segm.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2//imgproc/imgproc.hpp>

#define MaxNumCells 19							// Max goban size, square for the moment
#define MaxThresholdForColorDetection 100		// Max value set to 100 (thus percentage are used)
#define CentralValueForBlackDetection 60		// Start threshold for black detection. May be overrided in config file.
#define CentralValueForWhiteDetection 180		// Start threshold for white detection. May be overrided in config file.

#define ConfigFileName	"Go-CamRecorder.json"	// Name of config file.
#define DefaultKomi		"7.5"					// Default Komi value

/**
 * @class GobanState 
 * @brief Class to manage current state of the game
 */
class ConfigInfo : public Omiscid::Serializable
{
public:
	// As we do not want to type event name each time
	Omiscid::SimpleString EventName;									// Default event name (i.e. competition), empty if not defined

	int BlackThreshold				= MaxThresholdForColorDetection/2;	// Last threshold for Black
	int WhiteThreshold				= MaxThresholdForColorDetection/2;	// Last threshold for White stone
	int MotionThreshold				= 50;								// motion threshold
	Omiscid::SimpleString UploadWebSite;								// Website to upload to
	Omiscid::SimpleString UploadURL;									// URL within the website for to upload file

	/**
    * @brief Constructor
	*/
	ConfigInfo()
	{
		// If not found, no problem, get default (or current) value
		// thus, let's allow partial unserialization
		PartialUnserializationAllowed = true;
	}

	/**
	* @brief Virtual destructor
	*/
	virtual ~ConfigInfo() {}

	/**
	* @brief Virtual function to declare serialization
	*/
	virtual void DeclareSerializeMapping()
	{
		AddVarToSerialization(EventName);
		AddVarToSerialization(BlackThreshold);
		AddVarToSerialization(WhiteThreshold);
		AddVarToSerialization(UploadWebSite);
		AddVarToSerialization(UploadURL);
	}

	/**
	* @brief Load values from the ConfigFileName file.
	* @return True if ok.
	*/
	bool Load();

	/**
	* @brief Save values to the ConfigFileName file.
	* @return True if ok.
	*/
	bool Save();
};

extern ConfigInfo SingleConfig;		// Unique and global instance of ConfigInfo
	
#endif
