/* Copyright (c) Dmitry "Leo" Kuznetsov 2021-24 see LICENSE for details */
#include "ut/ut.h"
#include "ui/ui.h"

#pragma push_macro("ui_caption_glyph_rest")
#pragma push_macro("ui_caption_glyph_menu")
#pragma push_macro("ui_caption_glyph_dark")
#pragma push_macro("ui_caption_glyph_light")
#pragma push_macro("ui_caption_glyph_mini")
#pragma push_macro("ui_caption_glyph_maxi")
#pragma push_macro("ui_caption_glyph_full")
#pragma push_macro("ui_caption_glyph_quit")

#define ui_caption_glyph_rest  ut_glyph_desktop_window
#define ui_caption_glyph_menu  ut_glyph_trigram_for_heaven
#define ui_caption_glyph_dark  ut_glyph_crescent_moon
#define ui_caption_glyph_light ut_glyph_white_sun_with_rays
#define ui_caption_glyph_mini  ut_glyph_minimize
#define ui_caption_glyph_maxi  ut_glyph_maximize
#define ui_caption_glyph_full  ut_glyph_square_four_corners
#define ui_caption_glyph_quit  ut_glyph_cancellation_x

static void ui_caption_toggle_full(void) {
    ui_app.full_screen(!ui_app.is_full_screen);
    ui_caption.view.hidden = ui_app.is_full_screen;
    ui_app.request_layout();
}

static void ui_caption_esc_full_screen(ui_view_t* v, const char utf8[]) {
    swear(v == ui_caption.view.parent);
    // TODO: inside ui_app.c instead of here?
    if (utf8[0] == 033 && ui_app.is_full_screen) { ui_caption_toggle_full(); }
}

static void ui_caption_quit(ui_button_t* unused(b)) {
    ui_app.close();
}

static void ui_caption_mini(ui_button_t* unused(b)) {
    ui_app.show_window(ui.visibility.minimize);
}

static void ui_caption_mode_appearance(void) {
    if (ui_theme.is_app_dark()) {
        ui_view.set_text(&ui_caption.mode, "%s", ui_caption_glyph_light);
        ut_str_printf(ui_caption.mode.hint, "%s", ut_nls.str("Switch to Light Mode"));
    } else {
        ui_view.set_text(&ui_caption.mode, "%s", ui_caption_glyph_dark);
        ut_str_printf(ui_caption.mode.hint, "%s", ut_nls.str("Switch to Dark Mode"));
    }
}

static void ui_caption_mode(ui_button_t* unused(b)) {
    bool was_dark = ui_theme.is_app_dark();
    ui_app.light_mode =  was_dark;
    ui_app.dark_mode  = !was_dark;
    ui_theme.refresh();
    ui_caption_mode_appearance();
}

static void ui_caption_maximize_or_restore(void) {
    ui_view.set_text(&ui_caption.maxi, "%s",
        ui_app.is_maximized() ?
        ui_caption_glyph_rest : ui_caption_glyph_maxi);
    ut_str_printf(ui_caption.maxi.hint, "%s",
        ui_app.is_maximized() ?
        ut_nls.str("Restore") : ut_nls.str("Maximize"));
}

static void ui_caption_maxi(ui_button_t* unused(b)) {
    if (!ui_app.is_maximized()) {
        ui_app.show_window(ui.visibility.maximize);
    } else if (ui_app.is_maximized() || ui_app.is_minimized()) {
        ui_app.show_window(ui.visibility.restore);
    }
    ui_caption_maximize_or_restore();
}

static void ui_caption_full(ui_button_t* unused(b)) {
    ui_caption_toggle_full();
}

static int64_t ui_caption_hit_test(ui_view_t* v, int32_t x, int32_t y) {
    swear(v == &ui_caption.view);
    ui_point_t pt = { x, y };
//  traceln("%d,%d ui_caption.icon: %d,%d %dx%d inside: %d",
//      x, y,
//      ui_caption.icon.x, ui_caption.icon.y,
//      ui_caption.icon.w, ui_caption.icon.h,
//      ui_view.inside(&ui_caption.icon, &pt));
    if (ui_app.is_full_screen) {
        return ui.hit_test.client;
    } else if (!ui_caption.icon.hidden &&
                ui_view.inside(&ui_caption.icon, &pt)) {
        return ui.hit_test.system_menu;
    } else {
        ui_view_for_each(&ui_caption.view, c, {
            bool ignore = c->type == ui_view_container ||
                          c->type == ui_view_spacer ||
                          c->type == ui_view_label;
            if (!ignore && ui_view.inside(c, &pt)) {
                return ui.hit_test.client;
            }
        });
        return ui.hit_test.caption;
    }
}

static ui_color_t ui_caption_color(void) {
    ui_color_t c = ui_app.is_active() ?
        ui_colors.get_color(ui_color_id_active_title) :
        ui_colors.get_color(ui_color_id_inactive_title);
    return c;
}

static void ui_caption_button_measure(ui_view_t* v) {
    assert(v->type == ui_view_button);
    v->w = ui_app.caption_height;
    v->h = ui_app.caption_height;
//  traceln("%dx%d", v->w, v->h);
}

static void ui_caption_button_icon_paint(ui_view_t* v) {
    int32_t w = v->w;
    int32_t h = v->h;
//  swear(w == h && h == ui_app.caption_height);
    while (h > 16 && (h & (h - 1)) != 0) { h--; }
    w = h;
    int32_t dx = (v->w - w) / 2;
    int32_t dy = (v->h - h) / 2;
    ui_gdi.icon(v->x + dx, v->y + dy, w, h, v->icon);
}

static void ui_caption_prepare(ui_view_t* unused(v)) {
    ui_caption.title.hidden = false;
}

static void ui_caption_measured(ui_view_t* v) {
    // do not show title if there is not enough space
    ui_caption.title.hidden = v->w > ui_app.root->w;
    v->w = ui_app.root->w;
    const ui_ltrb_t insets = ui_view.gaps(v, &v->insets);
    v->h = insets.top + ui_app.caption_height + insets.bottom;
}

static void ui_caption_composed(ui_view_t* v) {
    v->x = ui_app.root->x;
    v->y = ui_app.root->y;
}

static void ui_caption_paint(ui_view_t* v) {
    ui_color_t background = ui_caption_color();
    ui_gdi.fill(v->x, v->y, v->w, v->h, background);
}

static void ui_caption_init(ui_view_t* v) {
    swear(v == &ui_caption.view, "caption is a singleton");
    ui_view_init_span(v);
    ui_caption.view.insets = (ui_gaps_t){ 0.125, 0.25, 0.125, 0.25 };
    ui_caption.view.hidden = false;
    v->parent->character = ui_caption_esc_full_screen; // ESC for full screen
    ui_view.add(&ui_caption.view,
        &ui_caption.icon,
        &ui_caption.menu,
        &ui_caption.title,
        &ui_caption.spacer,
        &ui_caption.mode,
        &ui_caption.mini,
        &ui_caption.maxi,
        &ui_caption.full,
        &ui_caption.quit,
        null);
    ui_caption.view.color_id = ui_color_id_window_text;
    static const ui_gaps_t p0 = { .left  = 0.0,   .top    = 0.0,
                                  .right = 0.0,   .bottom = 0.0};
    static const ui_gaps_t pd = { .left  = 0.25,  .top    = 0.0,
                                  .right = 0.25,  .bottom = 0.0};
    static const ui_gaps_t pb = { .left  = 0.25,  .top    = 0.0,
                                  .right = 0.25,  .bottom = 0.0};
    static const ui_gaps_t in = { .left  = 0.0,   .top    = 0.0,
                                  .right = 0.0,   .bottom = 0.0};
    ui_view_for_each(&ui_caption.view, c, {
        c->fm = &ui_app.fm.regular;
        c->color_id = ui_caption.view.color_id;
        if (c->type == ui_view_button) {
            c->padding = pb;
            c->flat = true;
            c->measure = ui_caption_button_measure;
        } else {
            c->padding = pd;
        }
        c->insets  = in;
        c->h = ui_app.caption_height;
        c->min_w_em = 0.5f;
        c->min_h_em = 0.5f;
    });
    strprintf(ui_caption.menu.hint, "%s", ut_nls.str("Menu"));
    strprintf(ui_caption.mode.hint, "%s", ut_nls.str("Switch to Light Mode"));
    strprintf(ui_caption.mini.hint, "%s", ut_nls.str("Minimize"));
    strprintf(ui_caption.maxi.hint, "%s", ut_nls.str("Maximize"));
    strprintf(ui_caption.full.hint, "%s", ut_nls.str("Full Screen (ESC to restore)"));
    strprintf(ui_caption.quit.hint, "%s", ut_nls.str("Close"));
    ui_caption.icon.icon = ui_app.icon;
    ui_caption.icon.padding = p0;
    ui_caption.icon.paint = ui_caption_button_icon_paint;
    ui_caption.view.align = ui.align.left;
    // TODO: this does not help because parent layout will set x and w again
    ui_caption.view.prepare = ui_caption_prepare;
    ui_caption.view.measured = ui_caption_measured;
    ui_caption.view.composed = ui_caption_composed;
    ui_view.set_text(&ui_caption.view, "ui_caption"); // for debugging
    ui_caption_maximize_or_restore();
    ui_caption.view.paint = ui_caption_paint;
    ui_caption_mode_appearance();
}

ui_caption_t ui_caption =  {
    .view = {
        .type     = ui_view_span,
        .fm       = &ui_app.fm.regular,
        .init     = ui_caption_init,
        .hit_test = ui_caption_hit_test,
        .hidden = true
    },
    .icon   = ui_button(ut_glyph_nbsp, 0.0, null),
    .title  = ui_label(0, ""),
    .spacer = ui_view(spacer),
    .menu   = ui_button(ui_caption_glyph_menu, 0.0, null),
    .mode   = ui_button(ui_caption_glyph_mini, 0.0, ui_caption_mode),
    .mini   = ui_button(ui_caption_glyph_mini, 0.0, ui_caption_mini),
    .maxi   = ui_button(ui_caption_glyph_maxi, 0.0, ui_caption_maxi),
    .full   = ui_button(ui_caption_glyph_full, 0.0, ui_caption_full),
    .quit   = ui_button(ui_caption_glyph_quit, 0.0, ui_caption_quit),
};

#pragma pop_macro("ui_caption_glyph_rest")
#pragma pop_macro("ui_caption_glyph_menu")
#pragma pop_macro("ui_caption_glyph_dark")
#pragma pop_macro("ui_caption_glyph_light")
#pragma pop_macro("ui_caption_glyph_mini")
#pragma pop_macro("ui_caption_glyph_maxi")
#pragma pop_macro("ui_caption_glyph_full")
#pragma pop_macro("ui_caption_glyph_quit")
