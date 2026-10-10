/* Record the actual emulator audio only while HYBRID_CAPTURE_FLAG exists.
 * This is a local asset extraction tool, never part of the game runtime. */
#define _GNU_SOURCE
#include <SDL_audio.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct {
    SDL_AudioCallback original;
    void *userdata;
    FILE *out;
} Capture;

static void capture(void *userdata, Uint8 *stream, int len) {
    Capture *c = userdata;
    c->original(c->userdata, stream, len);
    const char *flag = getenv("HYBRID_CAPTURE_FLAG");
    const char *path = getenv("HYBRID_CAPTURE_PCM");
    if (flag && path && access(flag, F_OK) == 0) {
        if (!c->out) c->out = fopen(path, "ab");
        if (c->out) { fwrite(stream, 1, len, c->out); fflush(c->out); }
    } else if (c->out) { fclose(c->out); c->out = NULL; }
}

SDL_AudioDeviceID SDL_OpenAudioDevice(const char *device, int iscapture,
        const SDL_AudioSpec *desired, SDL_AudioSpec *obtained, int changes) {
    typedef SDL_AudioDeviceID (*Open)(const char *, int, const SDL_AudioSpec *, SDL_AudioSpec *, int);
    Open open_audio = (Open)dlsym(RTLD_NEXT, "SDL_OpenAudioDevice");
    SDL_AudioSpec spec = *desired;
    if (!iscapture && desired->callback) {
        Capture *c = calloc(1, sizeof(*c));
        if (c) { c->original = spec.callback; c->userdata = spec.userdata;
            spec.callback = capture; spec.userdata = c; }
    }
    SDL_AudioDeviceID id = open_audio(device, iscapture, &spec, obtained, changes);
    if (id && !iscapture) {
        const SDL_AudioSpec *s = obtained ? obtained : &spec;
        fprintf(stderr, "[capture-audio] frequency=%d format=%04x channels=%d\n",
                s->freq, s->format, s->channels);
    }
    return id;
}
