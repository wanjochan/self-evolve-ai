/**
 * evolver0_simple.c - 简化版自进化内核
 * 暂时去掉TinyCC依赖，使用系统gcc进行编译
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

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
static int compile_and_run(const char *code, const char *temp_filename);
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
    state.params.population_size = 3;   // 每代3个变种(减少以提高速度)
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
    printf("evolver%d_simple - 简化版自进化内核\n", VERSION);
    printf("用法: %s [选项]\n", argv[0]);
    printf("选项:\n");
    printf("  -e              进入进化模式\n");
    printf("  -m <rate>       设置变异率 (0-100, 默认5)\n");
    printf("  -g <gens>       设置最大代数 (默认100)\n");
    printf("  -p <pop>        设置种群大小 (默认3)\n");
    printf("  -f <metric>     设置适应度度量 (s=大小, p=性能, c=能力)\n");
    
    return 0;
}

/**
 * 读取自身源代码
 */
static char* read_self() {
    // 获取当前可执行文件的路径
    char exe_path[1024] = {0};
    sprintf(exe_path, "evolver%d_simple.c", VERSION);
    
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
 * 使用系统gcc编译并运行代码
 */
static int compile_and_run(const char *code, const char *temp_filename) {
    char temp_source[256];
    char temp_binary[256];
    char compile_cmd[512];
    char run_cmd[256];
    
    sprintf(temp_source, "/tmp/%s.c", temp_filename);
    sprintf(temp_binary, "/tmp/%s", temp_filename);
    
    // 写入临时源文件
    FILE *f = fopen(temp_source, "w");
    if (!f) {
        fprintf(stderr, "无法创建临时文件: %s\n", temp_source);
        return -1;
    }
    
    fwrite(code, 1, strlen(code), f);
    fclose(f);
    
    // 编译
    sprintf(compile_cmd, "gcc -o %s %s 2>/dev/null", temp_binary, temp_source);
    int compile_result = system(compile_cmd);
    
    if (compile_result != 0) {
        // 清理临时文件
        unlink(temp_source);
        return -1;
    }
    
    // 运行
    sprintf(run_cmd, "%s -h 2>/dev/null", temp_binary);
    int run_result = system(run_cmd);
    
    // 清理临时文件
    unlink(temp_source);
    unlink(temp_binary);
    
    return (run_result == 0) ? 0 : -1;
}

/**
 * 对代码进行变异 - 改进版
 */
static char* mutate_code(const char *code, size_t code_size, EvolveParams params) {
    if (!code || code_size == 0) return NULL;
    
    // 创建代码副本
    char *mutated = (char*)malloc(code_size + 2048); // 更多额外空间
    if (!mutated) return NULL;
    
    strcpy(mutated, code);
    size_t mutated_size = code_size;
    
    // 改进的变异策略：多种变异类型
    int num_mutations = (code_size * params.mutation_rate) / 500; // 提高变异率
    if (num_mutations < 2) num_mutations = 2;
    
    for (int i = 0; i < num_mutations; i++) {
        int mutation_type = rand() % 5; // 5种变异类型
        
        switch (mutation_type) {
            case 0: // 修改注释内容
                {
                    char *comment_pos = strstr(mutated, "//");
                    if (comment_pos) {
                        char *line_end = strchr(comment_pos, '\n');
                        if (line_end) {
                            // 在注释后添加随机文本
                            char addition[64];
                            sprintf(addition, " [变异%d]", rand() % 1000);
                            
                            int insert_pos = line_end - mutated;
                            int add_len = strlen(addition);
                            
                            // 移动后续内容
                            memmove(mutated + insert_pos + add_len, 
                                    mutated + insert_pos, 
                                    mutated_size - insert_pos + 1);
                            
                            // 插入新内容
                            memcpy(mutated + insert_pos, addition, add_len);
                            mutated_size += add_len;
                        }
                    }
                }
                break;
                
            case 1: // 修改printf输出
                {
                    char *printf_pos = strstr(mutated, "printf(");
                    if (printf_pos) {
                        char *quote_start = strchr(printf_pos, '"');
                        if (quote_start) {
                            char *quote_end = strchr(quote_start + 1, '"');
                            if (quote_end && quote_end > quote_start + 10) {
                                // 在字符串中间插入随机字符
                                int pos = (quote_start - mutated) + 5 + (rand() % 5);
                                if (pos < (quote_end - mutated) && mutated[pos] != '\\') {
                                    mutated[pos] = 'A' + (rand() % 26);
                                }
                            }
                        }
                    }
                }
                break;
                
            case 2: // 修改数值常量
                {
                    char *number_patterns[] = {"1024", "100", "64", "256", "512"};
                    for (int j = 0; j < 5; j++) {
                        char *num_pos = strstr(mutated, number_patterns[j]);
                        if (num_pos && (rand() % 3 == 0)) {
                            int old_val = atoi(number_patterns[j]);
                            int new_val = old_val + (rand() % 100) - 50; // ±50变化
                            if (new_val < 1) new_val = 1;
                            
                            char new_num_str[32];
                            sprintf(new_num_str, "%d", new_val);
                            
                            int old_len = strlen(number_patterns[j]);
                            int new_len = strlen(new_num_str);
                            
                            // 替换数字
                            memmove(num_pos + new_len, num_pos + old_len,
                                    mutated_size - (num_pos - mutated) - old_len + 1);
                            memcpy(num_pos, new_num_str, new_len);
                            mutated_size = mutated_size - old_len + new_len;
                            break;
                        }
                    }
                }
                break;
                
            case 3: // 添加新的调试输出
                {
                    char *func_start = strstr(mutated, "static void evolve_next_generation");
                    if (func_start) {
                        char *brace_pos = strchr(func_start, '{');
                        if (brace_pos) {
                            char debug_line[] = "\n    printf(\"[DEBUG] 进化调试信息\\n\");";
                            int insert_pos = (brace_pos - mutated) + 1;
                            int add_len = strlen(debug_line);
                            
                            // 插入调试行
                            memmove(mutated + insert_pos + add_len,
                                    mutated + insert_pos,
                                    mutated_size - insert_pos + 1);
                            memcpy(mutated + insert_pos, debug_line, add_len);
                            mutated_size += add_len;
                        }
                    }
                }
                break;
                
            case 4: // 修改变异参数
                {
                    char *param_pos = strstr(mutated, "params.mutation_rate = ");
                    if (param_pos) {
                        char *semicolon = strchr(param_pos, ';');
                        if (semicolon) {
                            char new_rate[32];
                            int new_val = 3 + (rand() % 10); // 3-12范围
                            sprintf(new_rate, "params.mutation_rate = %d", new_val);
                            
                            int old_len = semicolon - param_pos;
                            int new_len = strlen(new_rate);
                            
                            memmove(param_pos + new_len, semicolon,
                                    mutated_size - (semicolon - mutated) + 1);
                            memcpy(param_pos, new_rate, new_len);
                            mutated_size = mutated_size - old_len + new_len;
                        }
                    }
                }
                break;
        }
    }
    
    // 更新版本号
    char *version_pos = strstr(mutated, "#define VERSION");
    if (version_pos) {
        char version_str[64];
        sprintf(version_str, "#define VERSION %d", VERSION + 1);
        
        // 找到行尾
        char *line_end = strchr(version_pos, '\n');
        if (line_end) {
            int old_line_len = line_end - version_pos;
            int new_line_len = strlen(version_str);
            
            // 移动后续内容
            memmove(version_pos + new_line_len, line_end, 
                    mutated_size - (line_end - mutated) + 1);
            
            // 插入新版本行
            memcpy(version_pos, version_str, new_line_len);
            mutated_size = mutated_size - old_line_len + new_line_len;
        }
    }
    
    mutated[mutated_size] = '\0';
    return mutated;
}

/**
 * 评估代码的适应度
 */
static double evaluate_fitness(const char *code, size_t code_size, char metric) {
    if (!code || code_size == 0) return 0.0;
    
    double fitness = 0.0;
    char temp_name[64];
    sprintf(temp_name, "evolver_test_%d", rand());
    
    switch (metric) {
        case 's': // 大小 (越小越好)
            fitness = 1000000.0 / (double)(code_size);
            break;
            
        case 'p': // 性能 (编译+运行时间，越快越好)
            {
                clock_t start = clock();
                int result = compile_and_run(code, temp_name);
                clock_t end = clock();
                
                if (result >= 0) {
                    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
                    fitness = 1.0 / (time_taken + 0.001); // 避免除零
                } else {
                    fitness = 0.0; // 编译失败
                }
            }
            break;
            
        case 'c': // 能力 (能够成功编译并返回正确结果)
            {
                int result = compile_and_run(code, temp_name);
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
    sprintf(next_filename, "evolver%d_simple.c", VERSION + 1);
    
    // 写入文件
    FILE *f = fopen(next_filename, "w");
    if (f) {
        fwrite(code, 1, strlen(code), f);
        fclose(f);
        printf("已保存新一代: %s (适应度: %.4f)\n", next_filename, fitness);
        
        // 尝试编译新版本
        char compile_cmd[256];
        sprintf(compile_cmd, "gcc -o evolver%d_simple %s", VERSION + 1, next_filename);
        if (system(compile_cmd) == 0) {
            printf("新版本编译成功！\n");
        } else {
            printf("新版本编译失败，但源码已保存\n");
        }
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
    
    // 评估当前版本的适应度
    state->fitness = evaluate_fitness(state->self_code, state->code_size, state->params.fitness_metric);
    printf("当前版本适应度: %.4f\n", state->fitness);
    
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