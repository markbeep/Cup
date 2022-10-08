#include <cairo/cairo.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int w;
    int h;
    double x_min;
    double x_max;
    double y_min;
    double y_max;
    double margin;
    bool display_as_int;
    double x_offset; // offsets the labels on the axis by a given amount
    double y_offset;
    int x_ticks;
    int y_ticks;
} graph_scale_t;

static void draw_text_centered(cairo_surface_t *surface, graph_scale_t gs, double x, double y, char *text, double font_size, double rotation) {
    (void)gs;
    cairo_t *cr = cairo_create(surface);
    cairo_text_extents_t extends = {0};
    cairo_set_source_rgba(cr, 1, 1, 1, 1);
    cairo_set_line_width(cr, 1.5);
    cairo_set_font_size(cr, font_size);
    cairo_text_extents(cr, text, &extends);
    cairo_translate(cr, x, y);
    cairo_rotate(cr, rotation);
    cairo_translate(cr, -x, -y);
    cairo_move_to(cr, x - extends.width / 2, y + extends.height / 2);
    cairo_text_path(cr, text);
    cairo_fill(cr);
}

static void draw_grid_axis(cairo_surface_t *surface, graph_scale_t gs) {
    cairo_t *cr = cairo_create(surface);

    // AXIS
    cairo_set_line_width(cr, 3);
    cairo_set_source_rgba(cr, 1, 1, 1, 1);
    cairo_move_to(cr, gs.w - gs.margin, gs.h - gs.margin);
    cairo_line_to(cr, gs.margin, gs.h - gs.margin);
    cairo_line_to(cr, gs.margin, gs.margin);
    cairo_stroke(cr);

    // LINE TICKS
    double x_spacing = (gs.x_ticks > 0) ? (gs.w - 2 * gs.margin) / gs.x_ticks : 0.0;
    double y_spacing = (gs.y_ticks > 0) ? (gs.h - 2 * gs.margin) / gs.y_ticks : 0.0;
    double x_diff = gs.x_max - gs.x_min;
    double x_val_spac = (gs.x_ticks > 0) ? x_diff / gs.x_ticks : 0.0;
    double y_diff = gs.y_max - gs.y_min;
    double y_val_spac = (gs.y_ticks > 0) ? y_diff / gs.y_ticks : 0.0;
    cairo_set_source_rgba(cr, 1, 1, 1, 0.6);
    cairo_set_line_width(cr, 0.5);

    // x axis ticks
    char text[20];
    for (int i = 0; i <= gs.x_ticks; i++) {
        cairo_move_to(cr, x_spacing * i + gs.margin, gs.h - gs.margin);
        cairo_line_to(cr, x_spacing * i + gs.margin, gs.margin);
        // X AXIS NUMBERING
        if (gs.display_as_int)
            sprintf(text, "%d", (int)(i * x_val_spac + gs.x_offset));
        else
            sprintf(text, "%.2f", i * x_val_spac + gs.x_offset);
        draw_text_centered(surface, gs, x_spacing * i + gs.margin, gs.h - gs.margin + 15, text, 15, 0);
    }
    // y axis ticks
    for (int i = 0; i <= gs.y_ticks; i++) {
        cairo_move_to(cr, gs.margin, y_spacing * i + gs.margin);
        cairo_line_to(cr, gs.w - gs.margin, y_spacing * i + gs.margin);
        // Y AXIS NUMBERING
        if (gs.display_as_int)
            sprintf(text, "%d", (int)(gs.y_max - i * y_val_spac + gs.y_offset));
        else
            sprintf(text, "%.2f", gs.y_max - i * y_val_spac + gs.y_offset);
        draw_text_centered(surface, gs, gs.margin - 15, y_spacing * i + gs.margin, text, 15, 0);
    }
    cairo_stroke(cr);
}

static void graph_coords(graph_scale_t gs, double x, double y, double *x_src, double *y_src) {
    double x_abs = gs.x_max - gs.x_min;
    double y_abs = gs.y_max - gs.y_min;
    *x_src = x * (gs.w - 2 * gs.margin) / x_abs + gs.margin;
    *y_src = gs.h - y * (gs.h - 2 * gs.margin) / y_abs - gs.margin;
}

static void draw_point(cairo_t *cr, double x, double y) {
    float rgb[3] = {255, 255, 255};
    cairo_set_source_rgba(cr, rgb[0] / 255, rgb[1] / 255, rgb[2] / 255, 1);
    cairo_set_line_width(cr, 3);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_move_to(cr, x, y);
    cairo_line_to(cr, x, y);
    cairo_stroke(cr);
}

static void draw_line_graph(cairo_surface_t *surface, graph_scale_t gs, double **points, int n) {
    if (n == 0)
        return;
    // order points by x using bubble sort
    for (int i = 0; i < n; i++) {
        for (int j = i; j < n - 1; j++) {
            if (points[j][0] > points[j + 1][0]) {
                double tmp_x = points[j][0];
                double tmp_y = points[j][1];
                points[j][0] = points[j + 1][0];
                points[j][1] = points[j + 1][1];
                points[j + 1][0] = tmp_x;
                points[j + 1][1] = tmp_y;
            }
        }
    }
    cairo_t *cr = cairo_create(surface);
    cairo_t *cr_2 = cairo_create(surface);
    cairo_set_line_width(cr_2, 2);
    float rgb[3] = {242, 243, 174};
    cairo_set_source_rgba(cr_2, rgb[0] / 255, rgb[1] / 255, rgb[2] / 255, 1);
    double x, y;
    for (int i = 0; i < n; i++) {
        graph_coords(gs, points[i][0], points[i][1], &x, &y);
        draw_point(cr, x, y);
        if (i == 0)
            cairo_move_to(cr_2, x, y);
        else
            cairo_line_to(cr_2, x, y);
    }
    cairo_stroke(cr_2);
}

void draw_efficiency_graph(char *fp, double **points, int n, char *title, char *x_label, char *y_label) {
    graph_scale_t gs = {
        .w = 1000,
        .h = 500,
        .x_max = -__INT_MAX__,
        .x_min = __INT_MAX__,
        .y_max = -__INT_MAX__,
        .y_min = __INT_MAX__,
        .margin = 60,
        .display_as_int = true,
        .x_ticks = (n > 10) ? 10 : (n == 1) ? 1 // n ticks if less than 10 values, else 10 ticks
                                            : n - 1,
        .y_ticks = 10,
        .x_offset = -(double)n,
        .y_offset = 0,
    };
    // finds the min and max x/y values
    for (int i = 0; i < n; i++) {
        if (points[i][0] > gs.x_max)
            gs.x_max = points[i][0];
        if (points[i][0] < gs.x_min)
            gs.x_min = points[i][0];
        if (points[i][1] > gs.y_max)
            gs.y_max = points[i][1];
        if (points[i][1] < gs.y_min)
            gs.y_min = points[i][1];
    }
    gs.y_min = 0;
    gs.y_max = (int)gs.y_max + 1;
    gs.y_ticks = gs.y_max;

    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, gs.w, gs.h);
    cairo_t *cr = cairo_create(surface);

    // background
    cairo_rectangle(cr, 0, 0, gs.w, gs.h);
    float bg_rgb[3] = {48, 52, 63};
    cairo_set_source_rgba(cr, bg_rgb[0] / 255, bg_rgb[1] / 255, bg_rgb[2] / 255, 1);
    cairo_fill(cr);

    // axis
    draw_grid_axis(surface, gs);

    // draw points
    draw_line_graph(surface, gs, points, n);

    // draw labels
    // TITLE
    draw_text_centered(surface, gs, gs.w / 2, gs.margin / 2, title, 25, 0);
    draw_text_centered(surface, gs, gs.margin / 2 - 10, gs.h / 2, y_label, 15, 3 * 3.14159 / 2);
    draw_text_centered(surface, gs, gs.w / 2, gs.h - gs.margin / 2 + 10, x_label, 15, 0);

    // save to file
    cairo_surface_write_to_png(surface, fp);
}

int main(void) {
    int size = 10;
    double **points = calloc(1, sizeof(double) * size);

    for (int i = 0; i < size; i++) {
        points[i] = malloc(sizeof(double) * 2);
        points[i][0] = i;
    }
    double b[10] = {3.5, 3.5, 3.5, 3.5, 3.5, 3.5, 3.5, 3.5, 3.5, 3.5};
    for (int i = 0; i < 10; i++) {
        points[i][1] = b[i];
    }
    draw_efficiency_graph("test.png", points, size, "Count Efficiency", "minutes", "msgs / sec");

    for (int i = 0; i < size; i++)
        free(points[i]);
    free(points);
    return 0;
}
