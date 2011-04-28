#ifndef __BANK_OPS_H__
#define __BANK_OPS_H__


#include <gtk/gtk.h>


int         bank_ops_new     (void);
int         bank_ops_open    (GtkWidget* parent_window);
int         bank_ops_save_as (GtkWidget* parent_window);
int         bank_ops_save    (GtkWidget* parent_window);
const char* bank_ops_bank    (void);

#endif /* __BANK_OPS_H__ */
