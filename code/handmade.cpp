internal void 
GameOutputSound(game_sound_output_buffer *SoundBuffer, int ToneHz)
{
  local_persist real32 tSine;
  int16 ToneVolume = 3000;
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

  local_persist int BlueOffset = 0;
  local_persist int GreenOffset = 0;
  local_persist int RedOffset = 0;
  local_persist int ToneHz = 2048;

  GameOutputSound(SoundBuffer, ToneHz);
  RenderWeirdGradient(Buffer, BlueOffset, GreenOffset, RedOffset);
}