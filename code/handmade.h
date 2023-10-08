
// Services that the platform layer provides to the game



// Services that the game provides to the platform layer

// Timing, Input, Bitmap Buffer, Sound Buffer
struct game_offscreen_buffer
{
  void *BitmapMemory;
  int BitmapWidth;
  int BitmapHeight;
  int Pitch;
  int BytesPerPixel;
};

struct game_sound_output_buffer
{
  int SamplesPerSecond;
  int SampleCount;
  int16 *Samples;
};

internal void GameUpdateAndRender(game_offscreen_buffer *Buffer, game_sound_output_buffer *SoundBuffer);