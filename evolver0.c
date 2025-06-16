/**
 * evolver0.c - 自解释进化内核
 * 基于TinyCC的自我修改和进化系统的初始版本
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <libtcc.h>

// 版本标识
#define VERSION 0
#define MAX_CODE_SIZE 1024*1024  // 1MB代码缓冲区
#define MAX_OUTPUT_SIZE 1024*1024  // 1MB输出缓冲区

// 进化参数
typedef struct {
    int mutation_rate;     // 变异率 (0-100)
    int max_generations;   // 最大代数
    int population_size;   // 种群大小
    char fitness_metric;   // 适应度度量方式 ('s'=大小, 'p'=性能, 'c'=能力)
} EvolveParams;

// 全局状态
typedef struct {
    char *self_code;       // 自身源代码
    size_t code_size;      // 源代码大小
    int generation;        // 当前代数
    double fitness;        // 当前适应度
    EvolveParams params;   // 进化参数
} EvolveState;

// 函数原型
static char* read_self();
static size_t get_file_size(FILE *f);
static int compile_and_run(const char *code);
static char* mutate_code(const char *code, size_t code_size, EvolveParams params);
static double evaluate_fitness(const char *code, size_t code_size, char metric);
static void save_if_better(const char *code, double fitness);
static void evolve_next_generation(EvolveState *state);

/**
 * 主函数 - 进化循环的入口点
 */
int main(int argc, char *argv[]) {
    srand(time(NULL));
    
    // 初始化进化状态
    EvolveState state;
    state.generation = VERSION;
    state.fitness = 0.0;
    state.params.mutation_rate = 5;    // 5%变异率
    state.params.max_generations = 100; // 最多100代
    state.params.population_size = 5;   // 每代5个变种
    state.params.fitness_metric = 's';  // 默认以大小为适应度
    
    // 解析命令行参数
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-m") == 0 && i+1 < argc) {
            state.params.mutation_rate = atoi(argv[i+1]);
            i++;
        } else if (strcmp(argv[i], "-g") == 0 && i+1 < argc) {
            state.params.max_generations = atoi(argv[i+1]);
            i++;
        } else if (strcmp(argv[i], "-p") == 0 && i+1 < argc) {
            state.params.population_size = atoi(argv[i+1]);
            i++;
        } else if (strcmp(argv[i], "-f") == 0 && i+1 < argc) {
            state.params.fitness_metric = argv[i+1][0];
            i++;
        } else if (strcmp(argv[i], "-e") == 0) {
            // 进入进化模式
            printf("启动进化模式，当前代数: %d\n", state.generation);
            
            // 读取自身代码
            state.self_code = read_self();
            if (!state.self_code) {
                fprintf(stderr, "无法读取自身代码\n");
                return 1;
            }
            state.code_size = strlen(state.self_code);
            
            // 进化下一代
            evolve_next_generation(&state);
            
            free(state.self_code);
            return 0;
        }
    }
    
    // 默认模式：打印帮助信息
    printf("evolver%d - 自解释进化内核\n", VERSION);
    printf("用法: %s [选项]\n", argv[0]);
    printf("选项:\n");
    printf("  -e              进入进化模式\n");
    printf("  -m <rate>       设置变异率 (0-100, 默认5)\n");
    printf("  -g <gens>       设置最大代数 (默认100)\n");
    printf("  -p <pop>        设置种群大小 (默认5)\n");
    printf("  -f <metric>     设置适应度度量 (s=大小, p=性能, c=能力)\n");
    
    return 0;
}

/**
 * 读取自身源代码
 */
static char* read_self() {
    // 获取当前可执行文件的路径
    char exe_path[1024] = {0};
    sprintf(exe_path, "evolver%d.c", VERSION);
    
    FILE *f = fopen(exe_path, "rb");
    if (!f) {
        fprintf(stderr, "无法打开源文件: %s\n", exe_path);
        return NULL;
    }
    
    // 获取文件大小
    size_t size = get_file_size(f);
    if (size == 0 || size > MAX_CODE_SIZE) {
        fprintf(stderr, "无效的文件大小: %zu\n", size);
        fclose(f);
        return NULL;
    }
    
    // 分配内存并读取文件
    char *code = (char*)malloc(size + 1);
    if (!code) {
        fprintf(stderr, "内存分配失败\n");
        fclose(f);
        return NULL;
    }
    
    size_t read_size = fread(code, 1, size, f);
    fclose(f);
    
    if (read_size != size) {
        fprintf(stderr, "读取失败: %zu != %zu\n", read_size, size);
        free(code);
        return NULL;
    }
    
    code[size] = '\0';
    return code;
}

/**
 * 获取文件大小
 */
static size_t get_file_size(FILE *f) {
    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    return size;
}

/**
 * 使用TinyCC编译并运行代码
 */
static int compile_and_run(const char *code) {
    TCCState *s;
    int ret;
    
    // 创建TCC编译状态
    s = tcc_new();
    if (!s) {
        fprintf(stderr, "无法创建TCC状态\n");
        return -1;
    }
    
    // 配置TCC
    tcc_set_output_type(s, TCC_OUTPUT_MEMORY);
    
    // 添加标准库
    tcc_add_library(s, "m"); // 数学库
    
    // 编译代码
    if (tcc_compile_string(s, code) == -1) {
        fprintf(stderr, "编译错误\n");
        tcc_delete(s);
        return -1;
    }
    
    // 重定位代码
    ret = tcc_relocate(s, TCC_RELOCATE_AUTO);
    if (ret == -1) {
        fprintf(stderr, "重定位错误\n");
        tcc_delete(s);
        return -1;
    }
    
    // 获取main函数指针
    int (*func)(int, char **) = (int (*)(int, char **))tcc_get_symbol(s, "main");
    if (!func) {
        fprintf(stderr, "找不到main函数\n");
        tcc_delete(s);
        return -1;
    }
    
    // 准备参数
    char *args[] = {"evolver_test", "-e", NULL};
    
    // 运行代码
    ret = func(2, args);
    
    // 清理
    tcc_delete(s);
    return ret;
}

/**
 * 对代码进行变异
 */
static char* mutate_code(const char *code, size_t code_size, EvolveParams params) {
    if (!code || code_size == 0) return NULL;
    
    // 创建代码副本
    char *mutated = (char*)malloc(code_size + 1024); // 额外空间用于插入
    if (!mutated) return NULL;
    
    strcpy(mutated, code);
    size_t mutated_size = code_size;
    
    // 简单变异策略：随机修改、插入或删除
    int num_mutations = (code_size * params.mutation_rate) / 100;
    if (num_mutations < 1) num_mutations = 1;
    
    for (int i = 0; i < num_mutations; i++) {
        int mutation_type = rand() % 3; // 0=修改, 1=插入, 2=删除
        int position = rand() % mutated_size;
        
        // 避开关键区域（如版本标识）
        char *version_pos = strstr(mutated, "#define VERSION");
        if (version_pos) {
            int version_idx = version_pos - mutated;
            if (position >= version_idx && position < version_idx + 20) {
                position = (position + 30) % mutated_size;
            }
        }
        
        switch (mutation_type) {
            case 0: // 修改
                if (mutated[position] != '\0') {
                    char new_char = 32 + (rand() % 94); // ASCII可打印字符
                    mutated[position] = new_char;
                }
                break;
                
            case 1: // 插入
                {
                    char insert_str[32];
                    int insert_len = rand() % 10 + 1;
                    for (int j = 0; j < insert_len; j++) {
                        insert_str[j] = 32 + (rand() % 94);
                    }
                    insert_str[insert_len] = '\0';
                    
                    // 移动后面的内容
                    memmove(mutated + position + insert_len, 
                            mutated + position, 
                            mutated_size - position + 1);
                    
                    // 插入新内容
                    memcpy(mutated + position, insert_str, insert_len);
                    mutated_size += insert_len;
                }
                break;
                
            case 2: // 删除
                if (position < mutated_size - 1) {
                    int delete_len = rand() % 5 + 1;
                    if (delete_len > mutated_size - position)
                        delete_len = mutated_size - position;
                    
                    // 移动后面的内容
                    memmove(mutated + position,
                            mutated + position + delete_len,
                            mutated_size - position - delete_len + 1);
                    
                    mutated_size -= delete_len;
                }
                break;
        }
    }
    
    // 确保字符串结束
    mutated[mutated_size] = '\0';
    
    // 更新版本号
    char *version_pos = strstr(mutated, "#define VERSION");
    if (version_pos) {
        char version_str[64];
        sprintf(version_str, "#define VERSION %d", VERSION + 1);
        memcpy(version_pos, version_str, strlen(version_str));
    }
    
    return mutated;
}

/**
 * 评估代码的适应度
 */
static double evaluate_fitness(const char *code, size_t code_size, char metric) {
    if (!code || code_size == 0) return 0.0;
    
    double fitness = 0.0;
    
    switch (metric) {
        case 's': // 大小 (越小越好)
            fitness = 1000000.0 / (double)(code_size);
            break;
            
        case 'p': // 性能 (编译+运行时间，越快越好)
            {
                clock_t start = clock();
                int result = compile_and_run(code);
                clock_t end = clock();
                
                if (result >= 0) {
                    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
                    fitness = 1.0 / (time_taken + 0.001); // 避免除零
                }
            }
            break;
            
        case 'c': // 能力 (能够成功编译并返回正确结果)
            {
                int result = compile_and_run(code);
                fitness = (result == 0) ? 1.0 : 0.0;
            }
            break;
            
        default:
            fitness = 0.0;
    }
    
    return fitness;
}

/**
 * 如果新代码更好，则保存
 */
static void save_if_better(const char *code, double fitness) {
    // 创建下一代文件名
    char next_filename[64];
    sprintf(next_filename, "evolver%d.c", VERSION + 1);
    
    // 写入文件
    FILE *f = fopen(next_filename, "wb");
    if (f) {
        fwrite(code, 1, strlen(code), f);
        fclose(f);
        printf("已保存新一代: %s (适应度: %.4f)\n", next_filename, fitness);
    } else {
        fprintf(stderr, "无法保存新一代: %s\n", next_filename);
    }
}

/**
 * 进化下一代
 */
static void evolve_next_generation(EvolveState *state) {
    if (!state || !state->self_code) return;
    
    printf("开始进化第%d代...\n", state->generation + 1);
    
    double best_fitness = state->fitness;
    char *best_code = NULL;
    
    // 生成多个变异体并评估
    for (int i = 0; i < state->params.population_size; i++) {
        printf("生成变异体 %d/%d...\n", i+1, state->params.population_size);
        
        char *mutated = mutate_code(state->self_code, state->code_size, state->params);
        if (!mutated) continue;
        
        double fitness = evaluate_fitness(mutated, strlen(mutated), state->params.fitness_metric);
        printf("变异体 %d 适应度: %.4f\n", i+1, fitness);
        
        if (fitness > best_fitness) {
            best_fitness = fitness;
            if (best_code) free(best_code);
            best_code = mutated;
        } else {
            free(mutated);
        }
    }
    
    // 如果找到更好的变异体，保存它
    if (best_code) {
        save_if_better(best_code, best_fitness);
        free(best_code);
    } else {
        printf("未找到更好的变异体，保持当前版本\n");
    }
} 