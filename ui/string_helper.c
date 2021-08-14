/*
 * string_helper.c
 *
 *  Created on: 2021年2月1日
 *      Author: tom
 */
#include "string_helper.h"

void string_replace(gchar **source, const gchar *replace_str,
		const gchar *replacement) {
	if(replacement==NULL||replace_str==NULL){
		g_printerr("%s Replacement == NULL\n",__func__);
	}
	gchar *p, *s = *source;
	GString *str = g_string_new("");
	while (1) {
		p = g_strstr_len(s, -1, replace_str);
		if (p == NULL)
			break;
		g_string_append_len(str, s, p - s);
		g_string_append_printf(str, "%s", replacement);
		s = p + strlen(replace_str);
	}
	g_string_append_printf(str, "%s", s);
	g_free(*source);
	*source = str->str;
	g_string_free(str, FALSE);
}

gchar* string_replace_not_free(const gchar *source, const gchar *replace_str,
		const gchar *replacement) {
	if(replacement==NULL||replace_str==NULL){
		g_printerr("%s Replacement == NULL\n",__func__);
		return g_strdup(source);
	}
	gchar *p, *s = source;
	GString *str = g_string_new("");
	while (1) {
		p = g_strstr_len(s, -1, replace_str);
		if (p == NULL)
			break;
		g_string_append_len(str, s, p - s);
		g_string_append_printf(str, "%s", replacement);
		s = p + strlen(replace_str);
	}
	g_string_append_printf(str, "%s", s);
	p = str->str;
	g_string_free(str, FALSE);
	return p;
}
