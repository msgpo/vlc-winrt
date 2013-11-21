﻿/*****************************************************************************
* Copyright © 2013 VideoLAN
*
* Authors: Kellen Sunderland <kellen _DOT_ sunderland _AT_ gmail _DOT_ com>
*
* This code is based on directfb.c, vmem.c, direct2d thanks VideoLAN team.
* Code also based on Microsoft DirectX / Xaml interop samples, thanks MS.
*
* This program is free software; you can redistribute it and/or modify it
* under the terms of the GNU Lesser General Public License as published by
* the Free Software Foundation; either version 2.1 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public License
* along with this program; if not, write to the Free Software Foundation,
* Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
*****************************************************************************/


/*****************************************************************************
* Preamble
*****************************************************************************/

#include "pch.h"
#include "targetver.h"
#include <ppltasks.h>
#include <windows.ui.xaml.media.dxinterop.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN /* Exclude rarely-used stuff from Windows headers */
#endif

#ifdef _MSC_VER /* help visual studio compile vlc headers */
#define inline __inline
#define strdup _strdup
#define ssize_t SSIZE_T
#define N_(x) x
#endif

using namespace concurrency;
using namespace Platform;
using namespace Microsoft::WRL;
using namespace Windows::Graphics::Display;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::UI::Core;

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_vout_display.h>
#include <vlc_picture_pool.h>

/*****************************************************************************
* Module descriptor
*****************************************************************************/
static int  Open(vlc_object_t *);
static void Close(vlc_object_t *);

vlc_module_begin()
    set_description(N_("Windows 8.1 video output"))
	set_shortname(N_("Video winrt"))

    set_category(CAT_VIDEO)
	set_subcategory(SUBCAT_VIDEO_VOUT)
	set_capability("vout display", 60)
	add_integer("winrt-d2dcontext", 0x0, NULL, NULL, true);

    set_callbacks(Open, Close)
vlc_module_end()

/*****************************************************************************
* Local prototypes
*****************************************************************************/
static picture_pool_t *Pool(vout_display_t *, unsigned);
static void           Display(vout_display_t *vd, picture_t *picture, subpicture_t *subpicture);
static int            Control(vout_display_t *vd, int query, va_list args);
static void           Manage(vout_display_t *vd);
static void           Prepare(vout_display_t *vd, picture_t *picture, subpicture_t *subpicture);
static int            CreateDeviceResources(vout_display_t* vd);

/* */
struct vout_display_sys_t {
	/* */
	int width;
	int height;

	/* */
	//TODO: check to see if these are all needed
	picture_pool_t             *pool;
	ID2D1Bitmap                *d2dbmp;
	ComPtr<ID2D1DeviceContext> d2dContext;
};


/**
* Renders video to a SwapChainPanel in WinRT Environments
* Currently this module only supports RGB
*/
static int Open(vlc_object_t *object)
{
	//TODO: error handling, cleanup
	vout_display_t *vd = (vout_display_t *) object;
	vout_display_sys_t *sys;

	vd->sys = sys = (vout_display_sys_t*) calloc(1, sizeof(*sys));
	if (!sys)
		return VLC_ENOMEM;

	//Todo: check on double click, hide mouse
	vout_display_info_t info = vd->info;
	info.is_slow = false;
	info.has_double_click = true;
	info.has_hide_mouse = false;
	info.has_pictures_invalid = false;
	vd->info = info;

	vd->fmt.i_chroma = VLC_CODEC_RGB32; /* masks change this to BGR32 for ID2D1Bitmap */
	vd->fmt.i_rmask = 0x0000ff00;
	vd->fmt.i_gmask = 0x00ff0000;
	vd->fmt.i_bmask = 0xff000000;

	vd->pool = Pool;
	vd->prepare = Prepare;
	vd->display = Display;
	vd->manage = Manage;
	vd->control = Control;

	return CreateDeviceResources(vd);
}

static void Close(vlc_object_t * object){
	vout_display_t *vd = (vout_display_t *) object;

	if (vd->sys->pool)
		picture_pool_Delete(vd->sys->pool);

	free(vd->sys);

	return;
}

static int OpenDisplay(vout_display_t *vd)
{
	//TODO: call callback and populate SwapChainPanel
	//TODO: intialize dependant and independant resources

	vout_display_sys_t *sys = vd->sys;
}

static void CloseDisplay(vout_display_t *vd)
{

}

static void Display(vout_display_t *vd, picture_t *picture, subpicture_t *subpicture)
{
	vout_display_sys_t *sys = vd->sys;
	//TODO: d2d magic

	vd->sys->d2dContext->BeginDraw();
	vd->sys->d2dContext->DrawBitmap(sys->d2dbmp, NULL);
	HRESULT hr = vd->sys->d2dContext->EndDraw();
}

static int Control(vout_display_t *vd, int query, va_list args)
{
	//TODO: do we care about resizes?  Windows 8 should take care of it
	switch (query) {
	case VOUT_DISPLAY_CHANGE_FULLSCREEN:
	case VOUT_DISPLAY_CHANGE_DISPLAY_SIZE: {
											   const vout_display_cfg_t *cfg = va_arg(args, const vout_display_cfg_t *);
											   if (cfg->display.width != vd->fmt.i_width ||
												   cfg->display.height != vd->fmt.i_height)
												   return VLC_EGENERIC;
											   if (cfg->is_fullscreen)
												   return VLC_EGENERIC;
											   return VLC_SUCCESS;
	}
	default:
		return VLC_EGENERIC;
	}
}

static void Manage(vout_display_t *vd)
{
	VLC_UNUSED(vd);
}

/**
* Handles pool allocations for bitmaps
*/
static picture_pool_t *Pool(vout_display_t *vd, unsigned count)
{
	vout_display_sys_t *sys = vd->sys;

	if (!sys->pool) {
		sys->pool = picture_pool_NewFromFormat(&vd->fmt, count);
	}

	return sys->pool;
}


/**
* Performs set up of ID2D1Bitmap memory ready for blitting
*/
static void Prepare(vout_display_t *vd, picture_t *picture, subpicture_t *subpicture)
{
	vout_display_sys_t *sys = vd->sys;

	if (sys->d2dbmp) 
	{
		HRESULT hr =  sys->d2dbmp->CopyFromMemory(NULL, picture->p[0].p_pixels, picture->p[0].i_pitch);

		if (hr != S_OK)
			msg_Err(vd, "Failed to copy bitmap memory (hr = 0x%x)!",
			(unsigned) hr);

	/*	HRESULT hr = sys->d2dbmp->CopyFromMemory(&D2D1::RectU(static_cast<UINT>(updateRect.Left),
			static_cast<UINT>(updateRect.Top),
			static_cast<UINT>(updateRect.Right),
			static_cast<UINT>(updateRect.Bottom)), picture->p[0].p_pixels, picture->p[0].i_pitch);*/

#ifndef NDEBUG
		msg_Dbg(vd, "Bitmap dbg: pitch = %d, bitmap = %p", picture->p[0].i_pitch, sys->d2dbmp);
#endif
	}

	VLC_UNUSED(subpicture);
}


// Initialize hardware-dependent resources.
static int CreateDeviceResources(vout_display_t* vd)
{
	D2D1_BITMAP_PROPERTIES props;
	D2D1_PIXEL_FORMAT pixFormat;
	D2D1_SIZE_U size;
	HRESULT hr;
	vout_display_sys_t *sys = vd->sys;
	float dpi = DisplayProperties::LogicalDpi;


	int panelInt = var_CreateGetInteger(vd, "winrt-d2dcontext");
	reinterpret_cast<IUnknown*>(panelInt)->QueryInterface(IID_PPV_ARGS(&sys->d2dContext));

	size.width = vd->fmt.i_width;
	size.height = vd->fmt.i_height;
	pixFormat.alphaMode = D2D1_ALPHA_MODE_IGNORE;
	pixFormat.format = DXGI_FORMAT_B8G8R8X8_UNORM;
	props.pixelFormat = pixFormat;
	props.dpiX = dpi;
	props.dpiY = dpi;
	
	hr = sys->d2dContext->CreateBitmap(size, props, &sys->d2dbmp);
	if (hr != S_OK)
		return VLC_EGENERIC;

	return VLC_SUCCESS;
}
