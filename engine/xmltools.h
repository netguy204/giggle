/*
 *  This file is part of GambitGameLib.
 *
 *  GambitGameLib is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GambitGameLib is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GambitGameLib.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef XMLTOOLS_H
#define XMLTOOLS_H

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <string.h>

xmlNode* xml_rootchildren(char *filename);
void xml_free(xmlNode* node);
char* node_attr(xmlNode* node, char* name, char* defvalue);

int streq_(const char* str1, const char* str2);
#define streq(a, b) streq_((const char*)a, (const char*)b)

#endif
