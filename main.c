#include <stdio.h>
#include <string.h>
#include "tokenizer.h"

void debug_print_encoded(IntArray *encoded)
{
    printf("Encoded IDs: ");
    for (int i = 0; i < encoded->size; ++i)
    {
        printf("%d ", encoded->ids[i]);
    }
    printf("\n");
}

int main()
{
    const char *pattern = "'(?:[sdmt]|ll|ve|re)| ?\\p{L}+| ?\\p{N}+| ?[^\\s\\p{L}\\p{N}]+|\\s+(?!\\S)|\\s+";

    RegexTokenizer tokenizer;
    init_regex_tokenizer(&tokenizer, pattern);

    const char *text = "hello world!!123안녕하세 😉";
    train_regex_tokenizer(&tokenizer, text, 300);

    IntArray encoded;
    encode_regex_tokenizer(&tokenizer, text, &encoded);

    debug_print_encoded(&encoded);

    char decoded[1024];
    decode_regex_tokenizer(&tokenizer, &encoded, decoded, sizeof(decoded));
    printf("Decoded text: %s\n", decoded);

    free_int_array(&encoded);
    free_regex_tokenizer(&tokenizer);
    return 0;
}
