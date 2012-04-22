/*
 * Copyright © 2011 Intel Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of the
 * copyright holders not be used in advertising or publicity
 * pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#ifdef HAVE_XORG_CONFIG_H
#include "xorg-config.h"
#endif

#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <wayland-client.h>
#include <X11/extensions/compositeproto.h>

#include <xorg-server.h>
#include <xf86Crtc.h>
#include <selection.h>
#include <compositeext.h>
#include <exevents.h>

#include "xwayland.h"
#include "xwayland-private.h"
#include "xserver-client-protocol.h"

static DevPrivateKeyRec xwl_window_private_key;

void		winMWExtWMResizeXWindow			(WindowPtr pWin, int w, int h)
{
	CARD32 *vlist = malloc(sizeof(CARD32)*2);
	
	vlist[0] = w;
	vlist[1] = h;
	ConfigureWindow (pWin, CWWidth | CWHeight, vlist, wClient(pWin));
	free(vlist);
}


static void
xwl_shell_handle_configure(void *data, struct wl_shell_surface *shell_surface, uint32_t edges, int32_t width, int32_t height)
{
	dHackP("data 0x%lx wh %d %d", data, width, height);
	struct xwl_window *xwl_window = data;
	if (width <= 0 || height <= 0)
		return;
	
	dHackP("shell_surface 0x%lx window 0x%lx", shell_surface, xwl_window->window);
	dHackP("window id %d", xwl_window->window->drawable.id);
//	winMWExtWMResizeXWindow (xwl_window->window, width, height);
	
	CARD32 *vlist = malloc(sizeof(CARD32)*2);
	
	vlist[0] = width;
	vlist[1] = height;
	ConfigureWindow (xwl_window->window, CWWidth | CWHeight, vlist, wClient(xwl_window->window));
	free(vlist);
	
/*	XWindowChanges wc;
	
	c->oldx = c->x;
	c->x = wc.x = x;
	c->oldy = c->y;
	c->y = wc.y = y;
	c->oldw = c->w;
	c->w = wc.width = w;
	c->oldh = c->h;
	c->h = wc.height = h;
	wc.border_width = c->bw;
	XConfigureWindow(dpy, c->win, CWX|CWY|CWWidth|CWHeight|CWBorderWidth, &wc);
	Client_StructureNotifySend(c);
	XSync(dpy, False);
	*/
	
//	int ret =  ConfigureWindow(xwl_window->window, Mask /*mask*/, XID* /*vlist*/, ClientPtr /*client*/);
	
//	xwl_window->resize_edges = edges;
//	window_schedule_resize(xwl_window, width, height);
}
static void
xwl_shell_handle_popup_done(void *data, struct wl_shell_surface *shell_surface)
{
//	printf ("HAUHTEONSUHETONSUHTENSOUHTEOTNSUHEOTNSUHTEOSUHTNSEOU xwl_shell_handle_popup_done\n");
/*	struct window *window = data;
	struct menu *menu = window->widget->user_data;

	/* FIXME: Need more context in this event, at least the input
	 * device.  Or just use wl_callback.  And this really needs to
	 * be a window vfunc that the menu can set.  And we need the
	 * time. *//*

	menu->func(window->parent, menu->current, window->parent->user_data);
	input_ungrab(menu->input);
	menu_destroy(menu);/**/
}

static void
xwl_shell_handle_ping(void *data, struct wl_shell_surface *shell_surface,
							uint32_t serial)
{
	wl_shell_surface_pong(shell_surface, serial);
}

static const struct wl_shell_surface_listener xwl_shell_surface_listener = {
	xwl_shell_handle_ping,
	xwl_shell_handle_configure,
	xwl_shell_handle_popup_done
};

static void
free_pixmap(void *data, struct wl_callback *callback, uint32_t time)
{
//	xf86DrvMsgVerb(0, X_INFO, 0, "AEUEUEOUUEAEUEUEOUUEAEUEUEOUUE  free_pixmap\n");
//	PixmapPtr pixmap = data;
//	ScreenPtr screen = pixmap->drawable.pScreen;
	
//	(*screen->DestroyPixmap)(pixmap);
//	wl_callback_destroy(callback);
}

static const struct wl_callback_listener free_pixmap_listener = {
	free_pixmap,
};

static void
xwl_window_attach(struct xwl_window *xwl_window, PixmapPtr pixmap)
{
	struct xwl_screen *xwl_screen = xwl_window->xwl_screen;
	struct wl_callback *callback;
	
	/* We can safely destroy the buffer because we only use one buffer
	* per surface in xwayland model */
	if (xwl_window->buffer)
		wl_buffer_destroy(xwl_window->buffer);
	
	
	xwl_screen->driver->create_window_buffer(xwl_window, pixmap);
	
	if (!xwl_window->buffer) {
		ErrorF("failed to create buffer\n");
		return;
	}
	
	wl_surface_attach(xwl_window->surface, xwl_window->buffer, 0, 0);
	
	struct wl_region *input_region = wl_compositor_create_region(xwl_screen->compositor);
	wl_region_add(input_region, 0, 0, pixmap->drawable.width, pixmap->drawable.height);
	
	if (input_region) {
		wl_surface_set_input_region(xwl_window->surface, input_region);
		wl_region_destroy(input_region);
		input_region = NULL;
	}
	
	struct wl_region *opaque_region = wl_compositor_create_region(xwl_screen->compositor);
	wl_region_add(opaque_region, 0, 0, pixmap->drawable.width, pixmap->drawable.height);
	
	if (opaque_region) {
		wl_surface_set_opaque_region(xwl_window->surface, opaque_region);
		wl_region_destroy(opaque_region);
		opaque_region = NULL;
	}
	
	wl_surface_damage(xwl_window->surface, 0, 0,
			pixmap->drawable.width,
			pixmap->drawable.height);
	
	
	callback = wl_display_sync(xwl_screen->display);
	wl_callback_add_listener(callback, &free_pixmap_listener, pixmap);
	pixmap->refcnt++;
}

static Bool
xwl_create_window(WindowPtr window)
{
	printf ("AEUEUEOUUEAEUEUEOUUEAEUEUEOUUE euhtoauhetonsuahtonsuehotaues \n");
	xf86DrvMsgVerb(0, X_INFO, 0, "@@@@@@@@@@@@@@@@@@@@@@@@@@@ euhtoauhetonsuahtonsuehotaues\n");
	xf86DrvMsgVerb(0, X_ERROR, 0, "@@@@@@@@@@@@@@@@@@@@@@@@@@@ euhtoauhetonsuahtonsuehotaues\n");
	dHackP ("window id %d", window->drawable.id);
    ScreenPtr screen = window->drawable.pScreen;
    struct xwl_screen *xwl_screen;
    Selection *selection;
    char buffer[32];
    int len, rc;
    Atom name;
    Bool ret;

    xwl_screen = xwl_screen_get(screen);

    screen->CreateWindow = xwl_screen->CreateWindow;
    ret = (*screen->CreateWindow)(window);
    xwl_screen->CreateWindow = screen->CreateWindow;
    screen->CreateWindow = xwl_create_window;

	struct xwl_window * xwl_window = calloc(sizeof *xwl_window, 1);
	xwl_window->xwl_screen = xwl_screen;
	xwl_window->window = window;
	xwl_window->surface = wl_compositor_create_surface(xwl_screen->compositor);
	
	if (xwl_window->surface == NULL) {
		ErrorF("wl_display_create_surface failed\n");
		return FALSE;
	}
	xorg_list_add(&xwl_window->link, &xwl_screen->window_list);
	xorg_list_init(&xwl_window->link_damage);
	
	wl_surface_set_user_data(xwl_window->surface, xwl_window);
	dixSetPrivate(&window->devPrivates, &xwl_window_private_key, xwl_window);
	
	
    if (!(xwl_screen->flags & XWL_FLAGS_ROOTLESS) || window->parent != NULL) {
	dHackP ("E window->parent != NULL");
	return ret;
	}
    len = snprintf(buffer, sizeof buffer, "_NET_WM_CM_S%d", screen->myNum);
    name = MakeAtom(buffer, len, TRUE);
    rc = AddSelection(&selection, name, serverClient);
    if (rc != Success)
	return ret;



    selection->lastTimeChanged = currentTime;
    selection->window = window->drawable.id;
    selection->pWin = window;
    selection->client = serverClient;

    CompositeRedirectSubwindows(window, CompositeRedirectManual);

	dHackP ("E");
    return ret;
}

static int
xwl_destroy_window (WindowPtr window)
{
	ScreenPtr screen = window->drawable.pScreen;
	struct xwl_screen *xwl_screen;
	Bool ret;
	
	if (window->parent == NULL)
		CompositeUnRedirectSubwindows (window, CompositeRedirectManual);
	
	xwl_screen = xwl_screen_get(screen);
	
	screen->DestroyWindow = xwl_screen->DestroyWindow;
	ret = (*screen->DestroyWindow)(window);
	xwl_screen->DestroyWindow = screen->DestroyWindow;
	screen->DestroyWindow = xwl_destroy_window;
	
	struct xwl_window *xwl_window = dixLookupPrivate(&window->devPrivates, &xwl_window_private_key);
	
	wl_surface_destroy(xwl_window->surface);
	xorg_list_del(&xwl_window->link);
	xorg_list_del(&xwl_window->link_damage);
	
	dixSetPrivate(&window->devPrivates, &xwl_window_private_key, NULL);
	
	free(xwl_window);
	
	
	return ret;
}

static void
damage_report(DamagePtr pDamage, RegionPtr pRegion, void *data)
{
//	xf86DrvMsgVerb(0, X_INFO, 0, "AEUEUEOUUEAEUEUEOUUEAEUEUEOUUE  %s\n", __FUNCTION__);
	struct xwl_window *xwl_window = data;
	struct xwl_screen *xwl_screen = xwl_window->xwl_screen;
	
//	dHackP();
	xorg_list_add(&xwl_window->link_damage, &xwl_screen->damage_window_list);
//	dHackP("E");
	
//	xf86DrvMsgVerb(0, X_INFO, 0, "AEUEUEOUUEAEUEUEOUUEAEUEUEOUUE  %s E\n", __FUNCTION__);
}

static void
damage_destroy(DamagePtr pDamage, void *data)
{
}

static Bool
xwl_realize_window(WindowPtr window)
{
	dHackP ("window id %d\n", window->drawable.id);
	ScreenPtr screen = window->drawable.pScreen;
	struct xwl_screen *xwl_screen;
	struct xwl_window *xwl_window;
	Bool ret;

	xwl_screen = xwl_screen_get(screen);

	screen->RealizeWindow = xwl_screen->RealizeWindow;
	ret = (*screen->RealizeWindow)(window);
	xwl_screen->RealizeWindow = xwl_screen->RealizeWindow;
	screen->RealizeWindow = xwl_realize_window;
	
	if (xwl_screen->flags & XWL_FLAGS_ROOTLESS) {
		if (window->redirectDraw != RedirectDrawManual) {
			dHackP ("E  != RedirectDrawManual");
			return ret;
		}
	} else {
		if (window->parent) {
			dHackP ("E  window->parent");
			return ret;
		}
	}
	xwl_window = dixLookupPrivate(&window->devPrivates, &xwl_window_private_key);
	if (!xwl_window) {
		ErrorF("xwl_realize_window dixLookupPrivate failed\n");
		return FALSE;
	}
	if (xwl_window->surface == NULL) {
		ErrorF("xwl_realize_window !xwl_window->surface\n");
		return FALSE;
	}
/*	xwl_window = calloc(sizeof *xwl_window, 1);
	xwl_window->xwl_screen = xwl_screen;
	xwl_window->window = window;
	xwl_window->surface = wl_compositor_create_surface(xwl_screen->compositor);
	
	if (xwl_window->surface == NULL) {
		ErrorF("wl_display_create_surface failed\n");
		return FALSE;
	}
	wl_surface_set_user_data(xwl_window->surface, xwl_window);
	dixSetPrivate(&window->devPrivates, &xwl_window_private_key, xwl_window);*/
	
//	if (xwl_screen->xorg_server) {
//		dHackP ("E  xserver_set_window_id");
//		xserver_set_window_id(xwl_screen->xorg_server, xwl_window->surface, window->drawable.id);
//	}
	
	/* we need to wait in order to avoid a race with Weston WM, when it would
	* try to anticipate XCB_MAP_NOTIFY, requiring create_surface completed
	* (for shell_surface_set_toplevel) */
//	wl_display_roundtrip(xwl_screen->display);
//	wl_display_flush(xwl_screen->display);
	
	dHackP ("xwl_window_attach");
	xwl_window_attach(xwl_window, (*screen->GetWindowPixmap)(window));
	dHackP ("xwl_window_attach A");
	
//	wl_display_roundtrip(xwl_screen->display);
//	wl_display_flush(xwl_screen->display);
	
	
	
	dHackP ("9");
	
	dHackP ("10");
	
	xwl_window->damage = DamageCreate(damage_report, damage_destroy, DamageReportNonEmpty, FALSE, screen, xwl_window);
	DamageRegister(&window->drawable, xwl_window->damage);

	dHackP ("11");
//	xorg_list_add(&xwl_window->link, &xwl_screen->window_list);
//	xorg_list_init(&xwl_window->link_damage);
	
	if (xwl_screen->shell) {
		dHackP ("xwl_screen->shell");
		xwl_window->shsurf = wl_shell_get_shell_surface (xwl_screen->shell, xwl_window->surface);
		dHackP ("xwl_window 0x%lx", xwl_window);
		dHackP ("xwl_screen->shell 0x%lx", xwl_window->shsurf);
		
		if (xwl_window->shsurf) {
			dHackP ("wl_shell_surface_set_user_data");
			wl_shell_surface_set_user_data(xwl_window->shsurf, xwl_window);
			wl_shell_surface_add_listener(xwl_window->shsurf, &xwl_shell_surface_listener, xwl_window);
		}
	//	wl_shell_surface_set_toplevel(xwl_window->shsurf);
	//	wl_shell_surface_set_transient(xwl_window->shsurf);
	/*	if (window->parent) {
			dHackP ("window->parent");
			struct xwl_window* xwl_parent = dixLookupPrivate(&window->parent->devPrivates, &xwl_window_private_key);
			if (xwl_parent) {
				dHackP ("xwl_parent");
				wl_shell_surface_set_transient(xwl_window->shsurf, xwl_parent->shsurf, window->drawable.x, window->drawable.y, 0);
			}else
				wl_shell_surface_set_toplevel(xwl_window->shsurf);
		}else/**/
			wl_shell_surface_set_toplevel(xwl_window->shsurf);
	}
//	wl_display_roundtrip(xwl_screen->display);
//	xwl_window_attach(xwl_window, (*screen->GetWindowPixmap)(window));
	wl_surface_attach(xwl_window->surface, xwl_window->buffer, 0, 0);
	
//	wl_display_roundtrip(xwl_screen->display);
	dHackP ("E");
    return ret;
}

static Bool
xwl_unrealize_window(WindowPtr window)
{
	dHackP ("window id %d\n", window->drawable.id);
	ScreenPtr screen = window->drawable.pScreen;
	struct xwl_screen *xwl_screen;
	struct xwl_window *xwl_window;
	struct xwl_input_device *xwl_input_device;
	Bool ret;

	xwl_screen = xwl_screen_get(screen);

	xorg_list_for_each_entry(xwl_input_device, &xwl_screen->input_device_list, link) {
		if (!xwl_input_device->focus_window)
			continue ;
		if (xwl_input_device->focus_window->window == window) {
			xwl_input_device->focus_window = NULL;
			SetDeviceRedirectWindow(xwl_input_device->pointer, PointerRootWin);
		}
	}

	screen->UnrealizeWindow = xwl_screen->UnrealizeWindow;
	ret = (*screen->UnrealizeWindow)(window);
	xwl_screen->UnrealizeWindow = screen->UnrealizeWindow;
	screen->UnrealizeWindow = xwl_unrealize_window;

	xwl_window = dixLookupPrivate(&window->devPrivates, &xwl_window_private_key);
	if (!xwl_window)
		return ret;

	if (xwl_window->shsurf)
		wl_shell_surface_destroy(xwl_window->shsurf);

	if (xwl_window->buffer)
		wl_buffer_destroy(xwl_window->buffer);
	/*
	wl_surface_destroy(xwl_window->surface);
	xorg_list_del(&xwl_window->link);
	xorg_list_del(&xwl_window->link_damage);*/

	if (xwl_window->damage) {
		DamageUnregister(&window->drawable, xwl_window->damage);
		DamageDestroy(xwl_window->damage);
	}
//	free(xwl_window);
//	dixSetPrivate(&window->devPrivates, &xwl_window_private_key, NULL);
	
	return ret;
}

static void
xwl_set_window_pixmap(WindowPtr window, PixmapPtr pixmap)
{
//	xf86DrvMsgVerb(0, X_INFO, 0, "AEUEUEOUUEAEUEUEOUUEAEUEUEOUUE  %s %lx\n", __FUNCTION__, pixmap);
//	xf86DrvMsgVerb(0, X_INFO, 0, "AEUEUEOUUEAEUEUEOUUEAEUEUEOUUE  %d %d\n", pixmap->drawable.width, pixmap->drawable.height);
    ScreenPtr screen = window->drawable.pScreen;
    struct xwl_screen *xwl_screen;
    struct xwl_window *xwl_window;

    xwl_screen = xwl_screen_get(screen);

    screen->SetWindowPixmap = xwl_screen->SetWindowPixmap;
    (*screen->SetWindowPixmap)(window, pixmap);
    xwl_screen->SetWindowPixmap = screen->SetWindowPixmap;
    screen->SetWindowPixmap = xwl_set_window_pixmap;
	
    xwl_window =
	dixLookupPrivate(&window->devPrivates, &xwl_window_private_key);
    if (xwl_window)
	xwl_window_attach(xwl_window, pixmap);
}

static void
xwl_move_window(WindowPtr window, int x, int y,
		   WindowPtr sibling, VTKind kind)
{
	xf86DrvMsgVerb(0, X_INFO, 0, "AEUEUEOUUEAEUEUEOUUEAEUEUEOUUE  %s\n", __FUNCTION__);
    ScreenPtr screen = window->drawable.pScreen;
    struct xwl_screen *xwl_screen;
    struct xwl_window *xwl_window;

    xwl_screen = xwl_screen_get(screen);

    screen->MoveWindow = xwl_screen->MoveWindow;
    (*screen->MoveWindow)(window, x, y, sibling, kind);
    xwl_screen->MoveWindow = screen->MoveWindow;
    screen->MoveWindow = xwl_move_window;

    xwl_window =
	dixLookupPrivate(&window->devPrivates, &xwl_window_private_key);
    if (xwl_window == NULL)
	return;
}

int
xwl_screen_init_window(struct xwl_screen *xwl_screen, ScreenPtr screen)
{
    if (!dixRegisterPrivateKey(&xwl_window_private_key, PRIVATE_WINDOW, 0))
	return BadAlloc;

    xwl_screen->CreateWindow = screen->CreateWindow;
    screen->CreateWindow = xwl_create_window;

    xwl_screen->DestroyWindow = screen->DestroyWindow;
    screen->DestroyWindow = xwl_destroy_window;

    xwl_screen->RealizeWindow = screen->RealizeWindow;
    screen->RealizeWindow = xwl_realize_window;

    xwl_screen->UnrealizeWindow = screen->UnrealizeWindow;
    screen->UnrealizeWindow = xwl_unrealize_window;

    xwl_screen->SetWindowPixmap = screen->SetWindowPixmap;
    screen->SetWindowPixmap = xwl_set_window_pixmap;

    xwl_screen->MoveWindow = screen->MoveWindow;
    screen->MoveWindow = xwl_move_window;

    return Success;
}
