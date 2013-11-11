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
#include "utils.h"
#include "giggle.h"
#include "game.h"
#include "android_fopen.h"
#include "testlib_internal.h"
#include "audio.h"
#include "input.h"

#include <jni.h>
#include <errno.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

extern int real_main(int argc, char ** argv);

extern "C" {
  JNIEXPORT void JNICALL Java_com_fiftyply_mosaic_MainActivity_nativeSetAssetManager(JNIEnv* env, jobject obj, jobject assetManager);
  JNIEXPORT int JNICALL Java_com_fiftyply_mosaic_GameSurfaceView_nativeStep(JNIEnv* env, jobject obj);
  JNIEXPORT void JNICALL Java_com_fiftyply_mosaic_GameSurfaceView_nativePause(JNIEnv* env, jobject obj);
  JNIEXPORT void JNICALL Java_com_fiftyply_mosaic_GameSurfaceView_nativeResume(JNIEnv* env, jobject obj);
  JNIEXPORT void JNICALL Java_com_fiftyply_mosaic_GameRenderer_nativeInit(JNIEnv* env, jobject obj);
  JNIEXPORT void JNICALL Java_com_fiftyply_mosaic_GameRenderer_nativeResize(JNIEnv* env, jobject obj, int w, int h);
  JNIEXPORT void JNICALL Java_com_fiftyply_mosaic_GameRenderer_nativeRender(JNIEnv* env, jobject obj);
  JNIEXPORT void JNICALL Java_com_fiftyply_mosaic_AudioThread_nativeFillAudioBuffer(JNIEnv* env, jobject obj, jobject buffer);
  JNIEXPORT void JNICALL Java_com_fiftyply_mosaic_MainActivity_getKeymap(JNIEnv* env, jobject obj, jobject map);
  JNIEXPORT void JNICALL Java_com_fiftyply_mosaic_MainActivity_handleKey(JNIEnv* env, jobject obj, int key, float value);

  JNIEXPORT jint JNICALL JNI_OnLoad( JavaVM *vm, void *pvt );
}

JNIEXPORT jint JNICALL JNI_OnLoad( JavaVM *vm, void *pvt ) {
  testlib_init();
  native_init();
  audio_init();
  return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL Java_com_fiftyply_mosaic_MainActivity_nativeSetAssetManager(JNIEnv* env, jobject obj, jobject assetManager) {
  AAssetManager* mgr = AAssetManager_fromJava(env, assetManager);
  android_fopen_set_asset_manager(mgr);
}

static int have_initialized = 0;

JNIEXPORT int JNICALL Java_com_fiftyply_mosaic_GameSurfaceView_nativeStep(JNIEnv* env, jobject obj) {
  //LOGI("nativeStep");
  if(!have_initialized) {
    renderer_enqueue_sync(renderer_init, NULL);
    game_init();
    have_initialized = 1;
  }

  return loop_once();
}

JNIEXPORT void JNICALL Java_com_fiftyply_mosaic_GameSurfaceView_nativePause(JNIEnv* env, jobject obj) {
  LOGI("nativePause");
  game_shutdown();
}

JNIEXPORT void JNICALL Java_com_fiftyply_mosaic_GameSurfaceView_nativeResume(JNIEnv* env, jobject obj) {
  LOGI("nativeResume");
}

JNIEXPORT void JNICALL Java_com_fiftyply_mosaic_GameRenderer_nativeInit(JNIEnv* env, jobject obj) {
  LOGI("nativeInit");
  renderer_gl_init(1360, 768);
}

JNIEXPORT void JNICALL Java_com_fiftyply_mosaic_GameRenderer_nativeResize(JNIEnv* env, jobject obj, int w, int h) {
  LOGI("nativeResize");
  renderer_resize(w, h);
}

int end_of_frame = 0;

void signal_render_complete(void* _allocator) {
  renderer_end_frame();
  StackAllocator allocator = (StackAllocator)_allocator;
  render_reply_queue->enqueue(allocator);
  end_of_frame = 1;
}

JNIEXPORT void JNICALL Java_com_fiftyply_mosaic_GameRenderer_nativeRender(JNIEnv* env, jobject obj) {
  //LOGI("nativeRender");
  // drain the render queue
  end_of_frame = 0;
  while(!end_of_frame) {
    process_render_command();
  }
}

JNIEXPORT void JNICALL Java_com_fiftyply_mosaic_AudioThread_nativeFillAudioBuffer(JNIEnv* env, jobject obj, jobject buffer) {
  void* bytes = env->GetDirectBufferAddress(buffer);
  long nBytes = env->GetDirectBufferCapacity(buffer);
  long nSamples = nBytes / sizeof(int16_t);
  audio_fill_buffer((int16_t*)bytes, nSamples);
}

JNIEXPORT void JNICALL Java_com_fiftyply_mosaic_MainActivity_getKeymap(JNIEnv* env, jobject obj, jobject map) {
  const char* map_class_name = "java/util/Map";
  jclass mapClass = env->FindClass(map_class_name);
  if(mapClass == NULL) {
    fail_exit("couldn't find %s", map_class_name);
  }

  jmethodID putMethod = env->GetMethodID(mapClass, "put",
                                         "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");
  if(putMethod == NULL) {
    fail_exit("couldn't find put method");
  }

  jclass intCls = env->FindClass("java/lang/Integer");
  jmethodID newInt = env->GetMethodID(intCls, "<init>", "(I)V");

  jstring keyNameString;
  jobject keyValue;

#define GEN_INPUT_TABLE(name, value)                                    \
  keyNameString = env->NewStringUTF(#name);                             \
  keyValue = env->NewObject(intCls, newInt, value);                     \
  env->CallObjectMethod(map, putMethod, keyNameString, keyValue);

  KEY_TABLE(GEN_INPUT_TABLE);
}

JNIEXPORT void JNICALL Java_com_fiftyply_mosaic_MainActivity_handleKey(JNIEnv* env, jobject obj, int key, float value) {
  handle_key((KeyNumber)key, value);
}

/*
void android_main(struct android_app* state) {
  LOGI("Hello, native world!");
  sleep(3);
  android_state = state;
  android_fopen_set_asset_manager(state->activity->assetManager);

  state->onAppCmd = engine_handle_cmd;
  state->onInputEvent = engine_handle_input;

  // Make sure glue isn't stripped.
  app_dummy();

  JavaVM* vm = state->activity->vm;

  // get the ouya constants
  JNIEnv* env;
  vm->AttachCurrentThread(&env, NULL);

  jclass ouyaClazz = env->FindClass("tv/ouya/console/api/OuyaController");
  int BUTTON_A = getStaticField(env, "BUTTON_A", ouyaClazz);
  LOGI("ButtonA is %d", BUTTON_A);

  // need need to pump events until we find a window
  int ident;
  int events;
  struct android_poll_source* source;

  int should_run = 1;
  // block until we get notification that our window exists
  while(should_run) {
  while ((ident=ALooper_pollAll(-1, NULL, &events, (void**)&source)) >= 0) {

    // Process this event.
    if (source != NULL) {
      source->process(state, source);
    }

    // we've got the event that says we can get going!
    if(can_start == 1) {
      real_main(0, NULL);
      should_run = 0;
    }
  }
  }

  vm->DetachCurrentThread();
}
*/
