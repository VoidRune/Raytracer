#pragma once
#include <string>
#include <xaudio2.h>
#include <xaudio2fx.h>
#include <xapofx.h>
#include <x3daudio.h>
#pragma comment(lib,"xaudio2.lib")

class Sound
{
public:
	Sound() {};
	~Sound()
	{
		delete[] buffer.pAudioData;
	}
private:
	friend class AudioEngine;

	WAVEFORMATEXTENSIBLE wfx = { 0 };
	XAUDIO2_BUFFER buffer = { 0 };

	bool loaded = false;

};

class SoundInstance
{
private:
	friend class AudioEngine;

	Sound* sound = nullptr;
	uint32_t voiceId = 0;
	uint32_t hashId = 0;
};

struct vec3f
{
	float x, y, z;
};

//enum REVERB_PRESET;

class AudioEngine
{
public:
	/*Returns the static instance							*/
	static inline AudioEngine& Get() { return *s_Instance; }
	/* Channels can have separate volumes					*/
	AudioEngine(int numOfChannel = 16, int numOfVoices = 32);
	~AudioEngine();

	void Play2D(Sound* sound, const int& submix);
	void Play2D(SoundInstance* instance, const int& submix);
	/* +z is sound from right								*/
	/* -z is sound from left								*/
	/* innerRadius is blending point between channels		*/
	void Play3D(Sound* sound, const int& submix, vec3f direction, float innerRadius = 3.0f);
	void Play3D(SoundInstance* instance, const int& submix, vec3f direction, float innerRadius = 3.0f);
	/* +z is sound from right								*/
	/* -z is sound from left								*/
	/* innerRadius is blending point between channels		*/
	void Update3D(SoundInstance* instance, const int& submix, vec3f direction, float innerRadius = 3.0f);

	//void SetReverb(SoundInstance* instance, REVERB_PRESET reverb);
	//void SetReverb(const int& submix, REVERB_PRESET reverb);

	/* Stops the last instance of this sound being played	*/
	void Stop(SoundInstance* instance);
	/* Sets the volume to desired channel					*/
	/* Number of channels is defined in the constructor		*/
	void SetVolume(float volume, const int& channel);
	/* Sets the master volume								*/
	/* Final volume is master volume * channel volume		*/
	void SetMasterVolume(float volume);

	/* Stops all currently playing sounds					*/
	/* and flushes all buffers								*/
	void Flush();

	/* Loads the sound in memory							*/
	/* Can load .wav .mp3 .ogg files						*/
	void LoadData(Sound* sound, const std::string& filename, bool looping = false);

	/* Returns pointer to sound on heap						*/
	/* Can load .wav .mp3 .ogg files						*/
	Sound* LoadData(const std::string& filename, bool looping = false);


	void CreateInstance(Sound* sound, SoundInstance* instance);

	AudioEngine(const AudioEngine&) = delete;
private:
	void Calculate3DMatrix(int channels, float* matrix, vec3f direction, float innerRadius);
	void LoadWav(Sound* sound, std::string filename, bool looping = false);
	void LoadMp3(Sound* sound, std::string filename, bool looping = false);
	void LoadOgg(Sound* sound, std::string filename, bool looping = false);
	void ConvertToStereo(Sound* sound);

	static AudioEngine* s_Instance;
};

/*
enum REVERB_PRESET
{
	REVERB_PRESET_DEFAULT,
	REVERB_PRESET_GENERIC,
	REVERB_PRESET_FOREST,
	REVERB_PRESET_PADDEDCELL,
	REVERB_PRESET_ROOM,
	REVERB_PRESET_BATHROOM,
	REVERB_PRESET_LIVINGROOM,
	REVERB_PRESET_STONEROOM,
	REVERB_PRESET_AUDITORIUM,
	REVERB_PRESET_CONCERTHALL,
	REVERB_PRESET_CAVE,
	REVERB_PRESET_ARENA,
	REVERB_PRESET_HANGAR,
	REVERB_PRESET_CARPETEDHALLWAY,
	REVERB_PRESET_HALLWAY,
	REVERB_PRESET_STONECORRIDOR,
	REVERB_PRESET_ALLEY,
	REVERB_PRESET_CITY,
	REVERB_PRESET_MOUNTAINS,
	REVERB_PRESET_QUARRY,
	REVERB_PRESET_PLAIN,
	REVERB_PRESET_PARKINGLOT,
	REVERB_PRESET_SEWERPIPE,
	REVERB_PRESET_UNDERWATER,
	REVERB_PRESET_SMALLROOM,
	REVERB_PRESET_MEDIUMROOM,
	REVERB_PRESET_LARGEROOM,
	REVERB_PRESET_MEDIUMHALL,
	REVERB_PRESET_LARGEHALL,
	REVERB_PRESET_PLATE,
};*/