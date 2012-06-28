/*  Phin is a fork of the PHAT Audio Toolkit.
    Phin is part of Petri-Foo. Petri-Foo is a fork of Specimen.

    Original author Pete Bessman
    Copyright 2005 Pete Bessman
    Copyright 2011 James W. Morris

    This file is part of Phin.

    Phin is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.

    Phin is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Phin.  If not, see <http://www.gnu.org/licenses/>.

    This file is a derivative of a PHAT original, modified 2011
*/
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <math.h>
#include "phinprivate.h"
#include "phinslider.h"

/* magic numbers */
enum
{
    SLIDER_WIDTH = 12,
    SLIDER_LENGTH = 48,
    SLIDER_DECO = 1,
    THRESHOLD = 4,
};

enum
{
    EDGE_V_WITH_0X =0x0001,
    EDGE_H_WITH_0X =0x0002,
    EDGE_SHADE =    0x0010,
    EDGE_HI =       0x0020,
    EDGE_INNER =    0x0040,
    EDGE_OUTER =    0x0080,
    EDGE_ZERO =     0x0F00
};


/* states */
enum
{
    STATE_NORMAL,
    STATE_CLICKED,
    STATE_SCROLL,
};

#define VALUE_RATIO_NORMAL  1.00
#define VALUE_RATIO_SHIFT   0.10
#define VALUE_RATIO_CTRL    0.01



/* signals */
enum
{
    VALUE_CHANGED_SIGNAL,
    CHANGED_SIGNAL,
    LAST_SIGNAL,
};

static int signals[LAST_SIGNAL];


typedef struct _PhinSliderPrivate PhinSliderPrivate;

#define PHIN_SLIDER_GET_PRIVATE(obj) \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), PHIN_TYPE_SLIDER, \
                                        PhinSliderPrivate))

struct _PhinSliderPrivate
{
    GtkAdjustment* adjustment;
    GtkAdjustment* adjustment_prv;
    double         val;
    double         center_val;
    int            xclick_root;
    int            yclick_root;
    int            xclick;
    int            yclick;
    double         value_update_ratio;
    int            state;
    gboolean       inverted;
    int            direction;
    gboolean       is_log;
    GtkOrientation orientation;
    GdkCursor*     arrow_cursor;
    GdkCursor*     empty_cursor;
    GdkWindow*     event_window;
    gboolean       use_default_value;
    gdouble        default_value;
};


/* forward declarations */
G_DEFINE_TYPE(PhinSlider, phin_slider, GTK_TYPE_WIDGET);


static void phin_slider_dispose     (GObject* object);
static void phin_slider_realize     (GtkWidget* widget);
static void phin_slider_unrealize   (GtkWidget *widget);
static void phin_slider_map         (GtkWidget *widget);
static void phin_slider_unmap       (GtkWidget *widget);

static void phin_slider_size_request        (GtkWidget*widget,
                                            GtkRequisition* requisition);

static void phin_slider_size_allocate       (GtkWidget* widget,
                                            GtkAllocation* allocation);

static gboolean phin_slider_expose          (GtkWidget* widget,
                                            GdkEventExpose* event);

static gboolean phin_slider_button_press    (GtkWidget* widget,
                                            GdkEventButton* event);

static gboolean phin_slider_button_release  (GtkWidget* widget,
                                            GdkEventButton* event);

static gboolean phin_slider_key_press       (GtkWidget* widget,
                                            GdkEventKey* event);

static gboolean phin_slider_scroll          (GtkWidget* widget,
                                            GdkEventScroll* event);

static gboolean phin_slider_motion_notify   (GtkWidget* widget,
                                            GdkEventMotion* event);

static void phin_slider_calc_layout         (PhinSlider* slider,
                                            int* x, int* y, int* w, int* h);

static void phin_slider_update_value        (PhinSlider* slider,
                                            int x_root, int y_root);

static void phin_slider_adjustment_changed  (GtkAdjustment* adjustment,
                                            PhinSlider* slider);

static void phin_slider_adjustment_value_changed(GtkAdjustment*,
                                                    PhinSlider*);


/**
 * phin_slider_set_value:
 * @slider: a #PhinSlider
 * @value: a new value for the slider
 * 
 * Sets the current value of the slider.  If the value is outside the
 * range of values allowed by @slider, it will be clamped to fit
 * within them.  The slider emits the "value-changed" signal if the
 * value changes.
 *
 */
void phin_slider_set_value (PhinSlider* slider, double value)
{
    PhinSliderPrivate* p = PHIN_SLIDER_GET_PRIVATE(slider);

    gdouble lower = gtk_adjustment_get_lower(p->adjustment);
    gdouble upper = gtk_adjustment_get_upper(p->adjustment);

    value = CLAMP (value, lower, upper);

    gtk_adjustment_set_value (p->adjustment, value);

    if(p->is_log)
    {
        gtk_adjustment_set_value(GTK_ADJUSTMENT(p->adjustment_prv),
                            log(value - lower) / log(upper - lower));
    }
    else
    {
        gtk_adjustment_set_value(GTK_ADJUSTMENT(p->adjustment_prv),
                            (value - lower) / (upper - lower));
    }
}


void phin_slider_set_log (PhinSlider* slider, gboolean is_log)
{
    PhinSliderPrivate* p = PHIN_SLIDER_GET_PRIVATE(slider);
    p->is_log = is_log;
}


gboolean phin_slider_is_log (PhinSlider* slider)
{
    PhinSliderPrivate* p = PHIN_SLIDER_GET_PRIVATE(slider);
    return p->is_log;
}


/**
 * phin_slider_get_value:
 * @slider: a #PhinSlider
 *
 * Retrieves the current value of the slider.
 *
 * Returns: current value of the slider.
 *
 */
double phin_slider_get_value (PhinSlider* slider)
{
    PhinSliderPrivate* p = PHIN_SLIDER_GET_PRIVATE(slider);

    gdouble lower = gtk_adjustment_get_lower(p->adjustment);
    gdouble upper = gtk_adjustment_get_upper(p->adjustment);
    gdouble value = 0;
    gdouble prv_value = gtk_adjustment_get_value(p->adjustment_prv);

    if(p->is_log)
    {   /* FIXME: log scale */
        value = exp(prv_value * log(upper - lower)) + lower;
    }
    else
    {
        value = prv_value * (upper - lower) + lower;
    }

    /* not sure what the purpose of this set value call is:
    gtk_adjustment_set_value(p->adjustment, value); */

    return value;
}

/*
http://stackoverflow.com/questions/846221/logarithmic-slider/846249#846249
  function logslider(value) {
  // value will be between 0 and 100
  var slidermin = 0;
  var slidermax = 100;

  // The result should be between 100 an 10000000
  var minv = Math.log(100);
  var maxv = Math.log(10000000);

  // calculate adjustment factor
  var scale = (maxv-minv) / (slidermax-slidermin);

  return Math.exp(minv + scale*(value-slidermin));
}

*/
/**
 * phin_slider_set_range:
 * @slider: a #PhinSlider
 * @lower: lowest allowable value
 * @upper: highest allowable value
 * 
 * Sets the range of allowable values for the slider, and  clamps the slider's
 * current value to be between @lower and @upper.
 *
 */
void phin_slider_set_range (PhinSlider* slider,
                                double lower, double upper)
{
    PhinSliderPrivate* p = PHIN_SLIDER_GET_PRIVATE(slider);
    double value;

    g_return_if_fail (lower <= upper);

    gtk_adjustment_set_lower(p->adjustment, lower);
    gtk_adjustment_set_upper(p->adjustment, upper);

    value = CLAMP(gtk_adjustment_get_value(p->adjustment), lower, upper);

    // XXX not sure about these
    gtk_adjustment_changed(p->adjustment);
    gtk_adjustment_set_value(p->adjustment, value);
}

/**
 * phin_slider_get_range:
 * @slider: a #PhinSlider
 * @lower: retrieves lowest allowable value
 * @upper: retrieves highest allowable value
 *
 * Places the range of allowable values for @slider into @lower
 * and @upper.  Either variable may be set to %NULL if you are not
 * interested in its value.
 *
 */
void phin_slider_get_range (PhinSlider* slider,
                                double* lower, double* upper)
{
    PhinSliderPrivate* p = PHIN_SLIDER_GET_PRIVATE(slider);

    if (lower)
        *lower = gtk_adjustment_get_lower(p->adjustment);
    if (upper)
        *upper = gtk_adjustment_get_upper(p->adjustment);
}

/**
 * phin_slider_set_adjustment:
 * @slider: a #PhinSlider
 * @adjustment: a #GtkAdjustment
 *
 * Sets the adjustment used by @slider.  Every #PhinSlider uses an
 * adjustment to store its current value and its range of allowable
 * values.  If @adjustment is %NULL, a new adjustment with a value of
 * zero and a range of [-1.0, 1.0] will be created.
 *
 */
void phin_slider_set_adjustment (PhinSlider* slider,
                                     GtkAdjustment* adjustment)
{
    PhinSliderPrivate* p = PHIN_SLIDER_GET_PRIVATE(slider);

    g_return_if_fail (p->adjustment != adjustment);

    if (!adjustment)
        adjustment = (GtkAdjustment*)
                        gtk_adjustment_new (0.0, -1.0, 1.0, 1.0, 1.0, 0.0);
    else
        g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));

    if (p->adjustment)
    {
        g_signal_handlers_disconnect_by_func(
                                p->adjustment,
                                phin_slider_adjustment_changed,
                                (gpointer)slider);

        g_signal_handlers_disconnect_by_func(
                                p->adjustment,
                                phin_slider_adjustment_value_changed,
                                (gpointer)slider);

        g_object_unref (p->adjustment);
    }

    p->adjustment = adjustment;
    g_object_ref (adjustment);
    g_object_ref_sink (GTK_OBJECT (adjustment));

    phin_slider_adjustment_changed(p->adjustment, slider);

    phin_slider_set_value(PHIN_SLIDER (slider),
                                gtk_adjustment_get_value(adjustment));
}

/**
 * phin_slider_get_adjustment:
 * @slider: a #PhinSlider
 *
 * Retrives the current adjustment in use by @slider.
 *
 * Returns: @slider's current #GtkAdjustment
 *
 */
GtkAdjustment* phin_slider_get_adjustment (PhinSlider* slider)
{
    PhinSliderPrivate* p = PHIN_SLIDER_GET_PRIVATE(slider);

    /* I can't imagine this ever being true, but just to be
     * "safe" ... */
    if (!p->adjustment)
        phin_slider_set_adjustment (slider, NULL);

    return p->adjustment;
}

/**
 * phin_slider_set_inverted:
 * @slider: a #PhinSlider
 * @inverted: %TRUE to invert the slider
 *  
 * Sets in which direction the slider should draw increasing
 * values.  By default, horizontal sliders draw low to high from
 * left to right, and vertical sliders draw from bottom to top.
 * You can reverse this behavior by setting @inverted to %TRUE.
 * 
 */
void phin_slider_set_inverted (PhinSlider* slider, gboolean inverted)
{
    PhinSliderPrivate* p = PHIN_SLIDER_GET_PRIVATE(slider);

    p->inverted = inverted;
    gtk_widget_queue_draw (GTK_WIDGET (slider));
}

/**
 * phin_slider_get_inverted:
 * @slider: a #PhinSlider
 *
 * Determines whether @slider is inverted or not.
 *
 * Returns: %TRUE if @slider is inverted
 *
 */
gboolean phin_slider_get_inverted (PhinSlider* slider)
{
    PhinSliderPrivate* p = PHIN_SLIDER_GET_PRIVATE(slider);
    return p->inverted;
}

/**
 * phin_slider_set_default_value:
 * @slider: a #PhinSlider
 * @value: the default value
 *  
 * Set default value of the slider. Slider is reset to this value
 * when middle mouse button is pressed.
 */
void phin_slider_set_default_value(PhinSlider* slider, gdouble value)
{
    PhinSliderPrivate* p = PHIN_SLIDER_GET_PRIVATE(slider);

    p->use_default_value = TRUE;
    p->default_value = value;
}

void phin_slider_set_orientation(PhinSlider* slider,
                                     GtkOrientation o)
{
    PhinSliderPrivate* p = PHIN_SLIDER_GET_PRIVATE(slider);
    p->orientation = o;
}


static void phin_slider_class_init (PhinSliderClass* klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    GtkWidgetClass* widget_class = GTK_WIDGET_CLASS(klass);
    phin_slider_parent_class = g_type_class_peek_parent(klass);

    debug ("class init\n");

    phin_slider_parent_class = g_type_class_peek(gtk_widget_get_type());

    object_class->dispose =                 phin_slider_dispose;
    widget_class->realize =                 phin_slider_realize;
    widget_class->unrealize =               phin_slider_unrealize;
    widget_class->map =                     phin_slider_map;
    widget_class->unmap =                   phin_slider_unmap;
    widget_class->expose_event =            phin_slider_expose;
    widget_class->size_request =            phin_slider_size_request;
    widget_class->size_allocate =           phin_slider_size_allocate;
    widget_class->button_press_event =      phin_slider_button_press;
    widget_class->button_release_event =    phin_slider_button_release;
    widget_class->key_press_event =         phin_slider_key_press;
    widget_class->scroll_event =            phin_slider_scroll;
    widget_class->motion_notify_event =     phin_slider_motion_notify;

    /**
     * PhinSlider::value-changed:
     * @slider: the object on which the signal was emitted
     *
     * The "value-changed" signal is emitted when the value of the
     * slider's adjustment changes.
     *
     */
    signals[VALUE_CHANGED_SIGNAL] =
        g_signal_new ("value-changed",
                      G_TYPE_FROM_CLASS(klass),
                      G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET(PhinSliderClass, value_changed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

    /**
     * PhinSlider::changed:
     * @slider: the object on which the signal was emitted
     *
     * The "changed" signal is emitted when any parameter of the
     * slider's adjustment changes, except for the %value parameter.
     *
     */
    signals[CHANGED_SIGNAL] =
        g_signal_new ("changed",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (PhinSliderClass, changed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

    klass->value_changed = NULL;
    klass->changed = NULL;

    g_type_class_add_private(object_class, sizeof(PhinSliderPrivate));
}


static void phin_slider_init (PhinSlider* slider)
{
    PhinSliderPrivate* p = PHIN_SLIDER_GET_PRIVATE(slider);

    gtk_widget_set_has_window(GTK_WIDGET(slider), FALSE);
    gtk_widget_set_can_focus(GTK_WIDGET(slider), TRUE);

    debug ("init\n");

    p->adjustment = NULL;
    p->adjustment_prv = (GtkAdjustment*)
                        gtk_adjustment_new (0.0, 0.0, 1.0, 0.1, 0.1, 0.0);
    p->val = 0.69;
    p->center_val = -1;
    p->xclick_root = 0;
    p->yclick_root = 0;
    p->xclick = 0;
    p->yclick = 0;
    p->value_update_ratio = VALUE_RATIO_NORMAL;
    p->inverted = FALSE;
    p->direction = 0;
    p->state = STATE_NORMAL;
    p->orientation = GTK_ORIENTATION_HORIZONTAL;
    p->arrow_cursor = NULL;
    p->empty_cursor = NULL;
    p->event_window = NULL;
    p->is_log = 0;
    p->use_default_value = FALSE;

    g_signal_connect (p->adjustment_prv, "changed",
                      G_CALLBACK (phin_slider_adjustment_changed),
                      (gpointer) slider);
    g_signal_connect (p->adjustment_prv, "value_changed",
                      G_CALLBACK (phin_slider_adjustment_value_changed),
                      (gpointer) slider);

    phin_slider_adjustment_changed (p->adjustment_prv, slider);
    phin_slider_adjustment_value_changed (p->adjustment_prv, slider);

}


static void phin_slider_dispose (GObject* object)
{
    PhinSlider* slider;
    PhinSliderPrivate* p;

    g_return_if_fail (object != NULL);
    g_return_if_fail (PHIN_IS_SLIDER (object));

    slider = PHIN_SLIDER(object);
    p = PHIN_SLIDER_GET_PRIVATE(object);

    debug ("dispose %p\n", object);

    if (p->arrow_cursor != NULL)
    {
        gdk_cursor_unref (p->arrow_cursor);
        p->arrow_cursor = NULL;
    }

    if (p->empty_cursor != NULL)
    {
        gdk_cursor_unref (p->empty_cursor);
        p->empty_cursor = NULL;
    }

    if (p->event_window != NULL)
    {
        gdk_window_destroy (p->event_window);
        p->event_window = NULL;
    }

    if (p->adjustment)
    {
        g_signal_handlers_disconnect_by_func(
                                p->adjustment,
                                phin_slider_adjustment_changed,
                                (gpointer)slider);

        g_signal_handlers_disconnect_by_func(
                                p->adjustment,
                                phin_slider_adjustment_value_changed,
                                (gpointer)slider);

        /* called ref on it so we must call unref */
        g_object_unref (p->adjustment);
        p->adjustment = NULL;
    }

    if (p->adjustment_prv)
    {
        g_signal_handlers_disconnect_by_func(
                                p->adjustment_prv,
                                phin_slider_adjustment_changed,
                                (gpointer)slider);

        g_signal_handlers_disconnect_by_func(
                                p->adjustment_prv,
                                phin_slider_adjustment_value_changed,
                                (gpointer)slider);
    }
}


static void phin_slider_realize (GtkWidget* widget)
{
    PhinSlider* slider;
    PhinSliderPrivate* p;
    GdkWindowAttr attributes;
    gint attributes_mask;
    GdkWindow* window;
    GtkStyle* style;
    debug ("realize\n");

    g_return_if_fail (widget != NULL);
    g_return_if_fail (PHIN_IS_SLIDER (widget));

    gtk_widget_set_realized(GTK_WIDGET(widget), TRUE);

    slider = PHIN_SLIDER(widget);
    p = PHIN_SLIDER_GET_PRIVATE(widget);

    if (p->orientation == GTK_ORIENTATION_VERTICAL)
    {
        p->arrow_cursor = gdk_cursor_new (GDK_SB_V_DOUBLE_ARROW);
    }
    else
    {
        p->arrow_cursor = gdk_cursor_new (GDK_SB_H_DOUBLE_ARROW);
    }

    p->empty_cursor = gdk_cursor_new(GDK_BLANK_CURSOR);

    window = gtk_widget_get_parent_window (widget);
    gtk_widget_set_window(widget, window);
    g_object_ref (window);


    style = gtk_widget_get_style(widget);
    style = gtk_style_attach(style, window);
/*    gtk_widget_set_style(widget, style);*/

    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.wclass      = GDK_INPUT_ONLY;
    attributes.event_mask  = gtk_widget_get_events (widget);

    attributes.event_mask |= (GDK_BUTTON_PRESS_MASK
                              | GDK_BUTTON_RELEASE_MASK
                              | GDK_POINTER_MOTION_MASK
                              | GDK_POINTER_MOTION_HINT_MASK
                              | GDK_ENTER_NOTIFY_MASK
                              | GDK_LEAVE_NOTIFY_MASK
                              | GDK_SCROLL_MASK);

    phin_slider_calc_layout(slider,
                                 &attributes.x,
                                 &attributes.y,
                                 &attributes.width,
                                 &attributes.height);

    attributes_mask = GDK_WA_X | GDK_WA_Y;

    p->event_window = gdk_window_new (gtk_widget_get_parent_window (widget),
                                           &attributes, attributes_mask);
    gdk_window_set_user_data (p->event_window, widget);
    gdk_window_set_cursor (p->event_window, p->arrow_cursor);

    gtk_widget_queue_draw(widget);
}


static void phin_slider_unrealize (GtkWidget *widget)
{
    PhinSliderPrivate* p = PHIN_SLIDER_GET_PRIVATE (widget);;
    GtkWidgetClass* klass = GTK_WIDGET_CLASS(phin_slider_parent_class);

    debug ("unrealize\n");

    gdk_cursor_unref (p->arrow_cursor);
    p->arrow_cursor = NULL;

    gdk_cursor_unref (p->empty_cursor);
    p->empty_cursor = NULL;

    gdk_window_set_user_data (p->event_window, NULL);
    gdk_window_destroy (p->event_window);
    p->event_window = NULL;

    if (klass->unrealize)
        klass->unrealize (widget);
}


static void phin_slider_map (GtkWidget *widget)
{
    PhinSliderPrivate* p = PHIN_SLIDER_GET_PRIVATE (widget);

    debug ("map\n");

    g_return_if_fail (PHIN_IS_SLIDER (widget));

    gdk_window_show (p->event_window);

    GTK_WIDGET_CLASS(phin_slider_parent_class)->map(widget);
}


static void phin_slider_unmap (GtkWidget *widget)
{
    PhinSliderPrivate* p = PHIN_SLIDER_GET_PRIVATE(widget);;

    debug ("unmap\n");

    g_return_if_fail (PHIN_IS_SLIDER (widget));

    gdk_window_hide (p->event_window);

    GTK_WIDGET_CLASS(phin_slider_parent_class)->unmap(widget);
}

static void phin_slider_size_request (GtkWidget*      widget,
                                          GtkRequisition* requisition)
{
    PhinSliderPrivate* p = PHIN_SLIDER_GET_PRIVATE (widget);;
    int focus_width, focus_pad;
    int pad;

    debug ("size request\n");

    g_return_if_fail (PHIN_IS_SLIDER (widget));

    gtk_widget_style_get (widget,
                          "focus-line-width", &focus_width,
                          "focus-padding", &focus_pad,
                          NULL);

    pad = 2 * (focus_width + focus_pad + SLIDER_DECO);

    if (p->orientation == GTK_ORIENTATION_VERTICAL)
    {
        requisition->width = SLIDER_WIDTH + pad;
        requisition->height = SLIDER_LENGTH + pad;
    }
    else
    {
        requisition->width = SLIDER_LENGTH + pad;
        requisition->height = SLIDER_WIDTH + pad;
    }

    debug("slider width:%d height:%d\n",   requisition->width,
                                            requisition->height);
}

static void phin_slider_size_allocate (GtkWidget*     widget,
                                           GtkAllocation* allocation)
{
    PhinSlider* slider = PHIN_SLIDER (widget);
    PhinSliderPrivate* p = PHIN_SLIDER_GET_PRIVATE (widget);;
    int x, y, w, h;

    debug ("size allocate\n");

    g_return_if_fail (widget != NULL);
    g_return_if_fail (PHIN_IS_SLIDER (widget));
    g_return_if_fail (allocation != NULL);

    gtk_widget_set_allocation(widget, allocation);

    phin_slider_calc_layout (slider, &x, &y, &w, &h);

    if (gtk_widget_get_realized(widget))
    {
        gdk_window_move_resize (p->event_window, x, y, w, h);
    }
}


static void draw_rectangle(cairo_t* cr, double x, double y,
                                        double w, double h,
                                        GdkColor* col, int edges)
{
    double r, g, b, f, a;

    gdk_col_to_double(col, &r, &g, &b);
    a = (r + g + b) / 3.0f;

    if (edges & EDGE_SHADE)
        f = a * 0.75;
    else if (edges & EDGE_HI)
        f = a * 1.5;
    else
        f = a;

    if (edges & EDGE_OUTER)
    {
        x -= 1;
        y -= 1;
        w += 2;
        h += 2;
    }

    cairo_set_source_rgb(cr, r, g, b);
    cairo_set_line_width(cr, 1.0);
    cairo_rectangle(cr, x, y, w, h);
    cairo_stroke_preserve(cr);
    cairo_fill(cr);

    #ifdef CAIRO_HAS_OPERATOR_HSL
    cairo_set_operator(cr, CAIRO_OPERATOR_HSL_LUMINOSITY);
    cairo_set_source_rgb(cr, f, f, f);
    #else
    cairo_set_source_rgb(cr, r * f, g * f, b * f);
    #endif
    cairo_rectangle(cr, x, y, w, h);
    cairo_stroke(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
}


static gboolean phin_slider_expose (GtkWidget*      widget,
                                        GdkEventExpose* event)
{
    PhinSliderPrivate* p = PHIN_SLIDER_GET_PRIVATE (widget);
    PhinSlider* slider;
    gboolean V = (p->orientation == GTK_ORIENTATION_VERTICAL);

    debug("expose");

    int x, y;
    int w, h;
    int cx, cy;     /* center line if applicable*/
    int fx, fy;     /* "filled" coordinates */
    int fw, fh;
    int edge_gap = 0;

    GtkStyle* style;
    cairo_t* cr;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (PHIN_IS_SLIDER (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    if (event->count > 0)
        return FALSE;

    slider = (PhinSlider*) widget;

    style = gtk_widget_get_style(widget);
    cr = gdk_cairo_create(gtk_widget_get_window(widget));
    cairo_set_line_width(cr, 1.0);

    phin_slider_calc_layout(slider, &x, &y, &w, &h);

    if (V)
    {
        cx = fx = x;
        fw = w;

        if (p->center_val >= 0)
        {
            fh = ABS(p->val - p->center_val) * h;
            fy = y + p->center_val * h;
            cy = fy;
            edge_gap = EDGE_V_WITH_0X;

            if ((p->val > p->center_val && !p->inverted)
             || (p->val < p->center_val &&  p->inverted))
            {
                fy -= fh;
            }
        }
        else
        {
            cy = 0; /* silence warning */
            fh = p->val * h;
            fy = p->inverted ? y : y + h - fh;
        }
    }
    else /* H */
    {
        cy = fy = y;
        fh = h;

        if (p->center_val >= 0)
        {
            fw = ABS(p->val - p->center_val) * w;
            fx = x + p->center_val * w;
            cx = fx;
            edge_gap = EDGE_H_WITH_0X;

            if ((p->val < p->center_val && !p->inverted)
             || (p->val > p->center_val &&  p->inverted))
            {
                fx -= fw;
            }
        }
        else
        {
            cx = 0; /* silence warning */
            fw = p->val * w;
            fx = p->inverted ? x + w - fw : x;
        }
    }

    if (!gtk_widget_is_sensitive(widget))
    {
        draw_rectangle(cr, x + 0.5,     y + 0.5,    w, h,
                                &style->dark[GTK_STATE_INSENSITIVE],
                                EDGE_SHADE | EDGE_OUTER);

        draw_rectangle(cr, fx + 0.5,    fy + 0.5,   fw, fh,
                                &style->fg[GTK_STATE_INSENSITIVE],
                                EDGE_HI | EDGE_INNER | edge_gap );
    }
    else
    {
        draw_rectangle(cr, x + 0.5,     y + 0.5,    w, h,
                                &style->dark[GTK_STATE_NORMAL],
                                EDGE_SHADE | EDGE_OUTER);

        draw_rectangle(cr, fx + 0.5,    fy + 0.5,   fw, fh,
                                &style->base[GTK_STATE_SELECTED],
                                EDGE_HI | EDGE_INNER | edge_gap);

        if (p->center_val >= 0)
        {   /* draw zero/center line */
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_SQUARE);
            cairo_set_line_width(cr, 1.0);
            gdk_cairo_set_source_color(cr, &style->base[GTK_STATE_NORMAL]);
            cairo_move_to(cr, cx + 0.5, cy + 0.5);
            cairo_line_to(cr, cx + 0.5 + (V ? w : 0),
                              cy + 0.5 + (V ? 0 : h));
            cairo_stroke(cr);
        }
    }

    cairo_destroy(cr);

    if (gtk_widget_has_focus (widget))
    {
        int focus_width, focus_pad, pad;

        gtk_widget_style_get (widget, "focus-line-width", &focus_width,
                                      "focus-padding", &focus_pad, NULL);
        pad = focus_width + focus_pad;
        x -= pad;
        y -= pad;
        w += 2 * pad + 1;
        h += 2 * pad + 1;

        gtk_paint_focus (style, gtk_widget_get_window(widget),
                                gtk_widget_get_state (widget),
                                NULL, widget, NULL,
                                x, y, w, h);
    }

    return FALSE;
}


static gboolean phin_slider_button_press (GtkWidget*      widget,
                                              GdkEventButton* event)
{
    PhinSliderPrivate* p = PHIN_SLIDER_GET_PRIVATE (widget);
    PhinSlider* slider;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (PHIN_IS_SLIDER (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    slider = (PhinSlider*) widget;

    if(event->button == 1)
    {
        gtk_widget_grab_focus (widget);
         
        if (p->state == STATE_SCROLL)
        {
            p->state = STATE_NORMAL;
            gdk_window_set_cursor (p->event_window, p->arrow_cursor);
            return FALSE;
        }

        gdk_window_set_cursor (p->event_window, p->empty_cursor);

        p->xclick_root = event->x_root;
        p->xclick = event->x;
        p->yclick_root = event->y_root;
        p->yclick = event->y;
        p->state = STATE_CLICKED;
    }
    else if (event->button == 2 && p->use_default_value)
    {
        phin_slider_set_value(slider, p->default_value);
        return TRUE;
    }

    return FALSE;
}


static gboolean phin_slider_button_release (GtkWidget*      widget,
                                                GdkEventButton* event)
{
    PhinSliderPrivate* p = PHIN_SLIDER_GET_PRIVATE (widget);

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (PHIN_IS_SLIDER (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    gdk_window_set_cursor (p->event_window, p->arrow_cursor);

    if (p->state == STATE_CLICKED)
    {
        p->state = STATE_NORMAL;

        phin_warp_pointer (event->x_root,
                           event->y_root,
                           p->xclick_root,
                           p->yclick_root);
    }

    return FALSE;
}

static gboolean phin_slider_key_press (GtkWidget* widget,
                                           GdkEventKey* event)
{
    PhinSliderPrivate* p = PHIN_SLIDER_GET_PRIVATE (widget);
    GtkAdjustment* adj = p->adjustment_prv;
    gdouble inc;

    debug ("key press\n");

    if (p->orientation == GTK_ORIENTATION_VERTICAL)
    {
        switch (event->keyval)
        {
        case GDK_Up:
            inc = gtk_adjustment_get_step_increment(adj);
            break;
        case GDK_Down:
            inc = -gtk_adjustment_get_step_increment(adj);
            break;
        case GDK_Page_Up:
            inc = gtk_adjustment_get_page_increment(adj);
            break;
        case GDK_Page_Down:
            inc = -gtk_adjustment_get_page_increment(adj);
            break;
        default:
            return FALSE;
        }
    }
    else
    {
        switch (event->keyval)
        {
        case GDK_Right:
            inc = gtk_adjustment_get_step_increment(adj);
            break;
        case GDK_Left:
            inc = -gtk_adjustment_get_step_increment(adj);
            break;
        case GDK_Page_Up:
            inc = gtk_adjustment_get_page_increment(adj);
            break;
        case GDK_Page_Down:
            inc = -gtk_adjustment_get_page_increment(adj);
            break;
        default:
            return FALSE;
        }
    }

    if (p->inverted)
        inc = -inc;

    gtk_adjustment_set_value (adj, gtk_adjustment_get_value(adj) + inc);

    return TRUE;
}

static gboolean phin_slider_scroll (GtkWidget* widget,
                                        GdkEventScroll* event)
{
    PhinSliderPrivate* p = PHIN_SLIDER_GET_PRIVATE (widget);

    gdouble val, pinc;

    gtk_widget_grab_focus (widget);

    p->state = STATE_SCROLL;

    p->xclick_root = event->x_root;
    p->yclick_root = event->y_root;
    p->xclick = event->x;
    p->yclick = event->y;

    /*  doesn't seem intuitive to me that the cursor disappears
        when using the scroll wheel to adjust the slider.
    gdk_window_set_cursor (p->event_window, p->empty_cursor); */

    val = gtk_adjustment_get_value(p->adjustment_prv);
    pinc = gtk_adjustment_get_page_increment(p->adjustment_prv);

    if (((event->direction == GDK_SCROLL_UP
       || event->direction == GDK_SCROLL_RIGHT) && !p->inverted)
     || ((event->direction == GDK_SCROLL_DOWN
       || event->direction == GDK_SCROLL_LEFT) && p->inverted))
    {
        gtk_adjustment_set_value (p->adjustment_prv, val + pinc);
    }
    else
    {
        gtk_adjustment_set_value (p->adjustment_prv, val - pinc);
    }

    return TRUE;
}

static gboolean phin_slider_motion_notify (GtkWidget*      widget,
                                               GdkEventMotion* event)
{
    PhinSliderPrivate* p = PHIN_SLIDER_GET_PRIVATE (widget);
    PhinSlider* slider;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (PHIN_IS_SLIDER (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    slider = (PhinSlider*) widget;

    switch (p->state)
    {
    case STATE_SCROLL:
        if (ABS (event->x - p->xclick) >= THRESHOLD
         || ABS (event->y - p->yclick) >= THRESHOLD)
        {
            gdk_window_set_cursor (p->event_window, p->arrow_cursor);
            p->state = STATE_NORMAL;
        }
    case STATE_NORMAL:
        goto skip;
    }

    if ((event->state & GDK_SHIFT_MASK))
        p->value_update_ratio = VALUE_RATIO_SHIFT;
    else if ((event->state & GDK_CONTROL_MASK))
        p->value_update_ratio = VALUE_RATIO_CTRL;
    else
        p->value_update_ratio = VALUE_RATIO_NORMAL;

    phin_slider_update_value (slider, event->x_root, event->y_root);

    if (p->orientation == GTK_ORIENTATION_VERTICAL)
    {
        int destx = event->x_root;
        int width;

        gdk_window_get_geometry (p->event_window,
                                 NULL, NULL, &width, NULL, NULL);

        if (event->state & GDK_CONTROL_MASK)
        {
            destx = p->xclick_root;
        }

        phin_warp_pointer (event->x_root, event->y_root, destx,
                                                p->yclick_root);
    }
    else
    {
        int desty = event->y_root;
        int height;

        gdk_window_get_geometry (p->event_window,
                                 NULL, NULL, NULL, &height, NULL);

        if (event->state & GDK_CONTROL_MASK)
        {
            desty = p->yclick_root;
        }
               
        phin_warp_pointer (event->x_root, event->y_root, p->xclick_root,
                                                                desty);
    }

    gtk_widget_queue_draw (widget);

skip:
    /* signal that we want more motion events */
    gdk_window_get_pointer (NULL, NULL, NULL, NULL);

    return FALSE;
}


static void phin_slider_calc_layout (PhinSlider* slider,
                                         int* x, int* y,
                                         int* w, int* h)
{
    /*  this routine calculates coordinates for cairo drawing
        and if clean crisp pixel perfect lines are desired (they
        are) then we're forced to add 0.5 to the coordinates.
     */

    PhinSliderPrivate* p = PHIN_SLIDER_GET_PRIVATE (slider);
    GtkWidget* widget = GTK_WIDGET (slider);
    int focus_width, focus_pad;
    int pad;
    GtkAllocation widget_alloc;

    gtk_widget_style_get (widget,
                          "focus-line-width", &focus_width,
                          "focus-padding", &focus_pad,
                          NULL);

    pad = focus_width + focus_pad + SLIDER_DECO;

    gtk_widget_get_allocation(widget, &widget_alloc);

    if (p->orientation == GTK_ORIENTATION_VERTICAL)
    {
        *x = widget_alloc.x + (widget_alloc.width - SLIDER_WIDTH) / 2;
        *y = widget_alloc.y + pad;
        *w = SLIDER_WIDTH;
        *h = widget_alloc.height - 2 * pad;
        *h -= (*h % 2);
    }
    else
    {
        *x = widget_alloc.x + pad;
        *y = widget_alloc.y + (widget_alloc.height - SLIDER_WIDTH) / 2;
        *w = widget_alloc.width - 2 * pad;
        *h = SLIDER_WIDTH;
        *w -= (*w % 2);
    }
}


static void phin_slider_update_value (PhinSlider* slider,
                                          int x_root, int y_root)
{
    PhinSliderPrivate* p = PHIN_SLIDER_GET_PRIVATE (slider);
    double oldval;
    double value;
    double inc;
    GtkAllocation slider_alloc;

    if (p->state != STATE_CLICKED)
        return;

    gtk_widget_get_allocation(GTK_WIDGET(slider), &slider_alloc);

    oldval = p->val;

    if (p->orientation == GTK_ORIENTATION_VERTICAL)
    {
        inc = ((p->yclick_root - y_root)
                * p->value_update_ratio / slider_alloc.height);
    }
    else
    {
        inc = ((x_root - p->xclick_root)
               * p->value_update_ratio / slider_alloc.width);
    }

    if (p->inverted)
        inc = -inc;

    p->val += inc;
    p->val = CLAMP (p->val, 0, 1);

    if (p->val != oldval)
    {
        value =
            (gtk_adjustment_get_lower(p->adjustment_prv) * (1.0 - p->val)
                 + gtk_adjustment_get_upper(p->adjustment_prv) * p->val);

        g_signal_handlers_block_by_func(
                                G_OBJECT(slider),
                                phin_slider_adjustment_value_changed,
                                (gpointer) slider);

        gtk_adjustment_set_value(p->adjustment_prv, value);

        g_signal_emit(G_OBJECT(slider), signals[VALUE_CHANGED_SIGNAL], 0);

        g_signal_handlers_unblock_by_func
                                (G_OBJECT (slider),
                                phin_slider_adjustment_value_changed,
                                (gpointer) slider);
    }
}


static void phin_slider_adjustment_changed (GtkAdjustment* adj,
                                                PhinSlider* slider)
{
    PhinSliderPrivate* p = PHIN_SLIDER_GET_PRIVATE (slider);
    GtkWidget* widget;
    double adj_lower, adj_upper, adj_value;

    g_return_if_fail (PHIN_IS_SLIDER (slider));

    widget = GTK_WIDGET (slider);

    adj_lower = gtk_adjustment_get_lower(adj);
    adj_upper = gtk_adjustment_get_upper(adj);
    adj_value = gtk_adjustment_get_value(adj);

    if (adj_lower < 0 && adj_upper > 0)
    {
        p->center_val = (-adj_lower / (adj_upper - adj_lower));
    }
    else
    {
        p->center_val = -1;
    }

    p->val = ((adj_value - adj_lower)
                   / (adj_upper - adj_lower));

    gtk_widget_queue_draw (GTK_WIDGET (slider));

    if (gtk_widget_get_realized (widget))
        gdk_window_process_updates (gtk_widget_get_window(widget), FALSE);

    g_signal_emit (G_OBJECT (slider), signals[CHANGED_SIGNAL], 0);
}

static void phin_slider_adjustment_value_changed (GtkAdjustment* adj,
                                                      PhinSlider* slider)
{
    PhinSliderPrivate* p = PHIN_SLIDER_GET_PRIVATE (slider);
    GtkWidget* widget;
    double adj_lower, adj_upper, adj_value;

    g_return_if_fail (PHIN_IS_SLIDER (slider));

    widget = GTK_WIDGET (slider);
    adj_lower = gtk_adjustment_get_lower(adj);
    adj_upper = gtk_adjustment_get_upper(adj);
    adj_value = gtk_adjustment_get_value(adj);

    p->val = ((adj_value - adj_lower) / (adj_upper - adj_lower));

    gtk_widget_queue_draw (widget);

    if (gtk_widget_get_realized (widget))
        gdk_window_process_updates (gtk_widget_get_window(widget), FALSE);

    g_signal_emit (G_OBJECT (slider), signals[VALUE_CHANGED_SIGNAL], 0);

    if (p->adjustment != NULL)
    {
        phin_slider_get_value(slider);
        /* update value of external adjustment */
    }
}
