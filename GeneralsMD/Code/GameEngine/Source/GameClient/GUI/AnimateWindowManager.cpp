/*
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

////////////////////////////////////////////////////////////////////////////////
//																																						//
//  (c) 2001-2003 Electronic Arts Inc.																				//
//																																						//
////////////////////////////////////////////////////////////////////////////////

// FILE: AnimateWindowManager.cpp /////////////////////////////////////////////////
//-----------------------------------------------------------------------------
//
//                       Electronic Arts Pacific.
//
//                       Confidential Information
//                Copyright (C) 2002 - All Rights Reserved
//
//-----------------------------------------------------------------------------
//
//	created:	Mar 2002
//
//	Filename: 	AnimateWindowManager.cpp
//
//	author:		Chris Huybregts
//
//	purpose:	This will contain the logic behind the different animations that
//						can happen to a window.
//
//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------------
// SYSTEM INCLUDES ////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// USER INCLUDES //////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------
#include "PreRTS.h"	// This must go first in EVERY cpp file in the GameEngine

#include "GameClient/AnimateWindowManager.h"
#include "GameClient/GameWindow.h"
#include "GameClient/Display.h"
#include "GameClient/ProcessAnimateWindow.h"
//-----------------------------------------------------------------------------
// DEFINES ////////////////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// AnimateWindow PUBLIC FUNCTIONS /////////////////////////////////////////////
//-----------------------------------------------------------------------------
namespace wnd
{
AnimateWindow::AnimateWindow()
{
	m_delay = 0;
	m_startPos.x = m_startPos.y = 0;
	m_endPos.x = m_endPos.y = 0;
	m_curPos.x = m_curPos.y = 0;
	m_win = nullptr;
	m_animType = WIN_ANIMATION_NONE;

	m_restPos.x = m_restPos.y = 0;
	m_vel.x = m_vel.y = 0.0f;
	m_needsToFinish = FALSE;
	m_isFinished = FALSE;
	m_endTime = 0;
	m_startTime = 0;
}
AnimateWindow::~AnimateWindow()
{
	m_win = nullptr;
}

void AnimateWindow::setAnimData( 	ICoord2D startPos, ICoord2D endPos,
																	ICoord2D curPos, ICoord2D restPos,
																	Coord2D vel, UnsignedInt startTime,
																	UnsignedInt endTime )

{
	setStartPos(startPos);
	setEndPos(endPos);
	setCurPos(curPos);
	setRestPos(restPos);
	m_vel = vel;
	m_startTime = startTime;
	m_endTime = endTime;

}

ICoord2D AnimateWindow::getStartPos()	{ return TheDisplay->fractionToPixels(m_startPos); }
ICoord2D AnimateWindow::getCurPos()		{ return TheDisplay->fractionToPixels(m_curPos); }
ICoord2D AnimateWindow::getEndPos()		{ return TheDisplay->fractionToPixels(m_endPos); }
ICoord2D AnimateWindow::getRestPos()	{ return TheDisplay->fractionToPixels(m_restPos); }

void AnimateWindow::setStartPos( ICoord2D startPos )	{ m_startPos = TheDisplay->pixelsToFraction(startPos); }
void AnimateWindow::setCurPos( ICoord2D curPos )			{ m_curPos = TheDisplay->pixelsToFraction(curPos); }
void AnimateWindow::setEndPos( ICoord2D endPos )			{ m_endPos = TheDisplay->pixelsToFraction(endPos); }
void AnimateWindow::setRestPos( ICoord2D restPos )		{ m_restPos = TheDisplay->pixelsToFraction(restPos); }

static Bool isSamePixel( const ICoord2D &a, const ICoord2D &b )
{
	return a.x == b.x && a.y == b.y;
}

void AnimateWindow::anchorToWindow( const ICoord2D &windowPixels, const Coord2D &windowFraction )
{
	if( isSamePixel(getEndPos(), windowPixels) )
		m_endPos = windowFraction;

	if( isSamePixel(getRestPos(), windowPixels) )
		m_restPos = windowFraction;
}

void AnimateWindow::snapWindowToAnchor()
{
	ICoord2D windowPixels;
	m_win->winGetPosition(&windowPixels.x, &windowPixels.y);

	if( isSamePixel(windowPixels, getEndPos()) )
	{
		m_win->winSetFractionalPosition(m_endPos);
		return;
	}

	if( isSamePixel(windowPixels, getRestPos()) )
		m_win->winSetFractionalPosition(m_restPos);
}

void AnimateWindow::placeWindowAtRest()
{
	m_win->winSetFractionalPosition(m_restPos);
}

} // namespace wnd

//-----------------------------------------------------------------------------
// PUBLIC FUNCTIONS ///////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------

static void clearWinList(AnimateWindowList &winList)
{
	wnd::AnimateWindow *win = NULL;
	while (!winList.empty())
	{
		win = *(winList.begin());
		winList.pop_front();
		if (win)
			deleteInstance(win);
		win = NULL;
	}
}

AnimateWindowManager::AnimateWindowManager()
{
// we don't allocate many of these, so no MemoryPools used
	m_slideFromRight = NEW ProcessAnimateWindowSlideFromRight;
	m_slideFromRightFast = NEW ProcessAnimateWindowSlideFromRightFast;
	m_slideFromLeft = NEW ProcessAnimateWindowSlideFromLeft;
	m_slideFromTop = NEW ProcessAnimateWindowSlideFromTop;
	m_slideFromTopFast = NEW ProcessAnimateWindowSlideFromTopFast;
	m_slideFromBottom = NEW ProcessAnimateWindowSlideFromBottom;
	m_spiral = NEW ProcessAnimateWindowSpiral;
	m_slideFromBottomTimed = NEW ProcessAnimateWindowSlideFromBottomTimed;
	m_winList.clear();
	m_needsUpdate = FALSE;
	m_reverse = FALSE;
	m_winMustFinishList.clear();
}
AnimateWindowManager::~AnimateWindowManager()
{
	delete m_slideFromRight;
	delete m_slideFromRightFast;
	delete m_slideFromLeft;
	delete m_slideFromTop;
	delete m_slideFromTopFast;
	delete m_slideFromBottom;
	delete m_spiral;
	delete m_slideFromBottomTimed;

	m_slideFromRight = nullptr;
	resetToRestPosition();
	clearWinList(m_winList);
	clearWinList(m_winMustFinishList);
}


void AnimateWindowManager::init()
{
	clearWinList(m_winList);
	clearWinList(m_winMustFinishList);
	m_needsUpdate = FALSE;
	m_reverse = FALSE;
}

void AnimateWindowManager::reset()
{
	resetToRestPosition();
	clearWinList(m_winList);
	clearWinList(m_winMustFinishList);
	m_needsUpdate = FALSE;
	m_reverse = FALSE;
}

static Bool updateForward( ProcessAnimateWindow *processAnim, wnd::AnimateWindow *animWin )
{
	const Bool wasFinished = animWin->isFinished();
	const Bool isFinished = processAnim->updateAnimateWindow(animWin);

	if( !wasFinished && animWin->isFinished() )
		animWin->snapWindowToAnchor();

	return isFinished;
}

void AnimateWindowManager::update()
{

	ProcessAnimateWindow *processAnim = nullptr;

	// if we need to update the windows that need to finish, update that list
	if(m_needsUpdate)
	{
		AnimateWindowList::iterator it = m_winMustFinishList.begin();
		m_needsUpdate = FALSE;

		while (it != m_winMustFinishList.end())
		{
			wnd::AnimateWindow *animWin = *it;
			if (!animWin)
			{
				DEBUG_CRASH(("There's No AnimateWindow in the AnimateWindow List"));
				return;
			}
			processAnim = getProcessAnimate( animWin->getAnimType() );
			if(processAnim)
			{
				if(m_reverse)
				{
					if(!processAnim->reverseAnimateWindow(animWin))
						m_needsUpdate = TRUE;
				}
				else
				{
					if(!updateForward(processAnim, animWin))
						m_needsUpdate = TRUE;
				}
			}

			it ++;
		}
	}

	AnimateWindowList::iterator it = m_winList.begin();

	while (it != m_winList.end())
	{
		wnd::AnimateWindow *animWin = *it;
		if (!animWin)
		{
			DEBUG_CRASH(("There's No AnimateWindow in the AnimateWindow List"));
			return;
		}
		processAnim = getProcessAnimate( animWin->getAnimType() );
		if(m_reverse)
		{
			if(processAnim)
				processAnim->reverseAnimateWindow(animWin);
		}
		else
		{
			if(processAnim)
				updateForward(processAnim, animWin);
		}
		it ++;
	}
}


void AnimateWindowManager::registerGameWindow(GameWindow *win, AnimTypes animType, Bool needsToFinish, UnsignedInt ms, UnsignedInt delayMs)
{
	if(!win)
	{
		DEBUG_CRASH(("Win was null as it was passed into registerGameWindow... not good indeed"));
		return;
	}
	if(animType <= WIN_ANIMATION_NONE || animType >= WIN_ANIMATION_COUNT )
	{
		DEBUG_CRASH(("an Invalid WIN_ANIMATION type was passed into registerGameWindow... please fix me "));
		return;
	}

	// Create a new AnimateWindow class and fill in it's data.
	wnd::AnimateWindow *animWin = wnd::AnimateWindow::createNewInstance();
	animWin->setGameWindow(win);
	animWin->setAnimType(animType);
	animWin->setNeedsToFinish(needsToFinish);
	animWin->setDelay(delayMs);

	ICoord2D windowPixels;
	win->winGetPosition(&windowPixels.x, &windowPixels.y);
	Coord2D windowFraction;
	win->winGetFractionalPosition(&windowFraction);

	// Run the window through the processAnim's init function.
	ProcessAnimateWindow *processAnim = getProcessAnimate( animType );
	if(processAnim)
	{
		processAnim->setMaxDuration(ms);
		processAnim->initAnimateWindow( animWin );
	}
	animWin->anchorToWindow(windowPixels, windowFraction);

	// Add the Window to the proper list
	if(needsToFinish)
	{
		m_winMustFinishList.push_back(animWin);
		m_needsUpdate = TRUE;
	}
	else
		m_winList.push_back(animWin);
}

ProcessAnimateWindow *AnimateWindowManager::getProcessAnimate( AnimTypes animType )
{
	switch (animType) {
	case WIN_ANIMATION_SLIDE_RIGHT:
		{
			return m_slideFromRight;
		}
	case WIN_ANIMATION_SLIDE_RIGHT_FAST:
		{
			return m_slideFromRightFast;
		}
	case WIN_ANIMATION_SLIDE_LEFT:
	{
			return m_slideFromLeft;
	}
	case WIN_ANIMATION_SLIDE_TOP:
	{
			return m_slideFromTop;
	}
	case WIN_ANIMATION_SLIDE_BOTTOM:
	{
			return m_slideFromBottom;
	}
	case WIN_ANIMATION_SPIRAL:
	{
			return m_spiral;
	}
	case WIN_ANIMATION_SLIDE_BOTTOM_TIMED:
	{
		return m_slideFromBottomTimed;
	}
	case WIN_ANIMATION_SLIDE_TOP_FAST:
	{
		return m_slideFromTopFast;
	}
		default:
		return nullptr;
	}
}

void AnimateWindowManager::reverseAnimateWindow()
{

	m_reverse = TRUE;
	m_needsUpdate = TRUE;
	ProcessAnimateWindow *processAnim = nullptr;

	UnsignedInt maxDelay = 0;
	AnimateWindowList::iterator it = m_winMustFinishList.begin();
	while (it != m_winMustFinishList.end())
	{
		wnd::AnimateWindow *animWin = *it;
		if (!animWin)
		{
			DEBUG_CRASH(("There's No AnimateWindow in the AnimateWindow List"));
			return;
		}
		if(animWin->getDelay() > maxDelay)
			maxDelay = animWin->getDelay();
		it ++;
	}

	it = m_winMustFinishList.begin();
	while (it != m_winMustFinishList.end())
	{
		wnd::AnimateWindow *animWin = *it;
		if (!animWin)
		{
			DEBUG_CRASH(("There's No AnimateWindow in the AnimateWindow List"));
			return;
		}
		// Run the window through the processAnim's init function.
		 processAnim = getProcessAnimate( animWin->getAnimType() );
		if(processAnim)
		{
			processAnim->initReverseAnimateWindow( animWin, maxDelay );
		}

		animWin->setFinished(FALSE);
		it ++;
	}

	it = m_winList.begin();

	while (it != m_winList.end())
	{
		wnd::AnimateWindow *animWin = *it;
		if (!animWin)
		{
			DEBUG_CRASH(("There's No AnimateWindow in the AnimateWindow List"));
			return;
		}
		processAnim = getProcessAnimate( animWin->getAnimType() );

		if(processAnim)
			processAnim->initReverseAnimateWindow(animWin);
		animWin->setFinished(FALSE);
		it ++;
	}

}

void AnimateWindowManager::resetToRestPosition()
{

	m_reverse = TRUE;
	m_needsUpdate = TRUE;

	AnimateWindowList::iterator it = m_winMustFinishList.begin();
	while (it != m_winMustFinishList.end())
	{
		wnd::AnimateWindow *animWin = *it;
		if (!animWin)
		{
			DEBUG_CRASH(("There's No AnimateWindow in the AnimateWindow List"));
			return;
		}
		if(animWin->getGameWindow())
			animWin->placeWindowAtRest();
		it ++;
	}
	it = 	m_winList.begin();
	while (it != m_winList.end())
	{
		wnd::AnimateWindow *animWin = *it;
		if (!animWin)
		{
			DEBUG_CRASH(("There's No AnimateWindow in the AnimateWindow List"));
			return;
		}
		if(animWin->getGameWindow())
			animWin->placeWindowAtRest();
		it ++;
	}


}

//-----------------------------------------------------------------------------
// PRIVATE FUNCTIONS //////////////////////////////////////////////////////////
//-----------------------------------------------------------------------------



