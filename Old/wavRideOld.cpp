//--------------------------------------------------------------------------------------
// File: XAudio2Sound3D.cpp
//
// XNA Developer Connection
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include "DXUT.h"
#include "DXUTgui.h"
#include "DXUTmisc.h"
#include "DXUTSettingsDlg.h"
#include "SDKmisc.h"
#include "SDKmesh.h"
#include "resource.h"
#include "audio.h"
#include "history.h"

//#define DEBUG_VS   // Uncomment this line to debug vertex shaders
//#define DEBUG_PS   // Uncomment this line to debug pixel shaders

//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
CDXUTDialogResourceManager      g_DialogResourceManager; // manager for shared resources of dialogs
CD3DSettingsDlg                 g_SettingsDlg;          // Device settings dialog
CDXUTTextHelper*                g_pTxtHelper = NULL;
CDXUTDialog                     g_HUD;                  // dialog for standard controls
CDXUTDialog                     g_SampleUI;             // dialog for sample specific controls

// Direct3D 9 resources
ID3DXFont*                      g_pFont9 = NULL;
ID3DXSprite*                    g_pSprite9 = NULL;
ID3DXEffect*                    g_pEffect9 = NULL;
IDirect3DVertexDeclaration9*    g_pVertexDecl = NULL;
LPDIRECT3DVERTEXBUFFER9         g_pvbFloor = NULL;

LPDIRECT3DVERTEXBUFFER9			g_pvbSourceA[15] = {NULL,NULL,NULL,NULL,
													NULL,NULL,NULL,NULL,
													NULL,NULL,NULL,NULL,
													NULL,NULL,NULL};
LPDIRECT3DVERTEXBUFFER9			g_pvbSourceWind;

const int						NUM_JUMP = 3;
const int						NUM_POINT = 30;
const int						NUM_PRE_VOICES = 12;
const int						NUM_VOICES = 15;

enum VoiceType					{ BLANK, TARGET, C_TRACER, F_TRACER };

//LPDIRECT3DVERTEXBUFFER9			g_pvbSource = NULL;

LPDIRECT3DVERTEXBUFFER9         g_pvbListener = NULL;
LPDIRECT3DVERTEXBUFFER9         g_pvbListenerCone = NULL;
LPDIRECT3DVERTEXBUFFER9         g_pvbInnerRadius = NULL;
LPDIRECT3DVERTEXBUFFER9         g_pvbGrid = NULL;

struct SOURCE_ATTR{
	bool isActive;
	bool isObs; //(is observable)
	bool pointFlag;
	float srcTimer;
	float delayTimer;
	float xOffset;
	float duration;
	VoiceType vType;
	SOURCE_ATTR* cTracerPtr;
	SOURCE_ATTR* fTracerPtr;
	int wavIndex;
};

int g_timer;
int f_timer;
int numActiveVoices;
int pointCounter;
int points[NUM_POINT];
int jumpPointCounter;
int jumpPoints[NUM_JUMP];
float currPos;
float posTracker[100];
int posCounter;
float accel;
float iAccel;
bool turnFlag;
bool inAir;
int airTimer;
int jumpDur;
float introVOffset;
bool jumpSuccess;
int jumpWindow;
bool closeGame;
float activeMusicScalar;
int preJumpNum;

SOURCE_ATTR g_sourceAttr[NUM_VOICES];

struct D3DVERTEX
{
    D3DXVECTOR3 p;
    D3DCOLOR c;
};


const LPWSTR g_SOUND_NAMES[] =
{
	L"xaudiowind.wav",
	L"snowslide2.wav",
	L"turnSlide1.wav",
	L"musicDrums1.wav",
	L"musicActive2.wav",
	L"success1.wav",
	L"failure1.wav",
	L"jumpStart3.wav",
	L"jumpEnd5.wav",
	L"snowSteps.wav",
	L"jumpVoice1.wav",
	L"gameOver.wav",
	L"C1-H.wav",
	L"C2-H.wav",
	L"C3-H.wav",
	L"C4-H.wav",
	L"C5-H.wav",
	L"C1-M.wav",
	L"C2-M.wav",
	L"C3-M.wav",
	L"C4-M.wav",
	L"C5-M.wav",
	L"C1-L.wav",
	L"C2-L.wav",
	L"C3-L.wav",
	L"C4-L.wav",
	L"C5-L.wav",
};

enum CONTROL_MODE
{
    CONTROL_SOURCE=0,
    CONTROL_LISTENER
}                               g_eControlMode = CONTROL_SOURCE;

// Must match order of g_PRESET_PARAMS
const LPWSTR g_PRESET_NAMES[ NUM_PRESETS ] =
{
    L"Forest",
    L"Default",
    L"Generic",
    L"Padded cell",
    L"Room",
    L"Bathroom",
    L"Living room",
    L"Stone room",
    L"Auditorium",
    L"Concert hall",
    L"Cave",
    L"Arena",
    L"Hangar",
    L"Carpeted hallway",
    L"Hallway",
    L"Stone Corridor",
    L"Alley",
    L"City",
    L"Mountains",
    L"Quarry",
    L"Plain",
    L"Parking lot",
    L"Sewer pipe",
    L"Underwater",
    L"Small room",
    L"Medium room",
    L"Large room",
    L"Medium hall",
    L"Large hall",
    L"Plate",
};

#define FLAG_MOVE_UP        0x1
#define FLAG_MOVE_LEFT      0x2
#define FLAG_MOVE_RIGHT     0x4
#define FLAG_MOVE_DOWN      0x8

int                             g_moveFlags = 0;

const float                     MOTION_SCALE = 10.0f;

//--------------------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------------------

// UI control IDs
#define IDC_STATIC              -1
#define IDC_TOGGLEFULLSCREEN    1
#define IDC_TOGGLEREF           2
#define IDC_CHANGEDEVICE        3
#define IDC_SOUND               4
#define IDC_CONTROL_MODE        5
#define IDC_PRESET              6
#define IDC_UP                  7
#define IDC_LEFT                8
#define IDC_RIGHT               9
#define IDC_DOWN                10
#define IDC_LISTENERCONE        11
#define IDC_INNERRADIUS         12

// Constants for colors
static const DWORD              SOURCE_COLOR = 0xffea1b1b;
static const DWORD              LISTENER_COLOR = 0xff2b2bff;
static const DWORD              FLOOR_COLOR = 0xff101010;
static const DWORD              GRID_COLOR = 0xff00a000;

//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext );
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext );
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext );
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );

bool CALLBACK IsD3D9DeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat,
                                      bool bWindowed, void* pUserContext );
HRESULT CALLBACK OnD3D9CreateDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
                                     void* pUserContext );
HRESULT CALLBACK OnD3D9ResetDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
                                    void* pUserContext );
void CALLBACK OnD3D9FrameRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext );
void CALLBACK OnD3D9LostDevice( void* pUserContext );
void CALLBACK OnD3D9DestroyDevice( void* pUserContext );

void InitApp();
void RenderText();

void CalculateAccel(float fElapsedTime);
void UpdateWind();
void UpdateSlide();
void UpdateTurnSlide();
bool SpawnVoice(int index, float delay, float duration, int width);
bool SpawnTracers(int index);
void ClearVoice(SOURCE_ATTR* sV);
void UpdateVoices(float fElapsedTime);
bool UpdateTracers(int i, float xT, float zT);
void UpdateMusic();
void PlayOneShot(int index);
bool DrawSource(HRESULT &hr, int index, IDirect3DDevice9* pd3dDevice, const D3DXMATRIXA16 *mScale);
void StartJump(int jumpTime);
void CheckJump();
void StartMusic();
void StopSlide();
void StopTurnSlide();
void SpawnJump();
void UpdateJump();
void deSpawnJump();

void UpdateWindIntro(int wOffset);
void UpdateFootStepsIntro();
bool SpawnIntroVoice(int index, float position);
bool SpawnIntroTracer(int index, bool c);
bool UpdateVoiceIntro(float fElapsedTime);
void deSpawnIntroVoice(int index);
void SpawnIntroJump();
bool UpdateJumpIntro(int fElapsedTime);
void deSpawnIntroJump();
bool UpdateIntroTracer(int i, float xT, float zT, float fElapsedTime, bool c);
void UpdateEndWind(int wOffset);
void WriteXML();
void StartGameOverVoice();
void UpdateGameOverVoice();

//stuff to handle the source voices
HRESULT CreateSourceVBuffers(HRESULT &hr, IDirect3DDevice9* pd3dDevice);
HRESULT FillSourceVBuffers(HRESULT &hr, D3DVERTEX* pVertices);

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // DXUT will create and use the best device (either D3D9 or D3D10)
    // that is available on the system depending on which D3D callbacks are set below

    // Set DXUT callbacks
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackKeyboard( OnKeyboard );
    DXUTSetCallbackFrameMove( OnFrameMove );
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );

    DXUTSetCallbackD3D9DeviceAcceptable( IsD3D9DeviceAcceptable );
    DXUTSetCallbackD3D9DeviceCreated( OnD3D9CreateDevice );
    DXUTSetCallbackD3D9DeviceReset( OnD3D9ResetDevice );
    DXUTSetCallbackD3D9DeviceLost( OnD3D9LostDevice );
    DXUTSetCallbackD3D9DeviceDestroyed( OnD3D9DestroyDevice );
    DXUTSetCallbackD3D9FrameRender( OnD3D9FrameRender );

    InitApp();

    HRESULT hr = InitAudio();
    if( FAILED( hr ) )
    {
        OutputDebugString( L"InitAudio() failed.  Disabling audio support\n" );
    }

    DXUTInit( true, true, NULL ); // Parse the command line, show msgboxes on error, no extra command line params
    DXUTSetCursorSettings( true, true );
    DXUTCreateWindow( L"WavRide" );
    DXUTCreateDevice( true, 1024, 768 );

	for(int i=0;i<NUM_PRE_VOICES;i++)
		hr = PrepareAudio( g_SOUND_NAMES[i], i );

	for(int i=NUM_PRE_VOICES;i<NUM_VOICES+NUM_PRE_VOICES;i++)
		hr = PrepareAudio( g_SOUND_NAMES[i], i );

    if( FAILED( hr ) )
    {
        OutputDebugString( L"PrepareAudio() failed\n" );
    }

	for(int i=0;i<NUM_VOICES;i++){
		g_sourceAttr[i].isActive = false;
		g_sourceAttr[i].isObs = false;
		g_sourceAttr[i].srcTimer = 0.0f;
		g_sourceAttr[i].duration = 0.0f;
		g_sourceAttr[i].pointFlag = false;
		g_sourceAttr[i].delayTimer = 0.0f;
		g_sourceAttr[i].xOffset = 0.0f;
		g_sourceAttr[i].vType = BLANK;
		g_sourceAttr[i].cTracerPtr = NULL;
		g_sourceAttr[i].fTracerPtr = NULL;
		g_sourceAttr[i].wavIndex = i + NUM_PRE_VOICES;
	}

	numActiveVoices = 0;
	pointCounter = -1;
	accel = 0.0f;
	iAccel = 0.0f;
	g_timer = 0;
	f_timer = 0;
	turnFlag = false;
	inAir = false;
	airTimer = 0;
	jumpDur = 0;
	introVOffset = 0.0f;
	jumpSuccess = false;
	jumpWindow = 0;
	closeGame = false;
	jumpPointCounter = 0;
	posCounter = 0;
	currPos = 0.0f;
	activeMusicScalar = 0.0f;
	preJumpNum = 0;

	for(int i=0;i<NUM_JUMP;i++){
		points[i] = 0;
	}

	for(int i=0;i<4;i++){
		jumpPoints[i] = 0;
	}

	for(int i=0;i<100;i++){
		posTracker[i] = 0.0f;
	}

    DXUTMainLoop(); // Enter into the DXUT render loop

    CleanupAudio();

    return DXUTGetExitCode();
}


//--------------------------------------------------------------------------------------
// Initialize the app
//--------------------------------------------------------------------------------------
void InitApp()
{
    //g_SettingsDlg.Init( &g_DialogResourceManager );
    //g_HUD.Init( &g_DialogResourceManager );
    //g_SampleUI.Init( &g_DialogResourceManager );

    //g_HUD.SetCallback( OnGUIEvent ); int iY = 10;
    //g_HUD.AddButton( IDC_TOGGLEFULLSCREEN, L"Toggle full screen", 35, iY, 125, 22 );
    //g_HUD.AddButton( IDC_TOGGLEREF, L"Toggle REF (F3)", 35, iY += 24, 125, 22, VK_F3 );
    //g_HUD.AddButton( IDC_CHANGEDEVICE, L"Change device (F2)", 35, iY += 24, 125, 22, VK_F2 );

    //g_SampleUI.SetCallback( OnGUIEvent );

    //
    // Sound control
    //

	/*
    CDXUTComboBox* pComboBox = NULL;
    g_SampleUI.AddStatic( IDC_STATIC, L"S(o)und", 10, 0, 105, 25 );
    
	g_SampleUI.AddComboBox( IDC_SOUND, 10, 25, 140, 24, 'O', false, &pComboBox );
    if( pComboBox )
        pComboBox->SetDropHeight( 50 );

    for( int i = 0; i < sizeof( g_SOUND_NAMES ) / sizeof( WCHAR* ); i++ )
    {
        pComboBox->AddItem( g_SOUND_NAMES[i], IntToPtr( i ) );
    }

    //
    // Control mode
    //

    g_SampleUI.AddStatic( IDC_STATIC, L"(C)ontrol mode", 10, 45, 105, 25 );
    g_SampleUI.AddComboBox( IDC_CONTROL_MODE, 10, 70, 140, 24, 'C', false, &pComboBox );
    if( pComboBox )
        pComboBox->SetDropHeight( 30 );

    pComboBox->AddItem( L"Source", IntToPtr( CONTROL_SOURCE ) );
    pComboBox->AddItem( L"Listener", IntToPtr( CONTROL_LISTENER ) );

    //
    // I3DL2 reverb preset control
    //

    g_SampleUI.AddStatic( IDC_STATIC, L"(R)everb", 10, 90, 105, 25 );
    g_SampleUI.AddComboBox( IDC_PRESET, 10, 115, 140, 24, 'R', false, &pComboBox );
    if( pComboBox )
        pComboBox->SetDropHeight( 50 );

    for( int i = 0; i < sizeof( g_PRESET_NAMES ) / sizeof( WCHAR* ); i++ )
    {
        pComboBox->AddItem( g_PRESET_NAMES[i], IntToPtr( i ) );
    }

    //
    // Movement buttons
    //

    iY = 160;
    g_SampleUI.AddButton( IDC_UP, L"(W)", 40, iY, 70, 24 );
    g_SampleUI.AddButton( IDC_LEFT, L"(A)", 5, iY += 30, 70, 24 );
    g_SampleUI.AddButton( IDC_RIGHT, L"(D)", 75, iY, 70, 24 );
    g_SampleUI.AddButton( IDC_DOWN, L"(S)", 40, iY += 30, 70, 24 );

    //
    // Listener cone and inner radius buttons
    //
    g_SampleUI.AddButton( IDC_LISTENERCONE, L"Toggle Listener Cone", 10, iY += 50, 125, 22);
    g_SampleUI.AddButton( IDC_INNERRADIUS, L"Toggle Inner Radius", 10, iY += 24, 125, 22);
	*/
	SetReverb( 18 );
}


//--------------------------------------------------------------------------------------
// Render the help and statistics text. This function uses the ID3DXFont interface for
// efficient text rendering.
//--------------------------------------------------------------------------------------
void RenderText()
{
	/*
    g_pTxtHelper->Begin();

    g_pTxtHelper->SetInsertionPos( 5, 5 );
    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 0.0f, 1.0f ) );
    g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( DXUTIsVsyncEnabled() ) );
    g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );

    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 0.0f, 0.0f, 1.0f ) );
    g_pTxtHelper->DrawFormattedTextLine( L"Source: %.1f, %.1f, %.1f",
                                         g_audioState.wavInstanceAr[3].emitter.Position.x, g_audioState.wavInstanceAr[3].emitter.Position.y,
                                         g_audioState.wavInstanceAr[3].emitter.Position.z );

    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 0.0f, 0.0f, 0.5f, 1.0f ) );
    g_pTxtHelper->DrawFormattedTextLine( L"Listener: %.1f, %.1f, %.1f",
                                         g_audioState.listener.Position.x, g_audioState.listener.Position.y,
                                         g_audioState.listener.Position.z );

    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );
    g_pTxtHelper->DrawTextLine( L"Coefficients:" );

    // Interpretation of channels depends on channel mask
    switch( g_audioState.dwChannelMask )
    {
        case SPEAKER_MONO:
            g_pTxtHelper->DrawFormattedTextLine( L" C: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[0] );
            break;

        case SPEAKER_STEREO:
            g_pTxtHelper->DrawFormattedTextLine( L" L: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[0] );
            g_pTxtHelper->DrawFormattedTextLine( L" R: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[1] );
            break;

        case SPEAKER_2POINT1:
            g_pTxtHelper->DrawFormattedTextLine( L" L: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[0] );
            g_pTxtHelper->DrawFormattedTextLine( L" R: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[1] );
            g_pTxtHelper->DrawFormattedTextLine( L" LFE: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[2] );
            break;

        case SPEAKER_SURROUND:
            g_pTxtHelper->DrawFormattedTextLine( L" L: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[0] );
            g_pTxtHelper->DrawFormattedTextLine( L" R: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[1] );
            g_pTxtHelper->DrawFormattedTextLine( L" C: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[2] );
            g_pTxtHelper->DrawFormattedTextLine( L" B: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[3] );
            break;

        case SPEAKER_QUAD:
            g_pTxtHelper->DrawFormattedTextLine( L" L: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[0] );
            g_pTxtHelper->DrawFormattedTextLine( L" R: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[1] );
            g_pTxtHelper->DrawFormattedTextLine( L" Lb: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[2] );
            g_pTxtHelper->DrawFormattedTextLine( L" Rb: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[3] );
            break;

        case SPEAKER_4POINT1:
            g_pTxtHelper->DrawFormattedTextLine( L" L: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[0] );
            g_pTxtHelper->DrawFormattedTextLine( L" R: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[1] );
            g_pTxtHelper->DrawFormattedTextLine( L" LFE: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[2] );
            g_pTxtHelper->DrawFormattedTextLine( L" Lb: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[3] );
            g_pTxtHelper->DrawFormattedTextLine( L" Rb: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[4] );
            break;

        case SPEAKER_5POINT1:
            g_pTxtHelper->DrawFormattedTextLine( L" L: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[0] );
            g_pTxtHelper->DrawFormattedTextLine( L" R: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[1] );
            g_pTxtHelper->DrawFormattedTextLine( L" C: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[2] );
            g_pTxtHelper->DrawFormattedTextLine( L" LFE: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[3] );
            g_pTxtHelper->DrawFormattedTextLine( L" Lb: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[4] );
            g_pTxtHelper->DrawFormattedTextLine( L" Rb: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[5] );
            break;

        case SPEAKER_7POINT1:
            g_pTxtHelper->DrawFormattedTextLine( L" L: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[0] );
            g_pTxtHelper->DrawFormattedTextLine( L" R: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[1] );
            g_pTxtHelper->DrawFormattedTextLine( L" C: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[2] );
            g_pTxtHelper->DrawFormattedTextLine( L" LFE: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[3] );
            g_pTxtHelper->DrawFormattedTextLine( L" Lb: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[4] );
            g_pTxtHelper->DrawFormattedTextLine( L" Rb: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[5] );
            g_pTxtHelper->DrawFormattedTextLine( L" Lfc: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[6] );
            g_pTxtHelper->DrawFormattedTextLine( L" Rfc: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[7] );
            break;

        case SPEAKER_5POINT1_SURROUND:
            g_pTxtHelper->DrawFormattedTextLine( L" L: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[0] );
            g_pTxtHelper->DrawFormattedTextLine( L" R: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[1] );
            g_pTxtHelper->DrawFormattedTextLine( L" C: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[2] );
            g_pTxtHelper->DrawFormattedTextLine( L" LFE: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[3] );
            g_pTxtHelper->DrawFormattedTextLine( L" Ls: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[4] );
            g_pTxtHelper->DrawFormattedTextLine( L" Rs: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[5] );
            break;

        case SPEAKER_7POINT1_SURROUND:
            g_pTxtHelper->DrawFormattedTextLine( L" L: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[0] );
            g_pTxtHelper->DrawFormattedTextLine( L" R: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[1] );
            g_pTxtHelper->DrawFormattedTextLine( L" C: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[2] );
            g_pTxtHelper->DrawFormattedTextLine( L" LFE: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[3] );
            g_pTxtHelper->DrawFormattedTextLine( L" Lb: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[4] );
            g_pTxtHelper->DrawFormattedTextLine( L" Rb: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[5] );
            g_pTxtHelper->DrawFormattedTextLine( L" Ls: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[6] );
            g_pTxtHelper->DrawFormattedTextLine( L" Rs: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[7] );
            break;

        default:
            // Generic channel output for non-standard channel masks
            g_pTxtHelper->DrawFormattedTextLine( L" [0]: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[0] );
            g_pTxtHelper->DrawFormattedTextLine( L" [1]: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[1] );
            g_pTxtHelper->DrawFormattedTextLine( L" [2]: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[2] );
            g_pTxtHelper->DrawFormattedTextLine( L" [3]: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[3] );
            g_pTxtHelper->DrawFormattedTextLine( L" [4]: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[4] );
            g_pTxtHelper->DrawFormattedTextLine( L" [5]: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[5] );
            g_pTxtHelper->DrawFormattedTextLine( L" [6]: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[6] );
            g_pTxtHelper->DrawFormattedTextLine( L" [7]: %.3f", g_audioState.wavInstanceAr[1].dspSettings.pMatrixCoefficients[7] );
            break;
    }

    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 0.5f, 0.5f, 0.5f, 1.0f ) );
    g_pTxtHelper->DrawFormattedTextLine( L"Distance: %.3f", g_audioState.wavInstanceAr[1].dspSettings.EmitterToListenerDistance );

    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 1.0f, 1.0f, 1.0f ) );
    g_pTxtHelper->DrawFormattedTextLine( L"Doppler factor: %.3f", g_audioState.wavInstanceAr[1].dspSettings.DopplerFactor );

    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 0.5f, 0.5f, 0.5f, 1.0f ) );
    g_pTxtHelper->DrawFormattedTextLine( L"LPF Direct: %.3f", g_audioState.wavInstanceAr[1].dspSettings.LPFDirectCoefficient );
    g_pTxtHelper->DrawFormattedTextLine( L"LPF Reverb: %.3f", g_audioState.wavInstanceAr[1].dspSettings.LPFReverbCoefficient );
    g_pTxtHelper->DrawFormattedTextLine( L"Reverb: %.3f", g_audioState.wavInstanceAr[1].dspSettings.ReverbLevel );
	g_pTxtHelper->DrawFormattedTextLine( L"Points: %d", pointCounter);
	g_pTxtHelper->DrawFormattedTextLine( L"Accel: %.3f", accel);
	g_pTxtHelper->DrawFormattedTextLine( L"iAccel: %.3f", iAccel);
	g_pTxtHelper->DrawFormattedTextLine( L"In Air: %d", inAir);

    g_pTxtHelper->End();
	*/
}


//--------------------------------------------------------------------------------------
// Rejects any D3D9 devices that aren't acceptable to the app by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D9DeviceAcceptable( D3DCAPS9* pCaps, D3DFORMAT AdapterFormat,
                                      D3DFORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    // Skip backbuffer formats that don't support alpha blending
    /*
	IDirect3D9* pD3D = DXUTGetD3D9Object();
    if( FAILED( pD3D->CheckDeviceFormat( pCaps->AdapterOrdinal, pCaps->DeviceType,
                                         AdapterFormat, D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING,
                                         D3DRTYPE_TEXTURE, BackBufferFormat ) ) )
        return false;

		*/
    // No fallback defined by this app, so reject any device that
    // doesn't support at least ps2.0
    /*
	if( pCaps->PixelShaderVersion < D3DPS_VERSION( 2, 0 ) )
        return false;
		*/
    return true;
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D10 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
	/*
    if( pDeviceSettings->ver == DXUT_D3D9_DEVICE )
    {
        IDirect3D9* pD3D = DXUTGetD3D9Object();
        D3DCAPS9 Caps;
        pD3D->GetDeviceCaps( pDeviceSettings->d3d9.AdapterOrdinal, pDeviceSettings->d3d9.DeviceType, &Caps );

        // If device doesn't support HW T&L or doesn't support 1.1 vertex shaders in HW
        // then switch to SWVP.
        if( ( Caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT ) == 0 ||
            Caps.VertexShaderVersion < D3DVS_VERSION( 1, 1 ) )
        {
            pDeviceSettings->d3d9.BehaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
        }

        // Debugging vertex shaders requires either REF or software vertex processing
        // and debugging pixel shaders requires REF.
#ifdef DEBUG_VS
        if( pDeviceSettings->d3d9.DeviceType != D3DDEVTYPE_REF )
        {
            pDeviceSettings->d3d9.BehaviorFlags &= ~D3DCREATE_HARDWARE_VERTEXPROCESSING;
            pDeviceSettings->d3d9.BehaviorFlags &= ~D3DCREATE_PUREDEVICE;
            pDeviceSettings->d3d9.BehaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
        }
#endif
#ifdef DEBUG_PS
        pDeviceSettings->d3d9.DeviceType = D3DDEVTYPE_REF;
#endif
    }

    // For the first device created if its a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        s_bFirstTime = false;
        if( ( DXUT_D3D9_DEVICE == pDeviceSettings->ver && pDeviceSettings->d3d9.DeviceType == D3DDEVTYPE_REF ) ||
            ( DXUT_D3D10_DEVICE == pDeviceSettings->ver &&
              pDeviceSettings->d3d10.DriverType == D3D10_DRIVER_TYPE_REFERENCE ) )
            DXUTDisplaySwitchingToREFWarning( pDeviceSettings->ver );
    }
	*/
    return true;
}


//--------------------------------------------------------------------------------------
// Create any D3D9 resources that will live through a device reset (D3DPOOL_MANAGED)
// and aren't tied to the back buffer size
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D9CreateDevice( IDirect3DDevice9* pd3dDevice, const D3DSURFACE_DESC* pBackBufferSurfaceDesc,
                                     void* pUserContext )
{
	
    HRESULT hr;
	/*
    V_RETURN( g_DialogResourceManager.OnD3D9CreateDevice( pd3dDevice ) );
    V_RETURN( g_SettingsDlg.OnD3D9CreateDevice( pd3dDevice ) );

    V_RETURN( D3DXCreateFont( pd3dDevice, 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET,
                              OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                              L"Arial", &g_pFont9 ) );

    // Read the D3DX effect file
    WCHAR str[MAX_PATH];
    DWORD dwShaderFlags = D3DXFX_NOT_CLONEABLE;
#ifdef DEBUG_VS
        dwShaderFlags |= D3DXSHADER_FORCE_VS_SOFTWARE_NOOPT;
    #endif
#ifdef DEBUG_PS
        dwShaderFlags |= D3DXSHADER_FORCE_PS_SOFTWARE_NOOPT;
    #endif
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"wavRide.fx" ) );
    V_RETURN( D3DXCreateEffectFromFile( pd3dDevice, str, NULL, NULL, dwShaderFlags,
                                        NULL, &g_pEffect9, NULL ) );

    //
    // Create render elements
    //

    // Define the vertex elements.
    D3DVERTEXELEMENT9 VertexElements[ 3 ] =
    {
        { 0,  0, D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
        { 0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    0 },
        D3DDECL_END()
    };

    // Create a vertex declaration from the element descriptions.
    V_RETURN( pd3dDevice->CreateVertexDeclaration( VertexElements, &g_pVertexDecl ) );

    // Create our vertex buffers
    V_RETURN( pd3dDevice->CreateVertexBuffer( 4 * sizeof( D3DVERTEX ), 0, 0, D3DPOOL_MANAGED, &g_pvbFloor, NULL ) );
	
	//V_RETURN( CreateSourceVBuffers(hr, pd3dDevice) );
	
    V_RETURN( pd3dDevice->CreateVertexBuffer( 3 * sizeof( D3DVERTEX ), 0, 0, D3DPOOL_MANAGED, &g_pvbListener, NULL ) );
    V_RETURN( pd3dDevice->CreateVertexBuffer( 7 * sizeof( D3DVERTEX ), 0, 0, D3DPOOL_MANAGED, &g_pvbListenerCone, NULL ) );
    V_RETURN( pd3dDevice->CreateVertexBuffer( 9 * sizeof( D3DVERTEX ), 0, 0, D3DPOOL_MANAGED, &g_pvbInnerRadius, NULL ) );

    const UINT lcount = 2 * ( ( ZMAX - ZMIN + 1 ) + ( XMAX - XMIN + 1 ) );
    V_RETURN( pd3dDevice->CreateVertexBuffer( 2 * sizeof( D3DVERTEX ) * lcount, 0, 0, D3DPOOL_MANAGED, &g_pvbGrid,
                                              NULL ) );

    D3DVERTEX* pVertices;
	*/
	/*
    // Fill the VB for the listener, listener cone, and inner radius
    V_RETURN( g_pvbListener->Lock( 0, 0, ( VOID** )&pVertices, 0 ) );
    pVertices[0].p = D3DXVECTOR3( -0.5f, -1.f, 0 );
    pVertices[0].c = LISTENER_COLOR;
    pVertices[1].p = D3DXVECTOR3( 0, 1.f, 0 );
    pVertices[1].c = LISTENER_COLOR;
    pVertices[2].p = D3DXVECTOR3( 0.5f, -1.f, 0 );
    pVertices[2].c = LISTENER_COLOR;
    g_pvbListener->Unlock();
    V_RETURN( g_pvbListenerCone->Lock( 0, 0, ( VOID** )&pVertices, 0 ) );
    pVertices[0].p = D3DXVECTOR3( -1.04f, -3.86f, 0 );
    pVertices[0].c = LISTENER_COLOR;
    pVertices[1].p = D3DXVECTOR3( 0, 0, 0 );
    pVertices[1].c = LISTENER_COLOR;
    pVertices[2].p = D3DXVECTOR3( -3.86f, 1.04f, 0 );
    pVertices[2].c = LISTENER_COLOR;
    pVertices[3].p = D3DXVECTOR3( 0, 0, 0 );
    pVertices[3].c = LISTENER_COLOR;
    pVertices[4].p = D3DXVECTOR3( 3.86f, 1.04f, 0 );
    pVertices[4].c = LISTENER_COLOR;
    pVertices[5].p = D3DXVECTOR3( 0, 0, 0 );
    pVertices[5].c = LISTENER_COLOR;
    pVertices[6].p = D3DXVECTOR3( 1.04f, -3.86f, 0 );
    pVertices[6].c = LISTENER_COLOR;
    g_pvbListenerCone->Unlock();
    V_RETURN( g_pvbInnerRadius->Lock( 0, 0, ( VOID** )&pVertices, 0 ) );
    pVertices[0].p = D3DXVECTOR3( 0.0f, -2.0f, 0 );
    pVertices[0].c = LISTENER_COLOR;
    pVertices[1].p = D3DXVECTOR3( 1.4f, -1.4f, 0 );
    pVertices[1].c = LISTENER_COLOR;
    pVertices[2].p = D3DXVECTOR3( 2.0f, 0.0f, 0 );
    pVertices[2].c = LISTENER_COLOR;
    pVertices[3].p = D3DXVECTOR3( 1.4f, 1.4f, 0 );
    pVertices[3].c = LISTENER_COLOR;
    pVertices[4].p = D3DXVECTOR3( 0.0f, 2.0f, 0 );
    pVertices[4].c = LISTENER_COLOR;
    pVertices[5].p = D3DXVECTOR3( -1.4f, 1.4f, 0 );
    pVertices[5].c = LISTENER_COLOR;
    pVertices[6].p = D3DXVECTOR3( -2.0f, 0.0f, 0 );
    pVertices[6].c = LISTENER_COLOR;
    pVertices[7].p = D3DXVECTOR3( -1.4f, -1.4f, 0 );
    pVertices[7].c = LISTENER_COLOR;
    pVertices[8].p = D3DXVECTOR3( 0.0f, -2.0f, 0 );
    pVertices[8].c = LISTENER_COLOR;
    g_pvbInnerRadius->Unlock();
	*/
    
	// Fill the VB for the source
	//V_RETURN( FillSourceVBuffers(hr, pVertices) );

	
	/*
		V_RETURN( g_pvbSourceA[0]->Lock( 0, 0, ( VOID** )&pVertices, 0 ) );
		pVertices[0].p = D3DXVECTOR3( -0.5f, -0.5f, 0 );
		pVertices[0].c = SOURCE_COLOR;
		pVertices[1].p = D3DXVECTOR3( -0.5f, 0.5f, 0 );
		pVertices[1].c = SOURCE_COLOR;
		pVertices[2].p = D3DXVECTOR3( 0.5f, -0.5f, 0 );
		pVertices[2].c = SOURCE_COLOR;
		pVertices[3].p = D3DXVECTOR3( 0.5f, 0.5f, 0 );
		pVertices[3].c = SOURCE_COLOR;
		g_pvbSourceA[0]->Unlock();

		V_RETURN( g_pvbSourceA[1]->Lock( 0, 0, ( VOID** )&pVertices, 0 ) );
		pVertices[0].p = D3DXVECTOR3( -0.5f, -0.5f, 0 );
		pVertices[0].c = SOURCE_COLOR;
		pVertices[1].p = D3DXVECTOR3( -0.5f, 0.5f, 0 );
		pVertices[1].c = SOURCE_COLOR;
		pVertices[2].p = D3DXVECTOR3( 0.5f, -0.5f, 0 );
		pVertices[2].c = SOURCE_COLOR;
		pVertices[3].p = D3DXVECTOR3( 0.5f, 0.5f, 0 );
		pVertices[3].c = SOURCE_COLOR;
		g_pvbSourceA[1]->Unlock();
	*/
	/*
    // Fill the VB for the floor
    V_RETURN( g_pvbFloor->Lock( 0, 0, ( VOID** )&pVertices, 0 ) );
    pVertices[0].p = D3DXVECTOR3( ( FLOAT )XMIN, ( FLOAT )ZMIN, 0 );
    pVertices[0].c = FLOOR_COLOR;
    pVertices[1].p = D3DXVECTOR3( ( FLOAT )XMIN, ( FLOAT )ZMAX, 0 );
    pVertices[1].c = FLOOR_COLOR;
    pVertices[2].p = D3DXVECTOR3( ( FLOAT )XMAX, ( FLOAT )ZMIN, 0 );
    pVertices[2].c = FLOOR_COLOR;
    pVertices[3].p = D3DXVECTOR3( ( FLOAT )XMAX, ( FLOAT )ZMAX, 0 );
    pVertices[3].c = FLOOR_COLOR;
    g_pvbFloor->Unlock();
	*/
	/*
    // Fill the VB for the grid
    INT i, j;
    V_RETURN( g_pvbGrid->Lock( 0, 0, ( VOID** )&pVertices, 0 ) );
    for( i = ZMIN, j = 0; i <= ZMAX; ++i, ++j )
    {
        pVertices[ j * 2 + 0 ].p = D3DXVECTOR3( ( FLOAT )XMIN, ( FLOAT )i, 0 );
        pVertices[ j * 2 + 0 ].c = GRID_COLOR;
        pVertices[ j * 2 + 1 ].p = D3DXVECTOR3( ( FLOAT )XMAX, ( FLOAT )i, 0 );
        pVertices[ j * 2 + 1 ].c = GRID_COLOR;
    }
    for( i = XMIN; i <= XMAX; ++i, ++j )
    {
        pVertices[ j * 2 + 0 ].p = D3DXVECTOR3( ( FLOAT )i, ( FLOAT )ZMIN, 0 );
        pVertices[ j * 2 + 0 ].c = GRID_COLOR;
        pVertices[ j * 2 + 1 ].p = D3DXVECTOR3( ( FLOAT )i, ( FLOAT )ZMAX, 0 );
        pVertices[ j * 2 + 1 ].c = GRID_COLOR;
    }
    g_pvbGrid->Unlock();
	*/
    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D9 resources that won't live through a device reset (D3DPOOL_DEFAULT)
// or that are tied to the back buffer size
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D9ResetDevice( IDirect3DDevice9* pd3dDevice,
                                    const D3DSURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
	
    HRESULT hr;
	/*
    V_RETURN( g_DialogResourceManager.OnD3D9ResetDevice() );
    V_RETURN( g_SettingsDlg.OnD3D9ResetDevice() );

    if( g_pFont9 ) V_RETURN( g_pFont9->OnResetDevice() );
    if( g_pEffect9 ) V_RETURN( g_pEffect9->OnResetDevice() );

    V_RETURN( D3DXCreateSprite( pd3dDevice, &g_pSprite9 ) );
    g_pTxtHelper = new CDXUTTextHelper( g_pFont9, g_pSprite9, NULL, NULL, 15 );

    g_HUD.SetLocation( pBackBufferSurfaceDesc->Width - 170, 0 );
    g_HUD.SetSize( 170, 170 );
    g_SampleUI.SetLocation( pBackBufferSurfaceDesc->Width - 170, pBackBufferSurfaceDesc->Height - 350 );
    g_SampleUI.SetSize( 170, 300 );
	*/
    return S_OK;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
    if( fElapsedTime > 0 )
    {
		static bool timeStartFlag = false;
		static bool gameStartFlag = false;
		static bool gameStopFlag = false;
		static bool jFlag = false;
		static int introState = 0;
		static int spawnCounter = -1;
		static int gameState = 0;
		static int introTimer = timeGetTime();
		static int traceTimer = 0;
		static int traceOffset = 0;
		static int traceCtr = 0;
		CalculateAccel(fElapsedTime);

		if(gameState==0){

			if(introState==4)
				UpdateWindIntro(traceOffset);
			else
				UpdateWindIntro(-1);
			UpdateFootStepsIntro();
			if(inAir==true)
				CheckJump();

			if(introState==0){
				g_audioState.wavInstanceAr[7].pSourceVoice->SetVolume(0.8f);
				g_audioState.wavInstanceAr[8].pSourceVoice->SetVolume(0.6f);
				g_audioState.wavInstanceAr[10].pSourceVoice->SetVolume(0.4f);
				if(timeGetTime() - introTimer > 5000){
					introState = 1;
					SpawnIntroVoice(0,16.0f);
					traceTimer = timeGetTime();
					traceCtr = 0;
				}
			}

			if(introState==1){
				traceOffset = timeGetTime() - traceTimer;
				if(traceOffset > 1000 && traceCtr == 0){
					SpawnIntroTracer(0,false);
					traceCtr = 1;
				}
				if(traceOffset > 2000 && traceCtr == 1){
					SpawnIntroTracer(0,true);
					traceCtr = 2;
				}
				if(traceOffset > 4000 && traceCtr == 2){
					SpawnIntroVoice(2,introVOffset);
					traceCtr = 3;
				}
				if(traceOffset > 5000 && traceCtr == 3){
					SpawnIntroTracer(2, false);
					traceCtr = 4;
				}
				if(traceOffset > 6000 && traceCtr == 4){
					SpawnIntroTracer(2, true);
					traceCtr = 5;
				}
				if(traceOffset > 8000 && traceCtr == 5){
					SpawnIntroVoice(0,introVOffset);
					traceCtr = 0;
					traceTimer = timeGetTime();
				}
				if(UpdateVoiceIntro(fElapsedTime)){
					introState = 2;
					deSpawnIntroVoice(0);
					deSpawnIntroVoice(2);
					SpawnIntroVoice(1,-16.0f);
					traceCtr = 0;
					traceTimer = timeGetTime();
				}
			}

			if(introState==2){
				traceOffset = timeGetTime() - traceTimer;
				if(traceOffset > 1000 && traceCtr == 0){
					SpawnIntroTracer(1,false);
					traceCtr = 1;
				}
				if(traceOffset > 2000 && traceCtr == 1){
					SpawnIntroTracer(1,true);
					traceCtr = 2;
				}
				if(traceOffset > 4000 && traceCtr == 2){
					SpawnIntroVoice(3,introVOffset);
					traceCtr = 3;
				}
				if(traceOffset > 5000 && traceCtr == 3){
					SpawnIntroTracer(3, false);
					traceCtr = 4;
				}
				if(traceOffset > 6000 && traceCtr == 4){
					SpawnIntroTracer(3, true);
					traceCtr = 5;
				}
				if(traceOffset > 8000 && traceCtr == 5){
					SpawnIntroVoice(1,introVOffset);
					traceCtr = 0;
					traceTimer = timeGetTime();
				}
				if(UpdateVoiceIntro(fElapsedTime)){
					introState = 3;
					deSpawnIntroVoice(1);
					deSpawnIntroVoice(3);
					SpawnIntroJump();
					traceCtr = 0;
					traceTimer = timeGetTime();
				}
			}

			if(introState==3){
				traceOffset = timeGetTime() - traceTimer;
				if(traceOffset > 3000 && traceCtr < 2){
					if(inAir==false)
						traceCtr = 1;
					if(traceCtr==1 && inAir==true){
						jumpSuccess = true;
						jumpDur = 4000;
						traceCtr = 2;
					}
				}
				if(traceOffset > 4000 && traceCtr < 2){
					PlayOneShot(6);
					preJumpNum++;
					SpawnIntroJump();
					traceCtr = 0;
					traceTimer = timeGetTime();
				}
				if(UpdateJumpIntro(fElapsedTime)){
					PlayOneShot(5);
					deSpawnIntroJump();
					introState = 4;
					traceCtr = 0;
					traceTimer = timeGetTime();
				}
			}

			if(introState==4){
				if(inAir==false){
					traceCtr = 0;
					traceOffset = timeGetTime() - traceTimer;
					gameState = 1;
					jumpSuccess = false;
					StartMusic();
				}
			}

		}//gameState==0

		else if(gameState==1){
			static int posTimer = timeGetTime();
			currPos += fElapsedTime * MOTION_SCALE * accel;
			activeMusicScalar -= fElapsedTime * 0.05f;
			if(timeGetTime() - posTimer > 500 && posCounter < 100){
				posTracker[posCounter] = currPos;
				posCounter++;
			}

			UpdateWind();
			UpdateSlide();
			UpdateTurnSlide();
			UpdateMusic();
			if(inAir==true)
				CheckJump();

			if(spawnCounter>=NUM_POINT)
				gameStopFlag = true;

			if(timeStartFlag)
				UpdateVoices(fElapsedTime);

			if(!timeStartFlag){
				timeStartFlag = true;
				g_timer = timeGetTime();
			}

			if(timeGetTime() - g_timer > 6000 && gameStartFlag==false){
				spawnCounter = 0;
				f_timer = timeGetTime() - 2000;
				gameStartFlag = true;
			}
			
			if(traceCtr==0){
				if(gameStopFlag==false){
					float tScale = ((float)spawnCounter)/((float)NUM_POINT);
					float tDur = 6.5f - (4.0f * tScale);
					int tSpacer = 3500 - (int)(3000.0f * tScale);

					if(timeGetTime() - f_timer > tSpacer && spawnCounter >= 0){
						SpawnVoice(spawnCounter % 5, 0.0f, tDur, 14);
						spawnCounter++;
						if(spawnCounter==8||spawnCounter==15)
							jFlag = false;
						if(spawnCounter==22)
							jFlag = false;
						f_timer = timeGetTime();
					}
				}
			}

			if(jFlag==false){
				if(spawnCounter > 0 && jumpPointCounter < NUM_JUMP){
					if((spawnCounter % 7 == 0) && traceCtr==0){
						jFlag = true;
						traceTimer = timeGetTime();
						traceCtr = 4;
					}
				}
			}

			if(traceCtr==4){
				traceOffset = timeGetTime() - traceTimer;
				if(traceOffset > 3000){
					SpawnJump();
					traceCtr = 1;
					traceTimer = timeGetTime();
				}
			}

			if(traceCtr==1||traceCtr==2){
				traceOffset = timeGetTime() - traceTimer;
				UpdateJump();
				if(traceOffset > 4000){
					PlayOneShot(6);
					StartJump(2000);
					jumpSuccess = false;
					deSpawnJump();
					traceCtr = 3;
					traceTimer = timeGetTime();
					activeMusicScalar -=1.0f;
				}
				else if(traceOffset > 3000){
					if(inAir==false && traceCtr==1)
						traceCtr = 2;
					if(inAir==true && traceCtr==2){
						PlayOneShot(5);
						jumpDur = 6000;
						jumpSuccess = true;
						activeMusicScalar += 1.2f;
						deSpawnJump();
						traceCtr = 3;
						traceTimer = timeGetTime();
					}
				}
			}

			if(traceCtr==3){
				traceOffset = timeGetTime() - traceTimer;
				if(jumpSuccess==true){
					if(inAir==false){
						traceCtr = 0;
						traceTimer = timeGetTime();
						f_timer = traceTimer - 4000;
						jumpSuccess = false;
					}
				}
				else{
					if(traceOffset > 4000){
						traceCtr = 0;
						traceTimer = timeGetTime();
						f_timer = traceTimer - 4000;
						jumpSuccess = false;
					}
				}
			}

			if(gameStopFlag==true)
				gameState = 2;

		}//gameState==1

		else if(gameState==2){

			static int endTimer = timeGetTime();

			UpdateWind();
			UpdateSlide();
			UpdateTurnSlide();
			UpdateMusic();
			UpdateVoices(fElapsedTime);

			if(timeGetTime() - endTimer > 5000){
				gameState = 3;
			}

		}//gameState==2

		else if(gameState==3){
			inAir = true;
			jumpDur = 6000;
			UpdateEndWind(0);
			StopSlide();
			StopTurnSlide();
			traceTimer = timeGetTime();
			gameState = 4;
		}//gameState==3
		
		else if(gameState==4){
			traceOffset = timeGetTime() - traceTimer;
			UpdateEndWind(traceOffset);
			CheckJump();
			if(traceOffset > 6000)
				gameState = 5;
		}//gameState==4

		if(gameState==5){
			WriteXML();
			gameState = 6;
			closeGame = true;
			traceTimer = timeGetTime();
		}//gameState==5

		if(gameState==6){
			traceOffset = timeGetTime() - traceTimer;
			if(traceOffset > 5000){
				StartGameOverVoice();
				closeGame = true;
				gameState = 7;
			}
		}//gameState==6;

		if(gameState==7){
			UpdateGameOverVoice();
		}
    }

    UpdateAudio( fElapsedTime );
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D9 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D9FrameRender( IDirect3DDevice9* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext )
{
    HRESULT hr;
	
    // If the settings dialog is being shown, then render it instead of rendering the app's scene
    if( g_SettingsDlg.IsActive() )
    {
        g_SettingsDlg.OnRender( fElapsedTime );
        return;
    }

    // Clear the render target and the zbuffer
    V( pd3dDevice->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_ARGB( 0, 45, 50, 170 ), 1.0f, 0 ) );

    // Render the scene
	//pd3dDevice->BeginScene()
	/*
    if( SUCCEEDED( pd3dDevice->BeginScene() ) )
    {

        V( g_pEffect9->SetTechnique( "RenderScene" ) );

        D3DXMATRIXA16 mScale;
        D3DXMatrixScaling( &mScale, 1.f / ( XMAX - XMIN ), 1.f / ( ZMAX - ZMIN ), 1 );

        DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"World" ); // These events are to help PIX identify what the code is doing

        UINT iPass, cPasses;
        V( g_pEffect9->Begin( &cPasses, 0 ) );

        V( pd3dDevice->SetVertexDeclaration( g_pVertexDecl ) );

        for( iPass = 0; iPass < cPasses; ++iPass )
        {
            V( g_pEffect9->BeginPass( iPass ) );

            V( g_pEffect9->SetMatrix( "g_mTransform", &mScale ) );
            V( g_pEffect9->CommitChanges() );
			/*
            // Draw the floor
            V( pd3dDevice->SetStreamSource( 0, g_pvbFloor, 0, sizeof( D3DVERTEX ) ) );
            V( pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2 ) );

            // Draw the grid
            V( pd3dDevice->SetStreamSource( 0, g_pvbGrid, 0, sizeof( D3DVERTEX ) ) );

            const UINT lcount = ( ( ZMAX - ZMIN + 1 ) + ( XMAX - XMIN + 1 ) );
            V( pd3dDevice->DrawPrimitive( D3DPT_LINELIST, 0, lcount ) );

            // Draw the listener
            {
                D3DXMATRIXA16 mTrans;
                D3DXMatrixTranslation( &mTrans, g_audioState.vListenerPos.x, g_audioState.vListenerPos.z, 0 );

                D3DXMATRIXA16 mRot;
                D3DXMatrixRotationZ( &mRot, -g_audioState.fListenerAngle );

                D3DXMATRIXA16 mat = mRot * mTrans * mScale;
                V( g_pEffect9->SetMatrix( "g_mTransform", &mat ) );
                V( g_pEffect9->CommitChanges() );

                pd3dDevice->SetStreamSource( 0, g_pvbListener, 0, sizeof( D3DVERTEX ) );
                pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 1 );
                if (g_audioState.fUseListenerCone)
                {
                    pd3dDevice->SetStreamSource( 0, g_pvbListenerCone, 0, sizeof( D3DVERTEX ) );
                    pd3dDevice->DrawPrimitive( D3DPT_LINESTRIP, 0, 6 );
                }
                if (g_audioState.fUseInnerRadius)
                {
                    pd3dDevice->SetStreamSource( 0, g_pvbInnerRadius, 0, sizeof( D3DVERTEX ) );
                    pd3dDevice->DrawPrimitive( D3DPT_LINESTRIP, 0, 8 );
                }
            }
			*/
			/*
			for(int i=0; i<NUM_VOICES; i++)
				DrawSource(hr, i, pd3dDevice, &mScale);
			*/
            // Draw the source, moved to DrawSource()
			/*
            {
                D3DXMATRIXA16 mTrans;
				//for(int g=0;g<8;g++){
					D3DXMatrixTranslation( &mTrans, g_audioState.wavInstanceAr[0].vEmitterPos.x,
						g_audioState.wavInstanceAr[0].vEmitterPos.z, 0 );

					D3DXMATRIXA16 mat = mTrans * mScale;
					V( g_pEffect9->SetMatrix( "g_mTransform", &mat ) );
					V( g_pEffect9->CommitChanges() );

					pd3dDevice->SetStreamSource( 0, g_pvbSourceA[0], 0, sizeof( D3DVERTEX ) );
					pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2 );

			}

			{
				D3DXMATRIXA16 mTrans;
				//for(int g=0;g<8;g++){
					D3DXMatrixTranslation( &mTrans, g_audioState.wavInstanceAr[1].vEmitterPos.x,
						g_audioState.wavInstanceAr[1].vEmitterPos.z, 0 );

					D3DXMATRIXA16 mat = mTrans * mScale;
					V( g_pEffect9->SetMatrix( "g_mTransform", &mat ) );
					V( g_pEffect9->CommitChanges() );

					pd3dDevice->SetStreamSource( 0, g_pvbSourceA[1], 0, sizeof( D3DVERTEX ) );
					pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2 );
				//}
            }
			*/
			/*
            V( g_pEffect9->EndPass() );
        }
		
        V( g_pEffect9->End() );

        DXUT_EndPerfEvent();

        DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" ); // These events are to help PIX identify what the code is doing
        RenderText();
        V( g_HUD.OnRender( fElapsedTime ) );
        V( g_SampleUI.OnRender( fElapsedTime ) );
        DXUT_EndPerfEvent();
		*/
        //V( pd3dDevice->EndScene() );
    
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing,
                          void* pUserContext )
{
    // We use a simple sound focus model of not hearing the sound if the application is full-screen and minimized
    if( uMsg == WM_ACTIVATEAPP )
    {
        if( wParam == TRUE && !DXUTIsActive() ) // Handle only if previously not active
        {
            if( !DXUTIsWindowed() )
                PauseAudio( true );
        }
        else if( wParam == FALSE && DXUTIsActive() ) // Handle only if previously active
        {
            if( !DXUTIsWindowed() )
                PauseAudio( false );
        }
    }

    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass messages to settings dialog if its active
    if( g_SettingsDlg.IsActive() )
    {
        g_SettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;
    *pbNoFurtherProcessing = g_SampleUI.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
    switch( nChar )
    {
		case 32:
			if( bKeyDown ){
				if(inAir==false){
					StartJump(1000);
					/*
					airTimer = timeGetTime();
					inAir = true;
					PlayOneShot(6);
					*/
				}
			}
			break;

        case 'W':
        case 'w':
            if( bKeyDown )
                g_moveFlags |= FLAG_MOVE_UP;
            else
                g_moveFlags &= ~FLAG_MOVE_UP;
            break;

        case 'D':
        case 'd':
            if( bKeyDown )
                g_moveFlags |= FLAG_MOVE_LEFT;
            else
                g_moveFlags &= ~FLAG_MOVE_LEFT;
            break;

        case 'A':
        case 'a':
            if( bKeyDown )
                g_moveFlags |= FLAG_MOVE_RIGHT;
            else
                g_moveFlags &= ~FLAG_MOVE_RIGHT;
            break;

        case 'S':
        case 's':
            if( bKeyDown )
                g_moveFlags |= FLAG_MOVE_DOWN;
            else
                g_moveFlags &= ~FLAG_MOVE_DOWN;
            break;
    }

}


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{
	/*
    CDXUTComboBox* pComboBox = NULL;

    switch( nControlID )
    {
        case IDC_TOGGLEFULLSCREEN:
            DXUTToggleFullScreen(); break;
        case IDC_TOGGLEREF:
            DXUTToggleREF(); break;
        case IDC_CHANGEDEVICE:
            g_SettingsDlg.SetActive( !g_SettingsDlg.IsActive() ); break;

        case IDC_SOUND:
            pComboBox = ( CDXUTComboBox* )pControl;
            PrepareAudio( g_SOUND_NAMES[ PtrToInt( pComboBox->GetSelectedData() ) ], 0 );
            break;

        case IDC_CONTROL_MODE:
            pComboBox = ( CDXUTComboBox* )pControl;
            g_eControlMode = ( CONTROL_MODE )( int )PtrToInt( pComboBox->GetSelectedData() );
            break;

        case IDC_PRESET:
            pComboBox = ( CDXUTComboBox* )pControl;
            SetReverb( ( int )PtrToInt( pComboBox->GetSelectedData() ) );
            break;

        case IDC_UP:
        {
            D3DXVECTOR3* vec =
                ( g_eControlMode == CONTROL_LISTENER ) ? &g_audioState.vListenerPos : &g_audioState.wavInstanceAr[0].vEmitterPos;
            vec->z += 0.5f;
            vec->z = min( float( ZMAX ), vec->z );
        }
            break;

        case IDC_LEFT:
        {
            D3DXVECTOR3* vec =
                ( g_eControlMode == CONTROL_LISTENER ) ? &g_audioState.vListenerPos : &g_audioState.wavInstanceAr[0].vEmitterPos;
            vec->x -= 0.5f;
            vec->x = max( float( XMIN ), vec->x );
        }
            break;

        case IDC_RIGHT:
        {
            D3DXVECTOR3* vec =
                ( g_eControlMode == CONTROL_LISTENER ) ? &g_audioState.vListenerPos : &g_audioState.wavInstanceAr[0].vEmitterPos;
            vec->x += 0.5f;
            vec->x = min( float( XMAX ), vec->x );
        }
            break;

        case IDC_DOWN:
        {
            D3DXVECTOR3* vec =
                ( g_eControlMode == CONTROL_LISTENER ) ? &g_audioState.vListenerPos : &g_audioState.wavInstanceAr[0].vEmitterPos;
            vec->z -= 0.5f;
            vec->z = max( float( ZMIN ), vec->z );
        }
            break;
        case IDC_LISTENERCONE:
        {
            g_audioState.fUseListenerCone = !g_audioState.fUseListenerCone;
        }
            break;
        case IDC_INNERRADIUS:
        {
            g_audioState.fUseInnerRadius = !g_audioState.fUseInnerRadius;
        }
            break;
    }
	*/
}


//--------------------------------------------------------------------------------------
// Release D3D9 resources created in the OnD3D9ResetDevice callback
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D9LostDevice( void* pUserContext )
{
    g_moveFlags = 0;

    g_DialogResourceManager.OnD3D9LostDevice();
    g_SettingsDlg.OnD3D9LostDevice();
    if( g_pFont9 ) g_pFont9->OnLostDevice();
    if( g_pEffect9 ) g_pEffect9->OnLostDevice();
    SAFE_RELEASE( g_pSprite9 );
    SAFE_DELETE( g_pTxtHelper );
}


//--------------------------------------------------------------------------------------
// Release D3D9 resources created in the OnD3D9CreateDevice callback
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D9DestroyDevice( void* pUserContext )
{
    g_DialogResourceManager.OnD3D9DestroyDevice();
    g_SettingsDlg.OnD3D9DestroyDevice();
	SAFE_RELEASE( g_pEffect9 );
    SAFE_RELEASE( g_pFont9 ); 
    SAFE_RELEASE( g_pVertexDecl );
    SAFE_RELEASE( g_pvbFloor );
	SAFE_RELEASE( g_pvbSourceWind );
	for(int h=0;h<NUM_VOICES;h++){
		SAFE_RELEASE( g_pvbSourceA[h] );
	}
    SAFE_RELEASE( g_pvbListener );
    SAFE_RELEASE( g_pvbListenerCone );
    SAFE_RELEASE( g_pvbInnerRadius );
    SAFE_RELEASE( g_pvbGrid );
}

HRESULT CreateSourceVBuffers(HRESULT &hr, IDirect3DDevice9* pd3dDevice){
	for(int i=0;i<NUM_VOICES;i++)
		V_RETURN( pd3dDevice->CreateVertexBuffer( 4 * sizeof( D3DVERTEX ), 0, 0, D3DPOOL_MANAGED, &g_pvbSourceA[i], NULL ) );

	return S_OK;
}

HRESULT FillSourceVBuffers(HRESULT &hr, D3DVERTEX* pVertices){
	for(int i=0; i<NUM_VOICES;i++){
		//if(g_sourceAttr[i].isActive){
			DWORD nuColor = SOURCE_COLOR + (DWORD)(5000*(i));
			V_RETURN( g_pvbSourceA[i]->Lock( 0, 0, ( VOID** )&pVertices, 0 ) );
			pVertices[0].p = D3DXVECTOR3( -0.5f, -0.5f, 0 );
			pVertices[0].c = nuColor;
			pVertices[1].p = D3DXVECTOR3( -0.5f, 0.5f, 0 );
			pVertices[1].c = nuColor;
			pVertices[2].p = D3DXVECTOR3( 0.5f, -0.5f, 0 );
			pVertices[2].c = nuColor;
			pVertices[3].p = D3DXVECTOR3( 0.5f, 0.5f, 0 );
			pVertices[3].c = nuColor;
			g_pvbSourceA[i]->Unlock();
		//}
		/*
		else{
			V_RETURN( g_pvbSourceA[i]->Lock( 0, 0, ( VOID** )&pVertices, 0 ) );
			pVertices[0].p = D3DXVECTOR3( 0.0f, 0.0f, 0 );
			pVertices[0].c = SOURCE_COLOR;
			pVertices[1].p = D3DXVECTOR3( 0.0f, 0.0f, 0 );
			pVertices[1].c = SOURCE_COLOR;
			pVertices[2].p = D3DXVECTOR3( 0.0f, 0.0f, 0 );
			pVertices[2].c = SOURCE_COLOR;
			pVertices[3].p = D3DXVECTOR3( 0.0f, 0.0f, 0 );
			pVertices[3].c = SOURCE_COLOR;
			g_pvbSourceA[i]->Unlock();
		}
		*/
	}
	
	return S_OK;
}

void CalculateAccel(float fElapsedTime){
	if( g_moveFlags & FLAG_MOVE_LEFT ){
		if(!inAir)
			accel -= fElapsedTime * MOTION_SCALE * 0.8f;
		else
			accel -= fElapsedTime * MOTION_SCALE * 0.3f;
		accel = max( accel, -2.0f);
	}
	else if( g_moveFlags & FLAG_MOVE_RIGHT ){
		if(!inAir)
			accel += fElapsedTime * MOTION_SCALE * 0.8f;
		else
			accel += fElapsedTime * MOTION_SCALE * 0.3f;
		accel = min( accel, 2.0f);
	}
	else{
		if(abs(accel)<0.05f)
			accel = 0.0f;
		else{
			if(!inAir)
				accel -= (accel/abs(accel)) * fElapsedTime * MOTION_SCALE * 0.3f;
			else
				accel -= (accel/abs(accel)) * fElapsedTime * MOTION_SCALE * 0.15f;
		}
	}
	iAccel = accel * 0.5f;
}

void UpdateWind(){
	D3DXVECTOR3* vecWind =
        ( g_eControlMode == CONTROL_LISTENER ) ? &g_audioState.vListenerPos : &g_audioState.wavInstanceAr[0].vEmitterPos;
	vecWind->x = (0.6f * accel);
	vecWind->y = (-0.15f * abs(accel));
	if(inAir==true)
		vecWind->y += ((float)(timeGetTime() - airTimer) - ((float)jumpDur * 0.5f)) / ((float)jumpDur * 0.5f);
	//not sure about the z movement of wind perspective yet
	//vec2->z = 0.01f - (0.0025f * abs(accel));
	vecWind->z = 0.0f;
}

void UpdateSlide(){
	D3DXVECTOR3* vecSlide =
        ( g_eControlMode == CONTROL_LISTENER ) ? &g_audioState.vListenerPos : &g_audioState.wavInstanceAr[1].vEmitterPos;
	vecSlide->x = (0.65f * accel);
	if(inAir==false)
		vecSlide->y = -0.75f;
	else
		vecSlide->y = -30.0f;
	//not sure about the z movement of wind perspective yet
	//vec2->z = 0.01f - (0.0025f * abs(accel));
	vecSlide->z = -0.08f + (0.04f * abs(accel));;
}

void UpdateTurnSlide(){
	static int turnTimer = timeGetTime();
	if(turnFlag==false&&abs(accel)>1.5f){
		turnFlag = true;
		turnTimer = timeGetTime();
	}
	else if(turnFlag==true&&abs(accel)<0.5f)
		turnFlag = false;

	D3DXVECTOR3* vecSlide =
        ( g_eControlMode == CONTROL_LISTENER ) ? &g_audioState.vListenerPos : &g_audioState.wavInstanceAr[2].vEmitterPos;
	vecSlide->x = (0.75f * accel);
	vecSlide->y = -0.6f;
	float volScalar = 1.2f - (0.001f * (float)(timeGetTime() - turnTimer));
	if(volScalar<0.0f)
		volScalar = 0.0f;
	else if(volScalar>1.1f)
		volScalar = abs(volScalar - 1.2f) * 11.0f;
	if(turnFlag==true)
		g_audioState.wavInstanceAr[2].pSourceVoice->SetVolume(volScalar);
	else
		g_audioState.wavInstanceAr[2].pSourceVoice->SetVolume(0.0f);
	if(inAir==true)
		vecSlide->y = -30.0f;
	//not sure about the z movement of wind perspective yet
	//vec2->z = 0.01f - (0.0025f * abs(accel));
	vecSlide->z = -0.1f + (0.1f * abs(accel));;
}

bool SpawnVoice(int index, float delay, float duration, int width){
	if(g_sourceAttr[index].isActive==true)
		return false;
	pointCounter++;
	PlayOneShot(g_sourceAttr[index].wavIndex);
	g_sourceAttr[index].isActive = true;
	g_sourceAttr[index].isObs = false;
	g_sourceAttr[index].pointFlag = false;
	g_sourceAttr[index].delayTimer = delay;
	g_sourceAttr[index].duration = duration;
	g_sourceAttr[index].srcTimer = duration;
	g_sourceAttr[index].xOffset = 
		(float)((rand() % (width - ((width % 2) - 1))) - (width / 2)) + accel;
	g_sourceAttr[index].vType = TARGET;
	return true;
}

bool SpawnTracers(int index){
	int cI = index + 10;
	int fI = index + 5;
	if(fI<0||cI>NUM_VOICES)
		return false;
	if(g_sourceAttr[cI].isObs==true)
		return false;
	if(g_sourceAttr[fI].isObs==true)
		return false;
	PlayOneShot(g_sourceAttr[cI].wavIndex);
	PlayOneShot(g_sourceAttr[fI].wavIndex);
	g_sourceAttr[cI].isActive = true;
	g_sourceAttr[fI].isActive = true;
	g_sourceAttr[cI].isObs = false;
	g_sourceAttr[fI].isObs = false;
	g_sourceAttr[cI].vType = C_TRACER;
	g_sourceAttr[fI].vType = F_TRACER;
	g_sourceAttr[index].cTracerPtr = &g_sourceAttr[cI];
	g_sourceAttr[index].fTracerPtr = &g_sourceAttr[fI];
	return true;
}

void ClearVoice(SOURCE_ATTR* sV){
	sV->isActive = false;
	sV->isObs = false;
	sV->pointFlag = false;
	sV->delayTimer = 0.0f;
	sV->srcTimer = 0.0f;
	sV->xOffset = 0.0f;
	sV->duration = 0.0f;
	if(sV->vType==TARGET){
		if(sV->cTracerPtr!=NULL)
			ClearVoice(sV->cTracerPtr);
		if(sV->fTracerPtr!=NULL)
			ClearVoice(sV->fTracerPtr);
	}
	sV->cTracerPtr = NULL;
	sV->fTracerPtr = NULL;
	sV->vType = BLANK;
}

void UpdateVoices(float fElapsedTime){
	for(int i=0;i<NUM_VOICES;i++){


		//main pos updates
		if(g_sourceAttr[i].isActive==true&&g_sourceAttr[i].vType==TARGET){

			if(g_sourceAttr[i].delayTimer > 0.0f){
				g_sourceAttr[i].delayTimer -= fElapsedTime;
			}

			else if(g_sourceAttr[i].srcTimer <= 0.0f){
				ClearVoice(&g_sourceAttr[i]);
			}

			else{
				if(g_sourceAttr[i].isObs==false){
					SpawnTracers(i);
					g_sourceAttr[i].isObs = true;
				}
				//pos scaling goes here
				float zScale = ((g_sourceAttr[i].srcTimer / 
					g_sourceAttr[i].duration)* 2.0f) - 1.0f;

				g_sourceAttr[i].srcTimer -= fElapsedTime;
				D3DXVECTOR3* vec =
					( g_eControlMode == CONTROL_LISTENER ) ?
					&g_audioState.vListenerPos :
					&g_audioState.wavInstanceAr[i+NUM_PRE_VOICES].vEmitterPos;
				if(g_sourceAttr[i].xOffset!=0.0f){
					vec->x = g_sourceAttr[i].xOffset;
					g_sourceAttr[i].xOffset = 0.0f;
				}
				vec->x += fElapsedTime * (MOTION_SCALE*1.75f) * (accel/2.0f);
				vec->x = (vec->x >= 0) ?
					min( float(XMAX), vec->x ) :
					max( float(XMIN), vec->x );
				if(zScale>=0.0f){
					vec->z = zScale * (float)ZMAX * 0.8f;
					vec->z = min( float( ZMAX ), vec->z );
				}
				else{
					vec->z = zScale * (float)ZMIN * (-1);
					vec->z = max( float( ZMIN ), vec->z );
				}
				if((vec->z>0) && (g_sourceAttr[i].pointFlag==true)){
					g_sourceAttr[i].pointFlag = false;
				}
				else if((vec->z<=0) && (g_sourceAttr[i].pointFlag==false)){
					g_sourceAttr[i].pointFlag = true;
					if(vec->x<4&&vec->x>(-4)){
						points[pointCounter] = 1;
						activeMusicScalar += 0.3;
						PlayOneShot(5);
					}
					else{
						PlayOneShot(6);
					}
				}
				//vec->y = -8.0f + (abs(vec->x) * 0.5f);
				vec->y = 0.0f;
				UpdateTracers(i, vec->x, vec->z);
			}
		}//end main pos updates

	}
}

bool UpdateTracers(int i, float xT, float zT){
	if(g_sourceAttr[i].cTracerPtr==NULL||g_sourceAttr[i].fTracerPtr==NULL)
		return false;
	SOURCE_ATTR* cV = g_sourceAttr[i].cTracerPtr;
	SOURCE_ATTR* fV = g_sourceAttr[i].fTracerPtr;
	cV->isObs = true;
	fV->isObs = true;
	D3DXVECTOR3* cVec = &g_audioState.wavInstanceAr[cV->wavIndex].vEmitterPos;
	D3DXVECTOR3* fVec = &g_audioState.wavInstanceAr[fV->wavIndex].vEmitterPos;
	cVec->x = xT * (0.6f);
	fVec->x = xT * (1.4f);
	if(zT>0){
		/*
		float zRatio = zT/ZMAX;
		if(zRatio>0.75f)
			cVec->z = fVec->z = (1.0f - ((1.0f - zRatio) * 3.0f)) * ZMAX;
		else
			cVec->z = fVec->z = (zRatio / 3.0f) * ZMAX;
		*/
		cVec->z = 1.4f * zT;
		fVec->z = 0.6f * zT;
	}
	else{
		float zRatio = zT/ZMIN;
		cVec->z = fVec->z = zRatio * ZMIN;
	}
	//cVec->y = -8.0f + (abs(cVec->x) * 0.5f);
	//fVec->y = -8.0f + (abs(fVec->x) * 0.5f);
	cVec->y = 0.0f;
	fVec->y = 0.0f;
	return true;
}

bool DrawSource(HRESULT &hr, int index, IDirect3DDevice9* pd3dDevice, const D3DXMATRIXA16* mScale){
	if(!g_sourceAttr[index].isActive)
		return false;
	if(!g_sourceAttr[index].isObs)
		return false;

	D3DXMATRIXA16 mTrans;
	D3DXMatrixTranslation( &mTrans, 
		g_audioState.wavInstanceAr[index + NUM_PRE_VOICES].vEmitterPos.x,
		g_audioState.wavInstanceAr[index + NUM_PRE_VOICES].vEmitterPos.z, 0 );

	D3DXMATRIXA16 mat = mTrans * (*mScale);
	V( g_pEffect9->SetMatrix( "g_mTransform", &mat ) );
	V( g_pEffect9->CommitChanges() );

	pd3dDevice->SetStreamSource( 0, g_pvbSourceA[index], 0, sizeof( D3DVERTEX ) );
	pd3dDevice->DrawPrimitive( D3DPT_TRIANGLESTRIP, 0, 2 );
	
	return true;
}

void UpdateMusic(){
	D3DXVECTOR3* vecWind =
        ( g_eControlMode == CONTROL_LISTENER ) ? &g_audioState.vListenerPos : &g_audioState.wavInstanceAr[3].vEmitterPos;
	vecWind->x = 0.0f;
	vecWind->y = 0.0f;
	//not sure about the z movement of wind perspective yet
	//vec2->z = 0.01f - (0.0025f * abs(accel));
	vecWind->z = 0.0f;
	D3DXVECTOR3* vecWind2 =
        ( g_eControlMode == CONTROL_LISTENER ) ? &g_audioState.vListenerPos : &g_audioState.wavInstanceAr[4].vEmitterPos;
	vecWind2->x = 0.0f;
	vecWind2->y = 0.0f;
	vecWind2->z = 0.0f;
	if(inAir==true)
		vecWind->y -= ((float)(timeGetTime() - airTimer)) / ((float)jumpDur * 0.01f);
	g_audioState.wavInstanceAr[3].pSourceVoice->SetVolume(0.9f);
	if(activeMusicScalar > 2.0f)
		activeMusicScalar = 2.0f;
	else if(activeMusicScalar < 0.0f)
		activeMusicScalar = 0.0f;
	g_audioState.wavInstanceAr[4].pSourceVoice->SetVolume(activeMusicScalar * 0.5f);
	//g_audioState.wavInstanceAr[4].pSourceVoice->SetVolume(0.0f);
}

void PlayOneShot(int index){
	D3DXVECTOR3* vecO =
        ( g_eControlMode == CONTROL_LISTENER ) ? &g_audioState.vListenerPos : &g_audioState.wavInstanceAr[index].vEmitterPos;
	if(index<NUM_PRE_VOICES){
		vecO->x = 0.0f;
		vecO->y = 0.0f;
		vecO->z = 0.0f;
	}
	if(index>=5&&index<=8)
		g_audioState.wavInstanceAr[index].pSourceVoice->SetVolume(0.75f);
	else if(index>11)
		g_audioState.wavInstanceAr[index].pSourceVoice->SetVolume(0.65f);
	g_audioState.wavInstanceAr[index].playNewLoop(index);
}

void StartJump(int jumpTime){
	airTimer = timeGetTime();
	inAir = true;
	PlayOneShot(7);
	jumpDur = jumpTime;
}

void CheckJump(){
	if(timeGetTime() - airTimer > jumpDur && inAir == true){
		inAir = false;
		PlayOneShot(8);
	}
}



void UpdateWindIntro(int wOffset){
	D3DXVECTOR3* vecWind =
        ( g_eControlMode == CONTROL_LISTENER ) ? &g_audioState.vListenerPos : &g_audioState.wavInstanceAr[0].vEmitterPos;
	if(wOffset==-1){
	vecWind->x = (-3.0f * iAccel);
	vecWind->y = 6.0f;
	if(inAir==true)
		vecWind->y += ((float)(timeGetTime() - airTimer) - ((float)jumpDur * 0.5f)) / ((float)jumpDur * 0.5f);
	//not sure about the z movement of wind perspective yet
	//vec2->z = 0.01f - (0.0025f * abs(accel));
	vecWind->z = 0.0f;
	}
	else{
		vecWind->x = 3.0f * iAccel;
		vecWind->y = 6.0f - (6.0f * (((float)wOffset) / 4000.0f));
		vecWind->z = 0.0f;
	}
}

void UpdateFootStepsIntro(){
	D3DXVECTOR3* vecF =
        ( g_eControlMode == CONTROL_LISTENER ) ? &g_audioState.vListenerPos : &g_audioState.wavInstanceAr[9].vEmitterPos;
	g_audioState.wavInstanceAr[9].pSourceVoice->SetVolume(1.5f * abs(iAccel));
	vecF->x = (- 0.8f * iAccel);
	vecF->y = 0.0f;
	if(inAir==true)
		g_audioState.wavInstanceAr[9].pSourceVoice->SetVolume(0.0f);
	//not sure about the z movement of wind perspective yet
	//vec2->z = 0.01f - (0.0025f * abs(accel));
	vecF->z = 0.0f;
}

bool SpawnIntroVoice(int index, float position){
	//if(g_sourceAttr[index].isActive==true)
		//return false;
	PlayOneShot(g_sourceAttr[index].wavIndex);
	g_sourceAttr[index].isActive = true;
	g_sourceAttr[index].isObs = false;
	g_sourceAttr[index].srcTimer = 2.0f;
	g_sourceAttr[index].xOffset = position;
	g_sourceAttr[index].vType = TARGET;
	return true;
}

bool UpdateVoiceIntro(float fElapsedTime){
	for(int i=0;i<NUM_VOICES;i++){
		//main pos updates
		if(g_sourceAttr[i].isActive==true&&g_sourceAttr[i].vType==TARGET){
			if(g_sourceAttr[i].isObs==false){
				g_sourceAttr[i].isObs = true;
			}
			if(g_sourceAttr[i].srcTimer > 0.0f){
				g_sourceAttr[i].srcTimer -= fElapsedTime;
				if(g_sourceAttr[i].srcTimer < 0.0f)
					g_sourceAttr[i].srcTimer = 0.0f;
			}
			D3DXVECTOR3* vec =
				( g_eControlMode == CONTROL_LISTENER ) ?
				&g_audioState.vListenerPos :
				&g_audioState.wavInstanceAr[i+NUM_PRE_VOICES].vEmitterPos;
				if(g_sourceAttr[i].xOffset!=0.0f){
					vec->x = g_sourceAttr[i].xOffset;
					g_sourceAttr[i].xOffset = 0.0f;
				}
				vec->x += fElapsedTime * (MOTION_SCALE*0.4f) * (iAccel/2.0f);
				vec->x = (vec->x >= 0) ?
					min( float(XMAX * 1.1f), vec->x ) :
					max( float(XMIN * 1.1f), vec->x );
				introVOffset = vec->x;
				vec->y = 0.0f;				
				vec->z = 20.0f * g_sourceAttr[i].srcTimer;
				UpdateIntroTracer(i, vec->x, vec->z, fElapsedTime, true);
				UpdateIntroTracer(i, vec->x, vec->z, fElapsedTime, false);
				if(vec->x<1&&vec->x>(-1)){
						PlayOneShot(5);
						return true;
				}				
			}
		}//end main pos updates
	return false;
}

bool SpawnIntroTracer(int index, bool c){
	if(c==true){
		int cI = index + 10;
		if(cI>NUM_VOICES)
			return false;
		//if(g_sourceAttr[cI].isObs==true)
			//return false;
		PlayOneShot(g_sourceAttr[cI].wavIndex);
		g_sourceAttr[cI].isActive = true;
		g_sourceAttr[cI].isObs = false;
		g_sourceAttr[cI].vType = C_TRACER;
		g_sourceAttr[cI].srcTimer = 2.0f;
		g_sourceAttr[index].cTracerPtr = &g_sourceAttr[cI];
		return true;
	}
	else{
		int fI = index + 5;
		if(fI<0)
			return false;
		//if(g_sourceAttr[fI].isObs==true)
			//return false;
		PlayOneShot(g_sourceAttr[fI].wavIndex);
		g_sourceAttr[fI].isActive = true;
		g_sourceAttr[fI].isObs = false;
		g_sourceAttr[fI].vType = F_TRACER;
		g_sourceAttr[fI].srcTimer = 2.0f;
		g_sourceAttr[index].fTracerPtr = &g_sourceAttr[fI];
		return true;
	}
}

void deSpawnIntroVoice(int index){
	D3DXVECTOR3* vec =
				( g_eControlMode == CONTROL_LISTENER ) ?
				&g_audioState.vListenerPos :
				&g_audioState.wavInstanceAr[g_sourceAttr[index].wavIndex].vEmitterPos;
	vec->x = 0.0f;
	vec->y = 0.0f;
	vec->z = ZMIN;
	UpdateIntroTracer(index, vec->x, vec->z, 0.01f, true);
	UpdateIntroTracer(index, vec->x, vec->z, 0.01f, false);
	ClearVoice(&g_sourceAttr[index]);
}
void SpawnIntroJump(){
	PlayOneShot(10);
}
bool UpdateJumpIntro(int fElapsedTime){
	D3DXVECTOR3* vec =
		&g_audioState.wavInstanceAr[10].vEmitterPos;
	vec->x = 0.0f;
	vec->y = 0.0f;
	vec->z = 0.0f;
	if(jumpSuccess==true){
		jumpSuccess = false;
		return true;
	}
	return false;
}
void deSpawnIntroJump(){
	D3DXVECTOR3* vec =
		&g_audioState.wavInstanceAr[10].vEmitterPos;
	vec->x = 0.0f;
	vec->y = 0.0f;
	vec->z = ZMIN;
}

bool UpdateIntroTracer(int i, float xT, float zT, float fElapsedTime, bool c){
	if(c){
		if(g_sourceAttr[i].cTracerPtr==NULL)
			return false;
		SOURCE_ATTR* cV = g_sourceAttr[i].cTracerPtr;
		cV->isObs = true;
		if(cV->srcTimer > 0.0f){
			cV->srcTimer -= fElapsedTime;
			if(cV->srcTimer < 0.0f)
				cV->srcTimer = 0.0f;
		}
		D3DXVECTOR3* cVec = &g_audioState.wavInstanceAr[cV->wavIndex].vEmitterPos;
		cVec->x = xT * (0.75f);
		cVec->y = 0.0f;
		if(zT >= 0.0f)
			cVec->z = 20.0f * cV->srcTimer;
		else
			cVec->z = zT;
		return true;
	}
	else{
		if(g_sourceAttr[i].fTracerPtr==NULL)
			return false;
		SOURCE_ATTR* fV = g_sourceAttr[i].fTracerPtr;
		fV->isObs = true;
		if(fV->srcTimer > 0.0f){
			fV->srcTimer -= fElapsedTime;
			if(fV->srcTimer < 0.0f)
				fV->srcTimer = 0.0f;
		}
		D3DXVECTOR3* fVec = &g_audioState.wavInstanceAr[fV->wavIndex].vEmitterPos;
		fVec->x = xT * (1.25f);
		fVec->y = 0.0f;
		if(zT >= 0.0f)
			fVec->z = 20.0f * fV->srcTimer;
		else
			fVec->z = zT;
		return true;
	}
}

void StartMusic(){
	g_audioState.wavInstanceAr[3].pSourceVoice->Start(0);
	g_audioState.wavInstanceAr[4].pSourceVoice->Start(0);
}

void StopSlide(){
	g_audioState.wavInstanceAr[1].pSourceVoice->SetVolume(0.0f);
}

void StopTurnSlide(){
	g_audioState.wavInstanceAr[2].pSourceVoice->SetVolume(0.0f);
}

void UpdateEndWind(int wOffset){
	D3DXVECTOR3* vecWind =
        ( g_eControlMode == CONTROL_LISTENER ) ? &g_audioState.vListenerPos : &g_audioState.wavInstanceAr[0].vEmitterPos;
	vecWind->x = 0.0f;
	vecWind->z = 0.0f;
	vecWind->y = ((float)wOffset) / 300.0f;
	D3DXVECTOR3* vecM =
        ( g_eControlMode == CONTROL_LISTENER ) ? &g_audioState.vListenerPos : &g_audioState.wavInstanceAr[3].vEmitterPos;
	vecM->x = 0.0f;
	vecM->z = 0.0f;
	vecM->y = ((float)wOffset) / 50.0f;
	D3DXVECTOR3* vecMA =
        ( g_eControlMode == CONTROL_LISTENER ) ? &g_audioState.vListenerPos : &g_audioState.wavInstanceAr[4].vEmitterPos;
	vecMA->x = 0.0f;
	vecMA->z = 0.0f;
	vecMA->y = ((float)wOffset) / 600.0f;
}

void SpawnJump(){
	PlayOneShot(10);
}

void UpdateJump(){
	D3DXVECTOR3* vec =
		&g_audioState.wavInstanceAr[10].vEmitterPos;
	vec->x = 0.0f;
	vec->y = 0.0f;
	vec->z = 0.0f;
}

void deSpawnJump(){
	D3DXVECTOR3* vec =
		&g_audioState.wavInstanceAr[10].vEmitterPos;
	vec->x = 0.0f;
	vec->y = 0.0f;
	vec->z = ZMIN;
	if(jumpSuccess==true)
		jumpPoints[jumpPointCounter] = 1;
	jumpPointCounter++;
}

void WriteXML(){
	CHistory* myHist = new CHistory;
	myHist->addPointArray(points,NUM_POINT);
	myHist->addJumpArray(jumpPoints,NUM_JUMP);
	myHist->addPJNToHist(preJumpNum);
	myHist->addPosArray(posTracker,100);
	delete myHist;
}

void StartGameOverVoice(){
	PlayOneShot(11);
}

void UpdateGameOverVoice(){
	D3DXVECTOR3* vec =
		&g_audioState.wavInstanceAr[11].vEmitterPos;
	vec->x = 0.0f;
	vec->y = 0.0f;
	vec->z = 0.0f;
	g_audioState.wavInstanceAr[11].pSourceVoice->SetVolume(2.0f);
	g_audioState.wavInstanceAr[3].pSourceVoice->SetVolume(0.0f);
	g_audioState.wavInstanceAr[4].pSourceVoice->SetVolume(0.0f);
}