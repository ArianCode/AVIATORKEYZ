// =============================================================================
//  SampleLibrary unit tests (committed API)
//
//  API: loadFromMemory(), loadSample(), clearAll(), getSampleMap(), getLastError()
//
//  Tests cover:
//    - loadFromMemory: null/zero-size data rejected with error
//    - loadFromMemory: valid WAV data accepted, region added to map
//    - loadFromMemory: region has correct rootNote, noteMin, noteMax
//    - clearAll: map becomes empty, error cleared
//    - Multiple loads: map grows; clearAll resets
//    - loadSample with unsupported extension returns false with error
//    - getSampleMap: initial state is empty
// =============================================================================

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "State/SampleLibrary.h"

// ---------------------------------------------------------------------------
// Build a minimal valid 16-bit PCM WAV (44-byte header + N silent frames)
// ---------------------------------------------------------------------------
static juce::MemoryBlock makeSilentWav (int numFrames = 512, int sampleRate = 44100)
{
    const int dataBytes  = numFrames * 2;
    const int riffSize   = 36 + dataBytes;

    juce::MemoryBlock mb;
    mb.setSize (44 + dataBytes, true);
    char* d = static_cast<char*> (mb.getData());

    auto w4 = [&] (int o, uint32_t v) {
        d[o]=(char)(v&0xFF); d[o+1]=(char)((v>>8)&0xFF);
        d[o+2]=(char)((v>>16)&0xFF); d[o+3]=(char)((v>>24)&0xFF);
    };
    auto w2 = [&] (int o, uint16_t v) {
        d[o]=(char)(v&0xFF); d[o+1]=(char)((v>>8)&0xFF);
    };

    d[0]='R'; d[1]='I'; d[2]='F'; d[3]='F';
    w4(4, (uint32_t)riffSize);
    d[8]='W'; d[9]='A'; d[10]='V'; d[11]='E';
    d[12]='f'; d[13]='m'; d[14]='t'; d[15]=' ';
    w4(16,16); w2(20,1); w2(22,1);
    w4(24,(uint32_t)sampleRate);
    w4(28,(uint32_t)(sampleRate*2)); w2(32,2); w2(34,16);
    d[36]='d'; d[37]='a'; d[38]='t'; d[39]='a';
    w4(40,(uint32_t)dataBytes);
    // sample data already zeroed
    return mb;
}

// Build a WAV with non-silent (triangle wave) content
static juce::MemoryBlock makeAudioWav (int numFrames = 512)
{
    juce::MemoryBlock mb = makeSilentWav (numFrames);
    char* d = static_cast<char*> (mb.getData());
    for (int i = 0; i < numFrames; ++i)
    {
        float t = static_cast<float> (i) / static_cast<float> (numFrames);
        auto s16 = static_cast<int16_t> ((t < 0.5f ? t : 1.0f - t) * 2.0f * 16000.0f);
        d[44 + i*2]     = (char)(s16 & 0xFF);
        d[44 + i*2 + 1] = (char)((s16 >> 8) & 0xFF);
    }
    return mb;
}


class SampleLibraryLoadTests : public juce::UnitTest
{
public:
    SampleLibraryLoadTests() : juce::UnitTest ("SampleLibrary_Load", "AviatorKeyz") {}

    void runTest() override
    {
        beginTest ("Initial sample map is empty");
        {
            SampleLibrary lib;
            expect (lib.getSampleMap().empty(),
                    "Fresh SampleLibrary must have empty sample map");
            expect (lib.getLastError().isEmpty(),
                    "Fresh SampleLibrary must have no error");
        }

        beginTest ("loadFromMemory rejects null data");
        {
            SampleLibrary lib;
            bool ok = lib.loadFromMemory (nullptr, 512, "test");
            expect (!ok, "loadFromMemory(nullptr) must return false");
            expect (lib.getLastError().isNotEmpty(),
                    "loadFromMemory(nullptr) must set an error string");
            expect (lib.getSampleMap().empty(),
                    "Map must remain empty after failed load");
        }

        beginTest ("loadFromMemory rejects zero-byte data");
        {
            SampleLibrary lib;
            const char dummy[4] = {};
            bool ok = lib.loadFromMemory (dummy, 0, "test");
            expect (!ok, "loadFromMemory(size=0) must return false");
            expect (lib.getLastError().isNotEmpty(),
                    "loadFromMemory(size=0) must set an error string");
        }

        beginTest ("loadFromMemory rejects invalid (non-WAV) data");
        {
            SampleLibrary lib;
            const char notAWav[64] = "This is not audio data at all.";
            bool ok = lib.loadFromMemory (notAWav, sizeof (notAWav), "garbage");
            expect (!ok, "loadFromMemory with garbage data must return false");
            expect (lib.getLastError().isNotEmpty(),
                    "loadFromMemory with garbage data must set an error string");
        }

        beginTest ("loadFromMemory accepts valid 16-bit PCM WAV");
        {
            SampleLibrary lib;
            auto wav = makeAudioWav (1024);
            bool ok = lib.loadFromMemory (wav.getData(), wav.getSize(), "test_sample");
            expect (ok, "loadFromMemory must accept valid WAV: " + lib.getLastError());
            if (ok)
            {
                expect (!lib.getSampleMap().empty(), "Map must have a region after successful load");
                expect (lib.getLastError().isEmpty(), "No error after successful load");
            }
        }

        beginTest ("loadFromMemory stores correct rootNote");
        {
            SampleLibrary lib;
            auto wav = makeAudioWav();
            bool ok = lib.loadFromMemory (wav.getData(), wav.getSize(), "s", 69);  // A4 root
            expect (ok, lib.getLastError());
            if (ok && !lib.getSampleMap().empty())
            {
                const int root = lib.getSampleMap().front().rootNote;
                expectEquals (root, 69, "rootNote 69 (A4) must be stored");
            }
        }

        beginTest ("loadFromMemory stores correct note range");
        {
            SampleLibrary lib;
            auto wav = makeAudioWav();
            bool ok = lib.loadFromMemory (wav.getData(), wav.getSize(), "s",
                                          60, 48, 72);
            expect (ok, lib.getLastError());
            if (ok && !lib.getSampleMap().empty())
            {
                const auto& r = lib.getSampleMap().front();
                expectEquals (r.noteMin, 48, "noteMin must be stored");
                expectEquals (r.noteMax, 72, "noteMax must be stored");
            }
        }

        beginTest ("loadFromMemory rejects out-of-range rootNote — clamps to [0,127]");
        {
            SampleLibrary lib;
            auto wav = makeAudioWav();
            bool ok = lib.loadFromMemory (wav.getData(), wav.getSize(), "s", 200);  // > 127
            expect (ok, "Load should succeed even with clamped rootNote");
            if (ok && !lib.getSampleMap().empty())
            {
                const int root = lib.getSampleMap().front().rootNote;
                expect (root <= 127, "rootNote must be clamped to <= 127");
                expect (root >= 0,   "rootNote must be clamped to >= 0");
            }
        }
    }
};

class SampleLibraryClearTests : public juce::UnitTest
{
public:
    SampleLibraryClearTests() : juce::UnitTest ("SampleLibrary_Clear", "AviatorKeyz") {}

    void runTest() override
    {
        beginTest ("clearAll resets map to empty");
        {
            SampleLibrary lib;
            auto wav = makeAudioWav();
            lib.loadFromMemory (wav.getData(), wav.getSize(), "s1");
            lib.loadFromMemory (wav.getData(), wav.getSize(), "s2");
            expect (!lib.getSampleMap().empty(), "Map should have 2 regions before clear");

            lib.clearAll();
            expect (lib.getSampleMap().empty(), "Map must be empty after clearAll");
            expect (lib.getLastError().isEmpty(), "Error must be cleared by clearAll");
        }

        beginTest ("clearAll then reload: map has new region only");
        {
            SampleLibrary lib;
            auto wav = makeAudioWav();
            lib.loadFromMemory (wav.getData(), wav.getSize(), "first");
            lib.clearAll();
            lib.loadFromMemory (wav.getData(), wav.getSize(), "second");

            expectEquals ((int)lib.getSampleMap().size(), 1,
                          "After clearAll + one reload, map must have exactly 1 region");
        }

        beginTest ("Multiple loads accumulate in map");
        {
            SampleLibrary lib;
            auto wav = makeAudioWav();
            lib.loadFromMemory (wav.getData(), wav.getSize(), "a");
            lib.loadFromMemory (wav.getData(), wav.getSize(), "b");
            lib.loadFromMemory (wav.getData(), wav.getSize(), "c");

            expectEquals ((int)lib.getSampleMap().size(), 3,
                          "Three loadFromMemory calls must produce 3 map regions");
        }
    }
};

class SampleLibraryFileLoadTests : public juce::UnitTest
{
public:
    SampleLibraryFileLoadTests() : juce::UnitTest ("SampleLibrary_FileLoad", "AviatorKeyz") {}

    void runTest() override
    {
        beginTest ("loadSample with unsupported extension returns false");
        {
            SampleLibrary lib;
            juce::File mp3 (juce::File::getSpecialLocation (juce::File::tempDirectory)
                              .getChildFile ("test.mp3"));
            bool ok = lib.loadSample (mp3);
            expect (!ok, "loadSample('.mp3') must return false");
            expect (lib.getLastError().isNotEmpty(),
                    "loadSample with bad extension must set an error");
        }

        beginTest ("loadSample with non-existent WAV file returns false");
        {
            SampleLibrary lib;
            juce::File nonExistent ("/tmp/does_not_exist_aviatorkeyz.wav");
            bool ok = lib.loadSample (nonExistent);
            expect (!ok, "loadSample with non-existent file must return false");
            expect (lib.getLastError().isNotEmpty(),
                    "Non-existent file must set an error string");
        }
    }
};

// Register
static SampleLibraryLoadTests     sampleLibLoadTests;
static SampleLibraryClearTests    sampleLibClearTests;
static SampleLibraryFileLoadTests sampleLibFileTests;
