#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define NOB_IMPLEMENTATION
#include "nob.h"

void hammingGenNumberMatrix(uint64_t m, uint64_t n, uint8_t numberMatrix[m][n]) {
    for (uint8_t y = 1; y <= n; ++y) {
        for (uint8_t x = 0; x < m; ++x) {
            numberMatrix[m-x-1][y-1] = (y >> x) & 1;
        }
    }
}

void hammingMatmul(uint64_t m, uint64_t n, uint8_t resultMatrix[m], uint8_t numberMatrix[m][n], uint8_t wordMatrix[n]) {
    for (uint64_t y = 0; y < m; ++y) {
        resultMatrix[y] = 0;
        for (uint64_t x = 0; x < n; ++x) {
            resultMatrix[y] ^= numberMatrix[y][x] * wordMatrix[x];
        }
    }
}

void hammingMatrixFromNumber(uint64_t m, uint64_t src, uint8_t dest[m]) {
    for (uint64_t y = 0; y < m; ++y) {
        dest[y] = (src >> (m - y - 1)) & 1;
    }
}

uint64_t hammingMatrixToNumber(uint64_t m, uint8_t src[m]) {
    uint64_t dest = 0;
    for (uint64_t y = 0; y < m; ++y) {
        dest |= src[y] << (m - y - 1);
    }
    return dest;
}

// Corrects the word and returns whether the word was already correct
bool hammingCorrectWord(uint64_t m, uint64_t n, uint8_t wordMatrix[n], uint8_t resultMatrix[m]) {
    uint64_t correctionPoint = hammingMatrixToNumber(m, resultMatrix);
    if (correctionPoint != 0) {
        wordMatrix[correctionPoint - 1] = (~wordMatrix[correctionPoint - 1]) & 1;
        return false;
    }
    return true;
}

void printMatrix(uint64_t rows, uint64_t cols, uint8_t matrix[rows][cols]) {
    for (uint8_t y = 0; y < rows; ++y) {
        for (uint8_t x = 0; x < cols; ++x) {
            printf("%d ", matrix[y][x]);
        }
        printf("\n");
    }
}

void print1DMatrix(uint64_t cols, uint8_t matrix[cols]) {
    for (uint8_t x = 0; x < cols; ++x) {
        printf("%d ", matrix[x]);
    }
    printf("\n");
}

void printDivider(uint64_t n) {
    for (uint8_t x = 0; x < 2*n-1; ++x) {
        printf("=");
    }
    printf("\n");
}

void printUsage(const char* program) {
    printf("Usage: %s [flags]\n", program);
    printf("Possible flags are:\n");
    printf("    --help      Display this help text.\n");
#ifdef _WIN32
    printf("    --color     Use colored text.\n");
#else
    printf("    --nocolor   Don't use colored text.\n");
#endif // _WIN32
}


// Colors from https://gist.github.com/RabaDabaDoba/145049536f815903c79944599c6f952a
// Text with red background
#define RED "\e[0;41m"
// Reset
#define COLOR_RESET "\e[0m"

// Use inferior random functions on Windows, because Windows isn't Posix compliant
#ifdef _WIN32
    #define random rand
    #define srandom srand
#endif // _WIN32


#define HAMMING_M 4
#define HAMMING_N ((uint64_t) pow(2, HAMMING_M) - 1)
#define HAMMING_BIGN ((uint64_t) pow(2, pow(2, HAMMING_M) - HAMMING_M - 1))

int main(int argc, char* argv[]) {
    srandom(time(NULL));

    const uint64_t m = HAMMING_M;
    const uint64_t n = HAMMING_N;
    const uint64_t N = HAMMING_BIGN;

    // Flag handling
    const char* program = nob_shift_args(&argc, &argv);
#ifdef _WIN32
    bool flag_noColor = true;
#else
    bool flag_noColor = false;
#endif // _WIN32
    while (argc > 0) {
        const char* arg = nob_shift_args(&argc, &argv);
        if (strcmp(arg, "--help") == 0) {
            printUsage(program);
            return 0;
        } else if (strcmp(arg, "--nocolor") == 0) {
            flag_noColor = true;
        } else if (strcmp(arg, "--color") == 0) {
            flag_noColor = false;
        } else {
            printUsage(program);
            return 1;
        }
    }

    printf("m: %"PRIu64"\nn: %"PRIu64"\nN: %"PRIu64"\n", m, n, N);
    printDivider(n);

    uint8_t numberMatrix[m][n];
    hammingGenNumberMatrix(m, n, numberMatrix);

    //// Generating the words

    // TODO: make this more memory efficient so it doesn't cause a stack overflow with m>4
    uint8_t words[N][n];

    printf("Calculating words...\t");
    uint64_t index = 0;
    uint8_t result[m];
    int prevPercent = 0;
    for (uint64_t currentWord = 0; currentWord < pow(2, n); ++currentWord) {
        hammingMatrixFromNumber(n, currentWord, words[index]);
        hammingMatmul(m, n, result, numberMatrix, words[index]);
        bool success = true;
        for (uint64_t y = 0; y < m; ++y) {
            if (result[y] > 0) {
                success = false;
                break;
            }
        }
        if (success) {
            index++;
            int percent = (int) floorf((float) index / N * 100);
            if (percent % 10 == 0 && percent > prevPercent) {
                printf("%d%%  ", percent);
                prevPercent = percent;
            }
            // If this was the last word, we can break out of the loop early
            // (this will probably never happen because all 1s is always a word)
            if (index == N)
                break;
        }
    }
    printf("\n");

    printDivider(n);

    //// Encoding text

    printf("Type the message you want to encode: ");
    char message[1024] = {0};
    fgets(message, 1024, stdin);
    uint64_t length = strlen(message);
    // Remove the newline at the end
    message[length-1] = '\0';
    length--;

    printf("Encoding message...\n");
    uint64_t encodedLength = n*length;
    uint8_t encodedMessage[encodedLength];

    // This only uses 127 of the 2048 words, but this is just a proof of concept.
    for (uint64_t i = 0; i < length; ++i) {
        for (uint64_t x = 0; x < n; ++x) {
            // Don't use only the first 127 words, spread them out instead.
            uint64_t wordIndex = (message[i]*N)/128;
            encodedMessage[n*i + x] = words[wordIndex][x];
            // encodedMessage[n*i + x] = words[(int8_t)message[i]][x];
        }
    }

    printf("Do you want to see the encoded message? [Y/n] ");
    char ynbuffer[20];
    fgets(ynbuffer, 20, stdin);
    if (!(ynbuffer[0] == 'n' || ynbuffer[0] == 'N')) {
        printDivider(n);
        for (uint64_t i = 0; i < encodedLength; ++i) {
            if (i % n == 0 && i != 0)
                printf(" ");
            if (i % (n*5) == 0 && i != 0)
                printf("\n");
            printf("%d", encodedMessage[i]);
        }
        printf("\n");
        printDivider(n);
    }

    printf("Now, we will simulate sending the message by flipping a few bits randomly.\n");
    printf("Please enter the chance of a bit being flipped (default: 1 in 100): 1 in ");
    char chanceBuffer[32];
    fgets(chanceBuffer, 32, stdin);
    double chanceDenom = strtod(chanceBuffer, NULL);
    if (chanceDenom == 0.0f)
        chanceDenom = 100.0f;
    double chance = 1.0 / chanceDenom;
    
    for (uint64_t i = 0; i < encodedLength; ++i) {
        double percent = (double)random() / RAND_MAX;
        if (percent <= chance) {
            encodedMessage[i] = (~encodedMessage[i]) & 1;
        }
    }

    printf("Do you want to see the recieved (corrupted) message? [y/N] ");
    fgets(ynbuffer, 20, stdin);
    if (ynbuffer[0] == 'y' || ynbuffer[0] == 'Y') {
        printDivider(n);
        for (uint64_t i = 0; i < encodedLength; ++i) {
            if (i % n == 0 && i != 0)
                printf(" ");
            if (i % (n*5) == 0 && i != 0)
                printf("\n");
            printf("%d", encodedMessage[i]);
        }
        printf("\n");
        printDivider(n);
    }

    printf("We will now correct the message!\n");
    printf("The corrected message is: ");

    uint8_t currentWord[n];
    uint8_t resultMatrix[m];
    uint64_t amountOfCorrections = 0;
    for (uint64_t i = 0; i < encodedLength; i += n) {
        // Populate the current word
        for (uint8_t y = 0; y < n; ++y) {
            currentWord[y] = encodedMessage[i + y];
        }

        // Correct the current word
        hammingMatmul(m, n, resultMatrix, numberMatrix, currentWord);
        bool thisWordWasCorrected = false;
        if (!hammingCorrectWord(m, n, currentWord, resultMatrix)) {
            amountOfCorrections++;
            thisWordWasCorrected = true;
        }

        // Now to really inefficiently check which word it is
        for (uint64_t y = 0; y < N; ++y) {
            bool thisIsIt = true;
            for (uint64_t x = 0; x < n; ++x) {
                if (currentWord[x] != words[y][x]) {
                    thisIsIt = false;
                    break;
                }
            }
            if (thisIsIt) {
                uint8_t c = (y * 128) / N;
                if (c > 127)
                    c = '?';

                // Color the corrected characters red if color is enabled
                if (thisWordWasCorrected && !flag_noColor)
                    printf(RED);

                printf("%c", (char) c);

                // Reset the color afterwards, of course
                if (thisWordWasCorrected && !flag_noColor)
                    printf(COLOR_RESET);

                break;
            }
        }
    }
    printf("\n");
    printDivider(n);
    printf("%"PRIu64" words were corrected.\n", amountOfCorrections);

#ifdef _WIN32
    printDivider(n);
    printf("Press ENTER to exit...\n");
    getchar();
#endif // _WIN32

    return 0;

    //// Correcting a word and printing stuff (unused now)

    {
        // Print the number matrix
        printMatrix(m, n, numberMatrix);

        // uint8_t wordMatrix[n];
        // uint8_t* wordMatrix = words[1364];
        uint8_t wordMatrix[] = {1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1};

        printDivider(n);
        // Print the word
        print1DMatrix(n, wordMatrix);
        printDivider(n);

        uint8_t resultMatrix[m];
        hammingMatmul(m, n, resultMatrix, numberMatrix, wordMatrix);

        print1DMatrix(m, resultMatrix);

        printDivider(n);

        // Correct the word
        if (hammingCorrectWord(m, n, wordMatrix, resultMatrix)) {
            // Print the word again
            print1DMatrix(n, wordMatrix);
        } else {
            printf("The word is correct!\n");
        }
    }

    return 0;
}