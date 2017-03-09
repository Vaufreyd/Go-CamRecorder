/**
 * @file HistoryValue.h
 * @ingroup Go-CamRecorder
 * @author Dominique Vaufreydaz, personnal project
 * @copyright All right reserved.
 */


#ifndef __HISTORY_VALUE_H__
#define __HISTORY_VALUE_H__

/**
 * @class HistoryValue 
 * @brief Template class to associate a value to a timestamp
 */
template<typename TYPE>
class HistoryValue
{
public:
	/**
    * @brief Default constructor
	*/
	HistoryValue()
	{
		HValue = (TYPE)0;
		Timestamp = 0.0;
	}

	/**
    * @brief Constructor
	*/
	HistoryValue(TYPE NewVal, double timestamp)
	{
		HValue = (TYPE)NewVal;
		Timestamp = timestamp;
	}

	/**
    * @brief Copy constructor
	*/
	HistoryValue(const HistoryValue& NewHistValue )
	{
		HValue = NewHistValue.HValue;
		Timestamp = NewHistValue.Timestamp;
	}

	double Timestamp;			// Timestamp
	TYPE HValue;				// Associated value
};

#endif // __HISTORY_VALUE_H__