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

/* colors: from 0 to 65535 */
enum
{
     /* background fades from 1 to 2 values */
     BACK_R1 = 0,
     BACK_R2 = 0,
     BACK_G1 = 0,
     BACK_G2 = 0,
     BACK_B1 = 5000,
     BACK_B2 = 20000,

     /* grid colors */
     GRID_R1 = 0,
     GRID_R2 = 0,
     GRID_G1 = 5000,
     GRID_G2 = 10000,
     GRID_B1 = 15000,
     GRID_B2 = 30000,
     GRID_Y = 8,			/* number of sqaures in Y direction */

     /* color for center line */
     CENT_R = 0000,
     CENT_G = 15000,
     CENT_B = 30000,

     /* colors for waveform */
     WAVE_R = 0000,
     WAVE_G = 40000,
     WAVE_B = 60000,

     /* color to draw parts of waveform outside of start/stop points */
     WAVE_RO = 0,
     WAVE_GO = 20000,
     WAVE_BO = 30000,

     /* color to draw loop points */
     LOOP_R = 50000,
     LOOP_G = 30000,
     LOOP_B = 20000,
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

static gboolean waveform_expose (GtkWidget * widget,
				 GdkEventExpose * event);

static gboolean waveform_button_press (GtkWidget * widget,
				       GdkEventButton * event);

/* local data */
static GtkWidgetClass *parent_class = NULL;

GType waveform_get_type ( )
{
     static GType wf_type = 0;

     if (!wf_type)
     {
	  static const GTypeInfo wf_info = {
	       sizeof (WaveformClass),
	       NULL,
	       NULL,
	       (GClassInitFunc) waveform_class_init,
	       NULL,
	       NULL,
	       sizeof (Waveform),
	       0,
	       (GInstanceInitFunc) waveform_init,
	  };

	  wf_type =
	       g_type_register_static (GTK_TYPE_WIDGET, "Waveform", &wf_info,
				       0);
     }

     return wf_type;
}

static void waveform_class_init (WaveformClass * klass)
{
     GtkObjectClass *object_class;
     GtkWidgetClass *widget_class;

     object_class = (GtkObjectClass *) klass;
     widget_class = (GtkWidgetClass *) klass;

     parent_class = gtk_type_class (gtk_widget_get_type ( ));

     object_class->destroy = waveform_destroy;

     widget_class->realize = waveform_realize;
     widget_class->expose_event = waveform_expose;
     widget_class->size_request = waveform_size_request;
     widget_class->size_allocate = waveform_size_allocate;
     widget_class->button_press_event = waveform_button_press;

     signals[CHANGED] =
	  g_signal_new ("changed",
			G_TYPE_FROM_CLASS (klass),
			G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
			G_STRUCT_OFFSET (WaveformClass, changed),
			NULL, NULL,
			g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

}

static void waveform_init (Waveform * wf)
{
     wf->interactive = FALSE;
     wf->width = 0;
     wf->height = 0;
     wf->patch = -1;
     wf->range_start = 0.0;
     wf->range_stop = 1.0;
}

static void waveform_destroy (GtkObject * object)
{
     g_return_if_fail (object != NULL);
     g_return_if_fail (IS_WAVEFORM (object));

     if (GTK_OBJECT_CLASS (parent_class)->destroy)
	  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void waveform_realize (GtkWidget * widget)
{
     GdkWindowAttr attributes;
     gint attributes_mask;

     g_return_if_fail (widget != NULL);
     g_return_if_fail (IS_WAVEFORM (widget));

     GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

     attributes.x = widget->allocation.x;
     attributes.y = widget->allocation.y;
     attributes.width = widget->allocation.width;
     attributes.height = widget->allocation.height;
     attributes.wclass = GDK_INPUT_OUTPUT;
     attributes.window_type = GDK_WINDOW_CHILD;
     attributes.event_mask = gtk_widget_get_events (widget) |
	  GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK |
	  GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK |
	  GDK_POINTER_MOTION_HINT_MASK;
     attributes.visual = gtk_widget_get_visual (widget);
     attributes.colormap = gtk_widget_get_colormap (widget);

     attributes_mask =
	  GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
     widget->window =
	  gdk_window_new (widget->parent->window, &attributes,
			  attributes_mask);

     widget->style = gtk_style_attach (widget->style, widget->window);

     gdk_window_set_user_data (widget->window, widget);

     gtk_style_set_background (widget->style, widget->window,
			       GTK_STATE_ACTIVE);
}

static void
waveform_size_request (GtkWidget * widget, GtkRequisition * requisition)
{
     requisition->width = WAVEFORM (widget)->width;
     requisition->height = WAVEFORM (widget)->height;
}

static void
waveform_size_allocate (GtkWidget * widget, GtkAllocation * allocation)
{
     Waveform *wf;

     g_return_if_fail (widget != NULL);
     g_return_if_fail (IS_WAVEFORM (widget));
     g_return_if_fail (allocation != NULL);

     widget->allocation = *allocation;
     wf = WAVEFORM (widget);
     wf->width = allocation->width;
     wf->height = allocation->height;

     if (GTK_WIDGET_REALIZED (widget))
     {

	  gdk_window_move_resize (widget->window,
				  allocation->x, allocation->y,
				  allocation->width, allocation->height);

     }
}

static gboolean
waveform_expose (GtkWidget * widget, GdkEventExpose * event)
{
     g_return_val_if_fail (widget != NULL, FALSE);
     g_return_val_if_fail (IS_WAVEFORM (widget), FALSE);
     g_return_val_if_fail (event != NULL, FALSE);

     if (event->count > 0)
	  return FALSE;

     waveform_draw (WAVEFORM (widget));
     return FALSE;
}

static gboolean
waveform_button_press (GtkWidget * widget, GdkEventButton * event)
{
     Waveform *wf;
     int frames;
     int start;
     int stop;
     float fpp;			/* frames per pixel */
     int sel, control, chaos;

     g_return_val_if_fail (widget != NULL, FALSE);
     g_return_val_if_fail (IS_WAVEFORM (widget), FALSE);
     g_return_val_if_fail (event != NULL, FALSE);
     if (!WAVEFORM (widget)->interactive)
     {
	  return FALSE;
     }

     wf = WAVEFORM (widget);
     frames = patch_get_frames (wf->patch);
     start = wf->range_start * frames;
     stop = wf->range_stop * frames;
     fpp = (stop - start) * 1.0 / wf->width;

     sel = (event->x * fpp) + start;	/* sel is now set to the frame
					 * which corresponds to where the
					 * user clicked */
     if (event->type == GDK_BUTTON_PRESS)
     {

	  /* during this process we ensure that values aren't set
	   * which are bogus in relation to each other.  For example,
	   * we make sure that the sample starting point is not
	   * greater than it's stopping point.
	   */

	  /* left button: start point */
	  /* anything else: stop point */
	  if (event->state & GDK_CONTROL_MASK)
	  {			/* we set play points for control clicks */
	       if (event->button == 1)
	       {
		    control = patch_get_sample_stop (wf->patch);
		    if (sel < control)
		    {
			 patch_set_sample_start (wf->patch, sel);

			 /* adjust starting loop point if we need to */
			 chaos = patch_get_loop_start (wf->patch);
			 if (sel > chaos)
			      patch_set_loop_start (wf->patch, sel);
		    }
	       }
	       else
	       {

		    control = patch_get_sample_start (wf->patch);
		    if (sel > control)
		    {
			 patch_set_sample_stop (wf->patch, sel);

			 /* adjust stopping loop point if we need to */
			 chaos = patch_get_loop_stop (wf->patch);
			 if (sel < chaos)
			      patch_set_loop_stop (wf->patch, sel);
		    }
	       }
	  }
	  else
	  {			/* otherwise, we set loop points */
	       if (event->button == 1)
	       {
		    control = patch_get_sample_start (wf->patch);
		    chaos = patch_get_loop_stop (wf->patch);
		    if (sel > control && sel < chaos)
		    {
			 patch_set_loop_start (wf->patch, sel);
		    }
	       }
	       else
	       {

		    control = patch_get_sample_stop (wf->patch);
		    chaos = patch_get_loop_start (wf->patch);
		    if (sel < control && sel > chaos)
		    {
			 patch_set_loop_stop (wf->patch, sel);
		    }
	       }
	  }
     }

     g_signal_emit_by_name(G_OBJECT(wf), "changed");
     gtk_widget_queue_draw (widget);
     return FALSE;
}

/* draw background gradient */
inline static void draw_back (GdkDrawable * surface, int w, int h,
			      GdkGC * gc)
{
     int y;
     int center = h / 2;
     float d;
     GdkColor color;

     for (y = 0; y < h; y++)
     {
	  d = 1.0 - (abs (y - center) * 1.0 / (center));
	  color.red = BACK_R1 * (1 - d) + BACK_R2 * d;
	  color.green = BACK_G1 * (1 - d) + BACK_G2 * d;
	  color.blue = BACK_B1 * (1 - d) + BACK_B2 * d;
	  gdk_gc_set_rgb_fg_color (gc, &color);

	  gdk_draw_line (surface, gc, 0, y, w - 1, y);
     }
}

/* draw grid gradient */
inline static void draw_grid (GdkDrawable * surface, int w, int h,
			      GdkGC * gc)
{
     int x, y;
     int center = h / 2;
     float d;
     GdkColor color;

     for (y = 0; y < h; y++)
     {
	  d = 1.0 - (abs (y - center) * 1.0 / (center));
	  if (y == center)
	  {
	       color.red = CENT_R;
	       color.green = CENT_G;
	       color.blue = CENT_B;
	  }
	  else
	  {
	       color.red = GRID_R1 * (1 - d) + GRID_R2 * d;
	       color.green = GRID_G1 * (1 - d) + GRID_G2 * d;
	       color.blue = GRID_B1 * (1 - d) + GRID_B2 * d;
	  }
	  gdk_gc_set_rgb_fg_color (gc, &color);

	  /* draw horizontal line */
	  if (y % (h / GRID_Y) == 0)
	  {

	       gdk_draw_line (surface, gc, 0, y, w - 1, y);

	  }
	  else
	  {

	       /* draw vertical line components */
	       for (x = (h / GRID_Y); x < w; x += (h / GRID_Y))
	       {
		    gdk_draw_point (surface, gc, x, y);
	       }
	  }
     }
}

/* draw waveform, using a method similar to Brensenham's Line-Drawing
 * Algorithm */
inline static void draw_wave (Waveform* wf, GdkDrawable* surface,
			      int w, int h, GdkGC* gc)
{
     int center = h / 2;
     int frames;
     int play_start, play_stop;
     int start, stop;
     const float* wav;
     GdkColor color;

     if (wf->patch < 0)
	  return;

     if ((wav = patch_get_sample (wf->patch)) == NULL)
	  return;

     frames = patch_get_frames (wf->patch);
     start = frames * wf->range_start;
     stop = frames * wf->range_stop;
     play_start = patch_get_sample_start (wf->patch);
     play_stop = patch_get_sample_stop (wf->patch);

     /* draw waveform when pixels >= frames */
     if (w >= (stop - start))
     {
	  int lx = 0;		/* last x val */
	  int ly = center;	/* last y val */
	  int x = 0;		/* x index */
	  int y = 0;		/* y val */
	  int f = start;	/* frame index */
	  int ferr = 0;		/* frame error value */
	  int visframes		/* number of frames that will be drawn */
	       = stop - start;

	  for (x = 0; x < w; x++)
	  {
	       if ((ferr += visframes) >= w)
	       {
		    ferr -= w;
	       }
	       else
	       {
		    continue;
	       }

	       y = (wav[f*2] + 1) / 2 * h;

	       /* set line color */
	       if (f < play_start || f > play_stop)
	       {
		    color.red = WAVE_RO;
		    color.green = WAVE_GO;
		    color.blue = WAVE_BO;
	       }
	       else
	       {
		    color.red = WAVE_R;
		    color.green = WAVE_G;
		    color.blue = WAVE_B;
	       }

	       gdk_gc_set_rgb_fg_color (gc, &color);
	       gdk_draw_line (surface, gc, lx, ly, x, y);

	       f++;
	       lx = x;
	       ly = y;
	  }
     }

     /* draw waveform when pixels < frames */
     else
     {
	  float lminy = 0;	/* last min val */
	  float lmaxy = 0;	/* last max val */
	  float miny = 2;	/* min val found over interval */
	  float maxy = -2;	/* max val found over interval */
	  int draw_miny = 0;	/* pixel value of miny */
	  int draw_maxy = 0;	/* pixel value of maxy */
	  int lx = 0;		/* last x value (prevents trouble when x == 0) */
	  int xerr = 0;		/* x error value */
	  int x = 0;		/* x index */
	  int f = 0;		/* frame index */
	  int s = 0;		/* sample index */
	  int visframes		/* number of frames that will be drawn */
	       = stop - start;

	  for (f = start; f < stop; f++)
	  {
	       s = f * 2;

	       if (wav[s] > maxy)
	       {
		    maxy = wav[s];
	       }
	       if (wav[s] < miny)
	       {
		    miny = wav[s];
	       }

	       if ((xerr += w) >= visframes)
	       {
		    xerr -= visframes;
	       }
	       else
	       {
		    continue;
	       }

	       /* set color of line */
	       if (f < play_start || f > play_stop)
	       {
		    color.red = WAVE_RO;
		    color.green = WAVE_GO;
		    color.blue = WAVE_BO;
	       }
	       else
	       {
		    color.red = WAVE_R;
		    color.green = WAVE_G;
		    color.blue = WAVE_B;
	       }
	       gdk_gc_set_rgb_fg_color (gc, &color);

	       /* calculate drawing coordinates */
	       draw_miny = (miny + 1) / 2 * h;
	       draw_maxy = (maxy + 1) / 2 * h;

	       /* connect to previously drawn segment; we put off
		* converting the lm??y values so that they don't get
		* calculated if they aren't needed (efficiency) */
	       if (maxy < lminy)
	       {
		    gdk_draw_line (surface, gc, lx, (lminy+1)/2 * h, x, draw_maxy);
	       }
	       else if (miny > lmaxy)
	       {
		    gdk_draw_line (surface, gc, lx, (lmaxy+1)/2 * h, x, draw_miny);
	       }

	       /* connect min val to max val */
	       gdk_draw_line (surface, gc, x, draw_miny, x, draw_maxy);

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

inline static void draw_loop (Waveform * wf, GdkDrawable * surface, int w,
			      int h, GdkGC * gc)
{
     int frames;
     int start, stop;
     int loop_start, loop_stop;
     float ppf;
     GdkColor color;

     frames = patch_get_frames (wf->patch);
     start = frames * wf->range_start;
     stop = frames * wf->range_stop;
     loop_start = patch_get_loop_start (wf->patch);
     loop_stop = patch_get_loop_stop (wf->patch);
     ppf = w / (stop - start * 1.0);

     /* prepare to draw loop points */
     color.red = LOOP_R;
     color.green = LOOP_G;
     color.blue = LOOP_B;
     gdk_gc_set_rgb_fg_color (gc, &color);
     gdk_gc_set_function (gc, GDK_COPY);

     /* draw starting loop point */
     if (loop_start > start)
     {
	  loop_start = (loop_start - start) * ppf;
	  gdk_draw_line (surface, gc, loop_start, 0, loop_start, h - 1);
     }

     /* draw stopping loop point */
     if (loop_stop < stop)
     {
	  loop_stop = (loop_stop - start) * ppf;
	  gdk_draw_line (surface, gc, loop_stop, 0, loop_stop, h - 1);
     }
}

void waveform_draw (Waveform * wf)
{
     GtkWidget *widget = GTK_WIDGET (wf);
     GdkGC *gc;
     int w, h;			/* width and height of drawing area */

     /* prepate to draw */
     if (!GTK_WIDGET_REALIZED (widget))
	  return;
     gdk_drawable_get_size (GDK_DRAWABLE (widget->window), &w, &h);
     gc = gdk_gc_new (GDK_DRAWABLE (widget->window));

     /* draw each component */
     draw_back (GDK_DRAWABLE (widget->window), w, h, gc);
     draw_grid (GDK_DRAWABLE (widget->window), w, h, gc);
     draw_wave (wf, GDK_DRAWABLE (widget->window), w, h, gc);
     draw_loop (wf, GDK_DRAWABLE (widget->window), w, h, gc);

     g_object_unref (gc);
}

GtkWidget *waveform_new (int id, int w, int h, gboolean interactive)
{
     Waveform *wf;

     wf = g_object_new (waveform_get_type ( ), NULL);

     wf->patch = id;
     wf->width = w;
     wf->height = h;
     wf->interactive = interactive;

     return GTK_WIDGET (wf);
}

void waveform_set_patch (Waveform * wf, int id)
{
     g_return_if_fail (wf != NULL);
     g_return_if_fail (IS_WAVEFORM (wf));

     wf->patch = id;

     gtk_widget_queue_draw (GTK_WIDGET (wf));
}

void waveform_set_size (Waveform * wf, int w, int h)
{
     g_return_if_fail (wf != NULL);
     g_return_if_fail (IS_WAVEFORM (wf));

     wf->width = w;
     wf->height = h;

     gtk_widget_queue_resize (GTK_WIDGET (wf));
}

void waveform_set_interactive (Waveform * wf, gboolean interactive)
{
     g_return_if_fail (wf != NULL);
     g_return_if_fail (IS_WAVEFORM (wf));

     wf->interactive = interactive;

     gtk_widget_queue_draw (GTK_WIDGET (wf));
}

void waveform_set_range (Waveform * wf, float start, float stop)
{
     g_return_if_fail (wf != NULL);
     g_return_if_fail (IS_WAVEFORM (wf));

     wf->range_start = (start < 0.0 || start > 1.0
			|| start > stop) ? 0.0 : start;
     wf->range_stop = (stop < 0.0 || start > 1.0
		       || start > stop) ? 1.0 : stop;

     gtk_widget_queue_draw (GTK_WIDGET (wf));
}

int waveform_get_patch (Waveform * wf)
{
     g_return_val_if_fail (wf != NULL, -1);
     g_return_val_if_fail (IS_WAVEFORM (wf), -1);

     return wf->patch;
}

void waveform_get_size (Waveform * wf, int *w, int *h)
{
     g_return_if_fail (wf != NULL);
     g_return_if_fail (IS_WAVEFORM (wf));

     *w = wf->width;
     *h = wf->height;
     return;
}

gboolean waveform_get_interactive (Waveform * wf)
{
     g_return_val_if_fail (wf != NULL, FALSE);
     g_return_val_if_fail (IS_WAVEFORM (wf), FALSE);

     return wf->interactive;
}

void waveform_get_range (Waveform * wf, float *start, float *stop)
{
     g_return_if_fail (wf != NULL);
     g_return_if_fail (IS_WAVEFORM (wf));

     *start = wf->range_start;
     *stop = wf->range_stop;
}
