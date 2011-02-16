#include <stdlib.h>
#include <math.h>
#include <gtk/gtk.h>

#include "petri-foo.h"
#include "patch.h"
#include "waveform.h"

/* signals */
enum
{
    CHANGED,
    LAST_SIGNAL,
};

static int signals[LAST_SIGNAL];

#define BG_DEAD_R1 0
#define BG_DEAD_R2 0.075
#define BG_DEAD_G1 0
#define BG_DEAD_G2 0
#define BG_DEAD_B1 0
#define BG_DEAD_B2 0

#define BG_PLAY_R1 0
#define BG_PLAY_R2 0
#define BG_PLAY_G1 0
#define BG_PLAY_G2 0
#define BG_PLAY_B1 0.0762951
#define BG_PLAY_B2 0.30518

#define BG_LOOP_R1 0
#define BG_LOOP_R2 0
#define BG_LOOP_G1 0.0381476
#define BG_LOOP_G2 0.15259
#define BG_LOOP_B1 0
#define BG_LOOP_B2 0

#define GRID_R1 0
#define GRID_R2 0
#define GRID_G1 0.0762951
#define GRID_G2 0.15259
#define GRID_B1 0.228885
#define GRID_B2 0.457771

#define CENT_R 0
#define CENT_G 0.228885
#define CENT_B 0.457771

#define WAVE_DEAD_R 0.25
#define WAVE_DEAD_G 0.25
#define WAVE_DEAD_B 0.25

#define WAVE_PLAY_R 0
#define WAVE_PLAY_G 0.610361
#define WAVE_PLAY_B 0.915541

#define WAVE_LOOP_R 0.30518
#define WAVE_LOOP_G 0.762951
#define WAVE_LOOP_B 0.457771

enum
{
     GRID_Y = 8 /* number of sqaures in Y direction */
};


G_DEFINE_TYPE(Waveform, waveform, GTK_TYPE_DRAWING_AREA)


typedef struct _WaveformPrivate WaveformPrivate;

#define WAVEFORM_GET_PRIVATE(obj) \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), WAVEFORM_TYPE, WaveformPrivate))


struct _WaveformPrivate
{
    GdkPixmap * pixmap;
    gboolean   interactive;
    int        width;
    int        height;
    float      range_start;
    float      range_stop;
    int patch;
};


/* forward declarations */
static void waveform_class_init (WaveformClass * wf);

static void waveform_init (Waveform * wf);
static void waveform_destroy (GtkObject * object);

static void waveform_realize (GtkWidget * widget);

static void waveform_size_request (GtkWidget * widget,
                                    GtkRequisition * requisition);

static void waveform_size_allocate (GtkWidget * widget,
                                    GtkAllocation * allocation);

static gboolean waveform_expose(GtkWidget * widget,
                                    GdkEventExpose * event);

static gboolean waveform_button_press(GtkWidget * widget,
                                    GdkEventButton * event);

static gboolean waveform_configure(GtkWidget * widget,
                                    GdkEventConfigure * event);

static void waveform_draw(Waveform * wf);


static void waveform_class_init (WaveformClass * klass)
{
    GtkObjectClass *object_class = GTK_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    object_class->destroy =         waveform_destroy;

    widget_class->expose_event =        waveform_expose;
    widget_class->configure_event =     waveform_configure;
    widget_class->size_request =        waveform_size_request;
    widget_class->button_press_event =  waveform_button_press;

    signals[CHANGED] =
        g_signal_new ("changed",
                        G_TYPE_FROM_CLASS (klass),
                        G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                        G_STRUCT_OFFSET (WaveformClass, changed),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

    g_type_class_add_private(object_class, sizeof(WaveformPrivate));
}


static void waveform_init (Waveform * wf)
{
    WaveformPrivate* p = WAVEFORM_GET_PRIVATE(wf);

    gtk_widget_add_events(GTK_WIDGET(wf), GDK_BUTTON_PRESS_MASK
                                        | GDK_BUTTON_RELEASE_MASK   );

    p->pixmap = NULL;
    p->interactive = FALSE;
    p->width = 0;
    p->height = 0;
    p->patch = -1;
    p->range_start = 0.0;
    p->range_stop = 1.0;
}


static void waveform_destroy (GtkObject * object)
{
    WaveformPrivate* p;

    g_return_if_fail (object != NULL);
    g_return_if_fail (IS_WAVEFORM (object));

    p = WAVEFORM_GET_PRIVATE(object);

    if (p->pixmap)
    {
        gdk_pixmap_unref(p->pixmap);
        p->pixmap = NULL;
    }
}



static void
waveform_size_request (GtkWidget * widget, GtkRequisition * requisition)
{
    WaveformPrivate* p = WAVEFORM_GET_PRIVATE(widget);

    requisition->width = p->width;
    requisition->height = p->height;

    debug("size_request:%d x %d\n", p->width, p->height);
}


static gboolean
waveform_configure(GtkWidget * widget, GdkEventConfigure * event)
{
    WaveformPrivate* p = WAVEFORM_GET_PRIVATE(widget);

debug("CONFIGURE\n");

    p->width = widget->allocation.width;
    p->height = widget->allocation.height;

debug("allocation w:%d h:%d\n",p->width, p->height);

    if (p->pixmap)
        gdk_pixmap_unref(p->pixmap);

    p->pixmap = gdk_pixmap_new(widget->window, p->width, p->height, -1);

    waveform_draw(WAVEFORM(widget));

    return TRUE;
}


static gboolean
waveform_expose (GtkWidget * widget, GdkEventExpose * event)
{
    WaveformPrivate* p;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (IS_WAVEFORM (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);
    g_return_val_if_fail (event->count == 0, FALSE);

    p = WAVEFORM_GET_PRIVATE(widget);

    /*  Assume that when event dimensions match waveform dimensions
     *  the entire waveform-widget needs to be redrawn fully (ie it's
     *  been scrolled or zoomed into). Any events which are only partial
     *  are assumed only to require redrawing of the pixmap.
     */
    if (event->area.width == p->width && event->area.height == p->height)
        waveform_draw(WAVEFORM(widget));

    gdk_draw_drawable(widget->window,
                      widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
                      p->pixmap,
                      event->area.x, event->area.y,
                      event->area.x, event->area.y,
                      event->area.width, event->area.height);

    return TRUE;
}


static gboolean
waveform_button_press (GtkWidget * widget, GdkEventButton * event)
{
    Waveform *wf;
    WaveformPrivate* p;
debug("button press\n");
    int frames;
    int start;
    int stop;
    float fpp;			// frames per pixel 
    int sel, control, chaos;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (IS_WAVEFORM (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    wf = WAVEFORM (widget);
    p = WAVEFORM_GET_PRIVATE(widget);

    if (!p->interactive)
        return FALSE;

    frames = patch_get_frames (p->patch);
    start = p->range_start * frames;
    stop =  p->range_stop * frames;
    fpp = (stop - start) * 1.0 / p->width;

    sel = (event->x * fpp) + start; /* sel is now set to the frame
                                     * which corresponds to where the
                                     * user clicked */
    if (event->type == GDK_BUTTON_PRESS)
    {

        /* during this process we ensure that values aren't set
         * which are bogus in relation to each other.  For example,
         * we make sure that the sample starting point is not
         * greater than it's stopping point.
         */

        /* left button: start point
         * anything else: stop point */

        if (event->state & GDK_CONTROL_MASK)
        {   /* we set play points for control clicks */
            if (event->button == 1)
            {
                control = patch_get_sample_stop (p->patch);
                if (sel < control)
                {
                    patch_set_sample_start (p->patch, sel);
                    /* adjust starting loop point if we need to */
                    chaos = patch_get_loop_start (p->patch);
                    if (sel > chaos)
                        patch_set_loop_start (p->patch, sel);
                }
            }
            else
            {
                control = patch_get_sample_start (p->patch);
                if (sel > control)
                {
                    patch_set_sample_stop (p->patch, sel);
                    /* adjust stopping loop point if we need to */
                    chaos = patch_get_loop_stop (p->patch);
                    if (sel < chaos)
                        patch_set_loop_stop (p->patch, sel);
                }
            }
        }
        else
        {   /* otherwise, we set loop points */
            if (event->button == 1)
            {
                control = patch_get_sample_start (p->patch);
                chaos = patch_get_loop_stop (p->patch);
                if (sel > control && sel < chaos)
                    patch_set_loop_start (p->patch, sel);
            }
            else
            {
                control = patch_get_sample_stop (p->patch);
                chaos = patch_get_loop_start (p->patch);
                if (sel < control && sel > chaos)
                    patch_set_loop_stop (p->patch, sel);
            }
        }
    }

    g_signal_emit_by_name(G_OBJECT(wf), "changed");
    waveform_draw(WAVEFORM(wf));
    gtk_widget_queue_draw (widget);

    return FALSE;
}


inline static void
cr_rect(cairo_t* cr, double x1, double y1, double x2, double y2)
{
    cairo_rectangle(cr, x1, y1, x2 - x1, y2 - y1);
}

static void draw_back(WaveformPrivate* p, int w, int h, cairo_t* cr)
{
    int y;
    int center = h / 2;
    float d;

    int frames;
    int start, stop;
    int play_start, play_stop;
    int loop_start, loop_stop;
    float ppf;

    cairo_pattern_t *bg_dead;
    cairo_pattern_t *bg_play;
    cairo_pattern_t *bg_loop;

    frames = patch_get_frames (p->patch);

    if (frames > 0)
    {
        start = frames * p->range_start;
        stop = frames * p->range_stop;
        play_start = patch_get_sample_start(p->patch);
        play_stop = patch_get_sample_stop(p->patch);
        loop_start = patch_get_loop_start (p->patch);
        loop_stop = patch_get_loop_stop (p->patch);

        ppf = w / (stop - start * 1.0);

        play_start = (play_start - start) * ppf;
        play_stop = (play_stop - start) * ppf;
        loop_start = (loop_start - start) * ppf;
        loop_stop = (loop_stop - start) * ppf;
    }
    else
    {   /* initialize so that a dead background is drawn */
        play_start = play_stop = loop_start = loop_stop = w;
    }

    bg_dead = cairo_pattern_create_linear (0, 0, 0, h);
    bg_play = cairo_pattern_create_linear (0, 0, 0, h);
    bg_loop = cairo_pattern_create_linear (0, 0, 0, h);

    cairo_pattern_add_color_stop_rgb (bg_dead, 0.0, BG_DEAD_R1,
                                                    BG_DEAD_G1,
                                                    BG_DEAD_B1);
    cairo_pattern_add_color_stop_rgb (bg_dead, 0.5, BG_DEAD_R2,
                                                    BG_DEAD_G2,
                                                    BG_DEAD_B2);
    cairo_pattern_add_color_stop_rgb (bg_dead, 1.0, BG_DEAD_R1,
                                                    BG_DEAD_G1,
                                                    BG_DEAD_B1);

    cairo_pattern_add_color_stop_rgb (bg_play, 0.0, BG_PLAY_R1,
                                                    BG_PLAY_G1,
                                                    BG_PLAY_B1);
    cairo_pattern_add_color_stop_rgb (bg_play, 0.5, BG_PLAY_R2,
                                                    BG_PLAY_G2,
                                                    BG_PLAY_B2);
    cairo_pattern_add_color_stop_rgb (bg_play, 1.0, BG_PLAY_R1,
                                                    BG_PLAY_G1,
                                                    BG_PLAY_B1);

    cairo_pattern_add_color_stop_rgb (bg_loop, 0.0, BG_LOOP_R1,
                                                    BG_LOOP_G1,
                                                    BG_LOOP_B1);
    cairo_pattern_add_color_stop_rgb (bg_loop, 0.5, BG_LOOP_R2,
                                                    BG_LOOP_G2,
                                                    BG_LOOP_B2);
    cairo_pattern_add_color_stop_rgb (bg_loop, 1.0, BG_LOOP_R1,
                                                    BG_LOOP_G1,
                                                    BG_LOOP_B1);

    if (play_start > 0)
    {   /* show left dead area if visible */
        int w1 = (play_start < w - 1) ? play_start : w - 1;
        cairo_rectangle (cr, 0, 0, w1, h);
        cairo_set_source (cr, bg_dead);
        cairo_fill (cr);

        if (play_start > w - 1)
            goto done;
    }

    if (loop_start > 0)
    {   /* show left play area if visible */
        int x1 = (play_start > 0) ? play_start : 0;
        int x2 = (loop_start < w - 1) ? loop_start : w - 1;
        cr_rect(cr, x1, 0, x2, h);
        cairo_set_source (cr, bg_play);
        cairo_fill (cr);

        if (loop_start > w - 1)
            goto done;
    }

    if (loop_start < w - 1 && loop_stop > 0)
    {
        /* the loop is visible, but are it's start/stop points? */
        int x1 = (loop_start > 0) ? loop_start: 0;
        int x2 = (loop_stop < w - 1) ? loop_stop : w - 1;
        cr_rect(cr, x1, 0, x2, h);
        cairo_set_source(cr, bg_loop);
        cairo_fill(cr);

        if (loop_stop > w - 1)
            goto done;
    }

    if (loop_stop < w - 1)
    {   /* right play area visible */
        int x1 = (loop_stop > 0) ? loop_stop : 0;
        int x2 = (play_stop < w - 1) ? play_stop : w - 1;
        cr_rect(cr, x1, 0, x2, h);
        cairo_set_source(cr, bg_play);
        cairo_fill(cr);

        if (play_stop > w - 1)
            goto done;
    }

    if (play_stop < w - 1)
    {   /* right dead area visible */
        int x1 = (play_stop > 0) ? play_stop : 0;
        cr_rect(cr, x1, 0, w, h);
        cairo_set_source(cr, bg_dead);
        cairo_fill(cr);
    }

done:
    cairo_pattern_destroy(bg_dead);
    cairo_pattern_destroy(bg_play);
    cairo_pattern_destroy(bg_loop);
}


static void draw_wave(WaveformPrivate* p, int w, int h, cairo_t* cr)
{
    int center = h / 2;
    int frames;
    int play_start, play_stop;
    int loop_start, loop_stop;
    int start, stop;
    const float* wav;

    if (p->patch < 0)
        return;

    if ((wav = patch_get_sample (p->patch)) == NULL)
        return;

    frames = patch_get_frames (p->patch);
    start = frames * p->range_start;
    stop = frames * p->range_stop;
    play_start = patch_get_sample_start (p->patch);
    play_stop = patch_get_sample_stop (p->patch);
    loop_start = patch_get_loop_start (p->patch);
    loop_stop = patch_get_loop_stop (p->patch);

    /* draw waveform when pixels >= frames */
    if (w >= (stop - start))
    {
        int lx = 0;     /* last x val */
        int ly = center;/* last y val */
        int x = 0;      /* x index */
        int y = 0;      /* y val */
        int f = start;  /* frame index */
        int ferr = 0;   /* frame error value */
        int visframes = stop - start;   /* number of frames that 
                                         * will be drawn */
        for (x = 0; x < w; x++)
        {
            if ((ferr += visframes) >= w)
                ferr -= w;
            else
                continue;

            y = (wav[f*2] + 1) / 2 * h;

            /* set line color */
            if (f < play_start || f > play_stop)
            {
                cairo_set_source_rgb(cr,    WAVE_DEAD_R,
                                            WAVE_DEAD_G,
                                            WAVE_DEAD_B );
            }
            else if (f >= loop_start && f <= loop_stop)
            {
                cairo_set_source_rgb(cr,    WAVE_LOOP_R,
                                            WAVE_LOOP_G,
                                            WAVE_LOOP_B );
            }
            else
            {
                cairo_set_source_rgb(cr,    WAVE_PLAY_R,
                                            WAVE_PLAY_G,
                                            WAVE_PLAY_B );
            }

            cairo_set_line_width(cr, 1.0);
            cairo_move_to (cr, lx, ly);
            cairo_line_to (cr, x, y);
            cairo_stroke(cr);
            f++;
            lx = x;
            ly = y;
        }
    }
    else /* draw waveform when pixels < frames */
    {
        float lminy = 0;    /* last min val */
        float lmaxy = 0;    /* last max val */
        float miny = 2;     /* min val found over interval */
        float maxy = -2;    /* max val found over interval */
        int draw_miny = 0;  /* pixel value of miny */
        int draw_maxy = 0;  /* pixel value of maxy */
        int lx = 0;	        /* last x value (prevents trouble when x == 0)*/
        int xerr = 0;       /* x error value */
        int x = 0;          /* x index */
        int f = 0;          /* frame index */
        int s = 0;          /* sample index */
        int visframes = stop - start;

        for (f = start; f < stop; f++)
        {
            s = f * 2;

            if (wav[s] > maxy)
                maxy = wav[s];

            if (wav[s] < miny)
                miny = wav[s];

            if ((xerr += w) >= visframes)
                xerr -= visframes;
            else
                continue;

            if (f < play_start || f > play_stop)
            {
                cairo_set_source_rgb(cr,    WAVE_DEAD_R,
                                            WAVE_DEAD_G,
                                            WAVE_DEAD_B );
            }
            else if (f >= loop_start && f <= loop_stop)
            {
                cairo_set_source_rgb(cr,    WAVE_LOOP_R,
                                            WAVE_LOOP_G,
                                            WAVE_LOOP_B );
            }
            else
            {
                cairo_set_source_rgb(cr,    WAVE_PLAY_R,
                                            WAVE_PLAY_G,
                                            WAVE_PLAY_B );
            }

            /* calculate drawing coordinates */
            draw_miny = (miny + 1) / 2 * h;
            draw_maxy = (maxy + 1) / 2 * h;

            /* connect to previously drawn segment; we put off
             * converting the lm??y values so that they don't get
             * calculated if they aren't needed (efficiency) */
            if (maxy < lminy)
            {
                cairo_move_to (cr, lx, (lminy+1)/2 * h);
                cairo_line_to (cr, x, draw_maxy);
/*              gdk_draw_line(surface, gc, lx, (lminy+1)/2 * h, x,
                                                                draw_maxy);
*/          }
            else if (miny > lmaxy)
            {
                cairo_move_to (cr, lx, (lmaxy+1)/2 * h);
                cairo_line_to (cr, x, draw_miny);
/*              gdk_draw_line (surface, gc, lx, (lmaxy+1)/2 * h, x,
                                                                draw_miny);
*/
            }

            /* connect min val to max val */
/*          gdk_draw_line (surface, gc, x, draw_miny, x, draw_maxy);
*/
            cairo_set_line_width(cr, 1.0);
            cairo_move_to (cr, x, draw_miny);
            cairo_line_to (cr, x, draw_maxy);

            cairo_stroke(cr);


            /* reset all vars */
            lx = x++;
            lminy = miny;
            lmaxy = maxy;

            /* this trick ensures that miny and maxy will be set at
            * least once */
            miny = 2;
            maxy = -2;
        }
    }
}


static void waveform_draw(Waveform * wf)
{
    GtkWidget* widget = GTK_WIDGET(wf);
    WaveformPrivate* p = WAVEFORM_GET_PRIVATE(wf);

    cairo_t* cr;

    if (!GTK_WIDGET_REALIZED (widget))
        return;

    cr = gdk_cairo_create(p->pixmap);

    cairo_rectangle(cr, 0, 0, p->width, p->height);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_fill_preserve(cr);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_stroke(cr);

    draw_back(p, p->width, p->height, cr);
    draw_wave(p, p->width, p->height, cr);

    cairo_destroy(cr);

    /* draw each component */
/*
    draw_back (p, GDK_DRAWABLE (widget->window), w, h, gc);
    draw_grid (GDK_DRAWABLE (widget->window), w, h, gc);
    draw_wave (p, GDK_DRAWABLE (widget->window), w, h, gc);
    draw_loop (p, GDK_DRAWABLE (widget->window), w, h, gc);

    g_object_unref (gc);
*/
}


GtkWidget *waveform_new (void)
{
    return GTK_WIDGET(g_object_new(WAVEFORM_TYPE, NULL));
}


void waveform_set_patch (Waveform * wf, int id)
{
    g_return_if_fail (wf != NULL);
    g_return_if_fail (IS_WAVEFORM (wf));
    WaveformPrivate* p = WAVEFORM_GET_PRIVATE(wf);
    p->patch = id;
    gtk_widget_queue_draw (GTK_WIDGET (wf));
}


void waveform_set_size (Waveform * wf, int w, int h)
{
    g_return_if_fail (wf != NULL);
    g_return_if_fail (IS_WAVEFORM (wf));
    WaveformPrivate* p = WAVEFORM_GET_PRIVATE(wf);
    p->width = w;
    p->height = h;
    gtk_widget_queue_resize (GTK_WIDGET (wf));
}


void waveform_set_interactive (Waveform * wf, gboolean interactive)
{
    g_return_if_fail (wf != NULL);
    g_return_if_fail (IS_WAVEFORM (wf));
    WaveformPrivate* p = WAVEFORM_GET_PRIVATE(wf);
    p->interactive = interactive;
    gtk_widget_queue_draw (GTK_WIDGET (wf));
}


void waveform_set_range (Waveform * wf, float start, float stop)
{
    g_return_if_fail (wf != NULL);
    g_return_if_fail (IS_WAVEFORM (wf));
    WaveformPrivate* p = WAVEFORM_GET_PRIVATE(wf);

    p->range_start = (start < 0.0 || start > 1.0 || start > stop)
                            ? 0.0 
                            : start;

    p->range_stop = (stop < 0.0 || start > 1.0 || start > stop)
                            ? 1.0
                            : stop;

    gtk_widget_queue_draw (GTK_WIDGET (wf));
}


int waveform_get_patch (Waveform * wf)
{
    g_return_val_if_fail (wf != NULL, -1);
    g_return_val_if_fail (IS_WAVEFORM (wf), -1);
    return WAVEFORM_GET_PRIVATE(wf)->patch;
}


void waveform_get_size (Waveform * wf, int *w, int *h)
{
    g_return_if_fail (wf != NULL);
    g_return_if_fail (IS_WAVEFORM (wf));
    WaveformPrivate* p = WAVEFORM_GET_PRIVATE(wf);
    *w = p->width;
    *h = p->height;
    return;
}


gboolean waveform_get_interactive (Waveform * wf)
{
     g_return_val_if_fail (wf != NULL, FALSE);
     g_return_val_if_fail (IS_WAVEFORM (wf), FALSE);
     return WAVEFORM_GET_PRIVATE(wf)->interactive;
}


void waveform_get_range (Waveform * wf, float *start, float *stop)
{
    g_return_if_fail (wf != NULL);
    g_return_if_fail (IS_WAVEFORM (wf));
    WaveformPrivate* p = WAVEFORM_GET_PRIVATE(wf);
    *start = p->range_start;
    *stop = p->range_stop;
}
