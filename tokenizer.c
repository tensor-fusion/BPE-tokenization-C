#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pcre.h>
#include "tokenizer.h"

void init_pair_count_table(PairCountTable *table, int initial_capacity)
{
    table->pairs = (Pair *)malloc(initial_capacity * sizeof(Pair));
    table->counts = (int *)malloc(initial_capacity * sizeof(int));
    if (table->pairs == NULL || table->counts == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    table->size = 0;
    table->capacity = initial_capacity;
}

void free_pair_count_table(PairCountTable *table)
{
    if (table->pairs)
        free(table->pairs);
    if (table->counts)
        free(table->counts);
    table->size = 0;
    table->capacity = 0;
}

int find_pair_index(PairCountTable *table, Pair pair)
{
    for (int i = 0; i < table->size; ++i)
    {
        if (table->pairs[i].first == pair.first && table->pairs[i].second == pair.second)
        {
            return i;
        }
    }
    return -1;
}

void add_or_update_pair_count(PairCountTable *table, Pair pair)
{
    int index = find_pair_index(table, pair);
    if (index >= 0)
    {
        table->counts[index]++;
    }
    else
    {
        if (table->size == table->capacity)
        {
            table->capacity *= 2;
            Pair *new_pairs = (Pair *)realloc(table->pairs, table->capacity * sizeof(Pair));
            int *new_counts = (int *)realloc(table->counts, table->capacity * sizeof(int));
            if (new_pairs == NULL || new_counts == NULL)
            {
                fprintf(stderr, "Memory reallocation failed\n");
                exit(1);
            }
            table->pairs = new_pairs;
            table->counts = new_counts;
        }
        table->pairs[table->size] = pair;
        table->counts[table->size] = 1;
        table->size++;
    }
}

void get_stats(int *ids, int length, PairCountTable *table)
{
    for (int i = 0; i < length - 1; ++i)
    {
        Pair pair = {ids[i], ids[i + 1]};
        add_or_update_pair_count(table, pair);
    }
}

void print_pair_counts(PairCountTable *table)
{
    for (int i = 0; i < table->size; ++i)
    {
        printf("Pair (%d, %d): %d\n", table->pairs[i].first, table->pairs[i].second, table->counts[i]);
    }
}

void init_int_array(IntArray *array, int initial_capacity)
{
    array->ids = (int *)malloc(initial_capacity * sizeof(int));
    if (array->ids == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    array->size = 0;
    array->capacity = initial_capacity;
}

void append_int_array(IntArray *array, int value)
{
    if (array->size == array->capacity)
    {
        array->capacity *= 2;
        int *new_ids = (int *)realloc(array->ids, array->capacity * sizeof(int));
        if (new_ids == NULL)
        {
            fprintf(stderr, "Memory reallocation failed\n");
            exit(1);
        }
        array->ids = new_ids;
    }
    array->ids[array->size++] = value;
}

void free_int_array(IntArray *array)
{
    if (array->ids)
        free(array->ids);
    array->size = 0;
    array->capacity = 0;
}

void merge(int *ids, int length, Pair pair, int idx, IntArray *result)
{
    init_int_array(result, length);
    printf("Merging pair: (%d, %d) into idx: %d\n", pair.first, pair.second, idx);

    int i = 0;
    while (i < length)
    {
        if (i < length - 1 && ids[i] == pair.first && ids[i + 1] == pair.second)
        {
            printf("Found pair at position %d, replacing with %d\n", i, idx);
            append_int_array(result, idx);
            i += 2;
        }
        else
        {
            append_int_array(result, ids[i]);
            i++;
        }
    }
    printf("Resulting array after merge: ");
    for (i = 0; i < result->size; ++i)
    {
        printf("%d ", result->ids[i]);
    }
    printf("\n");
}

void replace_control_characters(const char *input, char *output, int output_size)
{
    int j = 0;
    for (int i = 0; input[i] != '\0' && j < output_size - 1; ++i)
    {
        if (iscntrl((unsigned char)input[i]))
        {
            j += snprintf(output + j, output_size - j, "\\u%04x", (unsigned char)input[i]);
        }
        else
        {
            output[j++] = input[i];
        }
    }
    output[j] = '\0';
}

void render_token(const char *input, char *output, int output_size)
{
    char temp[output_size];
    snprintf(temp, output_size, "%s", input);
    replace_control_characters(temp, output, output_size);
}

void init_regex_tokenizer(RegexTokenizer *tokenizer, const char *pattern)
{
    tokenizer->merges = NULL;
    tokenizer->merge_size = 0;
    tokenizer->merge_capacity = 256;
    tokenizer->merges = (Pair *)malloc(tokenizer->merge_capacity * sizeof(Pair));
    if (tokenizer->merges == NULL)
    {
        fprintf(stderr, "Memory allocation failed for merges\n");
        exit(1);
    }
    tokenizer->pattern = strdup(pattern);
    if (tokenizer->pattern == NULL)
    {
        fprintf(stderr, "Memory allocation failed for pattern\n");
        exit(1);
    }

    const char *error;
    int erroffset;
    tokenizer->compiled_pattern = pcre_compile(
        tokenizer->pattern,
        PCRE_UTF8,
        &error,
        &erroffset,
        NULL);
    if (tokenizer->compiled_pattern == NULL)
    {
        fprintf(stderr, "PCRE compilation failed at offset %d: %s\n", erroffset, error);
        exit(1);
    }

    tokenizer->compiled_pattern_extra = pcre_study(tokenizer->compiled_pattern, 0, &error);
    if (error != NULL)
    {
        fprintf(stderr, "PCRE study failed: %s\n", error);
        exit(1);
    }

    tokenizer->special_tokens = NULL;
    tokenizer->special_size = 0;
    tokenizer->special_capacity = 0;
    tokenizer->vocab = (char **)malloc(256 * sizeof(char *));
    if (tokenizer->vocab == NULL)
    {
        fprintf(stderr, "Memory allocation failed for vocab\n");
        exit(1);
    }
    for (int i = 0; i < 256; ++i)
    {
        tokenizer->vocab[i] = (char *)malloc(2);
        if (tokenizer->vocab[i] == NULL)
        {
            fprintf(stderr, "Memory allocation failed for vocab[%d]\n", i);
            exit(1);
        }
        tokenizer->vocab[i][0] = (char)i;
        tokenizer->vocab[i][1] = '\0';
    }
}

void free_regex_tokenizer(RegexTokenizer *tokenizer)
{
    free(tokenizer->pattern);
    free(tokenizer->special_tokens);
    for (int i = 0; i < 256; ++i)
    {
        free(tokenizer->vocab[i]);
    }
    free(tokenizer->vocab);
    if (tokenizer->merges)
    {
        free(tokenizer->merges);
    }
    if (tokenizer->compiled_pattern)
    {
        pcre_free(tokenizer->compiled_pattern);
    }
    if (tokenizer->compiled_pattern_extra)
    {
        pcre_free_study(tokenizer->compiled_pattern_extra);
    }
}

void build_vocab(RegexTokenizer *tokenizer)
{
    for (int i = 0; i < 256; ++i)
    {
        tokenizer->vocab[i] = (char *)malloc(2);
        if (tokenizer->vocab[i] == NULL)
        {
            fprintf(stderr, "Memory allocation failed\n");
            exit(1);
        }
        tokenizer->vocab[i][0] = (char)i;
        tokenizer->vocab[i][1] = '\0';
    }
    for (int i = 0; i < tokenizer->merge_size; ++i)
    {
        Pair pair = tokenizer->merges[i];
        int idx = 256 + i;
        char *merged = (char *)malloc(strlen(tokenizer->vocab[pair.first]) + strlen(tokenizer->vocab[pair.second]) + 1);
        if (merged == NULL)
        {
            fprintf(stderr, "Memory allocation failed\n");
            exit(1);
        }
        strcpy(merged, tokenizer->vocab[pair.first]);
        strcat(merged, tokenizer->vocab[pair.second]);
        tokenizer->vocab[idx] = merged;
    }
    for (int i = 0; i < tokenizer->special_size; ++i)
    {
        int idx = tokenizer->special_tokens[i];
        char *special = (char *)malloc(2);
        if (special == NULL)
        {
            fprintf(stderr, "Memory allocation failed\n");
            exit(1);
        }
        special[0] = (char)idx;
        special[1] = '\0';
        tokenizer->vocab[idx] = special;
    }
}

void save_tokenizer(RegexTokenizer *tokenizer, const char *file_prefix)
{
    char model_file[256];
    snprintf(model_file, sizeof(model_file), "%s.model", file_prefix);
    FILE *f = fopen(model_file, "w");
    if (!f)
    {
        perror("Failed to open model file");
        return;
    }
    fprintf(f, "minbpe v1\n");
    fprintf(f, "%s\n", tokenizer->pattern);
    fprintf(f, "%d\n", tokenizer->special_size);
    for (int i = 0; i < tokenizer->special_size; ++i)
    {
        fprintf(f, "%d\n", tokenizer->special_tokens[i]);
    }
    for (int i = 0; i < tokenizer->merge_size; ++i)
    {
        fprintf(f, "%d %d\n", tokenizer->merges[i].first, tokenizer->merges[i].second);
    }
    fclose(f);
}

void load_tokenizer(RegexTokenizer *tokenizer, const char *model_file)
{
    FILE *f = fopen(model_file, "r");
    if (!f)
    {
        perror("Failed to open model file");
        return;
    }
    char version[16];
    fscanf(f, "%15s", version);
    if (strcmp(version, "minbpe v1") != 0)
    {
        fprintf(stderr, "Unsupported version: %s\n", version);
        fclose(f);
        return;
    }
    size_t pattern_size = 256;
    tokenizer->pattern = (char *)malloc(pattern_size);
    if (tokenizer->pattern == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    getline(&tokenizer->pattern, &pattern_size, f);
    int num_special;
    fscanf(f, "%d", &num_special);
    tokenizer->special_tokens = (int *)malloc(num_special * sizeof(int));
    if (tokenizer->special_tokens == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    tokenizer->special_size = num_special;
    for (int i = 0; i < num_special; ++i)
    {
        fscanf(f, "%d", &tokenizer->special_tokens[i]);
    }
    tokenizer->merge_capacity = 256;
    tokenizer->merges = (Pair *)malloc(tokenizer->merge_capacity * sizeof(Pair));
    if (tokenizer->merges == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    tokenizer->merge_size = 0;
    int idx = 256;
    while (fscanf(f, "%d %d", &tokenizer->merges[tokenizer->merge_size].first, &tokenizer->merges[tokenizer->merge_size].second) == 2)
    {
        tokenizer->merge_size++;
        if (tokenizer->merge_size == tokenizer->merge_capacity)
        {
            tokenizer->merge_capacity *= 2;
            Pair *new_merges = (Pair *)realloc(tokenizer->merges, tokenizer->merge_capacity * sizeof(Pair));
            if (new_merges == NULL)
            {
                fprintf(stderr, "Memory reallocation failed\n");
                exit(1);
            }
            tokenizer->merges = new_merges;
        }
        idx++;
    }
    fclose(f);
    build_vocab(tokenizer);
}

void train_regex_tokenizer(RegexTokenizer *tokenizer, const char *text, int vocab_size)
{
    if (vocab_size < 256)
    {
        fprintf(stderr, "Vocab size must be at least 256\n");
        return;
    }
    int num_merges = vocab_size - 256;

    // Split the text into text chunks using the regex pattern
    int rc;
    int ovector[30];
    const char *ptr = text;
    IntArray ids;
    init_int_array(&ids, 256);
    while ((rc = pcre_exec(tokenizer->compiled_pattern, tokenizer->compiled_pattern_extra, ptr, strlen(ptr), 0, 0, ovector, 30)) >= 0)
    {
        for (int i = ovector[0]; i < ovector[1]; ++i)
        {
            append_int_array(&ids, (int)(unsigned char)ptr[i]);
        }
        ptr += ovector[1];
    }

    // Initialize merge structures
    PairCountTable stats;
    init_pair_count_table(&stats, 256);

    // Iteratively merge the most common pairs to create new tokens
    for (int i = 0; i < num_merges; ++i)
    {
        // Count the number of times every consecutive pair appears
        get_stats(ids.ids, ids.size, &stats);

        // Find the pair with the highest count
        int max_count = -1;
        Pair max_pair = {0, 0};
        for (int j = 0; j < stats.size; ++j)
        {
            if (stats.counts[j] > max_count)
            {
                max_count = stats.counts[j];
                max_pair = stats.pairs[j];
            }
        }

        // Mint a new token: assign it the next available id
        int idx = 256 + i;

        // Replace all occurrences of pair in ids with idx
        IntArray new_ids;
        merge(ids.ids, ids.size, max_pair, idx, &new_ids);

        // Save the merge
        if (tokenizer->merge_size == tokenizer->merge_capacity)
        {
            tokenizer->merge_capacity *= 2;
            Pair *new_merges = (Pair *)realloc(tokenizer->merges, tokenizer->merge_capacity * sizeof(Pair));
            if (new_merges == NULL)
            {
                fprintf(stderr, "Memory reallocation failed\n");
                exit(1);
            }
            tokenizer->merges = new_merges;
        }
        tokenizer->merges[tokenizer->merge_size++] = max_pair;

        // Free old ids and use the new ids
        free_int_array(&ids);
        ids = new_ids;

        // Reset stats for the next iteration
        free_pair_count_table(&stats);
        init_pair_count_table(&stats, 256);
    }

    // Clean up
    free_int_array(&ids);
    free_pair_count_table(&stats);
}

void encode_regex_tokenizer(RegexTokenizer *tokenizer, const char *text, IntArray *result)
{
    int rc;
    int ovector[30];
    const char *ptr = text;
    init_int_array(result, 256);
    while ((rc = pcre_exec(tokenizer->compiled_pattern, tokenizer->compiled_pattern_extra, ptr, strlen(ptr), 0, 0, ovector, 30)) >= 0)
    {
        for (int i = ovector[0]; i < ovector[1]; ++i)
        {
            append_int_array(result, (int)(unsigned char)ptr[i]);
            printf("Encoding character: %c (%d)\n", ptr[i], (int)(unsigned char)ptr[i]);
        }
        ptr += ovector[1];
    }

    IntArray encoded;
    init_int_array(&encoded, result->size);

    while (result->size >= 2)
    {
        PairCountTable stats;
        init_pair_count_table(&stats, 256);
        get_stats(result->ids, result->size, &stats);
        int min_index = -1;
        Pair min_pair = {0, 0};
        for (int i = 0; i < stats.size; ++i)
        {
            if (stats.pairs[i].first >= tokenizer->merge_size || stats.pairs[i].second >= tokenizer->merge_size)
            {
                continue; // Skip invalid pairs
            }
            if (min_index == -1 || tokenizer->merges[stats.pairs[i].first].first < tokenizer->merges[min_pair.first].first)
            {
                min_pair = stats.pairs[i];
                min_index = i;
            }
        }
        if (min_index == -1)
        {
            break;
        }
        int idx = tokenizer->merges[min_pair.first].first;
        IntArray temp_encoded;
        init_int_array(&temp_encoded, result->size);
        merge(result->ids, result->size, min_pair, idx, &temp_encoded);
        free_int_array(result);
        *result = temp_encoded;
        free_pair_count_table(&stats);
    }

    for (int i = 0; i < result->size; ++i)
    {
        append_int_array(&encoded, result->ids[i]);
    }

    free_int_array(result);
    *result = encoded;
}

void add_decoded_token(int idx, int *decoded_tokens, int *decoded_size)
{
    decoded_tokens[*decoded_size] = idx;
    (*decoded_size)++;
}

int is_already_decoded(int idx, int *decoded_tokens, int decoded_size)
{
    for (int i = 0; i < decoded_size; ++i)
    {
        if (decoded_tokens[i] == idx)
        {
            return 1;
        }
    }
    return 0;
}

void decode_merge_token(RegexTokenizer *tokenizer, int idx, char *output, int *pos, int output_size, int *decoded_tokens, int *decoded_size)
{
    if (idx < 256)
    {
        // Single character token
        if (*pos < output_size - 1)
        {
            output[(*pos)++] = (char)idx;
            printf("Decoded single char: %c at pos: %d\n", (char)idx, *pos - 1);
        }
    }
    else if (idx >= 256 && idx < 256 + tokenizer->merge_size)
    {
        // Merge token
        int merge_idx = idx - 256;
        if (!is_already_decoded(idx, decoded_tokens, *decoded_size))
        {
            add_decoded_token(idx, decoded_tokens, decoded_size);
            decode_merge_token(tokenizer, tokenizer->merges[merge_idx].first, output, pos, output_size, decoded_tokens, decoded_size);
            decode_merge_token(tokenizer, tokenizer->merges[merge_idx].second, output, pos, output_size, decoded_tokens, decoded_size);
        }
    }
    else
    {
        fprintf(stderr, "Invalid token id: %d\n", idx);
    }
}

void decode_regex_tokenizer(RegexTokenizer *tokenizer, IntArray *ids, char *output, int output_size)
{
    int pos = 0;
    int decoded_tokens[1024];
    int decoded_size = 0;

    printf("Decoding sequence: ");
    for (int i = 0; i < ids->size; ++i)
    {
        printf("%d ", ids->ids[i]);
    }
    printf("\n");

    for (int i = 0; i < ids->size; ++i)
    {
        int idx = ids->ids[i];
        printf("Decoding token id: %d\n", idx);
        decode_merge_token(tokenizer, idx, output, &pos, output_size, decoded_tokens, &decoded_size);
    }
    output[pos] = '\0'; // Null-terminate the output string
    printf("Final decoded output: %s\n", output);
}
