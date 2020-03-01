#include <stdio.h>
#include <conio.h>
#include <vector>
#include <random>

#define COUNT_OF(x) (sizeof(x) / sizeof(x[0]))

// golden ratio is 1.61803398875
// golden ratio conjugate is 1 / goldenRatio and is just as irrational but numbers grow less quickly using it
static const float c_goldenRatioConjugate = 0.61803398875f;

struct Item
{
    char symbol;
    float weight;
};

const char* NoiseNames[] =
{
    "White Noise",
    "Blue Noise",
    "Golden Ratio",
    "Sobol"
};

enum class MainMenuOption : int
{
    WhiteNoise = 0,
    BlueNoise,
    GoldenRatio,
    Sobol,
    Exit
};

static size_t Ruler(size_t n)
{
    size_t ret = 0;
    while (n != 0 && (n & 1) == 0)
    {
        n /= 2;
        ++ret;
    }
    return ret;
}

float Sobol(size_t index, size_t &sampleInt)
{
    size_t ruler = Ruler(index + 1);
    size_t direction = size_t(size_t(1) << size_t(31 - ruler));
    sampleInt = sampleInt ^ direction;
    return float(sampleInt) / std::pow(2.0f, 32.0f);
}

MainMenuOption MainMenu()
{
    printf("Main Menu\n1) White Noise\n2) Blue Noise\n3) Golden Ratio\n4) Sobol\n5) Exit\n\n");
    while (true)
    {
        char c = (char)_getch();
        if (c >= '1' && c <= '5')
            return MainMenuOption(c - '1');
    }
}

void ShowHistogram(const Item* lootTable, size_t lootTableSize, size_t lootCount = 0)
{
    static const int c_histogramWidth = 40;

    float totalWeight = 0.0f;
    for (size_t index = 0; index < lootTableSize; ++index)
        totalWeight += lootTable[index].weight;

    printf("[");

    int charsSoFar = 0;
    for (size_t index = 0; index < lootTableSize; ++index)
    {
        int numChars = (index == lootTableSize - 1)
            ? c_histogramWidth - charsSoFar
            : int(float(c_histogramWidth) * lootTable[index].weight / totalWeight);
        charsSoFar += numChars;

        for (int i = 0; i < numChars; ++i)
            printf("%c", lootTable[index].symbol);
    }

    printf("]");

    if (lootCount > 0)
        printf(" %zu drops", lootCount);
    else
        printf("                    ");
}

float GenerateLootRNG(MainMenuOption option, std::mt19937& rng, size_t lootCount, float& goldenRatioSum, size_t& sobolSampleInt)
{
    switch (option)
    {
        case MainMenuOption::WhiteNoise:
        {
            std::uniform_real_distribution<float> dist(0.0f, 1.0f);
            return dist(rng);
        }
        case MainMenuOption::BlueNoise:
        {
            // TODO: this!
            return 0.0f;
        }
        case MainMenuOption::GoldenRatio:
        {
            goldenRatioSum += c_goldenRatioConjugate;
            goldenRatioSum = goldenRatioSum - floor(goldenRatioSum);
            return goldenRatioSum;
        }
        case MainMenuOption::Sobol:
        {
            return Sobol(lootCount, sobolSampleInt);
        }
    }
    return 0.0f;
}

float HistogramAbsError(const Item* normalizedLootTable, const Item* lootHistogram, size_t lootTableSize)
{
    float totalHistogramCount = 0.0f;
    for (size_t index = 0; index < lootTableSize; ++index)
        totalHistogramCount += lootHistogram[index].weight;
    
    if (totalHistogramCount == 0.0f)
        return 1.0f;

    float totalAbsError = 0.0f;
    for (size_t index = 0; index < lootTableSize; ++index)
    {
        float desired = normalizedLootTable[index].weight;
        float actual = lootHistogram[index].weight / totalHistogramCount;
        totalAbsError += abs(desired - actual);
    }
    return totalAbsError;
}

void LootMenu(MainMenuOption option, const Item* lootTable, size_t lootTableSize)
{
    std::random_device rd;
    std::mt19937 rng(rd());

    printf("Loot Menu - %s\n1) Drop 1 loot\n2) Drop 10 loot\n3) Drop 100 loot\n4) Converge\n5) Reset\n6) Main Menu\n\n", NoiseNames[(int)option]);

    std::vector<Item> lootHistogram(lootTableSize);
    for (size_t index = 0; index < lootTableSize; ++index)
    {
        lootHistogram[index].symbol = lootTable[index].symbol;
        lootHistogram[index].weight = 0.0f;
    }

    // show the actual histogram
    printf("Loot table histogram:\n");
    ShowHistogram(lootTable, lootTableSize);

    printf("\n\nLoot drop histogram:\n");

    size_t lootCount = 0;
    float goldenRatioSum = 0.0f;
    size_t sobolSampleInt = 0;

    auto DropLoot = [&] ()
    {
        float lootRandomNumber = GenerateLootRNG(option, rng, lootCount, goldenRatioSum, sobolSampleInt);

        for (size_t index = 0; index < lootTableSize; ++index)
        {
            lootRandomNumber -= lootTable[index].weight;

            if (lootRandomNumber <= 0.0f || index == lootTableSize - 1)
            {
                lootHistogram[index].weight += 1.0f;
                break;
            }
        }

        lootCount++;
    };

    while (true)
    {
        printf("\r");
        ShowHistogram(lootHistogram.data(), lootHistogram.size(), lootCount);

        char c = (char)_getch();

        switch (c)
        {
            case '1':
            {
                DropLoot();
                break;
            }
            case '2':
            {
                for (int i = 0; i < 10; ++i)
                    DropLoot();
                break;
            }
            case '3':
            {
                for (int i = 0; i < 100; ++i)
                    DropLoot();
                break;
            }
            case '4':
            {
                while (HistogramAbsError(lootTable, lootHistogram.data(), lootTableSize) > 0.01f)
                    DropLoot();
                break;
            }
            case '5':
            {
                for (size_t index = 0; index < lootTableSize; ++index)
                    lootHistogram[index].weight = 0.0f;
                lootCount = 0;
                goldenRatioSum = 0.0f;
                sobolSampleInt = 0;
                break;
            }
            case '6':
            {
                printf("\n\n");
                return;
            }
        }
    }
}

int main(int argc, char** argv)
{
    // The loot table
    Item lootTable[] =
    {
        {'A', 10},
        {'B', 30},
        {'C', 1}
    };

    // first normalize the loot table weights so that they add up to 1.0
    float totalWeight = 0.0f;
    for (int i = 0; i < COUNT_OF(lootTable); ++i)
        totalWeight += lootTable[i].weight;
    for (int i = 0; i < COUNT_OF(lootTable); ++i)
        lootTable[i].weight /= totalWeight;

    MainMenuOption option = MainMenu();

    while (option != MainMenuOption::Exit)
    {
        LootMenu(option, lootTable, COUNT_OF(lootTable));
        option = MainMenu();
    }

    return 1;
}

/*

TODO:

* ascii art for histogram of actual drops vs loot table.
 * menu above saying keys to press.
 * roll 1, roll 100, converge (show counts), converge x100.

* white noise, blue noise, golden ratio. halton or similar?
 * could do storageless shuffle for golden ratio (or just mention it)

* explain open / progressive sequences, and how blue noise is randomized LDS while the others are deterministic.


* link to this?
 https://blog.demofox.org/2017/05/29/when-random-numbers-are-too-random-low-discrepancy-sequences/
 https://blog.demofox.org/2017/10/20/generating-blue-noise-sample-points-with-mitchells-best-candidate-algorithm/


*/