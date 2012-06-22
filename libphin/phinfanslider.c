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
#include "phinfanslider.h"

/* magic numbers */
enum
{
    FAN_RISE = 3,
    FAN_RUN = 1,
    SLIDER_WIDTH = 16,
    SLIDER_LENGTH = 32,
    THRESHOLD = 4,
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


typedef struct _PhinFanSliderPrivate PhinFanSliderPrivate;

#define PHIN_FAN_SLIDER_GET_PRIVATE(obj) \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), PHIN_TYPE_FAN_SLIDER, \
                                        PhinFanSliderPrivate))

struct _PhinFanSliderPrivate
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
    int            fan_max_thickness;
    int            state;
    gboolean       inverted;
    int            direction;
    gboolean       is_log;
    GtkOrientation orientation;
    GtkWidget*     fan_window;
    GdkCursor*     arrow_cursor;
    GdkCursor*     empty_cursor;
    GdkWindow*     event_window;
    GtkWidget*     hint_window0;
    GtkWidget*     hint_window1;
    GdkRectangle   cur_fan;
    gboolean       use_default_value;
    gdouble        default_value;
};


/* forward declarations */
G_DEFINE_TYPE(PhinFanSlider, phin_fan_slider, GTK_TYPE_WIDGET);


static gboolean fans_active =       FALSE;
static int      fan_max_height =    0;
static int      fan_max_width  =    0;


static void phin_fan_slider_dispose         (GObject* object);
static void phin_fan_slider_realize         (GtkWidget* widget);
static void phin_fan_slider_unrealize       (GtkWidget *widget);
static void phin_fan_slider_map             (GtkWidget *widget);
static void phin_fan_slider_unmap           (GtkWidget *widget);

static void phin_fan_slider_draw_fan        (PhinFanSlider* slider);
static int  phin_fan_slider_get_fan_length  (PhinFanSlider* slider);
static void phin_fan_slider_update_hints    (PhinFanSlider* slider);

static void phin_fan_slider_size_request    (GtkWidget*widget,
                                            GtkRequisition* requisition);

static void phin_fan_slider_size_allocate   (GtkWidget* widget,
                                            GtkAllocation* allocation);

static gboolean phin_fan_slider_expose      (GtkWidget* widget,
                                            GdkEventExpose* event);

static gboolean phin_fan_slider_button_press(GtkWidget* widget,
                                            GdkEventButton* event);

static gboolean phin_fan_slider_button_release(GtkWidget* widget,
                                            GdkEventButton* event);

static gboolean phin_fan_slider_key_press   (GtkWidget* widget,
                                            GdkEventKey* event);

static gboolean phin_fan_slider_scroll      (GtkWidget* widget,
                                            GdkEventScroll* event);

static gboolean phin_fan_slider_motion_notify(GtkWidget* widget,
                                            GdkEventMotion* event);

static gboolean phin_fan_slider_enter_notify(GtkWidget* widget,
                                            GdkEventCrossing* event);

static gboolean phin_fan_slider_leave_notify(GtkWidget* widget,
                                            GdkEventCrossing* event);

static void phin_fan_slider_calc_layout     (PhinFanSlider* slider,
                                            int* x, int* y, int* w, int* h);

static void phin_fan_slider_update_value    (PhinFanSlider* slider,
                                            int x_root, int y_root);

static void phin_fan_slider_update_fan(PhinFanSlider* slider,
                                            int x, int y);

static gboolean phin_fan_slider_fan_expose (GtkWidget*      widget,
                                            GdkEventExpose* event,
                                            PhinFanSlider*  slider);

static void phin_fan_slider_fan_show(GtkWidget* widget, GtkWidget* slider);

static gboolean phin_fan_slider_hint_expose (GtkWidget* widget,
                                            GdkEventExpose* event,
                                            GtkWidget* slider);

static void phin_fan_slider_adjustment_changed(GtkAdjustment* adjustment,
                                            PhinFanSlider* slider);

static void phin_fan_slider_adjustment_value_changed(   GtkAdjustment*,
                                                        PhinFanSlider*);


void phin_fan_slider_set_fans_active(gboolean enable_fans)
{
    fans_active = enable_fans;
}

gboolean phin_fan_slider_get_fans_active(void)
{
    return fans_active;
}


/**
 * phin_fan_slider_set_value:
 * @slider: a #PhinFanSlider
 * @value: a new value for the slider
 * 
 * Sets the current value of the slider.  If the value is outside the
 * range of values allowed by @slider, it will be clamped to fit
 * within them.  The slider emits the "value-changed" signal if the
 * value changes.
 *
 */
void phin_fan_slider_set_value (PhinFanSlider* slider, double value)
{
    PhinFanSliderPrivate* p = PHIN_FAN_SLIDER_GET_PRIVATE(slider);

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


void phin_fan_slider_set_log (PhinFanSlider* slider, gboolean is_log)
{
    PhinFanSliderPrivate* p = PHIN_FAN_SLIDER_GET_PRIVATE(slider);
    p->is_log = is_log;
}


gboolean phin_fan_slider_is_log (PhinFanSlider* slider)
{
    PhinFanSliderPrivate* p = PHIN_FAN_SLIDER_GET_PRIVATE(slider);
    return p->is_log;
}


/**
 * phin_fan_slider_get_value:
 * @slider: a #PhinFanSlider
 *
 * Retrieves the current value of the slider.
 *
 * Returns: current value of the slider.
 *
 */
double phin_fan_slider_get_value (PhinFanSlider* slider)
{
    PhinFanSliderPrivate* p = PHIN_FAN_SLIDER_GET_PRIVATE(slider);

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

    /* not sure what the purpose of this set value call is: */
    gtk_adjustment_set_value(p->adjustment, value);

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
 * phin_fan_slider_set_range:
 * @slider: a #PhinFanSlider
 * @lower: lowest allowable value
 * @upper: highest allowable value
 * 
 * Sets the range of allowable values for the slider, and  clamps the slider's
 * current value to be between @lower and @upper.
 *
 */
void phin_fan_slider_set_range (PhinFanSlider* slider,
                                double lower, double upper)
{
    PhinFanSliderPrivate* p = PHIN_FAN_SLIDER_GET_PRIVATE(slider);
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
 * phin_fan_slider_get_range:
 * @slider: a #PhinFanSlider
 * @lower: retrieves lowest allowable value
 * @upper: retrieves highest allowable value
 *
 * Places the range of allowable values for @slider into @lower
 * and @upper.  Either variable may be set to %NULL if you are not
 * interested in its value.
 *
 */
void phin_fan_slider_get_range (PhinFanSlider* slider,
                                double* lower, double* upper)
{
    PhinFanSliderPrivate* p = PHIN_FAN_SLIDER_GET_PRIVATE(slider);

    if (lower)
        *lower = gtk_adjustment_get_lower(p->adjustment);
    if (upper)
        *upper = gtk_adjustment_get_upper(p->adjustment);
}

/**
 * phin_fan_slider_set_adjustment:
 * @slider: a #PhinFanSlider
 * @adjustment: a #GtkAdjustment
 *
 * Sets the adjustment used by @slider.  Every #PhinFanSlider uses an
 * adjustment to store its current value and its range of allowable
 * values.  If @adjustment is %NULL, a new adjustment with a value of
 * zero and a range of [-1.0, 1.0] will be created.
 *
 */
void phin_fan_slider_set_adjustment (PhinFanSlider* slider,
                                     GtkAdjustment* adjustment)
{
    PhinFanSliderPrivate* p = PHIN_FAN_SLIDER_GET_PRIVATE(slider);

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
                                phin_fan_slider_adjustment_changed,
                                (gpointer)slider);

        g_signal_handlers_disconnect_by_func(
                                p->adjustment,
                                phin_fan_slider_adjustment_value_changed,
                                (gpointer)slider);

        g_object_unref (p->adjustment);
    }

    p->adjustment = adjustment;
    g_object_ref (adjustment);
    g_object_ref_sink (GTK_OBJECT (adjustment));

    phin_fan_slider_adjustment_changed(p->adjustment, slider);

    phin_fan_slider_set_value(PHIN_FAN_SLIDER (slider),
                                gtk_adjustment_get_value(adjustment));
}

/**
 * phin_fan_slider_get_adjustment:
 * @slider: a #PhinFanSlider
 *
 * Retrives the current adjustment in use by @slider.
 *
 * Returns: @slider's current #GtkAdjustment
 *
 */
GtkAdjustment* phin_fan_slider_get_adjustment (PhinFanSlider* slider)
{
    PhinFanSliderPrivate* p = PHIN_FAN_SLIDER_GET_PRIVATE(slider);

    /* I can't imagine this ever being true, but just to be
     * "safe" ... */
    if (!p->adjustment)
        phin_fan_slider_set_adjustment (slider, NULL);

    return p->adjustment;
}

/**
 * phin_fan_slider_set_inverted:
 * @slider: a #PhinFanSlider
 * @inverted: %TRUE to invert the fanslider
 *  
 * Sets in which direction the fanslider should draw increasing
 * values.  By default, horizontal fansliders draw low to high from
 * left to right, and vertical fansliders draw from bottom to top.
 * You can reverse this behavior by setting @inverted to %TRUE.
 * 
 */
void phin_fan_slider_set_inverted (PhinFanSlider* slider, gboolean inverted)
{
    PhinFanSliderPrivate* p = PHIN_FAN_SLIDER_GET_PRIVATE(slider);

    p->inverted = inverted;
    gtk_widget_queue_draw (GTK_WIDGET (slider));
}

/**
 * phin_fan_slider_get_inverted:
 * @slider: a #PhinFanSlider
 *
 * Determines whether @slider is inverted or not.
 *
 * Returns: %TRUE if @slider is inverted
 *
 */
gboolean phin_fan_slider_get_inverted (PhinFanSlider* slider)
{
    PhinFanSliderPrivate* p = PHIN_FAN_SLIDER_GET_PRIVATE(slider);
    return p->inverted;
}

/**
 * phin_fan_slider_set_default_value:
 * @slider: a #PhinFanSlider
 * @value: the default value
 *  
 * Set default value of the slider. Slider is reset to this value
 * when middle mouse button is pressed.
 */
void phin_fan_slider_set_default_value(PhinFanSlider* slider, gdouble value)
{
    PhinFanSliderPrivate* p = PHIN_FAN_SLIDER_GET_PRIVATE(slider);

    p->use_default_value = TRUE;
    p->default_value = value;
}

void phin_fan_slider_set_orientation(PhinFanSlider* slider,
                                     GtkOrientation o)
{
    PhinFanSliderPrivate* p = PHIN_FAN_SLIDER_GET_PRIVATE(slider);
    p->orientation = o;
}


static void phin_fan_slider_class_init (PhinFanSliderClass* klass)
{
    GObjectClass* object_class = G_OBJECT_CLASS(klass);
    GtkWidgetClass* widget_class = GTK_WIDGET_CLASS(klass);
    phin_fan_slider_parent_class = g_type_class_peek_parent(klass);
    GdkScreen*      screen       = gdk_screen_get_default();

    debug ("class init\n");

    phin_fan_slider_parent_class = g_type_class_peek(gtk_widget_get_type());

    object_class->dispose =                 phin_fan_slider_dispose;
    widget_class->realize =                 phin_fan_slider_realize;
    widget_class->unrealize =               phin_fan_slider_unrealize;
    widget_class->map =                     phin_fan_slider_map;
    widget_class->unmap =                   phin_fan_slider_unmap;
    widget_class->expose_event =            phin_fan_slider_expose;
    widget_class->size_request =            phin_fan_slider_size_request;
    widget_class->size_allocate =           phin_fan_slider_size_allocate;
    widget_class->button_press_event =      phin_fan_slider_button_press;
    widget_class->button_release_event =    phin_fan_slider_button_release;
    widget_class->key_press_event =         phin_fan_slider_key_press;
    widget_class->scroll_event =            phin_fan_slider_scroll;
    widget_class->motion_notify_event =     phin_fan_slider_motion_notify;
    widget_class->enter_notify_event =      phin_fan_slider_enter_notify;
    widget_class->leave_notify_event =      phin_fan_slider_leave_notify;

    /**
     * PhinFanSlider::value-changed:
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
                      G_STRUCT_OFFSET(PhinFanSliderClass, value_changed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

    /**
     * PhinFanSlider::changed:
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
                      G_STRUCT_OFFSET (PhinFanSliderClass, changed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

    klass->value_changed = NULL;
    klass->changed = NULL;

    if (screen)
        fan_max_width = gdk_screen_get_width (screen);
    else
        fan_max_width = 1280;

    if (screen)
        fan_max_height = gdk_screen_get_height (screen);
    else
        fan_max_height = 1024;


    g_type_class_add_private(object_class, sizeof(PhinFanSliderPrivate));
}


static void phin_fan_slider_init (PhinFanSlider* slider)
{
    PhinFanSliderPrivate* p = PHIN_FAN_SLIDER_GET_PRIVATE(slider);

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
    p->fan_max_thickness = 1;
    p->inverted = FALSE;
    p->direction = 0;
    p->state = STATE_NORMAL;
    p->orientation = GTK_ORIENTATION_HORIZONTAL;
    p->fan_window = NULL;
    p->arrow_cursor = NULL;
    p->empty_cursor = NULL;
    p->event_window = NULL;
    p->hint_window0 = NULL;
    p->hint_window1 = NULL;
    p->is_log = 0;
    p->use_default_value = FALSE;

    g_signal_connect (p->adjustment_prv, "changed",
                      G_CALLBACK (phin_fan_slider_adjustment_changed),
                      (gpointer) slider);
    g_signal_connect (p->adjustment_prv, "value_changed",
                      G_CALLBACK (phin_fan_slider_adjustment_value_changed),
                      (gpointer) slider);

    phin_fan_slider_adjustment_changed (p->adjustment_prv, slider);
    phin_fan_slider_adjustment_value_changed (p->adjustment_prv, slider);

}


static void phin_fan_slider_dispose (GObject* object)
{
    PhinFanSlider* slider;
    PhinFanSliderPrivate* p;

    g_return_if_fail (object != NULL);
    g_return_if_fail (PHIN_IS_FAN_SLIDER (object));

    slider = PHIN_FAN_SLIDER(object);
    p = PHIN_FAN_SLIDER_GET_PRIVATE(object);

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

    if (p->fan_window != NULL)
    {
        gtk_widget_destroy (p->fan_window);
        p->fan_window = NULL;
    }

    if (p->hint_window0 != NULL)
    {
        gtk_widget_destroy (p->hint_window0);
        p->hint_window0 = NULL;
    }

    if (p->hint_window1 != NULL)
    {
        gtk_widget_destroy (p->hint_window1);
        p->hint_window1 = NULL;
    }

    if (p->adjustment)
    {
        g_signal_handlers_disconnect_by_func(
                                p->adjustment,
                                phin_fan_slider_adjustment_changed,
                                (gpointer)slider);

        g_signal_handlers_disconnect_by_func(
                                p->adjustment,
                                phin_fan_slider_adjustment_value_changed,
                                (gpointer)slider);

        /* called ref on it so we must call unref */
        g_object_unref (p->adjustment);
        p->adjustment = NULL;
    }

    if (p->adjustment_prv)
    {
        g_signal_handlers_disconnect_by_func(
                                p->adjustment_prv,
                                phin_fan_slider_adjustment_changed,
                                (gpointer)slider);

        g_signal_handlers_disconnect_by_func(
                                p->adjustment_prv,
                                phin_fan_slider_adjustment_value_changed,
                                (gpointer)slider);
    }
}


static void phin_fan_slider_realize (GtkWidget* widget)
{
    PhinFanSlider* slider;
    PhinFanSliderPrivate* p;
    GdkWindowAttr attributes;
    gint attributes_mask;
    GdkWindow* window;
    GtkStyle* style;
    debug ("realize\n");

    g_return_if_fail (widget != NULL);
    g_return_if_fail (PHIN_IS_FAN_SLIDER (widget));

    gtk_widget_set_realized(GTK_WIDGET(widget), TRUE);

    slider = PHIN_FAN_SLIDER(widget);
    p = PHIN_FAN_SLIDER_GET_PRIVATE(widget);

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

    phin_fan_slider_calc_layout(slider,
                                 &attributes.x,
                                 &attributes.y,
                                 &attributes.width,
                                 &attributes.height);

    attributes_mask = GDK_WA_X | GDK_WA_Y;

    p->event_window = gdk_window_new (gtk_widget_get_parent_window (widget),
                                           &attributes, attributes_mask);
    gdk_window_set_user_data (p->event_window, widget);
    gdk_window_set_cursor (p->event_window, p->arrow_cursor);

    p->fan_window = gtk_window_new (GTK_WINDOW_POPUP);

    gtk_window_resize(GTK_WINDOW(p->fan_window),
                                fan_max_width, fan_max_height);

    gtk_widget_set_app_paintable (p->fan_window, TRUE);

    g_signal_connect (G_OBJECT (p->fan_window),
                      "expose-event",
                      G_CALLBACK (phin_fan_slider_fan_expose),
                      (gpointer) slider);

    g_signal_connect (G_OBJECT (p->fan_window),
                      "show",
                      G_CALLBACK (phin_fan_slider_fan_show),
                      (gpointer) slider);

    g_signal_connect (G_OBJECT(p->fan_window),
                      "screen-changed",
                      G_CALLBACK (phin_screen_changed), NULL);

    phin_screen_changed(p->fan_window, NULL, NULL);

    p->hint_window0 = gtk_window_new (GTK_WINDOW_POPUP);
    gtk_widget_realize (p->hint_window0);
    g_signal_connect (G_OBJECT (p->hint_window0),
                      "expose-event",
                      G_CALLBACK (phin_fan_slider_hint_expose),
                      (gpointer) slider);

    p->hint_window1 = gtk_window_new (GTK_WINDOW_POPUP);
    gtk_widget_realize (p->hint_window1);
    g_signal_connect (G_OBJECT (p->hint_window1),
                      "expose-event",
                      G_CALLBACK (phin_fan_slider_hint_expose),
                      (gpointer) slider);

    /* a priming call */
    phin_fan_slider_update_hints (slider);

    gtk_widget_queue_draw(widget);
}

static void phin_fan_slider_unrealize (GtkWidget *widget)
{
    PhinFanSliderPrivate* p = PHIN_FAN_SLIDER_GET_PRIVATE (widget);;
    GtkWidgetClass* klass = GTK_WIDGET_CLASS(phin_fan_slider_parent_class);

    debug ("unrealize\n");

    gdk_cursor_unref (p->arrow_cursor);
    p->arrow_cursor = NULL;

    gdk_cursor_unref (p->empty_cursor);
    p->empty_cursor = NULL;

    gdk_window_set_user_data (p->event_window, NULL);
    gdk_window_destroy (p->event_window);
    p->event_window = NULL;

    gtk_widget_destroy (p->fan_window);
    p->fan_window = NULL;

    gtk_widget_destroy (p->hint_window0);
    p->hint_window0 = NULL;

    gtk_widget_destroy (p->hint_window1);
    p->hint_window1 = NULL;

    if (klass->unrealize)
        klass->unrealize (widget);
}


static void phin_fan_slider_map (GtkWidget *widget)
{
    PhinFanSliderPrivate* p = PHIN_FAN_SLIDER_GET_PRIVATE (widget);

    debug ("map\n");

    g_return_if_fail (PHIN_IS_FAN_SLIDER (widget));

    gdk_window_show (p->event_window);

    GTK_WIDGET_CLASS(phin_fan_slider_parent_class)->map(widget);
}


static void phin_fan_slider_unmap (GtkWidget *widget)
{
    PhinFanSliderPrivate* p = PHIN_FAN_SLIDER_GET_PRIVATE(widget);;

    debug ("unmap\n");

    g_return_if_fail (PHIN_IS_FAN_SLIDER (widget));

    gdk_window_hide (p->event_window);

    GTK_WIDGET_CLASS(phin_fan_slider_parent_class)->unmap(widget);
}

static void phin_fan_slider_size_request (GtkWidget*      widget,
                                          GtkRequisition* requisition)
{
    PhinFanSliderPrivate* p = PHIN_FAN_SLIDER_GET_PRIVATE (widget);;
    int focus_width, focus_pad;
    int pad;

    debug ("size request\n");

    g_return_if_fail (PHIN_IS_FAN_SLIDER (widget));

    gtk_widget_style_get (widget,
                          "focus-line-width", &focus_width,
                          "focus-padding", &focus_pad,
                          NULL);

    pad = 2 * (focus_width + focus_pad);

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
}

static void phin_fan_slider_size_allocate (GtkWidget*     widget,
                                           GtkAllocation* allocation)
{
    PhinFanSlider* slider = PHIN_FAN_SLIDER (widget);
    PhinFanSliderPrivate* p = PHIN_FAN_SLIDER_GET_PRIVATE (widget);;
    int x, y, w, h;

    debug ("size allocate\n");

    g_return_if_fail (widget != NULL);
    g_return_if_fail (PHIN_IS_FAN_SLIDER (widget));
    g_return_if_fail (allocation != NULL);

    gtk_widget_set_allocation(widget, allocation);

    phin_fan_slider_calc_layout (slider, &x, &y, &w, &h);

    if (p->orientation == GTK_ORIENTATION_VERTICAL)
    {
        p->fan_max_thickness = ((fan_max_height - h)
                                     / (2 * FAN_RISE / FAN_RUN));
    }
    else
    {
        p->fan_max_thickness = ((fan_max_width - w)
                                     / (2 * FAN_RISE / FAN_RUN));
    }

    if (gtk_widget_get_realized(widget))
    {
        gdk_window_move_resize (p->event_window, x, y, w, h);
    }
}


static void draw_fan_rectangle(cairo_t* cr, double x, double y,
                                            double w, double h,
                                            GdkColor* col)
{
    gdk_cairo_set_source_color(cr, col);
    cairo_set_line_width(cr, 1.0);
    cairo_rectangle(cr, x, y, w, h);
    cairo_stroke_preserve(cr);
    cairo_fill(cr);
}


static gboolean phin_fan_slider_expose (GtkWidget*      widget,
                                        GdkEventExpose* event)
{
    PhinFanSliderPrivate* p = PHIN_FAN_SLIDER_GET_PRIVATE (widget);
    PhinFanSlider* slider;
    gboolean V = (p->orientation == GTK_ORIENTATION_VERTICAL);
debug("expose");
    int x, y;
    int w, h;
    int cx, cy;     /* center line if applicable*/
    int vx, vy;     /* value line */
    int fx, fy;     /* "filled" coordinates */
    int fw, fh;

    GtkStyle* style;
    cairo_t* cr;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (PHIN_IS_FAN_SLIDER (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    if (event->count > 0)
        return FALSE;

    slider = (PhinFanSlider*) widget;

    style = gtk_widget_get_style(widget);
    cr = gdk_cairo_create(gtk_widget_get_window(widget));
    cairo_set_line_width(cr, 1.0);

    phin_fan_slider_calc_layout(slider, &x, &y, &w, &h);


    /* decrement probably due to offset x and y each by 0.5 to gain
       hard edged lines. */
    --w;
    --h;

    if (V)
    {
        vy = y + h - p->val * h;
        vx = cx = fx = x;
        fw = w;

        if (p->center_val >= 0)
        {
            fh = ABS(p->val - p->center_val) * h;
            fy = y + h - p->center_val * h;
            cy = fy;

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
        vx = x + p->val * w;
        vy = cy = fy = y;
        fh = h;

        if (p->center_val >= 0)
        {
            fw = ABS(p->val - p->center_val) * w;
            fx = x + p->center_val * w;

            cx = fx;

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
        draw_fan_rectangle(cr, x + 0.5,     y + 0.5,    w, h,
                                &style->dark[GTK_STATE_INSENSITIVE]);
        draw_fan_rectangle(cr, fx + 0.5,    fy + 0.5,   fw, fh,
                                &style->fg[GTK_STATE_INSENSITIVE]);
    }
    else
    {
        draw_fan_rectangle(cr, x + 0.5,     y + 0.5,    w, h,
                                &style->dark[GTK_STATE_NORMAL]);
        draw_fan_rectangle(cr, fx + 0.5,    fy + 0.5,   fw, fh,
                                &style->base[GTK_STATE_SELECTED]);

        if (p->center_val >= 0)
        {   /* draw zero/center line */
            gdk_cairo_set_source_color(cr, &style->base[GTK_STATE_NORMAL]);
            cairo_move_to(cr, cx + 0.5, cy + 0.5);
            cairo_line_to(cr, cx + 0.5 + (V ? w : 0),
                              cy + 0.5 + (V ? 0 : h));
            cairo_stroke(cr);
        }

        gdk_cairo_set_source_color(cr, &style->fg[GTK_STATE_NORMAL]);
        cairo_move_to(cr, vx + 0.5, vy + 0.5);
        cairo_line_to(cr, vx + 0.5 + (V ? w : 0), vy + 0.5 + (V ? 0 : h));
        cairo_stroke(cr);
    }

    cairo_destroy(cr);
    gtk_paint_shadow(style, gtk_widget_get_window(widget),
                            gtk_widget_get_state(widget),
                            GTK_SHADOW_IN,  NULL, widget, NULL,
                            x - 1, y - 1, w + 3, h + 3);

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

    if (gtk_widget_get_visible (p->fan_window))
        gtk_widget_queue_draw (p->fan_window);
     
    return FALSE;
}


static gboolean phin_fan_slider_button_press (GtkWidget*      widget,
                                              GdkEventButton* event)
{
    PhinFanSliderPrivate* p = PHIN_FAN_SLIDER_GET_PRIVATE (widget);
    PhinFanSlider* slider;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (PHIN_IS_FAN_SLIDER (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    slider = (PhinFanSlider*) widget;

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
        phin_fan_slider_set_value(slider, p->default_value);
        return TRUE;
    }

    return FALSE;
}


static gboolean phin_fan_slider_button_release (GtkWidget*      widget,
                                                GdkEventButton* event)
{
    PhinFanSliderPrivate* p = PHIN_FAN_SLIDER_GET_PRIVATE (widget);

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (PHIN_IS_FAN_SLIDER (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    gdk_window_set_cursor (p->event_window, p->arrow_cursor);

    if (p->state == STATE_CLICKED)
    {
        p->state = STATE_NORMAL;

        phin_warp_pointer (event->x_root,
                           event->y_root,
                           p->xclick_root,
                           p->yclick_root);

        if (gtk_widget_get_visible (p->fan_window))
            gtk_widget_hide (p->fan_window);

        if (gtk_widget_get_visible (p->hint_window0))
            gtk_widget_hide (p->hint_window0);

        if (gtk_widget_get_visible (p->hint_window1))
            gtk_widget_hide (p->hint_window1);
    }
     
    return FALSE;
}

static gboolean phin_fan_slider_key_press (GtkWidget* widget,
                                           GdkEventKey* event)
{
    PhinFanSliderPrivate* p = PHIN_FAN_SLIDER_GET_PRIVATE (widget);
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

static gboolean phin_fan_slider_scroll (GtkWidget* widget,
                                        GdkEventScroll* event)
{
    PhinFanSliderPrivate* p = PHIN_FAN_SLIDER_GET_PRIVATE (widget);

    gdouble val, pinc;

    gtk_widget_grab_focus (widget);

    p->state = STATE_SCROLL;

    p->xclick_root = event->x_root;
    p->yclick_root = event->y_root;
    p->xclick = event->x;
    p->yclick = event->y;

    gdk_window_set_cursor (p->event_window, p->empty_cursor);

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

/* ctrl locks precision, xshiftxlocksxvaluex shift increases precision */
static gboolean phin_fan_slider_motion_notify (GtkWidget*      widget,
                                               GdkEventMotion* event)
{
    PhinFanSliderPrivate* p = PHIN_FAN_SLIDER_GET_PRIVATE (widget);
    PhinFanSlider* slider;

    GtkAllocation fan_alloc;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (PHIN_IS_FAN_SLIDER (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    slider = (PhinFanSlider*) widget;
    gtk_widget_get_allocation(p->fan_window, &fan_alloc);

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

    if (fans_active && !(event->state & GDK_CONTROL_MASK))
        phin_fan_slider_update_fan (slider, event->x, event->y);

/*
    if (!(event->state & GDK_SHIFT_MASK))
        phin_fan_slider_update_value (slider, event->x_root, event->y_root);
*/


    /* shift no longer locks value */
    if ((event->state & GDK_SHIFT_MASK))
        p->value_update_ratio = VALUE_RATIO_SHIFT;
    else if ((event->state & GDK_CONTROL_MASK))
        p->value_update_ratio = VALUE_RATIO_CTRL;
    else
        p->value_update_ratio = VALUE_RATIO_NORMAL;

    phin_fan_slider_update_value (slider, event->x_root, event->y_root);

    if (p->orientation == GTK_ORIENTATION_VERTICAL)
    {
        int destx = event->x_root;
        int width;

        gdk_window_get_geometry (p->event_window,
                                 NULL, NULL, &width, NULL, NULL);

        if (gtk_widget_get_visible (p->fan_window))
        {
            if (event->x_root > p->xclick_root)
            {
                if (event->state & GDK_CONTROL_MASK)
                {
                    destx = (p->xclick_root
                             + width
                             - p->xclick
                             + fan_alloc.width);
                }
                else
                {
                    destx = MIN (event->x_root,
                                 p->xclick_root
                                 + width
                                 - p->xclick
                                 + fan_alloc.width);
                }
            }
            else
            {
                if (event->state & GDK_CONTROL_MASK)
                {
                    destx = (p->xclick_root
                             - p->xclick
                             - fan_alloc.width);
                }
                else
                {
                    destx = MAX (event->x_root,
                                 p->xclick_root
                                 - p->xclick
                                 - fan_alloc.width);
                }
            }
        }
        else if (event->state & GDK_CONTROL_MASK)
        {
            destx = p->xclick_root;
        }

        phin_warp_pointer (event->x_root,
                           event->y_root,
                           destx,
                           p->yclick_root);
    }
    else
    {
        int desty = event->y_root;
        int height;

        gdk_window_get_geometry (p->event_window,
                                 NULL, NULL, NULL, &height, NULL);

        if (gtk_widget_get_visible (p->fan_window))
        {
            if (event->y_root > p->yclick_root)
            {
                if (event->state & GDK_CONTROL_MASK)
                {
                    desty = (p->yclick_root
                             + height
                             - p->yclick
                             + fan_alloc.height);
                }
                else
                {
                    desty = MIN (event->y_root,
                                 p->yclick_root
                                 + height
                                 - p->yclick
                                 + fan_alloc.height);
                }
            }
            else
            {
                if (event->state & GDK_CONTROL_MASK)
                {
                    desty = (p->yclick_root
                             - p->yclick
                             - fan_alloc.height);
                }
                else
                {
                    desty = MAX (event->y_root,
                                 p->yclick_root
                                 - p->yclick
                                 - fan_alloc.height);
                }
            }
        }
        else if (event->state & GDK_CONTROL_MASK)
        {
            desty = p->yclick_root;
        }
               
        phin_warp_pointer (event->x_root,
                           event->y_root,
                           p->xclick_root,
                           desty);
    }

    gtk_widget_queue_draw (widget);

    /* necessary in case update_fan() doesn't get called */
    if (gtk_widget_get_visible(p->fan_window))
        gtk_widget_queue_draw (p->fan_window);

skip:
    /* signal that we want more motion events */
    gdk_window_get_pointer (NULL, NULL, NULL, NULL);

    return FALSE;
}


static gboolean phin_fan_slider_enter_notify (GtkWidget* widget,
                                              GdkEventCrossing* event)
{
    PhinFanSliderPrivate* p = PHIN_FAN_SLIDER_GET_PRIVATE (widget);
    GtkAllocation win0_alloc;
    GtkAllocation win1_alloc;

    int width;
    int height;
    PhinFanSlider* slider = PHIN_FAN_SLIDER (widget);

    if (p->state == STATE_NORMAL)
        gdk_window_set_cursor (p->event_window, p->arrow_cursor);

    phin_fan_slider_update_hints (slider);

    gdk_window_get_geometry (p->event_window,
                            NULL, NULL, &width, &height, NULL);

    gtk_widget_get_allocation(p->hint_window0, &win0_alloc);
    gtk_widget_get_allocation(p->hint_window1, &win1_alloc);

    if (p->orientation == GTK_ORIENTATION_VERTICAL)
    {
        gtk_window_move (GTK_WINDOW (p->hint_window0),
                        event->x_root - event->x - win0_alloc.width,
                        (event->y_root - event->y)
                        + (height - win0_alloc.height) / 2);

        gtk_window_move (GTK_WINDOW (p->hint_window1),
                        event->x_root - event->x + width,
                        (event->y_root - event->y)
                        + (height - win1_alloc.height) / 2);
    }
    else
    {
        gtk_window_move (GTK_WINDOW (p->hint_window0),
                        (event->x_root - event->x)
                        + (width - win0_alloc.width) / 2,
                        event->y_root - event->y - win0_alloc.height);

        gtk_window_move (GTK_WINDOW (p->hint_window1),
                        (event->x_root - event->x)
                        + (width - win1_alloc.width) / 2,
                        event->y_root - event->y + height);
    }

    return FALSE;
}

static gboolean phin_fan_slider_leave_notify (GtkWidget* widget,
                                              GdkEventCrossing* event)
{
    (void)event;
    PhinFanSliderPrivate* p = PHIN_FAN_SLIDER_GET_PRIVATE (widget);

    if (p->state == STATE_SCROLL)
    {
        gdk_window_set_cursor (p->event_window, NULL);
        p->state = STATE_NORMAL;
    }

    if (gtk_widget_get_visible (p->hint_window0))
        gtk_widget_hide (p->hint_window0);

    if (gtk_widget_get_visible (p->hint_window1))
        gtk_widget_hide (p->hint_window1);

    return FALSE;
}


/* helper to reduce copy & paste */
static void
fan_slider_draw(cairo_t* cr, GdkPixmap* bitmap, PhinFanSlider* slider,
                             double x, double y, double w, double h,
                             double length, gdouble value)
{
    PhinFanSliderPrivate* p = PHIN_FAN_SLIDER_GET_PRIVATE (slider);
    int offset;

    GtkWidget*  widget = GTK_WIDGET(slider);
    GtkStyle*   style = (bitmap) ? NULL : gtk_widget_get_style(widget);

    double r1, g1, b1, a1; /* slider bg */
    double r2, g2, b2, a2; /* slider fg */

    /*  length: (orientation dependent, orientation as slider length)
            length = fan length, so dependant on size of fan */

    /*  p->center_val: dependant on range of slider and if zero is
        within the range.
            == ratio in range 0.0 to 1.0 of where zero in the value range
            appears within the slider
                or
            == -1 if zero is not within the value range
    */

    if (bitmap)
    {
        r1 = g1 = b1 = a1 = 0.0;
        r2 = g2 = b2 = a2 = 1.0;
    }
    else
    {
        gdk_col_to_double(&style->dark[GTK_STATE_NORMAL], &r1, &g1, &b1);
        gdk_col_to_double(&style->base[GTK_STATE_SELECTED], &r2, &g2, &b2);
        a1 = 0.35;
        a2 = 0.75;
    }

    cairo_set_source_rgba (cr, 0, 0, 0, 0);
    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint (cr);

    cairo_set_line_width(cr, 1.0);

    if (p->orientation == GTK_ORIENTATION_VERTICAL)
    {
        int sign_cur_fan_w;

        float   cv = (p->center_val >= 0.0f)
                        ? p->center_val
                        : 0.0f;

        int fan_top_y = y - length / 2 + h / 2;
        int fan_val_y = length * value;
        int fan_cv_y =  length * cv;

        if (p->direction)
        {
            sign_cur_fan_w = 1 * p->cur_fan.width;
            offset = w;
        }
        else
        { 
            sign_cur_fan_w = -1 * p->cur_fan.width;
            offset = 0;
        }

        /* fan background */
        cairo_move_to(      cr, x + offset,     y);
        /* bottom from slider */
        cairo_line_to (     cr, x + offset + sign_cur_fan_w,
                                fan_top_y);
        /* vertical fan line */
        cairo_rel_line_to ( cr, 0,              length);
        /* top line back to slider */
        cairo_line_to(      cr, x + offset,     y + h);

        if (supports_alpha)
            cairo_set_source_rgba( cr, r1, g1, b1, a1);
        else
            cairo_set_source_rgb( cr, r1, g1, b1);

        cairo_fill( cr);

        if (bitmap)
        {
            /*  a bitmap is used as a mask so that when the fan slider is
                drawn on a non-composited display it does not wipe out the
                rest of the display for the duration of its showing.
                at least, that was the idea, unfortunately work it
                does not... */
            return;
        }

        /*  fan value foreground...
            starting at slider */
        cairo_move_to ( cr, x + offset, y + h - cv * h);
        /* center line from slider */
        cairo_line_to(  cr, x + offset + sign_cur_fan_w,
                            fan_top_y + length - fan_cv_y);
        /* vertical line from center value to value */
        cairo_rel_line_to(  cr, 0,  (fan_cv_y - fan_val_y));
        /* value line back to slider */
        cairo_line_to(  cr, x + offset, y + h - h * value);

        if (supports_alpha)
            cairo_set_source_rgba( cr, r2, g2, b2, a2);
        else
            cairo_set_source_rgb( cr, r2, g2, b2);

        cairo_fill(cr);
    }
    else /* (p->orientation == GTK_ORIENTATION_HORIZONTAL) */
    {
        int   sign_cur_fan_h;
        float cv = (p->center_val >= 0.0f) ? p->center_val : 0.0f;

        int fan_left_x =    x - length / 2 + w / 2;
        int fan_val_x =     length * value;
        int fan_cv_x =      length * cv;

        if (p->direction)
        {
            sign_cur_fan_h = 1 * p->cur_fan.height;
            offset = h;
        }
        else
        {
            sign_cur_fan_h = -1 * p->cur_fan.height;
            offset = 0;
        }

        /* fan background */
        cairo_move_to (     cr, x,          y + offset);
        /* left-hand line from slider */
        cairo_line_to (     cr, fan_left_x, y + offset + sign_cur_fan_h);
        /* horizontal fan line */
        cairo_rel_line_to ( cr, length,     0);
        /* right-hand line back to slider */
        cairo_line_to(      cr, x + w,      y + offset);

        if (supports_alpha)
            cairo_set_source_rgba(cr, r1, g1, b1, a1);
        else
            cairo_set_source_rgb( cr, r1, g1, b1);

        cairo_fill(cr);

        if (bitmap)
        {
            /*  a bitmap is used as a mask so that when the fan slider is
                drawn on a non-composited display it does not wipe out the
                rest of the display for the duration of its showing */
            return;
        }

        /*  fan value foreground...
            starting at slider */
        cairo_move_to ( cr, x + cv * w,     y + offset);
        /* left-hand line from slider */
        cairo_line_to ( cr, fan_left_x + fan_cv_x,
                            y + offset + sign_cur_fan_h);
        /* horizontal line from center value to value */
        cairo_rel_line_to ( cr, fan_val_x - fan_cv_x,   0);
        /* value line back to slider */
        cairo_line_to (cr,  x + w * value,  y + offset);

        if (supports_alpha)
            cairo_set_source_rgba(cr, r2, g2, b2, a2);
        else
            cairo_set_source_rgb( cr, r2, g2, b2);

        cairo_fill(cr);

        if (p->center_val >= 0)
        {
            if (supports_alpha)
                cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, a2);
            else
                cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
            cairo_move_to(cr, x + cv * w, y + offset);
            cairo_line_to(cr, fan_left_x + fan_cv_x,
                              y + offset + sign_cur_fan_h);
            cairo_stroke(cr);
        }

        cairo_set_line_width(cr, 0.5);
        if (supports_alpha)
            cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, a2);
        else
            cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
        cairo_move_to(cr, x + w * value, y + offset);
        cairo_line_to(cr,   fan_left_x + fan_val_x,
                            y + offset + sign_cur_fan_h);
        cairo_stroke(cr);
    }
}


static void phin_fan_slider_draw_fan (PhinFanSlider* slider)
{
    PhinFanSliderPrivate* p = PHIN_FAN_SLIDER_GET_PRIVATE (slider);
    GtkWidget* widget = GTK_WIDGET (slider);

    int     x, y, w, h;
    int     length;
    double  value;
    cairo_t* cr;

    if (!gtk_widget_is_drawable (p->fan_window))
        return;

    phin_fan_slider_calc_layout(slider, &x, &y, &w, &h);

    {
        int root_x, root_y;
        gdk_window_get_origin(gtk_widget_get_window(widget),
                                                    &root_x, &root_y);
        x += root_x;
        y += root_y;
    }

    length = phin_fan_slider_get_fan_length (slider);

    if (p->orientation == GTK_ORIENTATION_VERTICAL)
    {
        if (p->inverted)
            value = 1.0 - gtk_adjustment_get_value(p->adjustment_prv);
        else
            value = gtk_adjustment_get_value(p->adjustment_prv);
    }
    else
    {
        if (p->inverted)
            value = 1.0 - gtk_adjustment_get_value(p->adjustment_prv);
        else
            value = gtk_adjustment_get_value(p->adjustment_prv);
    }

    cr = gdk_cairo_create(gtk_widget_get_window(p->fan_window));

    fan_slider_draw(cr, NULL, slider, x + 0.5, y + 0.5, w, h,
                                                        length, value);

    cairo_destroy(cr);
}


static void phin_fan_slider_calc_layout (PhinFanSlider* slider,
                                         int* x, int* y,
                                         int* w, int* h)
{
    /*  this routine calculates coordinates for cairo drawing
        and if clean crisp pixel perfect lines are desired (they
        are) then we're forced to add 0.5 to the coordinates.
     */

    PhinFanSliderPrivate* p = PHIN_FAN_SLIDER_GET_PRIVATE (slider);
    GtkWidget* widget = GTK_WIDGET (slider);
    int focus_width, focus_pad;
    int pad;
    GtkAllocation widget_alloc;

    gtk_widget_style_get (widget,
                          "focus-line-width", &focus_width,
                          "focus-padding", &focus_pad,
                          NULL);

    pad = focus_width + focus_pad;

    gtk_widget_get_allocation(widget, &widget_alloc);

    if (p->orientation == GTK_ORIENTATION_VERTICAL)
    {
        *x = widget_alloc.x + (widget_alloc.width - SLIDER_WIDTH) / 2;
        *y = widget_alloc.y + pad;
        *w = SLIDER_WIDTH;
        *h = widget_alloc.height - 2 * pad;
    }
    else
    {
        *x = widget_alloc.x + pad;
        *y = widget_alloc.y + (widget_alloc.height - SLIDER_WIDTH) / 2;
        *w = widget_alloc.width - 2 * pad;
        *h = SLIDER_WIDTH;
    }
}


static void phin_fan_slider_update_value (PhinFanSlider* slider,
                                          int x_root, int y_root)
{
    PhinFanSliderPrivate* p = PHIN_FAN_SLIDER_GET_PRIVATE (slider);
    int length;
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
        if (gtk_widget_is_drawable (p->fan_window)
            && (x_root != p->xclick_root))
        {
            length = phin_fan_slider_get_fan_length (slider);
            inc = ((p->yclick_root - y_root)
                   * p->value_update_ratio / length);
        }
        else
        {
            inc = ((p->yclick_root - y_root)
                   * p->value_update_ratio / slider_alloc.height);
        }
    }
    else
    {
        if (gtk_widget_is_drawable (p->fan_window)
            && (y_root != p->yclick_root))
        {
            length = phin_fan_slider_get_fan_length (slider);
            inc = ((x_root - p->xclick_root)
                   * p->value_update_ratio / length);
        }
        else
        {
            inc = ((x_root - p->xclick_root)
                   * p->value_update_ratio / slider_alloc.width);
        }
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
                                phin_fan_slider_adjustment_value_changed,
                                (gpointer) slider);

        gtk_adjustment_set_value(p->adjustment_prv, value);

        g_signal_emit(G_OBJECT(slider), signals[VALUE_CHANGED_SIGNAL], 0);

        g_signal_handlers_unblock_by_func
                                (G_OBJECT (slider),
                                phin_fan_slider_adjustment_value_changed,
                                (gpointer) slider);
    }
}


static void phin_fan_slider_update_fan (PhinFanSlider* slider,
                                        int x, int y)
{
    PhinFanSliderPrivate* p = PHIN_FAN_SLIDER_GET_PRIVATE (slider);
    int width;
    int height;
    int w, h;
    GtkAllocation fan_win_alloc;

    if (p->state != STATE_CLICKED)
        return;

    gdk_window_get_geometry (p->event_window,
                             NULL, NULL, &w, &h, NULL);

    gtk_widget_get_allocation(p->fan_window, &fan_win_alloc);

    if (p->orientation == GTK_ORIENTATION_VERTICAL)
    {
        if (x > w)
        {
            width = x - w;
            width = CLAMP (width, 0, p->fan_max_thickness);
            p->cur_fan.width = width;
            p->cur_fan.height = fan_max_height;
            p->direction = 1;

            if (!gtk_widget_get_visible (p->fan_window))
                gtk_window_present (GTK_WINDOW (p->fan_window));

            if (gtk_widget_get_visible (p->hint_window0))
                gtk_widget_hide (p->hint_window0);

            if (gtk_widget_get_visible (p->hint_window1))
                gtk_widget_hide (p->hint_window1);
        }
        else if (x < 0)
        {
            width = -x;
            width = CLAMP (width, 0, p->fan_max_thickness);
            p->cur_fan.width = width;
            p->cur_fan.height = fan_max_height;
            p->direction = 0;

            if (!gtk_widget_get_visible (p->fan_window))
                gtk_window_present (GTK_WINDOW (p->fan_window));

            if (gtk_widget_get_visible (p->hint_window0))
                gtk_widget_hide (p->hint_window0);

            if (gtk_widget_get_visible (p->hint_window1))
                gtk_widget_hide (p->hint_window1);
        }
        else if (gtk_widget_get_visible (p->fan_window))
        {
            gtk_widget_hide (p->fan_window);
        }
    }
    else
    {
        if (y > h)
        {
            height = y - h;
            height = CLAMP (height, 0, p->fan_max_thickness);
            p->cur_fan.width = fan_max_width;
            p->cur_fan.height = height;
            p->direction = 1;

            if (!gtk_widget_get_visible (p->fan_window))
                gtk_window_present (GTK_WINDOW (p->fan_window));

            if (gtk_widget_get_visible (p->hint_window0))
                gtk_widget_hide (p->hint_window0);

            if (gtk_widget_get_visible (p->hint_window1))
                gtk_widget_hide (p->hint_window1);
        }
        else if (y < 0)
        {
            height = -y;
            height = CLAMP (height, 0, p->fan_max_thickness);
            p->cur_fan.width = fan_max_width;
            p->cur_fan.height = height;
            p->direction = 0;

            if (!gtk_widget_get_visible (p->fan_window))
                gtk_window_present (GTK_WINDOW (p->fan_window));

            if (gtk_widget_get_visible (p->hint_window0))
                gtk_widget_hide (p->hint_window0);

            if (gtk_widget_get_visible (p->hint_window1))
                gtk_widget_hide (p->hint_window1);
        }
        else if (gtk_widget_get_visible (p->fan_window))
        {
            gtk_widget_hide (p->fan_window);
        }
    }
}


static int phin_fan_slider_get_fan_length (PhinFanSlider* slider)
{
    PhinFanSliderPrivate* p = PHIN_FAN_SLIDER_GET_PRIVATE (slider);
    GtkAllocation fan_win_alloc;
    GtkAllocation slider_alloc;

    gtk_widget_get_allocation(p->fan_window, &fan_win_alloc);
    gtk_widget_get_allocation(GTK_WIDGET(slider), &slider_alloc);

    if (p->orientation == GTK_ORIENTATION_VERTICAL)
    {
        return 2 * (FAN_RISE / FAN_RUN)
                 * p->cur_fan.width + slider_alloc.height;
    }
    else
    {
        return 2 * (FAN_RISE / FAN_RUN)
                 * p->cur_fan.height + slider_alloc.width;
    }
}

static gboolean phin_fan_slider_fan_expose (GtkWidget*      widget,
                                            GdkEventExpose* event,
                                            PhinFanSlider*  slider)
{
    (void)widget; (void)event;
    phin_fan_slider_draw_fan (slider);

    return TRUE;
}

static void phin_fan_slider_fan_show (GtkWidget* widget,
                                      GtkWidget* slider)
{
    (void)widget; (void)slider;
}

static gboolean phin_fan_slider_hint_expose (GtkWidget* widget,
                                             GdkEventExpose* event,
                                             GtkWidget* slider)
{
    (void)widget; (void)slider; (void)event;
    return TRUE;
}


/*  setup the hint arrows. these should hint to the user to move the
 *  mouse in such a direction as to bring the fans out.
 */

static void phin_fan_slider_update_hints (PhinFanSlider* slider)
{
    (void)slider;
/*
    GdkRegion* oldclip0 = p->hint_clip0;
    GdkRegion* oldclip1 = p->hint_clip1;

    gtk_window_resize (GTK_WINDOW (p->hint_window0), 9, 9);
    gtk_window_resize (GTK_WINDOW (p->hint_window1), 9, 9);

    if (p->orientation == GTK_ORIENTATION_VERTICAL)
    {
        GdkPoint points0[7] = {
            { 8, 3 },
            { 4, 3 },
            { 4, 0 },
            { 0, 4 },
            { 4, 8 },
            { 4, 6 },
            { 8, 6 }
        };

        GdkPoint points1[7] = {
            { 0, 3 },
            { 4, 3 },
            { 4, 0 },
            { 8, 4 },
            { 4, 8 },
            { 4, 6 },
            { 0, 6 }
        };

        p->hint_clip0 = gdk_region_polygon (points0, 7, GDK_EVEN_ODD_RULE);
        p->hint_clip1 = gdk_region_polygon (points1, 7, GDK_EVEN_ODD_RULE);

        gdk_window_shape_combine_region (p->hint_window0->window,
                                         p->hint_clip0, 0, 0);
        gdk_window_shape_combine_region (p->hint_window1->window,
                                         p->hint_clip1, 0, 0);
    }
    else
    {
        GdkPoint points0[7] = {
            { 3, 8 },
            { 3, 4 },
            { 0, 4 },
            { 4,-1 },
            { 9, 4 },
            { 6, 4 },
            { 6, 8 }
        };

        GdkPoint points1[7] = {
            { 3, 0 },
            { 3, 4 },
            { 0, 4 },
            { 4, 9 },
            { 9, 4 },
            { 6, 4 },
            { 6, 0 }
        };

        p->hint_clip0 = gdk_region_polygon (points0, 7, GDK_EVEN_ODD_RULE);
        p->hint_clip1 = gdk_region_polygon (points1, 7, GDK_EVEN_ODD_RULE);

        gdk_window_shape_combine_region (p->hint_window0->window,
                                         p->hint_clip0, 0, 0);
        gdk_window_shape_combine_region (p->hint_window1->window,
                                         p->hint_clip1, 0, 0);
    }
          
    if (oldclip0 != NULL)
        gdk_region_destroy (oldclip0);
    if (oldclip1 != NULL)
        gdk_region_destroy (oldclip1);
*/
}


static void phin_fan_slider_adjustment_changed (GtkAdjustment* adj,
                                                PhinFanSlider* slider)
{
    PhinFanSliderPrivate* p = PHIN_FAN_SLIDER_GET_PRIVATE (slider);
    GtkWidget* widget;
    double adj_lower, adj_upper, adj_value;

    g_return_if_fail (PHIN_IS_FAN_SLIDER (slider));

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

static void phin_fan_slider_adjustment_value_changed (GtkAdjustment* adj,
                                                      PhinFanSlider* slider)
{
    PhinFanSliderPrivate* p = PHIN_FAN_SLIDER_GET_PRIVATE (slider);
    GtkWidget* widget;
    double adj_lower, adj_upper, adj_value;

    g_return_if_fail (PHIN_IS_FAN_SLIDER (slider));

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
        phin_fan_slider_get_value(slider);
        /* update value of external adjustment */
    }
}
