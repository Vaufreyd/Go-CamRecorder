/**
 * @file StoneState.h
 * @ingroup Go-CamRecorder
 * @author Dominique Vaufreydaz, personnal project
 * @copyright All right reserved.
 */

#ifndef __STONE_STATE_H__
#define __STONE_STATE_H__

/**
 * @class StoneState 
 * @brief Class containing information about stone (enum only for the moment).
 */
class StoneState 
{
public:
	// warning: Empty is not 0!
	enum { White = 0, Black = 1, Empty = 2, StateModulo = 2 };
};

#endif // __STONE_STATE_H__
