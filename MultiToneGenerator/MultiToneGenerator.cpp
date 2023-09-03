

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
				v64 = (int64_t)v;

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

		case 16:
			for (double v : m_data)
			{
				v *= INT16_MAX;
				v = 0 <= v ? v + 0.5 : v - 0.5;
				v64 = (int64_t)v;

				int16_t value;
				if (v64 <= INT16_MAX)
				{
					if (INT16_MIN <= v64)
					{
						value = (int32_t)v64;
					}
					else
					{
						value = INT16_MIN;
						clip = true;
					}
				}
				else
				{
					value = INT16_MAX;
					clip = true;
				}

				ofs.write((char*)&value, sizeof(value));
				ofs.write((char*)&value, sizeof(value));
			}
			break;
		}

		if (!clip)
		{
			printf("FileSaved: %s, Peak Level = %.1f dB\n", path.c_str(), log10(peak_level) * 20.0);
		}
		else
		{
			printf("FileSaved: %s, Peak Level = %.1f dB, CLIPPED!!\n", path.c_str(), log10(peak_level) * 20.0);
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


WAVE    generate_wave_amplitude_modulation(uint32_t sampling_freq, uint32_t duration, std::vector<tagTONE> tones)
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

	for (uint32_t i = 0; i < wave.m_data.size(); i++ )
	{
		double	v = 0.5 + (wave.m_data[i] / 2);

		wave.m_data[i] = i & 1 ? -v : v;
	}

	return	wave;
}

std::vector<tagTONE>	get_tone_single(double freq);
std::vector<tagTONE>	get_tone_smpte_60_7000();
std::vector<tagTONE>	get_tone_piano88();
std::vector<tagTONE>	get_tone_32();
std::vector<tagTONE>	get_tone_20_uneven();

int main( int argc, char* argv[])
{
	uint32_t				freq	= 48000; // [Hz]
	uint32_t				len		= 60;  // [sec]
	std::vector<double>		wave(freq * len, 0);

	printf("How to use\n");
	printf("\n");
	printf("> MultiToneGenerator.exe <SamplingFreq> <Duration>\n");
	printf("\n");

	if (2 <= argc)
	{
		freq = std::atoi(argv[1]);
	}

	if (3 <= argc)
	{
		len = std::atoi(argv[2]);
	}

	generate_wave(freq, len, get_tone_single(1000)).save_file("1_Sine_1kHz.wav");
	generate_wave(freq, len, std::vector<tagTONE>() ).save_file("2_Silent.wav");
	generate_wave(freq, len, get_tone_smpte_60_7000()).save_file("3_SMPTE_60Hz_7kHz.wav");
	generate_wave(freq, len, get_tone_32()).save_file("4_MultiTone_32.wav");
	generate_wave(freq, len, get_tone_20_uneven()).save_file("5_MultiTone_20uneven.wav");

	generate_wave_amplitude_modulation(freq, len, get_tone_single(100)).save_file("99_Sine_100Hz_AM.wav", 16);
	return  0;
}



std::vector<tagTONE>   get_tone_single( double freq )
{
	std::initializer_list<tagTONE>  tone =
	{
		{ 0.0, freq }
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

std::vector<tagTONE>   get_tone_smpte_60_7000()
{
	std::initializer_list<tagTONE>  tone =
	{
		{ -6.0, 60 },
		{ -30.0, 7000 }
	};

	return  tone;
}


std::vector<tagTONE>   get_tone_20_uneven()
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


std::vector<tagTONE>	get_tone_32()
{
	std::vector<tagTONE>	tone;

	for (int32_t i = -18; i <= 13; i++)
	{
		tagTONE	t;
		t.gain = i == 0 ? -20 : -26;
		t.freq = pow(2, i /3.0)*1000.0;
		tone.push_back(t);
	}

	return	tone;
}

std::vector<tagTONE>   get_tone_piano88()
{
	std::vector<tagTONE>	tone;

	for (uint32_t i = 0; i < 88; i++)
	{
		tagTONE	t;
		t.gain = -36;
		t.freq = pow(2, i / 12.0 ) * 27.5;
		tone.push_back(t);
	}

	return  tone;
}
