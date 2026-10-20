#include "loginui.h"
#include "internal.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <gio/gio.h>

cairo_surface_t *loginui_load_brand_logo(void) {
    const char *path = getenv("SINGULARITY_SPLASH_LOGO");
    if (!path || !path[0]) path = LOGINUI_BRAND_LOGO_PATH;
    return loginui_load_image(path, LOGINUI_BRAND_LOGO_SIZE * 2, LOGINUI_BRAND_LOGO_SIZE * 2);
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

void loginui_brand_layout(double width, double height, double scale,
                          LoginUiBrandLayout *out) {
    if (scale <= 0.0) scale = 1.0;
    double pw = width * scale, ph = height * scale;
    double k = ph >= 2000.0 ? 2.0 : (ph >= 1400.0 ? 1.5 : 1.0);
    double size = floor(LOGINUI_BRAND_LOGO_SIZE * k);
    if (size > pw * 0.5) size = floor(pw * 0.5);
    if (size > ph * 0.5) size = floor(ph * 0.5);
    double top = floor((ph - size) / 2.0);
    double left = floor((pw - size) / 2.0);
    double bar_w = floor(size * 220.0 / 256.0);
    double bar_h = fmax(2.0, floor(size / 64.0));
    out->logo_size = size / scale;
    out->logo_x = left / scale;
    out->logo_y = top / scale;
    out->bar_w = bar_w / scale;
    out->bar_h = bar_h / scale;
    out->bar_x = floor((pw - bar_w) / 2.0) / scale;
    out->bar_y = (top + size + floor(size * 48.0 / 256.0)) / scale;
}

void loginui_render_brand_progress(cairo_t *cr, int width, int height,
                                   double scale, cairo_surface_t *logo,
                                   double progress, double bar_alpha) {
    LoginUiBrandLayout l;
    loginui_brand_layout(width, height, scale, &l);
    cairo_save(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_set_source_rgb(cr, LOGINUI_BRAND_RED, LOGINUI_BRAND_GREEN, LOGINUI_BRAND_BLUE);
    cairo_paint(cr);
    if (logo) {
        int lw = cairo_image_surface_get_width(logo);
        int lh = cairo_image_surface_get_height(logo);
        if (lw > 0 && lh > 0) {
            cairo_save(cr);
            cairo_translate(cr, l.logo_x, l.logo_y);
            cairo_scale(cr, l.logo_size / lw, l.logo_size / lh);
            cairo_set_source_surface(cr, logo, 0, 0);
            cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_GOOD);
            cairo_paint(cr);
            cairo_restore(cr);
        }
    }
    if (bar_alpha > 0.0) {
        double r = l.bar_h / 2.0;
        lui_rounded_rect(cr, l.bar_x, l.bar_y, l.bar_w, l.bar_h, r);
        cairo_set_source_rgba(cr, 1, 1, 1, 0.18 * bar_alpha);
        cairo_fill(cr);
        if (progress < 0.0) progress = 0.0;
        if (progress > 1.0) progress = 1.0;
        double fill = l.bar_w * progress;
        if (fill >= l.bar_h) {
            lui_rounded_rect(cr, l.bar_x, l.bar_y, fill, l.bar_h, r);
            cairo_set_source_rgba(cr, 1, 1, 1, 0.92 * bar_alpha);
            cairo_fill(cr);
        }
    }
    cairo_restore(cr);
}

double loginui_progress_curve(double from, double to, double elapsed,
                              double tau) {
    if (elapsed <= 0.0 || tau <= 0.0) return from;
    return from + (to - from) * (1.0 - exp(-elapsed / tau));
}

bool loginui_progress_read(const char *path, double *value) {
    FILE *f = fopen(path, "r");
    if (!f) return false;
    double v = 0.0;
    int n = fscanf(f, "%lf", &v);
    fclose(f);
    if (n != 1 || v < 0.0 || v > 1.0) return false;
    *value = v;
    return true;
}

void loginui_progress_write(const char *path, double value) {
    char tmp[512];
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    FILE *f = fopen(tmp, "w");
    if (!f) return;
    fprintf(f, "%.4f\n", value);
    fclose(f);
    chmod(tmp, 0644);
    rename(tmp, path);
}
