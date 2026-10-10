#pragma once
#include <windows.h>
#include <mmsystem.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <cstring>
#include <string>

// Own the device and its buffer rather than relying on system event sounds.
class WaveMusic {
    HWAVEOUT device=nullptr;
    WAVEHDR header{};
    std::vector<char> bytes;
    bool prepared=false;
    static std::string describe(MMRESULT result){
        char message[256]{};waveOutGetErrorTextA(result,message,256);
        return std::string(message)+" ("+std::to_string(result)+")";
    }
public:
    WaveMusic()=default;
    WaveMusic(const WaveMusic&)=delete;
    WaveMusic& operator=(const WaveMusic&)=delete;
    ~WaveMusic(){stop();}
    void stop(){if(device){waveOutReset(device);if(prepared)waveOutUnprepareHeader(device,&header,sizeof(header));waveOutClose(device);}device=nullptr;prepared=false;header={};bytes.clear();}
    bool play(const std::filesystem::path& file,std::string& error){
        stop();std::ifstream input(file,std::ios::binary);
        if(!input){error="Cannot open music file; keep assets beside the executable.";return false;}
        char id[4],type[4];uint32_t size=0;
        if(!input.read(id,4)||std::memcmp(id,"RIFF",4)||!input.read(reinterpret_cast<char*>(&size),4)||!input.read(type,4)||std::memcmp(type,"WAVE",4)){error="Music file is not a WAV.";return false;}
        WAVEFORMATEX format{};bool found_format=false;
        while(input.read(id,4)&&input.read(reinterpret_cast<char*>(&size),4)){
            if(size>64*1024*1024){error="Invalid WAV chunk size.";return false;}
            if(!std::memcmp(id,"fmt ",4)){
                if(size<16){error="Invalid WAV format chunk.";return false;}
                if(!input.read(reinterpret_cast<char*>(&format),16))break;
                format.cbSize=0;found_format=true;input.seekg(size-16,std::ios::cur);
            }else if(!std::memcmp(id,"data",4)){bytes.resize(size);if(!input.read(bytes.data(),size)){bytes.clear();break;}}
            else input.seekg(size,std::ios::cur);
            if(size&1)input.seekg(1,std::ios::cur);
        }
        if(!found_format||bytes.empty()||format.wFormatTag!=WAVE_FORMAT_PCM||format.wBitsPerSample!=16||format.nChannels<1||format.nChannels>2||format.nBlockAlign!=format.nChannels*2||bytes.size()%format.nBlockAlign){error="Unsupported or incomplete PCM WAV.";return false;}
        MMRESULT result=waveOutOpen(&device,WAVE_MAPPER,&format,0,0,CALLBACK_NULL);
        if(result!=MMSYSERR_NOERROR){device=nullptr;error=describe(result);return false;}
        header.lpData=bytes.data();header.dwBufferLength=static_cast<DWORD>(bytes.size());header.dwFlags=WHDR_BEGINLOOP|WHDR_ENDLOOP;header.dwLoops=0xffffffff;
        result=waveOutPrepareHeader(device,&header,sizeof(header));
        if(result!=MMSYSERR_NOERROR){error=describe(result);stop();return false;}prepared=true;
        result=waveOutWrite(device,&header,sizeof(header));
        if(result!=MMSYSERR_NOERROR){error=describe(result);stop();return false;}
        error.clear();return true;
    }
    bool playing()const{return device&&prepared&&(header.dwFlags&WHDR_INQUEUE);}
};
