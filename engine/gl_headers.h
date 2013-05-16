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
/* the location of the GL headers is platform dependant. clean that up
   a bit */

#ifndef GLHEADERS_H
#define GLHEADERS_H

#ifdef BUILD_SDL
#ifdef __APPLE__
/* SDL on Mac */
#include <GL/glew.h>
#include <OpenGL/gl.h>
#else
/* SDL on linux */
#include <GL/glew.h>
#ifndef WINDOWS
#include <GL/gl.h>
#endif
#endif
#define glOrthof glOrtho

#else
#if defined(BUILD_RPI) || defined(BUILD_ANDROID)
/* Rpi */
#define GL_GLEXT_PROTOTYPES

#include "GLES/gl.h"
#include "GLES2/gl2.h"
#include "GLES2/gl2ext.h"
#include "EGL/egl.h"
#include "EGL/eglext.h"

#else
#error "Either BUILD_SDL or BUILD_RPI or BUILD_ANDROID must be defined"
#endif
#endif

#ifdef ANDROID
#define glMapBuffer glMapBufferOES
#define glUnmapBuffer glUnmapBufferOES
#define GL_WRITE_ONLY GL_WRITE_ONLY_OES
#endif

#endif
