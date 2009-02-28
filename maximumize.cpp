/*
 * Compiz Fusion Maximumize plugin
 *
 * Copyright 2007-2008 Kristian Lyngstøl <kristian@bohemians.org>
 * Copyright 2008 Eduardo Gurgel Pinho <edgurgel@gmail.com>
 * Copyright 2008 Marco Diego Aurelio Mesquita <marcodiegomesquita@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Author(s):
 * Kristian Lyngstøl <kristian@bohemians.org>
 * Eduardo Gurgel <edgurgel@gmail.com>
 * Marco Diego Aurélio Mesquita <marcodiegomesquita@gmail.com>
 *
 * Description:
 *
 * Maximumize resizes a window so it fills as much of the free space in any
 * direction as possible without overlapping with other windows.
 *
 */

#include "maximumize.h"

COMPIZ_PLUGIN_20081216 (maximumize, MaximumizePluginVTable);

/* Returns true if rectangles a and b intersect by at least 40 in both
 * directions
 */
bool 
MaximumizeScreen::substantialOverlap (CompRect a,
				      CompRect b)
{
    if (a.x () + a.width () <= b.x () + 40) return false;
    if (b.x () + b.width () <= a.x () + 40) return false;
    if (a.y () + a.height () <= b.y () + 40) return false;
    if (b.y () + b.height () <= a.y () + 40) return false;
    return true;
}


/* Set the rectangle to cover the window, including decoration. */
CompRect
MaximumizeScreen::setWindowBox (CompWindow *w)
{
    CompRect rect;
    rect.setGeometry (w->serverX () - w->input ().left,
		      w->serverY () - w->input ().top,
		      w->serverWidth () + w->input ().right + w->input ().left,
		      w->serverHeight () + w->input ().top + w->input ().bottom);

    return rect;
}


/* Generates a region containing free space (here the
 * active window counts as free space). The region argument
 * is the start-region (ie: the output dev).
 * Logic borrowed from opacify (courtesy of myself).
 */

CompRegion
MaximumizeScreen::findEmptyRegion (CompWindow *window,
				   CompRegion region)
{
    CompRegion     newRegion (region);
    CompRect       windowRect;

    if (optionGetIgnoreOverlapping ()) 
	windowRect = setWindowBox (window);

    foreach(CompWindow *w, screen->windows ())
    {
	CompRegion tmpRegion = emptyRegion;
	CompRect   tmpRect;

        if (w->id () == window->id ())
            continue;

        if (w->invisible () /*|| w->hidden */|| w->minimized ())
            continue;

	if (w->wmType () & CompWindowTypeDesktopMask)
	    continue;

	if (w->wmType () & CompWindowTypeDockMask)
	{
	    if (w->struts ()) 
	    {
		tmpRegion = tmpRegion.united (CompRect (w->struts ()->left));
		tmpRegion = tmpRegion.united (CompRect (w->struts ()->right));
		tmpRegion = tmpRegion.united (CompRect (w->struts ()->top));
		tmpRegion = tmpRegion.united (CompRect (w->struts ()->bottom));
		newRegion = newRegion.subtracted (tmpRegion);

	    }
	    continue;
	}

	if (optionGetIgnoreSticky () && 
	    (w->state () & CompWindowStateStickyMask) &&
	    !(w->wmType () & CompWindowTypeDockMask))
	    continue;

	tmpRect = setWindowBox (w);

	if (optionGetIgnoreOverlapping () &&
	    substantialOverlap (tmpRect, windowRect))
	    continue;
	tmpRegion = tmpRegion.united (tmpRect);
	newRegion = newRegion.subtracted (tmpRegion);
    }

    return newRegion;
}

/* Returns true if box a has a larger area than box b.  */
bool
MaximumizeScreen::boxCompare (BOX a,
			      BOX b)
{
    int areaA, areaB;

    areaA = (a.x2 - a.x1) * (a.y2 - a.y1);
    areaB = (b.x2 - b.x1) * (b.y2 - b.y1);

    return (areaA > areaB);
}

/* While the rectangle has space, add inc to i. When it CHEKCREC fails (ie:
 * we overstepped our boundaries), reduce i by inc; undo the last change.
 * inc is either 1 or -1, but could easily be looped over for fun and
 * profit. (Ie: start with -100, hit the wall, go with -20, then -1, etc.)
 * 
 * NOTE: We have to pass along tmp, r and w for CHECKREC. 
 * FIXME:  
 */

/* If a window, with decorations, defined by tmp and w is still in free
 * space, evaluate to true. 
 */
#define CHECKREC \
	XRectInRegion (r.handle (), tmp->x1 - w->input ().left, tmp->y1 - w->input ().top,\
		       tmp->x2 - tmp->x1 + w->input ().left + w->input ().right,\
		       tmp->y2 - tmp->y1 + w->input ().top + w->input ().bottom)\
	    == RectangleIn
/*#define CHECKREC \
	r.contains (CompRect (tmp->x1 - w->input ().left, tmp->y1 - w->input ().top,\
			      tmp->x2 - tmp->x1 + w->input ().left + w->input ().right,\
			      tmp->y2 - tmp->y1 + w->input ().top + w->input ().bottom))*/
void
MaximumizeScreen::growGeneric (CompWindow      *w,
			       BOX	       *tmp,
			       CompRegion      r,
			       short int       *i,
			       const short int inc)
{
    bool touch = false;
    while(CHECKREC)
    {
	*i += inc;
	touch = true;
    }
    if (touch)
	*i -= inc;
}
#undef CHECKREC

/* Extends the given box for Window w to fit as much space in region r.
 * If XFirst is true, it will first expand in the X direction,
 * then Y. This is because it gives different results. 
 */
BOX
MaximumizeScreen::extendBox (CompWindow   *w,
			     BOX	  tmp,
			     CompRegion	  r,
			     bool	  xFirst,
			     const MaxSet mset)
{
    if (xFirst)
    {
	growWidth (w, &tmp, r, mset);
	growHeight (w, &tmp, r, mset);
    }
    else
    {
	growHeight (w, &tmp, r, mset);
	growWidth (w, &tmp, r, mset);
    }

    return tmp;
}

/* These two functions set the width and height respectively, with gravity
 * towards the center of the window. They will set the box-width to width
 * as long as at least one of the sides can be modified. Same for height.
 */

void
MaximumizeScreen::setBoxWidth (BOX          *box,
			       const int    width,
			       const MaxSet mset)
{
    int original = box->x2 - box->x1;
    int increment;

    if (!mset.left && !mset.right)
	return ;

    if (mset.left != mset.right)
	increment = original - width;
    else
	increment = (original - width) / 2;

    if (mset.left) 
	box->x1 += increment;
    if (mset.right)
	box->x2 -= increment;
}


void
MaximumizeScreen::setBoxHeight (BOX	     *box,
				const int    height,
				const MaxSet mset)
{
    int original = box->y2 - box->y1;
    int increment;

    if (!mset.down && !mset.up)
	return ;

    if (mset.up != mset.down)
	increment = original - height;
    else
	increment = (original - height) / 2;

    if (mset.up) 
	box->y1 += increment;
    if (mset.down)
	box->y2 -= increment;
}


/* Unmaximize the window if it's maximized */
void
MaximumizeScreen::unmaximizeWindow (CompWindow *w)
{
    int state = w->state ();
    if (state & MAXIMIZE_STATE)
    {
	state &= (w->state () & ~MAXIMIZE_STATE);
	w->changeState (state);
    }
}

/* Reduce box size by setting width and height to 1/4th or the minimum size
 * allowed, whichever is bigger.
 */
BOX
MaximumizeScreen::minimumize (CompWindow *w,
			      BOX	 box,
			      MaxSet	 mset)
{
    const int min_width = w->sizeHints ().min_width;
    const int min_height = w->sizeHints ().min_height;
    int width, height;

    unmaximizeWindow (w);

    width = box.x2 - box.x1;
    height = box.y2 - box.y1;

    if (width/4 < min_width) 
	setBoxWidth (&box, min_width, mset);
    else 
	setBoxWidth (&box, width/4, mset);

    if (height/4 < min_height) 
	setBoxHeight (&box, min_height, mset);
    else 
	setBoxHeight (&box, height/4, mset);

    return box;
}

/* Create a box for resizing in the given region
 * Also shrinks the window box in case of minor overlaps.
 * FIXME: should be somewhat cleaner.
 */
BOX
MaximumizeScreen::findRect (CompWindow  *w,
			    CompRegion  r,
			    MaxSet      mset)
{
    BOX windowBox, ansA, ansB, orig;

    windowBox.x1 = w->serverX ();
    windowBox.x2 = w->serverX () + w->serverWidth ();
    windowBox.y1 = w->serverY ();
    windowBox.y2 = w->serverY () + w->serverHeight ();

    orig = windowBox;
    
    if (mset.shrink)
	windowBox = minimumize (w, windowBox, mset);
    
    if (!mset.grow)
	return windowBox;

    ansA = extendBox (w, windowBox, r, true, mset);
    ansB = extendBox (w, windowBox, r, false, mset);

    if (!optionGetAllowShrink ()) 
    {
	if (boxCompare (orig, ansA) &&
	    boxCompare (orig, ansB))
	    return orig; 
    }
    else
    {
	// Order is essential here. 
	if (!boxCompare (ansA, windowBox) &&
	    !boxCompare (ansB, windowBox))
	    return orig;
    }

    if (boxCompare (ansA, ansB))
	return ansA;
    else
	return ansB;

}

/* Calls out to compute the resize */
unsigned int
MaximumizeScreen::computeResize (CompWindow     *w,
				 XWindowChanges *xwc,
				 MaxSet	   mset)
{
    CompOutput   output;
    CompRegion   region;
    unsigned int mask = 0;
    BOX          box;
    int          outputDevice = -1;


    outputDevice = w->outputDevice ();

    output = screen->outputDevs ()[outputDevice];
    region = findEmptyRegion (w, CompRegion (output));

    if(region == emptyRegion)
	return mask;
    box = findRect (w, region, mset);

    if (box.x1 != w->serverX ())
	mask |= CWX;

    if (box.y1 != w->serverY ())
	mask |= CWY;

    if ((box.x2 - box.x1) != w->serverWidth ())
	mask |= CWWidth;

    if ((box.y2 - box.y1) != w->serverHeight ())
	mask |= CWHeight;

    xwc->x = box.x1;
    xwc->y = box.y1;
    xwc->width = box.x2 - box.x1; 
    xwc->height = box.y2 - box.y1;

    return mask;
}

/* General trigger. This is for maximumizing / minimumizing without a direction
 * Note that the function is static in the class so 'this' in unavailable,
 * we have to use MAX_SCREEN (screen) to get the object
 */
bool
MaximumizeScreen::triggerGeneral (CompAction         *action,
				  CompAction::State  state,
				  CompOption::Vector &options,
				  bool		     grow)
{
    Window     xid;
    CompWindow *w;

    MAX_SCREEN (screen);

    xid = CompOption::getIntOptionNamed (options, "window");
    w   = screen->findWindow (xid);
    if (w)
    {
	int            width, height;
	unsigned int   mask;
	XWindowChanges xwc;
	MaxSet	       mset;

	if (screen->otherGrabExist (0))
	   return false;

	mset.left = ms->optionGetMaximumizeLeft ();
	mset.right = ms->optionGetMaximumizeRight ();
	mset.up = ms->optionGetMaximumizeUp ();
	mset.down = ms->optionGetMaximumizeDown ();
    
	mset.grow = grow;
	mset.shrink = true;

	mask = computeResize (w, &xwc, mset); 
	if (mask)
	{
	    if (w->constrainNewWindowSize (xwc.width, xwc.height,
					   &width, &height))
	    {
		mask |= CWWidth | CWHeight;
		xwc.width  = width;
		xwc.height = height;
	    }

	    if (w->mapNum () && (mask & (CWWidth | CWHeight)))
		w->sendSyncRequest ();

	    w->configureXWindow (mask, &xwc);
	}
    }

    return true;
}

/* 
 * Maximizing on specified direction.
 */
bool
MaximumizeScreen::triggerDirection (CompAction         *action,
				    CompAction::State  state,
				    CompOption::Vector &options,
				    bool		     left,
				    bool		     right,
				    bool		     up,
				    bool		     down,
				    bool		     grow)
{
    Window     xid;
    CompWindow *w;

    xid = CompOption::getIntOptionNamed (options, "window");
    w   = screen->findWindow (xid);
    if (w)
    {
	int            width, height;
	unsigned int   mask;
	XWindowChanges xwc;
	MaxSet	       mset;

	if (screen->otherGrabExist (0))
	    return false;


	mset.left = left;
	mset.right = right;
	mset.up = up;
	mset.down = down;
	mset.grow = grow;
	mset.shrink = !grow;


	mask = computeResize (w, &xwc, mset); 
	if (mask)
	{
	    if (w->constrainNewWindowSize (xwc.width, xwc.height,
					   &width, &height))
	    {
		mask |= CWWidth | CWHeight;
		xwc.width  = width;
		xwc.height = height;
	    }

	    if (w->mapNum () && (mask & (CWWidth | CWHeight)))
		w->sendSyncRequest ();

	    w->configureXWindow (mask, &xwc);
	}
    }
    return false;
}

/* Configuration, initialization, boring stuff. --------------------- */

/* Screen Constructor */
MaximumizeScreen::MaximumizeScreen (CompScreen *screen) :
    PrivateHandler <MaximumizeScreen, CompScreen> (screen),
    MaximumizeOptions (maximumizeVTable->getMetadata ())
{
/* This macro uses boost::bind to have lots of callbacks trigger the same
 * function with different arguments */
#define MAXSET(opt, up, down, left, right)			      \
    optionSet##opt##Initiate (boost::bind (&MaximumizeScreen::triggerDirection,this, _1, _2, _3, up,  \
					   down, left, right, true))
#define MINSET(opt, up, down, left, right)			      \
    optionSet##opt##Initiate (boost::bind (&MaximumizeScreen::triggerDirection,this, _1, _2, _3, up,  \
					   down, left, right, false))

    /* Maximumize Bindings */
    optionSetTriggerMaxKeyInitiate (boost::bind (&MaximumizeScreen::triggerGeneral,this,
						 _1, _2, _3, true));
    /*      TriggerDirection, left, right, up, down */
    MAXSET (TriggerMaxLeft, true, false, false, false);
    MAXSET (TriggerMaxRight, true, false, false, false);
    MAXSET (TriggerMaxUp, false, false, true, false);
    MAXSET (TriggerMaxDown, false, false, false, true);
    MAXSET (TriggerMaxHorizontally, true, true, false, false);
    MAXSET (TriggerMaxVertically, false, false, true, true);
    MAXSET (TriggerMaxUpLeft, true, false, true, false);
    MAXSET (TriggerMaxUpRight, false, true, true, false);
    MAXSET (TriggerMaxDownLeft, true, false, false, true);
    MAXSET (TriggerMaxDownRight, false, true, false, true);

    /* Maximumize Bindings */
    optionSetTriggerMinKeyInitiate (boost::bind (&MaximumizeScreen::triggerGeneral,this,
						 _1, _2, _3, false));

    MINSET (TriggerMinLeft, true, false, false, false);
    MINSET (TriggerMinRight, true, false, false, false);
    MINSET (TriggerMinUp, false, false, true, false);
    MINSET (TriggerMinDown, false, false, false, true);
    MINSET (TriggerMinHorizontally, true, true, false, false);
    MINSET (TriggerMinVertically, false, false, true, true);
    MINSET (TriggerMinUpLeft, true, false, true, false);
    MINSET (TriggerMinUpRight, false, true, true, false);
    MINSET (TriggerMinDownLeft, true, false, false, true);
    MINSET (TriggerMinDownRight, false, true, false, true);

#undef MAXSET
#undef MINSET
}

/* Initial plugin init function called. Checks to see if we are ABI
 * compatible with core, otherwise unload */

bool
MaximumizePluginVTable::init ()
{
    if (!CompPlugin::checkPluginABI ("core", CORE_ABIVERSION))
	return false;

    return true;
}
