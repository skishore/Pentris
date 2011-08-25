
using namespace std;

#include <cstdlib>
#include <complex>
#include <vector>
#include <math.h>

vector<int> primes;
bool prgInited = false;
bool safe = false;
unsigned int x;
int p, q, n;

void initPRG() {
    vector<int> allPrimes;
    int smallPrimes = 1;
    bool isPrime;
    float root; 


    allPrimes.push_back(2);
    for (int i = 3; i < 256; i+=2) {
        isPrime = true;
        root = sqrt(i);
        for (int j = 0; j < allPrimes.size(); j++) {
            if (allPrimes[j] > root) 
                break;
            else if (i%allPrimes[j] == 0) {
                isPrime = false;
                break;
            }
        }
        if (isPrime) {
            allPrimes.push_back(i);
            if (i%4 == 3) {
                primes.push_back(i);
                if (i < 128) smallPrimes++;
            }
        }
    }

    primes.erase(primes.begin(), primes.begin() + smallPrimes);
    prgInited = true;
}

    #include<iostream>
void seedPRG(unsigned int seed, bool useSafety=false) {
    if (useSafety) {
        safe = true;
        srand(seed);
    }

    if (prgInited == false) initPRG();

    int numPrimes = primes.size();
    int pIndex = seed%numPrimes;
    int qIndex = (seed/numPrimes)%numPrimes;
    //if (qIndex >= pIndex) qIndex++;

    p = primes[pIndex];
    q = primes[qIndex];
    n = p*q;
    x = (seed/(numPrimes*numPrimes))%n;

    //if (x%p == 0) x += q;
    //if (x%q == 0) x += p;
}

// returns a pseudorandom 32-bit integer
unsigned int prg() {
    int curBit = 1;
    unsigned int result = 0;

    if (safe) return rand();

    for (int i = 0; i < 37; i++) {
        x = (x*x)%n;
        if (i < 32) {
            result += curBit*(x%2);
            curBit = 2*curBit;
        }
    }

    return result;
}

