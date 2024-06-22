#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stdbool.h>
#include <pcre.h>

typedef struct
{
    int first;
    int second;
} Pair;

typedef struct
{
    Pair *pairs;
    int *counts;
    int size;
    int capacity;
} PairCountTable;

typedef struct
{
    int *ids;
    int size;
    int capacity;
} IntArray;

typedef struct
{
    Pair *merges;
    int merge_size;
    int merge_capacity;
    char *pattern;
    pcre *compiled_pattern;
    pcre_extra *compiled_pattern_extra;
    int *special_tokens;
    int special_size;
    int special_capacity;
    char **vocab;
} RegexTokenizer;

void init_pair_count_table(PairCountTable *table, int initial_capacity);
void free_pair_count_table(PairCountTable *table);
void add_or_update_pair_count(PairCountTable *table, Pair pair);
void get_stats(int *ids, int length, PairCountTable *table);
void print_pair_counts(PairCountTable *table);

void init_int_array(IntArray *array, int initial_capacity);
void append_int_array(IntArray *array, int value);
void free_int_array(IntArray *array);

void merge(int *ids, int length, Pair pair, int idx, IntArray *result);

void replace_control_characters(const char *input, char *output, int output_size);
void render_token(const char *input, char *output, int output_size);

void init_regex_tokenizer(RegexTokenizer *tokenizer, const char *pattern);
void free_regex_tokenizer(RegexTokenizer *tokenizer);
void train_regex_tokenizer(RegexTokenizer *tokenizer, const char *text, int vocab_size);
void encode_regex_tokenizer(RegexTokenizer *tokenizer, const char *text, IntArray *result);
void decode_regex_tokenizer(RegexTokenizer *tokenizer, IntArray *ids, char *output, int output_size);
void build_vocab(RegexTokenizer *tokenizer);
void save_tokenizer(RegexTokenizer *tokenizer, const char *file_prefix);
void load_tokenizer(RegexTokenizer *tokenizer, const char *model_file);

#endif // TOKENIZER_H
