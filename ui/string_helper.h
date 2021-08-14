/*
 * string_helper.h
 *
 *  Created on: 2021年2月1日
 *      Author: tom
 */

#ifndef UI_STRING_HELPER_H_
#define UI_STRING_HELPER_H_
#include <glib.h>
void string_replace(gchar **source, const gchar *replace_str,
		const gchar *replacement);
gchar* string_replace_not_free(const gchar *source, const gchar *replace_str,
		const gchar *replacement);

#endif /* UI_STRING_HELPER_H_ */
