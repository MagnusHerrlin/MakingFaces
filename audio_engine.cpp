// ==============================================================================
// MakingFaces - SDL2_mixer & Intelligent Dialogue Audio Engine
// Architecture designed by Magnus Herrlin 2026
// ==============================================================================

// ==============================================================================
// Section A. Includes, Constants & Internal State
// ==============================================================================
#include "audio_engine.h"
#include "tft_compat.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>

namespace fs = std::filesystem;

namespace {

constexpr float    DEFAULT_AUTO_VOLUME      = 0.30f;
constexpr float    MAX_DMX_VOLUME           = 0.65f;
constexpr float    ENVELOPE_ATTACK          = 0.38f;
constexpr float    ENVELOPE_RELEASE         = 0.15f;
constexpr float    SPEECH_GAIN              = 4.20f;
constexpr float    AGC_MIN_PEAK             = 0.004f;
constexpr float    AGC_DECAY                = 0.996f;
constexpr float    AGC_NOISE_FLOOR          = 0.0012f;
constexpr uint32_t LIPSYNC_HOLD_TIMEOUT_MS  = 600UL;
constexpr uint32_t DIALOGUE_PAUSE_RESET_MS  = 1100UL;

std::mutex            g_audio_mutex;
std::string           g_audio_dir     = "audio";
std::string           g_capture_hint  = "Monitor";
bool                  g_audio_ready   = false;
Mix_Music*            g_current_music = nullptr;
float                 g_target_volume = DEFAULT_AUTO_VOLUME;
uint8_t               g_last_trigger  = 0;

std::atomic<bool>     g_monitor_running{false};
std::atomic<bool>     g_monitor_enabled{true};
std::thread           g_monitor_thread;

std::atomic<float>    g_mixer_envelope{0.0f};
std::atomic<float>    g_system_envelope{0.0f};
std::atomic<float>    g_left_envelope{0.0f};
std::atomic<float>    g_right_envelope{0.0f};
std::atomic<float>    g_active_gain{DEFAULT_AUTO_VOLUME};
std::atomic<uint32_t> g_last_audio_ms{0};
std::atomic<int>      g_active_speaker{1};


} // namespace

// ==============================================================================
// Section B. PulseAudio/PipeWire Monitor, Envelope Smoothing & SDL2_mixer Hooks
// ==============================================================================

// ------------------------------------------------------------------------------
// 1. Query PulseAudio/PipeWire sources to find active .monitor loopback
// ------------------------------------------------------------------------------
static std::string detectActiveMonitorSource(const std::string& hint)
{
  FILE* pipe = popen("pactl list short sources 2>/dev/null", "r");
  if (!pipe) {
    return "";
  }

  std::array<char, 256> line{};
  std::string fallbackMonitor;
  std::string runningMonitor;

  while (std::fgets(line.data(), static_cast<int>(line.size()), pipe) != nullptr) {
    std::string row(line.data());
    size_t tab1 = row.find('\t');
    if (tab1 == std::string::npos) {
      continue;
    }
    size_t tab2 = row.find('\t', tab1 + 1);
    if (tab2 == std::string::npos) {
      continue;
    }

    std::string srcName = row.substr(tab1 + 1, tab2 - (tab1 + 1));
    if (srcName.find(".monitor") == std::string::npos) {
      continue;
    }

    if (!hint.empty() && hint != "Monitor" && srcName.find(hint) != std::string::npos) {
      runningMonitor = srcName;
      break;
    }
    if (row.find("RUNNING") != std::string::npos) {
      runningMonitor = srcName;
    }
    if (fallbackMonitor.empty()) {
      fallbackMonitor = srcName;
    }
  }
  pclose(pipe);
  return !runningMonitor.empty() ? runningMonitor : fallbackMonitor;
}

// ------------------------------------------------------------------------------
// 2. Smooth an audio envelope atomic using asymmetric attack/release rates
// ------------------------------------------------------------------------------
static void updateEnvelopeAtomic(std::atomic<float>& envAtom, float target)
{
  if (target < 0.04f) {
    target = 0.0f;
  } else {
    g_last_audio_ms.store(millis(), std::memory_order_relaxed);
  }
  target = std::min(1.0f, target);

  float current = envAtom.load(std::memory_order_relaxed);
  float alpha   = (target > current) ? ENVELOPE_ATTACK : ENVELOPE_RELEASE;
  float next    = current + alpha * (target - current);
  if (next < 0.005f) {
    next = 0.0f;
  }
  envAtom.store(next, std::memory_order_relaxed);
}
// ------------------------------------------------------------------------------
// 3. Compute separate Left and Right channel RMS over interleaved S16LE stereo
// ------------------------------------------------------------------------------
static void computeStereoS16Rms(const int16_t* samples, int frameCount, float& outLeftRms, float& outRightRms)
{
  outLeftRms  = 0.0f;
  outRightRms = 0.0f;
  if (!samples || frameCount <= 0) {
    return;
  }

  double sumSqL = 0.0;
  double sumSqR = 0.0;
  for (int i = 0; i < frameCount; ++i) {
    double l = static_cast<double>(samples[i * 2]) / 32768.0;
    double r = static_cast<double>(samples[i * 2 + 1]) / 32768.0;
    sumSqL += l * l;
    sumSqR += r * r;
  }

  double invN = 1.0 / static_cast<double>(frameCount);
  outLeftRms  = static_cast<float>(std::sqrt(sumSqL * invN));
  outRightRms = static_cast<float>(std::sqrt(sumSqR * invN));
}

// ------------------------------------------------------------------------------
// 4. Update separate Left/Right stereo envelopes and dominant speaker side
// ------------------------------------------------------------------------------
static void updateStereoEnvelopes(float leftTarget, float rightTarget)
{
  updateEnvelopeAtomic(g_left_envelope, leftTarget);
  updateEnvelopeAtomic(g_right_envelope, rightTarget);
  updateEnvelopeAtomic(g_system_envelope, std::max(leftTarget, rightTarget));

  static int s_currentSide = 1;
  float peak = std::max(leftTarget, rightTarget);
  if (peak >= 0.06f) {
    if (leftTarget > rightTarget * 1.15f) {
      s_currentSide = 1;
    } else if (rightTarget > leftTarget * 1.15f) {
      s_currentSide = 2;
    }
    g_active_speaker.store(s_currentSide, std::memory_order_relaxed);
  }
}

// ------------------------------------------------------------------------------
// 5. SDL2_mixer post-mix hook measuring local stereo DMX music playback
// ------------------------------------------------------------------------------
static void SDLCALL audioPostMixCallback(void*, Uint8* stream, int len)
{
  if (!Mix_PlayingMusic() || !stream || len <= 0) {
    g_mixer_envelope.store(0.0f, std::memory_order_relaxed);
    return;
  }
  const int16_t* samples    = reinterpret_cast<const int16_t*>(stream);
  const int      frameCount = len / static_cast<int>(sizeof(int16_t) * 2);
  float rmsL = 0.0f;
  float rmsR = 0.0f;
  computeStereoS16Rms(samples, frameCount, rmsL, rmsR);

  float volComp = std::max(0.08f, g_active_gain.load(std::memory_order_relaxed));
  float envL    = std::min(1.0f, (rmsL / volComp) * SPEECH_GAIN);
  float envR    = std::min(1.0f, (rmsR / volComp) * SPEECH_GAIN);
  updateEnvelopeAtomic(g_mixer_envelope, std::max(envL, envR));
  updateStereoEnvelopes(envL, envR);
}

// ------------------------------------------------------------------------------
// 6. Background thread reading 24 kHz stereo monitor PCM via parec with AGC
// ------------------------------------------------------------------------------
static void systemMonitorWorker(std::string monitorSrc)
{
  std::string cmd = "parec --format=s16le --rate=24000 --channels=2 --latency-msec=60";
  if (!monitorSrc.empty()) {
    cmd += " -d \"" + monitorSrc + "\"";
  }
  cmd += " 2>/dev/null";

  FILE* pcmPipe = popen(cmd.c_str(), "r");
  if (!pcmPipe) {
    std::fprintf(stderr, "[AUDIO WARN] Failed to launch parec stereo monitor pipe\n");
    return;
  }

  constexpr int CHUNK_FRAMES = 512;
  std::array<int16_t, CHUNK_FRAMES * 2> buffer{};
  float agcPeak = 0.02f;

  while (g_monitor_running.load(std::memory_order_relaxed)) {
    size_t readSamples = std::fread(buffer.data(), sizeof(int16_t), buffer.size(), pcmPipe);
    int frameCount = static_cast<int>(readSamples / 2);
    if (frameCount <= 0) {
      break;
    }

    if (g_monitor_enabled.load(std::memory_order_relaxed)) {
      float rmsL = 0.0f;
      float rmsR = 0.0f;
      computeStereoS16Rms(buffer.data(), frameCount, rmsL, rmsR);
      float peakRms = std::max(rmsL, rmsR);

      if (peakRms > AGC_NOISE_FLOOR) {
        if (peakRms > agcPeak) {
          agcPeak = peakRms;
        } else {
          agcPeak = std::max(AGC_MIN_PEAK, agcPeak * AGC_DECAY);
        }
        float normL = std::min(1.0f, (rmsL / agcPeak) * 0.92f);
        float normR = std::min(1.0f, (rmsR / agcPeak) * 0.92f);
        updateStereoEnvelopes(normL, normR);
      } else {
        updateStereoEnvelopes(0.0f, 0.0f);
      }
    } else {
      agcPeak = 0.02f;
      g_system_envelope.store(0.0f, std::memory_order_relaxed);
      g_left_envelope.store(0.0f, std::memory_order_relaxed);
      g_right_envelope.store(0.0f, std::memory_order_relaxed);
    }
  }
  pclose(pcmPipe);
}


// ==============================================================================
// Section C. Public Audio Lifecycle, DMX Playback & Lip-Sync Queries
// ==============================================================================
namespace {

// ------------------------------------------------------------------------------
// 7. Map normalized 0.0..1.0 volume level to SDL2_mixer 0..MIX_MAX_VOLUME
// ------------------------------------------------------------------------------
int toSdlVolume(float normVol)
{
  float clamped = std::max(0.0f, std::min(1.0f, normVol));
  return static_cast<int>(std::round(clamped * MIX_MAX_VOLUME));
}

// ------------------------------------------------------------------------------
// 8. Resolve 3-digit track index to an existing .wav or .mp3 file path
// ------------------------------------------------------------------------------
bool resolveTrackFile(uint8_t trackNum, std::string& outPath)
{
  std::array<char, 8> baseName{};
  std::snprintf(baseName.data(), baseName.size(), "%03u", trackNum);

  fs::path wavPath = fs::path(g_audio_dir) / (std::string(baseName.data()) + ".wav");
  if (fs::exists(wavPath)) {
    outPath = wavPath.string();
    return true;
  }

  fs::path mp3Path = fs::path(g_audio_dir) / (std::string(baseName.data()) + ".mp3");
  if (fs::exists(mp3Path)) {
    outPath = mp3Path.string();
    return true;
  }
  return false;
}

// ------------------------------------------------------------------------------
// 9. Halt active SDL2_mixer music playback and release allocated Mix_Music
// ------------------------------------------------------------------------------
void stopCurrentPlayback(void)
{
  if (Mix_PlayingMusic()) {
    Mix_HaltMusic();
  }
  if (g_current_music) {
    Mix_FreeMusic(g_current_music);
    g_current_music = nullptr;
  }
  g_mixer_envelope.store(0.0f, std::memory_order_relaxed);
}

// ------------------------------------------------------------------------------
// 10. Load and play numbered audio track once via SDL2_mixer
// ------------------------------------------------------------------------------
void startTrackPlayback(uint8_t trackVal)
{
  std::string trackPath;
  if (!resolveTrackFile(trackVal, trackPath)) {
    std::printf("[AUDIO WARN] Track %03u (.wav/.mp3) not found in %s\n", trackVal, g_audio_dir.c_str());
    return;
  }

  stopCurrentPlayback();
  g_current_music = Mix_LoadMUS(trackPath.c_str());
  if (!g_current_music) {
    std::fprintf(stderr, "[AUDIO WARN] Failed to load %s: %s\n", trackPath.c_str(), Mix_GetError());
    return;
  }

  g_active_gain.store(g_target_volume, std::memory_order_relaxed);
  Mix_VolumeMusic(toSdlVolume(g_target_volume));
  Mix_PlayMusic(g_current_music, 1);
  std::printf("[AUDIO] Playing %s (vol=%.2f)\n", trackPath.c_str(), g_target_volume);
}

} // namespace

// ------------------------------------------------------------------------------
// 11. Open 48 kHz SDL2_mixer device and start PipeWire/PulseAudio monitor thread
// ------------------------------------------------------------------------------
void initAudioEngine(const std::string& audioDir, const std::string& captureDeviceHint)
{
  std::lock_guard<std::mutex> lock(g_audio_mutex);
  g_audio_dir    = audioDir.empty() ? "audio" : audioDir;
  g_capture_hint = captureDeviceHint.empty() ? "Monitor" : captureDeviceHint;

  if (Mix_OpenAudio(48000, AUDIO_S16SYS, 2, 2048) < 0) {
    std::fprintf(stderr, "[AUDIO WARN] Mix_OpenAudio failed: %s\n", Mix_GetError());
    g_audio_ready = false;
    return;
  }

  g_active_gain.store(DEFAULT_AUTO_VOLUME, std::memory_order_relaxed);
  Mix_VolumeMusic(toSdlVolume(DEFAULT_AUTO_VOLUME));
  Mix_SetPostMix(audioPostMixCallback, nullptr);

  std::string monitorSrc = (g_capture_hint.find(".monitor") != std::string::npos)
                           ? g_capture_hint
                           : detectActiveMonitorSource(g_capture_hint);

  std::printf("[AUDIO] Loopback monitor source: %s\n",
              !monitorSrc.empty() ? monitorSrc.c_str() : "default");

  g_monitor_running.store(true, std::memory_order_relaxed);
  g_monitor_thread = std::thread(systemMonitorWorker, monitorSrc);
  g_audio_ready    = true;
}

// ------------------------------------------------------------------------------
// 12. Stop monitor thread, halt playback and close the SDL2_mixer audio device
// ------------------------------------------------------------------------------
void shutdownAudioEngine(void)
{
  std::lock_guard<std::mutex> lock(g_audio_mutex);
  if (!g_audio_ready) {
    return;
  }

  g_monitor_enabled.store(false, std::memory_order_relaxed);
  g_monitor_running.store(false, std::memory_order_relaxed);
  if (g_monitor_thread.joinable()) {
    g_monitor_thread.detach();
  }

  Mix_SetPostMix(nullptr, nullptr);
  stopCurrentPlayback();
  Mix_CloseAudio();
  g_audio_ready = false;
}
// ------------------------------------------------------------------------------
// 13. Enable or disable system loopback lip-sync monitoring (off during DMX)
// ------------------------------------------------------------------------------
void setSystemAudioMonitorEnabled(bool enable)
{
  if (!g_audio_ready) {
    return;
  }
  bool prev = g_monitor_enabled.exchange(enable, std::memory_order_relaxed);
  if (prev != enable) {
    if (!enable) {
      g_system_envelope.store(0.0f, std::memory_order_relaxed);
    }
    std::printf("[AUDIO] System audio lip-sync monitor: %s\n", enable ? "ON (Autonomous)" : "OFF (DMX Active)");
  }
}

// ------------------------------------------------------------------------------
// 14. Update track, volume and rising-edge playback trigger from DMX channels
// ------------------------------------------------------------------------------
void updateAudioControlFromDmx(uint8_t trackVal, uint8_t volumeVal, uint8_t triggerVal)
{
  std::lock_guard<std::mutex> lock(g_audio_mutex);
  if (!g_audio_ready) {
    return;
  }

  float newVol = (static_cast<float>(volumeVal) / 255.0f) * MAX_DMX_VOLUME;
  if (std::fabs(newVol - g_target_volume) > 0.003f) {
    g_target_volume = newVol;
    g_active_gain.store(g_target_volume, std::memory_order_relaxed);
    Mix_VolumeMusic(toSdlVolume(g_target_volume));
  }

  bool triggerHigh     = (triggerVal > 127);
  bool prevTriggerHigh = (g_last_trigger > 127);

  if (triggerHigh && !prevTriggerHigh && trackVal > 0) {
    startTrackPlayback(trackVal);
  } else if (!triggerHigh && prevTriggerHigh) {
    stopCurrentPlayback();
  }

  g_last_trigger = triggerVal;
}

// ------------------------------------------------------------------------------
// 15. Stop any DMX-triggered track and restore default volume on DMX timeout
// ------------------------------------------------------------------------------
void resetAudioOnDmxTimeout(void)
{
  std::lock_guard<std::mutex> lock(g_audio_mutex);
  if (!g_audio_ready) {
    return;
  }
  if (g_last_trigger > 127) {
    g_last_trigger = 0;
    stopCurrentPlayback();
  }
  g_target_volume = DEFAULT_AUTO_VOLUME;
  g_active_gain.store(g_target_volume, std::memory_order_relaxed);
  Mix_VolumeMusic(toSdlVolume(DEFAULT_AUTO_VOLUME));
}

// ------------------------------------------------------------------------------
// 16. Return combined peak audio envelope (0.0..1.0) for real-time lip-sync
// ------------------------------------------------------------------------------
float getAudioEnvelope(void)
{
  if (!g_audio_ready) {
    return 0.0f;
  }
  float mixEnv = g_mixer_envelope.load(std::memory_order_relaxed);
  float sysEnv = g_monitor_enabled.load(std::memory_order_relaxed)
                 ? g_system_envelope.load(std::memory_order_relaxed)
                 : 0.0f;
  return std::max(mixEnv, sysEnv);
}

// ------------------------------------------------------------------------------
// 17. Return true when either local music or system loopback audio is active
// ------------------------------------------------------------------------------
bool isAudioPlaying(void)
{
  if (!g_audio_ready) {
    return false;
  }
  return (Mix_PlayingMusic() != 0) ||
         (g_monitor_enabled.load(std::memory_order_relaxed) &&
          g_system_envelope.load(std::memory_order_relaxed) > 0.04f);
}

// ------------------------------------------------------------------------------
// 18. Return true while lip-sync should override autonomous mouth animations
// ------------------------------------------------------------------------------
bool isAudioLipSyncOverrideActive(void)
{
  if (!g_audio_ready) {
    return false;
  }
  if (Mix_PlayingMusic() != 0) {
    return true;
  }
  if (!g_monitor_enabled.load(std::memory_order_relaxed)) {
    return false;
  }

  uint32_t lastMs = g_last_audio_ms.load(std::memory_order_relaxed);
  if (lastMs == 0) {
    return false;
  }
  return (millis() - lastMs) <= LIPSYNC_HOLD_TIMEOUT_MS;
}

// ------------------------------------------------------------------------------
// 19. Return active dialogue speaker side (1 = Left profile, 2 = Right profile)
// ------------------------------------------------------------------------------
int getActiveDialogueSpeaker(void)
{
  return g_active_speaker.load(std::memory_order_relaxed);
}

// ------------------------------------------------------------------------------
// 20. Return smoothed Left stereo channel audio envelope (0.0..1.0)
// ------------------------------------------------------------------------------
float getAudioLeftEnvelope(void)
{
  if (!g_audio_ready) {
    return 0.0f;
  }
  return g_left_envelope.load(std::memory_order_relaxed);
}

// ------------------------------------------------------------------------------
// 21. Return smoothed Right stereo channel audio envelope (0.0..1.0)
// ------------------------------------------------------------------------------
float getAudioRightEnvelope(void)
{
  if (!g_audio_ready) {
    return 0.0f;
  }
  return g_right_envelope.load(std::memory_order_relaxed);
}
