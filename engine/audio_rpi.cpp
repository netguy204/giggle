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
#include <stdio.h>
#include <assert.h>
#include "audio.h"
#include "threadlib.h"

extern "C" {
#include "bcm_host.h"
#include "ilclient.h"
}


#define NUM_SAMPLES 1024

ILCLIENT_T *client;
COMPONENT_T *audio_render;
pthread_t audio_thread;

int scaled_buffer_size(int samples) {
  return (samples * 16 * 2) >> 3;
}

uint32_t audio_get_latency() {
   OMX_PARAM_U32TYPE param;
   OMX_ERRORTYPE error;

   memset(&param, 0, sizeof(OMX_PARAM_U32TYPE));
   param.nSize = sizeof(OMX_PARAM_U32TYPE);
   param.nVersion.nVersion = OMX_VERSION;
   param.nPortIndex = 100;

   error = OMX_GetConfig(ILC_GET_HANDLE(audio_render), OMX_IndexConfigAudioRenderingLatency, &param);
   assert(error == OMX_ErrorNone);

   return param.nU32;
}

void* audio_exec(void* udata) {
  float buffer_time_us = (float)(1e6 * NUM_SAMPLES) / SAMPLE_FREQ;
  while(1) {
    /* get a buffer */
    OMX_BUFFERHEADERTYPE *hdr;
    while((hdr = ilclient_get_input_buffer(audio_render, 100, 0)) == NULL) {
      usleep(buffer_time_us / 4); // wait 1/4 of the time to drain a buffer
    }

    // fill the buffer
    audio_fill_buffer((int16_t*)hdr->pBuffer, NUM_SAMPLES * 2);
    hdr->nOffset = 0;
    hdr->nFilledLen = scaled_buffer_size(NUM_SAMPLES);

    // submit the buffer
    OMX_ERRORTYPE error;
    error = OMX_EmptyThisBuffer(ILC_GET_HANDLE(audio_render), hdr);
    assert(error == OMX_ErrorNone);

    // drive down the latency to a buffer's length or less
    uint32_t latency;
    while(audio_get_latency() > NUM_SAMPLES) {
      usleep(buffer_time_us / 2);
    }
  }
}

void native_audio_init() {
  OMX_ERRORTYPE error;
  OMX_PARAM_PORTDEFINITIONTYPE param;
  OMX_AUDIO_PARAM_PCMMODETYPE pcm;
  int32_t s;
  
  // create and start up everything
  client = ilclient_init();
  assert(client != NULL);
  
  error = OMX_Init();
  assert(error == OMX_ErrorNone);
  
  ilclient_create_component(client, &audio_render, (char*)"audio_render", (ILCLIENT_CREATE_FLAGS_T)(ILCLIENT_ENABLE_INPUT_BUFFERS | ILCLIENT_DISABLE_ALL_PORTS));
  assert(audio_render != NULL);
  
  // set up the number/size of buffers
  memset(&param, 0, sizeof(OMX_PARAM_PORTDEFINITIONTYPE));
  param.nSize = sizeof(OMX_PARAM_PORTDEFINITIONTYPE);
  param.nVersion.nVersion = OMX_VERSION;
  param.nPortIndex = 100;
  
  error = OMX_GetParameter(ILC_GET_HANDLE(audio_render),
			   OMX_IndexParamPortDefinition, &param);
  assert(error == OMX_ErrorNone);

  int size = scaled_buffer_size(NUM_SAMPLES);
  size = (size + 15) & ~15;
  param.nBufferSize = size;
  param.nBufferCountActual = 2;
  
  error = OMX_SetParameter(ILC_GET_HANDLE(audio_render),
			   OMX_IndexParamPortDefinition, &param);
  assert(error == OMX_ErrorNone);
  
  // set the pcm parameters
  memset(&pcm, 0, sizeof(OMX_AUDIO_PARAM_PCMMODETYPE));
  pcm.nSize = sizeof(OMX_AUDIO_PARAM_PCMMODETYPE);
  pcm.nVersion.nVersion = OMX_VERSION;
  pcm.nPortIndex = 100;
  pcm.nChannels = 2;
  pcm.eNumData = OMX_NumericalDataSigned;
  pcm.eEndian = OMX_EndianLittle;
  pcm.nSamplingRate = SAMPLE_FREQ;
  pcm.bInterleaved = OMX_TRUE;
  pcm.nBitPerSample = 16;
  pcm.ePCMMode = OMX_AUDIO_PCMModeLinear;
  pcm.eChannelMapping[0] = OMX_AUDIO_ChannelLF;
  pcm.eChannelMapping[1] = OMX_AUDIO_ChannelRF;
  
  error = OMX_SetParameter(ILC_GET_HANDLE(audio_render), OMX_IndexParamAudioPcm, &pcm);
  assert(error == OMX_ErrorNone);
  
  assert(ilclient_change_component_state(audio_render, OMX_StateIdle) == 0);
  if(ilclient_enable_port_buffers(audio_render, 100, NULL, NULL, NULL) < 0) {
    // error
    ilclient_change_component_state(audio_render, OMX_StateLoaded);

    COMPONENT_T* list[2];
    list[0] = audio_render;
    list[1] = 0;
    ilclient_cleanup_components(list);
    
    error = OMX_Deinit();
    assert(error == OMX_ErrorNone);
    
    ilclient_destroy(client);
    exit(-1);
  }
  
  assert(ilclient_change_component_state(audio_render, OMX_StateExecuting) == 0);

  // set the destination
  OMX_CONFIG_BRCMAUDIODESTINATIONTYPE ar_dest;
  
  const char* name = "hdmi";
  memset(&ar_dest, 0, sizeof(ar_dest));
  ar_dest.nSize = sizeof(OMX_CONFIG_BRCMAUDIODESTINATIONTYPE);
  ar_dest.nVersion.nVersion = OMX_VERSION;
  strcpy((char *)ar_dest.sName, name);

  error = OMX_SetConfig(ILC_GET_HANDLE(audio_render), OMX_IndexConfigBrcmAudioDestination, &ar_dest);
  assert(error == OMX_ErrorNone);

  // get the buffer flow going
  pthread_create(&audio_thread, NULL, audio_exec, NULL);
}
