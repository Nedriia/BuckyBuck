//
// Created by arthu on 03/11/2026.
//

#ifndef BUCKYBUCK_TIMEMANAGER_H
#define BUCKYBUCK_TIMEMANAGER_H
#include <chrono>

typedef std::chrono::nanoseconds nanoseconds;
class TimeManager
{
	public:

	TimeManager(){};
	~TimeManager(){};

	static void HandleTime( const std::chrono::steady_clock::time_point& start );

	static const double* GetTimeLastFrame() { return &s_iTimeLastFrame; }
	static nanoseconds* GetRefreshTick() { return &s_iCurrentTick; }

	static void SetRefreshTick( const double& iTick );
	static bool IsFrameDirty() { return s_bDirtyFrame; }
	static void SetFrameAsDirty() { s_bDirtyFrame = true; }
	private:
	static nanoseconds  s_iAccumulator;
	static nanoseconds	s_iCurrentTick;
	static double		s_iTimeLastFrame;
	static bool			s_bDirtyFrame;
};


#endif //BUCKYBUCK_TIMEMANAGER_H
