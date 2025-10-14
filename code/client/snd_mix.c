/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// snd_mix.c -- portable code to mix sounds for snd_dma.c

#include "client.h"
#include "snd_local.h"
//#include <inttypes.h>

static portable_samplepair_t paintbuffer[PAINTBUFFER_SIZE];
static int snd_vol;

int*     snd_p;  
int      snd_linear_count;
short*   snd_out;

#if defined(__GNUC__) || !id386

void S_WriteLinearBlastStereo16 (void)
{
	int		i;
	int		val;

	for (i=0 ; i<snd_linear_count ; i+=2)
	{
		val = snd_p[i]>>8;

#if 0
		if (&snd_p[i] < (int *)&paintbuffer[0]) {
			Com_Printf("^3%s() writing below paintbuffer memory %zu\n", __FUNCTION__, (int *)&paintbuffer[0] - &snd_p[i]);
		}
		if (&snd_p[i + 1] >= (int *)&paintbuffer[PAINTBUFFER_SIZE]) {
			Com_Printf("^3%s() writing past paintbuffer memory %zu\n", __FUNCTION__, &snd_p[i + 1] - (int *)&paintbuffer[PAINTBUFFER_SIZE]);
		}
#endif

		//Com_Printf("%s  val %d  %d < %d\n", __FUNCTION__, val, i, snd_linear_count);
		if (val > 0x7fff)
			snd_out[i] = 0x7fff;
		else if (val < -32768)
			snd_out[i] = -32768;
		else
			snd_out[i] = val;

		val = snd_p[i+1]>>8;
		if (val > 0x7fff)
			snd_out[i+1] = 0x7fff;
		else if (val < -32768)
			snd_out[i+1] = -32768;
		else
			snd_out[i+1] = val;
	}
}

#else   // MSVC on i386

__declspec( naked ) void S_WriteLinearBlastStereo16 (void)
{
	__asm {

 push edi
 push ebx
 mov ecx,ds:dword ptr[snd_linear_count]
 mov ebx,ds:dword ptr[snd_p]
 mov edi,ds:dword ptr[snd_out]
LWLBLoopTop:
 mov eax,ds:dword ptr[-8+ebx+ecx*4]
 sar eax,8
 cmp eax,07FFFh
 jg LClampHigh
 cmp eax,0FFFF8000h
 jnl LClampDone
 mov eax,0FFFF8000h
 jmp LClampDone
LClampHigh:
 mov eax,07FFFh
LClampDone:
 mov edx,ds:dword ptr[-4+ebx+ecx*4]
 sar edx,8
 cmp edx,07FFFh
 jg LClampHigh2
 cmp edx,0FFFF8000h
 jnl LClampDone2
 mov edx,0FFFF8000h
 jmp LClampDone2
LClampHigh2:
 mov edx,07FFFh
LClampDone2:
 shl edx,16
 and eax,0FFFFh
 or edx,eax
 mov ds:dword ptr[-4+edi+ecx*2],edx
 sub ecx,2
 jnz LWLBLoopTop
 pop ebx
 pop edi
 ret
	}
}

#endif

void S_TransferStereo16 (unsigned long *pbuf, int endtime)
{
	int		lpos;
	int		ls_paintedtime;

	snd_p = (int *) paintbuffer;
	ls_paintedtime = s_paintedtime;

	while (ls_paintedtime < endtime)
	{
	// handle recirculating buffer issues
		lpos = ls_paintedtime % dma.fullsamples;

		snd_out = (short *) pbuf + (lpos<<1); // lpos * dma.channels

		snd_linear_count = dma.fullsamples - lpos;
		if (ls_paintedtime + snd_linear_count > endtime)
			snd_linear_count = endtime - ls_paintedtime;

		snd_linear_count <<= 1; // snd_linear_count *= dma.channels

#if 0
		if ((snd_p + snd_linear_count) - (int *)paintbuffer >= (PAINTBUFFER_SIZE * 2)) {
			Com_Printf("^3%s() painting past paint buffer (%d > %d)\n", __FUNCTION__, (snd_p + snd_linear_count) - (int *)paintbuffer,  (PAINTBUFFER_SIZE * 2));
		}
#endif

	// write a linear blast of samples
		S_WriteLinearBlastStereo16 ();

		snd_p += snd_linear_count;
		ls_paintedtime += (snd_linear_count>>1); // snd_linear_count / dma.channels

		if (CL_VideoRecording(&afdMain)  &&  !(cl_freezeDemoPauseVideoRecording->integer  &&  cl_freezeDemo->integer)) {
			CL_WriteAVIAudioFrame(&afdMain, (byte *)snd_out, snd_linear_count << 1);  // snd_linear_count * (dma.samplebits/8)
		}
	}
}

/*
===================
S_TransferPaintBuffer

===================
*/
void S_TransferPaintBuffer(int endtime)
{
	int 	out_idx;
	int 	count;
	int 	*p;
	int 	step;
	int		val;
	int		i;
	unsigned long *pbuf;

	pbuf = (unsigned long *)dma.buffer;


	if ( s_testsound->integer ) {
		// write a fixed sine wave
		count = (endtime - s_paintedtime);
		for (i=0 ; i<count ; i++)
			paintbuffer[i].left = paintbuffer[i].right = sin((s_paintedtime+i)*0.1)*20000*256;
	}

	if (cl_volumeShowMeter->integer) {
		//double audioPower = 0.0;
		//uint64_t sum;
		double sum = 0.0;
		double rms;

		count = (endtime - s_paintedtime);
#if 0
		for (i = 0;  i < count;  i++) {
			int val;
			float s;

			val = paintbuffer[i].left >> 8;
			if (val > 0x7fff) {
				val = 0x7fff;
			} else if (val < -32768) {
				val = -32768;
			}
			s = fabs((float)val);
			audioPower += s * s;

			val = paintbuffer[i].right >> 8;
			if (val > 0x7fff) {
				val = 0x7fff;
			} else if (val < -32768) {
				val = -32768;
			}
			s = fabs((float)val);
			audioPower += s * s;
		}

		clc.audioPower = (audioPower / (32768.0f * 32768.0f *
										((float)(count * 2)))) * 100.0f;
		//Com_Printf("volume %f power\n", clc.audioPower);
#endif

		for (i = 0;  i < count;  i++) {
			int val;
			//float s;

			val = paintbuffer[i].left >> 8;
			if (val > 0x7fff) {
				val = 0x7fff;
			} else if (val < -32768) {
				val = -32768;
			}
			//s = fabs((float)val);
			//audioPower += s * s;
			sum += (double)(val * val);

			val = paintbuffer[i].right >> 8;
			if (val > 0x7fff) {
				val = 0x7fff;
			} else if (val < -32768) {
				val = -32768;
			}
			//s = fabs((float)val);
			//audioPower += s * s;
			sum += (double)(val * val);
		}

		clc.audioPower = ((double)sum / (32768.0f * 32768.0f *
										((float)(count * 2)))) * 100.0f;

		rms = sqrt(sum / (double)(count * 2));
		if (rms <= 0) {
			clc.audioDecibels = -90.0;
		} else {
			clc.audioDecibels = 20.0 * log10(rms / 32768.0);
		}

	}


	if (dma.samplebits == 16 && dma.channels == 2)
	{	// optimized case
		S_TransferStereo16 (pbuf, endtime);
	}
	else
	{	// general case
		p = (int *) paintbuffer;
		count = (endtime - s_paintedtime) * dma.channels;
		out_idx = ((unsigned int)s_paintedtime * dma.channels) % dma.samples;
		step = 3 - MIN(dma.channels, 2);

		if ((dma.isfloat) && (dma.samplebits == 32))
		{
			float *out = (float *) pbuf;
			for (i=0 ; i<count ; i++)
			{
				if ((i % dma.channels) >= 2)
				{
					val = 0;
				}
				else
				{
					val = *p >> 8;
					p+= step;
				}
				if (val > 0x7fff)
					val = 0x7fff;
				else if (val < -32767)  /* clamp to one less than max to make division max out at -1.0f. */
					val = -32767;
				out[out_idx] = ((float) val) / 32767.0f;
				out_idx = (out_idx + 1) % dma.samples;
			}
		}
		else if (dma.samplebits == 16)
		{
			short *out = (short *) pbuf;
			for (i=0 ; i<count ; i++)
			{
				if ((i % dma.channels) >= 2)
				{
					val = 0;
				}
				else
				{
					val = *p >> 8;
					p+= step;
				}
				if (val > 0x7fff)
					val = 0x7fff;
				else if (val < -32768)
					val = -32768;
				out[out_idx] = val;
				out_idx = (out_idx + 1) % dma.samples;
			}
		}
		else if (dma.samplebits == 8)
		{
			unsigned char *out = (unsigned char *) pbuf;
			for (i=0 ; i<count ; i++)
			{
				if ((i % dma.channels) >= 2)
				{
					val = 0;
				}
				else
				{
					val = *p >> 8;
					p+= step;
				}
				if (val > 0x7fff)
					val = 0x7fff;
				else if (val < -32768)
					val = -32768;
				out[out_idx] = (val>>8) + 128;
				out_idx = (out_idx + 1) % dma.samples;
			}
		}
	}
}


/*
===============================================================================

CHANNEL MIXING

===============================================================================
*/

static void S_PaintChannelFrom16_scalar( channel_t *ch, const sfx_t *sc, int count, int sampleOffset, float sampleOffsetf, int bufferOffset ) {
	int						data, aoff, boff;
	int						leftvol, rightvol;
	int						i, j;
	portable_samplepair_t	*samp;
	sndBuffer				*chunk;
	short					*samples;
	float					ooff, fdata[2], fdiv, fleftvol, frightvol;
	float scale;

	if (sc->soundChannels <= 0) {
		Com_Printf("^1%s() invalid channels: %d\n", __FUNCTION__, sc->soundChannels);
		return;
	}

#if 0
	if (sampleOffset + count > sc->soundLength) {
		Com_Printf("^3%s() sound length is less than count %d < %d\n", __FUNCTION__, sc->soundLength, sampleOffset + count);
	}
#endif

	if (s_useTimescale->integer) {
		scale = com_timescale->value;
	} else if (s_forceScale->value > 0.0) {
		scale = s_forceScale->value;
	} else {
		scale = 1;
	}

#if 0
	if (bufferOffset >= PAINTBUFFER_SIZE) {
		Com_Printf("^3%s() bufferOffset (%d) > PAINTBUFFER_SIZE (%d)\n", __FUNCTION__, bufferOffset, PAINTBUFFER_SIZE);
	}
#endif

	samp = &paintbuffer[ bufferOffset ];

	//Com_Printf("^3dopplerscale %f  '%s'\n", ch->dopplerScale, sc->soundName);
	if (ch->doppler) {
		sampleOffsetf = sampleOffsetf * ch->oldDopplerScale;
	}

	if ( sc->soundChannels == 2 ) {
		sampleOffset *= sc->soundChannels;
		sampleOffsetf *= (float)sc->soundChannels;

		// this is done later
		/*
		if ( sampleOffset & 1 ) {
			sampleOffset &= ~1;
		}
		*/
	}

	chunk = sc->soundData;
	while (sampleOffsetf >= (float)SND_CHUNK_SIZE) {
		chunk = chunk->next;
		sampleOffsetf -= (float)SND_CHUNK_SIZE;
		if (!chunk) {
			//Com_Printf("^1wtf !chunk\n");
			chunk = sc->soundData;
		}
	}

	if (!ch->doppler || (ch->dopplerScale * scale) <= 1.0f) {
		leftvol = ch->leftvol*snd_vol;
		rightvol = ch->rightvol*snd_vol;

		sampleOffset = floor(sampleOffsetf);
		if (sc->soundChannels == 2) {
			if ( sampleOffset & 1 ) {
				sampleOffset &= ~1;
			}
		}

		samples = chunk->sndChunk;

		for ( i=0 ; i<count ; i++ ) {
			while (sampleOffsetf >= SND_CHUNK_SIZE) {
				//Com_Printf("next chunk\n");
				chunk = chunk->next;
				if (!chunk) {
					//Com_Printf("bah\n");
					samples = NULL;
					sampleOffset = 0;
					sampleOffsetf = 0;
					break;
					//chunk = sc->soundData;
				}
				samples = chunk->sndChunk;
				sampleOffsetf -= (float)SND_CHUNK_SIZE;
				sampleOffset = floor(sampleOffsetf);
				if (sc->soundChannels == 2) {
					if ( sampleOffset & 1 ) {
						sampleOffset &= ~1;
					}
				}
			}

#if 0
			if (&samp[i] >= &samp[PAINTBUFFER_SIZE]) {
				Com_Printf("^3%s() nd writing past paintbuffer: %"PRIi64"\n", __FUNCTION__, (int64_t)(&samp[i] - &samp[PAINTBUFFER_SIZE]));
			}
#endif

			if (samples) {
				data = samples[sampleOffset];
			} else {
				samp[i].left = 0;
				samp[i].right = 0;
				continue;
			}
			sampleOffsetf += scale;
			sampleOffset = floor(sampleOffsetf);
			if (sc->soundChannels == 2) {
				if ( sampleOffset & 1 ) {
					sampleOffset &= ~1;
				}
			}

			samp[i].left += (data * leftvol)>>8;

			if ( sc->soundChannels == 2 ) {
				data = samples[sampleOffset++];
				sampleOffsetf++;
			}
			samp[i].right += (data * rightvol)>>8;

		}
	} else {
		//Com_Printf("^3doppler\n");
		fleftvol = ch->leftvol*snd_vol;
		frightvol = ch->rightvol*snd_vol;

		ooff = sampleOffsetf;
		if (sc->soundChannels == 2) {
			sampleOffset = floor(sampleOffsetf);
			if ( sampleOffset & 1 ) {
				sampleOffset &= ~1;
			}

			ooff = sampleOffset;
		}

		samples = chunk->sndChunk;

		for ( i=0 ; i<count ; i++ ) {

			aoff = ooff;
			ooff = ooff + (ch->dopplerScale * scale) * sc->soundChannels;
			boff = ooff;
			fdata[0] = fdata[1] = 0;

			for (j=aoff; j<boff; j += sc->soundChannels) {
				if (j == SND_CHUNK_SIZE) {
					chunk = chunk->next;
					if (!chunk) {
						chunk = sc->soundData;
					}
					samples = chunk->sndChunk;
					ooff -= SND_CHUNK_SIZE;
				}
				if (chunk->next) {
					if ( sc->soundChannels == 2 ) {
						fdata[0] += samples[j&(SND_CHUNK_SIZE-1)];
						fdata[1] += samples[(j+1)&(SND_CHUNK_SIZE-1)];
					} else {
						fdata[0] += samples[j&(SND_CHUNK_SIZE-1)];
						fdata[1] += samples[j&(SND_CHUNK_SIZE-1)];
					}
				} else {  // last chunk, don't read garbage
					if (j < (sc->soundLength % SND_CHUNK_SIZE)) {
						if ( sc->soundChannels == 2 ) {
							fdata[0] += samples[j&(SND_CHUNK_SIZE-1)];
							fdata[1] += samples[(j+1)&(SND_CHUNK_SIZE-1)];
						} else {
							fdata[0] += samples[j&(SND_CHUNK_SIZE-1)];
							fdata[1] += samples[j&(SND_CHUNK_SIZE-1)];
						}
					} else {
						// invalid data past j
						//FIXME should it even happen?
						//fdata += samples[j&(SND_CHUNK_SIZE-1)];
						//Com_Printf("^5would have overshot... %d\n", j);
					}
				}
			}

#if 0
			if (&samp[i] >= &samp[PAINTBUFFER_SIZE]) {
				Com_Printf("^3%s() nd writing past paintbuffer: %"PRIi64"\n", __FUNCTION__, (int64_t)(&samp[i] - &samp[PAINTBUFFER_SIZE]));
			}
#endif
			fdiv = 256 * (boff-aoff) / sc->soundChannels;
#if 0
			if (fdiv == 0) {
				Com_Error(ERR_DROP, "eek div 0");
			}
#endif
			samp[i].left += (fdata[0] * fleftvol)/fdiv;
			samp[i].right += (fdata[1] * frightvol)/fdiv;
		}
	}
}

static void S_PaintChannelFrom16( channel_t *ch, const sfx_t *sc, int count, int sampleOffset, float sampleOffsetf, int bufferOffset ) {
#if 0  //idppc_altivec
	if (com_altivec->integer) {
		// must be in a separate translation unit or G3 systems will crash.
		S_PaintChannelFrom16_altivec( paintbuffer, snd_vol, ch, sc, count, sampleOffset, bufferOffset );
		return;
	}
#endif
	S_PaintChannelFrom16_scalar( ch, sc, count, sampleOffset, sampleOffsetf, bufferOffset );
}

void S_PaintChannelFromWavelet( channel_t *ch, sfx_t *sc, int count, int sampleOffset, int bufferOffset ) {
	int						data;
	int						leftvol, rightvol;
	int						i;
	portable_samplepair_t	*samp;
	sndBuffer				*chunk;
	short					*samples;
	float scale;

	if (s_useTimescale->integer) {
		scale = com_timescale->value;
	} else if (s_forceScale->value > 0.0) {
		scale = s_forceScale->value;
	} else {
		scale = 1;
	}

	if (scale != 1.0) {
		Com_Printf("^3FIXME sound scaling not supported for %s\n", __FUNCTION__);
		scale = 1;
	}

	leftvol = ch->leftvol*snd_vol;
	rightvol = ch->rightvol*snd_vol;

	i = 0;
	samp = &paintbuffer[ bufferOffset ];
	chunk = sc->soundData;
	while (sampleOffset>=(SND_CHUNK_SIZE_FLOAT*4)) {
		chunk = chunk->next;
		sampleOffset -= (SND_CHUNK_SIZE_FLOAT*4);
		i++;
	}

	if (i!=sfxScratchIndex || sfxScratchPointer != sc) {
		decodeWavelet(chunk, sfxScratchBuffer);
		sfxScratchIndex = i;
		sfxScratchPointer = sc;
	}

	samples = sfxScratchBuffer;

	for ( i=0 ; i<count ; i++ ) {
		data  = samples[sampleOffset++];
		samp[i].left += (data * leftvol)>>8;
		samp[i].right += (data * rightvol)>>8;

		if (sampleOffset == SND_CHUNK_SIZE*2) {
			chunk = chunk->next;
			decodeWavelet(chunk, sfxScratchBuffer);
			sfxScratchIndex++;
			sampleOffset = 0;
		}
	}
}

void S_PaintChannelFromADPCM( channel_t *ch, sfx_t *sc, int count, int sampleOffset, int bufferOffset ) {
	int						data;
	int						leftvol, rightvol;
	int						i;
	portable_samplepair_t	*samp;
	sndBuffer				*chunk;
	short					*samples;
	float scale;

	if (s_useTimescale->integer) {
		scale = com_timescale->value;
	} else if (s_forceScale->value > 0.0) {
		scale = s_forceScale->value;
	} else {
		scale = 1;
	}

	if (scale != 1.0) {
		Com_Printf("^3FIXME sound scaling not supported for %s\n", __FUNCTION__);
		scale = 1;
	}

	leftvol = ch->leftvol*snd_vol;
	rightvol = ch->rightvol*snd_vol;

	i = 0;
	samp = &paintbuffer[ bufferOffset ];
	chunk = sc->soundData;

	if (ch->doppler) {
		sampleOffset = sampleOffset*ch->oldDopplerScale;
	}

	while (sampleOffset>=(SND_CHUNK_SIZE*4)) {
		chunk = chunk->next;
		sampleOffset -= (SND_CHUNK_SIZE*4);
		i++;
	}

	if (i!=sfxScratchIndex || sfxScratchPointer != sc) {
		S_AdpcmGetSamples( chunk, sfxScratchBuffer );
		sfxScratchIndex = i;
		sfxScratchPointer = sc;
	}

	samples = sfxScratchBuffer;

	for ( i=0 ; i<count ; i++ ) {
		data  = samples[sampleOffset++];
		samp[i].left += (data * leftvol)>>8;
		samp[i].right += (data * rightvol)>>8;

		if (sampleOffset == SND_CHUNK_SIZE*4) {
			chunk = chunk->next;
			S_AdpcmGetSamples( chunk, sfxScratchBuffer);
			sampleOffset = 0;
			sfxScratchIndex++;
		}
	}
}

void S_PaintChannelFromMuLaw( channel_t *ch, sfx_t *sc, int count, int sampleOffset, int bufferOffset ) {
	int						data;
	int						leftvol, rightvol;
	int						i;
	portable_samplepair_t	*samp;
	sndBuffer				*chunk;
	byte					*samples;
	float					ooff;
	float scale;

	if (s_useTimescale->integer) {
		scale = com_timescale->value;
	} else if (s_forceScale->value > 0.0) {
		scale = s_forceScale->value;
	} else {
		scale = 1;
	}

	if (scale != 1.0) {
		Com_Printf("^3FIXME sound scaling not supported for %s\n", __FUNCTION__);
		scale = 1;
	}

	leftvol = ch->leftvol*snd_vol;
	rightvol = ch->rightvol*snd_vol;

	samp = &paintbuffer[ bufferOffset ];
	chunk = sc->soundData;
	while (sampleOffset>=(SND_CHUNK_SIZE*2)) {
		chunk = chunk->next;
		sampleOffset -= (SND_CHUNK_SIZE*2);
		if (!chunk) {
			chunk = sc->soundData;
		}
	}

	if (!ch->doppler) {
		samples = (byte *)chunk->sndChunk + sampleOffset;
		for ( i=0 ; i<count ; i++ ) {
			data  = mulawToShort[*samples];
			samp[i].left += (data * leftvol)>>8;
			samp[i].right += (data * rightvol)>>8;
			samples++;
			if (chunk != NULL && samples == (byte *)chunk->sndChunk+(SND_CHUNK_SIZE*2)) {
				chunk = chunk->next;
				samples = (byte *)chunk->sndChunk;
			}
		}
	} else {
		ooff = sampleOffset;
		samples = (byte *)chunk->sndChunk;
		for ( i=0 ; i<count ; i++ ) {
			data  = mulawToShort[samples[(int)(ooff)]];
			ooff = ooff + ch->dopplerScale;
			samp[i].left += (data * leftvol)>>8;
			samp[i].right += (data * rightvol)>>8;
			if (ooff >= SND_CHUNK_SIZE*2) {
				chunk = chunk->next;
				if (!chunk) {
					chunk = sc->soundData;
				}
				samples = (byte *)chunk->sndChunk;
				ooff = 0.0;
			}
		}
	}
}

/*
===================
S_PaintChannels
===================
*/
void S_PaintChannels( int endtime ) {
	int 	i;
	int 	end;
	int 	stream;
	channel_t *ch;
	sfx_t	*sc;
	int		ltime, count;
	int		sampleOffset;
	float sampleOffsetf;
	int soundLength;
	float soundLengthf;
	float scale;
	qboolean paintedVoip;
	float voipGainOtherPlayback;

	if(s_muted->integer)
		snd_vol = 0;
	else
		snd_vol = s_volume->value*255;

	//Com_Printf ("%i to %i,  samples: %d\n", s_paintedtime, endtime, endtime - s_paintedtime);

#ifdef USE_VOIP
	voipGainOtherPlayback = cl_voipGainOtherPlayback->value;
#else
	voipGainOtherPlayback = 1.0f;
#endif

	if (s_useTimescale->integer) {
		scale = com_timescale->value;
	} else if (s_forceScale->value > 0.0) {
		scale = s_forceScale->value;
	} else {
		scale = 1;
	}

	while ( s_paintedtime < endtime ) {
		// if paintbuffer is smaller than DMA buffer
		// we may need to fill it multiple times
		end = endtime;
		if ( endtime - s_paintedtime > PAINTBUFFER_SIZE ) {
			end = s_paintedtime + PAINTBUFFER_SIZE;
			//Com_Printf("paintbuffersize\n");
		}

		// clear the paint buffer and mix any raw samples...
		Com_Memset(paintbuffer, 0, sizeof (paintbuffer));


		// check if there is voip data
		paintedVoip = qfalse;

		for (stream = 1; stream < MAX_RAW_STREAMS; stream++) {
			if ( s_rawend[stream] >= s_paintedtime ) {
				paintedVoip = qtrue;
				break;
			}
		}

		for (stream = 0; stream < MAX_RAW_STREAMS; stream++) {
			if ( s_rawend[stream] >= s_paintedtime ) {
				// copy from the streaming sound source
				const portable_samplepair_t *rawsamples = s_rawsamples[stream];
				const int stop = (end < s_rawend[stream]) ? end : s_rawend[stream];
				for ( i = s_paintedtime ; i < stop ; i++ ) {
					int s;

					//const int s = i&(MAX_RAW_SAMPLES-1);
					s = (int)floor((float)i * scale) & (MAX_RAW_SAMPLES - 1);
					//s = i & (MAX_RAW_SAMPLES - 1);

					if (stream == 0) {  // music
						if (paintedVoip) {
							paintbuffer[i-s_paintedtime].left += rawsamples[s].left * voipGainOtherPlayback;
							paintbuffer[i-s_paintedtime].right += rawsamples[s].right * voipGainOtherPlayback;

						} else {
							paintbuffer[i-s_paintedtime].left += rawsamples[s].left;
							paintbuffer[i-s_paintedtime].right += rawsamples[s].right;
						}
					} else {  // voip
						paintbuffer[i-s_paintedtime].left += rawsamples[s].left;
						paintbuffer[i-s_paintedtime].right += rawsamples[s].right;
					}
				}

				//Com_Printf("^5stream %d count: %d\n", stream, stop - s_paintedtime);
			}
		}

		if (paintedVoip) {
			//goto transfer;
			//Com_Printf("^2voip painted\n");
		}

		// paint in the channels.
		ch = s_channels;
		for ( i = 0; i < MAX_CHANNELS ; i++, ch++ ) {
			int leftVolOrig, rightVolOrig;

			//if ( !ch->thesfx || (ch->leftvol<0.25 && ch->rightvol<0.25 )) {
			if (!ch->thesfx) {
				continue;
			}

			leftVolOrig = ch->leftvol;
			rightVolOrig = ch->rightvol;

			if (ch->entchannel == CHAN_ANNOUNCER) {
				//Com_Printf("audio announcer  %d  %d\n", ch->leftvol, ch->rightvol);
				//FIXME wolfcam hack when announcer is playing and you switch views (origin stays with origin pov)
				ch->leftvol = ch->rightvol = 127;

				ch->leftvol = (float)ch->leftvol * s_announcerVolume->value;
				ch->rightvol = (float)ch->rightvol * s_announcerVolume->value;
			} else if (ch->entchannel == CHAN_KILLBEEP_SOUND) {
				ch->leftvol = ch->rightvol = 127;

				ch->leftvol = (float)ch->leftvol * s_killBeepVolume->value;
				ch->rightvol = (float)ch->rightvol * s_killBeepVolume->value;
			} else if (ch->entchannel == CHAN_LOCAL_SOUND) {
				ch->leftvol = ch->rightvol = 127;
			}

			//FIXME uhm, leftvol and rightvol are ints
			if (ch->leftvol < 0.25  &&  ch->rightvol < 0.25) {
#if 0
				if (!Q_stricmpn("sound/player/gibimp1", ch->thesfx->soundName, strlen("sound/player/gibimp1"))) {
					Com_Printf("low volume %p\n", ch);
				}
#endif
				//continue;
			}

			ltime = s_paintedtime;
			sc = ch->thesfx;

			if (sc->soundData == NULL  ||  sc->soundLength == 0) {
				continue;
			}

			sampleOffset = ltime - ch->startSample;
			sampleOffsetf = (float)sampleOffset * scale;
			sampleOffset = floor(sampleOffsetf);
			count = end - ltime;
			soundLengthf = (float)sc->soundLength / scale;
			soundLength = floor(soundLengthf);
			//soundLength = sc->soundLength;
#if 0
			if ( sampleOffset + count > soundLength ) {
				count = soundLength - sampleOffset;
			}
#endif

			if (sampleOffsetf + (float)count > soundLength) {
				count = soundLengthf - sampleOffsetf;
			}

			sampleOffset = ltime - ch->startSample;

			if (sampleOffset < 0) {
				// this can happen after /stopvideo
				//Com_Printf("smain %d\n", sampleOffset);
				//Com_Printf("  ltime %d  ch->startSample %d\n", ltime, ch->startSample);
				continue;
			}

			if (paintedVoip) {
				ch->leftvol *= voipGainOtherPlayback;
				ch->rightvol *= voipGainOtherPlayback;
			}

			if ( count > 0 ) {
				if( sc->soundCompressionMethod == SND_COMPRESSION_ADPCM) {
					S_PaintChannelFromADPCM		(ch, sc, count, sampleOffset, ltime - s_paintedtime);
				} else if( sc->soundCompressionMethod == SND_COMPRESSION_DAUB4) {
					S_PaintChannelFromWavelet	(ch, sc, count, sampleOffset, ltime - s_paintedtime);
				} else if( sc->soundCompressionMethod == SND_COMPRESSION_MULAW) {
					S_PaintChannelFromMuLaw	(ch, sc, count, sampleOffset, ltime - s_paintedtime);
				} else {
					S_PaintChannelFrom16		(ch, sc, count, sampleOffset, sampleOffsetf, ltime - s_paintedtime);
				}
			}

			ch->leftvol = leftVolOrig;
			ch->rightvol = rightVolOrig;
		}

		// paint in the looped channels.
		ch = loop_channels;
		for ( i = 0; i < numLoopChannels ; i++, ch++ ) {
			int leftVolOrig, rightVolOrig;

			if ( !ch->thesfx || (!ch->leftvol && !ch->rightvol )) {
				continue;
			}

			leftVolOrig = ch->leftvol;
			rightVolOrig = ch->rightvol;

			ltime = s_paintedtime;
			sc = ch->thesfx;

			if (sc->soundData==NULL || sc->soundLength==0) {
				continue;
			}
			// we might have to make two passes if it
			// is a looping sound effect and the end of
			// the sample is hit
			do {

				//Com_Printf("looped %d '%s' volL: %d  volR: %d\n", i, sc->soundName, ch->leftvol, ch->rightvol);

				sampleOffset = (ltime % sc->soundLength);

				soundLengthf = (float)sc->soundLength / scale;
				soundLength = floor(soundLengthf);
				//soundLength = ceil(soundLengthf);


				//soundLength = sc->soundLength;

				sampleOffsetf = (ltime % soundLength);
				sampleOffset = floor(sampleOffsetf);

				sampleOffsetf = (ltime % sc->soundLength);
				sampleOffsetf = (float)sampleOffset * scale;
				//soundLengthf = (float)sc->soundLength;

#if 0
				if (0) {  //(scale < 1.0) {
					sampleOffsetf = (float)sampleOffset * scale;
				} else {
					sampleOffsetf = (ltime % (int)((float)sc->soundLength / scale));
				}
#endif

				count = end - ltime;

#if 0
				if ( sampleOffset + count > soundLength ) {
					count = soundLength - sampleOffset;
				}
#endif

				if ((sampleOffsetf + (float)count) > soundLengthf) {
					count = soundLengthf - sampleOffsetf;
					//Com_Printf("%f > length %f\n", soundLengthf - sampleOffsetf, soundLengthf);
				}

				sampleOffset = (ltime % sc->soundLength);
				//sampleOffset = (ltime % soundLength);

				//Com_Printf("%d\n", sc->soundCompressionMethod);

				if (sampleOffset < 0) {
					break;
				}

				if (paintedVoip) {
					ch->leftvol *= voipGainOtherPlayback;
					ch->rightvol *= voipGainOtherPlayback;
				}

				if ( count > 0 ) {
					if( sc->soundCompressionMethod == SND_COMPRESSION_ADPCM) {
						S_PaintChannelFromADPCM		(ch, sc, count, sampleOffset, ltime - s_paintedtime);
					} else if( sc->soundCompressionMethod == SND_COMPRESSION_DAUB4) {
						S_PaintChannelFromWavelet	(ch, sc, count, sampleOffset, ltime - s_paintedtime);
					} else if( sc->soundCompressionMethod == SND_COMPRESSION_MULAW) {
						S_PaintChannelFromMuLaw		(ch, sc, count, sampleOffset, ltime - s_paintedtime);
					} else {
						S_PaintChannelFrom16		(ch, sc, count, sampleOffset, sampleOffsetf, ltime - s_paintedtime);
					}
					ltime += count;
				} else {
					//Com_Printf("break\n");
					break;
				}
			} while ( ltime < end);

			ch->leftvol = leftVolOrig;
			ch->rightvol = rightVolOrig;
		}

//transfer:
		// transfer out according to DMA format
		S_TransferPaintBuffer( end );
		s_paintedtime = end;
	}
}
