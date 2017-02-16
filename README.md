# Go-CamRecorder

Go-CamRecorder is a multiplatform recording tools based on OpenCV and the Omiscid Middleware written in C++.
It records Go games into sgf files using a simple Webcam or a Kinect on Linux/Windows (Tests were not done on MacOSX).
For the first version, only Kinect1 device on Windows is supported. Plan for next version is to include Kinect1 and Kinect2,
on Linux and Windows. 

## Short explanation

The current version of Go-CamRecorder works on an association with a camera/kinect and a goban. Up to now, the goban must not move
during recording. After a quick camera calibration phase, the software can track stones and produces an SGF file.
To do so, it computes artificial projection of stones all over the goban and creates for each one a so-called StoneDetector.

Go-CamRecorder can handle 9x9, 13x13 or 19x19 gobans. 
The size limitation is done while parsing program arguments. 
Actually, the source code can process every square goban with a size > 5.

### Calibration

The calibration phase consists in computing camera parameters. Placing 17 points on the image permits to compute projection parameters.
Stone in 3D can be projected in 2D camera space to place and size every stone detector on the goban.
The following images show how to placement of the 17 points on the goban. The [_aa_] position is in the left upper corner.

![Calibration points](/Images/CalibrationExample.png)

After computation of camera parameters, Go-CamRecorder shows the result to the user for validation. On the next image, blue spots
represent centers of stone detectors, the blue area is the processing zone.

![Calibration result](/Images/CalibrationResult.png) 

Calibration results are stored. If the camera/goban position does not change, there is no need to redo calibration.

### Online processing

During online processing, the first step is to detect motion. Motion is detected by image change when using webcam.
It is done by background detection when using Kinect. When motion is detected, stones are searched on the goban.
Next images depict stone detectors over the goban, black detection and white detection.

#### Stone detectors
![Detection area](/Images/DetectionArea.png)
#### Black detection
![Black detection](/Images/BlackDetection.png) 
#### White detection
![White detection](/Images/WhiteDetection.png) 

## Examples

Examples can be found on the [Go-CamRecorder YouTube channel](https://www.youtube.com/channel/UCmsQVrwGb3ARL4KsHE5NVpA).

Here is a screen capture of Go-CamRecorder while processing 
a Game (thank you to the player/recorder of this session). At left, you can see the goban state as detected
using computer vision by Go-CamRecorder 1.0a. At right, the processing frames captured using a webcam. There is an integration time between both views. It permits to deal with false stone detection, false stone loss, etc. Red squares represent motion detection on each stone detector on the goban. Black ellipses with blue border are detected black stones; white ellipses with black border are white detected stones. When visual perception failed, for instance when an arm hides a move and thus stones are detected in the wrong order, a comment is inserted in the SGF file to help humans correct the SGF file.

![White detection](Images/Go-CamRecorder_in_action.png).

The [SGF file produced by Go-CamRecorder from this video is available in the SGF_Examples folder of the repo](https://github.com/Vaufreyd/Go-CamRecorder/blob/master/SGF_Examples/2017-02-16.17-20_2612040790.sgf).

## Source code availability

A test in the wild of the software will be done at [the European Youth Go Championships 2017](http://eygc2017.jeudego.org/), in Grenoble. Release of the source code will follow this experiment.

