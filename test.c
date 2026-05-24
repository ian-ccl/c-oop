#ifndef TEST_H
#define TEST_H 1
#define MY_PRINT printf("hello\n"); 

typedef struct intResult intResult; struct intResult{
    int value;
    int err;
};typedef struct vec4 vec4; struct vec4{
    float v0;float v1;float v2;float v3;};typedef struct math_Point math_Point; struct math_Point{
    float x;
    float y;
};int add(int* self, int x) {
    return *self + x;
}

int main() {

    math_Point p;

    int *ptr = malloc(sizeof(int));
    *ptr = 5;
    int *ptr2 = malloc(sizeof(int));
    *ptr2 = 1;
    
    

    int x = 5;
int y = add(&x,10);
int z = 999;

    vec4 v;

    MY_PRINT

    const char* s = "repeat(999, nope)";
    const char* s2 = "class test { int x; }";

    free(ptr2);free(ptr);

    return 0;
}
#endif