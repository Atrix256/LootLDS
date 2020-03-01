#include <stdio.h>
#include <conio.h>
#include <vector>
#include <random>

// Golden ratio is 1.61803398875.
// Golden ratio conjugate is 1 / goldenRatio and is just as irrational but numbers grow less quickly using it.
// It's also goldenRatio - 1.
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

void BestCandidateBlueNoise(std::vector<float>& values, size_t numValues, std::mt19937& rng)
{
    // NOTE: this function is optimized towards generating multiple values at once (thus the sorted list)
    // but we use it to generate one at a time in this code

    // if they want less samples than there are, just truncate the sequence
    if (numValues <= values.size())
    {
        values.resize(numValues);
        return;
    }

    static std::uniform_real_distribution<float> dist(0, 1);

    // handle the special case of not having any values yet, so we don't check for it in the loops.
    if (values.size() == 0)
        values.push_back(dist(rng));

    // make a sorted list of existing samples
    std::vector<float> sortedValues;
    sortedValues = values;
    sortedValues.reserve(numValues);
    values.reserve(numValues);
    std::sort(sortedValues.begin(), sortedValues.end());

    // use whatever samples currently exist, and just add to them, since this is a progressive sequence
    for (size_t i = values.size(); i < numValues; ++i)
    {
        size_t numCandidates = values.size();
        float bestDistance = 0.0f;
        float bestCandidateValue = 0;
        size_t bestCandidateInsertLocation = 0;
        for (size_t candidate = 0; candidate < numCandidates; ++candidate)
        {
            float candidateValue = dist(rng);

            // binary search the sorted value list to find the values it's closest to.
            auto lowerBound = std::lower_bound(sortedValues.begin(), sortedValues.end(), candidateValue);
            size_t insertLocation = lowerBound - sortedValues.begin();

            // calculate the closest distance (torroidally) from this point to an existing sample by looking left and right.
            float distanceLeft = (insertLocation > 0)
                ? candidateValue - sortedValues[insertLocation - 1]
                : 1.0f + candidateValue - *sortedValues.rbegin();

            float distanceRight = (insertLocation < sortedValues.size())
                ? sortedValues[insertLocation] - candidateValue
                : distanceRight = 1.0f + sortedValues[0] - candidateValue;

            // whichever is closer left vs right is the closer point distance
            float minDist = std::min(distanceLeft, distanceRight);

            // keep the best candidate seen
            if (minDist > bestDistance)
            {
                bestDistance = minDist;
                bestCandidateValue = candidateValue;
                bestCandidateInsertLocation = insertLocation;
            }
        }

        // take the best candidate and also insert it into the sorted values
        sortedValues.insert(sortedValues.begin() + bestCandidateInsertLocation, bestCandidateValue);
        values.push_back(bestCandidateValue);
    }
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

void ShowHistogram(const std::vector<Item>& lootTable, size_t lootCount = 0, char lastDrop = 0)
{
    static const int c_histogramWidth = 40;

    float totalWeight = 0.0f;
    for (const Item& item : lootTable)
        totalWeight += item.weight;

    printf("[");

    if (totalWeight == 0.0f)
    {
        for (int i = 0; i < c_histogramWidth; ++i)
            printf("-");
    }
    else
    {
        int charsSoFar = 0;
        for (size_t index = 0; index < lootTable.size(); ++index)
        {
            int numChars = (index == lootTable.size() - 1)
                ? c_histogramWidth - charsSoFar
                : int(float(c_histogramWidth) * lootTable[index].weight / totalWeight);
            charsSoFar += numChars;

            for (int i = 0; i < numChars; ++i)
                printf("%c", lootTable[index].symbol);
        }
    }

    printf("]");

    if (lootCount > 0)
        printf(" %zu drops. Last=%c", lootCount, lastDrop);
    else
    {
        for (int i = 0; i < 20; ++i)
            printf(" ");
        for (int i = 0; i < 20; ++i)
            printf("%c", 8);
    }
}

float GenerateLootRNG(MainMenuOption option, std::mt19937& rng, size_t lootCount, float& goldenRatioSum, size_t& sobolSampleInt, std::vector<float>& blueNoise)
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
            BestCandidateBlueNoise(blueNoise, blueNoise.size() + 1, rng);
            return *blueNoise.rbegin();
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

float HistogramAbsError(const std::vector<Item>& normalizedLootTable, const std::vector<Item>& lootHistogram)
{
    float totalHistogramCount = 0.0f;
    for (const Item& item : lootHistogram)
        totalHistogramCount += item.weight;
    
    if (totalHistogramCount == 0.0f)
        return 1.0f;

    float totalAbsError = 0.0f;
    for (size_t index = 0; index < lootHistogram.size(); ++index)
    {
        float desired = normalizedLootTable[index].weight;
        float actual = lootHistogram[index].weight / totalHistogramCount;
        totalAbsError += abs(desired - actual);
    }
    return totalAbsError;
}

void NormalizeLootTable(std::vector<Item>& lootTable)
{
    float totalWeight = 0.0f;
    for (const Item& item : lootTable)
        totalWeight += item.weight;
    for (Item& item : lootTable)
        item.weight /= totalWeight;
}

void LootMenu(MainMenuOption option, std::vector<Item>& lootTable)
{
    std::random_device rd;
    std::mt19937 rng(rd());

    std::vector<Item> lootHistogram(lootTable.size());
    for (size_t index = 0; index < lootTable.size(); ++index)
    {
        lootHistogram[index].symbol = lootTable[index].symbol;
        lootHistogram[index].weight = 0.0f;
    }

    auto PrintMenuAndHistogram = [&]()
    {
        printf("Loot Menu - %s\n1) Drop 1 loot\n2) Drop 10 loot\n3) Drop 100 loot\n4) Converge (<=1%% error)\n5) Reset\n6) Randomize Loot Table & Reset\n7) Main Menu\n\n", NoiseNames[(int)option]);

        // show the actual histogram
        printf("Loot table histogram:\n");
        ShowHistogram(lootTable);

        printf("\n\nLoot drop histogram:\n");
    };

    size_t lootCount = 0;
    float goldenRatioSum = 0.0f;
    size_t sobolSampleInt = 0;
    std::vector<float> blueNoise;
    char lastDrop = 0;

    auto DropLoot = [&] ()
    {
        float lootRandomNumber = GenerateLootRNG(option, rng, lootCount, goldenRatioSum, sobolSampleInt, blueNoise);

        for (size_t index = 0; index < lootTable.size(); ++index)
        {
            lootRandomNumber -= lootTable[index].weight;

            if (lootRandomNumber <= 0.0f || index == lootTable.size() - 1)
            {
                lootHistogram[index].weight += 1.0f;
                lastDrop = lootHistogram[index].symbol;
                break;
            }
        }

        lootCount++;
    };

    PrintMenuAndHistogram();

    while (true)
    {
        printf("\r");
        ShowHistogram(lootHistogram, lootCount, lastDrop);

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
                while (HistogramAbsError(lootTable, lootHistogram) > 0.01f)
                    DropLoot();
                break;
            }
            case '5':
            {
                for (Item& item : lootHistogram)
                    item.weight = 0.0f;
                lootCount = 0;
                goldenRatioSum = 0.0f;
                sobolSampleInt = 0;
                blueNoise.clear();
                break;
            }
            case '6':
            {
                lootCount = 0;
                goldenRatioSum = 0.0f;
                sobolSampleInt = 0;
                blueNoise.clear();

                std::uniform_int_distribution<int> distInt(2, 6);
                std::uniform_real_distribution<float> distFloat(1.0f, 20.0f);
                lootTable.resize(distInt(rng));
                for (size_t index = 0; index < lootTable.size(); ++index)
                {
                    lootTable[index].symbol = 'A' + (char)index;
                    lootTable[index].weight = distFloat(rng);
                }
                NormalizeLootTable(lootTable);

                lootHistogram.resize(lootTable.size());
                for (size_t index = 0; index < lootTable.size(); ++index)
                {
                    lootHistogram[index].symbol = lootTable[index].symbol;
                    lootHistogram[index].weight = 0.0f;
                }

                printf("\n\n");
                PrintMenuAndHistogram();

                break;
            }
            case '7':
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
    std::vector<Item> lootTable =
    {
        {'A', 10},
        {'B', 30},
        {'C', 1}
    };

    // first normalize the loot table weights so that they add up to 1.0
    NormalizeLootTable(lootTable);

    MainMenuOption option = MainMenu();

    while (option != MainMenuOption::Exit)
    {
        LootMenu(option, lootTable);
        option = MainMenu();
    }

    return 1;
}
