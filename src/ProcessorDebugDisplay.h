//
// Created by arthu on 23/09/2026.
//
#pragma once

#ifndef BUCKYBUCK_PROCESSORDEBUGDISPLAY_H
#define BUCKYBUCK_PROCESSORDEBUGDISPLAY_H


class ProcessorDebugDisplay
{
public:
	ProcessorDebugDisplay(){};
	~ProcessorDebugDisplay(){};

	void Update();
private:
	void _DrawFlagsState();
	void _DrawRegistersState();
};


#endif //BUCKYBUCK_PROCESSORDEBUGDISPLAY_H
