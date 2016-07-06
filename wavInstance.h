#ifndef WAVINSTANCE_H
#define WAVINSTANCE_H

#include "DXUT.h"
#include "DXUTcamera.h"
#include "DXUTsettingsdlg.h"
#include "SDKmisc.h"
#include "SDKwavefile.h"
#include <xaudio2.h>
#include <xaudio2fx.h>
#include <x3daudio.h>

#define INPUTCHANNELS 1  // number of source channels
#define OUTPUTCHANNELS 8 // maximum number of destination channels supported in this sample

#define NUM_PRESETS 30

// Constants to define our world space
/*
const INT           XMIN = -20;
const INT           XMAX = 20;
const INT           ZMIN = -20;
const INT           ZMAX = 15;
*/

class CwavInstance
{
public:
    //bool bInitialized;
	CwavInstance();

	~CwavInstance();

	void clearVoice();

	void initValues(XAUDIO2_DEVICE_DETAILS details, int zmax2,
					UINT32 nChannels, FLOAT32* matrixCoefficients);
	HRESULT prepareWav(DWORD &cbWaveSize, HRESULT &hr, CWaveFile &wav,
		IXAudio2MasteringVoice* &pMasteringVoice,
		IXAudio2SubmixVoice* &pSubmixVoice,
		IXAudio2* &pXAudio2, WAVEFORMATEX *pwfx, int index);

	void playNewLoop(int index);

	bool started;
    // XAudio2
    //IXAudio2* pXAudio2;
    //IXAudio2MasteringVoice* pMasteringVoice;
    IXAudio2SourceVoice* pSourceVoice;
    //IXAudio2SubmixVoice* pSubmixVoice;
    //IUnknown* pReverbEffect;
    BYTE* pbSampleData;
	XAUDIO2_BUFFER buffer1;

    // 3D
    //X3DAUDIO_HANDLE x3DInstance;
    //int nFrameToApply3DAudio;

    //DWORD dwChannelMask;
    //UINT32 nChannels;

    X3DAUDIO_DSP_SETTINGS dspSettings;
    //X3DAUDIO_LISTENER listener;
    X3DAUDIO_EMITTER emitter;
    X3DAUDIO_CONE emitterCone;

    //D3DXVECTOR3 vListenerPos;
    D3DXVECTOR3 vEmitterPos;
    //float fListenerAngle;
    //bool  fUseListenerCone;
    //bool  fUseInnerRadius;
    //bool  fUseRedirectToLFE;

    FLOAT32 emitterAzimuths[INPUTCHANNELS];
    //FLOAT32 matrixCoefficients[INPUTCHANNELS * OUTPUTCHANNELS];
};


//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
//extern AUDIO_STATE  g_audioState;


//--------------------------------------------------------------------------------------
// External functions
//--------------------------------------------------------------------------------------
//HRESULT InitAudio();
//HRESULT PrepareAudio( const LPWSTR wavname );
//HRESULT UpdateAudio( float fElapsedTime );
//HRESULT SetReverb( int nReverb );
//VOID PauseAudio( bool resume );
//VOID CleanupAudio();

#endif