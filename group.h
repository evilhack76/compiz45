#ifndef _GROUP_H
#define _GROUP_H

/**
 *
 * Beryl group plugin
 *
 * group.h
 *
 * Copyright : (C) 2006 by Patrick Niklaus, Roi Cohen, Danny Baumann
 * Authors: Patrick Niklaus <patrick.niklaus@googlemail.com>
 *          Roi Cohen       <roico@beryl-project.org>
 *          Danny Baumann   <maniac@beryl-project.org>
 *
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
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <time.h>
#include <X11/Xlib.h>
#include <cairo/cairo-xlib-xrender.h>
#include <compiz.h>
#include "text-plugin.h"
#include <X11/Xatom.h>
#include <X11/extensions/shape.h>

#include <math.h>
#include <limits.h>

#include "group_options.h"

/*
 * Constants
 *
 */
#define PI 3.1415926535897

/*
 * Helpers
 *
 */
#define GET_GROUP_DISPLAY(d) ((GroupDisplay *) (d)->privates[displayPrivateIndex].ptr)
#define GROUP_DISPLAY(d) GroupDisplay *gd = GET_GROUP_DISPLAY (d)
#define GET_GROUP_SCREEN(s, gd) ((GroupScreen *) (s)->privates[ (gd)->screenPrivateIndex].ptr)
#define GROUP_SCREEN(s) GroupScreen *gs = GET_GROUP_SCREEN (s, GET_GROUP_DISPLAY (s->display))
#define GET_GROUP_WINDOW(w, gs) ((GroupWindow *) (w)->privates[ (gs)->windowPrivateIndex].ptr)
#define GROUP_WINDOW(w) GroupWindow *gw = GET_GROUP_WINDOW (w, GET_GROUP_SCREEN  (w->screen, GET_GROUP_DISPLAY (w->screen->display)))

#define WIN_X(w) (w->attrib.x)
#define WIN_Y(w) (w->attrib.y)
#define WIN_WIDTH(w) (w->attrib.width)
#define WIN_HEIGHT(w) (w->attrib.height)
#define WIN_BORDER(w) (w->attrib.border_width)

/* definitions used for glow painting */
#define WIN_REAL_X(w) (w->attrib.x - w->input.left)
#define WIN_REAL_Y(w) (w->attrib.y - w->input.top)
#define WIN_REAL_WIDTH(w) (w->width + 2 * w->attrib.border_width + w->input.left + w->input.right)
#define WIN_REAL_HEIGHT(w) (w->height + 2 * w->attrib.border_width + w->input.top + w->input.bottom)

#define NUM_OPTIONS(s) (sizeof ( (s)->opt) / sizeof (CompOption))
#define N_WIN_TYPE (sizeof (groupDefaultTypes) / sizeof (groupDefaultTypes[0]))
#define N_SEL_MODE (sizeof (groupSelectionModes) / sizeof (groupSelectionModes[0]))

#define REAL_POSITION(x, s) ( (x >= 0)? x: x + (s)->hsize * (s)->width )
#define VIEWPORT(x, s) ( ( REAL_POSITION(x, s) / (s)->width ) % (s)->hsize )

#define TOP_TAB(g) ((g)->topTab->window)
#define PREV_TOP_TAB(g) ((g)->prevTopTab->window)
#define NEXT_TOP_TAB(g) ((g)->nextTopTab->window)

#define HAS_TOP_WIN(group) (((group)->topTab) && ((group)->topTab->window))
#define HAS_PREV_TOP_WIN(group) (((group)->prevTopTab) && ((group)->prevTopTab->window))

#define IS_TOP_TAB(w, group) (HAS_TOP_WIN(group) && ((TOP_TAB(group)->id) == (w)->id))
#define IS_PREV_TOP_TAB(w, group) (HAS_PREV_TOP_WIN(group) && ((PREV_TOP_TAB(group)->id) == (w)->id))

/*
 * Structs
 *
 */

/*
 * window states  
 */
typedef enum {
	WindowNormal = 0,
	WindowMinimized,
	WindowShaded
} GroupWindowState;

/*
 * screengrab states
 */
typedef enum {
	ScreenGrabNone = 0,
	ScreenGrabSelect,
	ScreenGrabTabDrag
} GroupScreenGrabState;

/*
 * ungrouping states
 */
typedef enum {
	UngroupNone = 0,
	UngroupAll,
	UngroupSingle
} GroupUngroupState;

/*
 * rotation direction for change tab animation
 */
typedef enum {
	RotateUncertain = 0,
	RotateLeft,
	RotateRight
} ChangeTabAnimationDirection;

typedef struct _GlowTextureProperties {
	char * textureData;
	int textureSize;
	int glowOffset;
} GlowTextureProperties;

/*
 * structs for pending callbacks
 */
typedef struct _GroupPendingMoves GroupPendingMoves;
struct _GroupPendingMoves {
	CompWindow *w;
	int dx;
	int dy;
	Bool immediate;
	Bool sync;
	GroupPendingMoves *next;
};

typedef struct _GroupPendingGrabs GroupPendingGrabs;
struct _GroupPendingGrabs {
	CompWindow *w;
	int x;
	int y;
	unsigned int state;
	unsigned int mask;
	GroupPendingGrabs *next;
};

typedef struct _GroupPendingUngrabs GroupPendingUngrabs;
struct _GroupPendingUngrabs {
	CompWindow *w;
	GroupPendingUngrabs *next;
};

/*
 * pointer to display list
 */
int displayPrivateIndex;

/*
 * PaintState
 */

/* mask for screen wide actions executed in preparePaintScreen */
#define CHECK_WINDOW_PROPERTIES	(1 << 0)
#define APPLY_AUTO_TABBING	(1 << 1)

/* mask values for groupTabSetVisibility */
#define SHOW_BAR_INSTANTLY_MASK	(1 << 0)
#define PERMANENT		(1 << 1)

/* mask values for tabbing animation */
#define IS_ANIMATED		(1 << 0)
#define FINISHED_ANIMATION	(1 << 1)
#define CONSTRAINED_X		(1 << 2)
#define CONSTRAINED_Y		(1 << 3)
#define DONT_CONSTRAIN		(1 << 4)

typedef enum {
	PaintOff = 0,
	PaintFadeIn,
	PaintFadeOut,
	PaintOn,
	PaintPermanentOn
} PaintState;

typedef enum {
	AnimationNone = 0,
	AnimationPulse,
	AnimationReflex
} GroupAnimationType;

typedef struct _GroupCairoLayer {
	Pixmap			pixmap;
	CompTexture		texture;
	cairo_surface_t		*surface;
	cairo_t			*cairo;

	int texWidth, texHeight;

	PaintState		state;
	int			animationTime;
} GroupCairoLayer;

/*
 * GroupTabBarSlot
 */
typedef struct _GroupTabBarSlot GroupTabBarSlot;
struct _GroupTabBarSlot {
	GroupTabBarSlot	*prev;
	GroupTabBarSlot	*next;

	Region		region;

	CompWindow	*window;
	
	//for DnD animations.
	int		springX;
	int		speed;
	float		msSinceLastMove;
};

/*
 * GroupTabBar
 */
typedef struct _GroupTabBar {
	GroupTabBarSlot	*slots;
	GroupTabBarSlot	*revSlots;
	int		nSlots;

	GroupTabBarSlot	*hoveredSlot;
	GroupTabBarSlot *textSlot;

	GroupCairoLayer	*textLayer;
	GroupCairoLayer	*bgLayer;
	GroupCairoLayer	*selectionLayer;

	// for animations
	int bgAnimationTime;
	GroupAnimationType bgAnimation;

	PaintState	state;
	int		timeoutHandle;
	int		animationTime;
	Region		region;
	int		oldWidth;
	
	// for DnD animations.
	int		leftSpringX, rightSpringX;
	int		leftSpeed, rightSpeed;
	float		leftMsSinceLastMove, rightMsSinceLastMove;
} GroupTabBar;

/*
 * GroupGlow
 */

typedef struct _GlowQuad {
	BoxRec box;
	CompMatrix matrix;
} GlowQuad;

#define GLOWQUAD_TOPLEFT	0
#define GLOWQUAD_TOPRIGHT	1
#define GLOWQUAD_BOTTOMLEFT	2
#define GLOWQUAD_BOTTOMRIGHT	3
#define GLOWQUAD_TOP		4
#define GLOWQUAD_BOTTOM		5
#define GLOWQUAD_LEFT		6
#define GLOWQUAD_RIGHT		7
#define NUM_GLOWQUADS		8

/*
 * GroupSelection
 */
typedef struct _GroupSelection GroupSelection;
struct _GroupSelection {
	GroupSelection *prev;
	GroupSelection *next;

	CompScreen *screen;
	CompWindow **windows;
	int nWins;

	// unique identifier for this group
	long int identifier;

	GroupTabBarSlot* topTab;
	GroupTabBarSlot* prevTopTab;
	
	//Those two are only for the change-tab animation, when the tab was changed again during animation.
	//Another animation should be started again, switching for this window.
	ChangeTabAnimationDirection nextDirection;
	GroupTabBarSlot* nextTopTab;	
	
	GroupTabBarSlot* activateTab;

	GroupTabBar *tabBar;

	int changeAnimationTime;
	int changeAnimationDirection;
	PaintState changeState;
	Bool	changeTab;

	Bool doTabbing;
	PaintState tabbingState;

	GroupUngroupState ungroupState;

	Window grabWindow;
	unsigned int grabMask;

	int oldTopTabCenterX;
	int oldTopTabCenterY;

	Window inputPrevention;

	GLushort color[4];
};

typedef struct _GroupWindowHideInfo {
	Window frameWindow;
	unsigned long skipState;
	unsigned long shapeMask;
	XRectangle *inputRects;
	int nInputRects;
	int inputRectOrdering;
} GroupWindowHideInfo;

/*
 * GroupDisplay structure
 */
typedef struct _GroupDisplay {
	int screenPrivateIndex;
	HandleEventProc handleEvent;

	GroupSelection tmpSel;
	Bool ignoreMode;

	GlowTextureProperties *glowTextureProperties;

	Atom groupWinPropertyAtom;
} GroupDisplay;

/*
 * GroupScreen structure
 */
typedef struct _GroupScreen {
	int windowPrivateIndex;

	WindowMoveNotifyProc windowMoveNotify;
	WindowResizeNotifyProc windowResizeNotify;
	GetOutputExtentsForWindowProc getOutputExtentsForWindow;
	PreparePaintScreenProc preparePaintScreen;
	PaintScreenProc paintScreen;
	DrawWindowProc drawWindow;
	PaintWindowProc paintWindow;
	PaintTransformedScreenProc paintTransformedScreen;
	DonePaintScreenProc donePaintScreen;
	WindowGrabNotifyProc windowGrabNotify;
	WindowUngrabNotifyProc windowUngrabNotify;
	WindowAddNotifyProc windowAddNotify;
	DamageWindowRectProc damageWindowRect;
	WindowStateChangeNotifyProc windowStateChangeNotify;

	GroupPendingMoves *pendingMoves;
	GroupPendingGrabs *pendingGrabs;
	GroupPendingUngrabs *pendingUngrabs;

	GroupSelection *groups;

	Bool queued;
	Bool tabBarVisible;

	GroupScreenGrabState grabState;
	int grabIndex;

	GroupSelection *lastHoveredGroup;
	
	int showDelayTimeoutHandle;

	// for selection
	Bool painted;
	int vpX, vpY;
	Bool isRotating;
	int x1;
	int y1;
	int x2;
	int y2;
	
	//For d&d
	GroupTabBarSlot	*draggedSlot;
	int dragHoverTimeoutHandle;
	Bool dragged;
	int prevX, prevY;	//buffer for mouse coordinates

	CompTexture glowTexture;

	// pending screen wide actions
	unsigned int screenActions;
} GroupScreen;

/*
 * GroupWindow structure
 */
typedef struct _GroupWindow {
	GroupSelection *group;
	Bool inSelection;

	// for the tab bar
	GroupTabBarSlot *slot;
	int oldWindowState;

	// for resize notify...
	Bool needsPosSync;

	GlowQuad *glowQuads;

	GroupWindowState windowState;
	GroupWindowHideInfo *windowHideInfo;

	// for tab animation
	Bool ungroup;
	int animateState;
	XPoint mainTabOffset;
	XPoint destination;
	XPoint orgPos;
	float tx,ty;
	float xVelocity, yVelocity;

	int lastState;
} GroupWindow;

/*
 * Pre-Definitions
 *
 */

/*
 * group.c
 */
/* Compiz only functions */
Bool screenGrabExist(CompScreen *s, ...);

void groupUpdateWindowProperty(CompWindow *w);
GroupSelection* groupFindGroupByID(CompScreen *s, long int id);
Bool groupCheckWindowProperty(CompWindow *w, long int *id, Bool *tabbed, GLushort *color);
void groupGrabScreen(CompScreen * s, GroupScreenGrabState newState);
void groupHandleEvent(CompDisplay * d, XEvent * event);
Bool groupGroupWindows(CompDisplay * d, CompAction * action, CompActionState state, CompOption * option, int nOption);
Bool groupUnGroupWindows(CompDisplay * d, CompAction * action, CompActionState state, CompOption * option, int nOption);
int groupFindWindowIndex(CompWindow *w, GroupSelection *g);
void groupDeleteGroupWindow(CompWindow * w, Bool allowRegroup);
void groupDeleteGroup(GroupSelection *group);
void groupAddWindowToGroup(CompWindow * w, GroupSelection *group, long int initialIdent);
void groupSyncWindows(GroupSelection *group);
void groupRaiseWindows(CompWindow * top, GroupSelection *group);
Bool groupRemoveWindow(CompDisplay * d, CompAction * action, CompActionState state, CompOption * option, int nOption);
Bool groupCloseWindows(CompDisplay * d, CompAction * action, CompActionState state, CompOption * option, int nOption);
Bool groupChangeColor(CompDisplay * d, CompAction * action, CompActionState state, CompOption * option, int nOption);
Bool groupSetIgnore(CompDisplay * d, CompAction * action, CompActionState state, CompOption * option, int nOption);
Bool groupUnsetIgnore(CompDisplay * d, CompAction * action, CompActionState state, CompOption * option, int nOption);
void groupWindowResizeNotify(CompWindow * w, int dx, int dy, int dwidth, int dheight);
void groupWindowGrabNotify(CompWindow * w, int x, int y, unsigned int state, unsigned int mask);
void groupWindowUngrabNotify(CompWindow * w);
void groupWindowMoveNotify(CompWindow * w, int dx, int dy, Bool immediate);
void groupWindowStateChangeNotify(CompWindow *w);
void groupGetOutputExtentsForWindow(CompWindow * w, CompWindowExtents * output);
void groupWindowAddNotify(CompWindow * w);
Bool groupDamageWindowRect(CompWindow * w, Bool initial, BoxPtr rect);

/*
 * tab.c
 */
void groupSetWindowVisibility(CompWindow *w, Bool visible);
void groupClearWindowInputShape(CompWindow *w, GroupWindowHideInfo *hideInfo);
void groupHandleChanges(CompScreen* s);
void groupHandleScreenActions(CompScreen *s);
void groupHandleHoverDetection(GroupSelection *group);
void groupHandleTabBarFade(GroupSelection *group, int msSinceLastPaint);
void groupHandleTabBarAnimation(GroupSelection *group, int msSinceLastPaint);
void groupHandleTextFade(GroupSelection *group, int msSinceLastPaint);
void groupDrawTabAnimation(CompScreen * s, int msSinceLastPaint);
void groupUpdateTabBars(CompScreen *s, Window enteredWin);
void groupCheckForVisibleTabBars(CompScreen *s);
void groupGetDrawOffsetForSlot(GroupTabBarSlot *slot, int *hoffset, int *voffset);
void groupTabSetVisibility(GroupSelection *group, Bool visible, unsigned int mask);
Bool groupGetCurrentMousePosition(CompScreen *s, int *x, int *y);
void groupClearCairoLayer(GroupCairoLayer *layer);
void groupDestroyCairoLayer(CompScreen *s, GroupCairoLayer *layer);
GroupCairoLayer *groupRebuildCairoLayer(CompScreen *s, GroupCairoLayer *layer, int width, int height);
GroupCairoLayer *groupCreateCairoLayer(CompScreen *s, int width, int height);
void groupRecalcTabBarPos(GroupSelection *group, int middleX, int minX1, int maxX2);
void groupInsertTabBarSlotAfter(GroupTabBar *bar, GroupTabBarSlot *slot, GroupTabBarSlot *prevSlot);
void groupInsertTabBarSlotBefore(GroupTabBar *bar, GroupTabBarSlot *slot, GroupTabBarSlot *nextSlot);
void groupInsertTabBarSlot(GroupTabBar *bar, GroupTabBarSlot *slot);
void groupUnhookTabBarSlot(GroupTabBar *bar, GroupTabBarSlot *slot, Bool temporary);
void groupDeleteTabBarSlot(GroupTabBar *bar, GroupTabBarSlot *slot);
void groupCreateSlot(GroupSelection *group, CompWindow *w);
void groupApplyForces(CompScreen *s, GroupTabBar *bar, GroupTabBarSlot* draggedSlot);
void groupApplySpeeds(CompScreen* s, GroupTabBar* bar, int msSinceLastRepaint);
void groupInitTabBar(GroupSelection *group, CompWindow* topTab);
void groupDeleteTabBar(GroupSelection *group);
void groupStartTabbingAnimation(GroupSelection *group, Bool tab);
void groupTabGroup(CompWindow * main);
void groupUntabGroup(GroupSelection *group);
Bool groupInitTab(CompDisplay * d, CompAction * action, CompActionState state, CompOption * option, int nOption);
Bool groupChangeTab(GroupTabBarSlot* topTab, ChangeTabAnimationDirection direction);
Bool groupChangeTabLeft(CompDisplay * d, CompAction * action, CompActionState state, CompOption * option, int nOption);
Bool groupChangeTabRight(CompDisplay * d, CompAction * action, CompActionState state, CompOption * option, int nOption);
void groupUpdateInputPreventionWindow(GroupSelection* group);
void groupSwitchTopTabInput(GroupSelection *group, Bool enable);
void groupCreateInputPreventionWindow(GroupSelection* group); 
void groupDestroyInputPreventionWindow(GroupSelection* group); 
Region groupGetClippingRegion(CompWindow *w);

/*
 * paint.c
 */
void groupRecomputeGlow(CompScreen *s);
void groupComputeGlowQuads(CompWindow *w, CompMatrix *matrix);
void groupRenderTopTabHighlight(GroupSelection *group);
void groupRenderTabBarBackground(GroupSelection *group);
void groupRenderWindowTitle(GroupSelection *group);
void groupPaintThumb(GroupSelection *group, GroupTabBarSlot *slot, const CompTransform *transform, int targetOpacity);
void groupPaintTabBar(GroupSelection * group, const WindowPaintAttrib *wAttrib, const CompTransform *transform, unsigned int mask, Region clipRegion);
void groupPreparePaintScreen(CompScreen * s, int msSinceLastPaint);
Bool groupPaintScreen(CompScreen * s, const ScreenPaintAttrib * sAttrib, const CompTransform *transform, Region region, int output, unsigned int mask);
void groupPaintTransformedScreen(CompScreen * s, const ScreenPaintAttrib * sa, const CompTransform *transform, Region region, int output, unsigned int mask);
void groupDonePaintScreen(CompScreen * s);
Bool groupDrawWindow(CompWindow * w, const CompTransform *transform, const FragmentAttrib * attrib, Region region, unsigned int mask);
Bool groupPaintWindow(CompWindow * w, const WindowPaintAttrib * attrib, const CompTransform *transform, Region region, unsigned int mask);

/*
 * init.c
 */
Bool groupInitDisplay(CompPlugin * p, CompDisplay * d);
void groupFiniDisplay(CompPlugin * p, CompDisplay * d);
Bool groupInitScreen(CompPlugin * p, CompScreen * s);
void groupFiniScreen(CompPlugin * p, CompScreen * s);
Bool groupInitWindow(CompPlugin * p, CompWindow * w);
void groupFiniWindow(CompPlugin * p, CompWindow * w);
Bool groupInit(CompPlugin * p);
void groupFini(CompPlugin * p);

/*
 * queues.c
 */
void groupEnqueueMoveNotify (CompWindow *w, int dx, int dy, Bool immediate, Bool sync);
void groupDequeueMoveNotifies (CompScreen *s);
void groupEnqueueGrabNotify (CompWindow *w, int x, int y, unsigned int state, unsigned int mask);
void groupDequeueGrabNotifies (CompScreen *s);
void groupEnqueueUngrabNotify (CompWindow *w);
void groupDequeueUngrabNotifies (CompScreen *s);

/*
 * selection.c
 */
CompWindow **groupFindWindowsInRegion(CompScreen * s, Region reg, int *c);
void groupDeleteSelectionWindow(CompDisplay * d, CompWindow * w);
void groupAddWindowToSelection(CompDisplay * d, CompWindow * w);
void groupSelectWindow(CompDisplay * d, CompWindow * w);
Bool groupSelectSingle(CompDisplay * d, CompAction * action, CompActionState state, CompOption * option, int nOption);
Bool groupSelect(CompDisplay * d, CompAction * action, CompActionState state, CompOption * option, int nOption);
Bool groupSelectTerminate(CompDisplay * d, CompAction * action, CompActionState state, CompOption * option, int nOption);
void groupDamageSelectionRect(CompScreen* s, int xRoot, int yRoot);

#endif
