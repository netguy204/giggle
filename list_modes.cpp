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
#include <SDL/SDL.h>

int main(int argc, char ** argv) {
  SDL_Rect** modes;
  const SDL_VideoInfo* info;

  int i;

  if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
    fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
    exit(1);
  }
  
  /* Get available fullscreen/hardware modes */
  modes = SDL_ListModes(NULL, SDL_FULLSCREEN|SDL_OPENGL);
   
  /* Check if there are any modes available */
  if (modes == (SDL_Rect**)0) {
    printf("No modes available!\n");
    exit(-1);
  }
  
  /* Check if our resolution is restricted */
  if (modes == (SDL_Rect**)-1) {
    printf("All resolutions available.\n");
  } else{
    /* Print valid modes */
    printf("Available Modes\n");
    for (i=0; modes[i]; ++i) {
      printf("  %d x %d\n", modes[i]->w, modes[i]->h);
    }
  }

  info = SDL_GetVideoInfo();
  printf("suggested %d bits per pixel\n", info->vfmt->BitsPerPixel);
  if(info->hw_available) {
    printf("hardware acceleration available\n");
  }
  if(info->wm_available) {
    printf("window manager available\n");
  }
  printf("video memory: %d KB\n", info->video_mem);

  return 0;
}
