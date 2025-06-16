/**
 * evolver0_minimal.c - 最小化自进化内核
 * 专注于代码简化和优化的进化策略
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define VERSION 0
#define MAX_CODE_SIZE 1024*1024

typedef struct {
    int mutation_rate;
    int population_size;
    char fitness_metric;
} EvolveParams;

typedef struct {
    char *self_code;
    size_t code_size;
    int generation;
    double fitness;
    EvolveParams params;
} EvolveState;

static char* read_self();
static size_t get_file_size(FILE *f);
static int compile_and_test(const char *code, const char *temp_name);
static char* mutate_code_minimal(const char *code, size_t code_size, EvolveParams params);
static double evaluate_fitness(const char *code, size_t code_size, char metric);
static void save_if_better(const char *code, double fitness);
static void evolve_next_generation(EvolveState *state);

int main(int argc, char *argv[]) {
    srand(time(NULL));
    
    EvolveState state;
    state.generation = VERSION;
    state.fitness = 0.0;
    state.params.mutation_rate = 10;
    state.params.population_size = 5;
    state.params.fitness_metric = 's';
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-e") == 0) {
            printf("启动最小化进化模式，当前代数: %d\n", state.generation);
            
            state.self_code = read_self();
            if (!state.self_code) {
                fprintf(stderr, "无法读取自身代码\n");
                return 1;
            }
            state.code_size = strlen(state.self_code);
            
            evolve_next_generation(&state);
            
            free(state.self_code);
            return 0;
        }
    }
    
    printf("evolver%d_minimal - 最小化自进化内核\n", VERSION);
    printf("用法: %s -e\n", argv[0]);
    return 0;
}

static char* read_self() {
    char exe_path[256];
    sprintf(exe_path, "evolver%d_minimal.c", VERSION);
    
    FILE *f = fopen(exe_path, "rb");
    if (!f) return NULL;
    
    size_t size = get_file_size(f);
    if (size == 0 || size > MAX_CODE_SIZE) {
        fclose(f);
        return NULL;
    }
    
    char *code = (char*)malloc(size + 1);
    if (!code) {
        fclose(f);
        return NULL;
    }
    
    fread(code, 1, size, f);
    fclose(f);
    code[size] = '\0';
    return code;
}

static size_t get_file_size(FILE *f) {
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    return size;
}

static int compile_and_test(const char *code, const char *temp_name) {
    char temp_source[256], temp_binary[256], compile_cmd[512];
    
    sprintf(temp_source, "/tmp/%s.c", temp_name);
    sprintf(temp_binary, "/tmp/%s", temp_name);
    
    FILE *f = fopen(temp_source, "w");
    if (!f) return -1;
    
    fwrite(code, 1, strlen(code), f);
    fclose(f);
    
    sprintf(compile_cmd, "gcc -o %s %s 2>/dev/null", temp_binary, temp_source);
    int result = system(compile_cmd);
    
    unlink(temp_source);
    if (result == 0) {
        unlink(temp_binary);
        return 0;
    }
    return -1;
}

static char* mutate_code_minimal(const char *code, size_t code_size, EvolveParams params) {
    if (!code || code_size == 0) return NULL;
    
    char *mutated = (char*)malloc(code_size + 512);
    if (!mutated) return NULL;
    
    strcpy(mutated, code);
    size_t mutated_size = code_size;
    
    // 最小化变异策略：删除、简化、优化
    int num_mutations = 1 + (rand() % 3);
    
    for (int i = 0; i < num_mutations; i++) {
        int mutation_type = rand() % 4;
        
        switch (mutation_type) {
            case 0: // 删除多余的空行
                {
                    char *double_newline = strstr(mutated, "\n\n\n");
                    if (double_newline) {
                        memmove(double_newline + 1, double_newline + 2,
                                mutated_size - (double_newline - mutated) - 1);
                        mutated_size--;
                    }
                }
                break;
                
            case 1: // 简化变量名
                {
                    char *patterns[][2] = {
                        {"mutation_rate", "mr"},
                        {"population_size", "ps"},
                        {"fitness_metric", "fm"},
                        {"mutated_size", "ms"}
                    };
                    
                    for (int j = 0; j < 4; j++) {
                        char *pos = strstr(mutated, patterns[j][0]);
                        if (pos && (rand() % 3 == 0)) {
                            int old_len = strlen(patterns[j][0]);
                            int new_len = strlen(patterns[j][1]);
                            
                            memmove(pos + new_len, pos + old_len,
                                    mutated_size - (pos - mutated) - old_len + 1);
                            memcpy(pos, patterns[j][1], new_len);
                            mutated_size = mutated_size - old_len + new_len;
                            break;
                        }
                    }
                }
                break;
                
            case 2: // 删除注释
                {
                    char *comment = strstr(mutated, "//");
                    if (comment) {
                        char *line_end = strchr(comment, '\n');
                        if (line_end) {
                            memmove(comment, line_end,
                                    mutated_size - (line_end - mutated) + 1);
                            mutated_size -= (line_end - comment);
                        }
                    }
                }
                break;
                
            case 3: // 简化数字常量
                {
                    char *patterns[][2] = {
                        {"1024*1024", "1048576"},
                        {"256", "255"},
                        {"512", "511"}
                    };
                    
                    for (int j = 0; j < 3; j++) {
                        char *pos = strstr(mutated, patterns[j][0]);
                        if (pos && (rand() % 2 == 0)) {
                            int old_len = strlen(patterns[j][0]);
                            int new_len = strlen(patterns[j][1]);
                            
                            memmove(pos + new_len, pos + old_len,
                                    mutated_size - (pos - mutated) - old_len + 1);
                            memcpy(pos, patterns[j][1], new_len);
                            mutated_size = mutated_size - old_len + new_len;
                            break;
                        }
                    }
                }
                break;
        }
    }
    
    // 更新版本号
    char *version_pos = strstr(mutated, "#define VERSION");
    if (version_pos) {
        char version_str[32];
        sprintf(version_str, "#define VERSION %d", VERSION + 1);
        
        char *line_end = strchr(version_pos, '\n');
        if (line_end) {
            int old_len = line_end - version_pos;
            int new_len = strlen(version_str);
            
            memmove(version_pos + new_len, line_end,
                    mutated_size - (line_end - mutated) + 1);
            memcpy(version_pos, version_str, new_len);
            mutated_size = mutated_size - old_len + new_len;
        }
    }
    
    mutated[mutated_size] = '\0';
    return mutated;
}

static double evaluate_fitness(const char *code, size_t code_size, char metric) {
    if (!code || code_size == 0) return 0.0;
    
    char temp_name[32];
    sprintf(temp_name, "test_%d", rand());
    
    // 首先检查是否能编译
    if (compile_and_test(code, temp_name) != 0) {
        return 0.0; // 编译失败，适应度为0
    }
    
    // 编译成功，根据大小计算适应度
    return 1000000.0 / (double)code_size;
}

static void save_if_better(const char *code, double fitness) {
    char next_filename[64];
    sprintf(next_filename, "evolver%d_minimal.c", VERSION + 1);
    
    FILE *f = fopen(next_filename, "w");
    if (f) {
        fwrite(code, 1, strlen(code), f);
        fclose(f);
        printf("已保存新一代: %s (适应度: %.4f, 大小: %zu)\n", 
               next_filename, fitness, strlen(code));
        
        char compile_cmd[256];
        sprintf(compile_cmd, "gcc -o evolver%d_minimal %s", VERSION + 1, next_filename);
        if (system(compile_cmd) == 0) {
            printf("新版本编译成功！\n");
        }
    }
}

static void evolve_next_generation(EvolveState *state) {
    if (!state || !state->self_code) return;
    
    printf("开始最小化进化第%d代...\n", state->generation + 1);
    
    state->fitness = evaluate_fitness(state->self_code, state->code_size, state->params.fitness_metric);
    printf("当前版本适应度: %.4f (大小: %zu)\n", state->fitness, state->code_size);
    
    double best_fitness = state->fitness;
    char *best_code = NULL;
    
    for (int i = 0; i < state->params.population_size; i++) {
        printf("生成最小化变异体 %d/%d...\n", i+1, state->params.population_size);
        
        char *mutated = mutate_code_minimal(state->self_code, state->code_size, state->params);
        if (!mutated) continue;
        
        double fitness = evaluate_fitness(mutated, strlen(mutated), state->params.fitness_metric);
        printf("变异体 %d 适应度: %.4f (大小: %zu)\n", i+1, fitness, strlen(mutated));
        
        if (fitness > best_fitness) {
            best_fitness = fitness;
            if (best_code) free(best_code);
            best_code = mutated;
        } else {
            free(mutated);
        }
    }
    
    if (best_code) {
        save_if_better(best_code, best_fitness);
        free(best_code);
    } else {
        printf("未找到更好的变异体，保持当前版本\n");
    }
}