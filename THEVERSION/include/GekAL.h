#ifndef GEKAL
#define GEKAL

#include "gekrender.h"
#include <AL/al.h>
#include <AL/alc.h>
#include <fstream>

#ifdef LEAKYPIPES
#ifndef INCLUDED_LEAKCHECK
#define INCLUDED_LEAKCHECK
//#define STB_LEAKCHECK_IMPLEMENTATION
#include "stb_leakcheck.h"
#endif
#endif

inline std::string ErrorCheck(ALenum error) {
	if (error == AL_INVALID_NAME) {
		return "\nInvalid name";
	} else if (error == AL_INVALID_ENUM) {
		return "\nInvalid enum ";
	} else if (error == AL_INVALID_VALUE) {
		return "\nInvalid value ";
	} else if (error == AL_INVALID_OPERATION) {
		return "\nInvalid operation ";
	} else if (error == AL_OUT_OF_MEMORY) {
		return "\nOut of memory like! ";
	}

	return "\nDon't know ";
}

// START GekAL.h
inline bool isBigEndian() {
	int a = 1;
	return !((char*)&a)[0];
}

inline int convertToInt(char* buffer, int len) {
	int a = 0;
	if (!isBigEndian())
		for (int i = 0; i < len; i++)
			((char*)&a)[i] = buffer[i];
	else
		for (int i = 0; i < len; i++)
			((char*)&a)[3 - i] = buffer[i];
	return a;
}

// WAV File Loader
inline char* loadWAV(const char* fn, int& chan, int& samplerate, int& bps, int& size) {
	char buffer[4];
	int incrementer = 0; // for the crawler
	std::ifstream in(fn, std::ios::binary);
	in.read(buffer, 4);
	if (strncmp(buffer, "RIFF", 4) != 0) {
		std::cout << "this is not a valid WAVE file" << std::endl;
		return NULL;
	}
	in.read(buffer, 4);
	in.read(buffer, 4); // WAVE
	in.read(buffer, 4); // fmt
	in.read(buffer, 4); // 16
	in.read(buffer, 2); // 1
	in.read(buffer, 2);
	chan = convertToInt(buffer, 2);
	in.read(buffer, 4);
	samplerate = convertToInt(buffer, 4);
	in.read(buffer, 4);
	in.read(buffer, 2);
	in.read(buffer, 2);
	bps = convertToInt(buffer, 2);
	in.read(buffer, 4); // data
	if (strncmp(buffer, "data", 4) == 0) {
		in.read(buffer, 4);
		size = convertToInt(buffer, 4);
		char* data = new char[size];
		in.read(data, size);
		// std::cout << "\n USING DEFAULT LOADING METHOD";
		return data;
	} else {
		char crawler = 'F';
		bool foundD = false;
		bool foundDA = false;
		bool foundDAT = false;
		bool foundDATA = false;
		// crawl to the data
		while (!foundDATA && incrementer < 300) {
			in.read(&crawler, 1);
			if (foundDAT && crawler == 'a') {
				foundDATA = true;
				foundDAT = false;
				foundDA = false;
				foundD = false;
			} else if (foundDAT) {
				foundDATA = false;
				foundDAT = false;
				foundDA = false;
				foundD = false;
			}

			if (foundDA && crawler == 't') {
				foundDAT = true;
				foundDA = false;
				foundD = false;
			} else if (foundDA) {
				foundDATA = false;
				foundDAT = false;
				foundDA = false;
				foundD = false;
			}

			if (foundD && crawler == 'a') {
				foundDA = true;
				foundD = false;
			} else if (foundD) {
				foundDATA = false;
				foundDAT = false;
				foundDA = false;
				foundD = false;
			}
			if (crawler == 'd') {
				foundD = true;
			}
			incrementer++;
		}
		if (foundDATA) {
			// std::cout << "\nFOUND DATA!!!";
			in.read(buffer, 4);
			size = convertToInt(buffer, 4);
			// std::cout << "\nSize is " << size;
			char* data = new char[size];
			in.read(data, size);
			return data;
		} else {
			std::cout << "\nUH OH!";
			return nullptr;
		}
	}
}

inline ALuint loadWAVintoALBuffer(const char* fn) {
	ALuint return_val = 0;

	// OpenAL Loading
	int channel, sampleRate, bps, size;
	ALuint format = 0;
	alGenBuffers(1, &return_val);
	// Loading TONE.WAV
	char* TONE_WAV_DATA = nullptr;
	TONE_WAV_DATA = loadWAV(fn, channel, sampleRate, bps, size);
	// std::cout << "\nSAMPLE RATE: " << sampleRate;
	if (channel == 1) {
		if (bps == 8) {
			format = AL_FORMAT_MONO8;
			// std::cout << "\nMONO8 FORMAT";
		} else {
			format = AL_FORMAT_MONO16;
			// std::cout << "\nMONO16 FORMAT";
		}
	} else {
		if (bps == 8) {
			format = AL_FORMAT_STEREO8;
			// std::cout << "\nSTEREO8 FORMAT";
		} else {
			format = AL_FORMAT_STEREO16;
			// std::cout << "\nSTEREO16 FORMAT";
		}
	}
	alBufferData(return_val, format, TONE_WAV_DATA, size, sampleRate);

	if (TONE_WAV_DATA) // Gotta free what we malloc
		delete[] TONE_WAV_DATA;
	// std::cout << ErrorCheck(algetError());
	return return_val;
}

inline void syncCameraStateToALListener(gekRender::Camera* mycam, glm::vec3 vel = glm::vec3(0, 0, 0)) {
	ALfloat listenerOri[] = {0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f};
	alListener3f(AL_POSITION, mycam->pos.x, mycam->pos.y, mycam->pos.z);
	alListener3f(AL_VELOCITY, vel.x, vel.y, vel.z);

	listenerOri[0] = mycam->forward.x;
	listenerOri[1] = mycam->forward.y;
	listenerOri[2] = mycam->forward.z;

	listenerOri[3] = mycam->up.x;
	listenerOri[4] = mycam->up.y;
	listenerOri[5] = mycam->up.z;
	alListenerfv(AL_ORIENTATION, listenerOri);
}
// End GekAL.h

// OpenAL reference link
// https://www.openal.org/documentation/openal-1.1-specification.pdf

#endif
