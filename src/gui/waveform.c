#include <stdlib.h>
#include <math.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>

#include "petri-foo.h"
#include "waveform.h"
#include "patch_set_and_get.h"

/* signals */
enum
{
    PLAY_CHANGED,
    LOOP_CHANGED,
    MARK_CHANGED,
    VIEW_CHANGED,
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

#define CENT_R 0.214443
#define CENT_G 0.328885
#define CENT_B 0.557771

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


static const char* mark_names[] = {
    "Sample start",
    "Play start",
    "Loop start",
    "Loop end",
    "Play end",
    "Sample end"
};


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

    gboolean view_mark;
    int mark;
};

static int patch_get_frame_first(int id)
{
    return 0;
}

static int patch_get_frame_last(int id)
{
    return patch_get_frames(id);
}


static int (*mark_get[WF_MARK_STOP + 1])(int) = {
    patch_get_frame_first,
    patch_get_sample_start,
    patch_get_loop_start,
    patch_get_loop_stop,
    patch_get_sample_stop,
    patch_get_frame_last
};

static int (*mark_set[WF_MARK_STOP + 1])(int, int) = {
    0,
    patch_set_sample_start,
    patch_set_loop_start,
    patch_set_loop_stop,
    patch_set_sample_stop,
    0
};



const char** waveform_get_mark_names(void)
{
    return mark_names;
}

/* forward declarations */

G_DEFINE_TYPE(Waveform, waveform, GTK_TYPE_DRAWING_AREA)

static void waveform_dispose(GObject * object);

static void waveform_size_request (GtkWidget * widget,
                                    GtkRequisition * requisition);

static gboolean waveform_expose(GtkWidget * widget,
                                    GdkEventExpose * event);

static gboolean waveform_button_press(GtkWidget * widget,
                                    GdkEventButton * event);

static gboolean waveform_configure(GtkWidget * widget,
                                    GdkEventConfigure * event);

static void waveform_draw(Waveform * wf);


static void waveform_class_init (WaveformClass * klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    waveform_parent_class = g_type_class_peek_parent(klass);

    object_class->dispose =             waveform_dispose;
    widget_class->expose_event =        waveform_expose;
    widget_class->configure_event =     waveform_configure;
    widget_class->size_request =        waveform_size_request;
    widget_class->button_press_event =  waveform_button_press;

    signals[PLAY_CHANGED] =
        g_signal_new   ("play-changed",
                        G_TYPE_FROM_CLASS(klass),
                        G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                        G_STRUCT_OFFSET(WaveformClass, play_changed),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__VOID, G_TYPE_NONE,
                        0, NULL);

    signals[LOOP_CHANGED] =
        g_signal_new   ("loop-changed",
                        G_TYPE_FROM_CLASS(klass),
                        G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                        G_STRUCT_OFFSET(WaveformClass, loop_changed),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__VOID, G_TYPE_NONE,
                        0, NULL);

    signals[MARK_CHANGED] =
        g_signal_new   ("mark-changed",
                        G_TYPE_FROM_CLASS(klass),
                        G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                        G_STRUCT_OFFSET(WaveformClass, mark_changed),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__VOID, G_TYPE_NONE,
                        0, NULL);

    signals[MARK_CHANGED] =
        g_signal_new   ("view-changed",
                        G_TYPE_FROM_CLASS(klass),
                        G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                        G_STRUCT_OFFSET(WaveformClass, view_changed),
                        NULL, NULL,
                        g_cclosure_marshal_VOID__VOID, G_TYPE_NONE,
                        0, NULL);

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
    p->mark = WF_MARK_PLAY_START;
    p->view_mark = TRUE; /* should nearly always be TRUE */
}


static void waveform_dispose(GObject * object)
{
    WaveformPrivate* p;

    g_return_if_fail (object != NULL);
    g_return_if_fail (IS_WAVEFORM (object));

    p = WAVEFORM_GET_PRIVATE(object);

    if (p->pixmap)
    {
        g_object_unref(p->pixmap);
        p->pixmap = NULL;
    }

    G_OBJECT_CLASS(waveform_parent_class)->dispose(object);
}


static void
waveform_size_request (GtkWidget * widget, GtkRequisition * requisition)
{
    WaveformPrivate* p = WAVEFORM_GET_PRIVATE(widget);

    requisition->width = p->width;
    requisition->height = p->height;
}


static gboolean
waveform_configure(GtkWidget * widget, GdkEventConfigure * event)
{
    (void)event;

    WaveformPrivate* p = WAVEFORM_GET_PRIVATE(widget);

    p->width = widget->allocation.width;
    p->height = widget->allocation.height;

    if (p->pixmap)
        g_object_unref(p->pixmap);

    p->pixmap = gdk_pixmap_new(widget->window, p->width, p->height, -1);

    waveform_draw(WAVEFORM(widget));

    return TRUE;
}


static gboolean
waveform_expose (GtkWidget * widget, GdkEventExpose * event)
{
    WaveformPrivate* p;
    cairo_t* cr;

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

    cr = gdk_cairo_create (widget->window);
    gdk_cairo_set_source_pixmap (cr, p->pixmap, 0, 0);
    gdk_cairo_rectangle (cr, &event->area);
    cairo_fill (cr);
    cairo_destroy (cr);
    return TRUE;
}


static gboolean
waveform_button_press (GtkWidget * widget, GdkEventButton * event)
{
    Waveform *wf;
    WaveformPrivate* p;

    int frames;
    int start;
    int stop;
    float fpp;			// frames per pixel 
    int sel, control, chaos;

    gboolean loop_changed = FALSE;
    gboolean play_changed = FALSE;

    int mark;


    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (IS_WAVEFORM (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    wf = WAVEFORM (widget);
    p = WAVEFORM_GET_PRIVATE(widget);

    if (!p->interactive)
        return FALSE;

    mark = p->mark;

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
        {   /* we set play points for CTRL+button-click */
            if (event->button == 1)
            {
                control = patch_get_sample_stop (p->patch);
                if (sel < control)
                {
                    patch_set_sample_start (p->patch, sel);
                    mark = WF_MARK_PLAY_START;
                    play_changed = TRUE;
                    /* adjust starting loop point if we need to */
                    chaos = patch_get_loop_start (p->patch);
                    if (sel > chaos)
                    {
                        patch_set_loop_start (p->patch, sel);
                        loop_changed = TRUE;
                    }
                }
            }
            else
            {
                control = patch_get_sample_start (p->patch);
                if (sel > control)
                {
                    patch_set_sample_stop (p->patch, sel);
                    mark = WF_MARK_PLAY_STOP;
                    play_changed = TRUE;
                    /* adjust stopping loop point if we need to */
                    chaos = patch_get_loop_stop (p->patch);
                    if (sel < chaos)
                    {
                        patch_set_loop_stop (p->patch, sel);
                        loop_changed = TRUE;
                    }
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
                {
                    patch_set_loop_start (p->patch, sel);
                    mark = WF_MARK_LOOP_START;
                    loop_changed = TRUE;
                }
            }
            else
            {
                control = patch_get_sample_stop (p->patch);
                chaos = patch_get_loop_start (p->patch);
                if (sel < control && sel > chaos)
                {
                    patch_set_loop_stop (p->patch, sel);
                    mark = WF_MARK_LOOP_STOP;
                    loop_changed = TRUE;
                }
            }
        }
    }

    if (play_changed)
        g_signal_emit_by_name(G_OBJECT(wf), "play-changed");

    if (loop_changed)
        g_signal_emit_by_name(G_OBJECT(wf), "loop-changed");

    if (mark != p->mark)
    {
        p->mark = mark;
        p->view_mark = FALSE;
        g_signal_emit_by_name(G_OBJECT(wf), "mark-changed");
    }

    if (play_changed || loop_changed)
    {
        waveform_draw(WAVEFORM(wf));
        gtk_widget_queue_draw (widget);
    }

    return FALSE;
}


inline static void
cr_rect(cairo_t* cr, double x1, double y1, double x2, double y2)
{
    cairo_rectangle(cr, x1, y1, x2 - x1, y2 - y1);
}


static void draw_back(WaveformPrivate* p, int w, int h, cairo_t* cr)
{
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


static void draw_grid(WaveformPrivate* p, int w, int h, cairo_t* cr)
{
    (void)p; /* p may come in useful at some point */
    int x, y;
    int center = h / 2;
    int step = h / GRID_Y;

    cairo_pattern_t *fg = cairo_pattern_create_linear (0, 0, 0, h);

    cairo_pattern_add_color_stop_rgb(fg, 0.0, GRID_R1, GRID_G1, GRID_B1 );
    cairo_pattern_add_color_stop_rgb(fg, 0.5, GRID_R2, GRID_G2, GRID_B2 );
    cairo_pattern_add_color_stop_rgb(fg, 1.0, GRID_R1, GRID_G1, GRID_B1 );

    cairo_set_source(cr, fg);
    cairo_set_line_width(cr, 1.0);

    /*  Massive improvement here using Cairo drawing with gradients
     *  over GDK and extra for-loops. The 0.5 bs keeps the lines sharp.
     */

    for (x = 0; x < w; x += step)
    {
        cairo_move_to(cr, 0.5 + x, 0.5);
        cairo_line_to(cr, 0.5 + x, 0.5 + h);
        cairo_stroke(cr);
    }

    for (y = step; y < center; y += step)
    {
        cairo_move_to (cr, 0.5,         0.5 + center - y);
        cairo_line_to (cr, 0.5 + w - 1, 0.5 + center - y);
        cairo_move_to (cr, 0.5,         0.5 + center + y);
        cairo_line_to (cr, 0.5 + w - 1, 0.5 + center + y);
        cairo_stroke(cr);
    }

    cairo_set_source_rgb(cr, CENT_R, CENT_G, CENT_B);

    cairo_move_to (cr, 0.5,         0.5 + center);
    cairo_line_to (cr, 0.5 + w - 1, 0.5 + center);
    cairo_stroke(cr);
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

        cairo_set_line_width(cr, 1.0);

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

        cairo_set_line_width(cr, 1.0);

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
            cairo_stroke(cr);
            }
            else if (miny > lmaxy)
            {
                cairo_move_to (cr, lx, (lmaxy+1)/2 * h);
                cairo_line_to (cr, x, draw_miny);
            cairo_stroke(cr);
            }

            /* connect min val to max val */
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


char* int_to_str(int n, const char* fmt)
{
    char buf[80];

    if (snprintf(buf, 80, fmt, n) < 0)
        return 0;

    int len = strlen(buf);
    char* str = malloc(len + 1);

    if (!str)
        return 0;

    strcpy(str, buf);

    return str;
}


void draw_mark(WaveformPrivate* p, int w, int h, cairo_t* cr)
{
    int frames;
    int start, stop;
    int play_start, play_stop;
    int loop_start, loop_stop;
    float ppf;

    float textheight = w * (32.0f / 1680);

    char* pstart = "play";
    char* pstop = pstart;
    char* lstart = "loop";
    char* lstop = lstart;

    cairo_text_extents_t extents;

    frames = patch_get_frames (p->patch);

    if (frames <= 0)
        return;

    start = frames * p->range_start;
    stop = frames * p->range_stop;
    play_start = patch_get_sample_start (p->patch);
    play_stop = patch_get_sample_stop (p->patch);
    loop_start = patch_get_loop_start (p->patch);
    loop_stop = patch_get_loop_stop (p->patch);

    ppf = w / (stop - start * 1.0);

    cairo_set_line_width(cr, 1.0);

    if (p->interactive)
    {
        pstart = int_to_str(play_start, "%d");
        pstop = int_to_str(play_stop, "%d");
        lstart = int_to_str(loop_start, "%d");
        lstop = int_to_str(loop_stop, "%d");

        cairo_select_font_face (cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                                            CAIRO_FONT_WEIGHT_NORMAL );
        if (textheight < 8.0)
            textheight = 8.0;
        else if (textheight > 16.0)
            textheight = 16.0;

        cairo_set_font_size (cr, textheight);
    }

    cairo_set_source_rgb(cr, WAVE_PLAY_R, WAVE_PLAY_G, WAVE_PLAY_B);

    if (play_start >= start)
    {
        play_start = (play_start - start) * ppf;
        cairo_move_to(cr, 0.5 + play_start, 0);
        cairo_line_to(cr, 0.5 + play_start, h - 1);

        if (p->interactive)
        {
            cairo_text_extents (cr, pstart, &extents);
            cairo_move_to(cr, 0.5 + play_start + 1,
                                    extents.height - extents.y_bearing);
            cairo_show_text (cr, pstart);
        }
    }

    if (play_stop < stop)
    {
        play_stop = (play_stop - start) * ppf;
        cairo_move_to(cr, 0.5 + play_stop, 0);
        cairo_line_to(cr, 0.5 + play_stop, h - 1);

        if (p->interactive)
        {
            cairo_text_extents (cr, pstop, &extents);
            cairo_move_to(cr, 0.5 + play_stop - extents.x_advance
                          - extents.x_bearing - 1, h + extents.y_bearing);
            cairo_show_text (cr, pstop);
        }
    }

    cairo_stroke(cr);

    cairo_set_source_rgb(cr, WAVE_LOOP_R, WAVE_LOOP_G, WAVE_LOOP_B);

    if (loop_start >= start)
    {
        loop_start = (loop_start - start) * ppf;
        cairo_move_to(cr, 0.5 + loop_start, 0);
        cairo_line_to(cr, 0.5 + loop_start, h - 1);

        if (p->interactive)
        {
            cairo_text_extents (cr, lstart, &extents);
            cairo_move_to(cr, 0.5 + loop_start + 1, h + extents.y_bearing);
            cairo_show_text (cr, lstart);
        }
    }

    if (loop_stop < stop)
    {
        loop_stop = (loop_stop - start) * ppf;
        cairo_move_to(cr, 0.5 + loop_stop, 0);
        cairo_line_to(cr, 0.5 + loop_stop, h - 1);

        if (p->interactive)
        {
            cairo_text_extents (cr, lstop, &extents);
            cairo_move_to(cr, 0.5 + loop_stop
                                - extents.x_advance
                                - extents.x_bearing - 1,
                            extents.height - extents.y_bearing);
            cairo_show_text (cr, lstop);
        }
    }

    cairo_stroke(cr);

    if (p->interactive)
    {
        free(pstart);
        free(pstop);
        free(lstart);
        free(lstop);
    }
}


static void waveform_draw(Waveform * wf)
{
    GtkWidget* widget = GTK_WIDGET(wf);
    WaveformPrivate* p = WAVEFORM_GET_PRIVATE(wf);

    cairo_t* cr;

    if (!gtk_widget_get_realized(widget))
        return;

    debug("drawing...\n");

    cr = gdk_cairo_create(p->pixmap);

    cairo_rectangle(cr, 0, 0, p->width, p->height);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_fill_preserve(cr);
    cairo_set_source_rgb(cr, 0, 0, 0);
    cairo_stroke(cr);

    draw_back(p, p->width, p->height, cr);
    draw_grid(p, p->width, p->height, cr);
    draw_wave(p, p->width, p->height, cr);
    draw_mark(p, p->width, p->height, cr);

    cairo_destroy(cr);
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

int waveform_detect_single_mark(Waveform* wf)
{
    WaveformPrivate* p = WAVEFORM_GET_PRIVATE(wf);
    float start = p->range_start;
    float stop = p->range_stop;

    int frames = patch_get_frames(p->patch);
    int frame = (start + (stop - start) / 2) * frames;

    int x0 = start * frames;
    int x1 = stop * frames;

    int prev = mark_get[WF_MARK_START](p->patch);
    int cur =  mark_get[WF_MARK_PLAY_START](p->patch);
    int next;

    int m;
    gboolean set = FALSE;

    debug("frames:%d x0:%d x1:%d\n",x0,x1);

    for (m = WF_MARK_PLAY_START; m < WF_MARK_STOP; ++m)
    {
        if (prev < x0 && (next = mark_get[m + 1](p->patch)) > x1
         && cur >= x0 && cur <= x1)
        {
            debug("\t\t\tmark:%s is in view\n",mark_names[m]);
            set = TRUE;
            break;
        }
        prev = cur;
        cur = next;
    }

    if (set)
        return m;

    return -1;
}

int waveform_detect_nearest_mark(Waveform* wf, int frame)
{
    WaveformPrivate* p = WAVEFORM_GET_PRIVATE(wf);
    int m;
    int small_diff = patch_get_frames(p->patch);
    int small_mark = -1;

    for (m = WF_MARK_PLAY_START; m < WF_MARK_STOP; ++m)
    {
        int diff = abs(frame - mark_get[m](p->patch));
        if (diff < small_diff)
        {
            small_diff = diff;
            small_mark = m;
        }
    }

    return small_mark;
}


void waveform_set_range (Waveform * wf, float start, float stop)
{
    g_return_if_fail (wf != NULL);
    g_return_if_fail (IS_WAVEFORM (wf));

    WaveformPrivate* p = WAVEFORM_GET_PRIVATE(wf);
    int mark;

    p->range_start = (start < 0.0 || start > 1.0 || start > stop)
                            ? 0.0 
                            : start;

    p->range_stop = (stop < 0.0 || start > 1.0 || start > stop)
                            ? 1.0
                            : stop;

    debug("start:%f stop:%f\n",start,stop);
    debug("p->range_start:%f p->range_stop:%f\n",
           p->range_start,   p->range_stop);

    mark = waveform_detect_single_mark(wf);

    if (mark == -1)
    {
        int frames = patch_get_frames(p->patch);
        int frame = (start + (stop - start) / 2) * frames;
        mark = waveform_detect_nearest_mark(wf, frame);
    }

    if (mark > -1)
    {
        p->view_mark = FALSE;
        waveform_set_mark(wf, mark);
        p->view_mark = TRUE;
    }

    gtk_widget_queue_draw (GTK_WIDGET (wf));
}


void waveform_view_mark(Waveform* wf)
{
    WaveformPrivate* p = WAVEFORM_GET_PRIVATE(wf);
    int f = waveform_get_mark_frame(wf);
    int frames = patch_get_frames(p->patch);
    int x0 = frames * p->range_start;
    int x1 = frames * p->range_stop;
    int w = x1 - x0;
    int hw = w / 2;

    x0 = f - hw;
    x1 = f + hw;

    if (x0 < 0)
    {
        x0 = 0;
        x1 = w;
    }
    else if (x1 > frames)
    {
        x1 = frames;
        x0 = x1 - w;
    }

debug("x0:%d x1:%d\n", x0, x1);

    p->range_start = x0 / (float)frames;
    p->range_stop = x1 / (float)frames;

    gtk_widget_queue_draw(GTK_WIDGET(wf));

    g_signal_emit_by_name(G_OBJECT(wf), "view-changed");
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

static gboolean
valid_mark_and_frame_to_set(WaveformPrivate* p, int mark_id, int frame)
{
    int pstart = patch_get_sample_start(p->patch);
    int pstop = patch_get_sample_stop(p->patch);
    int lstart = patch_get_loop_start(p->patch);
    int lstop = patch_get_loop_stop(p->patch);

    if (mark_id < WF_MARK_PLAY_START
     || mark_id > WF_MARK_PLAY_STOP)
    {   /* cannot change sample start or stop marks */
        return FALSE;
    }

    switch(mark_id)
    {
    case WF_MARK_PLAY_START:
        if (frame >= pstop)
            return FALSE;
        if (frame < 0 || frame > lstart)
            return FALSE;
        break;
    case WF_MARK_LOOP_START:
        if (frame < pstart || frame >= lstop)
            return FALSE;
        break;
    case WF_MARK_LOOP_STOP:
        if (frame <= lstart || frame > pstop)
            return FALSE;
        break;
    case WF_MARK_PLAY_STOP:
        if (frame <= pstart || frame <= lstop)
            return FALSE;
        break;
    default: /* never valid whatever the case */
        return FALSE;
    }

    return TRUE;
}

void waveform_goto_mark_prev (Waveform* wf)
{
    WaveformPrivate* p = WAVEFORM_GET_PRIVATE(wf);
}

void waveform_goto_mark_next (Waveform* wf)
{
    WaveformPrivate* p = WAVEFORM_GET_PRIVATE(wf);
}

void waveform_set_mark(Waveform* wf, int mark_id)
{
    WaveformPrivate* p = WAVEFORM_GET_PRIVATE(wf);

    if (mark_id < WF_MARK_START || mark_id > WF_MARK_STOP)
        return;

    debug("setting mark to %d\n",mark_id);

    g_signal_emit_by_name(G_OBJECT(wf), "mark-changed");

    p->mark = mark_id;

    if (p->view_mark)
        waveform_view_mark(wf);
}

int waveform_get_mark(Waveform* wf)
{
    WaveformPrivate* p = WAVEFORM_GET_PRIVATE(wf);
    return p->mark;
}

int waveform_get_mark_spin_range(Waveform* wf, int* min, int* max)
{
    WaveformPrivate* p = WAVEFORM_GET_PRIVATE(wf);

    switch(p->mark)
    {
    case WF_MARK_START:
        break;
    case WF_MARK_STOP:
        *min = *max = patch_get_frames(p->patch) - 1;
        return -1;
    case WF_MARK_PLAY_START:
        *min = 0;
        *max = patch_get_loop_start(p->patch);
        return patch_get_sample_start(p->patch);
    case WF_MARK_PLAY_STOP:
        *min = patch_get_loop_stop(p->patch);
        *max = patch_get_frames(p->patch) - 1;
        return patch_get_sample_stop(p->patch);
    case WF_MARK_LOOP_START:
        *min = patch_get_sample_start(p->patch);
        *max = patch_get_loop_stop(p->patch) - 1;
        return patch_get_loop_start(p->patch);
    case WF_MARK_LOOP_STOP:
        *min = patch_get_loop_start(p->patch) + 1;
        *max = patch_get_sample_stop(p->patch);
        return patch_get_loop_stop(p->patch);
    default:
        break;
    }

    *min = *max = 0;
    return -1;
}

void waveform_set_mark_frame(Waveform* wf, int frame)
{
    WaveformPrivate* p = WAVEFORM_GET_PRIVATE(wf);

    if (!valid_mark_and_frame_to_set(p, p->mark, frame))
        return;

    switch(p->mark)
    {
    case WF_MARK_PLAY_START:
        patch_set_sample_start(p->patch, frame);
        break;
    case WF_MARK_LOOP_START:
        patch_set_loop_start(p->patch, frame);
        break;
    case WF_MARK_LOOP_STOP:
        patch_set_loop_stop(p->patch, frame);
        break;
    case WF_MARK_PLAY_STOP:
        patch_set_sample_stop(p->patch, frame);
        break;
    default:
        break;
    }
}

int waveform_get_mark_frame(Waveform* wf)
{
    WaveformPrivate* p = WAVEFORM_GET_PRIVATE(wf);
    return mark_get[p->mark](p->patch);
}
