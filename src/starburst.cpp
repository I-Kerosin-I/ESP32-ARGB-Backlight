#include "starburst.h"

star stars[MAX_STARS];

float maxSpeed = 375.0f;          // Max velocity
float particleIgnition = 125.0f;  // How long to "flash"
float particleFadeTime = 1500.0f; // Fade out time

byte starBurstData[2] =     {180, 200};  // Шанс появления (1 / x); Время угасания фрагментов (x * 7)  // TODO: 2 байта для FadeTime

uint32_t it;

uint8_t numStars = 1 + (NUM_LEDS >> 3);

CRGBPalette16 rainbowPalette = Rainbow_gp;


void starburstTick(CRGB* leds)
{
    it = millis();
    for (int j = 0; j < numStars; j++)
    {
        if (random16(starBurstData[0]*5) == 0 && stars[j].birth == 0)
        {
            // Pick a random color and location.
            stars[j].color = ColorFromPalette(rainbowPalette, random8(), 255, LINEARBLEND);
            stars[j].pos = random16(NUM_LEDS - 1);
            stars[j].vel = maxSpeed * (float)(random8()) / 255.0 * (float)(random8()) / 255.0 * 1.0;
            stars[j].birth = it;
            stars[j].lastUpdate = it;
            // more fragments means larger burst effect
            int numFrag = random8(3, 8);                                       // TODO: maka intensity mechanism
            
            for (int i = 0; i < STARBURST_MAX_FRAG; i++)
            {
                if (i < numFrag)
                    stars[j].fragment[i] = stars[j].pos;
                else
                    stars[j].fragment[i] = -1;
            }
        }
    }
    FastLED.clear();

    for (int j = 0; j < numStars; j++)
    {
        if (stars[j].birth != 0)
        {
            float dt = (it - stars[j].lastUpdate) / 1000.0;
            for (int i = 0; i < STARBURST_MAX_FRAG; i++)
            {
                int var = i >> 1;

                if (stars[j].fragment[i] > 0)
                {
                    // all fragments travel right, will be mirrored on other side
                    stars[j].fragment[i] += stars[j].vel * dt * (float)var / 3.0;
                }
                // DBG_PRINTLN("\tфрагмент сдвинут");
            }
            stars[j].lastUpdate = it;
            stars[j].vel -= 3 * stars[j].vel * dt;
        }
        CRGB c = stars[j].color;

        // If the star is brand new, it flashes white briefly.
        // Otherwise it just fades over time.
        float fade = 0.0f;
        float age = it - stars[j].birth;

        if (age < particleIgnition)
        {
            c = CRGB(color_blend(0xFFFFFFFF, RGBW32(c.r, c.g, c.b, 0), 254.5f * ((age / particleIgnition))));
        }
        else
        {
            // Figure out how much to fade and shrink the star based on
            // its age relative to its lifetime
            if (age > particleIgnition + starBurstData[1]*7)
            {
                fade = 1.0f; // Black hole, all faded out
                stars[j].birth = 0;
                c = CRGB(0x000000);
            }
            else
            {
                age -= particleIgnition;
                fade = (age / (starBurstData[1]*7)); // Fading star
                byte f = 254.5f * fade;
                c = CRGB(color_blend(RGBW32(c.r, c.g, c.b, 0), 0x00000000, f));
            }
        }
        float particleSize = (1.0f - fade) * 2.0f;

        for (size_t index = 0; index < STARBURST_MAX_FRAG * 2; index++)
        {
            bool mirrored = index & 0x1;
            uint8_t i = index >> 1;
            if (stars[j].fragment[i] > 0)
            {
                float loc = stars[j].fragment[i];
                if (mirrored)
                    loc -= (loc - stars[j].pos) * 2;
                int start = loc - particleSize;
                int end = loc + particleSize;
                if (start < 0)
                    start = 0;
                if (start == end)
                    end++;
                if (end > NUM_LEDS)
                    end = NUM_LEDS;
                for (int p = start; p < end; p++)
                {
                    leds[p] = CRGB(c.r, c.g, c.b);
                }
            }
        }
    }
}
