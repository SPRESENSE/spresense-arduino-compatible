/*
 *  OutputMixer.cpp - SPI implement file for the Spresense SDK
 *  Copyright 2018 Sony Semiconductor Solutions Corporation
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

//***************************************************************************
// Included Files
//***************************************************************************

#include <stdio.h>
#include <stdlib.h>
#include <arch/chip/cxd56_audio.h>
#include <arch/board/board.h>

#include "OutputMixer.h"

#include "memutil/msgq_id.h"
#include "memutil/mem_layout.h"
#include "memutil/memory_layout.h"

/****************************************************************************
 * Public API on OutputMixer Class
 ****************************************************************************/

err_t OutputMixer::create(void)
{
  /* Activate mixer feature. */

  AsCreateOutputMixParam_t output_mix_create_param;
  output_mix_create_param.msgq_id.mixer = MSGQ_AUD_OUTPUT_MIX;
  output_mix_create_param.msgq_id.render_path0_filter_dsp = MSGQ_AUD_PFDSP0;
  output_mix_create_param.msgq_id.render_path1_filter_dsp = MSGQ_AUD_PFDSP1;
  output_mix_create_param.pool_id.render_path0_filter_pcm = PF0_PCM_BUF_POOL;
  output_mix_create_param.pool_id.render_path1_filter_pcm = PF1_PCM_BUF_POOL;
  output_mix_create_param.pool_id.render_path0_filter_dsp = PF0_APU_CMD_POOL;
  output_mix_create_param.pool_id.render_path1_filter_dsp = PF1_APU_CMD_POOL;

  bool result = AS_CreateOutputMixer(&output_mix_create_param);

  if (!result)
    {
      print_err("Error: AS_ActivateOutputMix() failed. system memory insufficient!\n");
      return OUTPUTMIXER_ECODE_COMMAND_ERROR;
    }

  /* Create renderer feature. */

  AsCreateRendererParam_t renderer_act_param;

  renderer_act_param.msgq_id.dev0_req  = MSGQ_AUD_RND_PLY;
  renderer_act_param.msgq_id.dev0_sync = MSGQ_AUD_RND_PLY_SYNC;
  renderer_act_param.msgq_id.dev1_req  = 0xFF;
  renderer_act_param.msgq_id.dev1_sync = 0xFF;

  result = AS_CreateRenderer(&renderer_act_param);
  if (!result)
    {
      print_err("Error: AS_CreateRenderer() failure. system memory insufficient!\n");
      return OUTPUTMIXER_ECODE_COMMAND_ERROR;
    }

  return OUTPUTMIXER_ECODE_OK;
}

/*--------------------------------------------------------------------------*/
err_t OutputMixer::activate(AsOutputMixerHandle handle, OutputMixerCallback omcb)
{
  AsActivateOutputMixer mixer_act;

  mixer_act.output_device = HPOutputDevice;
  mixer_act.mixer_type    = MainOnly;
  mixer_act.cb            = omcb;

  AS_ActivateOutputMixer(handle, &mixer_act);

  return OUTPUTMIXER_ECODE_OK;
}

/*--------------------------------------------------------------------------*/
err_t OutputMixer::sendData(AsOutputMixerHandle handle,
                            PcmProcDoneCallback pcmdone_cb,
                            AsPcmDataParam pcm)
{
  AsSendDataOutputMixer data;

  data.handle   = handle;
  data.callback = pcmdone_cb;
  data.pcm      = pcm;

  AS_SendDataOutputMixer(&data);

  return OUTPUTMIXER_ECODE_OK;
}

/*--------------------------------------------------------------------------*/
err_t OutputMixer::deactivate(AsOutputMixerHandle handle)
{
  AsDeactivateOutputMixer mixer_deact;

  AS_DeactivateOutputMixer(handle, &mixer_deact);

  return OUTPUTMIXER_ECODE_OK;
}

/*--------------------------------------------------------------------------*/
err_t OutputMixer::activateBaseband(void)
{
  /* Power on and set up baseband */

  CXD56_AUDIO_ECODE error_code;

  error_code = cxd56_audio_poweron();

  if (error_code != CXD56_AUDIO_ECODE_OK)
    {
      print_err("cxd56_audio_poweron() error!\n");
      return OUTPUTMIXER_ECODE_COMMAND_ERROR;
    }

  error_code = cxd56_audio_set_spout(true);

  if (error_code != CXD56_AUDIO_ECODE_OK)
    {
      print_err("cxd56_audio_set_spout() error!\n");
      return OUTPUTMIXER_ECODE_COMMAND_ERROR;
    }

  error_code = cxd56_audio_en_output();

  if (error_code != CXD56_AUDIO_ECODE_OK)
    {
      print_err("cxd56_audio_en_output() error!\n");
      return OUTPUTMIXER_ECODE_COMMAND_ERROR;
    }

  //error_code = AS_SetOutputSelect(AS_OUT_DEV_SP);

  // if (error_code != E_AS_OK)
  //  {
  //    print_err("AS_SetOutputSelect() error!\n");
  //    return OUTPUTMIXER_ECODE_COMMAND_ERROR;
  //  }

  return OUTPUTMIXER_ECODE_OK;
}

/*--------------------------------------------------------------------------*/
err_t OutputMixer::deactivateBaseband(void)
{
  /* Disable output */

  CXD56_AUDIO_ECODE error_code = CXD56_AUDIO_ECODE_OK;

  error_code = cxd56_audio_dis_output();

  if (error_code != CXD56_AUDIO_ECODE_OK)
    {
      print_err("cxd56_audio_dis_output() error!\n");
      return OUTPUTMIXER_ECODE_COMMAND_ERROR;
    }

  /* Power Off Baseband */

  error_code = cxd56_audio_poweroff();

  if (error_code != CXD56_AUDIO_ECODE_OK)
    {
      print_err("cxd56_audio_poweroff() error!\n");
      return OUTPUTMIXER_ECODE_COMMAND_ERROR;
    }

  return OUTPUTMIXER_ECODE_OK;
}

/*--------------------------------------------------------------------------*/
err_t OutputMixer::setVolume(int master, int player0, int player1)
{
  CXD56_AUDIO_ECODE ret = cxd56_audio_set_vol(CXD56_AUDIO_VOLID_MIXER_OUT, master);

  if (ret != CXD56_AUDIO_ECODE_OK)
    {
      return OUTPUTMIXER_ECODE_COMMAND_ERROR;
    }

  ret = cxd56_audio_set_vol(CXD56_AUDIO_VOLID_MIXER_IN1, player0);

  if (ret != CXD56_AUDIO_ECODE_OK)
    {
      return OUTPUTMIXER_ECODE_COMMAND_ERROR;
    }

  ret = cxd56_audio_set_vol(CXD56_AUDIO_VOLID_MIXER_IN2, player1);

  if (ret != CXD56_AUDIO_ECODE_OK)
    {
      return OUTPUTMIXER_ECODE_COMMAND_ERROR;
    }

  return OUTPUTMIXER_ECODE_OK;
}


