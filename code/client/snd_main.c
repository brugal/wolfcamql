/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2005 Stuart Dalton (badcdev@gmail.com)

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

#include "client.h"
#include "snd_codec.h"
#include "snd_local.h"
#include "snd_public.h"

cvar_t *s_volume;
cvar_t *s_muted;
cvar_t *s_musicVolume;
cvar_t *s_doppler;
cvar_t *s_backend;
cvar_t *s_muteWhenMinimized;
cvar_t *s_muteWhenUnfocused;
cvar_t *s_announcerVolume;
cvar_t *s_killBeepVolume;
cvar_t *s_useTimescale;
cvar_t *s_forceScale;
cvar_t *s_showMiss;
cvar_t *s_maxSoundRepeatTime;
cvar_t *s_maxSoundInstances;
cvar_t *s_qlAttenuate;
cvar_t *s_debugMissingSounds;

static soundInterface_t si;

/*
=================
S_ValidateInterface
=================
*/
static qboolean S_ValidSoundInterface( soundInterface_t *si )
{
	if( !si->Shutdown ) return qfalse;
	if( !si->StartSound ) return qfalse;
	if( !si->StartLocalSound ) return qfalse;
	if( !si->StartBackgroundTrack ) return qfalse;
	if( !si->StopBackgroundTrack ) return qfalse;
	if( !si->RawSamples ) return qfalse;
	if( !si->StopAllSounds ) return qfalse;
	if( !si->ClearLoopingSounds ) return qfalse;
	if( !si->AddLoopingSound ) return qfalse;
	if( !si->AddRealLoopingSound ) return qfalse;
	if( !si->StopLoopingSound ) return qfalse;
	if( !si->Respatialize ) return qfalse;
	if( !si->UpdateEntityPosition ) return qfalse;
	if( !si->Update ) return qfalse;
	if( !si->DisableSounds ) return qfalse;
	if( !si->BeginRegistration ) return qfalse;
	if( !si->RegisterSound ) return qfalse;
	if( !si->ClearSoundBuffer ) return qfalse;
	if( !si->SoundInfo ) return qfalse;
	if( !si->SoundList ) return qfalse;

#ifdef USE_VOIP
	if( !si->StartCapture ) return qfalse;
	if( !si->AvailableCaptureSamples ) return qfalse;
	if( !si->Capture ) return qfalse;
	if( !si->StopCapture ) return qfalse;
	if( !si->MasterGain ) return qfalse;
#endif

	return qtrue;
}

/*
=================
S_StartSound
=================
*/
void S_StartSound( const vec3_t origin, int entnum, int entchannel, sfxHandle_t sfx )
{
	if( si.StartSound ) {
		si.StartSound( origin, entnum, entchannel, sfx );
	}
}

/*
=================
S_StartLocalSound
=================
*/
void S_StartLocalSound( sfxHandle_t sfx, int channelNum )
{
	if( si.StartLocalSound ) {
		si.StartLocalSound( sfx, channelNum );
	}
}

/*
=================
S_StartBackgroundTrack
=================
*/
void S_StartBackgroundTrack( const char *intro, const char *loop )
{
	// quakelive hack, all files default to ogg
	char introx[MAX_STRING_CHARS];
	char loopx[MAX_STRING_CHARS];

	//Com_Printf("S_StartBackgroundTrack intro:'%s'  loop:'%s'\n", intro, loop);

	COM_StripExtension(intro, introx, sizeof(introx));
	COM_StripExtension(loop, loopx, sizeof(loopx));

	Q_strcat(introx, sizeof(introx), ".ogg");
	Q_strcat(loopx, sizeof(loopx), ".ogg");
	//Com_Printf("intro:%s  loop:%s\n", introx, loopx);

	if (!*intro  &&  !*loop) {
		// request to stop background track
		introx[0] = '\0';
		loopx[0] = '\0';
	}

	if( si.StartBackgroundTrack ) {
		//si.StartBackgroundTrack( intro, loop );
		if (!*loop) {
			//Com_Printf("^2using intro as loop\n");
			si.StartBackgroundTrack(introx, introx);
		} else {
			//Com_Printf("^2loop and intro\n");
			si.StartBackgroundTrack( introx, loopx );
		}
	}
}

/*
=================
S_StopBackgroundTrack
=================
*/
void S_StopBackgroundTrack( void )
{
	if( si.StopBackgroundTrack ) {
		si.StopBackgroundTrack( );
	}
}

/*
=================
S_RawSamples
=================
*/
void S_RawSamples (int stream, int samples, int rate, int width, int channels,
		   const byte *data, float volume)
{
	if( si.RawSamples ) {
		si.RawSamples( stream, samples, rate, width, channels, data, volume );
	}
}

/*
=================
S_StopAllSounds
=================
*/
void S_StopAllSounds( void )
{
	if( si.StopAllSounds ) {
		si.StopAllSounds( );
	}
}

/*
=================
S_ClearLoopingSounds
=================
*/
void S_ClearLoopingSounds( qboolean killall )
{
	if( si.ClearLoopingSounds ) {
		si.ClearLoopingSounds( killall );
	}
}

/*
=================
S_AddLoopingSound
=================
*/
void S_AddLoopingSound( int entityNum, const vec3_t origin,
		const vec3_t velocity, sfxHandle_t sfx )
{
	//Com_Printf("adding looping sound: %d\n", sfx);
	if( si.AddLoopingSound ) {
		si.AddLoopingSound( entityNum, origin, velocity, sfx );
	}
}

/*
=================
S_AddRealLoopingSound
=================
*/
void S_AddRealLoopingSound( int entityNum, const vec3_t origin,
		const vec3_t velocity, sfxHandle_t sfx )
{
	if( si.AddRealLoopingSound ) {
		si.AddRealLoopingSound( entityNum, origin, velocity, sfx );
	}
}

/*
=================
S_StopLoopingSound
=================
*/
void S_StopLoopingSound( int entityNum )
{
	if( si.StopLoopingSound ) {
		si.StopLoopingSound( entityNum );
	}
}

/*
=================
S_Respatialize
=================
*/
void S_Respatialize( int entityNum, const vec3_t origin,
		const vec3_t axis[3], int inwater )
{
	if( si.Respatialize ) {
		si.Respatialize( entityNum, origin, axis, inwater );
	}
}

/*
=================
S_UpdateEntityPosition
=================
*/
void S_UpdateEntityPosition( int entityNum, const vec3_t origin )
{
	if( si.UpdateEntityPosition ) {
		si.UpdateEntityPosition( entityNum, origin );
	}
}

/*
=================
S_Update
=================
*/
void S_Update( void )
{
	if(s_muted->integer)
	{
		if(!(s_muteWhenMinimized->integer && com_minimized->integer) &&
		   !(s_muteWhenUnfocused->integer && com_unfocused->integer))
		{
			s_muted->integer = qfalse;
			s_muted->modified = qtrue;
		}
	}
	else
	{
		if((s_muteWhenMinimized->integer && com_minimized->integer) ||
		   (s_muteWhenUnfocused->integer && com_unfocused->integer))
		{
			s_muted->integer = qtrue;
			s_muted->modified = qtrue;
		}
	}

	if (CL_VideoRecording(&afdMain)  &&  (cl_freezeDemoPauseVideoRecording->integer  &&  cl_freezeDemo->integer)) {
		return;
	}

	if( si.Update ) {
		si.Update( );
	}
}

/*
=================
S_DisableSounds
=================
*/
void S_DisableSounds( void )
{
	if( si.DisableSounds ) {
		si.DisableSounds( );
	}
}

/*
=================
S_BeginRegistration
=================
*/
void S_BeginRegistration( void )
{
	if( si.BeginRegistration ) {
		si.BeginRegistration( );
	}
}

/*
=================
S_RegisterSound
=================
*/
sfxHandle_t	S_RegisterSound( const char *sample, qboolean compressed )
{
	sfxHandle_t s;
	char newName[MAX_QPATH];
	int slen;

	if (sample == NULL) {
		Com_Printf("^1%s() sample == NULL (shouldn't happen)\n", __FUNCTION__);
		return 0;
	}
	//Com_Printf("S_RegisterSound %s\n", sample);

	if( si.RegisterSound ) {
		slen = strlen(sample);
		if (slen >= 5) {
			// first try ogg
			//strncpy(newName, sample, MAX_QPATH);
			COM_StripExtension(sample, newName, sizeof(newName));
			slen = strlen(newName);
			if (slen + 5 >= sizeof(newName)) {
				Com_Printf("^3%s() couldn't add ogg extension to '%s', name is too long\n", __FUNCTION__, sample);
				s = 0;
			} else {
				newName[slen + 0] = '.';
				newName[slen + 1] = 'o';
				newName[slen + 2] = 'g';
				newName[slen + 3] = 'g';
				newName[slen + 4] = '\0';
				s = si.RegisterSound(newName, compressed);
			}
			if (!s) {
				s = si.RegisterSound(sample, compressed);
			}
			if (!s) {
				//Com_Printf("^1S_RegisterSound() couldn't open '%s' or '%s'\n", newName, sample);
				Com_Printf("^1S_RegisterSound() couldn't open '%s'\n", sample);
			}
			return s;
		} else {
			Com_Printf("^3FIXME S_RegisterSound() '%s'\n", sample);
			return si.RegisterSound( sample, compressed );
		}
	} else {
		return 0;
	}
}

void S_PrintSfxFilename (sfxHandle_t sfx)
{
	si.PrintSfxFilename(sfx);
}

/*
=================
S_ClearSoundBuffer
=================
*/
void S_ClearSoundBuffer( void )
{
	if( si.ClearSoundBuffer ) {
		si.ClearSoundBuffer( );
	}
}

/*
=================
S_SoundInfo
=================
*/
void S_SoundInfo( void )
{
	if( si.SoundInfo ) {
		si.SoundInfo( );
	}
}

/*
=================
S_SoundList
=================
*/
void S_SoundList( void )
{
	if( si.SoundList ) {
		si.SoundList( );
	}
}


#ifdef USE_VOIP
/*
=================
S_StartCapture
=================
*/
void S_StartCapture( void )
{
	if( si.StartCapture ) {
		si.StartCapture( );
	}
}

/*
=================
S_AvailableCaptureSamples
=================
*/
int S_AvailableCaptureSamples( void )
{
	if( si.AvailableCaptureSamples ) {
		return si.AvailableCaptureSamples( );
	}
	return 0;
}

/*
=================
S_Capture
=================
*/
void S_Capture( int samples, byte *data )
{
	if( si.Capture ) {
		si.Capture( samples, data );
	}
}

/*
=================
S_StopCapture
=================
*/
void S_StopCapture( void )
{
	if( si.StopCapture ) {
		si.StopCapture( );
	}
}

/*
=================
S_MasterGain
=================
*/
void S_MasterGain( float gain )
{
	if( si.MasterGain ) {
		si.MasterGain( gain );
	}
}
#endif

//=============================================================================

/*
=================
S_Play_f
=================
*/
void S_Play_f( void ) {
	int 		i;
	int                     c;
	sfxHandle_t	h;

	if( !si.RegisterSound || !si.StartLocalSound ) {
		return;
	}

	c = Cmd_Argc();

	if( c < 2 ) {
		Com_Printf ("Usage: play <sound filename> [sound filename] [sound filename] ...\n");
		return;
	}

	for( i = 1; i < c; i++ ) {
		h = S_RegisterSound(Cmd_Argv(i), qfalse);

		if( h ) {
			si.StartLocalSound( h, CHAN_LOCAL_SOUND );
		}
	}
}

/*
=================
S_Music_f
=================
*/
void S_Music_f( void ) {
	int		c;

	if( !si.StartBackgroundTrack ) {
		return;
	}

	c = Cmd_Argc();

	if ( c == 2 ) {
		si.StartBackgroundTrack( Cmd_Argv(1), NULL );
	} else if ( c == 3 ) {
		si.StartBackgroundTrack( Cmd_Argv(1), Cmd_Argv(2) );
	} else {
		Com_Printf ("Usage: music <musicfile> [loopfile]\n");
		return;
	}

}

/*
=================
S_Music_f
=================
*/
void S_StopMusic_f( void )
{
	if(!si.StopBackgroundTrack)
		return;

	si.StopBackgroundTrack();
}


//=============================================================================

/*
=================
S_Init
=================
*/
void S_Init( void )
{
	cvar_t		*cv;
	qboolean	started = qfalse;

	Com_Printf( "------ Initializing Sound ------\n" );

	s_volume = Cvar_Get( "s_volume", "0.8", CVAR_ARCHIVE );
	s_musicVolume = Cvar_Get( "s_musicvolume", "0", CVAR_ARCHIVE );
	s_muted = Cvar_Get("s_muted", "0", CVAR_ROM);
	s_doppler = Cvar_Get( "s_doppler", "1", CVAR_ARCHIVE );
	s_backend = Cvar_Get( "s_backend", "", CVAR_ROM );
	s_muteWhenMinimized = Cvar_Get( "s_muteWhenMinimized", "0", CVAR_ARCHIVE );
	s_muteWhenUnfocused = Cvar_Get( "s_muteWhenUnfocused", "0", CVAR_ARCHIVE );
	s_announcerVolume = Cvar_Get("s_announcerVolume", "1.0", CVAR_ARCHIVE);
	s_killBeepVolume = Cvar_Get("s_killBeepVolume", "1.0", CVAR_ARCHIVE);
	s_useTimescale = Cvar_Get("s_useTimescale", "0", CVAR_ARCHIVE);
	s_forceScale = Cvar_Get("s_forceScale", "0.0", CVAR_ARCHIVE);
	s_showMiss = Cvar_Get("s_showMiss", "0", CVAR_ARCHIVE);
	s_maxSoundRepeatTime = Cvar_Get("s_maxSoundRepeatTime", "0", CVAR_ARCHIVE);
	s_maxSoundInstances = Cvar_Get("s_maxSoundInstances", "96", CVAR_ARCHIVE);
	s_qlAttenuate = Cvar_Get("s_qlAttenuate", "1", CVAR_ARCHIVE);
	s_debugMissingSounds = Cvar_Get("s_debugMissingSounds", "0", CVAR_ARCHIVE);

	cv = Cvar_Get( "s_initsound", "1", 0 );
	if( !cv->integer ) {
		Com_Printf( "Sound disabled.\n" );
	} else {

		S_CodecInit( );

		Cmd_AddCommand( "play", S_Play_f );
		Cmd_AddCommand( "music", S_Music_f );
		Cmd_AddCommand( "stopmusic", S_StopMusic_f );
		Cmd_AddCommand( "s_list", S_SoundList );
		Cmd_AddCommand( "s_stop", S_StopAllSounds );
		Cmd_AddCommand( "s_info", S_SoundInfo );

		cv = Cvar_Get( "s_useOpenAL", "0", CVAR_ARCHIVE );
		if( cv->integer ) {
			//OpenAL
			started = S_AL_Init( &si );
			Cvar_Set( "s_backend", "OpenAL" );
		}

		if( !started ) {
			started = S_Base_Init( &si );
			Cvar_Set( "s_backend", "base" );
		}

		if( started ) {
			if( !S_ValidSoundInterface( &si ) ) {
				Com_Error( ERR_FATAL, "Sound interface invalid." );
			}

			S_SoundInfo( );
			Com_Printf( "Sound initialization successful.\n" );
		} else {
			Com_Printf( "Sound initialization failed.\n" );
		}
	}

	Com_Printf( "--------------------------------\n");
}

/*
=================
S_Shutdown
=================
*/
void S_Shutdown( void )
{
	if( si.Shutdown ) {
		si.Shutdown( );
	}

	Com_Memset( &si, 0, sizeof( soundInterface_t ) );

	Cmd_RemoveCommand( "play" );
	Cmd_RemoveCommand( "music");
	Cmd_RemoveCommand( "stopmusic");
	Cmd_RemoveCommand( "s_list" );
	Cmd_RemoveCommand( "s_stop" );
	Cmd_RemoveCommand( "s_info" );

	S_CodecShutdown( );
}

int S_Milliseconds (void)
{
	int t;

	t = Com_Milliseconds();

	return t;
}
