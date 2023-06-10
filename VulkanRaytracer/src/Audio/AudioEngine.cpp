#include "AudioEngine.h"

#define MINIMP3_IMPLEMENTATION
#include "minimp3.h"
#include "minimp3_ex.h"

#include "minivorbis.h"

#include <iostream>
#include <vector>
#include <filesystem>

static mp3dec_t s_Mp3d;
AudioEngine* AudioEngine::s_Instance = nullptr;
HRESULT FindChunk(HANDLE hFile, DWORD fourcc, DWORD& dwChunkSize, DWORD& dwChunkDataPosition);
HRESULT ReadChunkData(HANDLE hFile, void* buffer, DWORD buffersize, DWORD bufferoffset);

static const XAUDIO2FX_REVERB_I3DL2_PARAMETERS reverbPresets[] =
{
	XAUDIO2FX_I3DL2_PRESET_DEFAULT,
	XAUDIO2FX_I3DL2_PRESET_GENERIC,
	XAUDIO2FX_I3DL2_PRESET_FOREST,
	XAUDIO2FX_I3DL2_PRESET_PADDEDCELL,
	XAUDIO2FX_I3DL2_PRESET_ROOM,
	XAUDIO2FX_I3DL2_PRESET_BATHROOM,
	XAUDIO2FX_I3DL2_PRESET_LIVINGROOM,
	XAUDIO2FX_I3DL2_PRESET_STONEROOM,
	XAUDIO2FX_I3DL2_PRESET_AUDITORIUM,
	XAUDIO2FX_I3DL2_PRESET_CONCERTHALL,
	XAUDIO2FX_I3DL2_PRESET_CAVE,
	XAUDIO2FX_I3DL2_PRESET_ARENA,
	XAUDIO2FX_I3DL2_PRESET_HANGAR,
	XAUDIO2FX_I3DL2_PRESET_CARPETEDHALLWAY,
	XAUDIO2FX_I3DL2_PRESET_HALLWAY,
	XAUDIO2FX_I3DL2_PRESET_STONECORRIDOR,
	XAUDIO2FX_I3DL2_PRESET_ALLEY,
	XAUDIO2FX_I3DL2_PRESET_CITY,
	XAUDIO2FX_I3DL2_PRESET_MOUNTAINS,
	XAUDIO2FX_I3DL2_PRESET_QUARRY,
	XAUDIO2FX_I3DL2_PRESET_PLAIN,
	XAUDIO2FX_I3DL2_PRESET_PARKINGLOT,
	XAUDIO2FX_I3DL2_PRESET_SEWERPIPE,
	XAUDIO2FX_I3DL2_PRESET_UNDERWATER,
	XAUDIO2FX_I3DL2_PRESET_SMALLROOM,
	XAUDIO2FX_I3DL2_PRESET_MEDIUMROOM,
	XAUDIO2FX_I3DL2_PRESET_LARGEROOM,
	XAUDIO2FX_I3DL2_PRESET_MEDIUMHALL,
	XAUDIO2FX_I3DL2_PRESET_LARGEHALL,
	XAUDIO2FX_I3DL2_PRESET_PLATE,
};


struct Voice
{
	uint32_t id;
	IXAudio2SourceVoice* voice;
};
struct Submix
{
	IXAudio2SubmixVoice* submixVoice;
	XAUDIO2_SEND_DESCRIPTOR SFXSend = {};
	XAUDIO2_VOICE_SENDS SFXSendList = {};
};

std::vector<Voice> m_Voices;
std::vector<uint32_t> m_ids;
std::vector<uint32_t> m_hashes;
uint32_t m_currHash;

std::vector<Submix> m_Submix;

IXAudio2* pXAudio2;
X3DAUDIO_HANDLE audio3D;
IXAudio2MasteringVoice* pMasterVoice;

XAUDIO2_VOICE_DETAILS masterVoiceDetails;

/* REVERB */
//IUnknown* pXAPO;
//XAUDIO2_EFFECT_DESCRIPTOR effectDescriptor;
//XAUDIO2_EFFECT_CHAIN effectChain;

class VoiceCallback : public IXAudio2VoiceCallback
{
public:
	HANDLE hBufferEndEvent;
	VoiceCallback() : hBufferEndEvent(CreateEvent(NULL, FALSE, FALSE, NULL)) {}
	~VoiceCallback() { CloseHandle(hBufferEndEvent); }

	//Called when the voice has just finished playing a contiguous audio stream.
	void OnStreamEnd() { SetEvent(hBufferEndEvent); }

	//Unused methods are stubs
	void OnVoiceProcessingPassEnd() override { }
	void OnVoiceProcessingPassStart(UINT32 SamplesRequired) override { }
	void OnBufferEnd(void* pBufferContext) override
	{
		uint32_t id = (uint32_t)pBufferContext;
		m_ids.push_back(id);
	}
	void OnBufferStart(void* pBufferContext) override { }
	void OnLoopEnd(void* pBufferContext) override { }
	void OnVoiceError(void* pBufferContext, HRESULT Error) override { }
};

VoiceCallback vcb;

AudioEngine::AudioEngine(int numOfChannel, int numOfVoices)
{
	s_Instance = this;

	HRESULT hr;

	hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	assert(SUCCEEDED(hr));

	hr = XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	assert(SUCCEEDED(hr));

#ifdef _DEBUG
	XAUDIO2_DEBUG_CONFIGURATION debugConfig = {};
	debugConfig.TraceMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS;
	debugConfig.BreakMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS;
	pXAudio2->SetDebugConfiguration(&debugConfig);
#endif // _DEBUG

	hr = pXAudio2->CreateMasteringVoice(&pMasterVoice);
	assert(SUCCEEDED(hr));

	pMasterVoice->GetVoiceDetails(&masterVoiceDetails);

	// Without clamping sample rate, it was crashing 32bit 192kHz audio devices
	if (masterVoiceDetails.InputSampleRate > 48000)
		masterVoiceDetails.InputSampleRate = 48000;

	DWORD channelMask;
	pMasterVoice->GetChannelMask(&channelMask);
	hr = X3DAudioInitialize(channelMask, X3DAUDIO_SPEED_OF_SOUND, audio3D);
	assert(SUCCEEDED(hr));

	//hr = XAudio2CreateReverb(&pXAPO);
	//assert(SUCCEEDED(hr));
	//
	//effectDescriptor.InitialState = true;
	//effectDescriptor.OutputChannels = 2;
	//effectDescriptor.pEffect = pXAPO;
	//
	//effectChain.EffectCount = 1;
	//effectChain.pEffectDescriptors = &effectDescriptor;

	m_Voices.reserve(numOfVoices);
	m_ids.reserve(numOfVoices);
	m_hashes.reserve(numOfVoices);
	m_Submix.reserve(numOfChannel);
	m_currHash = 0;

	WAVEFORMATEXTENSIBLE wfxStereo = { 0 };
	wfxStereo.Format.wFormatTag = 1;
	wfxStereo.Format.nChannels = 2;
	wfxStereo.Format.nSamplesPerSec = 44100;
	wfxStereo.Format.nAvgBytesPerSec = 176400;
	wfxStereo.Format.nBlockAlign = 4;
	wfxStereo.Format.wBitsPerSample = 16;

	for (int i = 0; i < numOfVoices; i++)
	{
		m_Voices.emplace_back();
		m_Voices[i].id = i;
		m_ids.emplace_back(i);
		m_hashes.emplace_back(0xFFFFFFFF);
		hr = pXAudio2->CreateSourceVoice(&m_Voices[i].voice, (WAVEFORMATEX*)&wfxStereo,
			0, XAUDIO2_DEFAULT_FREQ_RATIO, &vcb, nullptr);
		assert(SUCCEEDED(hr));
		//hr = m_Voices[i].voice->SetEffectChain(&effectChain);
		//assert(SUCCEEDED(hr));
	}
	for (int i = 0; i < numOfChannel; i++)
	{
		m_Submix.emplace_back();
		hr = pXAudio2->CreateSubmixVoice(&m_Submix[i].submixVoice, 2, 44100, 0, 0, 0, 0);
		assert(SUCCEEDED(hr));
		m_Submix[i].SFXSend = { 0, m_Submix[i].submixVoice };
		m_Submix[i].SFXSendList = { 1, &m_Submix[i].SFXSend };
		//hr = m_Submix[i].submixVoice->SetEffectChain(&effectChain);
		//assert(SUCCEEDED(hr));
	}
}

void AudioEngine::Play2D(Sound* sound, const int& submix)
{
	if (!sound->loaded)
	{
		//Sound not loaded
		std::cout << "Sound not loaded!" << std::endl;
		return;
	}
	if (m_ids.size() == 0) { return; }

	uint32_t id = m_ids.back();
	m_ids.pop_back();

	XAUDIO2_VOICE_DETAILS details;
	m_Voices[id].voice->GetVoiceDetails(&details);
	if (details.InputSampleRate != sound->wfx.Format.nSamplesPerSec)
		m_Voices[id].voice->SetSourceSampleRate(sound->wfx.Format.nSamplesPerSec);

	sound->buffer.pContext = (void*)id;
	HRESULT hr = m_Voices[id].voice->SubmitSourceBuffer(&sound->buffer);

	m_Voices[id].voice->SetOutputVoices(&m_Submix[submix].SFXSendList);
	m_Voices[id].voice->Start();
}

void AudioEngine::Play2D(SoundInstance* instance, const int& submix)
{
	if (!instance->sound->loaded)
	{
		//Sound not loaded
		std::cout << "Sound not loaded!" << std::endl;
		return;
	}
	if (m_ids.size() == 0) { return; }

	uint32_t id = m_ids.back();
	m_ids.pop_back();

	XAUDIO2_VOICE_DETAILS details;
	m_Voices[id].voice->GetVoiceDetails(&details);
	if (details.InputSampleRate != instance->sound->wfx.Format.nSamplesPerSec)
		m_Voices[id].voice->SetSourceSampleRate(instance->sound->wfx.Format.nSamplesPerSec);

	instance->sound->buffer.pContext = (void*)id;
	instance->voiceId = id;
	HRESULT hr = m_Voices[id].voice->SubmitSourceBuffer(&instance->sound->buffer);

	m_Voices[id].voice->SetOutputVoices(&m_Submix[submix].SFXSendList);
	m_Voices[id].voice->Start();

	m_currHash++;

	m_hashes[id] = m_currHash;
	instance->hashId = m_currHash;
}

void AudioEngine::Play3D(Sound* sound, const int& submix, vec3f direction, float innerRadius)
{

	if (!sound->loaded)
	{
		//Sound not loaded
		std::cout << "Sound not loaded!" << std::endl;
		return;
	}

	if (m_ids.size() == 0) { return; }

	uint32_t id = m_ids.back();
	m_ids.pop_back();

	XAUDIO2_VOICE_DETAILS details;
	m_Voices[id].voice->GetVoiceDetails(&details);
	if (details.InputSampleRate != sound->wfx.Format.nSamplesPerSec)
		m_Voices[id].voice->SetSourceSampleRate(sound->wfx.Format.nSamplesPerSec);

	sound->buffer.pContext = (void*)id;
	HRESULT hr = m_Voices[id].voice->SubmitSourceBuffer(&sound->buffer);
	
	float matrix[4];
	Calculate3DMatrix(sound->wfx.Format.nChannels, matrix, direction, innerRadius);
	if (sound->wfx.Format.nChannels == 2)
	{
		//I have no idea why stereo sounds are mapped incorrectly
		float f = matrix[1];
		matrix[1] = matrix[2];
		matrix[2] = f;
	}
	m_Voices[id].voice->SetOutputVoices(&m_Submix[submix].SFXSendList);
	m_Voices[id].voice->SetOutputMatrix(nullptr, sound->wfx.Format.nChannels, masterVoiceDetails.InputChannels, matrix);
	m_Voices[id].voice->Start();
}

void AudioEngine::Play3D(SoundInstance* instance, const int& submix, vec3f direction, float innerRadius)
{
	if (!instance->sound->loaded)
	{
		//Sound not loaded
		std::cout << "Sound not loaded!" << std::endl;
		return;
	}
	if (m_ids.size() == 0) { return; }

	uint32_t id = m_ids.back();
	m_ids.pop_back();

	XAUDIO2_VOICE_DETAILS details;
	m_Voices[id].voice->GetVoiceDetails(&details);
	if (details.InputSampleRate != instance->sound->wfx.Format.nSamplesPerSec)
		m_Voices[id].voice->SetSourceSampleRate(instance->sound->wfx.Format.nSamplesPerSec);

	instance->sound->buffer.pContext = (void*)id;
	instance->voiceId = id;
	HRESULT hr = m_Voices[id].voice->SubmitSourceBuffer(&instance->sound->buffer);

	m_Voices[id].voice->SetOutputVoices(&m_Submix[submix].SFXSendList);
	m_Voices[id].voice->Start();

	m_currHash++;

	m_hashes[id] = m_currHash;
	instance->hashId = m_currHash;
}

void AudioEngine::Update3D(SoundInstance* instance, const int& submix, vec3f direction, float innerRadius)
{
	if (!instance->sound->loaded)
	{
		//Sound not loaded
		std::cout << "Sound not loaded!" << std::endl;
		return;
	}

	if (instance->hashId != m_hashes[instance->voiceId])
	{
		return;
	}

	int nChannels = instance->sound->wfx.Format.nChannels;

	float matrix[4];
	Calculate3DMatrix(nChannels, matrix, direction, innerRadius);

	if (nChannels == 2)
	{
		float f = matrix[1];
		matrix[1] = matrix[2];
		matrix[2] = f;
	}

	m_Voices[instance->voiceId].voice->SetOutputMatrix(nullptr, nChannels, masterVoiceDetails.InputChannels, matrix);
}

/*
void AudioEngine::SetReverb(SoundInstance* instance, REVERB_PRESET reverb)
{
	if (instance->hashId != m_hashes[instance->voiceId])
	{
		return;
	}

	//m_Voices[instance->voiceId].voice->SetEffectChain(&effectChain);
	
	//XAUDIO2FX_REVERB_PARAMETERS reverbParameters;
	//reverbParameters.ReflectionsDelay = XAUDIO2FX_REVERB_DEFAULT_REFLECTIONS_DELAY;
	//reverbParameters.ReverbDelay = XAUDIO2FX_REVERB_DEFAULT_REVERB_DELAY;
	//reverbParameters.RearDelay = XAUDIO2FX_REVERB_DEFAULT_REAR_DELAY;
	//reverbParameters.PositionLeft = XAUDIO2FX_REVERB_DEFAULT_POSITION;
	//reverbParameters.PositionRight = XAUDIO2FX_REVERB_DEFAULT_POSITION;
	//reverbParameters.PositionMatrixLeft = XAUDIO2FX_REVERB_DEFAULT_POSITION_MATRIX;
	//reverbParameters.PositionMatrixRight = XAUDIO2FX_REVERB_DEFAULT_POSITION_MATRIX;
	//reverbParameters.EarlyDiffusion = XAUDIO2FX_REVERB_DEFAULT_EARLY_DIFFUSION;
	//reverbParameters.LateDiffusion = XAUDIO2FX_REVERB_DEFAULT_LATE_DIFFUSION;
	//reverbParameters.LowEQGain = XAUDIO2FX_REVERB_DEFAULT_LOW_EQ_GAIN;
	//reverbParameters.LowEQCutoff = XAUDIO2FX_REVERB_DEFAULT_LOW_EQ_CUTOFF;
	//reverbParameters.HighEQGain = XAUDIO2FX_REVERB_DEFAULT_HIGH_EQ_GAIN;
	//reverbParameters.HighEQCutoff = XAUDIO2FX_REVERB_DEFAULT_HIGH_EQ_CUTOFF;
	//reverbParameters.RoomFilterFreq = XAUDIO2FX_REVERB_DEFAULT_ROOM_FILTER_FREQ;
	//reverbParameters.RoomFilterMain = XAUDIO2FX_REVERB_DEFAULT_ROOM_FILTER_MAIN;
	//reverbParameters.RoomFilterHF = XAUDIO2FX_REVERB_DEFAULT_ROOM_FILTER_HF;
	//reverbParameters.ReflectionsGain = XAUDIO2FX_REVERB_DEFAULT_REFLECTIONS_GAIN;
	//reverbParameters.ReverbGain = XAUDIO2FX_REVERB_DEFAULT_REVERB_GAIN;
	//reverbParameters.DecayTime = XAUDIO2FX_REVERB_DEFAULT_DECAY_TIME;
	//reverbParameters.Density = XAUDIO2FX_REVERB_DEFAULT_DENSITY;
	//reverbParameters.RoomSize = XAUDIO2FX_REVERB_DEFAULT_ROOM_SIZE;
	//reverbParameters.WetDryMix = XAUDIO2FX_REVERB_DEFAULT_WET_DRY_MIX;
	//
	//FXREVERB_PARAMETERS XAPOParameters;
	//XAPOParameters.Diffusion = 100.0f;
	//XAPOParameters.RoomSize = 10.0f;
	
	//hr = m_Voices[id].voice->SetEffectParameters(0, &reverbParameters, sizeof(reverbParameters));
	//hr = m_Voices[id].voice->SetEffectParameters(0, &XAPOParameters, sizeof(FXREVERB_PARAMETERS));

	//m_Voices[id].voice->EnableEffect(0);
	//m_Voices[id].voice->DisableEffect(0);
	XAUDIO2FX_REVERB_PARAMETERS native;
	ReverbConvertI3DL2ToNative(&reverbPresets[reverb], &native);
	HRESULT hr = m_Voices[instance->voiceId].voice->SetEffectParameters(0, &native, sizeof(native));
	assert(SUCCEEDED(hr));
}

void AudioEngine::SetReverb(const int& submix, REVERB_PRESET reverb)
{
	//IUnknown* pXAPO;
	//HRESULT hr = XAudio2CreateReverb(&pXAPO);

	//m_Submix[submix].submixVoice->SetEffectChain(&effectChain);

	XAUDIO2FX_REVERB_PARAMETERS native;
	ReverbConvertI3DL2ToNative(&reverbPresets[REVERB_PRESET_UNDERWATER], &native);
	HRESULT hr = m_Submix[submix].submixVoice->SetEffectParameters(0, &native, sizeof(native));
	assert(SUCCEEDED(hr));
}
*/

void AudioEngine::Stop(SoundInstance* instance)
{
	if (!instance->sound->loaded)
	{
		//Sound not loaded
		std::cout << "Sound not loaded!" << std::endl;
		return;
	}

	if (instance->hashId != m_hashes[instance->voiceId])
	{
		return;
	}
	m_Voices[instance->voiceId].voice->Stop();
	m_Voices[instance->voiceId].voice->FlushSourceBuffers();
}

void AudioEngine::Calculate3DMatrix(int channels, float* matrix, vec3f direction, float innerRadius)
{
	X3DAUDIO_LISTENER Listener = {};
	X3DAUDIO_EMITTER Emitter = {};

	X3DAUDIO_DSP_SETTINGS DSPSettings = { 0 };
	std::vector<float> channelAzimuths;

	channelAzimuths.resize(2);
	for (size_t i = 0; i < channelAzimuths.size(); ++i)
	{
		channelAzimuths[i] = X3DAUDIO_2PI * float(i) / float(channelAzimuths.size());
	}

	DSPSettings.SrcChannelCount = channels;
	DSPSettings.DstChannelCount = masterVoiceDetails.InputChannels;
	DSPSettings.pMatrixCoefficients = matrix;


	Emitter.OrientFront = { 0, 0, 0 };
	Emitter.OrientTop = { 0, 0, 0 };
	Emitter.Position = { 0, 0, 0 };
	Emitter.Velocity = { 0, 0, 0 };
	Emitter.pCone = NULL;
	Emitter.pLFECurve = NULL;
	Emitter.pLPFDirectCurve = NULL;
	Emitter.pLPFReverbCurve = NULL;
	Emitter.pVolumeCurve = NULL;
	Emitter.pReverbCurve = NULL;
	Emitter.InnerRadius = innerRadius;		//Blending between left and right when close to source
	Emitter.InnerRadiusAngle = 0.0f;
	Emitter.ChannelCount = channels;
	Emitter.ChannelRadius = 2.0f;
	Emitter.CurveDistanceScaler = 2.5f;
	Emitter.DopplerScaler = 2.0f;
	Emitter.pChannelAzimuths = channelAzimuths.data();


	Listener.OrientFront = { 1, 0, 0 };
	Listener.OrientTop = { 0, 1, 0 };
	Listener.Position = { direction.x, direction.y, direction.z };
	Listener.Velocity = { 0, 0, 0 };
	Listener.pCone = NULL;

	X3DAudioCalculate(audio3D, &Listener, &Emitter,
		X3DAUDIO_CALCULATE_MATRIX,
		&DSPSettings);
}

void AudioEngine::SetVolume(float volume, const int& channel)
{
	if (channel >= m_Submix.size())
	{
		std::cout << "channel " << channel << " does not exist!" << std::endl;

		return;
	}
	m_Submix[channel].submixVoice->SetVolume(volume);
}

void AudioEngine::SetMasterVolume(float volume)
{
	pMasterVoice->SetVolume(volume);
}

void AudioEngine::Flush()
{
	for (size_t i = 0; i < m_Voices.size(); i++)
	{
		m_Voices[i].voice->Stop();
		m_Voices[i].voice->FlushSourceBuffers();
	}
}

AudioEngine::~AudioEngine()
{
	//pXAPO->Release();

	for (size_t i = 0; i < m_Voices.size(); i++)
	{
		m_Voices[i].voice->DestroyVoice();
	}
	for (int i = 0; i < m_Submix.size(); i++)
	{
		m_Submix[i].submixVoice->DestroyVoice();
	}

	pMasterVoice->DestroyVoice();
	pXAudio2->Release();
	CoUninitialize();
}

void AudioEngine::LoadData(Sound* sound, const std::string& filename, bool looping)
{
	std::filesystem::path path = filename;
	std::string extension = path.extension().string();

	if (extension == ".mp3")  LoadMp3(sound, filename, looping);
	if (extension == ".ogg")  LoadOgg(sound, filename, looping);
	if (extension == ".wav")  LoadWav(sound, filename, looping);
	ConvertToStereo(sound);
}
Sound* AudioEngine::LoadData(const std::string& filename, bool looping)
{
	Sound* sound = new Sound;

	LoadData(sound, filename, looping);

	return sound;
}

void AudioEngine::CreateInstance(Sound* sound, SoundInstance* instance)
{
	instance->sound = sound;
}

void AudioEngine::LoadWav(Sound* sound, std::string filename, bool looping)
{
	HRESULT hr;

	// Open the file
	HANDLE hFile = CreateFileA(
		(LPCSTR)filename.c_str(),
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		0,
		NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		std::cout << "audio file: " << filename.c_str() << " could not be opened" << std::endl;
		return;
	}

	SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

	DWORD dwChunkSize;
	DWORD dwChunkPosition;

	//check the file type, should be fourccWAVE or 'XWMA'
	FindChunk(hFile, 'FFIR', dwChunkSize, dwChunkPosition);
	DWORD filetype;
	ReadChunkData(hFile, &filetype, sizeof(DWORD), dwChunkPosition);

	FindChunk(hFile, ' tmf', dwChunkSize, dwChunkPosition);
	ReadChunkData(hFile, &sound->wfx, dwChunkSize, dwChunkPosition);

	//fill out the audio data buffer with the contents of the fourccDATA chunk
	FindChunk(hFile, 'atad', dwChunkSize, dwChunkPosition);
	BYTE* pDataBuffer = new BYTE[dwChunkSize];
	ReadChunkData(hFile, pDataBuffer, dwChunkSize, dwChunkPosition);

	sound->buffer.AudioBytes = dwChunkSize;
	sound->buffer.pAudioData = pDataBuffer;
	sound->buffer.Flags = XAUDIO2_END_OF_STREAM;
	sound->buffer.LoopCount = looping ? XAUDIO2_LOOP_INFINITE : 0;

	sound->loaded = true;
}
void AudioEngine::LoadMp3(Sound* sound, std::string filename, bool looping)
{
	mp3dec_file_info_t info;
	int loadResult = mp3dec_load(&s_Mp3d, filename.c_str(), &info, NULL, NULL);
	if (loadResult == -3)
	{
		std::cout << "audio file: " << filename.c_str() << " could not be opened" << std::endl;
		return;
	}
	uint32_t size = info.samples * sizeof(mp3d_sample_t);

	auto sampleRate = info.hz;
	auto channels = info.channels;

	sound->buffer.AudioBytes = size;
	sound->buffer.pAudioData = (BYTE*)info.buffer;
	sound->buffer.Flags = XAUDIO2_END_OF_STREAM;
	sound->buffer.LoopCount = looping ? XAUDIO2_LOOP_INFINITE : 0;

	sound->wfx.Format.wFormatTag = WAVE_FORMAT_PCM;
	sound->wfx.Format.nChannels = channels;
	sound->wfx.Format.nSamplesPerSec = sampleRate;
	sound->wfx.Format.wBitsPerSample = sizeof(short) * 8;
	sound->wfx.Format.nBlockAlign = channels * sizeof(short);
	sound->wfx.Format.nAvgBytesPerSec = sound->wfx.Format.nSamplesPerSec * sound->wfx.Format.nBlockAlign;

	sound->loaded = true;
}
void AudioEngine::LoadOgg(Sound* sound, std::string filename, bool looping)
{
	FILE* fp;
	fopen_s(&fp, filename.c_str(), "rb");
	if (!fp) {
		std::cout << "audio file: " << filename.c_str() << " could not be opened" << std::endl;
	}
	//Open sound stream. 
	OggVorbis_File vorbis;
	if (ov_open_callbacks(fp, &vorbis, NULL, 0, OV_CALLBACKS_DEFAULT) != 0) {
		std::cout << "audio file: " << filename.c_str() << " could not be opened" << std::endl;
		return;
	}
	// Print sound information. 
	vorbis_info* info = ov_info(&vorbis, -1);

	uint64_t samples = ov_pcm_total(&vorbis, -1);
	uint32_t bufferSize = 2 * info->channels * samples; // 2 bytes per sample (I'm guessing...)
	//printf("Ogg file %d Hz, %d channels, %d kbit/s.\n", info->rate, info->channels, info->bitrate_nominal / 1024);

	uint8_t* oggBuffer = new uint8_t[bufferSize];
	uint8_t* bufferPtr = oggBuffer;
	int eof = 0;
	while (!eof)
	{
		int current_section;
		long length = ov_read(&vorbis, (char*)bufferPtr, 4096, 0, 2, 1, &current_section);
		bufferPtr += length;
		if (length <= 0)
		{
			eof = 1;
		}
	}

	sound->buffer.AudioBytes = bufferSize;
	sound->buffer.pAudioData = oggBuffer;
	sound->buffer.Flags = XAUDIO2_END_OF_STREAM;
	sound->buffer.LoopCount = looping ? XAUDIO2_LOOP_INFINITE : 0;

	sound->wfx.Format.wFormatTag = WAVE_FORMAT_PCM;
	sound->wfx.Format.nChannels = info->channels;
	sound->wfx.Format.nSamplesPerSec = info->rate;
	sound->wfx.Format.wBitsPerSample = sizeof(short) * 8;
	sound->wfx.Format.nBlockAlign = info->channels * sizeof(short);

	sound->wfx.Format.nAvgBytesPerSec = sound->wfx.Format.nSamplesPerSec * sound->wfx.Format.nBlockAlign;

	sound->loaded = true;

	ov_clear(&vorbis);
}

void AudioEngine::ConvertToStereo(Sound* sound)
{
	if (sound->wfx.Format.nChannels != 1)
		return;

	uint32_t monoSamples = sound->buffer.AudioBytes;
	//monoSamples are in bytes
	//stereo is uint16_t
	uint16_t* stereo = new uint16_t[monoSamples];
	uint16_t* mono = (uint16_t*)sound->buffer.pAudioData;

	uint32_t samples16 = monoSamples / 2;
	for (size_t i = 0; i < samples16; ++i)
	{
		stereo[i * 2] = mono[i];
		stereo[i * 2 + 1] = mono[i];
	}

	delete[] mono;

	sound->buffer.AudioBytes = monoSamples * 2;
	sound->buffer.pAudioData = (BYTE*)stereo;
	sound->wfx.Format.nChannels = 2;
	sound->wfx.Format.nBlockAlign = 2 * sizeof(short);
	sound->wfx.Format.nAvgBytesPerSec = sound->wfx.Format.nSamplesPerSec * sound->wfx.Format.nBlockAlign;
}

HRESULT FindChunk(HANDLE hFile, DWORD fourcc, DWORD& dwChunkSize, DWORD& dwChunkDataPosition)
{
	HRESULT hr = S_OK;
	if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
		return HRESULT_FROM_WIN32(GetLastError());

	DWORD dwChunkType;
	DWORD dwChunkDataSize;
	DWORD dwRIFFDataSize = 0;
	DWORD dwFileType;
	DWORD bytesRead = 0;
	DWORD dwOffset = 0;

	while (hr == S_OK)
	{
		DWORD dwRead;
		if (0 == ReadFile(hFile, &dwChunkType, sizeof(DWORD), &dwRead, NULL))
			hr = HRESULT_FROM_WIN32(GetLastError());

		if (0 == ReadFile(hFile, &dwChunkDataSize, sizeof(DWORD), &dwRead, NULL))
			hr = HRESULT_FROM_WIN32(GetLastError());

		switch (dwChunkType)
		{
		case 'FFIR':
			dwRIFFDataSize = dwChunkDataSize;
			dwChunkDataSize = 4;
			if (0 == ReadFile(hFile, &dwFileType, sizeof(DWORD), &dwRead, NULL))
				hr = HRESULT_FROM_WIN32(GetLastError());
			break;

		default:
			if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, dwChunkDataSize, NULL, FILE_CURRENT))
				return HRESULT_FROM_WIN32(GetLastError());
		}

		dwOffset += sizeof(DWORD) * 2;

		if (dwChunkType == fourcc)
		{
			dwChunkSize = dwChunkDataSize;
			dwChunkDataPosition = dwOffset;
			return S_OK;
		}

		dwOffset += dwChunkDataSize;

		if (bytesRead >= dwRIFFDataSize) return S_FALSE;

	}

	return S_OK;

}

HRESULT ReadChunkData(HANDLE hFile, void* buffer, DWORD buffersize, DWORD bufferoffset)
{
	HRESULT hr = S_OK;
	if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, bufferoffset, NULL, FILE_BEGIN))
		return HRESULT_FROM_WIN32(GetLastError());
	DWORD dwRead;
	if (0 == ReadFile(hFile, buffer, buffersize, &dwRead, NULL))
		hr = HRESULT_FROM_WIN32(GetLastError());
	return hr;
}