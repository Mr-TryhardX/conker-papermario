#include "wave_music.hpp"
#include <iostream>

int wmain(int argc,wchar_t** argv){
    if(argc!=2){std::cerr<<"Provide one PCM WAV path.\n";return 2;}
    WaveMusic music;std::string error;
    if(!music.play(std::filesystem::path(argv[1]),error)){std::cerr<<error<<"\n";return 1;}
    Sleep(6000);
    if(!music.playing()){std::cerr<<"Loop buffer is not queued.\n";return 1;}
    music.stop();if(music.playing()){std::cerr<<"Audio did not stop.\n";return 1;}
    if(music.play(L"missing-audio-test.wav",error)){std::cerr<<"Missing file unexpectedly played.\n";return 1;}
    std::cout<<"PASS: WAV opened, loop queued, playback stopped, missing file rejected.\n";return 0;
}
