/*
 * MyLogo.h
 *
 *  Created on: 2019年4月5日
 *      Author: tom
 */

#ifndef MYLOGO_H_
#define MYLOGO_H_

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS
#define MY_TYPE_LOGO_WINDOW my_logo_get_type()
G_DECLARE_DERIVABLE_TYPE(MyLogo,my_logo,MY,LOGO_WINDOW,GtkContainer);

typedef struct _MyLogoClass{
	GtkContainerClass parent_class;
	void (*logo_draw)(MyLogo *self,cairo_t *cr);
};

MyLogo *my_logo_new(GtkWindow *root_window,gboolean move_by_logo);
void my_logo_pack(MyLogo *self,GtkWidget *widget,gint x,gint y,gint w, gint h,gboolean relative);
G_END_DECLS


#endif /* MYLOGO_H_ */
