/*
 * MyLogo.c
 *
 *  Created on: 2019年4月5日
 *      Author: tom
 */

#include "MyLogo.h"

typedef struct {
	gdouble x, y, w, h;
	gboolean relative;
} ChildData;

typedef struct {
	GHashTable *child_table;
	GtkWindow *root_window;
	gint press_x,press_y;
	gboolean press;
} MyLogoPrivate;

G_DEFINE_TYPE_WITH_CODE(MyLogo, my_logo, GTK_TYPE_CONTAINER,
		G_ADD_PRIVATE(MyLogo));

static void my_logo_finalize(MyLogo *self) {
	MyLogoPrivate *priv = my_logo_get_instance_private(self);
	GList *k = g_hash_table_get_keys(priv->child_table);
	GList *l;
	ChildData *c_data;
	for (l = k; l != NULL; l = l->next) {
		c_data = g_hash_table_lookup(priv->child_table, l->data);
		g_free(c_data);
	}
	g_list_free(k);
	g_hash_table_unref(priv->child_table);
}

gboolean my_logo_draw(MyLogo *self, cairo_t *cr) {
	GtkWidget *parent;
	GtkAllocation alloc;
	gtk_widget_get_allocation(self,&alloc);
	cairo_surface_t *surf=cairo_image_surface_create(CAIRO_FORMAT_ARGB32,alloc.width,alloc.height);
	cairo_t *cr2=cairo_create(surf);
	MyLogoPrivate *priv = my_logo_get_instance_private(self);
	cairo_save(cr2);
	cairo_set_source_rgba(cr2, 0., 0., 0., 0.);
	cairo_set_operator(cr2, CAIRO_OPERATOR_SOURCE);
	cairo_paint(cr2);
	cairo_restore(cr2);
	cairo_save(cr2);
	g_signal_emit_by_name(self, "logo_draw", cr2, NULL);
	cairo_restore(cr2);
	GList *l, *k = g_hash_table_get_keys(priv->child_table);
	for (l = k; l != NULL; l = l->next) {
		gtk_container_propagate_draw(self, l->data, cr2);
	};
	cairo_save(cr);
	cairo_set_source_surface(cr,surf,0,0);
	cairo_paint(cr);

	cairo_destroy(cr2);
	cairo_surface_destroy(surf);
	cairo_restore(cr);
	g_list_free(k);
	return TRUE;
}


void my_logo_get_preferred_height(GtkWidget *widget, gint *minimum_height,
		gint *natural_height){
	*minimum_height=240;
	*natural_height=240;

};
void my_logo_get_preferred_width_for_height(GtkWidget *widget, gint height,
		gint *minimum_width, gint *natural_width){
	*minimum_width=height/9*16;

};
void my_logo_get_preferred_width(GtkWidget *widget, gint *minimum_width,
		gint *natural_width){
	*minimum_width=320;
	*natural_width=320;
};
void my_logo_get_preferred_height_for_width(GtkWidget *widget, gint width,
		gint *minimum_height, gint *natural_height){
	*minimum_height=width/16*9;
	*natural_height=width/16*9*1.2;
};


void my_logo_size_allocate(MyLogo *self, GtkAllocation *allocation) {
	ChildData *data;
	GtkAllocation child_alloc,alloc;
	MyLogoPrivate *priv = my_logo_get_instance_private(self);
	if(allocation!=NULL)	gtk_widget_set_allocation(self,allocation);
	gtk_widget_get_allocation(self,&alloc);
	GList *l, *k = g_hash_table_get_keys(priv->child_table);
	for (l = k; l != NULL; l = l->next) {
		data = g_hash_table_lookup(priv->child_table, l->data);
		if (data->relative) {
			child_alloc.x =  alloc.width * data->x / 100;
			child_alloc.y = alloc.height* data->y / 100;
			child_alloc.width = alloc.width * data->w / 100;
			child_alloc.height = alloc.height * data->h / 100;
		} else {
			child_alloc.x = data->x;
			child_alloc.y = data->y;
			child_alloc.width =data->w;
			child_alloc.height = data->h;
		}
		if(child_alloc.x<0)child_alloc.x=0;
		if(child_alloc.y<0)child_alloc.y=0;
		if(child_alloc.width<=0)child_alloc.width=12;
		if(child_alloc.height<=0)child_alloc.height=24;
		gtk_widget_size_allocate(l->data, &child_alloc);
	}
	g_list_free(k);
}
;

static void my_logo_add(MyLogo *self, GtkWidget *child) {
	gint m=0,n=0;
	MyLogoPrivate *priv = my_logo_get_instance_private(self);
	ChildData *data = g_malloc0(sizeof(ChildData));
	gtk_widget_get_preferred_height(child,&m,&n);
	data->h=n;
	gtk_widget_get_preferred_width_for_height(child,n,&m,&n);
	data->w=n;
	g_hash_table_insert(priv->child_table, child, data);
	gtk_widget_set_parent(child, self);
}

static void my_logo_remove(MyLogo *self, GtkWidget *child) {
	MyLogoPrivate *priv = my_logo_get_instance_private(self);
	ChildData *data = g_hash_table_lookup(priv->child_table, child);
	if (data == NULL)
		return;
	g_free(data);
	g_hash_table_remove(priv->child_table, child);
	gtk_widget_unparent(child);
}

static void my_logo_forall(MyLogo *self,
		gboolean include_internals, GtkCallback callback,
		gpointer callback_data) {
	MyLogoPrivate *priv = my_logo_get_instance_private(self);
	GList *k = g_hash_table_get_keys(priv->child_table);
	GList *l;
	ChildData *c_data;
	for (l = k; l != NULL; l = l->next) {
		(* callback)(l->data, callback_data);
	}
	g_list_free(k);
}


gboolean my_logo_motion_notify_event	(GtkWindow *window,
					 GdkEventMotion      *event,MyLogo *self){
	MyLogoPrivate *priv = my_logo_get_instance_private(self);
	if(priv->press)gtk_window_move(window,event->x_root-priv->press_x,event->y_root-priv->press_y);
	return priv->press;
};

gboolean my_logo_press_event (GtkWindow *window,
		 GdkEventButton  *event,
		 MyLogo *self){
	MyLogoPrivate *priv = my_logo_get_instance_private(self);
	priv->press_x=event->x/1;
	priv->press_y=event->y/1;
	priv->press=TRUE;
	return FALSE;
}

gboolean my_logo_release_event (GtkWindow *window,
		 GdkEventButton  *event,
		 MyLogo *self){
	MyLogoPrivate *priv = my_logo_get_instance_private(self);
	priv->press=FALSE;
	return FALSE;
}

static void my_logo_class_init(MyLogoClass *klass) {
	GObjectClass *obj = klass;
	GtkWidgetClass *gtk = klass;
	GtkContainerClass *container = klass;
	obj->finalize = my_logo_finalize;
	gtk->draw = my_logo_draw;
	gtk->size_allocate = my_logo_size_allocate;
	gtk->get_preferred_height=my_logo_get_preferred_height;
	gtk->get_preferred_height_for_width=my_logo_get_preferred_height_for_width;
	gtk->get_preferred_width=my_logo_get_preferred_width;
	gtk->get_preferred_width_for_height=my_logo_get_preferred_width_for_height;
	container->add = my_logo_add;
	container->remove = my_logo_remove;
	container->forall = my_logo_forall;
	g_signal_new("logo_draw", MY_TYPE_LOGO_WINDOW, G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET(MyLogoClass, logo_draw), NULL, NULL, NULL,
			G_TYPE_NONE, 1, G_TYPE_POINTER, NULL);

}


static void my_logo_init(MyLogo *self) {
	GtkAllocation alloc;
	MyLogoPrivate *priv = my_logo_get_instance_private(self);
	priv->child_table = g_hash_table_new(g_direct_hash, g_direct_equal);
	priv->press=FALSE;
	gtk_widget_set_has_window (self, FALSE);
}

MyLogo *my_logo_new(GtkWindow *root_window,gboolean moveable) {
	MyLogo *logo = g_object_new(MY_TYPE_LOGO_WINDOW, NULL);
	MyLogoPrivate *priv = my_logo_get_instance_private(logo);
	priv->root_window=root_window;
	gtk_widget_set_app_paintable(root_window, TRUE);
	GdkScreen *screen = gdk_screen_get_default();
	GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
	gtk_widget_set_visual(root_window, visual);
	if(moveable){
	gtk_widget_set_events(root_window,GDK_BUTTON1_MOTION_MASK|GDK_BUTTON_PRESS|GDK_BUTTON_RELEASE_MASK);
	g_signal_connect_after(root_window,"motion-notify-event",my_logo_motion_notify_event,logo);
	g_signal_connect_after(root_window,"button-press-event",my_logo_press_event,logo);
	g_signal_connect_after(root_window,"button-release-event",my_logo_release_event,logo);
	}
	return logo;
}
;

void my_logo_pack(MyLogo *self,GtkWidget *widget,gint x,gint y,gint w, gint h,gboolean relative){
	GtkAllocation alloc;
	MyLogoPrivate *priv = my_logo_get_instance_private(self);
	ChildData *data = g_malloc0(sizeof(ChildData));
	data->x=abs(x);
	data->y=abs(y);
	data->w=abs(w);
	data->h=abs(h);
	data->relative=relative;
	g_hash_table_insert(priv->child_table, widget, data);
	gtk_widget_set_parent(widget, self);
	my_logo_size_allocate(self,NULL);
	gtk_widget_queue_draw(self);
};
