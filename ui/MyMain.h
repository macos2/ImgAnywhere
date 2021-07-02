/*
 * MyMain.h
 *
 *  Created on: 2021年7月1日
 *      Author: TOM
 */

#ifndef UI_MYMAIN_H_
#define UI_MYMAIN_H_

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "MyVideoArea.h"
G_BEGIN_DECLS

#define MY_TYPE_MAIN my_main_get_type()
G_DECLARE_DERIVABLE_TYPE(MyMain,my_main,MY,MAIN,GtkWindow);

typedef struct _MyMainClass{
  GtkWindowClass parent_class;
};

G_END_DECLS
#endif /* UI_MYMAIN_H_ */
