/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://chibios-gfx.com/license.html
 */

/**
 * @file    src/gwin/slider.c
 * @brief   GWIN sub-system slider code.
 *
 * @defgroup Slider Slider
 * @ingroup GWIN
 *
 * @{
 */

#include "gfx.h"

#if (GFX_USE_GWIN && GWIN_NEED_SLIDER) || defined(__DOXYGEN__)

#include "gwin/internal.h"

#ifndef GWIN_SLIDER_DEAD_BAND
	#define GWIN_SLIDER_DEAD_BAND	5
#endif

#if GFX_USE_GINPUT && GINPUT_NEED_MOUSE
	static void trackSliderDraw(GHandle gh, coord_t x, coord_t y);
#endif

static const GSliderDrawStyle GSliderDefaultStyle = {
	HTML2COLOR(0x404040),		// color_edge;
	HTML2COLOR(0x000000),		// color_thumb;
	HTML2COLOR(0x00E000),		// color_active;
	HTML2COLOR(0xE0E0E0),		// color_inactive;
};

// Process an event callback
static void gwinSliderCallback(void *param, GEvent *pe) {
	GSourceListener	*psl;
	#define gh		((GHandle)param)
	#define gsw		((GSliderObject *)param)
	#define gsh		((GSourceHandle)param)
	#define pme		((GEventMouse *)pe)
	#define pde		((GEventDial *)pe)
	#define pse		((GEventGWinSlider *)pe)

	switch (pe->type) {
	#if GFX_USE_GINPUT && GINPUT_NEED_MOUSE
		case GEVENT_MOUSE:
		case GEVENT_TOUCH:
			// If not tracking we only only interested in a mouse down over the slider
			if (!gsw->tracking) {
				if ((pme->meta & GMETA_MOUSE_DOWN)
						&& pme->x >= gh->x && pme->x < gh->x + gh->width
						&& pme->y >= gh->y && pme->y < gh->y + gh->height) {
					gsw->tracking = TRUE;
					trackSliderDraw(gh, pme->x-gh->x, pme->y-gh->y);
				}
				return;
			}

			// We are tracking the mouse

			// Test for button up
			if ((pme->meta & GMETA_MOUSE_UP)) {
				gsw->tracking = FALSE;

				#if !GWIN_BUTTON_LAZY_RELEASE
					// Are we over the slider?
					if (pme->x < gh->x || pme->x >= gh->x + gh->width
							|| pme->y < gh->y || pme->y >= gh->y + gh->height) {
						// No - restore the slider
						gwinSliderDraw(gh);
						return;
					}
				#endif

				// Set the new position
				if (gh->width < gh->height)
					gwinSetSliderPosition(gh,
						(uint16_t)((uint32_t)(gh->height-1-pme->y+gh->y-GWIN_SLIDER_DEAD_BAND)*(gsw->max-gsw->min)/(gh->height-2*GWIN_SLIDER_DEAD_BAND) + gsw->min));
				else
					gwinSetSliderPosition(gh,
						(uint16_t)((uint32_t)(pme->x-gh->x-GWIN_SLIDER_DEAD_BAND)*(gsw->max-gsw->min)/(gh->width-2*GWIN_SLIDER_DEAD_BAND) + gsw->min));

				// Update the display
				gwinSliderDraw(gh);

				// Generate the event
				break;
			}

			// If mouse down - track movement
			if ((pme->current_buttons & GINPUT_MOUSE_BTN_LEFT))
				trackSliderDraw(gh, pme->x-gh->x, pme->y-gh->y);

			return;
	#endif
	#if GFX_USE_GINPUT && GINPUT_NEED_DIAL
		case GEVENT_DIAL:
			// Set the new position
			gwinSetSliderPosition(gh, (uint16_t)((uint32_t)pde->value*(gsw->max-gsw->min)/ginputGetDialRange(pde->instance) + gsw->min));

			// Update the display
			gwinSliderDraw(gh);

			// Generate the event
			break;
	#endif

	default:
		return;
	}

	// Trigger a GWIN Slider Event
	psl = 0;
	while ((psl = geventGetSourceListener(gsh, psl))) {
		if (!(pe = geventGetEventBuffer(psl)))
			continue;
		pse->type = GEVENT_GWIN_SLIDER;
		pse->slider = gh;
		pse->position = gsw->pos;
		geventSendEvent(psl);
	}

	#undef pse
	#undef pme
	#undef pxe
	#undef gsh
	#undef gsw
	#undef gh
}

GHandle gwinCreateSlider(GSliderObject *gs, coord_t x, coord_t y, coord_t width, coord_t height) {
	if (!(gs = (GSliderObject *)_gwinInit((GWindowObject *)gs, x, y, width, height, sizeof(GSliderObject))))
		return 0;
	gs->gwin.type = GW_SLIDER;
	gs->fn = gwinSliderDraw_Std;
	gs->param = 0;
	gwinSetSliderStyle(&gs->gwin, &GSliderDefaultStyle);
	gs->min = 0;
	gs->max = 100;
	gs->pos = 0;
	gs->tracking = FALSE;
	geventListenerInit(&gs->listener);
	geventRegisterCallback(&gs->listener, gwinSliderCallback, gs);
	return (GHandle)gs;
}

void gwinSetSliderRange(GHandle gh, int min, int max) {
	#define gsw		((GSliderObject *)gh)

	if (gh->type != GW_SLIDER)
		return;

	if (min == max)		// prevent divide by 0 errors.
		max++;
	gsw->min = min;
	gsw->max = max;
	gsw->pos = min;
	#undef gsw
}

void gwinSetSliderPosition(GHandle gh, int pos) {
	#define gsw		((GSliderObject *)gh)

	if (gh->type != GW_SLIDER)
		return;

	if (gsw->min <= gsw->max) {
		if (pos < gsw->min) gsw->pos = gsw->min;
		else if (pos > gsw->max) gsw->pos = gsw->max;
		else gsw->pos = pos;
	} else {
		if (pos > gsw->min) gsw->pos = gsw->min;
		else if (pos < gsw->max) gsw->pos = gsw->max;
		else gsw->pos = pos;
	}
	#undef gsw
}

void gwinSetSliderStyle(GHandle gh, const GSliderDrawStyle *pStyle) {
	#define gsw		((GSliderObject *)gh)

	if (gh->type != GW_SLIDER)
		return;

	gsw->style.color_edge		= pStyle->color_edge;
	gsw->style.color_thumb	= pStyle->color_thumb;
	gsw->style.color_active	= pStyle->color_active;
	gsw->style.color_inactive	= pStyle->color_inactive;
	#undef gsw
}

#if GFX_USE_GINPUT && GINPUT_NEED_MOUSE
	static void trackSliderDraw(GHandle gh, coord_t x, coord_t y) {
		#define gsw		((GSliderObject *)gh)

		#if GDISP_NEED_CLIP
			gdispSetClip(gh->x, gh->y, gh->width, gh->height);
		#endif

		if (gh->height <= gh->width)
			gsw->fn(gh, FALSE, x, &gsw->style, gsw->param);
		else
			gsw->fn(gh, TRUE, y, &gsw->style, gsw->param);
	
		#undef gbw
	}
#endif

void gwinSliderDraw(GHandle gh) {
	#define gsw		((GSliderObject *)gh)
	
	if (gh->type != GW_SLIDER)
		return;

	#if GDISP_NEED_CLIP
		gdispSetClip(gh->x, gh->y, gh->width, gh->height);
	#endif

	if (gh->height <= gh->width)
		gsw->fn(gh, FALSE, ((gh->width-1)*(gsw->pos-gsw->min))/(gsw->max-gsw->min), &gsw->style, gsw->param);
	else
		gsw->fn(gh, TRUE, gh->height-1-((gh->height-1)*(gsw->pos-gsw->min))/(gsw->max-gsw->min), &gsw->style, gsw->param);

	#undef gbw
}

void gwinSetSliderCustom(GHandle gh, GSliderDrawFunction fn, void *param) {
	#define gsw		((GSliderObject *)gh)

	if (gh->type != GW_SLIDER)
		return;

	gsw->fn = fn ? fn : gwinSliderDraw_Std;
	gsw->param = param;

	#undef gsw
}

void gwinSliderSetEnabled(GHandle gh, bool_t enabled) {
	if (gh->type != GW_SLIDER)
		return;

	gh->enabled = enabled;
}

void gwinSliderDraw_Std(GHandle gh, bool_t isVertical, coord_t thumbpos, const GSliderDrawStyle *pstyle, void *param) {
	(void) param;

	if (isVertical) {
		if (thumbpos != gh->height-1)
			gdispFillArea(gh->x, gh->y+thumbpos, gh->width, gh->height - thumbpos, pstyle->color_active);
		if (thumbpos != 0)
			gdispFillArea(gh->x, gh->y, gh->width, thumbpos, pstyle->color_inactive);
		gdispDrawBox(gh->x, gh->y, gh->width, gh->height, pstyle->color_edge);
		gdispDrawLine(gh->x, gh->y+thumbpos, gh->x+gh->width-1, gh->y+thumbpos, pstyle->color_thumb);
		if (thumbpos >= 2)
			gdispDrawLine(gh->x, gh->y+thumbpos-2, gh->x+gh->width-1, gh->y+thumbpos-2, pstyle->color_thumb);
		if (thumbpos <= gh->height-2)
			gdispDrawLine(gh->x, gh->y+thumbpos+2, gh->x+gh->width-1, gh->y+thumbpos+2, pstyle->color_thumb);
	} else {
		if (thumbpos != gh->width-1)
			gdispFillArea(gh->x+thumbpos, gh->y, gh->width-thumbpos, gh->height, pstyle->color_inactive);
		if (thumbpos != 0)
			gdispFillArea(gh->x, gh->y, thumbpos, gh->height, pstyle->color_active);
		gdispDrawBox(gh->x, gh->y, gh->width, gh->height, pstyle->color_edge);
		gdispDrawLine(gh->x+thumbpos, gh->y, gh->x+thumbpos, gh->y+gh->height-1, pstyle->color_thumb);
		if (thumbpos >= 2)
			gdispDrawLine(gh->x+thumbpos-2, gh->y, gh->x+thumbpos-2, gh->y+gh->height-1, pstyle->color_thumb);
		if (thumbpos <= gh->width-2)
			gdispDrawLine(gh->x+thumbpos+2, gh->y, gh->x+thumbpos+2, gh->y+gh->height-1, pstyle->color_thumb);
	}
}

#if GFX_USE_GINPUT && GINPUT_NEED_MOUSE
	bool_t gwinAttachSliderMouse(GHandle gh, uint16_t instance) {
		GSourceHandle	gsh;

		if (gh->type != GW_SLIDER || !(gsh = ginputGetMouse(instance)))
			return FALSE;

		return geventAttachSource(&((GSliderObject *)gh)->listener, gsh, GLISTEN_MOUSEMETA|GLISTEN_MOUSEDOWNMOVES);
	}
#endif

#if GFX_USE_GINPUT && GINPUT_NEED_DIAL
	bool_t gwinAttachSliderDial(GHandle gh, uint16_t instance) {
		GSourceHandle	gsh;

		if (gh->type != GW_SLIDER || !(gsh = ginputGetDial(instance)))
			return FALSE;

		return geventAttachSource(&((GSliderObject *)gh)->listener, gsh, 0);
	}
#endif

#endif /* GFX_USE_GWIN && GWIN_NEED_BUTTON */
/** @} */

