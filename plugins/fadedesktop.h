   /**
 *
 * Compiz fade to desktop plugin
 *
 * fadedesktop.cpp
 *
 * Copyright (c) 2021-2023 Robert Carr <racarr@beryl-project.org>
 *                       Danny Baumann <maniac@beryl-project.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 **/

#include "fadedesktop.h"

COMPIZ_PLUGIN_20090315 (fadedesktop, FadedesktopPluginVTable);

void
FadedesktopScreen::activateEvent (bool activating)
{
    CompOption::Vector o;

    o.push_back (CompOption ("root", CompOption::TypeInt));
    o.push_back (CompOption ("active", CompOption::TypeBool));

    o[0].value (). set ((int) screen->root ());
    o[1].value (). set (activating);

    screen->handleCompizEvent ("fadedesktop", "activate", o);
}

bool
FadedesktopWindow::isFadedesktopWindow ()
{
    FD_SCREEN (screen);

    if (!window->managed ())
        return false;

    if (window->grabbed ())
        return false;

    if (window->wmType () & (CompWindowTypeDesktopMask |
                             CompWindowTypeDockMask))
        return false;

    if (window->state () & CompWindowStateSkipPagerMask)
        return false;

    if (!fs->optionGetWindowMatch ().evaluate (window))
        return false;

    return true;
}

void
FadedesktopScreen::preparePaint (int msSinceLastPaint)
{
    fadeTime -= msSinceLastPaint;
    if (fadeTime < 0)
        fadeTime = 0;

    if (state == Out || state == In)
    {
        for (auto& w : screen->windows ())
        {
            bool doFade;

            FD_WINDOW (w);

            if (state == Out)
                doFade = fw->fading && w->inShowDesktopMode ();
            else
                doFade = fw->fading && !w->inShowDesktopMode ();

            if (doFade)
            {
                float windowFadeTime;

                if (state == Out)
                    windowFadeTime = fadeTime;
                else
                    windowFadeTime = optionGetFadetime () - fadeTime;

                fw->opacity = fw->cWindow->opacity () *
                              (windowFadeTime / optionGetFadetime ());
            }
        }
    }

    cScreen->preparePaint (msSinceLastPaint);
}

void
FadedesktopScreen::donePaint ()
{
    if (state == Out || state == In)
    {
        if (fadeTime <= 0)
        {
            bool isStillSD = false;

            for (auto& w : screen->windows ())
            {
                FD_WINDOW (w);

                if (fw->fading)
                {
                    if (state == Out)
                    {
                        w->hide ();
                        fw->isHidden = true;
                    }
                    fw->fading = false;
                }
                if (w->inShowDesktopMode ())
                    isStillSD = true;
            }

            if
