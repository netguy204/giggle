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
#include "xmltools.h"

#include <stdio.h>

xmlNode* xml_rootchildren(char *filename) {
  xmlDoc* doc = xmlParseFile(filename);
  xmlNode* root = xmlDocGetRootElement(doc);
  return root->children;
}

void xml_free(xmlNode* node) {
  xmlFreeDoc(node->doc);
}

char* node_attr(xmlNode* node, char* name, char* defvalue) {
  char* result = (char*)xmlGetProp(node, (const xmlChar*)name);
  if(result == NULL) {
    if(strcmp(defvalue, "error") == 0) {
      fprintf(stderr, "line %d: attribute %s is required\n", node->line, name);
      exit(1);
    } else {
      return defvalue;
    }
  }

  return result;
}

int streq_(const char* str1, const char* str2) {
  return strcmp(str1, str2) == 0;
}
