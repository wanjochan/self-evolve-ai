/**
 * evolver5_minimal.c - 自进化内核 v5
 * 优化目标：减少依赖，提高代码效率
 */

// 最小化标准库依赖
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define VERSION 5
#define MAX_CODE_SIZE 1048576

typedef struct {
    int mr;  // mutation_rate
    int ps;  // population_size
    char fm; // fitness_metric
} EvolveParams;

typedef struct {
    char *code;
    size_t size;
    int gen;
    double fit;
    EvolveParams p;
} EvolveState;

// 函数声明
static char* read_self();
static size_t get_file_size(FILE *f);
static int compile_test(const char *code, const char *fname);
static char* mutate_code(const char *code, size_t sz, EvolveParams p);
static double eval_fit(const char *code, size_t sz, char m);
static void save_best(const char *code, double fit);
static void evolve(EvolveState *s);

int main(int argc, char *argv[]) {
    srand(time(0));
    
    EvolveState s;
    s.gen = VERSION;
    s.fit = 0.0;
    s.p.mr = 8;  // 降低变异率
    s.p.ps = 4;  // 减少种群大小
    s.p.fm = 's';
    
    if (argc > 1 && !strcmp(argv[1], "-e")) {
        printf("进化模式 v%d\n", s.gen);
        s.code = read_self();
        if (!s.code) return 1;
        
        s.size = strlen(s.code);
        evolve(&s);
        free(s.code);
    } else {
        printf("使用 -e 启动进化模式\n");
    }
    
    return 0;
}

static char* read_self() {
    FILE *f = fopen("evolver4_minimal.c", "r");
    if (!f) return NULL;
    
    size_t sz = get_file_size(f);
    char *buf = (char*)malloc(sz + 1);
    if (!buf) { fclose(f); return NULL; }
    
    fread(buf, 1, sz, f);
    buf[sz] = '\0';
    fclose(f);
    return buf;
}

static size_t get_file_size(FILE *f) {
    fseek(f, 0, SEEK_END);
    size_t sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    return sz;
}

static int compile_test(const char *code, const char *fname) {
    // 确保代码中包含 main 函数
    if (!strstr(code, "int main")) {
        return 0;
    }
    
    // 将代码写入临时文件
    FILE *f = fopen("/tmp/evolver_temp.c", "w");
    if (!f) return 0;
    fprintf(f, "%s", code);
    fclose(f);
    
    // 编译测试
    int r = system("gcc -o /tmp/evolver_test /tmp/evolver_temp.c -Werror -Wfatal-errors 2>/dev/null");
    
    // 清理
    unlink("/tmp/evolver_temp.c");
    unlink("/tmp/evolver_test");
    
    return r == 0;
}

static char* mutate_code(const char *code, size_t sz, EvolveParams p) {
    if (!code || !sz) return NULL;
    
    char *m = (char*)malloc(sz + 256);
    if (!m) return NULL;
    
    strcpy(m, code);
    size_t ms = sz;
    
    // 变异策略
    int mutations = 1 + (rand() % 3);
    
    for (int i = 0; i < mutations; i++) {
        int type = rand() % 6;
        
        switch (type) {
            case 0: // 移除多余空格
                {
                    char *ptr = strstr(m, "  ");
                    if (ptr) {
                        memmove(ptr, ptr + 1, ms - (ptr - m));
                        ms--;
                    }
                }
                break;
                
            case 1: // 简化变量名
                {
                    char *vars[] = {"mutated", "code_size", "params", "temp"};
                    for (int j = 0; j < 4; j++) {
                        char *pos = strstr(m, vars[j]);
                        if (pos && (rand() % 3 == 0)) {
                            int len = strlen(vars[j]);
                            pos[1] = 'x';
                            memmove(pos + 2, pos + len, ms - (pos - m) - len + 1);
                            ms -= (len - 2);
                            break;
                        }
                    }
                }
                break;
                
            case 2: // 简化数字常量
                {
                    int num = 1 << (rand() % 10 + 5);
                    char old[16], new[16];
                    snprintf(old, sizeof(old), "%d", num);
                    snprintf(new, sizeof(new), "0x%x", num);
                    
                    char *pos = strstr(m, old);
                    if (pos && (rand() % 2 == 0)) {
                        int old_len = strlen(old);
                        int new_len = strlen(new);
                        memmove(pos + new_len, pos + old_len, ms - (pos - m) - old_len + 1);
                        memcpy(pos, new, new_len);
                        ms = ms - old_len + new_len;
                    }
                }
                break;
                
            case 3: // 移除空行
                {
                    char *nl = strstr(m, "\n\n\n");
                    if (nl) {
                        memmove(nl + 1, nl + 2, ms - (nl - m) - 1);
                        ms--;
                    }
                }
                break;
                
            case 4: // 简化条件判断
                {
                    char *if_pos = strstr(m, "if (");
                    if (if_pos) {
                        char *end = strchr(if_pos, ')');
                        if (end && (end - if_pos < 20)) {
                            char *then = strstr(end, "){");
                            if (then && (rand() % 2 == 0)) {
                                memmove(if_pos + 2, if_pos + 3, ms - (if_pos - m) - 2);
                                ms--;
                            }
                        }
                    }
                }
                break;
                
            case 5: // 简化格式化字符串
                {
                    char *fmt = strstr(m, "%s");
                    if (fmt && (rand() % 2 == 0)) {
                        memmove(fmt, fmt + 1, ms - (fmt - m));
                        ms--;
                    }
                }
                break;
        }
    }
    
    // 更新版本号
    char *ver = strstr(m, "#define VERSION");
    if (ver) {
        char new_ver[32];
        snprintf(new_ver, sizeof(new_ver), "#define VERSION %d", VERSION + 1);
        char *end = strchr(ver, '\n');
        if (end) {
            int old_len = end - ver;
            int new_len = strlen(new_ver);
            memmove(ver + new_len, end, ms - (end - m) + 1);
            memcpy(ver, new_ver, new_len);
            ms = ms - old_len + new_len;
        }
    }
    
    m[ms] = '\0';
    return m;
}

static double eval_fit(const char *code, size_t sz, char m) {
    if (m == 's') return -(double)sz; // 目标：最小化代码大小
    return 0.0;
}

static void save_best(const char *code, double fit) {
    char fname[64];
    snprintf(fname, sizeof(fname), "evolver%d_minimal.c", VERSION + 1);
    
    FILE *f = fopen(fname, "w");
    if (f) {
        fwrite(code, 1, strlen(code), f);
        fclose(f);
        printf("保存新版本: %s (适应度: %.1f)\n", fname, -fit);
    }
}

static void evolve(EvolveState *s) {
    printf("开始进化...\n");
    
    // 评估当前代码
    double best_fit = eval_fit(s->code, s->size, s->p.fm);
    printf("当前适应度: %.1f\n", -best_fit);
    
    // 进化循环
    for (int i = 0; i < s->p.ps; i++) {
        char *mutant = mutate_code(s->code, s->size, s->p);
        if (!mutant) continue;
        
        // 确保变异后的代码仍然包含必要的结构
        if (strstr(mutant, "int main") && compile_test(mutant, "temp_evolver")) {
            double fit = eval_fit(mutant, strlen(mutant), s->p.fm);
            if (fit > best_fit) {
                best_fit = fit;
                save_best(mutant, fit);
                break;
            }
        }
        
        free(mutant);
    }
}
