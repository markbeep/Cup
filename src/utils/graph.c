#include <cairo/cairo.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct {
    int w;
    int h;
    double x_min;
    double x_max;
    double y_min;
    double y_max;
    double margin;
    bool display_as_int;
    int x_ticks;
    int y_ticks;
} graph_scale_t;

static void draw_grid_axis(cairo_t *cr, graph_scale_t gs) {
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
    cairo_set_source_rgba(cr, 1, 1, 1, 0.6);
    cairo_set_line_width(cr, 0.5);
    // x axis ticks
    for (int i = 0; i <= gs.x_ticks; i++) {
        cairo_move_to(cr, x_spacing * i + gs.margin, gs.h - gs.margin);
        cairo_line_to(cr, x_spacing * i + gs.margin, gs.margin);
    }
    // y axis ticks
    for (int i = 0; i <= gs.y_ticks; i++) {
        cairo_move_to(cr, gs.margin, y_spacing * i + gs.margin);
        cairo_line_to(cr, gs.w - gs.margin, y_spacing * i + gs.margin);
    }
    cairo_stroke(cr);

    // NUMBERING
    char t[20];
    if (gs.display_as_int)
        sprintf(t, "%d", (int)gs.x_max);
    else
        sprintf(t, "%.2f", gs.x_max);
    cairo_set_source_rgba(cr, 1, 1, 1, 1);
    cairo_set_line_width(cr, 1.5);
    cairo_move_to(cr, gs.w - 2 * gs.margin, gs.h - gs.margin / 3);
    cairo_text_path(cr, t);
    cairo_fill(cr);
}

static void graph_coords(graph_scale_t gs, double x, double y, double *x_src, double *y_src) {
    *x_src = x * (gs.w - 2 * gs.margin) / (gs.x_max - gs.x_min) + gs.margin;
    *y_src = gs.h - y * (gs.h - 2 * gs.margin) / (gs.y_max - gs.y_min) - gs.margin;
}

static void draw_point(cairo_t *cr, double x, double y) {
    cairo_set_source_rgba(cr, 1, 0, 0, 1);
    cairo_set_line_width(cr, 5);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    cairo_move_to(cr, x, y);
    cairo_line_to(cr, x, y);
    cairo_stroke(cr);
}

static void draw_line_graph(cairo_surface_t *surface, graph_scale_t gs, double **points, size_t n) {
    if (n == 0)
        return;
    // order points by x using bubble sort
    for (size_t i = 0; i < n; i++) {
        for (size_t j = i; j < n - 1; j++) {
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
    cairo_set_source_rgba(cr_2, 1, 0, 0, 1);
    double x, y;
    for (size_t i = 0; i < n; i++) {
        graph_coords(gs, points[i][0], points[i][1], &x, &y);
        draw_point(cr, x, y);
        if (i == 0)
            cairo_move_to(cr_2, x, y);
        else
            cairo_line_to(cr_2, x, y);
    }
    cairo_stroke(cr_2);
}

void draw_efficiency_graph(char *fp, double **points, size_t n) {
    graph_scale_t gs = {
        .w = 500,
        .h = 250,
        .x_max = 100.0,
        .x_min = 0.0,
        .y_max = 10.0,
        .y_min = 0.0,
        .margin = 20,
        .display_as_int = true,
        .x_ticks = 30,
        .y_ticks = 20,
    };
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, gs.w, gs.h);
    cairo_t *cr = cairo_create(surface);

    // background
    cairo_rectangle(cr, 0, 0, gs.w, gs.h);
    cairo_set_source_rgba(cr, 0, 0, 0, 1);
    cairo_fill(cr);

    // axis
    draw_grid_axis(cr, gs);

    // draw points
    draw_line_graph(surface, gs, points, n);

    // save to file
    cairo_surface_write_to_png(surface, fp);
}

int main_graph(void) {
    draw_efficiency_graph("test.png", NULL, 0);
    return 0;
}
