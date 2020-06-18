#include <tuple>
#include <cstdint>
using namespace std;

/* 
 * Returns (lowest clashing address, stride between clashes, number of clashes)
 * See https://smartech.gatech.edu/handle/1853/38798
 * Ranges must be at least overlapping
 */
typedef tuple<uintptr_t, uintptr_t, uintptr_t> AliasT;
AliasT dynamic_gcd(uintptr_t low1, uintptr_t low2, uintptr_t high1, uintptr_t high2, uintptr_t stride1, uintptr_t stride2);
