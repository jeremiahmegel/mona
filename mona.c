// written by nick welch <nick@incise.org>.  author disclaims copyright.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <limits.h>

#include <cairo.h>
#include <cairo-svg.h>
#include <cairo-xlib.h>

#include <X11/Xlib.h>

#define RANDINT(max) (int)((random() / (double)RAND_MAX) * (max))
#define RANDDOUBLE(max) ((random() / (double)RAND_MAX) * max)
#define ABS(val) ((val) < 0 ? -(val) : (val))
#define CLAMP(val, min, max) ((val) < (min) ? (min) : \
                              (val) > (max) ? (max) : (val))

int WIDTH;
int HEIGHT;

int NUM_POINTS = 6;
int NUM_SHAPES = 40;

int display_window = 0;
char * png_filename = NULL;
char * svg_filename = NULL;

Display * dpy;
int screen;
Window win;
GC gc;
Pixmap pixmap;

void x_init(void)
{
    if(!(dpy = XOpenDisplay(NULL)))
    {
        fprintf(stderr, "Failed to open X display %s\n", XDisplayName(NULL));
        exit(1);
    }

    screen = DefaultScreen(dpy);

    XSetWindowAttributes attr;
    attr.background_pixmap = ParentRelative;
    win = XCreateWindow(dpy, DefaultRootWindow(dpy), 0, 0,
                   WIDTH, HEIGHT, 0,
                   DefaultDepth(dpy, screen), CopyFromParent, DefaultVisual(dpy, screen),
                   CWBackPixmap, &attr);

    pixmap = XCreatePixmap(dpy, win, WIDTH, HEIGHT,
            DefaultDepth(dpy, screen));

    gc = XCreateGC(dpy, pixmap, 0, NULL);

    XSelectInput(dpy, win, ExposureMask);

    XMapWindow(dpy, win);
}

typedef struct {
    double x, y;
} point_t;

typedef struct {
    double r, g, b, a;
    point_t * points;
} shape_t;

shape_t * dna_best;
shape_t * dna_test;

int mutated_shape;

void draw_shape(shape_t * dna, cairo_t * cr, int i)
{
    cairo_set_line_width(cr, 0);
    shape_t * shape = &dna[i];
    cairo_set_source_rgba(cr, shape->r, shape->g, shape->b, shape->a);
    cairo_move_to(cr, shape->points[0].x, shape->points[0].y);
    for(int j = 1; j < NUM_POINTS; j++)
        cairo_line_to(cr, shape->points[j].x, shape->points[j].y);
    cairo_fill(cr);
}

void draw_dna(shape_t * dna, cairo_t * cr)
{
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_rectangle(cr, 0, 0, WIDTH, HEIGHT);
    cairo_fill(cr);
    for(int i = 0; i < NUM_SHAPES; i++)
        draw_shape(dna, cr, i);
}

void init_dna(shape_t * dna)
{
    for(int i = 0; i < NUM_SHAPES; i++)
    {
        dna[i].points = calloc(NUM_POINTS, sizeof(point_t));
        for(int j = 0; j < NUM_POINTS; j++)
        {
            dna[i].points[j].x = RANDDOUBLE(WIDTH);
            dna[i].points[j].y = RANDDOUBLE(HEIGHT);
        }
        dna[i].r = RANDDOUBLE(1);
        dna[i].g = RANDDOUBLE(1);
        dna[i].b = RANDDOUBLE(1);
        dna[i].a = RANDDOUBLE(1);
        //dna[i].r = 0.5;
        //dna[i].g = 0.5;
        //dna[i].b = 0.5;
        //dna[i].a = 1;
    }
}

int mutate(void)
{
    mutated_shape = RANDINT(NUM_SHAPES);
    double roulette = RANDDOUBLE(2.8);
    double drastic = RANDDOUBLE(2);
     
    // mutate color
    if(roulette<1)
    {
        if(dna_test[mutated_shape].a < 0.01 // completely transparent shapes are stupid
                || roulette<0.25)
        {
            if(drastic < 1)
            {
                dna_test[mutated_shape].a += RANDDOUBLE(0.1);
                dna_test[mutated_shape].a = CLAMP(dna_test[mutated_shape].a, 0.0, 1.0);
            }
            else
                dna_test[mutated_shape].a = RANDDOUBLE(1.0);
        }
        else if(roulette<0.50)
        {
            if(drastic < 1)
            {
                dna_test[mutated_shape].r += RANDDOUBLE(0.1);
                dna_test[mutated_shape].r = CLAMP(dna_test[mutated_shape].r, 0.0, 1.0);
            }
            else
                dna_test[mutated_shape].r = RANDDOUBLE(1.0);
        }
        else if(roulette<0.75)
        {
            if(drastic < 1)
            {
                dna_test[mutated_shape].g += RANDDOUBLE(0.1);
                dna_test[mutated_shape].g = CLAMP(dna_test[mutated_shape].g, 0.0, 1.0);
            }
            else
                dna_test[mutated_shape].g = RANDDOUBLE(1.0);
        }
        else
        {
            if(drastic < 1)
            {
                dna_test[mutated_shape].b += RANDDOUBLE(0.1);
                dna_test[mutated_shape].b = CLAMP(dna_test[mutated_shape].b, 0.0, 1.0);
            }
            else
                dna_test[mutated_shape].b = RANDDOUBLE(1.0);
        }
    }
    
    // mutate shape
    else if(roulette < 2.0)
    {
        int point_i = RANDINT(NUM_POINTS);
        if(roulette<1.5)
        {
            if(drastic < 1)
            {
                dna_test[mutated_shape].points[point_i].x += (int)RANDDOUBLE(WIDTH/10.0);
                dna_test[mutated_shape].points[point_i].x = CLAMP(dna_test[mutated_shape].points[point_i].x, 0, WIDTH-1);
            }
            else
                dna_test[mutated_shape].points[point_i].x = RANDDOUBLE(WIDTH);
        }
        else
        {
            if(drastic < 1)
            {
                dna_test[mutated_shape].points[point_i].y += (int)RANDDOUBLE(HEIGHT/10.0);
                dna_test[mutated_shape].points[point_i].y = CLAMP(dna_test[mutated_shape].points[point_i].y, 0, HEIGHT-1);
            }
            else
                dna_test[mutated_shape].points[point_i].y = RANDDOUBLE(HEIGHT);
        }
    }

    // mutate stacking
    else
    {
        int destination = RANDINT(NUM_SHAPES);
        shape_t s = dna_test[mutated_shape];
        dna_test[mutated_shape] = dna_test[destination];
        dna_test[destination] = s;
        return destination;
    }
    return -1;

}

int MAX_FITNESS = -1;

unsigned char * goal_data = NULL;

int difference(cairo_surface_t * test_surf, cairo_surface_t * goal_surf)
{
    unsigned char * test_data = cairo_image_surface_get_data(test_surf);
    if(!goal_data)
        goal_data = cairo_image_surface_get_data(goal_surf);

    int difference = 0;

    int my_max_fitness = 0;

    for(int y = 0; y < HEIGHT; y++)
    {
        for(int x = 0; x < WIDTH; x++)
        {
            int thispixel = y*WIDTH*4 + x*4;

            unsigned char test_a = test_data[thispixel];
            unsigned char test_r = test_data[thispixel + 1];
            unsigned char test_g = test_data[thispixel + 2];
            unsigned char test_b = test_data[thispixel + 3];

            unsigned char goal_a = goal_data[thispixel];
            unsigned char goal_r = goal_data[thispixel + 1];
            unsigned char goal_g = goal_data[thispixel + 2];
            unsigned char goal_b = goal_data[thispixel + 3];

            if(MAX_FITNESS == -1)
                my_max_fitness += goal_a + goal_r + goal_g + goal_b;

            difference += ABS(test_a - goal_a);
            difference += ABS(test_r - goal_r);
            difference += ABS(test_g - goal_g);
            difference += ABS(test_b - goal_b);
        }
    }

    if(MAX_FITNESS == -1)
        MAX_FITNESS = my_max_fitness;
    return difference;
}

void copy_surf_to(cairo_surface_t * surf, cairo_t * cr)
{
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_rectangle(cr, 0, 0, WIDTH, HEIGHT);
    cairo_fill(cr);
    //cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(cr, surf, 0, 0);
    cairo_paint(cr);
}

void copy_shape(shape_t * from_shape, shape_t * to_shape)
{
    to_shape->r = from_shape->r;
    to_shape->g = from_shape->g;
    to_shape->b = from_shape->b;
    to_shape->a = from_shape->a;
    for(int i = 0; i < NUM_POINTS; i++)
    {
        to_shape->points[i].x = from_shape->points[i].x;
        to_shape->points[i].y = from_shape->points[i].y;
    }
}

static void mainloop(cairo_surface_t * pngsurf)
{
    struct timeval start;
    gettimeofday(&start, NULL);

    dna_best = calloc(NUM_SHAPES, sizeof(shape_t));
    dna_test = calloc(NUM_SHAPES, sizeof(shape_t));
    init_dna(dna_best);
    for (int i = 0; i < NUM_SHAPES; i++)
    {
        dna_test[i].points = calloc(NUM_POINTS, sizeof(point_t));
        copy_shape(&(dna_best[i]), &(dna_test[i]));
    }

    cairo_surface_t * xsurf;
    cairo_t * xcr;
    if(display_window == 1)
    {
        xsurf = cairo_xlib_surface_create(
                dpy, pixmap, DefaultVisual(dpy, screen), WIDTH, HEIGHT);
        xcr = cairo_create(xsurf);
    }

    cairo_surface_t * svg_surf;
    cairo_surface_t * svg_surf2;
    cairo_t * svg_cr;
    if(svg_filename != NULL)
    {
        svg_surf = cairo_svg_surface_create(svg_filename, WIDTH, HEIGHT);
        svg_cr = cairo_create(svg_surf);
    }

    cairo_surface_t * test_surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, WIDTH, HEIGHT);
    cairo_t * test_cr = cairo_create(test_surf);

    cairo_surface_t * goalsurf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, WIDTH, HEIGHT);
    cairo_t * goalcr = cairo_create(goalsurf);
    copy_surf_to(pngsurf, goalcr);

    int lowestdiff = INT_MAX;
    int teststep = 0;
    int beststep = 0;
    for(;;) {
        int other_mutated = mutate();
        draw_dna(dna_test, test_cr);

        int diff = difference(test_surf, goalsurf);
        if(diff < lowestdiff)
        {
            beststep++;
            // test is good, copy to best
            copy_shape(&(dna_test[mutated_shape]), &(dna_best[mutated_shape]));
            if(other_mutated >= 0)
            {
                copy_shape(&(dna_test[other_mutated]), &(dna_best[other_mutated]));
                if(svg_filename != NULL)
                    draw_dna(dna_test, svg_cr);
            }
            if(display_window == 1)
            {
                copy_surf_to(test_surf, xcr); // also copy to display
                XCopyArea(dpy, pixmap, win, gc,
                        0, 0,
                        WIDTH, HEIGHT,
                        0, 0);
            }
            lowestdiff = diff;
        }
        else
        {
            // test sucks, copy best back over test
            copy_shape(&(dna_best[mutated_shape]), &(dna_test[mutated_shape]));
            if(other_mutated >= 0)
                copy_shape(&(dna_best[other_mutated]), &(dna_test[other_mutated]));
        }

        teststep++;

#ifdef TIMELIMIT
        struct timeval t;
        gettimeofday(&t, NULL);
        if(t.tv_sec - start.tv_sec > TIMELIMIT)
        {
            printf("%0.6f\n", ((MAX_FITNESS-lowestdiff) / (float)MAX_FITNESS)*100);
#ifdef DUMP
            char filename[50];
            sprintf(filename, "%d.data", getpid());
            FILE * f = fopen(filename, "w");
            fwrite(dna_best, sizeof(shape_t), NUM_SHAPES, f);
            fclose(f);
#endif
            return;
        }
#else
        if(teststep % 100 == 0)
            printf("Step = %d/%d\nFitness = %0.6f%%\n",
                    beststep, teststep, ((MAX_FITNESS-lowestdiff) / (float)MAX_FITNESS)*100);

        if (teststep % 1000 == 0) {
            if(svg_filename != NULL)
            {
                svg_surf2 = cairo_svg_surface_create(svg_filename, WIDTH, HEIGHT);
                cairo_destroy(svg_cr);
                svg_cr = cairo_create(svg_surf2);
                cairo_set_source_surface(svg_cr, cairo_surface_reference(svg_surf), 0, 0);
                cairo_rectangle(svg_cr, 0, 0, WIDTH, HEIGHT);
                cairo_fill(svg_cr);
                cairo_surface_finish(svg_surf);
                cairo_surface_destroy(svg_surf);
                svg_surf = svg_surf2;
            }

            if(png_filename != NULL)
            {
                cairo_surface_write_to_png(test_surf, png_filename);
            }
        }
#endif

        if(display_window == 1 && teststep % 100 == 0 && XPending(dpy))
        {
            XEvent xev;
            XNextEvent(dpy, &xev);
            switch(xev.type) {
                case Expose:
                    XCopyArea(dpy, pixmap, win, gc,
                            xev.xexpose.x, xev.xexpose.y,
                            xev.xexpose.width, xev.xexpose.height,
                            xev.xexpose.x, xev.xexpose.y);
            }
        }
    }
}

int main(int argc, char ** argv) {
    char * sourcefile = NULL;
    int c;
    while ((c = getopt(argc, argv, "v:e:wi:p:s:")) != -1) {
        switch(c)
        {
            case 'v':
                // Vertices
                NUM_POINTS = atoi(optarg);
                break;
            case 'e':
                // Elements
                NUM_SHAPES = atoi(optarg);
                break;
            case 'w':
                // Window
                display_window = 1;
                break;
            case 'i':
                // Input
                sourcefile = optarg;
                break;
            case 'p':
                // PNG
                png_filename = optarg;
                break;
            case 's':
                // SVG
                svg_filename = optarg;
                break;
            default:
                return 1;
        }
    }
    if(sourcefile == NULL)
    {
        fprintf(stderr, "The -i option is required\n");
        return 1;
    }

    cairo_surface_t * pngsurf = cairo_image_surface_create_from_png(sourcefile);

    WIDTH = cairo_image_surface_get_width(pngsurf);
    HEIGHT = cairo_image_surface_get_height(pngsurf);

    srandom(getpid() + time(NULL));
    if(display_window == 1)
    {
        x_init();
    }
    mainloop(pngsurf);
}

