

#include <iostream>
#include <vector>
#include <cstdint>
#include <initializer_list>
#include <fstream>


#define _USE_MATH_DEFINES
#include <math.h>



class WAVE
{
public:
	WAVE(uint32_t sampling_freq, uint32_t duration) :
		m_data(sampling_freq* duration, 0),
		m_freq(sampling_freq)
	{
	}

	bool	save_file(const std::string& path, uint16_t bit_depth=32)
	{
		// http://soundfile.sapp.org/doc/WaveFormat/
		// https://docs.microsoft.com/en-us/previous-versions//ms713231(v=vs.85)?redirectedfrom=MSDN
		typedef struct RIFF_CHUNK
		{
			char		szRIFF[4];     // = 'RIFF'
			uint32_t	nChunkDataSize;
			char		szFourCC[4];   // = 'WAVE'
		} RIFF_CHUNK;

		typedef struct RIFF_SUB_CHUNK
		{
			uint32_t	nChunkID; // = 'fmt ' or 'data'
			uint32_t	nChunkSize;
		} RIFF_SUB_CHUNK;

		typedef struct WAVEFORMAT
		{
			uint16_t	wFormatTag;
			uint16_t	nChannels;
			uint32_t	nSamplesPerSec;
			uint32_t	nAvgBytesPerSec;
			uint16_t	nBlockAlign;
			uint16_t	wBitsPerSample;
		} WAVEFORMAT;

		RIFF_CHUNK  tChunk = {
			{ 'R', 'I', 'F', 'F' },
			0,
			{ 'W', 'A', 'V', 'E' } };

		RIFF_SUB_CHUNK  tSubChunkFmt = {
			' tmf',
			sizeof(WAVEFORMAT) };

		WAVEFORMAT      tFormat = {
			1,
			2,
			m_freq,
			m_freq * 2 * bit_depth / 8,
			(uint16_t)(bit_depth / 8 * 2),
			bit_depth };

		RIFF_SUB_CHUNK  tSubChunkData = {
			'atad',
			(uint32_t)(bit_depth / 8 * 2 * m_data.size()) };

		std::ofstream   ofs(path, std::ios::binary);

		tChunk.nChunkDataSize =
			sizeof(RIFF_SUB_CHUNK) +
			tSubChunkFmt.nChunkSize +
			sizeof(RIFF_SUB_CHUNK) +
			tSubChunkData.nChunkSize;

		if (ofs.fail())
		{
			printf("FileSave FAILED: %s\n", path.c_str());
			return	false;
		}

		ofs.write((char*)&tChunk, sizeof(tChunk));
		ofs.write((char*)&tSubChunkFmt, sizeof(tSubChunkFmt));
		ofs.write((char*)&tFormat, sizeof(tFormat));
		ofs.write((char*)&tSubChunkData, sizeof(tSubChunkData));

		int64_t	v64;
		bool	clip	 = false;
		double	peak_level = 0;
		for (double v : m_data)
		{
			peak_level = std::max(peak_level, std::abs(v));
		}

		switch (bit_depth)
		{
		case 32:
			for (double v : m_data)
			{
				v *= INT32_MAX;
				v = 0 <= v ? v + 0.5 : v - 0.5;
				v64 = v;

				int32_t value;
				if (v64 <= INT32_MAX)
				{
					if (INT32_MIN <= v64)
					{
						value = (int32_t)v64;
					}
					else
					{
						value = INT32_MIN;
						clip = true;
					}
				}
				else
				{
					value = INT32_MAX;
					clip = true;
				}

				ofs.write((char*)&value, sizeof(value));
				ofs.write((char*)&value, sizeof(value));
			}
			break;
		}

		if (!clip)
		{
			printf("FileSaved: %s, Peak Level = %.1f dB\n", path.c_str(), log10f(peak_level) * 20.0);
		}
		else
		{
			printf("FileSaved: %s, Peak Level = %.1f dB, CLIPPED!!\n", path.c_str(), log10f(peak_level) * 20.0);
		}
		return  true;
	}

public:
	uint32_t			m_freq;
	std::vector<double>	m_data;
};


typedef struct tagTONE
{
	double	gain;
	double	freq;
} tagTONE;


WAVE    generate_wave(uint32_t sampling_freq, uint32_t duration, std::vector<tagTONE> tones)
{
	WAVE	wave(sampling_freq, duration);

	for (auto v : tones)
	{
		double	scale = pow(10.0, v.gain / 20.0);
		double	period = sampling_freq / v.freq;

		for (size_t i = 0; i < wave.m_data.size(); i++)
		{
			double  rad = i * 2 * M_PI / period;

			wave.m_data[i] += sin(rad) * scale;
		}
	}

	return	wave;
}

std::vector<tagTONE>	get_tone_1kHz();
std::vector<tagTONE>	get_tone_standard_imd();
std::vector<tagTONE>	get_tone_piano88();
std::vector<tagTONE>	get_tone_piano88_ACDF();
std::vector<tagTONE>	get_tone_piano88_ACF();
std::vector<tagTONE>	get_tone_20();

int main( int argc, char* argv[])
{
	uint32_t				freq	= 48000; // [Hz]
	uint32_t				len		= 60;  // [sec]
	std::vector<double>		wave(freq * len, 0);

	printf("How to use\n");
	printf("\n");
	printf("> MultiToneGenerator.exe <SamplingFreq>\n");
	printf("\n");

	if (2 <= argc)
	{
		freq = std::atoi(argv[1]);
	}

	generate_wave(freq, len, get_tone_1kHz()).save_file("Sine_1kHz.wav");
	generate_wave(freq, len, get_tone_standard_imd()).save_file("IMD_60Hz_7kHz.wav");
	generate_wave(freq, len, get_tone_20()).save_file("MultiTone_20.wav");

	generate_wave(freq, len, get_tone_piano88()).save_file("MultiTone_piano88.wav");
	generate_wave(freq, len, get_tone_piano88_ACDF()).save_file("MultiTone_piano88_ACD#F#.wav");
	generate_wave(freq, len, get_tone_piano88_ACF()).save_file("MultiTone_piano88_AC#F.wav");

	return  0;
}



std::vector<tagTONE>   get_tone_1kHz()
{
	std::initializer_list<tagTONE>  tone =
	{
		{ 0.0, 1000 }
	};

	// Distortion level at sampling_freq=48k
	//  gain	3rd,	5th,	7th,	9th
	//	WG:		166.6,	160.3,	159.7,	174.6
	//	0.000:	166.7,	160.3,	159.7,	174.6
	//	0.100:	179.5,	181.2,	160.9,	165.2
	//	0.010:	165.3,	203.4,	170.4,	161.9
	//	0.001:	158.7,	164.0,	171.2,	165.8

	return  tone;
}

std::vector<tagTONE>   get_tone_standard_imd()
{
	std::initializer_list<tagTONE>  tone =
	{
		{ -6.0, 60 },
		{ -30.0, 7000 }
	};

return  tone;
}


std::vector<tagTONE>   get_tone_20()
{
	std::initializer_list<tagTONE>  tone =
	{
		{ -20.0, 30 },
		{ -20.0, 40 },
		{ -20.0, 50 },
		{ -20.0, 70 },
		{ -20.0, 100 },
		{ -20.0, 150 },
		{ -20.0, 200 },
		{ -20.0, 300 },
		{ -20.0, 400 },
		{ -20.0, 500 },
		{ -20.0, 700 },
		{ -14.0, 1000 },
		{ -20.0, 1500 },
		{ -20.0, 2000 },
		{ -20.0, 3000 },
		{ -20.0, 4000 },
		{ -20.0, 5000 },
		{ -20.0, 7000 },
		{ -20.0, 10000 },
		{ -20.0, 15000 },
	};


	return  tone;
}


std::vector<tagTONE>   get_tone_piano88()
{
	std::initializer_list<tagTONE>  tone =
	{
		{ -36.0,   27.500 },	// 	A0
		{ -36.0,   29.135 },	// 	A#0
		{ -36.0,   30.868 },	// 	B0
		{ -36.0,   32.703 },	// 	C1
		{ -36.0,   34.648 },	// 	C#1
		{ -36.0,   36.708 },	// 	D1
		{ -36.0,   38.891 },	// 	D#1
		{ -36.0,   41.203 },	// 	E1
		{ -36.0,   43.654 },	// 	F1
		{ -36.0,   46.249 },	// 	F#1
		{ -36.0,   48.999 },	// 	G1
		{ -36.0,   51.913 },	// 	G#1
		{ -36.0,   55.000 },	// 	A1
		{ -36.0,   58.270 },	// 	A#1
		{ -36.0,   61.735 },	// 	B1
		{ -36.0,   65.406 },	// 	C2
		{ -36.0,   69.296 },	// 	C#2
		{ -36.0,   73.416 },	// 	D2
		{ -36.0,   77.782 },	// 	D#2
		{ -36.0,   82.407 },	// 	E2
		{ -36.0,   87.307 },	// 	F2
		{ -36.0,   92.499 },	// 	F#2
		{ -36.0,   97.999 },	// 	G2
		{ -36.0,  103.826 },	// 	G#2
		{ -36.0,  110.000 },	// 	A2
		{ -36.0,  116.541 },	// 	A#2
		{ -36.0,  123.471 },	// 	B2
		{ -36.0,  130.813 },	// 	C3
		{ -36.0,  138.591 },	// 	C#3
		{ -36.0,  146.832 },	// 	D3
		{ -36.0,  155.563 },	// 	D#3
		{ -36.0,  164.814 },	// 	E3
		{ -36.0,  174.614 },	// 	F3
		{ -36.0,  184.997 },	// 	F#3
		{ -36.0,  195.998 },	// 	G3
		{ -36.0,  207.652 },	// 	G#3
		{ -36.0,  220.000 },	// 	A3
		{ -36.0,  233.082 },	// 	A#3
		{ -36.0,  246.942 },	// 	B3
		{ -36.0,  261.626 },	// 	C4
		{ -36.0,  277.183 },	// 	C#4
		{ -36.0,  293.665 },	// 	D4
		{ -36.0,  311.127 },	// 	D#4
		{ -36.0,  329.628 },	// 	E4
		{ -36.0,  349.228 },	// 	F4
		{ -36.0,  369.994 },	// 	F#4
		{ -36.0,  391.995 },	// 	G4
		{ -36.0,  415.305 },	// 	G#4
		{ -30.0,  440.000 },	// 	A4
		{ -36.0,  466.164 },	// 	A#4
		{ -36.0,  493.883 },	// 	B4
		{ -36.0,  523.251 },	// 	C5
		{ -36.0,  554.365 },	// 	C#5
		{ -36.0,  587.330 },	// 	D5
		{ -36.0,  622.254 },	// 	D#5
		{ -36.0,  659.255 },	// 	E5
		{ -36.0,  698.456 },	// 	F5
		{ -36.0,  739.989 },	// 	F#5
		{ -36.0,  783.991 },	// 	G5
		{ -36.0,  830.609 },	// 	G#5
		{ -36.0,  880.000 },	// 	A5
		{ -36.0,  932.328 },	// 	A#5
		{ -36.0,  987.767 },	// 	B5
		{ -36.0, 1046.502 },	// 	C6
		{ -36.0, 1108.731 },	// 	C#6
		{ -36.0, 1174.659 },	// 	D6
		{ -36.0, 1244.508 },	// 	D#6
		{ -36.0, 1318.510 },	// 	E6
		{ -36.0, 1396.913 },	// 	F6
		{ -36.0, 1479.978 },	// 	F#6
		{ -36.0, 1567.982 },	// 	G6
		{ -36.0, 1661.219 },	// 	G#6
		{ -36.0, 1760.000 },	// 	A6
		{ -36.0, 1864.655 },	// 	A#6
		{ -36.0, 1975.533 },	// 	B6
		{ -36.0, 2093.005 },	// 	C7
		{ -36.0, 2217.461 },	// 	C#7
		{ -36.0, 2349.318 },	// 	D7
		{ -36.0, 2489.016 },	// 	D#7
		{ -36.0, 2637.020 },	// 	E7
		{ -36.0, 2793.826 },	// 	F7
		{ -36.0, 2959.955 },	// 	F#7
		{ -36.0, 3135.963 },	// 	G7
		{ -36.0, 3322.438 },	// 	G#7
		{ -36.0, 3520.000 },	// 	A7
		{ -36.0, 3729.310 },	// 	A#7
		{ -36.0, 3951.066 },	// 	B7
		{ -36.0, 4186.009 },	// 	C8
	};

	return  tone;
}


std::vector<tagTONE>   get_tone_piano88_ACDF()
{
	std::initializer_list<tagTONE>  tone =
	{
		{ -36.0,   27.500 },	// 	A0
		{ -36.0,   32.703 },	// 	C1
		{ -36.0,   38.891 },	// 	D#1
		{ -36.0,   46.249 },	// 	F#1
		{ -36.0,   55.000 },	// 	A1
		{ -36.0,   65.406 },	// 	C2
		{ -36.0,   77.782 },	// 	D#2
		{ -36.0,   92.499 },	// 	F#2
		{ -36.0,  110.000 },	// 	A2
		{ -36.0,  130.813 },	// 	C3
		{ -36.0,  155.563 },	// 	D#3
		{ -36.0,  184.997 },	// 	F#3
		{ -36.0,  220.000 },	// 	A3
		{ -36.0,  261.626 },	// 	C4
		{ -36.0,  311.127 },	// 	D#4
		{ -36.0,  369.994 },	// 	F#4
		{ -30.0,  440.000 },	// 	A4
		{ -36.0,  523.251 },	// 	C5
		{ -36.0,  622.254 },	// 	D#5
		{ -36.0,  739.989 },	// 	F#5
		{ -36.0,  880.000 },	// 	A5
		{ -36.0, 1046.502 },	// 	C6
		{ -36.0, 1244.508 },	// 	D#6
		{ -36.0, 1479.978 },	// 	F#6
		{ -36.0, 1760.000 },	// 	A6
		{ -36.0, 2093.005 },	// 	C7
		{ -36.0, 2489.016 },	// 	D#7
		{ -36.0, 2959.955 },	// 	F#7
		{ -36.0, 3520.000 },	// 	A7
		{ -36.0, 4186.009 },	// 	C8
	};

	return  tone;
}

std::vector<tagTONE>   get_tone_piano88_ACF()
{
	std::initializer_list<tagTONE>  tone =
	{
		{ -36.0,   27.500 },	// 	A0
		{ -36.0,   34.648 },	// 	C#1
		{ -36.0,   43.654 },	// 	F1
		{ -36.0,   55.000 },	// 	A1
		{ -36.0,   69.296 },	// 	C#2
		{ -36.0,   87.307 },	// 	F2
		{ -36.0,  110.000 },	// 	A2
		{ -36.0,  138.591 },	// 	C#3
		{ -36.0,  174.614 },	// 	F3
		{ -36.0,  220.000 },	// 	A3
		{ -36.0,  277.183 },	// 	C#4
		{ -36.0,  349.228 },	// 	F4
		{ -30.0,  440.000 },	// 	A4
		{ -36.0,  554.365 },	// 	C#5
		{ -36.0,  698.456 },	// 	F5
		{ -36.0,  880.000 },	// 	A5
		{ -36.0, 1108.731 },	// 	C#6
		{ -36.0, 1396.913 },	// 	F6
		{ -36.0, 1760.000 },	// 	A6
		{ -36.0, 2217.461 },	// 	C#7
		{ -36.0, 2793.826 },	// 	F7
		{ -36.0, 3520.000 },	// 	A7
	};

	return  tone;
}
