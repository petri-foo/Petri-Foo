#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <math.h>
#include "phatprivate.h"
#include "phatfanslider.h"

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

/* signals */
enum
{
    VALUE_CHANGED_SIGNAL,
    CHANGED_SIGNAL,
    LAST_SIGNAL,
};

static int phat_fan_slider_signals [LAST_SIGNAL] = { 0, 0 };

static GtkWidgetClass* parent_class   = NULL;
static int             fan_max_height = 0;
static int             fan_max_width  = 0;

static void phat_fan_slider_class_init      (PhatFanSliderClass* klass);
static void phat_fan_slider_init            (PhatFanSlider* slider);
static void phat_fan_slider_destroy         (GtkObject* object);
static void phat_fan_slider_realize         (GtkWidget* widget);
static void phat_fan_slider_unrealize       (GtkWidget *widget);
static void phat_fan_slider_map             (GtkWidget *widget);
static void phat_fan_slider_unmap           (GtkWidget *widget);
static void phat_fan_slider_draw_fan        (PhatFanSlider* slider);
static int  phat_fan_slider_get_fan_length  (PhatFanSlider* slider);


static void phat_fan_slider_update_hints    (PhatFanSlider* slider);

static void phat_fan_slider_size_request    (GtkWidget*widget,
                                            GtkRequisition* requisition);

static void phat_fan_slider_size_allocate   (GtkWidget* widget,
                                            GtkAllocation* allocation);

static gboolean phat_fan_slider_expose      (GtkWidget* widget,
                                            GdkEventExpose* event);

static gboolean phat_fan_slider_button_press(GtkWidget* widget,
                                            GdkEventButton* event);

static gboolean phat_fan_slider_button_release  (GtkWidget* widget,
                                                GdkEventButton* event);

static gboolean phat_fan_slider_key_press   (GtkWidget* widget,
                                            GdkEventKey* event);

static gboolean phat_fan_slider_scroll      (GtkWidget* widget,
                                            GdkEventScroll* event);

static gboolean phat_fan_slider_motion_notify   (GtkWidget* widget,
                                                GdkEventMotion* event);

static gboolean phat_fan_slider_enter_notify    (GtkWidget* widget,
                                                GdkEventCrossing* event);

static gboolean phat_fan_slider_leave_notify    (GtkWidget* widget,
                                                GdkEventCrossing* event);

static void phat_fan_slider_calc_layout     (PhatFanSlider* slider,
                                            int* x, int* y, int* w, int* h);

static void phat_fan_slider_update_value    (PhatFanSlider* slider,
                                            int x_root, int y_root);

static void phat_fan_slider_update_fan      (PhatFanSlider* slider,
                                            int x, int y);

static gboolean phat_fan_slider_fan_expose (GtkWidget*      widget,
                                            GdkEventExpose* event,
                                            PhatFanSlider*  slider);

static void phat_fan_slider_fan_show (GtkWidget* widget,
                                      GtkWidget* slider);

static gboolean phat_fan_slider_hint_expose (GtkWidget* widget,
                                             GdkEventExpose* event,
                                             GtkWidget* slider);

static void phat_fan_slider_adjustment_changed (GtkAdjustment* adjustment,
                                                PhatFanSlider* slider);

static void phat_fan_slider_adjustment_value_changed (GtkAdjustment* adjustment,
                                                      PhatFanSlider* slider);

GType phat_fan_slider_get_type ( )
{
    static GType type = 0;

    if (!type)
    {
        static const GTypeInfo info =
            {
                sizeof (PhatFanSliderClass),
                NULL,
                NULL,
                (GClassInitFunc) phat_fan_slider_class_init,
                NULL,
                NULL,
                sizeof (PhatFanSlider),
                0,
                (GInstanceInitFunc) phat_fan_slider_init,
                NULL
            };

        type = g_type_register_static (GTK_TYPE_WIDGET,
                                       "PhatFanSlider",
                                       &info,
                                       G_TYPE_FLAG_ABSTRACT);
    }

    return type;
}

/**
 * phat_fan_slider_set_value:
 * @slider: a #PhatFanSlider
 * @value: a new value for the slider
 * 
 * Sets the current value of the slider.  If the value is outside the
 * range of values allowed by @slider, it will be clamped to fit
 * within them.  The slider emits the "value-changed" signal if the
 * value changes.
 *
 */
void phat_fan_slider_set_value (PhatFanSlider* slider, double value)
{
    g_return_if_fail (PHAT_IS_FAN_SLIDER (slider));

    gdouble lower = gtk_adjustment_get_lower(slider->adjustment);
    gdouble upper = gtk_adjustment_get_upper(slider->adjustment);

    value = CLAMP (value, lower, upper);

    gtk_adjustment_set_value (slider->adjustment, value);

    if(slider->is_log)
    {           
        gtk_adjustment_set_value(GTK_ADJUSTMENT(slider->adjustment_prv),
                    log(value - lower) / log(upper - lower));
    }
    else
    {
        gtk_adjustment_set_value(GTK_ADJUSTMENT(slider->adjustment_prv),
                    (value - lower) / (upper - lower));
    }
}

void phat_fan_slider_set_log (PhatFanSlider* slider, gboolean is_log)
{
    g_return_if_fail (PHAT_IS_FAN_SLIDER (slider));

    slider->is_log = is_log;
}

gboolean phat_fan_slider_is_log (PhatFanSlider* slider)
{
    g_return_val_if_fail (PHAT_IS_FAN_SLIDER (slider), 0);

    return slider->is_log;
}

    

/**
 * phat_fan_slider_get_value:
 * @slider: a #PhatFanSlider
 *
 * Retrieves the current value of the slider.
 *
 * Returns: current value of the slider.
 *
 */
double phat_fan_slider_get_value (PhatFanSlider* slider)
{
    g_return_val_if_fail (PHAT_IS_FAN_SLIDER (slider), 0);

    gdouble lower = gtk_adjustment_get_lower(slider->adjustment);
    gdouble upper = gtk_adjustment_get_upper(slider->adjustment);
    gdouble value = 0;
    gdouble prv_value = gtk_adjustment_get_value(slider->adjustment_prv);

    if(slider->is_log)
    {
        value = exp(prv_value * log(upper - lower)) + lower;
    }
    else
    {
        value = prv_value * (upper - lower) + lower;
    }

    gtk_adjustment_set_value(slider->adjustment, value);
    return value;
}

/**
 * phat_fan_slider_set_range:
 * @slider: a #PhatFanSlider
 * @lower: lowest allowable value
 * @upper: highest allowable value
 * 
 * Sets the range of allowable values for the slider, and  clamps the slider's
 * current value to be between @lower and @upper.
 *
 */
void phat_fan_slider_set_range (PhatFanSlider* slider,
                                double lower, double upper)
{
    double value;
     
    g_return_if_fail (PHAT_IS_FAN_SLIDER (slider));
    g_return_if_fail (lower <= upper);

    gtk_adjustment_set_lower(slider->adjustment, lower);
    gtk_adjustment_set_upper(slider->adjustment, upper);

    value = CLAMP (gtk_adjustment_get_value(slider->adjustment), lower, upper);

    // XXX not sure about these
    gtk_adjustment_changed (slider->adjustment);
    gtk_adjustment_set_value (slider->adjustment, value);
}

/**
 * phat_fan_slider_get_range:
 * @slider: a #PhatFanSlider
 * @lower: retrieves lowest allowable value
 * @upper: retrieves highest allowable value
 *
 * Places the range of allowable values for @slider into @lower
 * and @upper.  Either variable may be set to %NULL if you are not
 * interested in its value.
 *
 */
void phat_fan_slider_get_range (PhatFanSlider* slider,
                                double* lower, double* upper)
{
    g_return_if_fail (PHAT_IS_FAN_SLIDER (slider));

    if (lower)
        *lower = gtk_adjustment_get_lower(slider->adjustment);
    if (upper)
        *upper = gtk_adjustment_get_upper(slider->adjustment);
}

/**
 * phat_fan_slider_set_adjustment:
 * @slider: a #PhatFanSlider
 * @adjustment: a #GtkAdjustment
 *
 * Sets the adjustment used by @slider.  Every #PhatFanSlider uses an
 * adjustment to store its current value and its range of allowable
 * values.  If @adjustment is %NULL, a new adjustment with a value of
 * zero and a range of [-1.0, 1.0] will be created.
 *
 */
void phat_fan_slider_set_adjustment (PhatFanSlider* slider,
                                     GtkAdjustment* adjustment)
{
    g_return_if_fail (PHAT_IS_FAN_SLIDER (slider));
    g_return_if_fail (slider->adjustment != adjustment);

    if (!adjustment)
        adjustment = (GtkAdjustment*) gtk_adjustment_new (0.0, -1.0, 1.0, 1.0, 1.0, 0.0);
    else
        g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));

    if (slider->adjustment)
    {
        g_signal_handlers_disconnect_by_func (slider->adjustment,
                                              phat_fan_slider_adjustment_changed,
                                              (gpointer) slider);
        g_signal_handlers_disconnect_by_func (slider->adjustment,
                                              phat_fan_slider_adjustment_value_changed,
                                              (gpointer) slider);
        g_object_unref (slider->adjustment);
    }

    slider->adjustment = adjustment;
    g_object_ref (adjustment);
    g_object_ref_sink (GTK_OBJECT (adjustment));

    phat_fan_slider_adjustment_changed(slider->adjustment, slider);

    phat_fan_slider_set_value(PHAT_FAN_SLIDER (slider),
                                gtk_adjustment_get_value(adjustment));
}

/**
 * phat_fan_slider_get_adjustment:
 * @slider: a #PhatFanSlider
 *
 * Retrives the current adjustment in use by @slider.
 *
 * Returns: @slider's current #GtkAdjustment
 *
 */
GtkAdjustment* phat_fan_slider_get_adjustment (PhatFanSlider* slider)
{
    g_return_val_if_fail (PHAT_IS_FAN_SLIDER (slider), NULL);

    /* I can't imagine this ever being true, but just to be
     * "safe" ... */
    if (!slider->adjustment)
        phat_fan_slider_set_adjustment (slider, NULL);

    return slider->adjustment;
}

/**
 * phat_fan_slider_set_inverted:
 * @slider: a #PhatFanSlider
 * @inverted: %TRUE to invert the fanslider
 *  
 * Sets in which direction the fanslider should draw increasing
 * values.  By default, horizontal fansliders draw low to high from
 * left to right, and vertical fansliders draw from bottom to top.
 * You can reverse this behavior by setting @inverted to %TRUE.
 * 
 */
void phat_fan_slider_set_inverted (PhatFanSlider* slider, gboolean inverted)
{
    g_return_if_fail (PHAT_IS_FAN_SLIDER (slider));

    slider->inverted = inverted;
    gtk_widget_queue_draw (GTK_WIDGET (slider));
}

/**
 * phat_fan_slider_get_inverted:
 * @slider: a #PhatFanSlider
 *
 * Determines whether @slider is inverted or not.
 *
 * Returns: %TRUE if @slider is inverted
 *
 */
gboolean phat_fan_slider_get_inverted (PhatFanSlider* slider)
{
    g_return_val_if_fail (PHAT_IS_FAN_SLIDER (slider), FALSE);

    return slider->inverted;
}

/**
 * phat_fan_slider_set_default_value:
 * @slider: a #PhatFanSlider
 * @value: the default value
 *  
 * Set default value of the slider. Slider is reset to this value
 * when middle mouse button is pressed.
 */
void phat_fan_slider_set_default_value(PhatFanSlider* slider, gdouble value)
{
    g_return_if_fail(PHAT_IS_FAN_SLIDER(slider));

    slider->use_default_value = TRUE;
    slider->default_value = value;
}

static void phat_fan_slider_class_init (PhatFanSliderClass* klass)
{
    GtkObjectClass* object_class = (GtkObjectClass*) klass;
    GtkWidgetClass* widget_class = (GtkWidgetClass*) klass;
    GdkScreen*      screen       = gdk_screen_get_default ( );

    debug ("class init\n");
     
    parent_class = g_type_class_peek (gtk_widget_get_type ());

    object_class->destroy = phat_fan_slider_destroy;

    widget_class->realize = phat_fan_slider_realize;
    widget_class->unrealize = phat_fan_slider_unrealize;
    widget_class->map = phat_fan_slider_map;
    widget_class->unmap = phat_fan_slider_unmap;
    widget_class->expose_event = phat_fan_slider_expose;
    widget_class->size_request = phat_fan_slider_size_request;
    widget_class->size_allocate = phat_fan_slider_size_allocate;
    widget_class->button_press_event = phat_fan_slider_button_press;
    widget_class->button_release_event = phat_fan_slider_button_release;
    widget_class->key_press_event = phat_fan_slider_key_press;
    widget_class->scroll_event = phat_fan_slider_scroll;
    widget_class->motion_notify_event = phat_fan_slider_motion_notify;
    widget_class->enter_notify_event = phat_fan_slider_enter_notify;
    widget_class->leave_notify_event = phat_fan_slider_leave_notify;
     
    /**
     * PhatFanSlider::value-changed:
     * @slider: the object on which the signal was emitted
     *
     * The "value-changed" signal is emitted when the value of the
     * slider's adjustment changes.
     *
     */
    phat_fan_slider_signals[VALUE_CHANGED_SIGNAL] =
        g_signal_new ("value-changed",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (PhatFanSliderClass, value_changed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

    /**
     * PhatFanSlider::changed:
     * @slider: the object on which the signal was emitted
     *
     * The "changed" signal is emitted when any parameter of the
     * slider's adjustment changes, except for the %value parameter.
     *
     */
    phat_fan_slider_signals[CHANGED_SIGNAL] =
        g_signal_new ("changed",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (PhatFanSliderClass, changed),
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
}

static void phat_fan_slider_init (PhatFanSlider* slider)
{
    gtk_widget_set_has_window(GTK_WIDGET(slider), FALSE);
    gtk_widget_set_can_focus(GTK_WIDGET(slider), TRUE);

    debug ("init\n");

    slider->adjustment = NULL;
    slider->adjustment_prv = (GtkAdjustment*) gtk_adjustment_new (0.0, 0.0, 1.0, 0.1, 0.1, 0.0);
    slider->val = 0.69;
    slider->center_val = -1;
    slider->xclick_root = 0;
    slider->yclick_root = 0;
    slider->xclick = 0;
    slider->yclick = 0;
    slider->fan_max_thickness = 1;
    slider->inverted = FALSE;
    slider->direction = 0;
    slider->state = STATE_NORMAL;
    slider->orientation = GTK_ORIENTATION_HORIZONTAL;
    slider->fan_window = NULL;
    slider->arrow_cursor = NULL;
    slider->empty_cursor = NULL;
    slider->event_window = NULL;
    slider->hint_window0 = NULL;
    slider->hint_window1 = NULL;
    slider->is_log = 0;
    slider->use_default_value = FALSE;

    g_signal_connect (slider->adjustment_prv, "changed",
                      G_CALLBACK (phat_fan_slider_adjustment_changed),
                      (gpointer) slider);
    g_signal_connect (slider->adjustment_prv, "value_changed",
                      G_CALLBACK (phat_fan_slider_adjustment_value_changed),
                      (gpointer) slider);

    phat_fan_slider_adjustment_changed (slider->adjustment_prv, slider);
    phat_fan_slider_adjustment_value_changed (slider->adjustment_prv, slider);

}

static void phat_fan_slider_destroy (GtkObject* object)
{
    GtkObjectClass* klass;
    PhatFanSlider* slider;
    GtkWidget* widget;
     
    debug ("destroy %p\n", object);
     
    g_return_if_fail (object != NULL);
    g_return_if_fail (PHAT_IS_FAN_SLIDER (object));

    klass = GTK_OBJECT_CLASS (parent_class);
    slider = (PhatFanSlider*) object;
    widget = GTK_WIDGET (object);

    if (slider->arrow_cursor != NULL)
    {
        gdk_cursor_unref (slider->arrow_cursor);
        slider->arrow_cursor = NULL;
    }

    if (slider->empty_cursor != NULL)
    {
        gdk_cursor_unref (slider->empty_cursor);
        slider->empty_cursor = NULL;
    }

    if (slider->event_window != NULL)
    {
        gdk_window_destroy (slider->event_window);
        slider->event_window = NULL;
    }

    if (slider->fan_window != NULL)
    {
        gtk_widget_destroy (slider->fan_window);
        slider->fan_window = NULL;
    }

    if (slider->hint_window0 != NULL)
    {
        gtk_widget_destroy (slider->hint_window0);
        slider->hint_window0 = NULL;
    }

    if (slider->hint_window1 != NULL)
    {
        gtk_widget_destroy (slider->hint_window1);
        slider->hint_window1 = NULL;
    }

    if (slider->adjustment)
    {
        g_signal_handlers_disconnect_by_func (slider->adjustment,
                                              phat_fan_slider_adjustment_changed,
                                              (gpointer) slider);
        g_signal_handlers_disconnect_by_func (slider->adjustment,
                                              phat_fan_slider_adjustment_value_changed,
                                              (gpointer) slider);
        // called ref on it so we must call unref
        g_object_unref (slider->adjustment);
        slider->adjustment = NULL;
    }

    if (slider->adjustment_prv)
    {
        g_signal_handlers_disconnect_by_func (slider->adjustment_prv,
                                              phat_fan_slider_adjustment_changed,
                                              (gpointer) slider);
        g_signal_handlers_disconnect_by_func (slider->adjustment_prv,
                                              phat_fan_slider_adjustment_value_changed,
                                              (gpointer) slider);
        //didn't call ref on this one so just destroy
        gtk_object_destroy ((GtkObject*)slider->adjustment_prv);
        slider->adjustment_prv = NULL;
    }

     
    if (klass->destroy)
        klass->destroy (object);
}

static void phat_fan_slider_realize (GtkWidget* widget)
{
    PhatFanSlider* slider;
    GdkWindowAttr attributes;
    gint attributes_mask;
    GdkWindow* window;
    GtkStyle* style;
    debug ("realize\n");

    g_return_if_fail (widget != NULL);
    g_return_if_fail (PHAT_IS_FAN_SLIDER (widget));

    gtk_widget_set_realized(GTK_WIDGET(widget), TRUE);

    slider = (PhatFanSlider*) widget;
     
    if (slider->orientation == GTK_ORIENTATION_VERTICAL)
    {
        slider->arrow_cursor = gdk_cursor_new (GDK_SB_V_DOUBLE_ARROW);
    }
    else
    {
        slider->arrow_cursor = gdk_cursor_new (GDK_SB_H_DOUBLE_ARROW);
    }

    slider->empty_cursor = gdk_cursor_new(GDK_BLANK_CURSOR);

    window = gtk_widget_get_parent_window (widget);
    gtk_widget_set_window(widget, window);
    g_object_ref (window);

    style = gtk_widget_get_style(widget);
    style = gtk_style_attach(style, window);
    gtk_widget_set_style(widget, style);

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
    phat_fan_slider_calc_layout (slider,
                                 &attributes.x,
                                 &attributes.y,
                                 &attributes.width,
                                 &attributes.height);
    attributes_mask = GDK_WA_X | GDK_WA_Y;

    slider->event_window = gdk_window_new (gtk_widget_get_parent_window (widget),
                                           &attributes, attributes_mask);
    gdk_window_set_user_data (slider->event_window, widget);
    gdk_window_set_cursor (slider->event_window, slider->arrow_cursor);

    slider->fan_window = gtk_window_new (GTK_WINDOW_POPUP);
    gtk_window_resize(GTK_WINDOW(slider->fan_window), fan_max_width, fan_max_height);
    gtk_widget_set_app_paintable (slider->fan_window, TRUE);

    g_signal_connect (G_OBJECT (slider->fan_window),
                      "expose-event",
                      G_CALLBACK (phat_fan_slider_fan_expose),
                      (gpointer) slider);

    g_signal_connect (G_OBJECT (slider->fan_window),
                      "show",
                      G_CALLBACK (phat_fan_slider_fan_show),
                      (gpointer) slider);

    g_signal_connect (G_OBJECT(slider->fan_window),
                      "screen-changed",
                      G_CALLBACK (phat_screen_changed), NULL);

    phat_screen_changed(slider->fan_window, NULL, NULL);

    slider->hint_window0 = gtk_window_new (GTK_WINDOW_POPUP);
    gtk_widget_realize (slider->hint_window0);
    g_signal_connect (G_OBJECT (slider->hint_window0),
                      "expose-event",
                      G_CALLBACK (phat_fan_slider_hint_expose),
                      (gpointer) slider);

    slider->hint_window1 = gtk_window_new (GTK_WINDOW_POPUP);
    gtk_widget_realize (slider->hint_window1);
    g_signal_connect (G_OBJECT (slider->hint_window1),
                      "expose-event",
                      G_CALLBACK (phat_fan_slider_hint_expose),
                      (gpointer) slider);

    /* a priming call */
    phat_fan_slider_update_hints (slider);
}

static void phat_fan_slider_unrealize (GtkWidget *widget)
{
    PhatFanSlider* slider = PHAT_FAN_SLIDER (widget);
    GtkWidgetClass* klass = GTK_WIDGET_CLASS (parent_class);

    debug ("unrealize\n");
     
    gdk_cursor_unref (slider->arrow_cursor);
    slider->arrow_cursor = NULL;

    gdk_cursor_unref (slider->empty_cursor);
    slider->empty_cursor = NULL;

    gdk_window_set_user_data (slider->event_window, NULL);
    gdk_window_destroy (slider->event_window);
    slider->event_window = NULL;

    gtk_widget_destroy (slider->fan_window);
    slider->fan_window = NULL;

    gtk_widget_destroy (slider->hint_window0);
    slider->hint_window0 = NULL;

    gtk_widget_destroy (slider->hint_window1);
    slider->hint_window1 = NULL;

    if (klass->unrealize)
        klass->unrealize (widget);
}


static void phat_fan_slider_map (GtkWidget *widget)
{
    PhatFanSlider* slider;

    debug ("map\n");
     
    g_return_if_fail (PHAT_IS_FAN_SLIDER (widget));
    slider = (PhatFanSlider*) widget;

    gdk_window_show (slider->event_window);

    GTK_WIDGET_CLASS (parent_class)->map (widget);
}

static void phat_fan_slider_unmap (GtkWidget *widget)
{
    PhatFanSlider* slider;

    debug ("unmap\n");

    g_return_if_fail (PHAT_IS_FAN_SLIDER (widget));
    slider = (PhatFanSlider*) widget;

    gdk_window_hide (slider->event_window);

    GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}

static void phat_fan_slider_size_request (GtkWidget*      widget,
                                          GtkRequisition* requisition)
{
    PhatFanSlider* slider;
    int focus_width, focus_pad;
    int pad;

    debug ("size request\n");

    g_return_if_fail (PHAT_IS_FAN_SLIDER (widget));
    slider = (PhatFanSlider*) widget;

    gtk_widget_style_get (widget,
                          "focus-line-width", &focus_width,
                          "focus-padding", &focus_pad,
                          NULL);

    pad = 2 * (focus_width + focus_pad);

    if (slider->orientation == GTK_ORIENTATION_VERTICAL)
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

static void phat_fan_slider_size_allocate (GtkWidget*     widget,
                                           GtkAllocation* allocation)
{
    PhatFanSlider* slider;
    int x, y;
    int w, h;

    debug ("size allocate\n");

    g_return_if_fail (widget != NULL);
    g_return_if_fail (PHAT_IS_FAN_SLIDER (widget));
    g_return_if_fail (allocation != NULL);

    slider = (PhatFanSlider*) widget;

    gtk_widget_set_allocation(widget, allocation);

    phat_fan_slider_calc_layout (slider, &x, &y, &w, &h);

    if (slider->orientation == GTK_ORIENTATION_VERTICAL)
    {
        slider->fan_max_thickness = ((fan_max_height - h)
                                     / (2*FAN_RISE/FAN_RUN));
    }
    else
    {
        slider->fan_max_thickness = ((fan_max_width - w)
                                     / (2*FAN_RISE/FAN_RUN));
    }
     
    if (gtk_widget_get_realized(widget))
    {
        gdk_window_move_resize (slider->event_window,
                                x, y, w, h);
    }
}




static void draw_fan_rectangle(cairo_t* cr, double x, double y, double w, double h,
                                        GdkColor* col)
{
    gdk_cairo_set_source_color(cr, col);
    cairo_set_line_width(cr, 1.0);
    cairo_rectangle(cr, x, y, w, h);
    cairo_stroke_preserve(cr);
    cairo_fill(cr);
}


static gboolean phat_fan_slider_expose (GtkWidget*      widget,
                                        GdkEventExpose* event)
{
    PhatFanSlider* slider;
    int x, y;
    int w, h;
    int fx, fy;                /* "filled" coordinates */
    int fw, fh;

    GtkStyle* style;
    cairo_t* cr;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (PHAT_IS_FAN_SLIDER (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    if (event->count > 0)
        return FALSE;

    slider = (PhatFanSlider*) widget;

    style = gtk_widget_get_style(widget);
    cr = gdk_cairo_create(gtk_widget_get_window(widget));

    phat_fan_slider_calc_layout (slider, &x, &y, &w, &h);
     
    if (slider->orientation == GTK_ORIENTATION_VERTICAL)
    {
        if (slider->center_val >= 0)
        {
            fw = w;
            fh = ABS (slider->val - slider->center_val) * h;
            fx = x;
            fy = y + h - (slider->center_val * h);

            if ((slider->val > slider->center_val && !slider->inverted)
                || (slider->val < slider->center_val && slider->inverted))
            {
                fy -= fh;
            }

        }
        else
        {
            fw = w;
            fh = slider->val * h;
            fx = x;
            fy = (slider->inverted)? y: y + h - fh;
        }
    }
    else
    {
        if (slider->center_val >= 0)
        {
            fw = ABS (slider->val - slider->center_val) * w;
            fh = h;
            fx = x + (slider->center_val * w);
            fy = y;

            if ((slider->val < slider->center_val && !slider->inverted)
                || (slider->val > slider->center_val && slider->inverted))
            {
                fx -= fw;
            }
        }
        else
        {
            fw = slider->val * w;
            fh = h;
            fx = (slider->inverted)? x + w - fw: x;
            fy = y;
        }
    }

    if (!gtk_widget_is_sensitive(widget))
    {
        draw_fan_rectangle(cr, x, y, w, h, &style->dark[GTK_STATE_INSENSITIVE]);
        draw_fan_rectangle(cr, fx, fy, fw, fh, &style->fg[GTK_STATE_INSENSITIVE]);
    }
    else
    {
        draw_fan_rectangle(cr, x, y, w, h, &style->dark[GTK_STATE_NORMAL]);
        draw_fan_rectangle(cr, fx, fy, fw, fh, &style->base[GTK_STATE_SELECTED]);
    }

    cairo_destroy(cr);

    if (gtk_widget_has_focus (widget))
    {
        int focus_width, focus_pad;
        int pad;
          
        gtk_widget_style_get (widget,
                              "focus-line-width", &focus_width,
                              "focus-padding", &focus_pad,
                              NULL);

        pad = focus_width + focus_pad;

        x -= pad;
        y -= pad;
        w += 2*pad;
        h += 2*pad;

        gtk_paint_focus (style, gtk_widget_get_window(widget),
                                gtk_widget_get_state (widget),
                                NULL, widget, NULL,
                                x, y, w, h);
    }

    if (gtk_widget_get_visible (slider->fan_window))
        gtk_widget_queue_draw (slider->fan_window);
     
    return FALSE;
}


static gboolean phat_fan_slider_button_press (GtkWidget*      widget,
                                              GdkEventButton* event)
{
    PhatFanSlider* slider;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (PHAT_IS_FAN_SLIDER (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    slider = (PhatFanSlider*) widget;

    if(event->button == 1)
    {
        gtk_widget_grab_focus (widget);
         
        if (slider->state == STATE_SCROLL)
        {
            slider->state = STATE_NORMAL;
            gdk_window_set_cursor (slider->event_window, slider->arrow_cursor);
            return FALSE;
        }

        gdk_window_set_cursor (slider->event_window, slider->empty_cursor);

        slider->xclick_root = event->x_root;
        slider->xclick = event->x;
        slider->yclick_root = event->y_root;
        slider->yclick = event->y;
        slider->state = STATE_CLICKED;
    }
    else if (event->button == 2 && slider->use_default_value)
    {
        phat_fan_slider_set_value(slider, slider->default_value);
        return TRUE;
    }

    return FALSE;
}


static gboolean phat_fan_slider_button_release (GtkWidget*      widget,
                                                GdkEventButton* event)
{
    PhatFanSlider* slider;
     
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (PHAT_IS_FAN_SLIDER (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    slider = (PhatFanSlider*) widget;
    gdk_window_set_cursor (slider->event_window, slider->arrow_cursor);

    if (slider->state == STATE_CLICKED)
    {
        slider->state = STATE_NORMAL;

        phat_warp_pointer (event->x_root,
                           event->y_root,
                           slider->xclick_root,
                           slider->yclick_root);

        if (gtk_widget_get_visible (slider->fan_window))
            gtk_widget_hide (slider->fan_window);

        if (gtk_widget_get_visible (slider->hint_window0))
            gtk_widget_hide (slider->hint_window0);

        if (gtk_widget_get_visible (slider->hint_window1))
            gtk_widget_hide (slider->hint_window1);
    }
     
    return FALSE;
}

static gboolean phat_fan_slider_key_press (GtkWidget* widget,
                                           GdkEventKey* event)
{
    PhatFanSlider* slider = PHAT_FAN_SLIDER (widget);
    GtkAdjustment* adj = slider->adjustment_prv;
    gdouble inc;

    debug ("key press\n");

    if (slider->orientation == GTK_ORIENTATION_VERTICAL)
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

    if (slider->inverted)
        inc = -inc;
     
    gtk_adjustment_set_value (adj, gtk_adjustment_get_value(adj) + inc);

    return TRUE;
}

static gboolean phat_fan_slider_scroll (GtkWidget* widget,
                                        GdkEventScroll* event)
{
    PhatFanSlider* slider = PHAT_FAN_SLIDER (widget);

    gdouble val, pinc;

    gtk_widget_grab_focus (widget);
     
    slider->state = STATE_SCROLL;

    slider->xclick_root = event->x_root;
    slider->yclick_root = event->y_root;
    slider->xclick = event->x;
    slider->yclick = event->y;

    gdk_window_set_cursor (slider->event_window, slider->empty_cursor);

    val = gtk_adjustment_get_value(slider->adjustment_prv);
    pinc = gtk_adjustment_get_page_increment(slider->adjustment_prv);

    if (((event->direction == GDK_SCROLL_UP
          || event->direction == GDK_SCROLL_RIGHT) && !slider->inverted)

        || ((event->direction == GDK_SCROLL_DOWN
             || event->direction == GDK_SCROLL_LEFT) && slider->inverted))
    {
        gtk_adjustment_set_value (slider->adjustment_prv, val + pinc);
    }
    else
    {
        gtk_adjustment_set_value (slider->adjustment_prv, val - pinc);
    }

    return TRUE;
}

/* ctrl locks precision, shift locks value */
static gboolean phat_fan_slider_motion_notify (GtkWidget*      widget,
                                               GdkEventMotion* event)
{
    PhatFanSlider* slider;

    GtkAllocation fan_alloc;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (PHAT_IS_FAN_SLIDER (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);

    slider = (PhatFanSlider*) widget;
    gtk_widget_get_allocation(slider->fan_window, &fan_alloc);

    switch (slider->state)
    {
    case STATE_SCROLL:
        if (ABS (event->x - slider->xclick) >= THRESHOLD
            || ABS (event->y - slider->yclick) >= THRESHOLD)
        {
            gdk_window_set_cursor (slider->event_window, slider->arrow_cursor);
            slider->state = STATE_NORMAL;
        }
    case STATE_NORMAL:
        goto skip;
        break;
    }

    if (!(event->state & GDK_CONTROL_MASK))
        phat_fan_slider_update_fan (slider, event->x, event->y);

    if (!(event->state & GDK_SHIFT_MASK))
        phat_fan_slider_update_value (slider, event->x_root, event->y_root);

    if (slider->orientation == GTK_ORIENTATION_VERTICAL)
    {
        int destx = event->x_root;
        int width;

        gdk_window_get_geometry (slider->event_window,
                                 NULL, NULL, &width, NULL, NULL);

        if (gtk_widget_get_visible (slider->fan_window))
        {
            if (event->x_root > slider->xclick_root)
            {
                if (event->state & GDK_CONTROL_MASK)
                {
                    destx = (slider->xclick_root
                             + width
                             - slider->xclick
                             + fan_alloc.width);
                }
                else
                {
                    destx = MIN (event->x_root,
                                 slider->xclick_root
                                 + width
                                 - slider->xclick
                                 + fan_alloc.width);
                }
            }
            else
            {
                if (event->state & GDK_CONTROL_MASK)
                {
                    destx = (slider->xclick_root
                             - slider->xclick
                             - fan_alloc.width);
                }
                else
                {
                    destx = MAX (event->x_root,
                                 slider->xclick_root
                                 - slider->xclick
                                 - fan_alloc.width);
                }
            }
        }
        else if (event->state & GDK_CONTROL_MASK)
        {
            destx = slider->xclick_root;
        }

        phat_warp_pointer (event->x_root,
                           event->y_root,
                           destx,
                           slider->yclick_root);
    }
    else
    {
        int desty = event->y_root;
        int height;

        gdk_window_get_geometry (slider->event_window,
                                 NULL, NULL, NULL, &height, NULL);

        if (gtk_widget_get_visible (slider->fan_window))
        {
            if (event->y_root > slider->yclick_root)
            {
                if (event->state & GDK_CONTROL_MASK)
                {
                    desty = (slider->yclick_root
                             + height
                             - slider->yclick
                             + fan_alloc.height);
                }
                else
                {
                    desty = MIN (event->y_root,
                                 slider->yclick_root
                                 + height
                                 - slider->yclick
                                 + fan_alloc.height);
                }
            }
            else
            {
                if (event->state & GDK_CONTROL_MASK)
                {
                    desty = (slider->yclick_root
                             - slider->yclick
                             - fan_alloc.height);
                }
                else
                {
                    desty = MAX (event->y_root,
                                 slider->yclick_root
                                 - slider->yclick
                                 - fan_alloc.height);
                }
            }
        }
        else if (event->state & GDK_CONTROL_MASK)
        {
            desty = slider->yclick_root;
        }
               
        phat_warp_pointer (event->x_root,
                           event->y_root,
                           slider->xclick_root,
                           desty);
    }

    gtk_widget_queue_draw (widget);

    /* necessary in case update_fan() doesn't get called */
    if (gtk_widget_get_visible(slider->fan_window))
        gtk_widget_queue_draw (slider->fan_window);

skip:
    /* signal that we want more motion events */
    gdk_window_get_pointer (NULL, NULL, NULL, NULL);

    return FALSE;
}


static gboolean phat_fan_slider_enter_notify (GtkWidget* widget,
                                              GdkEventCrossing* event)
{
    GtkAllocation win0_alloc;
    GtkAllocation win1_alloc;

    int width;
    int height;
    PhatFanSlider* slider = PHAT_FAN_SLIDER (widget);

    if (slider->state == STATE_NORMAL)
        gdk_window_set_cursor (slider->event_window, slider->arrow_cursor);

    phat_fan_slider_update_hints (slider);

    gdk_window_get_geometry (slider->event_window,
                            NULL, NULL, &width, &height, NULL);

    gtk_widget_get_allocation(slider->hint_window0, &win0_alloc);
    gtk_widget_get_allocation(slider->hint_window1, &win1_alloc);

    if (slider->orientation == GTK_ORIENTATION_VERTICAL)
    {
        gtk_window_move (GTK_WINDOW (slider->hint_window0),
                        event->x_root - event->x - win0_alloc.width,
                        (event->y_root - event->y) + (height - win0_alloc.height) / 2);

        gtk_window_move (GTK_WINDOW (slider->hint_window1),
                        event->x_root - event->x + width,
                        (event->y_root - event->y) + (height - win1_alloc.height) / 2);
    }
    else
    {
        gtk_window_move (GTK_WINDOW (slider->hint_window0),
                        (event->x_root - event->x) + (width - win0_alloc.width) / 2,
                        event->y_root - event->y - win0_alloc.height);

        gtk_window_move (GTK_WINDOW (slider->hint_window1),
                        (event->x_root - event->x) + (width - win1_alloc.width) / 2,
                        event->y_root - event->y + height);
    }

    return FALSE;
}

static gboolean phat_fan_slider_leave_notify (GtkWidget* widget,
                                              GdkEventCrossing* event)
{
    PhatFanSlider* slider = PHAT_FAN_SLIDER (widget);
     
    if (slider->state == STATE_SCROLL)
    {
        gdk_window_set_cursor (slider->event_window, NULL);
        slider->state = STATE_NORMAL;
    }

    if (gtk_widget_get_visible (slider->hint_window0))
        gtk_widget_hide (slider->hint_window0);

    if (gtk_widget_get_visible (slider->hint_window1))
        gtk_widget_hide (slider->hint_window1);

    return FALSE;
}


/* helper to reduce copy & paste */
static void
fan_slider_draw(cairo_t* cr, GdkPixmap* bitmap, PhatFanSlider* slider,
                    int x, int y, int w, int h, int length, gdouble value)
{
    int offset;

    GtkWidget*  widget = GTK_WIDGET(slider);
    GtkStyle*   style = (bitmap) ? NULL : gtk_widget_get_style(widget);

    double r1, g1, b1, a1; /* slider bg */
    double r2, g2, b2, a2; /* slider fg */

    /*  length: (orientation dependent, orientation as slider length)
            length = fan length, so dependant on size of fan */

    /*  slider->center_val: dependant on range of slider and if zero is
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

    if (!supports_alpha || bitmap)
        cairo_set_source_rgb (cr, 0, 0, 0);
    else
        cairo_set_source_rgba (cr, 0, 0, 0, 0);

    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint (cr);

    if (slider->orientation == GTK_ORIENTATION_VERTICAL)
    {
        int sign_cur_fan_w;

        float   cv = (slider->center_val >= 0.0f)
                        ? slider->center_val
                        : 0.0f;

        int fan_top_y = y - length / 2 + h / 2;
        int fan_val_y = length * value;
        int fan_cv_y =  length * cv;

        if (slider->direction)
        {
            sign_cur_fan_w = 1 * slider->cur_fan.width;
            offset = w;
        }
        else
        { 
            sign_cur_fan_w = -1 * slider->cur_fan.width;
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
                rest of the display for the duration of its showing */
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
    else /* (slider->orientation == GTK_ORIENTATION_HORIZONTAL) */
    {
        int     sign_cur_fan_h;
        float   cv = (slider->center_val >= 0.0f)
                        ? slider->center_val
                        : 0.0f;

        int fan_left_x =    x - length / 2 + w / 2;
        int fan_val_x =     length * value;
        int fan_cv_x =      length * cv;

        if (slider->direction)
        {
            sign_cur_fan_h = 1 * slider->cur_fan.height;
            offset = h;
        }
        else
        {
            sign_cur_fan_h = -1 * slider->cur_fan.height;
            offset = 0;
        }

        /* fan background */
        cairo_move_to (     cr, x,          y + offset);
        /* left-hand line from slider */
        cairo_line_to ( cr, fan_left_x,     y + offset + sign_cur_fan_h);
        /* horizontal fan line */
        cairo_rel_line_to ( cr, length,     0);
        /* right-hand line back to slider */
        cairo_line_to(      cr, x + w,      y + offset);

        if (supports_alpha)
            cairo_set_source_rgba( cr, r1, g1, b1, a1);
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
            cairo_set_source_rgba( cr, r2, g2, b2, a2);
        else
            cairo_set_source_rgb( cr, r2, g2, b2);

        cairo_fill(cr);

    }
}


static void phat_fan_slider_draw_fan (PhatFanSlider* slider)
{
    GtkWidget* widget = GTK_WIDGET (slider);

    int     x, y, w, h;
    int     length;
    double  value;
    cairo_t* cr;

    if (!gtk_widget_is_drawable (slider->fan_window))
        return;

    phat_fan_slider_calc_layout(slider, &x, &y, &w, &h);

    {
        int root_x, root_y;
        gdk_window_get_origin(gtk_widget_get_window(widget),
                                                    &root_x, &root_y);
        x += root_x;
        y += root_y;
    }

printf("x:%d y:%d w:%d h:%d\n",x,y,w,h);

    cr = gdk_cairo_create(gtk_widget_get_window(slider->fan_window));


    int ww=gdk_window_get_width(gtk_widget_get_window(slider->fan_window));
    int wh=gdk_window_get_height(gtk_widget_get_window(slider->fan_window));

    printf("window w:%d x h:%d\n",ww,wh);

        cairo_surface_t *surface = 0;

    if (!supports_alpha)
    {
        GdkPixmap* bitmap = gdk_pixmap_new(NULL, w, h, 1);

        fan_slider_draw(cr, bitmap, slider, x, y, w, h, length, value);

/*
        int stride = cairo_format_stride_for_width (CAIRO_FORMAT_A1, w);
        unsigned char *data = malloc(stride * h);

        surface = cairo_image_surface_create_for_data(data, CAIRO_FORMAT_A1,
                                                      w, h, stride);

        cairo_set_source_surface(cr, surface, 0, 0);
        fan_slider_draw(cr, TRUE, slider, x, y, w, h, length, value);

/*
        gdk_window_shape_combine_mask(
                                gtk_widget_get_window(slider->fan_window),
                                                         GdkBitmap *mask,
                                                         gint x,
                                                         gint y);
*/
    }
/*
    if (supports_alpha)
        cairo_set_source_rgba (cr, 1.0, 1.0, 1.0, 0.0);
    else
    {
        cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
    }

    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint (cr);
*/

    length = phat_fan_slider_get_fan_length (slider);

    if (slider->orientation == GTK_ORIENTATION_VERTICAL)
    {
        if (slider->inverted)
            value = 1.0 - gtk_adjustment_get_value(slider->adjustment_prv);
        else
            value = gtk_adjustment_get_value(slider->adjustment_prv);
    }
    else
    {
        if (slider->inverted)
            value = 1.0 - gtk_adjustment_get_value(slider->adjustment_prv);
        else
            value = gtk_adjustment_get_value(slider->adjustment_prv);
    }


    fan_slider_draw(cr, NULL, slider, x, y, w, h, length, value);


    cairo_destroy(cr);
}


static void phat_fan_slider_calc_layout (PhatFanSlider* slider,
                                         int* x, int* y, int* w, int* h)
{
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

    if (slider->orientation == GTK_ORIENTATION_VERTICAL)
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


static void phat_fan_slider_update_value (PhatFanSlider* slider,
                                          int x_root, int y_root)
{
    int length;
    double oldval;
    double value;
    double inc;
    GtkAllocation slider_alloc;
     
    if (slider->state != STATE_CLICKED)
        return;

    gtk_widget_get_allocation(GTK_WIDGET(slider), &slider_alloc);

    oldval = slider->val;

    if (slider->orientation == GTK_ORIENTATION_VERTICAL)
    {
        if (gtk_widget_is_drawable (slider->fan_window)
            && (x_root != slider->xclick_root))
        {
            length = phat_fan_slider_get_fan_length (slider);
            inc = ((slider->yclick_root - y_root)
                   * 1.0 / length);
        }
        else
        {
            inc = ((slider->yclick_root - y_root)
                   * 1.0 / slider_alloc.height);
        }
    }
    else
    {
        if (gtk_widget_is_drawable (slider->fan_window)
            && (y_root != slider->yclick_root))
        {
            length = phat_fan_slider_get_fan_length (slider);
            inc = ((x_root - slider->xclick_root)
                   * 1.0 / length);
        }
        else
        {
            inc = ((x_root - slider->xclick_root)
                   * 1.0 / slider_alloc.width);
        }
    }

    if (slider->inverted)
        inc = -inc;

    slider->val += inc;
    slider->val = CLAMP (slider->val, 0, 1);

    if (slider->val != oldval)
    {
        value = (gtk_adjustment_get_lower(slider->adjustment_prv) * (1.0 - slider->val)
                 + gtk_adjustment_get_upper(slider->adjustment_prv) * slider->val);

        g_signal_handlers_block_by_func (G_OBJECT (slider),
                                         phat_fan_slider_adjustment_value_changed,
                                         (gpointer) slider);

        gtk_adjustment_set_value (slider->adjustment_prv, value);

        g_signal_emit (G_OBJECT (slider),
                       phat_fan_slider_signals[VALUE_CHANGED_SIGNAL], 0);

        g_signal_handlers_unblock_by_func (G_OBJECT (slider),
                                           phat_fan_slider_adjustment_value_changed,
                                           (gpointer) slider);
    }
}


static void phat_fan_slider_update_fan (PhatFanSlider* slider,
                                        int x, int y)
{
    int width;
    int height;
    int w, h;
    GtkAllocation fan_win_alloc;

    if (slider->state != STATE_CLICKED)
        return;

    gdk_window_get_geometry (slider->event_window,
                             NULL, NULL, &w, &h, NULL);

    gtk_widget_get_allocation(slider->fan_window, &fan_win_alloc);

    if (slider->orientation == GTK_ORIENTATION_VERTICAL)
    {
        if (x > w)
        {
            width = x - w;
            width = CLAMP (width, 0, slider->fan_max_thickness);
            slider->cur_fan.width = width;
            slider->cur_fan.height = fan_max_height;
            slider->direction = 1;

            if (!gtk_widget_get_visible (slider->fan_window))
                gtk_window_present (GTK_WINDOW (slider->fan_window));

            if (gtk_widget_get_visible (slider->hint_window0))
                gtk_widget_hide (slider->hint_window0);

            if (gtk_widget_get_visible (slider->hint_window1))
                gtk_widget_hide (slider->hint_window1);
        }
        else if (x < 0)
        {
            width = -x;
            width = CLAMP (width, 0, slider->fan_max_thickness);
            slider->cur_fan.width = width;
            slider->cur_fan.height = fan_max_height;
            slider->direction = 0;

            if (!gtk_widget_get_visible (slider->fan_window))
                gtk_window_present (GTK_WINDOW (slider->fan_window));

            if (gtk_widget_get_visible (slider->hint_window0))
                gtk_widget_hide (slider->hint_window0);

            if (gtk_widget_get_visible (slider->hint_window1))
                gtk_widget_hide (slider->hint_window1);
        }
        else if (gtk_widget_get_visible (slider->fan_window))
        {
            gtk_widget_hide (slider->fan_window);
        }
    }
    else
    {
        if (y > h)
        {
            height = y - h;
            height = CLAMP (height, 0, slider->fan_max_thickness);
            slider->cur_fan.width = fan_max_width;
            slider->cur_fan.height = height;
            slider->direction = 1;

            if (!gtk_widget_get_visible (slider->fan_window))
                gtk_window_present (GTK_WINDOW (slider->fan_window));

            if (gtk_widget_get_visible (slider->hint_window0))
                gtk_widget_hide (slider->hint_window0);

            if (gtk_widget_get_visible (slider->hint_window1))
                gtk_widget_hide (slider->hint_window1);
        }
        else if (y < 0)
        {
            height = -y;
            height = CLAMP (height, 0, slider->fan_max_thickness);
            slider->cur_fan.width = fan_max_width;
            slider->cur_fan.height = height;
            slider->direction = 0;

            if (!gtk_widget_get_visible (slider->fan_window))
                gtk_window_present (GTK_WINDOW (slider->fan_window));

            if (gtk_widget_get_visible (slider->hint_window0))
                gtk_widget_hide (slider->hint_window0);

            if (gtk_widget_get_visible (slider->hint_window1))
                gtk_widget_hide (slider->hint_window1);
        }
        else if (gtk_widget_get_visible (slider->fan_window))
        {
            gtk_widget_hide (slider->fan_window);
        }
    }
}

static int phat_fan_slider_get_fan_length (PhatFanSlider* slider)
{
    GtkAllocation fan_win_alloc;
    GtkAllocation slider_alloc;

    gtk_widget_get_allocation(slider->fan_window, &fan_win_alloc);
    gtk_widget_get_allocation(GTK_WIDGET(slider), &slider_alloc);

    if (slider->orientation == GTK_ORIENTATION_VERTICAL)
    {
        return 2 * (FAN_RISE / FAN_RUN)
            * slider->cur_fan.width
            + slider_alloc.height;
    }
    else
    {
        return 2 * (FAN_RISE / FAN_RUN)
            * slider->cur_fan.height
            + slider_alloc.width;
    }
}

static gboolean phat_fan_slider_fan_expose (GtkWidget*      widget,
                                            GdkEventExpose* event,
                                            PhatFanSlider*  slider)
{
    phat_fan_slider_draw_fan (slider);

    return TRUE;
}

static void phat_fan_slider_fan_show (GtkWidget* widget,
                                      GtkWidget* slider)
{
}

static gboolean phat_fan_slider_hint_expose (GtkWidget* widget,
                                             GdkEventExpose* event,
                                             GtkWidget* slider)
{
    return TRUE;
}


/*  setup the hint arrows. these should hint to the user to move the
 *  mouse in such a direction as to bring the fans out.
 */

static void phat_fan_slider_update_hints (PhatFanSlider* slider)
{
/*
    GdkRegion* oldclip0 = slider->hint_clip0;
    GdkRegion* oldclip1 = slider->hint_clip1;

    gtk_window_resize (GTK_WINDOW (slider->hint_window0), 9, 9);
    gtk_window_resize (GTK_WINDOW (slider->hint_window1), 9, 9);

    if (slider->orientation == GTK_ORIENTATION_VERTICAL)
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

        slider->hint_clip0 = gdk_region_polygon (points0, 7, GDK_EVEN_ODD_RULE);
        slider->hint_clip1 = gdk_region_polygon (points1, 7, GDK_EVEN_ODD_RULE);

        gdk_window_shape_combine_region (slider->hint_window0->window,
                                         slider->hint_clip0, 0, 0);
        gdk_window_shape_combine_region (slider->hint_window1->window,
                                         slider->hint_clip1, 0, 0);
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

        slider->hint_clip0 = gdk_region_polygon (points0, 7, GDK_EVEN_ODD_RULE);
        slider->hint_clip1 = gdk_region_polygon (points1, 7, GDK_EVEN_ODD_RULE);

        gdk_window_shape_combine_region (slider->hint_window0->window,
                                         slider->hint_clip0, 0, 0);
        gdk_window_shape_combine_region (slider->hint_window1->window,
                                         slider->hint_clip1, 0, 0);
    }
          
    if (oldclip0 != NULL)
        gdk_region_destroy (oldclip0);
    if (oldclip1 != NULL)
        gdk_region_destroy (oldclip1);
*/
}


static void phat_fan_slider_adjustment_changed (GtkAdjustment* adj,
                                                PhatFanSlider* slider)
{
    GtkWidget* widget;
    double adj_lower, adj_upper, adj_value;

    g_return_if_fail (PHAT_IS_FAN_SLIDER (slider));

    widget = GTK_WIDGET (slider);

    adj_lower = gtk_adjustment_get_lower(adj);
    adj_upper = gtk_adjustment_get_upper(adj);
    adj_value = gtk_adjustment_get_value(adj);

    if (adj_lower < 0 && adj_upper > 0)
    {
        slider->center_val = (-adj_lower / (adj_upper - adj_lower));
    }
    else
    {
        slider->center_val = -1;
    }

    slider->val = ((adj_value - adj_lower)
                   / (adj_upper - adj_lower));

    gtk_widget_queue_draw (GTK_WIDGET (slider));

    if (gtk_widget_get_realized (widget))
        gdk_window_process_updates (gtk_widget_get_window(widget), FALSE);

    g_signal_emit (G_OBJECT (slider),
                   phat_fan_slider_signals[CHANGED_SIGNAL], 0);
}

static void phat_fan_slider_adjustment_value_changed (GtkAdjustment* adj,
                                                      PhatFanSlider* slider)
{
    GtkWidget* widget;
    double adj_lower, adj_upper, adj_value;

    g_return_if_fail (PHAT_IS_FAN_SLIDER (slider));

    widget = GTK_WIDGET (slider);
    adj_lower = gtk_adjustment_get_lower(adj);
    adj_upper = gtk_adjustment_get_upper(adj);
    adj_value = gtk_adjustment_get_value(adj);

    slider->val = ((adj_value - adj_lower) / (adj_upper - adj_lower));

    gtk_widget_queue_draw (widget);

    if (gtk_widget_get_realized (widget))
        gdk_window_process_updates (gtk_widget_get_window(widget), FALSE);

    g_signal_emit (G_OBJECT (slider),
                   phat_fan_slider_signals[VALUE_CHANGED_SIGNAL], 0);

    if (slider->adjustment != NULL)
    {
        phat_fan_slider_get_value(slider); /* update value of external adjustment */
    }
}
