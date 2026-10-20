#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "render.h"
#include "RenderTabs.h"
#ifdef SAM_DEBUG
#include "debug.h"
#endif

unsigned char wait1 = 7;
unsigned char wait2 = 6;

extern unsigned char A, X, Y;
extern unsigned char mem44, mem47, mem49, mem39, mem50, mem51, mem53, mem56;
extern unsigned char speed, pitch;
extern int singmode;
extern unsigned char phonemeIndexOutput[60];
extern unsigned char stressOutput[60];
extern unsigned char phonemeLengthOutput[60];

unsigned char pitches[256];
unsigned char frequency1[256];
unsigned char frequency2[256];
unsigned char frequency3[256];
unsigned char amplitude1[256];
unsigned char amplitude2[256];
unsigned char amplitude3[256];
unsigned char sampledConsonantFlag[256];

void AddInflection(unsigned char mem48, unsigned char phase1);
unsigned char trans(unsigned char mem39212, unsigned char mem39213);

static unsigned char timeline[16];
static size_t timelineReadPosition;
static unsigned char pcmFifo[16];
static unsigned char pcmFifoRead;
static unsigned char pcmFifoWrite;
static unsigned char pcmFifoCount;
static int renderActive;
static unsigned char renderPhase1, renderPhase2, renderPhase3;
static unsigned char renderMem38, renderSpeedCounter, renderMem48;
static int sampleActive;
static int sampleCompletionPending;
static int sampleKind;
static int sampleContinuation;
static int samplePendingSecond;
static int sampleByteDone;
static unsigned char sampleIndex, sampleByte, sampleBits, samplePhase;
static unsigned char sampledConsonantPosition;
static unsigned char sampleSavedY;
int bufferpos;

int timetable[5][5] =
{
    {162, 167, 167, 127, 128},
    {226, 60, 60, 0, 0},
    {225, 60, 59, 0, 0},
    {200, 0, 0, 54, 55},
    {199, 0, 0, 54, 54}
};
static unsigned oldtimetableindex;

static void QueueSafeSamples(size_t endPosition)
{
    while (timelineReadPosition < endPosition && pcmFifoCount < sizeof(pcmFifo))
    {
        pcmFifo[pcmFifoWrite] = timeline[timelineReadPosition % sizeof(timeline)];
        timeline[timelineReadPosition % sizeof(timeline)] = 128;
        pcmFifoWrite = (unsigned char)((pcmFifoWrite + 1) % sizeof(pcmFifo));
        pcmFifoCount++;
        timelineReadPosition++;
    }
}

void RenderFlushFinalSamples(void)
{
    QueueSafeSamples((size_t)bufferpos / 50);
}

void RenderResetStream(void)
{
    memset(timeline, 128, sizeof(timeline));
    timelineReadPosition = 0;
    pcmFifoRead = pcmFifoWrite = pcmFifoCount = 0;
    bufferpos = 0;
    oldtimetableindex = 0;
    renderActive = sampleActive = sampleCompletionPending = 0;
    sampledConsonantPosition = 0;
}

size_t RenderReadSamples(unsigned char *pcm, size_t capacity)
{
    size_t count = 0;
    while (count < capacity && pcmFifoCount != 0)
    {
        pcm[count++] = pcmFifo[pcmFifoRead];
        pcmFifoRead = (unsigned char)((pcmFifoRead + 1) % sizeof(pcmFifo));
        pcmFifoCount--;
    }
    return count;
}

void Output8BitAry(int index, unsigned char ary[5])
{
    int k;
    size_t start;
    bufferpos += timetable[oldtimetableindex][index];
    oldtimetableindex = (unsigned)index;
    start = (size_t)bufferpos / 50;
    QueueSafeSamples(start);
    for (k = 0; k < 5; k++)
        timeline[(start + (size_t)k) % sizeof(timeline)] = ary[k];
}

void Output8Bit(int index, unsigned char value)
{
    unsigned char ary[5] = { value, value, value, value, value };
    Output8BitAry(index, ary);
}

// Initialize a sampled-phoneme iterator. It emits one timing event per call.
static void StartRenderSample(int continuation)
{
    unsigned char selector = (unsigned char)(mem39 & 7);
    sampleSavedY = Y;
    mem49 = Y;
    mem47 = (unsigned char)(selector - 1);
    mem53 = tab48426[mem47];
    sampleKind = ((mem39 & 248) == 0) ? 2 : 1;
    sampleContinuation = continuation;
    samplePendingSecond = sampleByteDone = 0;
    sampleBits = 0;
    if (sampleKind == 1)
        sampleIndex = (unsigned char)((mem39 & 248) ^ 255);
    else
    {
        samplePhase = (unsigned char)((pitches[mem49] >> 4) ^ 255);
        sampleIndex = sampledConsonantPosition;
    }
    sampleActive = 1;
}

static void AdvanceRenderSampleByte(void)
{
    sampleBits = 0;
    sampleIndex++;
    if (sampleKind == 1)
    {
        if (sampleIndex == 0)
        {
            sampleActive = 0;
            sampleCompletionPending = 1;
            mem44 = 1;
            Y = sampleSavedY;
        }
    }
    else
    {
        samplePhase++;
        if (samplePhase == 0)
        {
            sampleActive = 0;
            sampleCompletionPending = 1;
            sampledConsonantPosition = sampleIndex;
            mem44 = 1;
            Y = sampleSavedY;
            A = 1;
        }
    }
}

static int NextRenderSample(int *index, unsigned char *value)
{
    unsigned char highBit;
    unsigned char tableIndex;
    if (!sampleActive) return 0;
    if (samplePendingSecond)
    {
        samplePendingSecond = 0;
        *index = 2;
        *value = 5 * 16;
        if (sampleByteDone) AdvanceRenderSampleByte();
        return 1;
    }
    if (sampleBits == 0)
    {
        sampleByte = sampleTable[(unsigned int)mem47 * 256U + sampleIndex];
        sampleBits = 8;
    }
    highBit = (unsigned char)(sampleByte & 128);
    sampleByte = (unsigned char)(sampleByte << 1);
    sampleBits--;
    sampleByteDone = (sampleBits == 0);
    if (sampleKind == 1)
    {
        if (highBit == 0)
        {
            tableIndex = (unsigned char)(mem53 & 15);
            *index = 1;
            *value = (unsigned char)(tableIndex * 16);
            if (tableIndex == 0)
            {
                samplePendingSecond = 1;
                return 1;
            }
        }
        else { *index = 2; *value = 5 * 16; }
        if (sampleByteDone) AdvanceRenderSampleByte();
    }
    else
    {
        *index = highBit != 0 ? 3 : 4;
        *value = highBit != 0 ? (unsigned char)(26 * 16) : (unsigned char)(6 * 16);
        if (sampleByteDone) AdvanceRenderSampleByte();
    }
    return 1;
}

unsigned char Read(unsigned char p, unsigned char Y)
{
    switch(p)
    {
    case 168: return pitches[Y];
    case 169: return frequency1[Y];
    case 170: return frequency2[Y];
    case 171: return frequency3[Y];
    case 172: return amplitude1[Y];
    case 173: return amplitude2[Y];
    case 174: return amplitude3[Y];
    }
    printf("Error reading to tables");
    return 0;
}

void Write(unsigned char p, unsigned char Y, unsigned char value)
{

    switch(p)
    {
    case 168: pitches[Y] = value; return;
    case 169: frequency1[Y] = value;  return;
    case 170: frequency2[Y] = value;  return;
    case 171: frequency3[Y] = value;  return;
    case 172: amplitude1[Y] = value;  return;
    case 173: amplitude2[Y] = value;  return;
    case 174: amplitude3[Y] = value;  return;
    }
    printf("Error writing to tables\n");
}



// -------------------------------------------------------------------------
//Code48227
// Render a sampled sound from the sampleTable.
//
//   Phoneme   Sample Start   Sample End
//   32: S*    15             255
//   33: SH    257            511
//   34: F*    559            767
//   35: TH    583            767
//   36: /H    903            1023
//   37: /X    1135           1279
//   38: Z*    84             119
//   39: ZH    340            375
//   40: V*    596            639
//   41: DH    596            631
//
//   42: CH
//   43: **    399            511
//
//   44: J*
//   45: **    257            276
//   46: **
//
//   66: P*
//   67: **    743            767
//   68: **
//
//   69: T*
//   70: **    231            255
//   71: **
//
// The SampledPhonemesTable[] holds flags indicating if a phoneme is
// voiced or not. If the upper 5 bits are zero, the sample is voiced.
//
// Samples in the sampleTable are compressed, with bits being converted to
// bytes from high bit to low, as follows:
//
//   unvoiced 0 bit   -> X
//   unvoiced 1 bit   -> 5
//
//   voiced 0 bit     -> 6
//   voiced 1 bit     -> 24
//
// Where X is a value from the table:
//
//   { 0x18, 0x1A, 0x17, 0x17, 0x17 };
//
// The index into this table is determined by masking off the lower
// 3 bits from the SampledPhonemesTable:
//
//        index = (SampledPhonemesTable[i] & 7) - 1;
//
// For voices samples, samples are interleaved between voiced output.


#if 0 // Superseded by the resumable sampled-phoneme iterator above.
// Code48227()
void RenderSample(unsigned char *mem66)
{
    int tempA;
    // current phoneme's index
    mem49 = Y;

    // mask low three bits and subtract 1 get value to
    // convert 0 bits on unvoiced samples.
    A = mem39&7;
    X = A-1;

    // store the result
    mem56 = X;

    // determine which offset to use from table { 0x18, 0x1A, 0x17, 0x17, 0x17 }
    // T, S, Z                0          0x18
    // CH, J, SH, ZH          1          0x1A
    // P, F*, V, TH, DH       2          0x17
    // /H                     3          0x17
    // /X                     4          0x17

    // get value from the table
    mem53 = tab48426[X];
    mem47 = X;      //46016+mem[56]*256

    // voiced sample?
    A = mem39 & 248;
    if(A == 0)
    {
        // voiced phoneme: Z*, ZH, V*, DH
        Y = mem49;
        A = pitches[mem49] >> 4;

        // jump to voiced portion
        goto pos48315;
    }

    Y = A ^ 255;
pos48274:

    // step through the 8 bits in the sample
    mem56 = 8;

    // get the next sample from the table
    // mem47*256 = offset to start of samples
    A = sampleTable[mem47*256+Y];
pos48280:

    // left shift to get the high bit
    tempA = A;
    A = A << 1;

    // bit not set?
    if ((tempA & 128) == 0)
    {
        // convert the bit to value from table
        X = mem53;
        // output the byte
        Output8Bit(1, (X&0x0f) * 16);
        // if X != 0, exit loop
        if(X != 0) goto pos48296;
    }

    // output a 5 for the on bit
    Output8Bit(2, 5 * 16);

pos48296:

    X = 0;

    // decrement counter
    mem56--;

    // if not done, jump to top of loop
    if (mem56 != 0) goto pos48280;

    // increment position
    Y++;
    if (Y != 0) goto pos48274;

    // restore values and return
    mem44 = 1;
    Y = mem49;
    return;


    unsigned char phase1;

pos48315:
// handle voiced samples here

   // number of samples?
    phase1 = A ^ 255;

    Y = *mem66;
    do
    {

        // shift through all 8 bits
        mem56 = 8;

        // fetch value from table
        A = sampleTable[mem47*256+Y];

        // loop 8 times
        do
        {
            // left shift and check high bit
            tempA = A;
            A = A << 1;
            if ((tempA & 128) != 0)
            {
                // if bit set, output 26
                X = 26;
                Output8Bit(3, (X&0xf)*16);
            } else
            {
                //timetable 4
                // bit is not set, output a 6
                X=6;
                Output8Bit(4, (X&0xf)*16);
            }

            mem56--;
        } while(mem56 != 0);

        // move ahead in the table
        Y++;

        // continue until counter done
        phase1++;

    } while (phase1 != 0);
    //  if (phase1 != 0) goto pos48321;

    // restore values and return
    A = 1;
    mem44 = 1;
    *mem66 = Y;
    Y = mem49;
    return;
}
#endif



// RENDER THE PHONEMES IN THE LIST
//
// The phoneme list is converted into sound through the steps:
//
// 1. Copy each phoneme <length> number of times into the frames list,
//    where each frame represents 10 milliseconds of sound.
//
// 2. Determine the transitions lengths between phonemes, and linearly
//    interpolate the values across the frames.
//
// 3. Offset the pitches by the fundamental frequency.
//
// 4. Render the each frame.


void Render()
{
    unsigned char phase1 = 0;  //mem43
    unsigned char phase2=0;
    unsigned char phase3=0;
    unsigned char mem38=0;
    unsigned char mem40=0;
    unsigned char speedcounter=0; //mem45
    unsigned char mem48=0;
    int i;
    renderActive = 0;
    if (phonemeIndexOutput[0] == 255) return; //exit if no data

    A = 0;
    X = 0;
    mem44 = 0;


// CREATE FRAMES
//
// The length parameter in the list corresponds to the number of frames
// to expand the phoneme to. Each frame represents 10 milliseconds of time.
// So a phoneme with a length of 7 = 7 frames = 70 milliseconds duration.
//
// The parameters are copied from the phoneme to the frame verbatim.


do
{
    // get the index
    Y = mem44;
    // get the phoneme at the index
    A = phonemeIndexOutput[mem44];
    mem56 = A;

    // if terminal phoneme, exit the loop
    if (A == 255) break;

    // period phoneme *.
    if (A == 1)
    {
       // add rising inflection
        A = 1;
        mem48 = 1;
        //goto pos48376;
        AddInflection(mem48, phase1);
    }

    // question mark phoneme?
    if (A == 2)
    {
        // create falling inflection
        mem48 = 255;
        AddInflection(mem48, phase1);
    }

    // get the stress amount (more stress = higher pitch)
    phase1 = tab47492[stressOutput[Y] + 1];

    // get number of frames to write
    phase2 = phonemeLengthOutput[Y];
    Y = mem56;

    // copy from the source to the frames list
    do
    {
        frequency1[X] = freq1data[Y];     // F1 frequency
        frequency2[X] = freq2data[Y];     // F2 frequency
        frequency3[X] = freq3data[Y];     // F3 frequency
        amplitude1[X] = ampl1data[Y];     // F1 amplitude
        amplitude2[X] = ampl2data[Y];     // F2 amplitude
        amplitude3[X] = ampl3data[Y];     // F3 amplitude
        sampledConsonantFlag[X] = sampledConsonantFlags[Y];        // phoneme data for sampled consonants
        pitches[X] = pitch + phase1;      // pitch
        X++;
        phase2--;
    } while(phase2 != 0);
    mem44++;
} while(mem44 != 0);

// -------------------
// CREATE TRANSITIONS
//
// Linear transitions are now created to smoothly connect the
// end of one sustained portion of a phoneme to the following
// phoneme.
//
// To do this, three tables are used:
//
//  Table         Purpose
//  =========     ==================================================
//  blendRank     Determines which phoneme's blend values are used.
//
//  blendOut      The number of frames at the end of the phoneme that
//                will be used to transition to the following phoneme.
//
//  blendIn       The number of frames of the following phoneme that
//                will be used to transition into that phoneme.
//
// In creating a transition between two phonemes, the phoneme
// with the HIGHEST rank is used. Phonemes are ranked on how much
// their identity is based on their transitions. For example,
// vowels are and diphthongs are identified by their sustained portion,
// rather than the transitions, so they are given low values. In contrast,
// stop consonants (P, B, T, K) and glides (Y, L) are almost entirely
// defined by their transitions, and are given high rank values.
//
// Here are the rankings used by SAM:
//
//     Rank    Type                         Phonemes
//     2       All vowels                   IY, IH, etc.
//     5       Diphthong endings            YX, WX, ER
//     8       Terminal liquid consonants   LX, WX, YX, N, NX
//     9       Liquid consonants            L, RX, W
//     10      Glide                        R, OH
//     11      Glide                        WH
//     18      Voiceless fricatives         S, SH, F, TH
//     20      Voiced fricatives            Z, ZH, V, DH
//     23      Plosives, stop consonants    P, T, K, KX, DX, CH
//     26      Stop consonants              J, GX, B, D, G
//     27-29   Stop consonants (internal)   **
//     30      Unvoiced consonants          /H, /X and Q*
//     160     Nasal                        M
//
// To determine how many frames to use, the two phonemes are
// compared using the blendRank[] table. The phoneme with the
// higher rank is selected. In case of a tie, a blend of each is used:
//
//      if blendRank[phoneme1] ==  blendRank[phomneme2]
//          // use lengths from each phoneme
//          outBlendFrames = outBlend[phoneme1]
//          inBlendFrames = outBlend[phoneme2]
//      else if blendRank[phoneme1] > blendRank[phoneme2]
//          // use lengths from first phoneme
//          outBlendFrames = outBlendLength[phoneme1]
//          inBlendFrames = inBlendLength[phoneme1]
//      else
//          // use lengths from the second phoneme
//          // note that in and out are SWAPPED!
//          outBlendFrames = inBlendLength[phoneme2]
//          inBlendFrames = outBlendLength[phoneme2]
//
// Blend lengths can't be less than zero.
//
// Transitions are assumed to be symetrical, so if the transition
// values for the second phoneme are used, the inBlendLength and
// outBlendLength values are SWAPPED.
//
// For most of the parameters, SAM interpolates over the range of the last
// outBlendFrames-1 and the first inBlendFrames.
//
// The exception to this is the Pitch[] parameter, which is interpolates the
// pitch from the CENTER of the current phoneme to the CENTER of the next
// phoneme.
//
// Here are two examples. First, For example, consider the word "SUN" (S AH N)
//
//    Phoneme   Duration    BlendWeight    OutBlendFrames    InBlendFrames
//    S         2           18             1                 3
//    AH        8           2              4                 4
//    N         7           8              1                 2
//
// The formant transitions for the output frames are calculated as follows:
//
//     flags ampl1 freq1 ampl2 freq2 ampl3 freq3 pitch
//    ------------------------------------------------
// S
//    241     0     6     0    73     0    99    61   Use S (weight 18) for transition instead of AH (weight 2)
//    241     0     6     0    73     0    99    61   <-- (OutBlendFrames-1) = (1-1) = 0 frames
// AH
//      0     2    10     2    66     0    96    59 * <-- InBlendFrames = 3 frames
//      0     4    14     3    59     0    93    57 *
//      0     8    18     5    52     0    90    55 *
//      0    15    22     9    44     1    87    53
//      0    15    22     9    44     1    87    53
//      0    15    22     9    44     1    87    53   Use N (weight 8) for transition instead of AH (weight 2).
//      0    15    22     9    44     1    87    53   Since N is second phoneme, reverse the IN and OUT values.
//      0    11    17     8    47     1    98    56 * <-- (InBlendFrames-1) = (2-1) = 1 frames
// N
//      0     8    12     6    50     1   109    58 * <-- OutBlendFrames = 1
//      0     5     6     5    54     0   121    61
//      0     5     6     5    54     0   121    61
//      0     5     6     5    54     0   121    61
//      0     5     6     5    54     0   121    61
//      0     5     6     5    54     0   121    61
//      0     5     6     5    54     0   121    61
//
// Now, consider the reverse "NUS" (N AH S):
//
//     flags ampl1 freq1 ampl2 freq2 ampl3 freq3 pitch
//    ------------------------------------------------
// N
//     0     5     6     5    54     0   121    61
//     0     5     6     5    54     0   121    61
//     0     5     6     5    54     0   121    61
//     0     5     6     5    54     0   121    61
//     0     5     6     5    54     0   121    61
//     0     5     6     5    54     0   121    61   Use N (weight 8) for transition instead of AH (weight 2)
//     0     5     6     5    54     0   121    61   <-- (OutBlendFrames-1) = (1-1) = 0 frames
// AH
//     0     8    11     6    51     0   110    59 * <-- InBlendFrames = 2
//     0    11    16     8    48     0    99    56 *
//     0    15    22     9    44     1    87    53   Use S (weight 18) for transition instead of AH (weight 2)
//     0    15    22     9    44     1    87    53   Since S is second phoneme, reverse the IN and OUT values.
//     0     9    18     5    51     1    90    55 * <-- (InBlendFrames-1) = (3-1) = 2
//     0     4    14     3    58     1    93    57 *
// S
//   241     2    10     2    65     1    96    59 * <-- OutBlendFrames = 1
//   241     0     6     0    73     0    99    61

    A = 0;
    mem44 = 0;
    mem49 = 0; // mem49 starts at as 0
    X = 0;
    while(1) //while No. 1
    {

        // get the current and following phoneme
        Y = phonemeIndexOutput[X];
        A = phonemeIndexOutput[X+1];
        X++;

        // exit loop at end token
        if (A == 255) break;//goto pos47970;


        // get the ranking of each phoneme
        X = A;
        mem56 = blendRank[A];
        A = blendRank[Y];

        // compare the rank - lower rank value is stronger
        if (A == mem56)
        {
            // same rank, so use out blend lengths from each phoneme
            phase1 = outBlendLength[Y];
            phase2 = outBlendLength[X];
        } else
        if (A < mem56)
        {
            // first phoneme is stronger, so us it's blend lengths
            phase1 = inBlendLength[X];
            phase2 = outBlendLength[X];
        } else
        {
            // second phoneme is stronger, so use it's blend lengths
            // note the out/in are swapped
            phase1 = outBlendLength[Y];
            phase2 = inBlendLength[Y];
        }

        Y = mem44;
        A = mem49 + phonemeLengthOutput[mem44]; // A is mem49 + length
        mem49 = A; // mem49 now holds length + position
        A = A + phase2; //Maybe Problem because of carry flag

        speedcounter = A;
        mem47 = 168;
        phase3 = mem49 - phase1; // what is mem49
        A = phase1 + phase2; // total transition?
        mem38 = A;

        X = A;
        X -= 2;
        if ((X & 128) == 0)
        do   //while No. 2
        {

          // mem47 is used to index the tables:
          // 168  pitches[]
          // 169  frequency1
          // 170  frequency2
          // 171  frequency3
          // 172  amplitude1
          // 173  amplitude2
          // 174  amplitude3

            mem40 = mem38;

            if (mem47 == 168)     // pitch
            {

               // unlike the other values, the pitches[] interpolates from
               // the middle of the current phoneme to the middle of the
               // next phoneme

                unsigned char mem36, mem37;
                // half the width of the current phoneme
                mem36 = phonemeLengthOutput[mem44] >> 1;
                // half the width of the next phoneme
                mem37 = phonemeLengthOutput[mem44+1] >> 1;
                // sum the values
                mem40 = mem36 + mem37; // length of both halves
                mem37 += mem49; // center of next phoneme
                mem36 = mem49 - mem36; // center index of current phoneme
                A = Read(mem47, mem37); // value at center of next phoneme - end interpolation value
                //A = mem[address];

                Y = mem36; // start index of interpolation
                mem53 = A - Read(mem47, mem36); // value to center of current phoneme
            } else
            {
                // value to interpolate to
                A = Read(mem47, speedcounter);
                // position to start interpolation from
                Y = phase3;
                // value to interpolate from
                mem53 = A - Read(mem47, phase3);
            }

            // ML : Code47503 is division with remainder, and mem50 gets the sign

            // calculate change per frame
            signed char m53 = (signed char)mem53;
            mem50 = mem53 & 128;
            unsigned char m53abs = abs(m53);
            mem51 = m53abs % mem40; //abs((char)m53) % mem40;
            mem53 = (unsigned char)((signed char)(m53) / mem40);

            // interpolation range
            X = mem40; // number of frames to interpolate over
            Y = phase3; // starting frame


            // linearly interpolate values

            mem56 = 0;

            while(1)     //while No. 3
            {
                A = Read(mem47, Y) + mem53; //carry alway cleared

                mem48 = A;
                Y++;
                X--;
                if(X == 0) break;

                mem56 += mem51;
                if (mem56 >= mem40)  //???
                {
                    mem56 -= mem40; //carry? is set
                    //if ((mem56 & 128)==0)
                    if ((mem50 & 128)==0)
                    {
                        if(mem48 != 0) mem48++;
                    } else mem48--;
                }
                Write(mem47, Y, mem48);
            } //while No. 3

            mem47++;
            //if (mem47 != 175) goto pos47810;
        } while (mem47 != 175);     //while No. 2

        mem44++;
        X = mem44;
    }  //while No. 1

    // add the length of this phoneme
    mem48 = mem49 + phonemeLengthOutput[mem44];


// ASSIGN PITCH CONTOUR
//
// This subtracts the F1 frequency from the pitch to create a
// pitch contour. Without this, the output would be at a single
// pitch level (monotone).


    // don't adjust pitch if in sing mode
    if (!singmode)
    {
        // iterate through the buffer
        for(i=0; i<256; i++) {
            // subtract half the frequency of the formant 1.
            // this adds variety to the voice
            pitches[i] -= (frequency1[i] >> 1);
        }
    }

    phase1 = 0;
    phase2 = 0;
    phase3 = 0;
    mem49 = 0;
    speedcounter = 72; //sam standard speed

// RESCALE AMPLITUDE
//
// Rescale volume from a linear scale to decibels.
//

    //amplitude rescaling
    for(i=255; i>=0; i--)
    {
        amplitude1[i] = amplitudeRescale[amplitude1[i]];
        amplitude2[i] = amplitudeRescale[amplitude2[i]];
        amplitude3[i] = amplitudeRescale[amplitude3[i]];
    }

    Y = 0;
    A = pitches[0];
    mem44 = A;
    X = A;
    mem38 = A - (A>>2);     // 3/4*A ???

#ifdef SAM_DEBUG
{
    PrintOutput(sampledConsonantFlag, frequency1, frequency2, frequency3, amplitude1, amplitude2, amplitude3, pitches);
}
#endif

// PROCESS THE FRAMES
//
// In traditional vocal synthesis, the glottal pulse drives filters, which
// are attenuated to the frequencies of the formants.
//
// SAM generates these formants directly with sin and rectangular waves.
// To simulate them being driven by the glottal pulse, the waveforms are
// reset at the beginning of each glottal pulse.

    //finally the loop for sound output
    renderPhase1 = phase1;
    renderPhase2 = phase2;
    renderPhase3 = phase3;
    renderMem38 = mem38;
    renderMem48 = mem48;
    renderSpeedCounter = speedcounter;
    sampleActive = 0;
    sampleCompletionPending = 0;
    sampledConsonantPosition = 0;
    renderActive = 1;
    return;
}

int RenderNext(void)
{
    unsigned char flag;
    unsigned char ary[5];
    unsigned int p1, p2, p3;
    int k;
    int outputPending;
    int sampleOutputIndex;
    unsigned char sampleOutputValue;

    if (!renderActive) return 0;

    for (;;)
    {
        outputPending = 0;
        if (sampleCompletionPending)
        {
            sampleCompletionPending = 0;
            if (sampleContinuation == 1)
            {
                Y += 2;
                renderMem48 -= 2;
                if (renderMem48 == 0)
                {
                    renderActive = 0;
                    return 0;
                }
                renderSpeedCounter = speed;
                outputPending = 1;
                goto update_glottal_pulse;
            }

            A = pitches[Y];
            mem44 = A;
            renderMem38 = (unsigned char)(A - (A >> 2));
            renderPhase1 = renderPhase2 = renderPhase3 = 0;
            continue;
        }

        if (sampleActive)
        {
            if (NextRenderSample(&sampleOutputIndex, &sampleOutputValue))
            {
                Output8Bit(sampleOutputIndex, sampleOutputValue);
                return 1;
            }

            continue;
        }

        flag = sampledConsonantFlag[Y];
        mem39 = flag;
        if ((flag & 248) != 0)
        {
            StartRenderSample(1);
            if (NextRenderSample(&sampleOutputIndex, &sampleOutputValue))
            {
                Output8Bit(sampleOutputIndex, sampleOutputValue);
                return 1;
            }
            continue;
        }

        p1 = (unsigned int)renderPhase1 * 256U;
        p2 = (unsigned int)renderPhase2 * 256U;
        p3 = (unsigned int)renderPhase3 * 256U;
        for (k = 0; k < 5; k++)
        {
            signed char sp1 = (signed char)sinus[0xff & (p1 >> 8)];
            signed char sp2 = (signed char)sinus[0xff & (p2 >> 8)];
            signed char rp3 = (signed char)rectangle[0xff & (p3 >> 8)];
            signed int sin1 = sp1 * ((unsigned char)amplitude1[Y] & 0x0f);
            signed int sin2 = sp2 * ((unsigned char)amplitude2[Y] & 0x0f);
            signed int rect = rp3 * ((unsigned char)amplitude3[Y] & 0x0f);
            signed int mux = sin1 + sin2 + rect;
            mux /= 32;
            mux += 128;
            ary[k] = (unsigned char)mux;
            p1 += frequency1[Y] * 64U;
            p2 += frequency2[Y] * 64U;
            p3 += frequency3[Y] * 64U;
        }
        Output8BitAry(0, ary);
        outputPending = 1;

        renderSpeedCounter--;
        if (renderSpeedCounter == 0)
        {
            Y++;
            renderMem48--;
        }

        if (renderMem48 == 0)
        {
            renderActive = 0;
            return outputPending;
        }
        if (renderSpeedCounter == 0) renderSpeedCounter = speed;

    update_glottal_pulse:
        mem44--;
        if (mem44 == 0)
        {
            A = pitches[Y];
            mem44 = A;
            renderMem38 = (unsigned char)(A - (A >> 2));
            renderPhase1 = 0;
            renderPhase2 = 0;
            renderPhase3 = 0;
            return outputPending;
        }

        renderMem38--;
        if (renderMem38 != 0 || mem39 == 0)
        {
            renderPhase1 += frequency1[Y];
            renderPhase2 += frequency2[Y];
            renderPhase3 += frequency3[Y];
            if (outputPending) return 1;
            continue;
        }

        StartRenderSample(2);
        if (outputPending) return 1;
    }
}


// Create a rising or falling inflection 30 frames prior to
// index X. A rising inflection is used for questions, and
// a falling inflection is used for statements.

void AddInflection(unsigned char mem48, unsigned char phase1)
{
    // store the location of the punctuation
    mem49 = X;
    A = X;
    int Atemp = A;

    // backup 30 frames
    A = A - 30;
    // if index is before buffer, point to start of buffer
    if (Atemp <= 30) A=0;
    X = A;

    // FIXME: Explain this fix better, it's not obvious
    // ML : A =, fixes a problem with invalid pitch with '.'
    while( (A=pitches[X]) == 127) X++;


pos48398:
    // add the inflection direction
    A += mem48;
    phase1 = A;

    // set the inflection
    pitches[X] = A;
pos48406:

    // increment the position
    X++;

    // exit if the punctuation has been reached
    if (X == mem49) return; //goto pos47615;
    if (pitches[X] == 255) goto pos48406;
    A = phase1;
    goto pos48398;
}

/*
    SAM's voice can be altered by changing the frequencies of the
    mouth formant (F1) and the throat formant (F2). Only the voiced
    phonemes (5-29 and 48-53) are altered.
*/
void SetMouthThroat(unsigned char mouth, unsigned char throat)
{
    unsigned char initialFrequency;
    unsigned char newFrequency = 0;

    // mouth formants (F1) 5..29
    unsigned char mouthFormants5_29[30] = {
        0, 0, 0, 0, 0, 10,
        14, 19, 24, 27, 23, 21, 16, 20, 14, 18, 14, 18, 18,
        16, 13, 15, 11, 18, 14, 11, 9, 6, 6, 6};

    // throat formants (F2) 5..29
    unsigned char throatFormants5_29[30] = {
    255, 255,
    255, 255, 255, 84, 73, 67, 63, 40, 44, 31, 37, 45, 73, 49,
    36, 30, 51, 37, 29, 69, 24, 50, 30, 24, 83, 46, 54, 86};

    // there must be no zeros in this 2 tables
    // formant 1 frequencies (mouth) 48..53
    unsigned char mouthFormants48_53[6] = {19, 27, 21, 27, 18, 13};

    // formant 2 frequencies (throat) 48..53
    unsigned char throatFormants48_53[6] = {72, 39, 31, 43, 30, 34};

    unsigned char pos = 5; //mem39216

    // recalculate formant frequencies 5..29 for the mouth (F1) and throat (F2)
    while(pos != 30)
    {
        // recalculate mouth frequency
        initialFrequency = mouthFormants5_29[pos];
        if (initialFrequency != 0) newFrequency = trans(mouth, initialFrequency);
        freq1data[pos] = newFrequency;

        // recalculate throat frequency
        initialFrequency = throatFormants5_29[pos];
        if(initialFrequency != 0) newFrequency = trans(throat, initialFrequency);
        freq2data[pos] = newFrequency;
        pos++;
    }

    // recalculate formant frequencies 48..53
    pos = 48;
    Y = 0;
    while(pos != 54)
    {
        // recalculate F1 (mouth formant)
        initialFrequency = mouthFormants48_53[Y];
        newFrequency = trans(mouth, initialFrequency);
        freq1data[pos] = newFrequency;

        // recalculate F2 (throat formant)
        initialFrequency = throatFormants48_53[Y];
        newFrequency = trans(throat, initialFrequency);
        freq2data[pos] = newFrequency;
        Y++;
        pos++;
    }
}

unsigned char trans(unsigned char mem39212, unsigned char mem39213)
{
    unsigned char carry;
    int temp;
    unsigned char mem39214, mem39215;
    A = 0;
    mem39215 = 0;
    mem39214 = 0;
    X = 8;
    do
    {
        carry = mem39212 & 1;
        mem39212 = mem39212 >> 1;
        if (carry != 0)
        {
            carry = 0;
            A = mem39215;
            temp = (int)A + (int)mem39213;
            A = A + mem39213;
            if (temp > 255) carry = 1;
            mem39215 = A;
        }
        temp = mem39215 & 1;
        mem39215 = (mem39215 >> 1) | (carry?128:0);
        carry = temp;
        X--;
    } while (X != 0);
    temp = mem39214 & 128;
    mem39214 = (mem39214 << 1) | (carry?1:0);
    carry = temp;
    temp = mem39215 & 128;
    mem39215 = (mem39215 << 1) | (carry?1:0);
    carry = temp;

    return mem39215;
}
