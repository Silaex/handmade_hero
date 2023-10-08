internal void
GameOutputSound(game_sound_output_buffer *SoundBuffer)
{
  local_persist real32 tSine;
  int16 ToneVolume = 3000;
  int ToneHz = 128;
  int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

  int16 *SampleOut = SoundBuffer->Samples;
  for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
  {
    real32 SineValue = sinf(tSine);
    int16 SampleValue = (int16)(SineValue * ToneVolume);
    *SampleOut++ = SampleValue;
    *SampleOut++ = SampleValue;

    tSine += 2.0f * Pi32 * 1.0f / (real32)WavePeriod;
  }
}

internal void
RenderWeirdGradient(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset, int RedOffset)
{
	uint8 *Row = (uint8 *)Buffer->BitmapMemory;

	for (int Y = 0; Y < Buffer->BitmapHeight; ++Y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for (int X = 0; X < Buffer->BitmapWidth; ++X)
		{
			uint8 Blue = (X - Y + BlueOffset) ;
			uint8 Green = (X + GreenOffset);
			uint8 Red = (Y + X + RedOffset);

			*Pixel++ = ((Red << 16) | (Green << 8) | Blue);
		}

		Row += Buffer->Pitch;
	}
}

internal void GameUpdateAndRender(game_offscreen_buffer *Buffer, game_sound_output_buffer *SoundBuffer)
{
  GameOutputSound(SoundBuffer);

  int BlueOffset = 0;
  int GreenOffset = 0;
  int RedOffset = 0;
  RenderWeirdGradient(Buffer, BlueOffset, GreenOffset, RedOffset);
}