#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "phinprivate.h"
#include "phinsliderbutton.h"

/* signals */
enum
{
    CHANGED_SIGNAL,
    VALUE_CHANGED_SIGNAL,
    LAST_SIGNAL,
};

/* hilite states */
enum
{
    LEFT_ARROW = 1,
    RIGHT_ARROW,
    LABEL,
};

/* action states */
enum
{
    STATE_NORMAL,
    STATE_PRESSED,
    STATE_SLIDE,
    STATE_ENTRY,
    STATE_SCROLL,
};

/* magic numbers */
enum
{
    SCROLL_THRESHOLD = 4,
    DIGITS = 2,
    MAXDIGITS = 20,
};


static GtkHBoxClass* parent_class;
static int signals[LAST_SIGNAL];


static void phin_slider_button_class_init               (PhinSliderButtonClass* klass);
static void phin_slider_button_init                     (PhinSliderButton* button);
static void phin_slider_button_destroy                  (GtkObject* object);
static void phin_slider_button_realize                  (GtkWidget* widget);
static void phin_slider_button_unrealize                (GtkWidget* widget);
static void phin_slider_button_map                      (GtkWidget* widget);
static void phin_slider_button_unmap                    (GtkWidget* widget);
static void phin_slider_button_size_allocate            (GtkWidget* widget,
                                                         GtkAllocation* allocation);
static gboolean phin_slider_button_expose               (GtkWidget* widget,
                                                         GdkEventExpose* event);
static void phin_slider_button_adjustment_changed       (GtkAdjustment* adjustment,
                                                         PhinSliderButton* button);
static void phin_slider_button_adjustment_value_changed (GtkAdjustment* adjustment,
                                                         PhinSliderButton* button);
static void phin_slider_button_entry_activate           (GtkEntry* widget,
                                                         PhinSliderButton* button);
static gboolean phin_slider_button_entry_focus_out      (GtkEntry* entry,
                                                         GdkEventFocus* event,
                                                         PhinSliderButton* button);
static gboolean phin_slider_button_entry_key_press      (GtkEntry* entry,
                                                         GdkEventKey* event,
                                                         PhinSliderButton* button);
static gboolean phin_slider_button_button_press         (GtkWidget* widget,
                                                         GdkEventButton* event);
static gboolean phin_slider_button_button_release       (GtkWidget* widget,
                                                         GdkEventButton* event);
static gboolean phin_slider_button_key_press            (GtkWidget* widget,
                                                         GdkEventKey* event);
static gboolean phin_slider_button_scroll               (GtkWidget* widget,
                                                         GdkEventScroll* event);
static gboolean phin_slider_button_enter_notify         (GtkWidget* widget,
                                                         GdkEventCrossing* event);
static gboolean phin_slider_button_leave_notify         (GtkWidget* widget,
                                                         GdkEventCrossing* event);
static gboolean phin_slider_button_motion_notify        (GtkWidget* widget,
                                                         GdkEventMotion* event);

/* internal utility functions */
static int check_pointer (PhinSliderButton* button, int x, int y);
static void update_cursor (PhinSliderButton* button);
static void update_label (PhinSliderButton* button);
static void entry_cancel (PhinSliderButton* button);
static void update_size (PhinSliderButton* button);
static char* value_to_string(PhinSliderButton* button, double value);


GType phin_slider_button_get_type ( )
{
    static GType type = 0;

    if (!type)
    {
        static const GTypeInfo info =
            {
                sizeof (PhinSliderButtonClass),
                NULL,
                NULL,
                (GClassInitFunc) phin_slider_button_class_init,
                NULL,
                NULL,
                sizeof (PhinSliderButton),
                0,
                (GInstanceInitFunc) phin_slider_button_init,
                NULL
            };

        type = g_type_register_static (GTK_TYPE_HBOX,
                                       "PhinSliderButton",
                                       &info,
                                       0);
    }

    return type;
}



/**
 * phin_slider_button_new:
 * @adjustment: the #GtkAdjustment that the new button will use
 * @digits: number of decimal digits to display
 * 
 * Creates a new #PhinSliderButton.
 *
 * Returns: a newly created #PhinSliderButton
 * 
 */
GtkWidget* phin_slider_button_new (GtkAdjustment* adjustment,
                                   int            digits)
{
    PhinSliderButton* button;

    debug ("new\n");

    gdouble adj_lower = gtk_adjustment_get_lower(adjustment);
    gdouble adj_upper = gtk_adjustment_get_upper(adjustment);
    gdouble adj_value = gtk_adjustment_get_value(adjustment);

    g_assert (adj_lower < adj_upper);
    g_assert ((adj_value >= adj_lower)
              && (adj_value <= adj_upper));

    button = g_object_new (PHIN_TYPE_SLIDER_BUTTON, NULL);

    button->digits = (digits >= 0) ? digits : DIGITS;

    if (button->digits > MAXDIGITS)
        button->digits = MAXDIGITS;

    phin_slider_button_set_adjustment (button, adjustment);
     
    return (GtkWidget*) button;
}



/**
 * phin_slider_button_new_with_range:
 * @value: the initial value the new button should have
 * @lower: the lowest value the new button will allow
 * @upper: the highest value the new button will allow
 * @step: increment added or subtracted when sliding
 * @digits: number of decimal digits to display
 *
 * Creates a new #PhinSliderButton.  The slider will create a new
 * #GtkAdjustment from @value, @lower, @upper, and @step.  If these
 * parameters represent a bogus configuration, the program will
 * terminate.
 *
 * Returns: a newly created #PhinSliderButton
 *
 */
GtkWidget* phin_slider_button_new_with_range (double value, double lower,
                                              double upper, double step,
                                              int    digits)
{
    GtkAdjustment* adj;

    adj = (GtkAdjustment*) gtk_adjustment_new (value, lower, upper, step, step, 0);

    return phin_slider_button_new (adj, digits);
}



/**
 * phin_slider_button_set_adjustment:
 * @button: a #PhinSliderButton
 * @adjustment: a #GtkAdjustment
 *
 * Sets the adjustment used by @button.  If @adjustment is %NULL, a
 * new adjustment with a value of zero and a range of [-1.0, 1.0] will
 * be created.
 *
 */
void phin_slider_button_set_adjustment (PhinSliderButton* button,
                                        GtkAdjustment* adjustment)
{
    g_return_if_fail (PHIN_IS_SLIDER_BUTTON (button));
    g_return_if_fail (button->adjustment != adjustment);

    if (!adjustment)
        adjustment = (GtkAdjustment*) gtk_adjustment_new (0.0, -1.0, 1.0, 1.0, 1.0, 0.0);
    else
        g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));

    if (button->adjustment)
    {
        g_signal_handlers_disconnect_by_func (button->adjustment,
                                              phin_slider_button_adjustment_changed,
                                              (gpointer) button);
        g_signal_handlers_disconnect_by_func (button->adjustment,
                                              phin_slider_button_adjustment_value_changed,
                                              (gpointer) button);
        g_object_unref (button->adjustment);
    }

    button->adjustment = adjustment;
    g_object_ref (adjustment);
    g_object_ref_sink (GTK_OBJECT (adjustment));

    g_signal_connect (adjustment, "changed",
                      G_CALLBACK (phin_slider_button_adjustment_changed),
                      (gpointer) button);
    g_signal_connect (adjustment, "value_changed",
                      G_CALLBACK (phin_slider_button_adjustment_value_changed),
                      (gpointer) button);

    phin_slider_button_adjustment_changed (adjustment, button);
    phin_slider_button_adjustment_value_changed (adjustment, button);
}


/**
 * phin_slider_button_get_adjustment:
 * @button: a #PhinSliderButton
 *
 * Retrives the current adjustment in use by @button.
 *
 * Returns: @button's current #GtkAdjustment
 *
 */
GtkAdjustment* phin_slider_button_get_adjustment (PhinSliderButton* button)
{
    g_return_val_if_fail (PHIN_IS_SLIDER_BUTTON (button), NULL);

    if (!button->adjustment)
        phin_slider_button_set_adjustment (button, NULL);

    return button->adjustment;
}



/**
 * phin_slider_button_set_value:
 * @button: a #PhinSliderButton
 * @value: a new value for the button
 * 
 * Sets the current value of the button.  If the value is outside the
 * range of values allowed by @button, it will be clamped.  The button
 * emits the "value-changed" signal if the value changes.
 *
 */
void phin_slider_button_set_value (PhinSliderButton* button, double value)
{
    g_return_if_fail (PHIN_IS_SLIDER_BUTTON (button));

    gdouble adj_lower = gtk_adjustment_get_lower(button->adjustment);
    gdouble adj_upper = gtk_adjustment_get_upper(button->adjustment);

    value = CLAMP (value, adj_lower, adj_upper);

    gtk_adjustment_set_value (button->adjustment, value);
}



/**
 * phin_slider_button_get_value:
 * @button: a #PhinSliderButton
 *
 * Retrieves the current value of the button.
 *
 * Returns: current value of the button
 *
 */
double phin_slider_button_get_value (PhinSliderButton* button)
{
    g_return_val_if_fail (PHIN_IS_SLIDER_BUTTON (button), 0);

    return gtk_adjustment_get_value(button->adjustment);
}



/**
 * phin_slider_button_set_range:
 * @button: a #PhinSliderButton
 * @lower: lowest allowable value
 * @upper: highest allowable value
 * 
 * Sets the range of allowable values for the button, and clamps the
 * button's current value to be between @lower and @upper.
 */
void phin_slider_button_set_range (PhinSliderButton* button,
                                   double lower, double upper)
{
    double value;
     
    g_return_if_fail (PHIN_IS_SLIDER_BUTTON (button));
    g_return_if_fail (lower <= upper);

    gtk_adjustment_set_lower(button->adjustment, lower);
    gtk_adjustment_set_upper(button->adjustment, upper);

    value = CLAMP (gtk_adjustment_get_value(button->adjustment),
                   gtk_adjustment_get_lower(button->adjustment), /* ott? */
                   gtk_adjustment_get_upper(button->adjustment));

    gtk_adjustment_changed (button->adjustment);
    gtk_adjustment_set_value (button->adjustment, value);
}



/**
 * phin_slider_button_get_range:
 * @button: a #PhinSliderButton
 * @lower: retrieves lowest allowable value
 * @upper: retrieves highest allowable value
 *
 * Places the range of allowable values for @button into @lower
 * and @upper.  Either variable may be set to %NULL if you are not
 * interested in its value.
 *
 */
void phin_slider_button_get_range (PhinSliderButton* button,
                                   double* lower, double* upper)
{
    g_return_if_fail (PHIN_IS_SLIDER_BUTTON (button));

    if (lower)
        *lower = gtk_adjustment_get_lower(button->adjustment);
    if (upper)
        *upper = gtk_adjustment_get_upper(button->adjustment);
}



/**
 * phin_slider_button_set_increment:
 * @button: a #PhinSliderButton
 * @step: step increment value
 * @page: page increment value
 *
 * Sets the increments the button should use.
 *
 */
void phin_slider_button_set_increment (PhinSliderButton* button,
                                       double step, double page)
{
    g_return_if_fail (PHIN_IS_SLIDER_BUTTON (button));

    gtk_adjustment_set_step_increment(button->adjustment, step);
    gtk_adjustment_set_page_increment(button->adjustment, page);
}



/**
 * phin_slider_button_get_increment:
 * @button: a #PhinSliderButton
 * @step: retrieves step increment value
 * @page: retrieves page increment value
 *
 * Places the button's increment values into @step and @page.  Either
 * variable may be set to %NULL if you are not interested in its
 * value.
 *
 */
void phin_slider_button_get_increment (PhinSliderButton* button,
                                       double* step, double* page)
{
    g_return_if_fail (PHIN_IS_SLIDER_BUTTON (button));

    if (step)
        *step = gtk_adjustment_get_step_increment(button->adjustment);
    if (page)
        *page = gtk_adjustment_get_page_increment(button->adjustment);
}



/**
 * phin_slider_button_set_format:
 * @button: a #PhinSliderButton
 * @digits: number of decimal digits to display
 * @prefix: text to prepend to number
 * @postfix: text to append to number
 *
 * Sets the way @button renders it's label.  If the first character in
 * either @prefix or @postfix is '\0' the corresponding parameter will
 * be unset.  If you don't want to adjust @digits, set it to a
 * negative value.  If you don't want to adjust @prefix and/or
 * @postfix, set them to %NULL.
 *
 */
void phin_slider_button_set_format (PhinSliderButton* button,
                                    int         digits,
                                    const char* prefix,
                                    const char* postfix)
{
    g_return_if_fail (PHIN_IS_SLIDER_BUTTON (button));

    if (digits >= 0)
    {
        button->digits = digits;

        if (button->digits > MAXDIGITS)
            button->digits = MAXDIGITS;
    }

    if (prefix)
    {
        g_free (button->prefix);

        if (*prefix == '\0')
        {
            button->prefix = NULL;
        }
        else
        {
            button->prefix = g_strdup (prefix);
        }
    }

    if (postfix)
    {
        g_free (button->postfix);

        if (*postfix == '\0')
        {
            button->postfix = NULL;
        }
        else
        {
            button->postfix = g_strdup (postfix);
        }
    }

    update_size (button);
    update_label (button);
}



/**
 * phin_slider_button_get_format:
 * @button: a #PhinSliderButton
 * @digits: retrieves the number of decimal digits to display
 * @prefix: retrieves text prepended to number
 * @postfix: retrieves text appended to number
 *
 * Retrieves the information @button uses to create its label.  The
 * value returned will point to the button's local copy, so don't
 * write to it.  Set the pointers for any value you aren't interested
 * in to %NULL.
 * 
 */
void phin_slider_button_get_format (PhinSliderButton* button,
                                    int*   digits,
                                    char** prefix,
                                    char** postfix)
{
    g_return_if_fail (PHIN_IS_SLIDER_BUTTON (button));

    if (digits)
        *digits = button->digits;

    if (prefix)
        *prefix = button->prefix;

    if (postfix)
        *postfix = button->postfix;
}


/**
 * phin_slider_button_set_threshold:
 * @button: a #PhinSliderButton
 * @threshold: an unsigned int >= 1
 *
 * Sets the threshold for @button.  The threshold is how far the user
 * has to move the mouse to effect a change when sliding.
 * 
 */
void phin_slider_button_set_threshold (PhinSliderButton* button,
                                       guint threshold)
{
    g_return_if_fail (PHIN_IS_SLIDER_BUTTON (button));
    g_return_if_fail (threshold != 0);

    button->threshold = threshold;
}


/**
 * phin_slider_button_get_threshold:
 * @button: a #PhinSliderButton
 *
 * Retrieves the threshold for @button
 *
 * Returns: the threshold for @button, or -1 if @button is invalid
 * 
 */
int phin_slider_button_get_threshold (PhinSliderButton* button)
{
    g_return_val_if_fail (PHIN_IS_SLIDER_BUTTON (button), -1);

    return button->threshold;
}


static void phin_slider_button_class_init (PhinSliderButtonClass* klass)
{
    GtkObjectClass* object_class = (GtkObjectClass*) klass;
    GtkWidgetClass* widget_class = (GtkWidgetClass*) klass;

    //debug ("class init\n");
     
    parent_class = g_type_class_peek (gtk_hbox_get_type ());

    object_class->destroy = phin_slider_button_destroy;

    widget_class->realize = phin_slider_button_realize;
    widget_class->unrealize = phin_slider_button_unrealize;
    widget_class->map = phin_slider_button_map;
    widget_class->unmap = phin_slider_button_unmap;
    widget_class->size_allocate = phin_slider_button_size_allocate;
    widget_class->expose_event = phin_slider_button_expose;
    widget_class->button_press_event = phin_slider_button_button_press;
    widget_class->button_release_event = phin_slider_button_button_release;
    widget_class->key_press_event = phin_slider_button_key_press;
    widget_class->scroll_event = phin_slider_button_scroll;
    widget_class->enter_notify_event = phin_slider_button_enter_notify;
    widget_class->leave_notify_event = phin_slider_button_leave_notify;
    widget_class->motion_notify_event = phin_slider_button_motion_notify;

    /**
     * PhinSliderButton::value-changed:
     * @button: the object on which the signal was emitted
     *
     * The "value-changed" signal is emitted when the value of the
     * button's adjustment changes.
     *
     */
    signals[VALUE_CHANGED_SIGNAL] =
        g_signal_new ("value-changed",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (PhinSliderButtonClass, value_changed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

    /**
     * PhinSliderButton::changed:
     * @button: the object on which the signal was emitted
     *
     * The "changed" signal is emitted when any parameter of the
     * slider's adjustment changes, except for the %value parameter.
     *
     */
    signals[CHANGED_SIGNAL] =
        g_signal_new ("changed",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
                      G_STRUCT_OFFSET (PhinSliderButtonClass, changed),
                      NULL, NULL,
                      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

     
    klass->value_changed = NULL;
    klass->changed = NULL;
}



static void phin_slider_button_init (PhinSliderButton* button)
{
    //debug ("init\n");

    GtkBox* box = GTK_BOX (button);
    GtkWidget* widget = GTK_WIDGET (button);
    GtkContainer* container = GTK_CONTAINER (button);
    int focus_width, focus_pad;
    GtkStyle* style;

    gtk_widget_set_can_focus(widget, TRUE);
     
    /* our parent class sets this to false; we need it to be true so
     * that we are drawn without glitches when first shown
     * (mapped) */
    gtk_widget_set_redraw_on_allocate (GTK_WIDGET (box), TRUE);
     
    button->arrow_cursor = NULL;
    button->empty_cursor = NULL;
    button->event_window = NULL;
    button->left_arrow = gtk_arrow_new (GTK_ARROW_LEFT, GTK_SHADOW_NONE);
    button->right_arrow = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_NONE);
    button->label = gtk_label_new (NULL);
    button->prefix_label = NULL;
    button->postfix_label = NULL;
    button->entry = gtk_entry_new ( );
    button->adjustment = NULL;
    button->prefix = NULL;
    button->postfix = NULL;
    button->digits = DIGITS;
    button->hilite = 0;
    button->state = STATE_NORMAL;
    button->xpress = 0;
    button->ypress = 0;
    button->xpress_root = 0;
    button->ypress_root = 0;
    button->threshold = 3;
    button->slid = FALSE;
    button->firstrun = 1;
     
    gtk_box_pack_start (box, button->left_arrow, FALSE, FALSE, 0);
    gtk_box_pack_start (box, button->label, TRUE, TRUE, 0);
    gtk_box_pack_start (box, button->entry, TRUE, TRUE, 0);
    gtk_box_pack_start (box, button->right_arrow, FALSE, FALSE, 0);

    gtk_widget_style_get (widget,
                          "focus-line-width", &focus_width,
                          "focus-padding", &focus_pad,
                          NULL);

    style = gtk_widget_get_style(widget);

    gtk_container_set_border_width (container,
                                    MAX (MAX (style->xthickness,
                                              style->ythickness),
                                         focus_width + focus_pad));

    gtk_entry_set_has_frame (GTK_ENTRY (button->entry), FALSE);

    gtk_entry_set_alignment (GTK_ENTRY (button->entry), 0.5);

    g_signal_connect (G_OBJECT (button->entry), "activate",
                      G_CALLBACK (phin_slider_button_entry_activate), (gpointer) button);
    g_signal_connect (G_OBJECT (button->entry), "focus-out-event",
                      G_CALLBACK (phin_slider_button_entry_focus_out), (gpointer) button);
    g_signal_connect (G_OBJECT (button->entry), "key-press-event",
                      G_CALLBACK (phin_slider_button_entry_key_press), (gpointer) button);

    gtk_misc_set_alignment (GTK_MISC (button->left_arrow), 0.5, 0.5);
    gtk_misc_set_alignment (GTK_MISC (button->right_arrow), 0.5, 0.5);
       
}



static void phin_slider_button_destroy (GtkObject* object)
{
    GtkObjectClass* klass;
    PhinSliderButton* button;
    GtkWidget* widget;
     
    //debug ("destroy %p\n", object);
     
    g_return_if_fail (object != NULL);
    g_return_if_fail (PHIN_IS_SLIDER_BUTTON (object));

    klass = GTK_OBJECT_CLASS (parent_class);
    button = (PhinSliderButton*) object;
    widget = GTK_WIDGET (object);

    if (button->arrow_cursor)
    {
        gdk_cursor_unref (button->arrow_cursor);
        button->arrow_cursor = NULL;
    }

    if (button->empty_cursor)
    {
        gdk_cursor_unref (button->empty_cursor);
        button->empty_cursor = NULL;
    }

    if (button->event_window)
    {
        gdk_window_set_user_data (button->event_window, NULL);
        gdk_window_destroy (button->event_window);
        button->event_window = NULL;
    }

    if (button->left_arrow)
    {
        gtk_widget_destroy (button->left_arrow);
        button->left_arrow = NULL;
    }

    if (button->right_arrow)
    {
        gtk_widget_destroy (button->right_arrow);
        button->right_arrow = NULL;
    }
     
    if (button->label)
    {
        gtk_widget_destroy (button->label);
        button->label = NULL;
    }

    if (button->prefix_label)
    {
        gtk_widget_destroy (button->prefix_label);
        button->prefix_label = NULL;
    }

    if (button->postfix_label)
    {
        gtk_widget_destroy (button->postfix_label);
        button->postfix_label = NULL;
    }

    if (button->entry)
    {
        gtk_widget_destroy (button->entry);
        button->entry = NULL;
    }

    if (button->adjustment)
    {
        g_signal_handlers_disconnect_by_func (button->adjustment,
                                              phin_slider_button_adjustment_changed,
                                              (gpointer) button);
        g_signal_handlers_disconnect_by_func (button->adjustment,
                                              phin_slider_button_adjustment_value_changed,
                                              (gpointer) button);
        g_object_unref (button->adjustment);
        button->adjustment = NULL;
    }

    if (button->prefix)
    {
        g_free (button->prefix);
        button->prefix = NULL;
    }

    if (button->postfix)
    {
        g_free (button->postfix);
        button->postfix = NULL;
    }
     
    if (klass->destroy)
        klass->destroy (object);
}



static void phin_slider_button_realize (GtkWidget* widget)
{
    GtkWidgetClass* klass = GTK_WIDGET_CLASS (parent_class);
    PhinSliderButton* button = (PhinSliderButton*) widget;
    GdkWindowAttr attributes;
    int attributes_mask;
    GtkAllocation widget_alloc;
     
    debug ("realize\n");
     
    g_return_if_fail (widget != NULL);
    g_return_if_fail (PHIN_IS_SLIDER_BUTTON (widget));

    if (klass->realize)
        klass->realize (widget);

    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.wclass      = GDK_INPUT_ONLY;
    attributes.event_mask  = gtk_widget_get_events (widget);
    attributes.event_mask |= (GDK_BUTTON_PRESS_MASK
                              | GDK_BUTTON_RELEASE_MASK
                              | GDK_POINTER_MOTION_MASK
                              | GDK_POINTER_MOTION_HINT_MASK
                              | GDK_ENTER_NOTIFY_MASK
                              | GDK_LEAVE_NOTIFY_MASK
                              | GDK_SCROLL_MASK
                              | GDK_KEY_PRESS_MASK);

    gtk_widget_get_allocation(widget, &widget_alloc);

    attributes.x = widget_alloc.x;
    attributes.y = widget_alloc.y;
    attributes.width = widget_alloc.width;
    attributes.height = widget_alloc.height;
    attributes_mask = GDK_WA_X | GDK_WA_Y;

    button->event_window = gdk_window_new (gtk_widget_get_parent_window (widget),
                                           &attributes, attributes_mask);
    gdk_window_set_user_data (button->event_window, widget);

    button->arrow_cursor = gdk_cursor_new (GDK_SB_H_DOUBLE_ARROW);
    button->empty_cursor = gdk_cursor_new (GDK_BLANK_CURSOR);
}



static void phin_slider_button_unrealize (GtkWidget *widget)
{
    PhinSliderButton* button = PHIN_SLIDER_BUTTON (widget);
    GtkWidgetClass* klass = GTK_WIDGET_CLASS (parent_class);

    debug ("unrealize\n");
     
    gdk_cursor_unref (button->arrow_cursor);
    button->arrow_cursor = NULL;

    gdk_cursor_unref (button->empty_cursor);
    button->empty_cursor = NULL;

    gdk_window_set_user_data (button->event_window, NULL);
    gdk_window_destroy (button->event_window);
    button->event_window = NULL;
     
    if (klass->unrealize)
        klass->unrealize (widget);
}


static void phin_slider_button_map (GtkWidget *widget)
{
    PhinSliderButton* button;

    //debug ("map\n");
     
    g_return_if_fail (PHIN_IS_SLIDER_BUTTON (widget));
    button = (PhinSliderButton*) widget;

    gtk_widget_show (button->left_arrow);
    gtk_widget_show (button->label);
    gtk_widget_show (button->right_arrow);
    gdk_window_show (button->event_window);
     
    if (button->prefix_label)
        gtk_widget_show (button->prefix_label);
    if (button->postfix_label)
        gtk_widget_show (button->postfix_label);

    GTK_WIDGET_CLASS (parent_class)->map (widget);

    gtk_widget_queue_draw (widget);
}


static void phin_slider_button_unmap (GtkWidget *widget)
{
    PhinSliderButton* button;

    //debug ("unmap\n");
     
    g_return_if_fail (PHIN_IS_SLIDER_BUTTON (widget));
    button = (PhinSliderButton*) widget;

    gtk_widget_hide (button->left_arrow);
    gtk_widget_hide (button->label);
    gtk_widget_hide (button->right_arrow);
    gdk_window_hide (button->event_window);
     
    if (button->prefix_label)
        gtk_widget_hide (button->prefix_label);
    if (button->postfix_label)
        gtk_widget_hide (button->postfix_label);
     
    GTK_WIDGET_CLASS (parent_class)->unmap (widget);
}


static void phin_slider_button_size_allocate (GtkWidget* widget,
                                              GtkAllocation* allocation)
{
    g_return_if_fail (widget != NULL);
    g_return_if_fail (allocation != NULL);
    g_return_if_fail (PHIN_IS_SLIDER_BUTTON (widget));

    debug ("size allocate\n");
     
    GTK_WIDGET_CLASS (parent_class)->size_allocate (widget, allocation);

    if (gtk_widget_get_realized (widget))
    {
        gdk_window_move_resize (PHIN_SLIDER_BUTTON (widget)->event_window,
                                allocation->x,
                                allocation->y,
                                allocation->width,
                                allocation->height);
        //make sure entry is hidden at start
        if(PHIN_SLIDER_BUTTON (widget)->firstrun)
        {
            gtk_widget_hide (PHIN_SLIDER_BUTTON (widget)->entry);
            PHIN_SLIDER_BUTTON (widget)->firstrun = 0;
        }
    }
}

static gboolean phin_slider_button_expose (GtkWidget*      widget,
                                           GdkEventExpose* event)
{
    PhinSliderButton* button;

    /* <GRRRRRRRRRRRR> */
    GtkAllocation a;
    GtkAllocation arrow_a;
    GtkAllocation label_a;
    /* </GRRRRRRRRRRRR> */

    GtkStyle* style;
    GdkWindow* window;
    int p;                      /* pad */
     
    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (PHIN_IS_SLIDER_BUTTON (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);
    g_return_val_if_fail (gtk_widget_is_drawable (widget), FALSE);
    g_return_val_if_fail (event->count == 0, FALSE);

    //debug ("expose\n");

    button = PHIN_SLIDER_BUTTON (widget);
    gtk_widget_get_allocation(widget, &a);
    gtk_widget_get_allocation(button->left_arrow, &arrow_a);
    gtk_widget_get_allocation(button->label, &label_a);

    style = gtk_widget_get_style(widget);
    window = gtk_widget_get_window(widget);

    p = gtk_container_get_border_width (GTK_CONTAINER (widget));

    /* clear the box */
    gtk_paint_box (style, window,
                   GTK_STATE_NORMAL,
                   GTK_SHADOW_NONE,
                   NULL,
                   widget,
                   NULL,        /* if this is "buttondefault" the
                                 * smooth engine crashes */
                   a.x, a.y, a.width, a.height);

    /* paint any applicable hilites */
    if (button->state == STATE_NORMAL)
    {
        if (button->hilite == LEFT_ARROW)
        {
            gtk_paint_box (style, window,
                           GTK_STATE_PRELIGHT,
                           GTK_SHADOW_NONE,
                           NULL,
                           widget,
                           "button",
                           a.x, a.y,
                           arrow_a.width + p,
                           a.height);
        }
        else if (button->hilite == RIGHT_ARROW)
        {
            int offset;
            int width;

            offset = (a.x + arrow_a.width + label_a.width + p);

            width = a.x + a.width - offset;
               
            if (button->prefix_label)
            {
                GtkAllocation pre_a;
                gtk_widget_get_allocation(button->prefix_label, &pre_a);
                offset += pre_a.width;
            }

            if (button->postfix_label)
            {
                GtkAllocation post_a;
                gtk_widget_get_allocation(button->postfix_label, &post_a);
                offset += post_a.width;
            }
               
            gtk_paint_box (style, window,
                           GTK_STATE_PRELIGHT,
                           GTK_SHADOW_NONE,
                           NULL,
                           widget, "button",
                           offset, a.y, width, a.height);
        }
        else if (button->hilite == LABEL)
        {
            int offset;
            int width;

            offset = a.x + arrow_a.width + p;

            width = label_a.width;

            if (button->prefix_label)
            {
                GtkAllocation pre_a;
                gtk_widget_get_allocation(button->prefix_label, &pre_a);
                width += pre_a.width;
            }

            if (button->postfix_label)
            {
                GtkAllocation post_a;
                gtk_widget_get_allocation(button->prefix_label, &post_a);
                width += post_a.width;
            }

            gtk_paint_box (style, window,
                           GTK_STATE_PRELIGHT,
                           GTK_SHADOW_NONE,
                           NULL,
                           widget, "button",
                           offset, a.y, width, a.height);
        }
    }
     
    /* paint our border */
    gtk_paint_shadow (style, window,
                      GTK_STATE_NORMAL,
                      GTK_SHADOW_OUT,
                      NULL,
                      widget,
                      "buttondefault",
                      a.x, a.y, a.width, a.height);

    gtk_paint_shadow (style, window,
                      GTK_STATE_NORMAL,
                      GTK_SHADOW_OUT,
                      NULL,
                      widget,
                      "button",
                      a.x, a.y, a.width, a.height);

    /* paint the focus if we have it */
    if (gtk_widget_get_can_focus (widget))
    {
        int x, y;
        int width, height;

        x = a.x;
        y = a.y;
        width = a.width;
        height = a.height;
          
        x += p;
        y += p;
        width -= 2*p;
        height -= 2*p;

        gtk_paint_focus (style, window, gtk_widget_get_state (widget),
                         NULL, widget, "button",
                         x, y, width, height);
    }

    GTK_WIDGET_CLASS (parent_class)->expose_event (widget, event);
     
    return FALSE;
}



static void phin_slider_button_adjustment_changed (GtkAdjustment* adjustment,
                                                   PhinSliderButton* button)
{
    g_return_if_fail (PHIN_IS_SLIDER_BUTTON (button));

    //debug ("adjustment changed\n");
     
    update_size (button);
    update_label (button);
     
    g_signal_emit (G_OBJECT (button), signals[CHANGED_SIGNAL], 0);
}


static void phin_slider_button_adjustment_value_changed (GtkAdjustment* adjustment,
                                                         PhinSliderButton* button)
{
    g_return_if_fail (PHIN_IS_SLIDER_BUTTON (button));

    //debug ("adjustment value changed\n");
     
    update_label (button);

    g_signal_emit (G_OBJECT (button), signals[VALUE_CHANGED_SIGNAL], 0);
}


static void phin_slider_button_entry_activate (GtkEntry* entry,
                                               PhinSliderButton* button)
{
    double newval;

    debug ("entry activate\n");
     
    button->state = STATE_NORMAL;
     
    newval = g_strtod (gtk_entry_get_text (GTK_ENTRY (entry)), NULL);

    gtk_adjustment_set_value (button->adjustment, newval);

    gtk_widget_hide (GTK_WIDGET (entry));
    gtk_widget_show (button->label);
    gtk_widget_queue_draw (GTK_WIDGET (button));
}


static gboolean phin_slider_button_entry_focus_out (GtkEntry* entry,
                                                    GdkEventFocus* event,
                                                    PhinSliderButton* button)
{
    GtkWidget* widget = GTK_WIDGET (button);

    debug ("entry focus out\n");

    entry_cancel (button);
    button->hilite = 0;

    /* check to see if the pointer wandered outside our toplevel; if
     * it did, we grab the focus so that a widget has the focus when
     * our toplevel gets the focus back */
    if (gtk_widget_is_toplevel (widget))
    {
        GtkWidget* toplevel = gtk_widget_get_toplevel (widget);
        GdkScreen* screen = gtk_window_get_screen (GTK_WINDOW (toplevel));
        GdkDisplay* display = gdk_screen_get_display (screen);
        GdkWindow* curwindow = gdk_display_get_window_at_pointer (display, NULL, NULL);

        if (curwindow)
        {
            curwindow = gdk_window_get_toplevel (curwindow);
            GdkWindow* topwin = gtk_widget_get_window(toplevel);

            if (curwindow != topwin)
            {
                debug ("%p, %p\n", curwindow, topwin);
                gtk_widget_grab_focus (widget);
            }
        }
        else
        {
            gtk_widget_grab_focus (widget);
        }
    }

    return FALSE;
}

          
static gboolean phin_slider_button_entry_key_press (GtkEntry* entry,
                                                    GdkEventKey* event,
                                                    PhinSliderButton* button)
{
    debug ("entry key press\n");
     
    if (event->keyval == GDK_Escape)
    {
        entry_cancel (button);
        gtk_widget_grab_focus (GTK_WIDGET (button));
    }

    return FALSE;
}


static gboolean phin_slider_button_button_press (GtkWidget* widget,
                                                 GdkEventButton* event)
{
    PhinSliderButton* button = PHIN_SLIDER_BUTTON (widget);

    debug ("button press\n");
     
    if (event->type != GDK_BUTTON_PRESS)
        return FALSE;

    if(event->button == 1)
    {
        double adj_value;
        double step_inc;

        switch (button->state)
        {
        case STATE_NORMAL:
            button->xpress = event->x;
            button->ypress = event->y;
            button->xpress_root = event->x_root;
            button->ypress_root = event->y_root;
            button->slid = FALSE;
            adj_value = gtk_adjustment_get_value(button->adjustment);
            step_inc = gtk_adjustment_get_step_increment(button->adjustment);

            if (button->hilite == LEFT_ARROW)
            {
                button->state = STATE_PRESSED;
         
                gtk_adjustment_set_value (button->adjustment, (adj_value - step_inc));
            }
            else if (button->hilite == RIGHT_ARROW)
            {
                button->state = STATE_PRESSED;
         
                gtk_adjustment_set_value (button->adjustment, (adj_value + step_inc));
            }
            else
            {
                button->state = STATE_SLIDE;
            }
            break;
        case STATE_SCROLL:
            button->state = STATE_NORMAL;
            update_cursor (button);
            break;
        case STATE_ENTRY:
            entry_cancel (button);
            button->state = STATE_NORMAL;
            update_cursor (button);
            break;
        }
        
        update_cursor (button);
        gtk_widget_grab_focus (widget);
        gtk_widget_queue_draw (widget);
    }
    return FALSE;
}
          

static gboolean phin_slider_button_button_release (GtkWidget* widget,
                                                   GdkEventButton* event)
{
    PhinSliderButton* button = PHIN_SLIDER_BUTTON (widget);
    char* s;
    char* t;

    debug ("button release\n");
    /* react to left button only */
    if(event->button == 1)
    {   
        switch (button->state)
        {
        case STATE_SLIDE:
            if (!button->slid)
            {
                double adj_value = gtk_adjustment_get_value(button->adjustment);
                button->state = STATE_ENTRY;
                s = value_to_string(button, adj_value);

                for (t = s; *t == ' '; t++);
                gtk_entry_set_text (GTK_ENTRY (button->entry), t);
                g_free (s);
                s = t = NULL;
                                  
                gtk_widget_hide (button->label);
                if (button->prefix_label)
                    gtk_widget_hide (button->prefix_label);
                if (button->postfix_label)
                    gtk_widget_hide (button->postfix_label);
                   
                gtk_widget_show (button->entry);

                gtk_widget_grab_focus (button->entry);
            }
            else
            {
                button->state = STATE_NORMAL;
                phin_warp_pointer (event->x_root, event->y_root,
                                   button->xpress_root, button->ypress_root);
                update_cursor (button);
            }
            break;
        case STATE_PRESSED:
            button->state = STATE_NORMAL;
            update_cursor (button);
            break;
        }

        gtk_widget_queue_draw (widget);
    }
    return FALSE;
}


static gboolean phin_slider_button_key_press (GtkWidget* widget,
                                              GdkEventKey* event)
{
    PhinSliderButton* button = PHIN_SLIDER_BUTTON (widget);
    GtkAdjustment* adj = button->adjustment;
    double val = gtk_adjustment_get_value(adj);

    //debug ("key press\n");

    switch (event->keyval)
    {
    case GDK_Up:
        gtk_adjustment_set_value (adj, val + gtk_adjustment_get_step_increment(adj));
        break;
    case GDK_Down:
        gtk_adjustment_set_value (adj, val - gtk_adjustment_get_step_increment(adj));
        break;
    case GDK_Page_Up:
        gtk_adjustment_set_value (adj, val + gtk_adjustment_get_page_increment(adj));
        break;
    case GDK_Page_Down:
        gtk_adjustment_set_value (adj, val - gtk_adjustment_get_page_increment(adj));
        break;
    default:
        return FALSE;
    }

    return TRUE;
}
     
static gboolean phin_slider_button_scroll (GtkWidget* widget,
                                           GdkEventScroll* event)
{
    PhinSliderButton* button = PHIN_SLIDER_BUTTON (widget);
    GtkAdjustment* adj;
    double val, inc;

    //debug ("scroll\n");
     
    if (button->state != STATE_NORMAL
        && button->state != STATE_SCROLL)
        return FALSE;
     
    button->state = STATE_SCROLL;
    update_cursor (button);

    button->xpress_root = event->x_root;
    button->ypress_root = event->y_root;
    button->xpress = event->x;
    button->ypress = event->y;

    adj = button->adjustment;
    val = gtk_adjustment_get_value(adj);
    inc = gtk_adjustment_get_page_increment(adj);

    if (event->direction == GDK_SCROLL_UP
        || event->direction == GDK_SCROLL_RIGHT)
    {
        gtk_adjustment_set_value (adj, val + inc);
    }
    else
    {
        gtk_adjustment_set_value (adj, val - inc);
    }

    gtk_widget_grab_focus (widget);
    return FALSE;
}


static gboolean phin_slider_button_enter_notify (GtkWidget* widget,
                                                 GdkEventCrossing* event)
{
    PhinSliderButton* button = PHIN_SLIDER_BUTTON (widget);
    int old = button->hilite;

    //debug ("enter\n");

    button->hilite = check_pointer (button, event->x, event->y);

    if (button->hilite != old)
    {
        update_cursor (button);
        gtk_widget_queue_draw (widget);
    }
     
    return FALSE;
}


static gboolean phin_slider_button_leave_notify (GtkWidget* widget,
                                                 GdkEventCrossing* event)
{
    PhinSliderButton* button = PHIN_SLIDER_BUTTON (widget);

    //debug ("leave\n");
     
    button->hilite = 0;
    if (button->state == STATE_SCROLL)
        button->state = STATE_NORMAL;
    update_cursor (button);
    gtk_widget_queue_draw (widget);

    return FALSE;
}


static gboolean phin_slider_button_motion_notify (GtkWidget* widget,
                                                  GdkEventMotion* event)
{
    PhinSliderButton* button = PHIN_SLIDER_BUTTON (widget);
    int old = button->hilite;
    int xdiff = 0;
    double inc;
     
    //debug ("motion\n");
     
    button->hilite = check_pointer (button, event->x, event->y);
     
    if (button->hilite != old)
    {
        update_cursor (button);
        gtk_widget_queue_draw (widget);
    }

    xdiff = (event->x - button->xpress) / button->threshold;

    old = button->state;
    if (ABS (xdiff) >= 1)
    {
        switch (button->state)
        {
        case STATE_SLIDE:
            button->slid = TRUE;

            inc = gtk_adjustment_get_step_increment(button->adjustment) * xdiff;

            gtk_adjustment_set_value (button->adjustment,
                        gtk_adjustment_get_value(button->adjustment) + inc);

            phin_warp_pointer (event->x_root, event->y_root,
                               button->xpress_root, button->ypress_root);
            break;
        case STATE_PRESSED:
            button->state = STATE_SLIDE;

            button->xpress_root = event->x_root;
            button->ypress_root = event->y_root;
            button->xpress = event->x;
            button->ypress = event->y;
               
            update_cursor (button);
            break;
        case STATE_SCROLL:
            button->state = STATE_NORMAL;
            update_cursor (button);
            break;
        }
    }
    else if (button->state == STATE_SCROLL
             && (ABS (event->x - button->xpress) >= SCROLL_THRESHOLD
                 || ABS (event->y - button->ypress) >= SCROLL_THRESHOLD))
    {
        button->state = STATE_NORMAL;
        update_cursor (button);
    }

    if (button->state != old)
        gtk_widget_queue_draw (widget);
     
    /* signal that we want more events */
    gdk_window_get_pointer (NULL, NULL, NULL, NULL);
     
    return FALSE;
}


static int check_pointer (PhinSliderButton* button, int x, int y)
{
    GtkWidget* widget = GTK_WIDGET (button);
    int p = gtk_container_get_border_width (GTK_CONTAINER (button));
    int val;
    GtkAllocation a;
    GtkAllocation la;
    GtkAllocation ra;

    gtk_widget_get_allocation(widget, &a);
    gtk_widget_get_allocation(button->left_arrow, &la);
    gtk_widget_get_allocation(button->right_arrow, &ra);

    if ((y < 0 || y > a.height) || (x < 0 || x > a.width))
    {
        val = 0;
    }
    else if (x <= la.width + p)
    {
        val = LEFT_ARROW;
    }
    else if (x >= (a.width - ra.width - p))
    {
        val = RIGHT_ARROW;
    }
    else
    {
        val = LABEL;
    }

    return val;
}     


static void entry_cancel (PhinSliderButton* button)
{
    button->state = STATE_NORMAL;


    g_signal_handlers_block_by_func (button->entry,
                                     phin_slider_button_entry_focus_out,
                                     button);
    gtk_widget_hide (button->entry);
    g_signal_handlers_unblock_by_func (button->entry,
                                       phin_slider_button_entry_focus_out,
                                       button);
    gtk_widget_show (button->label);

    if (button->prefix_label)
        gtk_widget_show (button->prefix_label);
    if (button->postfix_label)
        gtk_widget_show (button->postfix_label);
     
    gtk_widget_queue_draw (GTK_WIDGET (button));
}     


static void update_cursor (PhinSliderButton* button)
{
    switch (button->state)
    {
    case STATE_ENTRY:
    case STATE_PRESSED:
        gdk_window_set_cursor (button->event_window, NULL);
        break;
    case STATE_SLIDE:
    case STATE_SCROLL:
        gdk_window_set_cursor (button->event_window, button->empty_cursor);
        break;
    default:
        if (button->hilite == LABEL)
        {
            gdk_window_set_cursor (button->event_window, button->arrow_cursor);
        }
        else
        {
            gdk_window_set_cursor (button->event_window, NULL);
        }
        break;
    }
}


static void update_label (PhinSliderButton* button)
{
    char* s;

    s = value_to_string (button, gtk_adjustment_get_value(button->adjustment));

    gtk_label_set_text (GTK_LABEL (button->label), s);
    gtk_widget_queue_draw (GTK_WIDGET (button)); /* just to be safe */

    g_free (s);
}


/* Update the sizes and existence of the labels we use, depending on
 * whether button->prefix and/or button->postfix are set.
 */
static void update_size (PhinSliderButton* button)
{
    GtkRequisition req;
    GString* s = g_string_new (NULL);
    GString* t = g_string_new (NULL);
    char* u;
    int label_len = 0;
    int prefix_len = 0;
    int postfix_len = 0;
/*  double label_frac = 0; */
    double prefix_frac = 0;
    double postfix_frac = 0;
    int width = 0;
    int width_avail = 0;
    int width_alloc = 0;


    u = value_to_string(button, gtk_adjustment_get_lower(button->adjustment));
    s = g_string_new(u);
    g_free(u);
    
    u = value_to_string(button, gtk_adjustment_get_upper(button->adjustment));
    t = g_string_new(u);
    g_free(u);
    
    label_len = MAX (s->len, t->len);

    if (button->prefix)
        prefix_len = strlen (button->prefix);

    if (button->postfix)
        postfix_len = strlen (button->postfix);

    width = label_len + prefix_len + postfix_len;
    gtk_entry_set_width_chars (GTK_ENTRY (button->entry), width);

    gtk_widget_size_request (button->entry, &req);
     
/*  label_frac = label_len * 1.0 / width; */
    prefix_frac = prefix_len * 1.0 / width;
    postfix_frac = postfix_len * 1.0 / width;
    width_avail = req.width;

    if (button->prefix)
    {
        if (!button->prefix_label)
        {
            button->prefix_label = gtk_label_new (NULL);
            gtk_misc_set_alignment(GTK_MISC(button->prefix_label), 0.0, 0.5);
            gtk_box_pack_start (GTK_BOX (button), button->prefix_label,
                                TRUE, TRUE, 0);
            gtk_box_reorder_child (GTK_BOX (button), button->prefix_label, 1);

            if (gtk_widget_get_mapped (GTK_WIDGET (button)))
                gtk_widget_show (button->prefix_label);
        }
          
        gtk_label_set_text (GTK_LABEL (button->prefix_label),
                            button->prefix);

        width_alloc = req.width * prefix_frac;
        gtk_widget_set_size_request (button->prefix_label,
                                     width_alloc,
                                     req.height);
        width_avail -= width_alloc;
    }
    else if (button->prefix_label)
    {
        gtk_widget_destroy (button->prefix_label);
        button->prefix_label = NULL;
    }

    if (button->postfix)
    {
        if (!button->postfix_label)
        {
            button->postfix_label = gtk_label_new (NULL);
            gtk_misc_set_alignment(GTK_MISC(button->postfix_label), 1.0, 0.5);
            gtk_box_pack_start (GTK_BOX (button), button->postfix_label,
                                TRUE, TRUE, 0);
            gtk_box_reorder_child (GTK_BOX (button), button->postfix_label,
                                   (button->prefix_label)? 3: 2);

            if (gtk_widget_get_mapped (GTK_WIDGET (button)))
                gtk_widget_show (button->postfix_label);
        }
          
        gtk_label_set_text (GTK_LABEL (button->postfix_label),
                            button->postfix);

        width_alloc = req.width * postfix_frac;
        gtk_widget_set_size_request (button->postfix_label,
                                     width_alloc,
                                     req.height);
        width_avail -= width_alloc;
    }
    else if (button->postfix_label)
    {
        gtk_widget_destroy (button->postfix_label);
        button->postfix_label = NULL;
    }

    /* we allocate the remainder to avoid floating point errors */
    gtk_widget_set_size_request (button->label,
                                 width_avail,
                                 req.height);
    gtk_widget_queue_draw (GTK_WIDGET (button));

    g_string_free (s, TRUE);
    g_string_free (t, TRUE);
}


static char* value_to_string(PhinSliderButton* button, double value)
{
    char* s;

    s = g_strdup_printf ("%.*f", button->digits, value);

    return s;
}
