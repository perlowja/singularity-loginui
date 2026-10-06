#include "loginui.h"
#include "internal.h"

#include <math.h>
#include <stdlib.h>
#include <gio/gio.h>

cairo_surface_t *loginui_load_brand_logo(void) {
    const char *path = getenv("SINGULARITY_SPLASH_LOGO");
    if (!path || !path[0]) path = LOGINUI_BRAND_LOGO_PATH;
    return loginui_load_image(path, LOGINUI_BRAND_LOGO_SIZE, LOGINUI_BRAND_LOGO_SIZE);
}

void loginui_render_brand(cairo_t *cr, int width, int height,
                          cairo_surface_t *logo, double track_alpha,
                          double phase) {
    double w = width, h = height;
    cairo_save(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_set_source_rgb(cr, LOGINUI_BRAND_RED, LOGINUI_BRAND_GREEN, LOGINUI_BRAND_BLUE);
    cairo_paint(cr);

    double size = LOGINUI_BRAND_LOGO_SIZE;
    double top = floor((h - size) / 2.0);
    if (logo) {
        int lw = cairo_image_surface_get_width(logo);
        int lh = cairo_image_surface_get_height(logo);
        if (lw > 0 && lh > 0) {
            cairo_save(cr);
            cairo_translate(cr, floor((w - size) / 2.0), top);
            cairo_scale(cr, size / lw, size / lh);
            cairo_set_source_surface(cr, logo, 0, 0);
            cairo_paint(cr);
            cairo_restore(cr);
        }
    }

    if (track_alpha > 0.0) {
        double track_w = 220, track_h = 4, r = track_h / 2.0;
        double track_x = w / 2.0 - track_w / 2.0;
        double track_y = top + size + 48;
        lui_rounded_rect(cr, track_x, track_y, track_w, track_h, r);
        cairo_set_source_rgba(cr, 1, 1, 1, 0.12 * track_alpha);
        cairo_fill(cr);
        double seg_w = track_w * 0.30;
        double t = 0.5 + 0.5 * sin(phase * 1.6);
        lui_rounded_rect(cr, track_x + t * (track_w - seg_w), track_y, seg_w, track_h, r);
        cairo_set_source_rgba(cr, 1, 1, 1, 0.85 * track_alpha);
        cairo_fill(cr);
    }
    cairo_restore(cr);
}

bool loginui_reduced_motion(void) {
    const char *forced = getenv("SINGULARITY_REDUCE_MOTION");
    if (forced && forced[0]) return forced[0] == '1';
    const char *schema_id = getenv("SINGULARITY_DESKTOP_SCHEMA");
    if (!schema_id || !schema_id[0]) schema_id = "dev.sinty.desktop";
    GSettingsSchemaSource *source = g_settings_schema_source_get_default();
    if (!source) return false;
    GSettingsSchema *schema = g_settings_schema_source_lookup(source, schema_id, TRUE);
    if (!schema) return false;
    bool reduced = false;
    if (g_settings_schema_has_key(schema, "reduce-motion")) {
        GSettings *settings = g_settings_new_full(schema, NULL, NULL);
        reduced = g_settings_get_boolean(settings, "reduce-motion");
        g_object_unref(settings);
    }
    g_settings_schema_unref(schema);
    return reduced;
}

double loginui_scene_seconds(void) {
    return (loginui_reduced_motion() ? LOGINUI_MOTION_SMALL_MS : LOGINUI_MOTION_SCENE_MS) / 1000.0;
}
