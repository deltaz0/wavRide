#include "DXUT.h"
#include "DXUTcamera.h"
#include "DXUTsettingsdlg.h"
#include "SDKmisc.h"
#include "SDKwavefile.h"
#include <xaudio2.h>
#include <xaudio2fx.h>
#include <x3daudio.h>

#include "wavInstance.h"

static const X3DAUDIO_DISTANCE_CURVE_POINT Emitter_LFE_CurvePoints[3] = { 0.0f, 1.0f, 0.25f, 0.0f, 1.0f, 0.0f };
static const X3DAUDIO_DISTANCE_CURVE       Emitter_LFE_Curve          = { (X3DAUDIO_DISTANCE_CURVE_POINT*)&Emitter_LFE_CurvePoints[0], 3 };

// Specify reverb send level distance curve such that reverb send increases
// slightly with distance before rolling off to silence.
// With the direct channels being increasingly attenuated with distance,
// this has the effect of increasing the reverb-to-direct sound ratio,
// reinforcing the perception of distance.
static const X3DAUDIO_DISTANCE_CURVE_POINT Emitter_Reverb_CurvePoints[3] = { 0.0f, 0.5f, 0.75f, 1.0f, 1.0f, 0.0f };
static const X3DAUDIO_DISTANCE_CURVE       Emitter_Reverb_Curve          = { (X3DAUDIO_DISTANCE_CURVE_POINT*)&Emitter_Reverb_CurvePoints[0], 3 };

// XAudio2
//IXAudio2* pXAudio2;
//IXAudio2MasteringVoice* pMasteringVoice;
IXAudio2SourceVoice* pSourceVoice;
//IXAudio2SubmixVoice* pSubmixVoice;
//IUnknown* pReverbEffect;
BYTE* pbSampleData;

// 3D
//X3DAUDIO_HANDLE x3DInstance;
//int nFrameToApply3DAudio;

//DWORD dwChannelMask;
//UINT32 nChannels;

X3DAUDIO_DSP_SETTINGS dspSettings;
//X3DAUDIO_LISTENER listener;
X3DAUDIO_EMITTER emitter;
X3DAUDIO_CONE emitterCone;

XAUDIO2_BUFFER buffer1 = {0};
bool started;

//D3DXVECTOR3 vListenerPos;
D3DXVECTOR3 vEmitterPos;
//float fListenerAngle;
//bool  fUseListenerCone;
//bool  fUseInnerRadius;
//bool  fUseRedirectToLFE;

FLOAT32 emitterAzimuths[INPUTCHANNELS];
//FLOAT32 matrixCoefficients[INPUTCHANNELS * OUTPUTCHANNELS];

CwavInstance::CwavInstance(){
	pSourceVoice = NULL;
	pbSampleData = NULL;
}

void CwavInstance::initValues(XAUDIO2_DEVICE_DETAILS details, int zmax2,
								UINT32 nChannels, FLOAT32* matrixCoefficients){
	vEmitterPos = D3DXVECTOR3( 0, 0.0f, float( zmax2 ) );

    //fListenerAngle = 0;
    //fUseRedirectToLFE = ((details.OutputFormat.dwChannelMask & SPEAKER_LOW_FREQUENCY) != 0);

	emitter.pCone = &emitterCone;
    emitter.pCone->InnerAngle = 0.0f;
    // Setting the inner cone angles to X3DAUDIO_2PI and
    // outer cone other than 0 causes
    // the emitter to act like a point emitter using the
    // INNER cone settings only.
    emitter.pCone->OuterAngle = 0.0f;
    // Setting the outer cone angles to zero causes
    // the emitter to act like a point emitter using the
    // OUTER cone settings only.
    emitter.pCone->InnerVolume = 0.0f;
    emitter.pCone->OuterVolume = 1.0f;
    emitter.pCone->InnerLPF = 0.0f;
    emitter.pCone->OuterLPF = 1.0f;
    emitter.pCone->InnerReverb = 0.0f;
    emitter.pCone->OuterReverb = 1.0f;

    emitter.Position = vEmitterPos;
    emitter.OrientFront = D3DXVECTOR3( 0, 0, 1 );
    emitter.OrientTop = D3DXVECTOR3( 0, 1, 0 );
    emitter.ChannelCount = INPUTCHANNELS;
    emitter.ChannelRadius = 1.0f;
    emitter.pChannelAzimuths = emitterAzimuths;

    // Use of Inner radius allows for smoother transitions as
    // a sound travels directly through, above, or below the listener.
    // It also may be used to give elevation cues.
    emitter.InnerRadius = 2.0f;
    emitter.InnerRadiusAngle = X3DAUDIO_PI/4.0f;;

    emitter.pVolumeCurve = (X3DAUDIO_DISTANCE_CURVE*)&X3DAudioDefault_LinearCurve;
    emitter.pLFECurve    = (X3DAUDIO_DISTANCE_CURVE*)&Emitter_LFE_Curve;
    emitter.pLPFDirectCurve = NULL; // use default curve
    emitter.pLPFReverbCurve = NULL; // use default curve
    emitter.pReverbCurve    = (X3DAUDIO_DISTANCE_CURVE*)&Emitter_Reverb_Curve;
    emitter.CurveDistanceScaler = 14.0f;
    emitter.DopplerScaler = 0.1f;
	
	started = false;

	dspSettings.SrcChannelCount = INPUTCHANNELS;
    dspSettings.DstChannelCount = nChannels;
    dspSettings.pMatrixCoefficients = matrixCoefficients;
}

CwavInstance::~CwavInstance(){
	if(pbSampleData)
		SAFE_DELETE_ARRAY(pbSampleData);
}

void CwavInstance::clearVoice(){
	if(pSourceVoice){
		pSourceVoice->DestroyVoice();
		pSourceVoice = NULL;
	}
}

HRESULT CwavInstance::prepareWav(DWORD &cbWaveSize, HRESULT &hr, CWaveFile &wav,
	IXAudio2MasteringVoice* &pMasteringVoice, IXAudio2SubmixVoice* &pSubmixVoice,
	IXAudio2* &pXAudio2, WAVEFORMATEX *pwfx, int index){
	if( pSourceVoice )
    {
        pSourceVoice->Stop( 0 );
        pSourceVoice->DestroyVoice();
        pSourceVoice = 0;
    }
	if( pbSampleData )
		SAFE_DELETE_ARRAY( pbSampleData );
	pbSampleData = new BYTE[ cbWaveSize ];
	V_RETURN( wav.Read( pbSampleData, cbWaveSize, &cbWaveSize ) );
	//
    // Play the wave using a source voice that sends to both the submix and mastering voices
    //
    XAUDIO2_SEND_DESCRIPTOR sendDescriptors[2];
    sendDescriptors[0].Flags = XAUDIO2_SEND_USEFILTER; // LPF direct-path
    sendDescriptors[0].pOutputVoice = pMasteringVoice;
    sendDescriptors[1].Flags = XAUDIO2_SEND_USEFILTER; // LPF reverb-path -- omit for better performance at the cost of less realistic occlusion
    sendDescriptors[1].pOutputVoice = pSubmixVoice;
    const XAUDIO2_VOICE_SENDS sendList = { 2, sendDescriptors };
	// create the source voice
    V_RETURN( pXAudio2->CreateSourceVoice( &pSourceVoice, pwfx, 0,
                                                        2.0f, NULL, &sendList ) );

	XAUDIO2_BUFFER buffer = {0};
    // Submit the wave sample data using an XAUDIO2_BUFFER structure
    buffer.pAudioData = pbSampleData;
    buffer.Flags = XAUDIO2_END_OF_STREAM;
    buffer.AudioBytes = cbWaveSize;
	if(index>4 && index!=9)
		buffer.LoopCount = 0;
	else
		buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
	if(index==11)
		buffer.LoopCount = XAUDIO2_LOOP_INFINITE;

	buffer1 = buffer;

    V_RETURN( pSourceVoice->SubmitSourceBuffer( &buffer ) );

    //V_RETURN( pSourceVoice->Start( 0 ) );

    //nFrameToApply3DAudio = 0;
	return S_OK;
}

void CwavInstance::playNewLoop(int index){
	if(started==true)
		pSourceVoice->SubmitSourceBuffer( &buffer1 );
	else
		started = true;
	pSourceVoice->Start(0);
}