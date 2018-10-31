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
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "phinprivate.h"
#include "phinsliderbutton.h"

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

/* signals */
enum
{
    CHANGED_SIGNAL,
    VALUE_CHANGED_SIGNAL,
    LAST_SIGNAL,
};

static int signals[LAST_SIGNAL];


typedef struct _PhinSliderButtonPrivate PhinSliderButtonPrivate;

#define PHIN_SLIDER_BUTTON_GET_PRIVATE(obj) \
    (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
    PHIN_TYPE_SLIDER_BUTTON, PhinSliderButtonPrivate))


struct _PhinSliderButtonPrivate
{
    GtkAdjustment*  adjustment;

    GdkCursor*  arrow_cursor;
    GdkCursor*  empty_cursor;
    GdkWindow*  event_window;
    GtkWidget*  left_arrow;
    GtkWidget*  right_arrow;
    GtkWidget*  label;
    GtkWidget*  prefix_label;
    GtkWidget*  postfix_label;
    GtkWidget*  entry;

    char*   prefix;
    char*   postfix;
    int     digits;
    int     hilite;
    int     state;
    int     xpress_root;
    int     ypress_root;
    int     xpress;
    int     ypress;
    int     firstrun;
    guint   threshold;
    gboolean slid;
};

/* forward declarations */
G_DEFINE_TYPE(PhinSliderButton, phin_slider_button, GTK_TYPE_HBOX)

static void phin_slider_button_dispose(         GObject* object);
static void phin_slider_button_realize(         GtkWidget* widget);
static void phin_slider_button_unrealize(       GtkWidget* widget);
static void phin_slider_button_map(             GtkWidget* widget);
static void phin_slider_button_unmap(           GtkWidget* widget);
static void phin_slider_button_size_allocate(   GtkWidget* widget,
                                                GtkAllocation*);

static gboolean phin_slider_button_expose(          GtkWidget*,
                                                    GdkEventExpose*);

static void phin_slider_button_adjustment_changed(  GtkAdjustment*,
                                                    PhinSliderButton* );

static void phin_slider_button_adjustment_value_changed(GtkAdjustment*,
                                                        PhinSliderButton*);

static void phin_slider_button_entry_activate(      GtkEntry*,
                                                    PhinSliderButton*);

static gboolean phin_slider_button_entry_focus_out( GtkEntry*,
                                                    GdkEventFocus*,
                                                    PhinSliderButton*);

static gboolean phin_slider_button_entry_key_press( GtkEntry* entry,
                                                    GdkEventKey* event,
                                                    PhinSliderButton*);

static gboolean phin_slider_button_button_press(    GtkWidget*,
                                                    GdkEventButton*);

static gboolean phin_slider_button_button_release(  GtkWidget*,
                                                    GdkEventButton*);

static gboolean phin_slider_button_key_press(       GtkWidget*,
                                                    GdkEventKey*);

static gboolean phin_slider_button_scroll(          GtkWidget*,
                                                    GdkEventScroll*);

static gboolean phin_slider_button_enter_notify(    GtkWidget*,
                                                    GdkEventCrossing*);

static gboolean phin_slider_button_leave_notify(    GtkWidget*,
                                                    GdkEventCrossing*);

static gboolean phin_slider_button_motion_notify(   GtkWidget*,
                                                    GdkEventMotion*);

/* internal utility functions */
static int      check_pointer(  PhinSliderButton*, int x, int y);
static void     update_cursor(  PhinSliderButton*);
static void     update_label(   PhinSliderButton*);
static void     entry_cancel(   PhinSliderButton*);
static void     update_size(    PhinSliderButton*);
static char*    value_to_string(PhinSliderButton*, double value);



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
GtkWidget* phin_slider_button_new(GtkAdjustment* adj, int digits)
{
    PhinSliderButton* button;
    PhinSliderButtonPrivate* p;

    debug ("new\n");

    gdouble adj_lower = gtk_adjustment_get_lower(adj);
    gdouble adj_upper = gtk_adjustment_get_upper(adj);
    gdouble adj_value = gtk_adjustment_get_value(adj);

    g_assert (adj_lower < adj_upper);
    g_assert((adj_value >= adj_lower)
          && (adj_value <= adj_upper));

    button = g_object_new (PHIN_TYPE_SLIDER_BUTTON, NULL);
    p = PHIN_SLIDER_BUTTON_GET_PRIVATE(button);

    p->digits = (digits >= 0) ? digits : DIGITS;

    if (p->digits > MAXDIGITS)
        p->digits = MAXDIGITS;

    phin_slider_button_set_adjustment(button, adj);
     
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

    adj = (GtkAdjustment*)
        gtk_adjustment_new (value, lower, upper, step, step, 0);

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
                                        GtkAdjustment* adj)
{
    PhinSliderButtonPrivate* p = PHIN_SLIDER_BUTTON_GET_PRIVATE(button);

    if (!adj)
        adj = (GtkAdjustment*)
                gtk_adjustment_new (0.0, -1.0, 1.0, 1.0, 1.0, 0.0);

    if (p->adjustment)
    {
        g_signal_handlers_disconnect_by_func(p->adjustment,
            phin_slider_button_adjustment_changed, (gpointer)button);
        g_signal_handlers_disconnect_by_func(p->adjustment,
            phin_slider_button_adjustment_value_changed, (gpointer)button);
        g_object_unref(p->adjustment);
    }

    p->adjustment = adj;
    g_object_ref (adj);
    g_object_ref_sink (G_OBJECT(adj));

    g_signal_connect (adj, "changed",
                  G_CALLBACK(phin_slider_button_adjustment_changed),
                  (gpointer) button);

    g_signal_connect(adj, "value_changed",
                  G_CALLBACK(phin_slider_button_adjustment_value_changed),
                  (gpointer) button);

    phin_slider_button_adjustment_changed (adj, button);
    phin_slider_button_adjustment_value_changed (adj, button);
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
    PhinSliderButtonPrivate* p = PHIN_SLIDER_BUTTON_GET_PRIVATE(button);

    if (!p->adjustment)
        phin_slider_button_set_adjustment (button, NULL);

    return p->adjustment;
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
    PhinSliderButtonPrivate* p = PHIN_SLIDER_BUTTON_GET_PRIVATE(button);

    gdouble adj_lower = gtk_adjustment_get_lower(p->adjustment);
    gdouble adj_upper = gtk_adjustment_get_upper(p->adjustment);

    value = CLAMP (value, adj_lower, adj_upper);

    gtk_adjustment_set_value (p->adjustment, value);
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
    PhinSliderButtonPrivate* p = PHIN_SLIDER_BUTTON_GET_PRIVATE(button);
    return gtk_adjustment_get_value(p->adjustment);
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
    PhinSliderButtonPrivate* p = PHIN_SLIDER_BUTTON_GET_PRIVATE(button);
    double value;

    g_return_if_fail (lower <= upper);

    gtk_adjustment_set_lower(p->adjustment, lower);
    gtk_adjustment_set_upper(p->adjustment, upper);

    value = CLAMP (gtk_adjustment_get_value(p->adjustment),
                   gtk_adjustment_get_lower(p->adjustment), /* ott? */
                   gtk_adjustment_get_upper(p->adjustment));

    gtk_adjustment_changed (p->adjustment);
    gtk_adjustment_set_value (p->adjustment, value);
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
    PhinSliderButtonPrivate* p = PHIN_SLIDER_BUTTON_GET_PRIVATE(button);

    if (lower)
        *lower = gtk_adjustment_get_lower(p->adjustment);
    if (upper)
        *upper = gtk_adjustment_get_upper(p->adjustment);
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
    PhinSliderButtonPrivate* p = PHIN_SLIDER_BUTTON_GET_PRIVATE(button);

    gtk_adjustment_set_step_increment(p->adjustment, step);
    gtk_adjustment_set_page_increment(p->adjustment, page);
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
    PhinSliderButtonPrivate* p = PHIN_SLIDER_BUTTON_GET_PRIVATE(button);

    if (step)
        *step = gtk_adjustment_get_step_increment(p->adjustment);
    if (page)
        *page = gtk_adjustment_get_page_increment(p->adjustment);
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
    PhinSliderButtonPrivate* p = PHIN_SLIDER_BUTTON_GET_PRIVATE(button);

    if (digits >= 0)
    {
        p->digits = digits;

        if (p->digits > MAXDIGITS)
            p->digits = MAXDIGITS;
    }

    if (prefix)
    {
        g_free (p->prefix);

        if (*prefix == '\0')
        {
            p->prefix = NULL;
        }
        else
        {
            p->prefix = g_strdup (prefix);
        }
    }

    if (postfix)
    {
        g_free (p->postfix);

        if (*postfix == '\0')
        {
            p->postfix = NULL;
        }
        else
        {
            p->postfix = g_strdup (postfix);
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
    PhinSliderButtonPrivate* p = PHIN_SLIDER_BUTTON_GET_PRIVATE(button);

    if (digits)
        *digits = p->digits;

    if (prefix)
        *prefix = p->prefix;

    if (postfix)
        *postfix = p->postfix;
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
    PhinSliderButtonPrivate* p = PHIN_SLIDER_BUTTON_GET_PRIVATE(button);
    g_return_if_fail (threshold != 0);

    p->threshold = threshold;
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
    PhinSliderButtonPrivate* p = PHIN_SLIDER_BUTTON_GET_PRIVATE(button);
    return p->threshold;
}


static void phin_slider_button_class_init (PhinSliderButtonClass* klass)
{
    GObjectClass*   object_class = G_OBJECT_CLASS(klass);
    GtkWidgetClass* widget_class = GTK_WIDGET_CLASS(klass);

    phin_slider_button_parent_class = g_type_class_peek_parent(klass);

    g_type_class_add_private(object_class, sizeof(PhinSliderButtonPrivate));

    object_class->dispose = phin_slider_button_dispose;

    widget_class->realize =             phin_slider_button_realize;
    widget_class->unrealize =           phin_slider_button_unrealize;
    widget_class->map =                 phin_slider_button_map;
    widget_class->unmap =               phin_slider_button_unmap;
    widget_class->size_allocate =       phin_slider_button_size_allocate;
    /* TODO widget_class->expose_event =        phin_slider_button_expose;*/
    widget_class->button_press_event =  phin_slider_button_button_press;
    widget_class->button_release_event= phin_slider_button_button_release;
    widget_class->key_press_event =     phin_slider_button_key_press;
    widget_class->scroll_event =        phin_slider_button_scroll;
    widget_class->enter_notify_event =  phin_slider_button_enter_notify;
    widget_class->leave_notify_event =  phin_slider_button_leave_notify;
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
    PhinSliderButtonPrivate* p = PHIN_SLIDER_BUTTON_GET_PRIVATE(button);
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

    p->arrow_cursor = NULL;
    p->empty_cursor = NULL;
    p->event_window = NULL;
    p->left_arrow = gtk_arrow_new (GTK_ARROW_LEFT, GTK_SHADOW_NONE);
    p->right_arrow = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_NONE);
    p->label = gtk_label_new (NULL);
    p->prefix_label = NULL;
    p->postfix_label = NULL;
    p->entry = gtk_entry_new ( );
    p->adjustment = NULL;
    p->prefix = NULL;
    p->postfix = NULL;
    p->digits = DIGITS;
    p->hilite = 0;
    p->state = STATE_NORMAL;
    p->xpress = 0;
    p->ypress = 0;
    p->xpress_root = 0;
    p->ypress_root = 0;
    p->threshold = 3;
    p->slid = FALSE;
    p->firstrun = 1;
     
    gtk_box_pack_start (box, p->left_arrow, FALSE, FALSE, 0);
    gtk_box_pack_start (box, p->label, TRUE, TRUE, 0);
    gtk_box_pack_start (box, p->entry, TRUE, TRUE, 0);
    gtk_box_pack_start (box, p->right_arrow, FALSE, FALSE, 0);

    gtk_widget_style_get (widget,
                          "focus-line-width",   &focus_width,
                          "focus-padding",      &focus_pad,
                          NULL);

    style = gtk_widget_get_style(widget);

    gtk_container_set_border_width (container,
                MAX(    MAX(style->xthickness, style->ythickness),
                        focus_width + focus_pad));

    gtk_entry_set_has_frame (GTK_ENTRY (p->entry), FALSE);

    gtk_entry_set_alignment (GTK_ENTRY (p->entry), 0.5);

    g_signal_connect (G_OBJECT (p->entry), "activate",
                      G_CALLBACK (phin_slider_button_entry_activate),
                      (gpointer) button);

    g_signal_connect (G_OBJECT (p->entry), "focus-out-event",
                      G_CALLBACK (phin_slider_button_entry_focus_out),
                      (gpointer) button);

    g_signal_connect (G_OBJECT (p->entry), "key-press-event",
                      G_CALLBACK (phin_slider_button_entry_key_press),
                      (gpointer) button);

    gtk_misc_set_alignment (GTK_MISC (p->left_arrow), 0.5, 0.5);
    gtk_misc_set_alignment (GTK_MISC (p->right_arrow), 0.5, 0.5);
}



static void phin_slider_button_dispose (GObject* object)
{
    PhinSliderButtonPrivate* p;

    g_return_if_fail (object != NULL);
    g_return_if_fail (PHIN_IS_SLIDER_BUTTON (object));

    p = PHIN_SLIDER_BUTTON_GET_PRIVATE(object);

    if (p->arrow_cursor)
    {
        gdk_cursor_unref (p->arrow_cursor);
        p->arrow_cursor = NULL;
    }

    if (p->empty_cursor)
    {
        gdk_cursor_unref (p->empty_cursor);
        p->empty_cursor = NULL;
    }

    if (p->event_window)
    {
        gdk_window_set_user_data (p->event_window, NULL);
        gdk_window_destroy (p->event_window);
        p->event_window = NULL;
    }

    if (p->left_arrow)
    {
        gtk_widget_destroy (p->left_arrow);
        p->left_arrow = NULL;
    }

    if (p->right_arrow)
    {
        gtk_widget_destroy (p->right_arrow);
        p->right_arrow = NULL;
    }
     
    if (p->label)
    {
        gtk_widget_destroy (p->label);
        p->label = NULL;
    }

    if (p->prefix_label)
    {
        gtk_widget_destroy (p->prefix_label);
        p->prefix_label = NULL;
    }

    if (p->postfix_label)
    {
        gtk_widget_destroy (p->postfix_label);
        p->postfix_label = NULL;
    }

    if (p->entry)
    {
        gtk_widget_destroy (p->entry);
        p->entry = NULL;
    }

    if (p->adjustment)
    {
        g_signal_handlers_disconnect_by_func(p->adjustment,
                            phin_slider_button_adjustment_changed,
                            (gpointer) PHIN_SLIDER_BUTTON(object));

        g_signal_handlers_disconnect_by_func(p->adjustment,
                            phin_slider_button_adjustment_value_changed,
                            (gpointer) PHIN_SLIDER_BUTTON(object));

        g_object_unref (p->adjustment);
        p->adjustment = NULL;
    }

    if (p->prefix)
    {
        g_free (p->prefix);
        p->prefix = NULL;
    }

    if (p->postfix)
    {
        g_free (p->postfix);
        p->postfix = NULL;
    }

    G_OBJECT_CLASS(phin_slider_button_parent_class)->dispose(object);
}



static void phin_slider_button_realize (GtkWidget* widget)
{
    PhinSliderButton*           button = PHIN_SLIDER_BUTTON(widget);
    GtkWidgetClass*             klass;
    PhinSliderButtonPrivate*    p;

    GdkWindowAttr   attributes;
    GtkAllocation   widget_alloc;
    int             attributes_mask;

    debug ("realize\n");

    g_return_if_fail (widget != NULL);
    g_return_if_fail (PHIN_IS_SLIDER_BUTTON (widget));

    klass = GTK_WIDGET_CLASS(phin_slider_button_parent_class);

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

    p = PHIN_SLIDER_BUTTON_GET_PRIVATE(button);
    p->event_window = gdk_window_new(gtk_widget_get_parent_window(widget),
                                           &attributes, attributes_mask);
    gdk_window_set_user_data (p->event_window, widget);

    p->arrow_cursor = gdk_cursor_new (GDK_SB_H_DOUBLE_ARROW);
    p->empty_cursor = gdk_cursor_new (GDK_BLANK_CURSOR);
}



static void phin_slider_button_unrealize (GtkWidget *widget)
{
    PhinSliderButton*           button = PHIN_SLIDER_BUTTON(widget);
    GtkWidgetClass*             klass;
    PhinSliderButtonPrivate*    p;

    debug ("unrealize\n");

    p = PHIN_SLIDER_BUTTON_GET_PRIVATE(button);
    gdk_cursor_unref (p->arrow_cursor);
    p->arrow_cursor = NULL;

    gdk_cursor_unref (p->empty_cursor);
    p->empty_cursor = NULL;

    gdk_window_set_user_data (p->event_window, NULL);
    gdk_window_destroy (p->event_window);
    p->event_window = NULL;
     
    klass = GTK_WIDGET_CLASS(phin_slider_button_parent_class);

    if (klass->unrealize)
        klass->unrealize (widget);
}


static void phin_slider_button_map (GtkWidget *widget)
{
    PhinSliderButton*           button;
    PhinSliderButtonPrivate*    p;

    debug ("map\n");

    g_return_if_fail (PHIN_IS_SLIDER_BUTTON (widget));
    button = PHIN_SLIDER_BUTTON(widget);
    p = PHIN_SLIDER_BUTTON_GET_PRIVATE(button);

    gtk_widget_show (p->left_arrow);
    gtk_widget_show (p->label);
    gtk_widget_show (p->right_arrow);
    gdk_window_show (p->event_window);
     
    if (p->prefix_label)
        gtk_widget_show (p->prefix_label);

    if (p->postfix_label)
        gtk_widget_show (p->postfix_label);

    GTK_WIDGET_CLASS(phin_slider_button_parent_class)->map(widget);

    gtk_widget_queue_draw(widget);
}


static void phin_slider_button_unmap (GtkWidget *widget)
{
    PhinSliderButton*           button;
    PhinSliderButtonPrivate*    p;

    debug ("unmap\n");

    g_return_if_fail (PHIN_IS_SLIDER_BUTTON (widget));
    button = PHIN_SLIDER_BUTTON(widget);
    p = PHIN_SLIDER_BUTTON_GET_PRIVATE(button);

    gtk_widget_hide (p->left_arrow);
    gtk_widget_hide (p->label);
    gtk_widget_hide (p->right_arrow);
    gdk_window_hide (p->event_window);
     
    if (p->prefix_label)
        gtk_widget_hide (p->prefix_label);

    if (p->postfix_label)
        gtk_widget_hide (p->postfix_label);

    GTK_WIDGET_CLASS(phin_slider_button_parent_class)->unmap(widget);
}


static void phin_slider_button_size_allocate (GtkWidget* widget,
                                              GtkAllocation* allocation)
{
    PhinSliderButton*           button;
    PhinSliderButtonPrivate*    p;

    g_return_if_fail (widget != NULL);
    g_return_if_fail (allocation != NULL);
    g_return_if_fail (PHIN_IS_SLIDER_BUTTON (widget));

    debug ("size allocate\n");

    button = PHIN_SLIDER_BUTTON(widget);
    p = PHIN_SLIDER_BUTTON_GET_PRIVATE(button);

    GTK_WIDGET_CLASS(phin_slider_button_parent_class)
                        ->size_allocate (widget, allocation);

    if (gtk_widget_get_realized (widget))
    {
        gdk_window_move_resize (p->event_window,
                                allocation->x,
                                allocation->y,
                                allocation->width,
                                allocation->height);
        /* make sure entry is hidden at start */
        if(p->firstrun)
        {
            gtk_widget_hide(p->entry);
            p->firstrun = 0;
        }
    }
}


static gboolean phin_slider_button_expose (GtkWidget*      widget,
                                           GdkEventExpose* event)
{
    PhinSliderButton*           button;
    PhinSliderButtonPrivate*    p;

    /* <GRRRRRRRRRRRR> */
    GtkAllocation a;
    GtkAllocation arrow_a;
    GtkAllocation label_a;
    /* </GRRRRRRRRRRRR> */

    GtkStyle* style;
    GdkWindow* window;
    int pad;                      /* pad */
	cairo_t* cr;

    g_return_val_if_fail (widget != NULL, FALSE);
    g_return_val_if_fail (PHIN_IS_SLIDER_BUTTON (widget), FALSE);
    g_return_val_if_fail (event != NULL, FALSE);
    g_return_val_if_fail (gtk_widget_is_drawable (widget), FALSE);
    g_return_val_if_fail (event->count == 0, FALSE);

    //debug ("expose\n");

    button = PHIN_SLIDER_BUTTON(widget);
    p = PHIN_SLIDER_BUTTON_GET_PRIVATE(button);

    gtk_widget_get_allocation(widget, &a);
    gtk_widget_get_allocation(p->left_arrow, &arrow_a);
    gtk_widget_get_allocation(p->label, &label_a);

    style = gtk_widget_get_style(widget);
    window = gtk_widget_get_window(widget);

    pad = gtk_container_get_border_width (GTK_CONTAINER (widget));

    /* clear the box */
	cr = gdk_cairo_create(window);
	gtk_paint_box (style, cr,
                   GTK_STATE_NORMAL,
                   GTK_SHADOW_NONE,
                   widget,
                   NULL,        /* if this is "buttondefault" the
                                 * smooth engine crashes */
                   a.x, a.y, a.width, a.height);
	cairo_destroy(cr);

    /* paint any applicable hilites */
    if (p->state == STATE_NORMAL)
    {
        if (p->hilite == LEFT_ARROW)
        {
			cr = gdk_cairo_create(window);
            gtk_paint_box (style, cr,
                           GTK_STATE_PRELIGHT,
                           GTK_SHADOW_NONE,
                           widget,
                           "button",
                           a.x, a.y,
                           arrow_a.width + pad,
                           a.height);
			cairo_destroy(cr);
		}
        else if (p->hilite == RIGHT_ARROW)
        {
            int offset;
            int width;

            offset = (a.x + arrow_a.width + label_a.width + pad);

            width = a.x + a.width - offset;
               
            if (p->prefix_label)
            {
                GtkAllocation pre_a;
                gtk_widget_get_allocation(p->prefix_label, &pre_a);
                offset += pre_a.width;
            }

            if (p->postfix_label)
            {
                GtkAllocation post_a;
                gtk_widget_get_allocation(p->postfix_label, &post_a);
                offset += post_a.width;
            }
               
			cr = gdk_cairo_create(window);
            gtk_paint_box (style, cr,
                           GTK_STATE_PRELIGHT,
                           GTK_SHADOW_NONE,
                           widget, "button",
                           offset, a.y, width, a.height);
			cairo_destroy(cr);
        }
        else if (p->hilite == LABEL)
        {
            int offset;
            int width;

            offset = a.x + arrow_a.width + pad;

            width = label_a.width;

            if (p->prefix_label)
            {
                GtkAllocation pre_a;
                gtk_widget_get_allocation(p->prefix_label, &pre_a);
                width += pre_a.width;
            }

            if (p->postfix_label)
            {
                GtkAllocation post_a;
                gtk_widget_get_allocation(p->postfix_label, &post_a);
                width += post_a.width;
            }

			cr = gdk_cairo_create(window);
			gtk_paint_box (style, cr,
                           GTK_STATE_PRELIGHT,
                           GTK_SHADOW_NONE,
                           widget, "button",
						   offset, a.y, width, a.height);
			cairo_destroy(cr);
        }
    }
     
    /* paint our border */
	cr = gdk_cairo_create(window);
	gtk_paint_shadow (style, cr,
                      GTK_STATE_NORMAL,
                      GTK_SHADOW_OUT,
                      widget,
                      "buttondefault",
                      a.x, a.y, a.width, a.height);

    gtk_paint_shadow (style, cr,
                      GTK_STATE_NORMAL,
                      GTK_SHADOW_OUT,
                      widget,
                      "button",
                      a.x, a.y, a.width, a.height);
	cairo_destroy(cr);

    /* paint the focus if we have it */
    if (gtk_widget_has_focus (widget))
    {
        int x, y;
        int width, height;

        x = a.x;
        y = a.y;
        width = a.width;
        height = a.height;
          
        x += pad;
        y += pad;
        width -= 2 * pad;
        height -= 2 * pad;

		cr = gdk_cairo_create(gtk_widget_get_window(widget));
        gtk_paint_focus (style, cr,
                                gtk_widget_get_state (widget),
                                widget, "button",
                                x, y, width, height);
		cairo_destroy(cr);
    }

    /*TODO GTK_WIDGET_CLASS(phin_slider_button_parent_class)
                            ->expose_event (widget, event);*/
     
    return FALSE;
}



static void phin_slider_button_adjustment_changed (GtkAdjustment* adj,
                                                   PhinSliderButton* button)
{
    (void)adj;
    g_return_if_fail (PHIN_IS_SLIDER_BUTTON (button));
    update_size (button);
    update_label (button);
    g_signal_emit (G_OBJECT (button), signals[CHANGED_SIGNAL], 0);
}


static void phin_slider_button_adjustment_value_changed (GtkAdjustment* adj,
                                                   PhinSliderButton* button)
{
    (void)adj;
    g_return_if_fail (PHIN_IS_SLIDER_BUTTON (button));
    update_label (button);
    g_signal_emit (G_OBJECT (button), signals[VALUE_CHANGED_SIGNAL], 0);
}


static void phin_slider_button_entry_activate (GtkEntry* entry,
                                               PhinSliderButton* button)
{
    PhinSliderButtonPrivate* p = PHIN_SLIDER_BUTTON_GET_PRIVATE(button);

    double newval;

    debug ("entry activate\n");
     
    p->state = STATE_NORMAL;
     
    newval = g_strtod (gtk_entry_get_text (GTK_ENTRY (entry)), NULL);

    gtk_adjustment_set_value (p->adjustment, newval);

    gtk_widget_hide (GTK_WIDGET (entry));
    gtk_widget_show (p->label);
    gtk_widget_queue_draw (GTK_WIDGET (button));
}


static gboolean phin_slider_button_entry_focus_out(GtkEntry* entry,
                                                   GdkEventFocus* event,
                                                   PhinSliderButton* button)
{
    (void)entry; (void)event;
    PhinSliderButtonPrivate* p = PHIN_SLIDER_BUTTON_GET_PRIVATE(button);
    GtkWidget* widget = GTK_WIDGET (button);

    debug ("entry focus out\n");

    entry_cancel (button);
    p->hilite = 0;

    /* check to see if the pointer wandered outside our toplevel; if
     * it did, we grab the focus so that a widget has the focus when
     * our toplevel gets the focus back */
    if (gtk_widget_is_toplevel (widget))
    {
        GtkWidget* toplevel = gtk_widget_get_toplevel (widget);
        GdkScreen* screen = gtk_window_get_screen (GTK_WINDOW (toplevel));
        GdkDisplay* display = gdk_screen_get_display (screen);
        GdkWindow* curwindow =
            gdk_display_get_window_at_pointer (display, NULL, NULL);

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


static gboolean phin_slider_button_entry_key_press(GtkEntry* entry,
                                                   GdkEventKey* event,
                                                   PhinSliderButton* button)
{
    (void)entry;
    debug ("entry key press\n");

    if (event->keyval == GDK_KEY_Escape)
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
    PhinSliderButtonPrivate* p = PHIN_SLIDER_BUTTON_GET_PRIVATE(button);

    debug ("button press\n");
     
    if (event->type != GDK_BUTTON_PRESS)
        return FALSE;

    if(event->button == 1)
    {
        double adj_value;
        double step_inc;

        switch (p->state)
        {
        case STATE_NORMAL:
            p->xpress = event->x;
            p->ypress = event->y;
            p->xpress_root = event->x_root;
            p->ypress_root = event->y_root;
            p->slid = FALSE;
            adj_value = gtk_adjustment_get_value(p->adjustment);
            step_inc = gtk_adjustment_get_step_increment(p->adjustment);

            if (p->hilite == LEFT_ARROW)
            {
                p->state = STATE_PRESSED;
                gtk_adjustment_set_value(p->adjustment,
                                            (adj_value - step_inc));
            }
            else if (p->hilite == RIGHT_ARROW)
            {
                p->state = STATE_PRESSED;
                gtk_adjustment_set_value(p->adjustment,
                                            (adj_value + step_inc));
            }
            else
            {
                p->state = STATE_SLIDE;
            }
            break;

        case STATE_SCROLL:
            p->state = STATE_NORMAL;
            update_cursor (button);
            break;

        case STATE_ENTRY:
            entry_cancel (button);
            p->state = STATE_NORMAL;
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
    PhinSliderButtonPrivate* p = PHIN_SLIDER_BUTTON_GET_PRIVATE(widget);

    char* s;
    char* t;

    debug ("button release\n");
    /* react to left button only */
    if(event->button == 1)
    {   
        switch (p->state)
        {
        case STATE_SLIDE:
            if (!p->slid)
            {
                double adj_value = gtk_adjustment_get_value(p->adjustment);
                p->state = STATE_ENTRY;
                s = value_to_string(button, adj_value);

                for (t = s; *t == ' '; t++);
                gtk_entry_set_text (GTK_ENTRY (p->entry), t);
                g_free (s);
                s = t = NULL;
                                  
                gtk_widget_hide (p->label);
                if (p->prefix_label)
                    gtk_widget_hide (p->prefix_label);
                if (p->postfix_label)
                    gtk_widget_hide (p->postfix_label);
                   
                gtk_widget_show (p->entry);

                gtk_widget_grab_focus (p->entry);
            }
            else
            {
                p->state = STATE_NORMAL;
                phin_warp_pointer (event->x_root, event->y_root,
                                   p->xpress_root, p->ypress_root);
                update_cursor (button);
            }
            break;
        case STATE_PRESSED:
            p->state = STATE_NORMAL;
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
    PhinSliderButtonPrivate* p = PHIN_SLIDER_BUTTON_GET_PRIVATE(widget);
    GtkAdjustment* adj = p->adjustment;
    double val = gtk_adjustment_get_value(adj);

    //debug ("key press\n");

    switch (event->keyval)
    {
    case GDK_KEY_Up:
        gtk_adjustment_set_value(adj,
                        val + gtk_adjustment_get_step_increment(adj));
        break;
    case GDK_KEY_Down:
        gtk_adjustment_set_value (adj,
                        val - gtk_adjustment_get_step_increment(adj));
        break;
    case GDK_KEY_Page_Up:
        gtk_adjustment_set_value (adj,
                        val + gtk_adjustment_get_page_increment(adj));
        break;
    case GDK_KEY_Page_Down:
        gtk_adjustment_set_value (adj,
                        val - gtk_adjustment_get_page_increment(adj));
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
    PhinSliderButtonPrivate* p = PHIN_SLIDER_BUTTON_GET_PRIVATE(widget);
    GtkAdjustment* adj;
    double val, inc;

    //debug ("scroll\n");

    if (p->state != STATE_NORMAL
     && p->state != STATE_SCROLL)
    {
        return FALSE;
    }

    p->state = STATE_SCROLL;
    update_cursor (button);

    p->xpress_root = event->x_root;
    p->ypress_root = event->y_root;
    p->xpress = event->x;
    p->ypress = event->y;

    adj = p->adjustment;
    val = gtk_adjustment_get_value(adj);
    inc = gtk_adjustment_get_page_increment(adj);

    if (event->direction == GDK_SCROLL_UP
     || event->direction == GDK_SCROLL_RIGHT)
    {
        gtk_adjustment_set_value (adj, val + inc);
    }
    else
        gtk_adjustment_set_value (adj, val - inc);

    gtk_widget_grab_focus (widget);
    return FALSE;
}


static gboolean phin_slider_button_enter_notify (GtkWidget* widget,
                                                 GdkEventCrossing* event)
{
    PhinSliderButton* button = PHIN_SLIDER_BUTTON (widget);
    PhinSliderButtonPrivate* p = PHIN_SLIDER_BUTTON_GET_PRIVATE(widget);

    int old = p->hilite;

    p->hilite = check_pointer(button, event->x, event->y);

    if (p->hilite != old)
    {
        update_cursor (button);
        gtk_widget_queue_draw (widget);
    }

    return FALSE;
}


static gboolean phin_slider_button_leave_notify (GtkWidget* widget,
                                                 GdkEventCrossing* event)
{
    (void)event;

    PhinSliderButton* button = PHIN_SLIDER_BUTTON (widget);
    PhinSliderButtonPrivate* p = PHIN_SLIDER_BUTTON_GET_PRIVATE(widget);

    p->hilite = 0;

    if (p->state == STATE_SCROLL)
        p->state = STATE_NORMAL;

    update_cursor (button);
    gtk_widget_queue_draw (widget);

    return FALSE;
}


static gboolean phin_slider_button_motion_notify (GtkWidget* widget,
                                                  GdkEventMotion* event)
{
    PhinSliderButton* button = PHIN_SLIDER_BUTTON (widget);
    PhinSliderButtonPrivate* p = PHIN_SLIDER_BUTTON_GET_PRIVATE(widget);
    int old = p->hilite;
    int xdiff = 0;
    double inc;

    p->hilite = check_pointer (button, event->x, event->y);

    if (p->hilite != old)
    {
        update_cursor (button);
        gtk_widget_queue_draw (widget);
    }

    xdiff = (event->x - p->xpress) / p->threshold;
    old = p->state;

    if (ABS (xdiff) >= 1)
    {
        switch (p->state)
        {
        case STATE_SLIDE:
            p->slid = TRUE;

            inc = gtk_adjustment_get_step_increment(p->adjustment) * xdiff;

            gtk_adjustment_set_value (p->adjustment,
                        gtk_adjustment_get_value(p->adjustment) + inc);

            phin_warp_pointer (event->x_root, event->y_root,
                               p->xpress_root, p->ypress_root);
            break;

        case STATE_PRESSED:
            p->state = STATE_SLIDE;
            p->xpress_root = event->x_root;
            p->ypress_root = event->y_root;
            p->xpress = event->x;
            p->ypress = event->y;
            update_cursor (button);
            break;

        case STATE_SCROLL:
            p->state = STATE_NORMAL;
            update_cursor (button);
            break;
        }
    }
    else if (p->state == STATE_SCROLL
        && (ABS (event->x - p->xpress) >= SCROLL_THRESHOLD
         || ABS (event->y - p->ypress) >= SCROLL_THRESHOLD))
    {
        p->state = STATE_NORMAL;
        update_cursor (button);
    }

    if (p->state != old)
        gtk_widget_queue_draw (widget);
     
    /* signal that we want more events */
    gdk_window_get_pointer (NULL, NULL, NULL, NULL);
     
    return FALSE;
}


static int check_pointer (PhinSliderButton* button, int x, int y)
{
    GtkWidget* widget = GTK_WIDGET (button);
    PhinSliderButtonPrivate* p = PHIN_SLIDER_BUTTON_GET_PRIVATE(widget);

    GtkAllocation a;
    GtkAllocation la;
    GtkAllocation ra;

    int pad = gtk_container_get_border_width (GTK_CONTAINER (button));
    int val;

    gtk_widget_get_allocation(widget,         &a);
    gtk_widget_get_allocation(p->left_arrow,  &la);
    gtk_widget_get_allocation(p->right_arrow, &ra);

    if ((y < 0 || y > a.height) || (x < 0 || x > a.width))
    {
        val = 0;
    }
    else if (x <= la.width + pad)
    {
        val = LEFT_ARROW;
    }
    else if (x >= (a.width - ra.width - pad))
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
    PhinSliderButtonPrivate* p = PHIN_SLIDER_BUTTON_GET_PRIVATE(button);

    p->state = STATE_NORMAL;

    g_signal_handlers_block_by_func (p->entry,
                                     phin_slider_button_entry_focus_out,
                                     button);
    gtk_widget_hide (p->entry);
    g_signal_handlers_unblock_by_func (p->entry,
                                       phin_slider_button_entry_focus_out,
                                       button);
    gtk_widget_show (p->label);

    if (p->prefix_label)
        gtk_widget_show (p->prefix_label);

    if (p->postfix_label)
        gtk_widget_show (p->postfix_label);

    gtk_widget_queue_draw (GTK_WIDGET (button));
}


static void update_cursor (PhinSliderButton* button)
{
    PhinSliderButtonPrivate* p = PHIN_SLIDER_BUTTON_GET_PRIVATE(button);

    switch (p->state)
    {
    case STATE_ENTRY:
    case STATE_PRESSED:
        gdk_window_set_cursor (p->event_window, NULL);
        break;
    case STATE_SLIDE:
    case STATE_SCROLL:
        gdk_window_set_cursor (p->event_window, p->empty_cursor);
        break;
    default:
        if (p->hilite == LABEL)
        {
            gdk_window_set_cursor (p->event_window, p->arrow_cursor);
        }
        else
        {
            gdk_window_set_cursor (p->event_window, NULL);
        }
        break;
    }
}


static void update_label (PhinSliderButton* button)
{
    PhinSliderButtonPrivate* p = PHIN_SLIDER_BUTTON_GET_PRIVATE(button);
    char* s;

    s = value_to_string (button, gtk_adjustment_get_value(p->adjustment));

    gtk_label_set_text (GTK_LABEL (p->label), s);
    gtk_widget_queue_draw (GTK_WIDGET (button)); /* just to be safe */

    g_free (s);
}


/* Update the sizes and existence of the labels we use, depending on
 * whether p->prefix and/or p->postfix are set.
 */
static void update_size (PhinSliderButton* button)
{
    PhinSliderButtonPrivate* p = PHIN_SLIDER_BUTTON_GET_PRIVATE(button);
    GtkRequisition req;
    GString* s = g_string_new (NULL);
    GString* t = g_string_new (NULL);
    char* u;
    int label_len = 0;
    int prefix_len = 0;
    int postfix_len = 0;
    double prefix_frac = 0;
    double postfix_frac = 0;
    int width = 0;
    int width_avail = 0;
    int width_alloc = 0;

    u = value_to_string(button, gtk_adjustment_get_lower(p->adjustment));
    s = g_string_new(u);
    g_free(u);

    u = value_to_string(button, gtk_adjustment_get_upper(p->adjustment));
    t = g_string_new(u);
    g_free(u);

    label_len = MAX (s->len, t->len);

    if (p->prefix)
        prefix_len = strlen (p->prefix);

    if (p->postfix)
        postfix_len = strlen (p->postfix);

    width = label_len + prefix_len + postfix_len;
    gtk_entry_set_width_chars (GTK_ENTRY (p->entry), width);

    gtk_widget_size_request (p->entry, &req);

    prefix_frac = prefix_len * 1.0 / width;
    postfix_frac = postfix_len * 1.0 / width;
    width_avail = req.width;

    if (p->prefix)
    {
        if (!p->prefix_label)
        {
            p->prefix_label = gtk_label_new (NULL);
            gtk_misc_set_alignment(GTK_MISC(p->prefix_label), 0.0, 0.5);
            gtk_box_pack_start (GTK_BOX (button), p->prefix_label,
                                TRUE, TRUE, 0);
            gtk_box_reorder_child (GTK_BOX (button), p->prefix_label, 1);

            if (gtk_widget_get_mapped (GTK_WIDGET (button)))
                gtk_widget_show (p->prefix_label);
        }

        gtk_label_set_text (GTK_LABEL (p->prefix_label),
                            p->prefix);

        width_alloc = req.width * prefix_frac;
        gtk_widget_set_size_request (p->prefix_label,
                                     width_alloc,
                                     req.height);
        width_avail -= width_alloc;
    }
    else if (p->prefix_label)
    {
        gtk_widget_destroy (p->prefix_label);
        p->prefix_label = NULL;
    }

    if (p->postfix)
    {
        if (!p->postfix_label)
        {
            p->postfix_label = gtk_label_new (NULL);
            gtk_misc_set_alignment(GTK_MISC(p->postfix_label), 1.0, 0.5);
            gtk_box_pack_start (GTK_BOX (button), p->postfix_label,
                                TRUE, TRUE, 0);
            gtk_box_reorder_child (GTK_BOX (button), p->postfix_label,
                                   (p->prefix_label)? 3: 2);

            if (gtk_widget_get_mapped (GTK_WIDGET (button)))
                gtk_widget_show (p->postfix_label);
        }

        gtk_label_set_text (GTK_LABEL (p->postfix_label),
                            p->postfix);

        width_alloc = req.width * postfix_frac;
        gtk_widget_set_size_request (p->postfix_label,
                                     width_alloc,
                                     req.height);
        width_avail -= width_alloc;
    }
    else if (p->postfix_label)
    {
        gtk_widget_destroy (p->postfix_label);
        p->postfix_label = NULL;
    }

    /* we allocate the remainder to avoid floating point errors */
    gtk_widget_set_size_request (p->label,
                                 width_avail,
                                 req.height);
    gtk_widget_queue_draw (GTK_WIDGET (button));

    g_string_free (s, TRUE);
    g_string_free (t, TRUE);
}


static char* value_to_string(PhinSliderButton* button, double value)
{
    PhinSliderButtonPrivate* p = PHIN_SLIDER_BUTTON_GET_PRIVATE(button);
    return g_strdup_printf ("%.*f", p->digits, value);
}
