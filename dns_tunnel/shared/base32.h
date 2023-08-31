/**
 * Tento soubor byl převzat z https://github.com/google/google-authenticator-libpam/blob/master/src/base32.c
 * Následně byl upraven aby se v zakódovaných datech nepoužívala čísla, ale jen písmena
*/

#ifndef _BASE32_H_
#define _BASE32_H_


/**
 * Tato metoda dekóduje data.
 * @param encoded zakódovaná data
 * @param result proměnná do které se vkládájí dekódovaná data
 * @param bufSize velikost proměnné result
*/
int base32_decode(const uint8_t *encoded, uint8_t *result, int bufSize);

/**
 * Tato metoda zakódovává data.
 * @param data raw data
 * @param length délka raw dat
 * @param result proměnná do které se vkládájí zakódovaná data
 * @param bufSize velikost proměnné result
*/
int base32_encode(const uint8_t *data, int length, uint8_t *result, int bufSize);

#endif