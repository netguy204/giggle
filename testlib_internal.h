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
#ifndef TESTLIB_INTERNAL_H
#define TESTLIB_INTERNAL_H

#include "threadlib.h"

/* happens on the renderer thread. */
void renderer_init(void* empty);
void renderer_shutdown(void* empty); // barrier
void renderer_begin_frame(void* empty);
void renderer_end_frame();
void signal_render_complete(void* empty); // barrier
void renderer_finish_image_load(ImageResource resource);
void renderer_finish_image_free(void* texturep);
void sprite_render_to_screen(Sprite sprite);

void at_exit();

/* internal data structures */
extern ThreadBarrier render_barrier;
extern CommandQueue* render_queue;

#endif
