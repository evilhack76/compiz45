/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */

/*
 * Copyright (C) 2006 Novell, Inc.
 * Copyright (C) 2010 Sam Spilsbury
 * Copyright (C) 2011 Canonical Ltd.
 * Copyright (C) 2016 Alberts Muktupāvels
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *     Alberts Muktupāvels <alberts.muktupavels@gmail.com>
 *     David Reveman <davidr@novell.com>
 *     Sam Spilsbury <smspillaz@gmail.com>
 */

#include "config.h"
#include "gtk-window-decorator.h"
#include "gwd-cairo-window-decoration-util.h"
#include "gwd-theme-cairo.h"

#define STROKE_ALPHA 0.6f

struct _GWDThemeCairo
{
    GObject parent;
};

G_DEFINE_TYPE (GWDThemeCairo, gwd_theme_cairo, GWD_TYPE_THEME)

static void
button_state_offsets (gdouble  x,
                      gdouble  y,
                      guint    state,
                      gdouble *return_x,
                      gdouble *return_y)
{
    static gdouble off[]	= { 0.0, 0.0, 0.0, 0.5 };

    *return_x  = x + off[state];
    *return_y  = y + off[state];
}

static void
button_state_paint (cairo_t         *cr,
                    GtkStyleContext *context,
                    decor_color_t   *color,
                    guint            state)
{
    GdkRGBA fg;

#define IN_STATE (PRESSED_EVENT_WINDOW | IN_EVENT_WINDOW)

    gtk_style_context_save (context);
    gtk_style_context_set_state (context, GTK_STATE_FLAG_NORMAL);
    gtk_style_context_get_color (context, GTK_STATE_FLAG_NORMAL, &fg);
    gtk_style_context_restore (context);

    fg.alpha = STROKE_ALPHA;

    if ((state & IN_STATE) == IN_STATE) {
        if (state & IN_EVENT_WINDOW)
            cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
        else
            cairo_set_source_rgba (cr, color->r, color->g, color->b, 0.95);

        cairo_fill_preserve (cr);

        gdk_cairo_set_source_rgba (cr, &fg);

        cairo_set_line_width (cr, 1.0);
        cairo_stroke (cr);
        cairo_set_line_width (cr, 2.0);
    } else {
        gdk_cairo_set_source_rgba (cr, &fg);
        cairo_stroke_preserve (cr);

        if (state & IN_EVENT_WINDOW)
            cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
        else
            cairo_set_source_rgba (cr, color->r, color->g, color->b, 0.95);

        cairo_fill (cr);
    }
}

static void
draw_close_button (cairo_t *cr,
                   gdouble  s)
{
    cairo_rel_move_to (cr, 0.0, s);

    cairo_rel_line_to (cr, s, -s);
    cairo_rel_line_to (cr, s, s);
    cairo_rel_line_to (cr, s, -s);
    cairo_rel_line_to (cr, s, s);

    cairo_rel_line_to (cr, -s, s);
    cairo_rel_line_to (cr, s, s);
    cairo_rel_line_to (cr, -s, s);
    cairo_rel_line_to (cr, -s, -s);

    cairo_rel_line_to (cr, -s, s);
    cairo_rel_line_to (cr, -s, -s);
    cairo_rel_line_to (cr, s, -s);

    cairo_close_path (cr);
}

static void
draw_max_button (cairo_t *cr,
                 gdouble  s)
{
    cairo_rel_line_to (cr, 12.0, 0.0);
    cairo_rel_line_to (cr, 0.0, 12.0);
    cairo_rel_line_to (cr, -12.0, 0.0);

    cairo_close_path (cr);

    cairo_rel_move_to (cr, 2.0, s);

    cairo_rel_line_to (cr, 12.0 - 4.0, 0.0);
    cairo_rel_line_to (cr, 0.0, 12.0 - s - 2.0);
    cairo_rel_line_to (cr, -(12.0 - 4.0), 0.0);

    cairo_close_path (cr);
}

static void
draw_unmax_button (cairo_t *cr,
                   gdouble  s)
{
    cairo_rel_move_to (cr, 1.0, 1.0);

    cairo_rel_line_to (cr, 10.0, 0.0);
    cairo_rel_line_to (cr, 0.0, 10.0);
    cairo_rel_line_to (cr, -10.0, 0.0);

    cairo_close_path (cr);

    cairo_rel_move_to (cr, 2.0, s);

    cairo_rel_line_to (cr, 10.0 - 4.0, 0.0);
    cairo_rel_line_to (cr, 0.0, 10.0 - s - 2.0);
    cairo_rel_line_to (cr, -(10.0 - 4.0), 0.0);

    cairo_close_path (cr);
}

static void
draw_min_button (cairo_t *cr,
                 gdouble  s)
{
    cairo_rel_move_to (cr, 0.0, 8.0);

    cairo_rel_line_to (cr, 12.0, 0.0);
    cairo_rel_line_to (cr, 0.0, s);
    cairo_rel_line_to (cr, -12.0, 0.0);

    cairo_close_path (cr);
}

static void
calc_button_size (decor_t *decor)
{
    gint button_width = 0;

    if (decor->actions & WNCK_WINDOW_ACTION_CLOSE)
        button_width += 17;

    if (decor->actions & (WNCK_WINDOW_ACTION_MAXIMIZE_HORIZONTALLY |
                          WNCK_WINDOW_ACTION_MAXIMIZE_VERTICALLY |
                          WNCK_WINDOW_ACTION_UNMAXIMIZE_HORIZONTALLY |
                          WNCK_WINDOW_ACTION_UNMAXIMIZE_VERTICALLY))
        button_width += 17;

    if (decor->actions & WNCK_WINDOW_ACTION_MINIMIZE)
        button_width += 17;

    if (button_width)
        ++button_width;

    decor->button_width = button_width;
}

static gboolean
button_present (decor_t *decor,
                gint     i)
{
    switch (i) {
        case BUTTON_MIN:
            if (decor->actions & WNCK_WINDOW_ACTION_MINIMIZE)
                return TRUE;
            break;

        case BUTTON_MAX:
            if (decor->actions & WNCK_WINDOW_ACTION_MAXIMIZE)
                return TRUE;
            break;

        case BUTTON_CLOSE:
            if (decor->actions & WNCK_WINDOW_ACTION_CLOSE)
                return TRUE;
            break;

        case BUTTON_MENU:
        case BUTTON_SHADE:
        case BUTTON_ABOVE:
        case BUTTON_STICK:
        case BUTTON_UNSHADE:
        case BUTTON_UNABOVE:
        case BUTTON_UNSTICK:
            break;

        default:
            break;
    }

    return FALSE;
}

static void
gwd_theme_cairo_draw_window_decoration (GWDTheme *theme,
                                        decor_t  *decor)
{
    cairo_t *cr;
    GtkStyleContext *context;
    GdkRGBA bg, fg;
    cairo_surface_t *surface;
    decor_color_t color;
    gdouble alpha;
    gdouble x1, y1, x2, y2, x, y, h;
    gint corners = SHADE_LEFT | SHADE_RIGHT | SHADE_TOP | SHADE_BOTTOM;
    gint top;
    gint button_x;

    if (!decor->surface)
        return;

    context = gtk_widget_get_style_context (decor->frame->style_window_rgba);

    gtk_style_context_save (context);
    gtk_style_context_set_state (context, GTK_STATE_FLAG_NORMAL);
    gtk_style_context_get_background_color (context, GTK_STATE_FLAG_NORMAL, &bg);
    gtk_style_context_get_color (context, GTK_STATE_FLAG_NORMAL, &fg);
    gtk_style_context_restore (context);

    if (decor->state & (WNCK_WINDOW_STATE_MAXIMIZED_HORIZONTALLY |
                        WNCK_WINDOW_STATE_MAXIMIZED_VERTICALLY))
        corners = 0;

    color.r = bg.red;
    color.g = bg.green;
    color.b = bg.blue;

    if (decor->buffer_surface)
        surface = decor->buffer_surface;
    else
        surface = decor->surface;

    cr = cairo_create (surface);

    if (!cr)
        return;

    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);

    top = decor->frame->win_extents.top + decor->frame->titlebar_height;

    x1 = decor->context->left_space - decor->frame->win_extents.left;
    y1 = decor->context->top_space - decor->frame->win_extents.top - decor->frame->titlebar_height;
    x2 = decor->width - decor->context->right_space + decor->frame->win_extents.right;
    y2 = decor->height - decor->context->bottom_space + decor->frame->win_extents.bottom;

    h = decor->height - decor->context->top_space - decor->context->bottom_space;

    cairo_set_line_width (cr, 1.0);

    if (!decor->frame_window)
        draw_shadow_background (decor, cr, decor->shadow, decor->context);

    if (decor->active) {
        decor_color_t *title_color = _title_color;

        alpha = decoration_alpha + 0.3;

        fill_rounded_rectangle (cr,
                                x1 + 0.5,
                                y1 + 0.5,
                                decor->frame->win_extents.left - 0.5,
                                top - 0.5,
                                5.0, CORNER_TOPLEFT & corners,
                                &title_color[0], 1.0, &title_color[1], alpha,
                                SHADE_TOP | SHADE_LEFT);

        fill_rounded_rectangle (cr,
                                x1 + decor->frame->win_extents.left,
                                y1 + 0.5,
                                x2 - x1 - decor->frame->win_extents.left -
                                decor->frame->win_extents.right,
                                top - 0.5,
                                5.0, 0,
                                &title_color[0], 1.0, &title_color[1], alpha,
                                SHADE_TOP);

        fill_rounded_rectangle (cr,
                                x2 - decor->frame->win_extents.right,
                                y1 + 0.5,
                                decor->frame->win_extents.right - 0.5,
                                top - 0.5,
                                5.0, CORNER_TOPRIGHT & corners,
                                &title_color[0], 1.0, &title_color[1], alpha,
                                SHADE_TOP | SHADE_RIGHT);
    } else {
        alpha = decoration_alpha;

        fill_rounded_rectangle (cr,
                                x1 + 0.5,
                                y1 + 0.5,
                                decor->frame->win_extents.left - 0.5,
                                top - 0.5,
                                5.0, CORNER_TOPLEFT & corners,
                                &color, 1.0, &color, alpha,
                                SHADE_TOP | SHADE_LEFT);

        fill_rounded_rectangle (cr,
                                x1 + decor->frame->win_extents.left,
                                y1 + 0.5,
                                x2 - x1 - decor->frame->win_extents.left -
                                decor->frame->win_extents.right,
                                top - 0.5,
                                5.0, 0,
                                &color, 1.0, &color, alpha,
                                SHADE_TOP);

        fill_rounded_rectangle (cr,
                                x2 - decor->frame->win_extents.right,
                                y1 + 0.5,
                                decor->frame->win_extents.right - 0.5,
                                top - 0.5,
                                5.0, CORNER_TOPRIGHT & corners,
                                &color, 1.0, &color, alpha,
                                SHADE_TOP | SHADE_RIGHT);
    }

    fill_rounded_rectangle (cr,
                            x1 + 0.5,
                            y1 + top,
                            decor->frame->win_extents.left - 0.5,
                            h,
                            5.0, 0,
                            &color, 1.0, &color, alpha,
                            SHADE_LEFT);

    fill_rounded_rectangle (cr,
                            x2 - decor->frame->win_extents.right,
                            y1 + top,
                            decor->frame->win_extents.right - 0.5,
                            h,
                            5.0, 0,
                            &color, 1.0, &color, alpha,
                            SHADE_RIGHT);


    fill_rounded_rectangle (cr,
                            x1 + 0.5,
                            y2 - decor->frame->win_extents.bottom,
                            decor->frame->win_extents.left - 0.5,
                            decor->frame->win_extents.bottom - 0.5,
                            5.0, CORNER_BOTTOMLEFT & corners,
                            &color, 1.0, &color, alpha,
                            SHADE_BOTTOM | SHADE_LEFT);

    fill_rounded_rectangle (cr,
                            x1 + decor->frame->win_extents.left,
                            y2 - decor->frame->win_extents.bottom,
                            x2 - x1 - decor->frame->win_extents.left -
                            decor->frame->win_extents.right,
                            decor->frame->win_extents.bottom - 0.5,
                            5.0, 0,
                            &color, 1.0, &color, alpha,
                            SHADE_BOTTOM);

    fill_rounded_rectangle (cr,
                            x2 - decor->frame->win_extents.right,
                            y2 - decor->frame->win_extents.bottom,
                            decor->frame->win_extents.right - 0.5,
                            decor->frame->win_extents.bottom - 0.5,
                            5.0, CORNER_BOTTOMRIGHT & corners,
                            &color, 1.0, &color, alpha,
                            SHADE_BOTTOM | SHADE_RIGHT);

    cairo_rectangle (cr,
                     decor->context->left_space,
                     decor->context->top_space,
                     decor->width - decor->context->left_space -
                     decor->context->right_space,
                     h);
    gdk_cairo_set_source_rgba (cr, &bg);
    cairo_fill (cr);

    cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

    if (decor->active) {
        fg.alpha = 0.7;
        gdk_cairo_set_source_rgba (cr, &fg);

        cairo_move_to (cr, x1 + 0.5, y1 + top - 0.5);
        cairo_rel_line_to (cr, x2 - x1 - 1.0, 0.0);

        cairo_stroke (cr);
    }

    rounded_rectangle (cr,
                       x1 + 0.5, y1 + 0.5,
                       x2 - x1 - 1.0, y2 - y1 - 1.0,
                       5.0,
                       (CORNER_TOPLEFT | CORNER_TOPRIGHT | CORNER_BOTTOMLEFT |
                        CORNER_BOTTOMRIGHT) & corners);

    cairo_clip (cr);

    cairo_translate (cr, 1.0, 1.0);

    rounded_rectangle (cr,
                       x1 + 0.5, y1 + 0.5,
                       x2 - x1 - 1.0, y2 - y1 - 1.0,
                       5.0,
                       (CORNER_TOPLEFT | CORNER_TOPRIGHT | CORNER_BOTTOMLEFT |
                        CORNER_BOTTOMRIGHT) & corners);

    cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.4);

    cairo_stroke (cr);

    cairo_translate (cr, -2.0, -2.0);

    rounded_rectangle (cr,
                       x1 + 0.5, y1 + 0.5,
                       x2 - x1 - 1.0, y2 - y1 - 1.0,
                       5.0,
                       (CORNER_TOPLEFT | CORNER_TOPRIGHT | CORNER_BOTTOMLEFT |
                        CORNER_BOTTOMRIGHT) & corners);

    cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 0.1);

    cairo_stroke (cr);

    cairo_translate (cr, 1.0, 1.0);

    cairo_reset_clip (cr);

    rounded_rectangle (cr,
                       x1 + 0.5, y1 + 0.5,
                       x2 - x1 - 1.0, y2 - y1 - 1.0,
                       5.0,
                       (CORNER_TOPLEFT | CORNER_TOPRIGHT | CORNER_BOTTOMLEFT |
                        CORNER_BOTTOMRIGHT) & corners);

    fg.alpha = alpha;
    gdk_cairo_set_source_rgba (cr, &fg);

    cairo_stroke (cr);

    cairo_set_line_width (cr, 2.0);

    button_x = decor->width - decor->context->right_space - 13;

    if (decor->actions & WNCK_WINDOW_ACTION_CLOSE) {
        button_state_offsets (button_x,
                              y1 - 3.0 + decor->frame->titlebar_height / 2,
                              decor->button_states[BUTTON_CLOSE], &x, &y);

        button_x -= 17;

        if (decor->active) {
            cairo_move_to (cr, x, y);
            draw_close_button (cr, 3.0);
            button_state_paint (cr, context, &color,
                                decor->button_states[BUTTON_CLOSE]);
        } else {
            fg.alpha = alpha * 0.75;
            gdk_cairo_set_source_rgba (cr, &fg);

            cairo_move_to (cr, x, y);
            draw_close_button (cr, 3.0);
            cairo_fill (cr);
        }
    }

    if (decor->actions & WNCK_WINDOW_ACTION_MAXIMIZE) {
        button_state_offsets (button_x,
                              y1 - 3.0 + decor->frame->titlebar_height / 2,
                              decor->button_states[BUTTON_MAX], &x, &y);

        button_x -= 17;

        cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);

        if (decor->active) {
            fg.alpha = STROKE_ALPHA;
            gdk_cairo_set_source_rgba (cr, &fg);

            cairo_move_to (cr, x, y);

            if (decor->state & (WNCK_WINDOW_STATE_MAXIMIZED_HORIZONTALLY |
                                WNCK_WINDOW_STATE_MAXIMIZED_VERTICALLY))
                draw_unmax_button (cr, 4.0);
            else
                draw_max_button (cr, 4.0);

            button_state_paint (cr, context, &color,
                                decor->button_states[BUTTON_MAX]);
        } else {
            fg.alpha = alpha * 0.75;
            gdk_cairo_set_source_rgba (cr, &fg);

            cairo_move_to (cr, x, y);

            if (decor->state & (WNCK_WINDOW_STATE_MAXIMIZED_HORIZONTALLY |
                                WNCK_WINDOW_STATE_MAXIMIZED_VERTICALLY))
                draw_unmax_button (cr, 4.0);
            else
                draw_max_button (cr, 4.0);

            cairo_fill (cr);
        }
    }

    if (decor->actions & WNCK_WINDOW_ACTION_MINIMIZE) {
        button_state_offsets (button_x,
                              y1 - 3.0 + decor->frame->titlebar_height / 2,
                              decor->button_states[BUTTON_MIN], &x, &y);

        button_x -= 17;

        if (decor->active) {
            fg.alpha = STROKE_ALPHA;
            gdk_cairo_set_source_rgba (cr, &fg);

            cairo_move_to (cr, x, y);
            draw_min_button (cr, 4.0);
            button_state_paint (cr, context, &color,
                                decor->button_states[BUTTON_MIN]);
        } else {
            fg.alpha = alpha * 0.75;
            gdk_cairo_set_source_rgba (cr, &fg);

            cairo_move_to (cr, x, y);
            draw_min_button (cr, 4.0);
            cairo_fill (cr);
        }
    }

    if (decor->layout) {
        if (decor->active) {
            cairo_move_to (cr,
                           decor->context->left_space + 21.0,
                           y1 + 2.0 + (decor->frame->titlebar_height - decor->frame->text_height) / 2.0);

            fg.alpha = STROKE_ALPHA;
            gdk_cairo_set_source_rgba (cr, &fg);

            pango_cairo_layout_path (cr, decor->layout);
            cairo_stroke (cr);

            cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
        } else {
            fg.alpha = alpha;
            gdk_cairo_set_source_rgba (cr, &fg);
        }

        cairo_move_to (cr,
                       decor->context->left_space + 21.0,
                        y1 + 2.0 + (decor->frame->titlebar_height - decor->frame->text_height) / 2.0);

        pango_cairo_show_layout (cr, decor->layout);
    }

    if (decor->icon) {
        cairo_translate (cr,
                         decor->context->left_space + 1,
                         y1 - 5.0 + decor->frame->titlebar_height / 2);
        cairo_set_source (cr, decor->icon);
        cairo_rectangle (cr, 0.0, 0.0, 16.0, 16.0);
        cairo_clip (cr);

        if (decor->active)
            cairo_paint (cr);
        else
            cairo_paint_with_alpha (cr, alpha);
    }

    cairo_destroy (cr);

    copy_to_front_buffer (decor);

    if (decor->frame_window) {
        GdkWindow *gdk_frame_window = gtk_widget_get_window (decor->decor_window);
        GdkPixbuf *pixbuf = gdk_pixbuf_get_from_surface (decor->surface, 0, 0,
                                                         decor->width, decor->height);

        gtk_image_set_from_pixbuf (GTK_IMAGE (decor->decor_image), pixbuf);
        g_object_unref (pixbuf);

        gtk_window_resize (GTK_WINDOW (decor->decor_window), decor->width, decor->height);
        gdk_window_move (gdk_frame_window, 0, 0);
        gdk_window_lower (gdk_frame_window);
    }

    if (decor->prop_xid) {
        decor_update_window_property (decor);
        decor->prop_xid = 0;
    }
}

static gboolean
gwd_theme_cairo_calc_decoration_size (GWDTheme *theme,
                                      decor_t  *decor,
                                      gint      w,
                                      gint      h,
                                      gint      name_width,
                                      gint     *width,
                                      gint     *height)
{
    decor_layout_t layout;
    gint top_width;

    if (!decor->decorated)
        return FALSE;

    /* To avoid wasting texture memory, we only calculate the minimal
     * required decoration size then clip and stretch the texture where
     * appropriate
     */

    if (!decor->frame_window) {
        calc_button_size (decor);

        if (w < ICON_SPACE + decor->button_width)
            return FALSE;

        top_width = name_width + decor->button_width + ICON_SPACE;
        if (w < top_width)
            top_width = MAX (ICON_SPACE + decor->button_width, w);

        if (decor->active)
            decor_get_default_layout (&decor->frame->window_context_active, top_width, 1, &layout);
        else
            decor_get_default_layout (&decor->frame->window_context_inactive, top_width, 1, &layout);

        if (!decor->context || memcmp (&layout, &decor->border_layout, sizeof (layout))) {
            *width  = layout.width;
            *height = layout.height;

            decor->border_layout = layout;
            if (decor->active) {
                decor->context = &decor->frame->window_context_active;
                decor->shadow = decor->frame->border_shadow_active;
            } else {
                decor->context = &decor->frame->window_context_inactive;
                decor->shadow = decor->frame->border_shadow_inactive;
            }

            return TRUE;
        }
    } else {
        calc_button_size (decor);

        /* _default_win_extents + top height */

        top_width = name_width + decor->button_width + ICON_SPACE;
        if (w < top_width)
            top_width = MAX (ICON_SPACE + decor->button_width, w);

        decor_get_default_layout (&decor->frame->window_context_no_shadow,
                                  decor->client_width, decor->client_height,
                                  &layout);

        *width = layout.width;
        *height = layout.height;

        decor->border_layout = layout;
        if (decor->active) {
            decor->context = &decor->frame->window_context_active;
            decor->shadow = decor->frame->border_shadow_active;
        } else {
            decor->context = &decor->frame->window_context_inactive;
            decor->shadow = decor->frame->border_shadow_inactive;
        }

        return TRUE;
    }

    return FALSE;
}

static void
gwd_theme_cairo_update_border_extents (GWDTheme      *theme,
                                       decor_frame_t *frame)
{
    frame = gwd_decor_frame_ref (frame);

    gwd_cairo_window_decoration_get_extents (&frame->win_extents,
                                             &frame->max_win_extents);

    frame->titlebar_height = frame->max_titlebar_height =
        (frame->text_height < 17) ? 17 : frame->text_height;

    gwd_decor_frame_unref (frame);
}

static void
gwd_theme_cairo_get_event_window_position (GWDTheme *theme,
                                           decor_t  *decor,
                                           gint      i,
                                           gint      j,
                                           gint      width,
                                           gint      height,
                                           gint     *x,
                                           gint     *y,
                                           gint     *w,
                                           gint     *h)
{
    if (decor->frame_window) {
        *x = pos[i][j].x + pos[i][j].xw * width + decor->frame->win_extents.left;
        *y = pos[i][j].y + decor->frame->win_extents.top +
             pos[i][j].yh * height + pos[i][j].yth * (decor->frame->titlebar_height - 17);

        if (i == 0 && (j == 0 || j == 2))
            *y -= decor->frame->titlebar_height;
    } else {
        *x = pos[i][j].x + pos[i][j].xw * width;
        *y = pos[i][j].y +
             pos[i][j].yh * height + pos[i][j].yth * (decor->frame->titlebar_height - 17);
    }

    if ((decor->state & WNCK_WINDOW_STATE_MAXIMIZED_HORIZONTALLY) && (j == 0 || j == 2)) {
        *w = 0;
    } else {
        *w = pos[i][j].w + pos[i][j].ww * width;
    }

    if ((decor->state & WNCK_WINDOW_STATE_MAXIMIZED_VERTICALLY) && (i == 0 || i == 2)) {
        *h = 0;
    } else {
        *h = pos[i][j].h +
             pos[i][j].hh * height + pos[i][j].hth * (decor->frame->titlebar_height - 17);
    }
}

static gboolean
gwd_theme_cairo_get_button_position (GWDTheme *theme,
                                     decor_t  *decor,
                                     gint      i,
                                     gint      width,
                                     gint      height,
                                     gint     *x,
                                     gint     *y,
                                     gint     *w,
                                     gint     *h)
{
    if (i > BUTTON_MENU)
        return FALSE;

    if (decor->frame_window) {
        *x = bpos[i].x + bpos[i].xw * width + decor->frame->win_extents.left + 4;
        *y = bpos[i].y + bpos[i].yh * height + bpos[i].yth *
             (decor->frame->titlebar_height - 17) + decor->frame->win_extents.top + 2;
    } else {
        *x = bpos[i].x + bpos[i].xw * width;
        *y = bpos[i].y + bpos[i].yh * height + bpos[i].yth *
             (decor->frame->titlebar_height - 17);
    }

    *w = bpos[i].w + bpos[i].ww * width;
    *h = bpos[i].h + bpos[i].hh * height + bpos[i].hth +
         (decor->frame->titlebar_height - 17);

    /* hack to position multiple buttons on the right */
    if (i != BUTTON_MENU) {
        gint position = 0;
        gint button = 0;

        while (button != i) {
            if (button_present (decor, button))
                position++;
            button++;
        }

        *x -= 10 + 16 * position;
    }

    return TRUE;
}

static void
gwd_theme_cairo_class_init (GWDThemeCairoClass *cairo_class)
{
    GWDThemeClass *theme_class = GWD_THEME_CLASS (cairo_class);

    theme_class->draw_window_decoration = gwd_theme_cairo_draw_window_decoration;
    theme_class->calc_decoration_size = gwd_theme_cairo_calc_decoration_size;
    theme_class->update_border_extents = gwd_theme_cairo_update_border_extents;
    theme_class->get_event_window_position = gwd_theme_cairo_get_event_window_position;
    theme_class->get_button_position = gwd_theme_cairo_get_button_position;
}

static void
gwd_theme_cairo_init (GWDThemeCairo *cairo)
{
}
